"""
cfrds - Pure Python implementation of the ColdFusion Remote Development Service (RDS) protocol.
This package is completely standalone and does NOT require libcfrds.so or any native C extension.
"""

from enum import IntEnum
import http.client
import os
import re
import sys
from typing import Optional, List, Dict, Any, Union, Tuple, Iterator


# Status codes (internal use — errors raise CFRDSError)
CFRDS_STATUS_OK = 0
CFRDS_STATUS_MEMORY_ERROR = 1
CFRDS_STATUS_PARAM_IS_NULL = 2
CFRDS_STATUS_SERVER_IS_NULL = 3
CFRDS_STATUS_INVALID_INPUT_PARAMETER = 4
CFRDS_STATUS_INDEX_OUT_OF_BOUNDS = 5
CFRDS_STATUS_COMMAND_FAILED = 6
CFRDS_STATUS_RESPONSE_ERROR = 7
CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND = 8
CFRDS_STATUS_DIR_ALREADY_EXISTS = 9
CFRDS_STATUS_SOCKET_HOST_NOT_FOUND = 10
CFRDS_STATUS_SOCKET_CREATION_FAILED = 11
CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED = 12
CFRDS_STATUS_WRITING_TO_SOCKET_FAILED = 13
CFRDS_STATUS_PARTIALLY_WRITE_TO_SOCKET = 14
CFRDS_STATUS_READING_FROM_SOCKET_FAILED = 15
CFRDS_STATUS_RESPONSE_TOO_LARGE = 16


# Debugger event types
class DebuggerType(IntEnum):
    CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT_SET = 0
    CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT = 1
    CFRDS_DEBUGGER_EVENT_TYPE_STEP = 2
    CFRDS_DEBUGGER_EVENT_UNKNOWN = 3


CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT_SET = DebuggerType.CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT_SET
CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT = DebuggerType.CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT
CFRDS_DEBUGGER_EVENT_TYPE_STEP = DebuggerType.CFRDS_DEBUGGER_EVENT_TYPE_STEP
CFRDS_DEBUGGER_EVENT_UNKNOWN = DebuggerType.CFRDS_DEBUGGER_EVENT_UNKNOWN


class CFRDSError(Exception):
    """Exception raised for ColdFusion RDS protocol errors."""
    def __init__(self, message: str = ""):
        super().__init__(message)

    @property
    def status(self) -> int:
        msg = str(self)
        if "is required" in msg or "PARAM_IS_NULL" in msg:
            return 2  # CFRDS_STATUS_PARAM_IS_NULL
        if "Invalid total items count" in msg or "RESPONSE_ERROR" in msg:
            return 7  # CFRDS_STATUS_RESPONSE_ERROR
        if "Socket host not found" in msg:
            return 10  # CFRDS_STATUS_SOCKET_HOST_NOT_FOUND
        if "Socket creation failed" in msg:
            return 11  # CFRDS_STATUS_SOCKET_CREATION_FAILED
        if "Connection to server failed" in msg or "Connection timed out" in msg:
            return 12  # CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED
        if "COMMAND_FAILED" in msg:
            return 6  # CFRDS_STATUS_COMMAND_FAILED
        return 6  # CFRDS_STATUS_COMMAND_FAILED (default)


def _escape_xml(val: str) -> str:
    return (
        val.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
        .replace("'", "&apos;")
    )


def _parse_wddx_node(elem: Any) -> Any:
    tag = elem.tag
    if "}" in tag:
        tag = tag.split("}", 1)[1]
    tag = tag.lower()

    if tag == "null":
        return None
    elif tag == "boolean":
        return elem.attrib.get("value") == "true"
    elif tag == "number":
        text = elem.text or "0"
        try:
            return float(text) if "." in text or "e" in text.lower() else int(text)
        except ValueError:
            try:
                return float(text)
            except ValueError:
                return text
    elif tag == "string":
        return elem.text if elem.text is not None else ""
    elif tag == "array":
        items = []
        for child in elem:
            items.append(_parse_wddx_node(child))
        return items
    elif tag == "struct":
        obj = {}
        for var in elem:
            var_tag = var.tag
            if "}" in var_tag:
                var_tag = var_tag.split("}", 1)[1]
            if var_tag.lower() == "var":
                name = var.attrib.get("name")
                if name:
                    val = None
                    if len(var) > 0:
                        val = _parse_wddx_node(var[0])
                    obj[name] = val
        return obj
    else:
        if len(elem) > 0:
            return _parse_wddx_node(elem[0])
        return elem.text if elem.text is not None else None


def _wddx_deserialize(xml_str: str) -> Any:
    if not xml_str or not xml_str.strip():
        return None
    import xml.etree.ElementTree as ET
    try:
        root = ET.fromstring(xml_str)
        data_elem = None
        for child in root:
            tag = child.tag
            if "}" in tag:
                tag = tag.split("}", 1)[1]
            if tag.lower() == "data":
                data_elem = child
                break
        if data_elem is not None:
            if len(data_elem) > 0:
                return _parse_wddx_node(data_elem[0])
            return None
        return _parse_wddx_node(root)
    except Exception:
        return None

# Password Obfuscation (XOR with "4p0L@r1$")
_FILLUP_KEY = "4p0L@r1$".encode("utf-8")
_HEX_CHARS = "0123456789abcdef"

def _encode_password(password: str) -> str:
    if not password:
        return ""
    pwd_bytes = password.encode("utf-8")
    out = []
    for i, b in enumerate(pwd_bytes):
        enc = b ^ _FILLUP_KEY[i % len(_FILLUP_KEY)]
        out.append(_HEX_CHARS[(enc & 0xF0) >> 4])
        out.append(_HEX_CHARS[enc & 0x0F])
    return "".join(out)


# Response Parsing Utilities
def _parse_number(data: bytes, offset: List[int]) -> int:
    colon_pos = data.find(b":", offset[0])
    if colon_pos == -1:
        raise CFRDSError("Failed to parse number: missing ':' delimiter")
    try:
        val = int(data[offset[0]:colon_pos].decode("utf-8"))
    except ValueError:
        raise CFRDSError("Failed to parse number: non-integer value")
    offset[0] = colon_pos + 1
    return val


def _parse_string(data: bytes, offset: List[int]) -> str:
    size = _parse_number(data, offset)
    if size < 0 or offset[0] + size > len(data):
        raise CFRDSError("Failed to parse string: bounds error")
    raw_str = data[offset[0]:offset[0] + size].decode("utf-8", errors="replace")
    offset[0] += size
    return raw_str


def _parse_bytearray(data: bytes, offset: List[int]) -> bytes:
    size = _parse_number(data, offset)
    if size < 0 or offset[0] + size > len(data):
        raise CFRDSError("Failed to parse bytearray: bounds error")
    raw_bytes = data[offset[0]:offset[0] + size]
    offset[0] += size
    return raw_bytes


def _parse_string_list_item(s: str) -> List[str]:
    """Parses a comma-separated list of items, respecting double quotes."""
    items: List[str] = []
    i = 0
    n = len(s)
    while i < n:
        if s[i] == '"':
            end_q = s.find('"', i + 1)
            if end_q == -1:
                items.append(s[i + 1:])
                break
            items.append(s[i + 1:end_q])
            i = end_q + 1
            if i < n and s[i] == ',':
                i += 1
        else:
            comma_pos = s.find(',', i)
            if comma_pos == -1:
                items.append(s[i:])
                break
            items.append(s[i:comma_pos])
            i = comma_pos + 1
    return items


def _safe_int(s: Optional[str]) -> int:
    if not s:
        return 0
    try:
        return int(s)
    except ValueError:
        return 0



# Struct Result Container Objects for C-API Compatibility
class DirListing:
    def __init__(self, items: List[Dict[str, Any]]):
        self.items = items

    def __len__(self) -> int:
        return len(self.items)

    def __getitem__(self, idx: Any) -> Any:
        return self.items[idx]

    def __iter__(self) -> Iterator[Dict[str, Any]]:
        return iter(self.items)

    def to_list(self) -> List[Dict[str, Any]]:
        return list(self.items)

    def __repr__(self) -> str:
        return f"DirListing({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, DirListing):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False

class FileContent:
    def __init__(self, data: bytes, modified: str, permission: str):
        self.data = data
        self.size = len(data)
        self.modified = modified
        self.permission = permission

    def to_dict(self) -> Dict[str, Any]:
        return {
            "data": self.data,
            "size": self.size,
            "modified": self.modified,
            "permission": self.permission,
        }

    def __getitem__(self, key: Any) -> Any:
        if isinstance(key, str):
            return getattr(self, key)
        keys = ["data", "size", "modified", "permission"]
        return getattr(self, keys[key])

    def keys(self) -> List[str]:
        return ["data", "size", "modified", "permission"]

    def values(self) -> List[Any]:
        return [self.data, self.size, self.modified, self.permission]

    def items(self) -> List[Tuple[str, Any]]:
        return list(self.to_dict().items())

    def __iter__(self) -> Iterator[str]:
        return iter(self.keys())

    def __repr__(self) -> str:
        return f"FileContent({self.to_dict()!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, FileContent):
            return self.to_dict() == other.to_dict()
        if isinstance(other, dict):
            return self.to_dict() == other
        return False


class DSNInfo:
    def __init__(self, names: List[str]):
        self.names = names

    def __len__(self) -> int:
        return len(self.names)

    def __getitem__(self, idx: Any) -> Any:
        return self.names[idx]

    def __iter__(self) -> Iterator[str]:
        return iter(self.names)

    def to_list(self) -> List[str]:
        return list(self.names)

    def __repr__(self) -> str:
        return f"DSNInfo({self.names!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, DSNInfo):
            return self.names == other.names
        if isinstance(other, list):
            return self.names == other
        return False


class TableInfo:
    def __init__(self, items: List[Dict[str, Optional[str]]]):
        self.items = items

    def __len__(self) -> int:
        return len(self.items)

    def __getitem__(self, idx: Any) -> Any:
        return self.items[idx]

    def __iter__(self) -> Iterator[Dict[str, Optional[str]]]:
        return iter(self.items)

    def to_list(self) -> List[Dict[str, Optional[str]]]:
        return list(self.items)

    def __repr__(self) -> str:
        return f"TableInfo({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, TableInfo):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False


class ColumnInfo:
    def __init__(self, items: List[Dict[str, Any]]):
        self.items = items

    def __len__(self) -> int:
        return len(self.items)

    def __getitem__(self, idx: Any) -> Any:
        return self.items[idx]

    def __iter__(self) -> Iterator[Dict[str, Any]]:
        return iter(self.items)

    def to_list(self) -> List[Dict[str, Any]]:
        return list(self.items)

    def __repr__(self) -> str:
        return f"ColumnInfo({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, ColumnInfo):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False


class PrimaryKeyInfo:
    def __init__(self, items: List[Dict[str, Any]]):
        self.items = items

    def __len__(self) -> int:
        return len(self.items)

    def __getitem__(self, idx: Any) -> Any:
        return self.items[idx]

    def __iter__(self) -> Iterator[Dict[str, Any]]:
        return iter(self.items)

    def to_list(self) -> List[Dict[str, Any]]:
        return list(self.items)

    def __repr__(self) -> str:
        return f"PrimaryKeyInfo({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, PrimaryKeyInfo):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False


class ForeignKeyInfo:
    def __init__(self, items: List[Dict[str, Any]]):
        self.items = items

    def __len__(self) -> int:
        return len(self.items)

    def __getitem__(self, idx: Any) -> Any:
        return self.items[idx]

    def __iter__(self) -> Iterator[Dict[str, Any]]:
        return iter(self.items)

    def to_list(self) -> List[Dict[str, Any]]:
        return list(self.items)

    def __repr__(self) -> str:
        return f"ForeignKeyInfo({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, ForeignKeyInfo):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False


class ImportedKeyInfo:
    def __init__(self, items: List[Dict[str, Any]]):
        self.items = items

    def __len__(self) -> int:
        return len(self.items)

    def __getitem__(self, idx: Any) -> Any:
        return self.items[idx]

    def __iter__(self) -> Iterator[Dict[str, Any]]:
        return iter(self.items)

    def to_list(self) -> List[Dict[str, Any]]:
        return list(self.items)

    def __repr__(self) -> str:
        return f"ImportedKeyInfo({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, ImportedKeyInfo):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False


class ExportedKeyInfo:
    def __init__(self, items: List[Dict[str, Any]]):
        self.items = items

    def __len__(self) -> int:
        return len(self.items)

    def __getitem__(self, idx: Any) -> Any:
        return self.items[idx]

    def __iter__(self) -> Iterator[Dict[str, Any]]:
        return iter(self.items)

    def to_list(self) -> List[Dict[str, Any]]:
        return list(self.items)

    def __repr__(self) -> str:
        return f"ExportedKeyInfo({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, ExportedKeyInfo):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False


class ResultSet:
    def __init__(self, columns: int, rows: int, names: List[str], values: List[List[Optional[str]]]):
        self.columns = columns
        self.rows = rows
        self.names = names
        self.values = values

    def to_dict(self) -> Dict[str, Any]:
        return {
            "columns": self.columns,
            "rows": self.rows,
            "names": self.names,
            "values": self.values,
        }

    def __len__(self) -> int:
        return self.rows

    def __getitem__(self, key: Any) -> Any:
        if isinstance(key, str):
            return self.to_dict()[key]
        return self.values[key]

    def keys(self) -> List[str]:
        return ["columns", "rows", "names", "values"]

    def __iter__(self) -> Iterator[List[Optional[str]]]:
        return iter(self.values)

    def __repr__(self) -> str:
        return f"ResultSet({self.to_dict()!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, ResultSet):
            return self.to_dict() == other.to_dict()
        if isinstance(other, dict):
            return self.to_dict() == other
        return False


class SQLMetadata:
    def __init__(self, items: List[Dict[str, Optional[str]]]):
        self.items = items

    def __len__(self) -> int:
        return len(self.items)

    def __getitem__(self, idx: Any) -> Any:
        return self.items[idx]

    def __iter__(self) -> Iterator[Dict[str, Optional[str]]]:
        return iter(self.items)

    def to_list(self) -> List[Dict[str, Optional[str]]]:
        return list(self.items)

    def __repr__(self) -> str:
        return f"SQLMetadata({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, SQLMetadata):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False


class SupportedCommands:
    def __init__(self, commands: List[str]):
        self.commands = commands

    def __len__(self) -> int:
        return len(self.commands)

    def __getitem__(self, idx: Any) -> Any:
        return self.commands[idx]

    def __iter__(self) -> Iterator[str]:
        return iter(self.commands)

    def to_list(self) -> List[str]:
        return list(self.commands)

    def __repr__(self) -> str:
        return f"SupportedCommands({self.commands!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, SupportedCommands):
            return self.commands == other.commands
        if isinstance(other, list):
            return self.commands == other
        return False


class DebuggerEvent:
    def __init__(self, event_type: int, data: Dict[str, Any]):
        self.event_type = event_type
        self.data = data

    def to_dict(self) -> Dict[str, Any]:
        return {"event_type": self.event_type, "type": self.event_type, "data": self.data}

    def __getitem__(self, key: Any) -> Any:
        if isinstance(key, str):
            if key in ("event_type", "type"):
                return self.event_type
            if key == "data":
                return self.data
            return self.data.get(key)
        keys = ["event_type", "data"]
        return getattr(self, keys[key])

    def keys(self) -> List[str]:
        return ["event_type", "type", "data"]

    def __iter__(self) -> Iterator[str]:
        return iter(self.keys())

    def __repr__(self) -> str:
        return f"DebuggerEvent({self.to_dict()!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, DebuggerEvent):
            return self.to_dict() == other.to_dict()
        if isinstance(other, dict):
            return self.to_dict() == other
        return False


class SecurityAnalyzerResult:
    def __init__(self, data: Dict[str, Any]):
        self.data = data

    def to_dict(self) -> Dict[str, Any]:
        return dict(self.data)

    def __getitem__(self, key: str) -> Any:
        return self.data[key]

    def keys(self) -> Any:
        return self.data.keys()

    def values(self) -> Any:
        return self.data.values()

    def items(self) -> Any:
        return self.data.items()

    def __iter__(self) -> Iterator[str]:
        return iter(self.data)

    def __repr__(self) -> str:
        return f"SecurityAnalyzerResult({self.data!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, SecurityAnalyzerResult):
            return self.data == other.data
        if isinstance(other, dict):
            return self.data == other
        return False


class CustomTagPaths:
    def __init__(self, paths: List[str]):
        self.paths = paths

    def __len__(self) -> int:
        return len(self.paths)

    def __getitem__(self, idx: Any) -> Any:
        return self.paths[idx]

    def __iter__(self) -> Iterator[str]:
        return iter(self.paths)

    def to_list(self) -> List[str]:
        return list(self.paths)

    def __repr__(self) -> str:
        return f"CustomTagPaths({self.paths!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, CustomTagPaths):
            return self.paths == other.paths
        if isinstance(other, list):
            return self.paths == other
        return False


class Mappings:
    def __init__(self, keys_or_mappings: Any, values_list: Optional[List[str]] = None, mappings_dict: Optional[Dict[str, str]] = None):
        if isinstance(keys_or_mappings, dict) and values_list is None:
            self.mappings = keys_or_mappings
            self.keys_list = list(keys_or_mappings.keys())
            self.values_list = list(keys_or_mappings.values())
        else:
            self.keys_list = list(keys_or_mappings)
            self.values_list = list(values_list) if values_list is not None else []
            if mappings_dict is not None:
                self.mappings = mappings_dict
            else:
                self.mappings = {}
                for k, v in zip(self.keys_list, self.values_list):
                    self.mappings[k] = v

    @property
    def keys(self) -> Any:
        return self.keys_list

    @property
    def values(self) -> Any:
        return self.values_list

    def __len__(self) -> int:
        return len(self.keys_list)

    def __getitem__(self, key: Any) -> Any:
        if isinstance(key, str):
            return self.mappings[key]
        return (self.keys_list[key], self.values_list[key])

    def items(self) -> Any:
        return self.mappings.items()

    def __iter__(self) -> Iterator[str]:
        return iter(self.mappings)

    def to_dict(self) -> Dict[str, str]:
        return dict(self.mappings)

    def __repr__(self) -> str:
        return f"Mappings({self.mappings!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, Mappings):
            return self.mappings == other.mappings
        if isinstance(other, dict):
            return self.mappings == other
        return False


# High-Level Server API
class Server:
    def __init__(self, hostname: str = "127.0.0.1", port: int = 8500, username: str = "admin", password: str = ""):
        self.host = hostname
        self.port = port
        self.username = username
        self.orig_password = password
        self.encoded_password = _encode_password(password) if password else ""
        self._conn: Optional[http.client.HTTPConnection] = None

    def close(self) -> None:
        if self._conn is not None:
            try:
                self._conn.close()
            except Exception:
                pass
            self._conn = None
        self.orig_password = ""
        self.encoded_password = ""
        self.username = ""
        self.host = ""
        self.port = 0

    def __enter__(self) -> "Server":
        return self

    def __exit__(self, exc_type: Any, exc_val: Any, exc_tb: Any) -> None:
        self.close()

    def get_host(self) -> str:
        return self.host

    def get_port(self) -> int:
        return self.port

    def get_username(self) -> str:
        return self.username

    def get_password(self) -> str:
        return self.orig_password

    def _send_rds_command(self, command: str, args: List[Union[str, bytes]]) -> bytes:
        all_items: List[bytes] = []
        for a in args:
            if isinstance(a, str):
                all_items.append(a.encode("utf-8"))
            else:
                all_items.append(a)

        if self.username is not None:
            all_items.append(self.username.encode("utf-8"))
        if self.orig_password is not None:
            all_items.append(self.encoded_password.encode("utf-8"))

        total_cnt = len(all_items)
        buf = bytearray()
        buf.extend(f"{total_cnt}:".encode("utf-8"))
        for item in all_items:
            buf.extend(f"STR:{len(item)}:".encode("utf-8"))
            buf.extend(item)

        payload = bytes(buf)

        path = f"/CFIDE/main/ide.cfm?CFSRV=IDE&ACTION={command}"
        headers = {
            "User-Agent": "Mozilla/3.0 (compatible; Macromedia RDS Client)",
            "Accept": "text/html, */*",
            "Accept-Encoding": "deflate",
            "Content-Type": "text/html",
            "Content-Length": str(len(payload)),
            "Connection": "keep-alive",
        }

        reused = self._conn is not None
        try:
            if self._conn is None:
                self._conn = http.client.HTTPConnection(self.host, self.port, timeout=30)
            conn = self._conn
            conn.request("POST", path, body=payload, headers=headers)
            resp = conn.getresponse()
            if resp.status != 200:
                raise CFRDSError(f"HTTP_RESPONSE_NOT_FOUND: HTTP {resp.status} {resp.reason}")
            body = resp.read()
        except Exception as e:
            if self._conn is not None:
                try:
                    self._conn.close()
                except Exception:
                    pass
                self._conn = None

            if reused and not isinstance(e, CFRDSError):
                return self._send_rds_command(command, args)

            if isinstance(e, CFRDSError):
                raise
            import socket
            import errno
            desc = "Connection to server failed"
            if isinstance(e, (socket.gaierror, socket.herror)):
                desc = "Socket host not found"
            elif isinstance(e, OSError) and e.errno in (errno.EACCES, errno.EPERM, errno.EADDRNOTAVAIL, errno.EMFILE, errno.ENFILE):
                desc = "Socket creation failed"
            msg = f"{desc}: {e}"
            raise CFRDSError(msg)

        try:
            offset = [0]
            err_code = _parse_number(body, offset)
        except Exception as e:
            raise CFRDSError(f"RESPONSE_ERROR: {str(e)}")

        if err_code < 0:
            err_msg = body[offset:].decode("utf-8", errors="replace")
            raise CFRDSError(f"COMMAND_FAILED: {err_msg}")

        return body

    # Browse Directory
    def browse_dir(self, path: str) -> List[Dict[str, Any]]:
        if path is None:
            raise CFRDSError("path is required")
        raw = self._send_rds_command("BROWSEDIR", [path, ""])
        offset = [0]
        total = _parse_number(raw, offset)
        if total < 0 or (total != 0 and total % 5 != 0):
            raise CFRDSError("Invalid total items count in browse_dir response")
        cnt = total // 5
        items: List[Dict[str, Any]] = []
        for _ in range(cnt):
            str_kind = _parse_string(raw, offset)
            filename = _parse_string(raw, offset)
            str_perms = _parse_string(raw, offset)
            str_size = _parse_string(raw, offset)
            str_ts = _parse_string(raw, offset)

            kind = 'F'
            if str_kind == "D:":
                kind = 'D'

            perms_num = int(str_perms) if str_perms else 0
            size = int(str_size) if str_size else 0

            modified = 0
            if str_ts and "," in str_ts:
                parts = str_ts.split(",")
                num1 = int(parts[0])
                num2 = int(parts[1])
                modified = num1 + (num2 << 32)
                modified = (modified // 10000) - 11644473600000

            # Map permission flags from C source's DirListing_item_get_permissions semantics:
            # - 0x01: Read-only (R) -> maps to FILE_ATTRIBUTE_READONLY (1)
            # - 0x02: Hidden (H)    -> maps to FILE_ATTRIBUTE_HIDDEN (2)
            # - 0x04: System (S)    -> maps to FILE_ATTRIBUTE_SYSTEM (4)
            # - 0x10: Directory (D) -> maps to FILE_ATTRIBUTE_DIRECTORY (16)
            # - 0x20: Archive (A)   -> maps to FILE_ATTRIBUTE_ARCHIVE (32)
            # - 0x80: Normal (N)    -> maps to FILE_ATTRIBUTE_NORMAL (128)
            perms_str = (
                ("D" if (perms_num & 0x10) or kind == 'D' else "-") +
                ("R" if perms_num & 0x01 else "-") +
                ("H" if perms_num & 0x02 else "-") +
                ("S" if perms_num & 0x04 else "-") +
                ("A" if perms_num & 0x20 else "-") +
                ("N" if perms_num & 0x80 else "-")
            )

            items.append({
                "kind": kind,
                "name": filename,
                "permissions": perms_str,
                "size": size,
                "modified": modified,
            })
        return items

    # File Operations
    def file_read(self, filepath: str) -> FileContent:
        if filepath is None:
            raise CFRDSError("filepath is required")
        raw = self._send_rds_command("FILEIO", [filepath, "READ", ""])
        offset = [0]
        total = _parse_number(raw, offset)
        data_bytes = _parse_bytearray(raw, offset)
        modified = _parse_string(raw, offset)
        permission = _parse_string(raw, offset)
        return FileContent(data_bytes, modified, permission)

    def file_write(self, filepath: str, content: Union[bytes, bytearray, str]) -> None:
        if filepath is None:
            raise CFRDSError("filepath is required")
        if content is None:
            raise CFRDSError("content is required")
        if isinstance(content, str):
            data_bytes = content.encode("utf-8")
        else:
            data_bytes = bytes(content)
        self._send_rds_command("FILEIO", [filepath, "WRITE", "", data_bytes])

    def file_rename(self, filepath_from: str, filepath_to: str) -> None:
        if filepath_from is None:
            raise CFRDSError("filepath_from is required")
        if filepath_to is None:
            raise CFRDSError("filepath_to is required")
        self._send_rds_command("FILEIO", [filepath_from, "RENAME", "", filepath_to])

    def file_remove(self, filepath: str) -> None:
        if filepath is None:
            raise CFRDSError("filepath is required")
        self._send_rds_command("FILEIO", [filepath, "REMOVE", "", "F"])

    def dir_remove(self, dirpath: str) -> None:
        if dirpath is None:
            raise CFRDSError("dirpath is required")
        self._send_rds_command("FILEIO", [dirpath, "REMOVE", "", "D"])

    def file_exists(self, pathname: str) -> bool:
        if pathname is None:
            raise CFRDSError("pathname is required")
        try:
            self._send_rds_command("FILEIO", [pathname, "EXISTENCE", "", ""])
            return True
        except CFRDSError as e:
            if "COMMAND_FAILED" in str(e):
                return False
            raise

    def dir_create(self, dirpath: str) -> None:
        if dirpath is None:
            raise CFRDSError("dirpath is required")
        self._send_rds_command("FILEIO", [dirpath, "CREATE", "", ""])

    def cf_root_dir(self) -> Optional[str]:
        raw = self._send_rds_command("FILEIO", ["", "CF_DIRECTORY"])
        offset = [0]
        _parse_number(raw, offset)
        path_str = _parse_string(raw, offset)
        return path_str

    # SQL Operations
    def sql_dsninfo(self) -> List[str]:
        raw = self._send_rds_command("DBFUNCS", ["", "DSNINFO"])
        offset = [0]
        cnt = _parse_number(raw, offset)
        dsns: List[str] = []
        for _ in range(cnt):
            item = _parse_string(raw, offset)
            fields = _parse_string_list_item(item)
            name = fields[0] if fields else item
            dsns.append(name)
        return dsns

    def sql_tableinfo(self, connection_name: str) -> List[Dict[str, Optional[str]]]:
        if connection_name is None:
            raise CFRDSError("connection_name is required")
        raw = self._send_rds_command("DBFUNCS", [connection_name, "TABLEINFO"])
        offset = [0]
        cnt = _parse_number(raw, offset)
        tables: List[Dict[str, Optional[str]]] = []
        for _ in range(cnt):
            item = _parse_string(raw, offset)
            fields = _parse_string_list_item(item)
            f1 = fields[0] if len(fields) > 0 else ""
            f2 = fields[1] if len(fields) > 1 else ""
            f3 = fields[2] if len(fields) > 2 else ""
            f4 = fields[3] if len(fields) > 3 else ""
            tables.append({"unknown": f1, "schema": f2, "name": f3, "type": f4})
        return tables

    def sql_columninfo(self, connection_name: str, table_name: str) -> List[Dict[str, Any]]:
        if connection_name is None:
            raise CFRDSError("connection_name is required")
        if table_name is None:
            raise CFRDSError("table_name is required")
        raw = self._send_rds_command("DBFUNCS", [connection_name, "COLUMNINFO", table_name])
        offset = [0]
        cnt = _parse_number(raw, offset)
        cols: List[Dict[str, Any]] = []
        for _ in range(cnt):
            item = _parse_string(raw, offset)
            fields = _parse_string_list_item(item)
            cols.append({
                "schema": fields[0] if len(fields) > 0 else "",
                "owner": fields[1] if len(fields) > 1 else "",
                "table": fields[2] if len(fields) > 2 else "",
                "name": fields[3] if len(fields) > 3 else "",
                "type": _safe_int(fields[4]) if len(fields) > 4 else 0,
                "typeStr": fields[5] if len(fields) > 5 else "",
                "precision": _safe_int(fields[6]) if len(fields) > 6 else 0,
                "length": _safe_int(fields[7]) if len(fields) > 7 else 0,
                "scale": _safe_int(fields[8]) if len(fields) > 8 else 0,
                "radix": _safe_int(fields[9]) if len(fields) > 9 else 0,
                "nullable": _safe_int(fields[10]) if len(fields) > 10 else 0,
            })
        return cols

    def sql_primarykeys(self, connection_name: str, table_name: str) -> List[Dict[str, Any]]:
        if connection_name is None:
            raise CFRDSError("connection_name is required")
        if table_name is None:
            raise CFRDSError("table_name is required")
        raw = self._send_rds_command("DBFUNCS", [connection_name, "PRIMARYKEYS", table_name])
        offset = [0]
        cnt = _parse_number(raw, offset)
        keys: List[Dict[str, Any]] = []
        for _ in range(cnt):
            item = _parse_string(raw, offset)
            fields = _parse_string_list_item(item)
            keys.append({
                "pkcatalog": fields[0] if len(fields) > 0 else "",
                "pkowner": fields[1] if len(fields) > 1 else "",
                "pktable": fields[2] if len(fields) > 2 else "",
                "pkcolumn": fields[3] if len(fields) > 3 else "",
                "fkcatalog": fields[4] if len(fields) > 4 else "",
                "fkowner": fields[5] if len(fields) > 5 else "",
                "fktable": fields[6] if len(fields) > 6 else "",
                "fkcolumn": fields[7] if len(fields) > 7 else "",
                "key_sequence": _safe_int(fields[8]) if len(fields) > 8 else 0,
                "updaterule": _safe_int(fields[9]) if len(fields) > 9 else 0,
                "deleterule": _safe_int(fields[10]) if len(fields) > 10 else 0,
            })
        return keys

    def sql_importedkeys(self, connection_name: str, table_name: str) -> List[Dict[str, Any]]:
        if connection_name is None:
            raise CFRDSError("connection_name is required")
        if table_name is None:
            raise CFRDSError("table_name is required")
        raw = self._send_rds_command("DBFUNCS", [connection_name, "IMPORTEDKEYS", table_name])
        offset = [0]
        cnt = _parse_number(raw, offset)
        keys: List[Dict[str, Any]] = []
        for _ in range(cnt):
            item = _parse_string(raw, offset)
            fields = _parse_string_list_item(item)
            keys.append({
                "pkcatalog": fields[0] if len(fields) > 0 else "",
                "pkowner": fields[1] if len(fields) > 1 else "",
                "pktable": fields[2] if len(fields) > 2 else "",
                "pkcolumn": fields[3] if len(fields) > 3 else "",
                "fkcatalog": fields[4] if len(fields) > 4 else "",
                "fkowner": fields[5] if len(fields) > 5 else "",
                "fktable": fields[6] if len(fields) > 6 else "",
                "fkcolumn": fields[7] if len(fields) > 7 else "",
                "key_sequence": _safe_int(fields[8]) if len(fields) > 8 else 0,
                "updaterule": _safe_int(fields[9]) if len(fields) > 9 else 0,
                "deleterule": _safe_int(fields[10]) if len(fields) > 10 else 0,
            })
        return keys

    def sql_exportedkeys(self, connection_name: str, table_name: str) -> List[Dict[str, Any]]:
        if connection_name is None:
            raise CFRDSError("connection_name is required")
        if table_name is None:
            raise CFRDSError("table_name is required")
        raw = self._send_rds_command("DBFUNCS", [connection_name, "EXPORTEDKEYS", table_name])
        offset = [0]
        cnt = _parse_number(raw, offset)
        keys: List[Dict[str, Any]] = []
        for _ in range(cnt):
            item = _parse_string(raw, offset)
            fields = _parse_string_list_item(item)
            keys.append({
                "pkcatalog": fields[0] if len(fields) > 0 else "",
                "pkowner": fields[1] if len(fields) > 1 else "",
                "pktable": fields[2] if len(fields) > 2 else "",
                "pkcolumn": fields[3] if len(fields) > 3 else "",
                "fkcatalog": fields[4] if len(fields) > 4 else "",
                "fkowner": fields[5] if len(fields) > 5 else "",
                "fktable": fields[6] if len(fields) > 6 else "",
                "fkcolumn": fields[7] if len(fields) > 7 else "",
                "key_sequence": _safe_int(fields[8]) if len(fields) > 8 else 0,
                "updaterule": _safe_int(fields[9]) if len(fields) > 9 else 0,
                "deleterule": _safe_int(fields[10]) if len(fields) > 10 else 0,
            })
        return keys

    def sql_sqlstmnt(self, connection_name: str, sql: str) -> Dict[str, Any]:
        if connection_name is None:
            raise CFRDSError("connection_name is required")
        if sql is None:
            raise CFRDSError("sql is required")
        raw = self._send_rds_command("DBFUNCS", [connection_name, "SQLSTMNT", sql])
        offset = [0]
        cnt = _parse_number(raw, offset)
        rows = max(0, cnt - 1)
        if cnt <= 0:
            return {"columns": 0, "rows": 0, "names": [], "values": []}

        col_str = _parse_string(raw, offset)
        names = _parse_string_list_item(col_str)
        cols = len(names)

        data_rows: List[List[str]] = []
        for _ in range(rows):
            r_str = _parse_string(raw, offset)
            r_vals = _parse_string_list_item(r_str)
            data_rows.append(r_vals)

        return {"columns": cols, "rows": rows, "names": names, "values": data_rows}

    def sql_metadata(self, connection_name: str, sql: str) -> List[Dict[str, Optional[str]]]:
        if connection_name is None:
            raise CFRDSError("connection_name is required")
        if sql is None:
            raise CFRDSError("sql is required")
        raw = self._send_rds_command("DBFUNCS", [connection_name, "SQLMETADATA", sql])
        offset = [0]
        cnt = _parse_number(raw, offset)
        meta: List[Dict[str, Optional[str]]] = []
        for _ in range(cnt):
            item = _parse_string(raw, offset)
            fields = _parse_string_list_item(item)
            meta.append({
                "name": fields[0] if len(fields) > 0 else "",
                "type": fields[1] if len(fields) > 1 else "",
                "jtype": fields[2] if len(fields) > 2 else "",
            })
        return meta

    def sql_getsupportedcommands(self) -> List[str]:
        raw = self._send_rds_command("DBFUNCS", ["", "SUPPORTEDCOMMANDS"])
        offset = [0]
        cnt = _parse_number(raw, offset)
        cmds: List[str] = []
        for _ in range(cnt):
            cmd_str = _parse_string(raw, offset)
            cmds.extend(_parse_string_list_item(cmd_str))
        return cmds

    def sql_dbdescription(self, connection_name: str) -> Optional[str]:
        if connection_name is None:
            raise CFRDSError("connection_name is required")
        raw = self._send_rds_command("DBFUNCS", [connection_name, "DBDESCRIPTION"])
        offset = [0]
        _parse_number(raw, offset)
        item = _parse_string(raw, offset)
        fields = _parse_string_list_item(item)
        return fields[0] if fields else item

    # Debugger Operations
    def debugger_start(self) -> str:
        raw = self._send_rds_command("DBGREQUEST", ["DBG_START", ""])
        offset = [0]
        _parse_number(raw, offset)
        session_id = _parse_string(raw, offset)
        return session_id

    def debugger_stop(self, session_name: str) -> None:
        if session_name is None:
            raise CFRDSError("session_name is required")
        self._send_rds_command("DBGREQUEST", ["DBG_STOP", session_name])

    def debugger_get_server_info(self, session_name: str) -> int:
        if session_name is None:
            raise CFRDSError("session_name is required")
        raw = self._send_rds_command("DBGREQUEST", ["DBG_GET_DEBUG_SERVER_INFO", session_name])
        offset = [0]
        _parse_number(raw, offset)
        wddx_xml = _parse_string(raw, offset)
        parsed = _wddx_deserialize(wddx_xml)
        if parsed and isinstance(parsed, dict) and "DEBUG_SERVER_PORT" in parsed:
            try:
                return int(float(parsed["DEBUG_SERVER_PORT"]))
            except (ValueError, TypeError):
                pass
        return 0

    def debugger_breakpoint_on_exception(self, session_name: str, enable: bool) -> None:
        if session_name is None:
            raise CFRDSError("session_name is required")
        if enable is None:
            raise CFRDSError("enable is required")
        val = "true" if enable else "false"
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SESSION_BREAK_ON_EXCEPTION</string></var><var name='BREAK_ON_EXCEPTION'><boolean value='{val}'/></var></struct></array></data></wddxPacket>"
        self._send_rds_command("DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_breakpoint(self, session_name: str, filepath: str, line: int, enable: bool) -> None:
        if session_name is None:
            raise CFRDSError("session_name is required")
        if filepath is None:
            raise CFRDSError("filepath is required")
        if line is None:
            raise CFRDSError("line is required")
        if enable is None:
            raise CFRDSError("enable is required")
        cmd = "SET_BREAKPOINT" if enable else "UNSET_BREAKPOINT"
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>{cmd}</string></var><var name='FILE'><string>{_escape_xml(filepath)}</string></var><var name='Y'><number>{line}</number></var><var name='SEQ'><number>1.0</number></var></struct></array></data></wddxPacket>"
        self._send_rds_command("DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_clear_all_breakpoints(self, session_name: str) -> None:
        if session_name is None:
            raise CFRDSError("session_name is required")
        wddx = "<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>UNSET_ALL_BREAKPOINTS</string></var></struct></array></data></wddxPacket>"
        self._send_rds_command("DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def _parse_debugger_event(self, raw: bytes) -> Optional[Dict[str, Any]]:
        if not raw:
            return None
        offset = [0]
        _parse_number(raw, offset)
        wddx_xml = _parse_string(raw, offset)
        if not wddx_xml:
            return None

        parsed = _wddx_deserialize(wddx_xml)
        if not parsed:
            return None
        data = parsed[0] if isinstance(parsed, list) and len(parsed) > 0 else (parsed if isinstance(parsed, dict) else {})

        evt_name = data.get("EVENT") or data.get("COMMAND")
        thread_name = str(data.get("THREAD") or data.get("THREAD_ID") or data.get("THREAD_NAME") or "main")
        data["thread_name"] = thread_name
        data["thread_id"] = thread_name

        def _to_int(val: Any) -> int:
            try:
                return int(float(val))
            except (ValueError, TypeError):
                return 0

        if evt_name == "CF_BREAKPOINT_SET":
            return {
                "type": CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT_SET,
                "data": {
                    "pathname": data.get("CFML_PATH") or data.get("FILE", ""),
                    "req_line": _to_int(data.get("REQ_LINE_NUM")),
                    "act_line": _to_int(data.get("ACTUAL_LINE_NUM")),
                    "thread_name": thread_name,
                    "thread_id": thread_name,
                    **data,
                },
            }
        elif evt_name in ("CF_BREAKPOINT_HIT", "CF_STEP", "BREAKPOINT", "STEP"):
            return {
                "type": CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT if "BREAKPOINT" in str(evt_name) else CFRDS_DEBUGGER_EVENT_TYPE_STEP,
                "data": {
                    "source": data.get("CFML_PATH") or data.get("FILE", ""),
                    "line": _to_int(data.get("REQ_LINE_NUM") or data.get("LINE")),
                    "thread_name": thread_name,
                    "thread_id": thread_name,
                    **data,
                },
            }
        return {"type": CFRDS_DEBUGGER_EVENT_UNKNOWN, "data": data}

    def debugger_get_debug_events(self, session_name: str) -> Optional[Dict[str, Any]]:
        """
        Fetches debugger events for the specified session.
        NOTE: This is a long-polling request on the ColdFusion server that blocks until a debugger event occurs or times out.
        """
        if session_name is None:
            raise CFRDSError("session_name is required")
        raw = self._send_rds_command("DBGREQUEST", ["DBG_EVENTS", session_name])
        return self._parse_debugger_event(raw)

    def debugger_all_fetch_flags_enabled(
        self, session_name: str, threads: bool, watch: bool, scopes: bool, cf_trace: bool, java_trace: bool
    ) -> Optional[Dict[str, Any]]:
        """
        Configures fetch flags and waits for debugger events.
        NOTE: This is a long-polling request on the ColdFusion server that blocks until a debugger event occurs or times out.
        """
        if session_name is None:
            raise CFRDSError("session_name is required")
        if threads is None:
            raise CFRDSError("threads is required")
        if watch is None:
            raise CFRDSError("watch is required")
        if scopes is None:
            raise CFRDSError("scopes is required")
        if cf_trace is None:
            raise CFRDSError("cf_trace is required")
        if java_trace is None:
            raise CFRDSError("java_trace is required")
        def b(v: bool) -> str:
            return "true" if v else "false"
        wddx = (f"<wddxPacket version='1.0'><header/><data><struct type='java.util.HashMap'>"
                f"<var name='THREADS'><boolean value='{b(threads)}'/></var>"
                f"<var name='WATCH'><boolean value='{b(watch)}'/></var>"
                f"<var name='SCOPES'><boolean value='{b(scopes)}'/></var>"
                f"<var name='CF_TRACE'><boolean value='{b(cf_trace)}'/></var>"
                f"<var name='JAVA_TRACE'><boolean value='{b(java_trace)}'/></var>"
                f"</struct></data></wddxPacket>")
        raw = self._send_rds_command("DBGREQUEST", ["DBG_EVENTS", session_name, wddx])
        return self._parse_debugger_event(raw)

    def debugger_step_in(self, session_name: str, thread_name: str) -> None:
        if session_name is None:
            raise CFRDSError("session_name is required")
        if thread_name is None:
            raise CFRDSError("thread_name is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_IN</string></var><var name='THREAD'><string>{_escape_xml(thread_name)}</string></var></struct></array></data></wddxPacket>"
        self._send_rds_command("DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_step_over(self, session_name: str, thread_name: str) -> None:
        if session_name is None:
            raise CFRDSError("session_name is required")
        if thread_name is None:
            raise CFRDSError("thread_name is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_OVER</string></var><var name='THREAD'><string>{_escape_xml(thread_name)}</string></var></struct></array></data></wddxPacket>"
        self._send_rds_command("DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_step_out(self, session_name: str, thread_name: str) -> None:
        if session_name is None:
            raise CFRDSError("session_name is required")
        if thread_name is None:
            raise CFRDSError("thread_name is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_OUT</string></var><var name='THREAD'><string>{_escape_xml(thread_name)}</string></var></struct></array></data></wddxPacket>"
        self._send_rds_command("DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_continue(self, session_name: str, thread_name: str) -> None:
        if session_name is None:
            raise CFRDSError("session_name is required")
        if thread_name is None:
            raise CFRDSError("thread_name is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>CONTINUE</string></var><var name='THREAD'><string>{_escape_xml(thread_name)}</string></var></struct></array></data></wddxPacket>"
        self._send_rds_command("DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_watch_expression(self, session_name: str, thread_name: str, expression: str) -> None:
        if session_name is None:
            raise CFRDSError("session_name is required")
        if thread_name is None:
            raise CFRDSError("thread_name is required")
        if expression is None:
            raise CFRDSError("expression is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>GET_SINGLE_CF_VARIABLE</string></var><var name='VARIABLE_NAME'><string>{_escape_xml(expression)}</string></var><var name='THREAD'><string>{_escape_xml(thread_name)}</string></var></struct></array></data></wddxPacket>"
        self._send_rds_command("DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_set_variable(self, session_name: str, thread_name: str, variable: str, value: str) -> None:
        if session_name is None:
            raise CFRDSError("session_name is required")
        if thread_name is None:
            raise CFRDSError("thread_name is required")
        if variable is None:
            raise CFRDSError("variable is required")
        if value is None:
            raise CFRDSError("value is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_VARIABLE_VALUE</string></var><var name='VARIABLE_NAME'><string>{_escape_xml(variable)}</string></var><var name='VARIABLE_VALUE'><string>{_escape_xml(value)}</string></var><var name='THREAD'><string>{_escape_xml(thread_name)}</string></var></struct></array></data></wddxPacket>"
        self._send_rds_command("DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_watch_variables(self, session_name: str, variables: str) -> None:
        if session_name is None:
            raise CFRDSError("session_name is required")
        if variables is None:
            raise CFRDSError("variables is required")
        vars_list = [v.strip() for v in variables.split(",") if v.strip()]
        var_tags = "".join([f"<string>{_escape_xml(v)}</string>" for v in vars_list])
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_WATCH_VARIABLES</string></var><var name='WATCH'><array length='{len(vars_list)}'>{var_tags}</array></var></struct></array></data></wddxPacket>"
        self._send_rds_command("DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_get_output(self, session_name: str, thread_name: str) -> str:
        if session_name is None:
            raise CFRDSError("session_name is required")
        if thread_name is None:
            raise CFRDSError("thread_name is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>GET_OUTPUT</string></var><var name='BODY_ONLY'><boolean value='true'/></var><var name='THREAD'><string>{_escape_xml(thread_name)}</string></var></struct></array></data></wddxPacket>"
        raw = self._send_rds_command("DBGREQUEST", ["DBG_REQUEST", session_name, wddx])
        if not raw:
            return ""
        offset = [0]
        _parse_number(raw, offset)
        wddx_xml = _parse_string(raw, offset)
        if not wddx_xml:
            return ""
        parsed = _wddx_deserialize(wddx_xml)
        if parsed and isinstance(parsed, dict) and "VALUE" in parsed:
            val = parsed["VALUE"]
            return str(val) if val is not None else ""
        return ""

    def debugger_set_scope_filter(self, session_name: str, filter_str: str) -> None:
        if session_name is None:
            raise CFRDSError("session_name is required")
        if filter_str is None:
            raise CFRDSError("filter_str is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_SCOPE_FILTER</string></var><var name='FILTER'><string>{_escape_xml(filter_str)}</string></var></struct></array></data></wddxPacket>"
        self._send_rds_command("DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    # Security Analyzer Operations
    def security_analyzer_scan(self, pathnames: str, recursively: bool = True, cores: int = 1) -> int:
        if pathnames is None:
            raise CFRDSError("pathnames is required")
        raw = self._send_rds_command("SECURITYANALYZER", ["scan", pathnames, "true" if recursively else "false", str(cores)])
        offset = [0]
        _parse_number(raw, offset)
        json_str = _parse_string(raw, offset)
        import json
        try:
            data = json.loads(json_str)
            return int(data.get("id", 0))
        except Exception:
            return 0

    def security_analyzer_cancel(self, command_id: int) -> None:
        if command_id is None:
            raise CFRDSError("command_id is required")
        self._send_rds_command("SECURITYANALYZER", ["cancel", str(command_id)])

    def security_analyzer_status(self, command_id: int) -> Dict[str, Any]:
        if command_id is None:
            raise CFRDSError("command_id is required")
        raw = self._send_rds_command("SECURITYANALYZER", ["status", str(command_id)])
        offset = [0]
        _parse_number(raw, offset)
        json_str = _parse_string(raw, offset)
        import json
        try:
            data = json.loads(json_str)
            return {
                "totalfiles": int(data.get("totalfiles", 0)),
                "filesvisitedcount": int(data.get("filesvisitedcount", 0)),
                "percentage": int(data.get("percentage", 0)),
                "lastupdated": int(data.get("lastupdated", 0)),
            }
        except Exception:
            return {
                "totalfiles": 0,
                "filesvisitedcount": 0,
                "percentage": 0,
                "lastupdated": 0,
            }

    def security_analyzer_result(self, command_id: int) -> Optional[Dict[str, Any]]:
        if command_id is None:
            raise CFRDSError("command_id is required")
        raw = self._send_rds_command("SECURITYANALYZER", ["result", str(command_id)])
        offset = [0]
        _parse_number(raw, offset)
        json_str = _parse_string(raw, offset)
        import json
        try:
            return json.loads(json_str)
        except Exception:
            return {"raw": json_str}

    def security_analyzer_clean(self, command_id: int) -> None:
        if command_id is None:
            raise CFRDSError("command_id is required")
        self._send_rds_command("SECURITYANALYZER", ["clean", str(command_id)])

    # IDE Default
    def ide_default(self, version: int = 1) -> Dict[str, Any]:
        if version is None:
            raise CFRDSError("version is required")
        raw = self._send_rds_command("IDE_DEFAULT", ["", f"{version},"])
        offset = [0]
        _parse_number(raw, offset)
        n1 = _parse_string(raw, offset)
        s_ver = _parse_string(raw, offset)
        c_ver = _parse_string(raw, offset)
        n2 = _parse_string(raw, offset)
        n3 = _parse_string(raw, offset)
        return {
            "num1": _safe_int(n1),
            "server_version": s_ver,
            "client_version": c_ver,
            "num2": _safe_int(n2),
            "num3": _safe_int(n3),
        }

    # Admin API Operations
    def adminapi_debugging_getlogproperty(self, logdirectory: str) -> Optional[str]:
        if logdirectory is None:
            raise CFRDSError("logdirectory is required")
        raw = self._send_rds_command("ADMINAPI", ["cfide.adminapi.debugging", "getlogproperty", logdirectory])
        offset = [0]
        _parse_number(raw, offset)
        prop_str = _parse_string(raw, offset)
        return prop_str

    def adminapi_extensions_getcustomtagpaths(self) -> List[str]:
        raw = self._send_rds_command("ADMINAPI", ["cfide.adminapi.extensions", "getcustomtagpaths"])
        offset = [0]
        _parse_number(raw, offset)
        xml_str = _parse_string(raw, offset)
        paths: List[str] = []
        if xml_str:
            parsed = _wddx_deserialize(xml_str)
            if isinstance(parsed, list):
                for item in parsed:
                    if isinstance(item, str):
                        paths.append(item)
            elif isinstance(parsed, str):
                paths.append(parsed)
        return paths

    def adminapi_extensions_setmapping(self, name: str, path: str) -> None:
        if name is None:
            raise CFRDSError("name is required")
        if path is None:
            raise CFRDSError("path is required")
        arg_str = f"name:{name};path:{path}"
        self._send_rds_command("ADMINAPI", ["cfide.adminapi.extensions", "setmappings", arg_str])

    def adminapi_extensions_deletemapping(self, mapping: str) -> None:
        if mapping is None:
            raise CFRDSError("mapping is required")
        # NOTE: "deleltemappings" (with the extra 'l') is a required typo hardcoded in the Adobe ColdFusion RDS backend.
        self._send_rds_command("ADMINAPI", ["cfide.adminapi.extensions", "deleltemappings", mapping])

    def adminapi_extensions_getmappings(self) -> Mappings:
        raw = self._send_rds_command("ADMINAPI", ["cfide.adminapi.extensions", "getmappings"])
        offset = [0]
        _parse_number(raw, offset)
        xml_str = _parse_string(raw, offset)
        keys_list: List[str] = []
        values_list: List[str] = []
        mappings_dict: Dict[str, str] = {}
        if xml_str and xml_str.strip():
            import xml.etree.ElementTree as ET
            try:
                root = ET.fromstring(xml_str)
                struct_elem = None
                def find_struct(elem):
                    nonlocal struct_elem
                    tag = elem.tag
                    if "}" in tag:
                        tag = tag.split("}", 1)[1]
                    if tag.lower() == "struct":
                        struct_elem = elem
                        return
                    for child in elem:
                        find_struct(child)
                        if struct_elem is not None:
                            return
                find_struct(root)
                if struct_elem is not None:
                    for var in struct_elem:
                        var_tag = var.tag
                        if "}" in var_tag:
                            var_tag = var_tag.split("}", 1)[1]
                        if var_tag.lower() == "var":
                            name = var.attrib.get("name")
                            if name:
                                val_str = ""
                                if len(var) > 0:
                                    parsed_val = _parse_wddx_node(var[0])
                                    val_str = str(parsed_val) if parsed_val is not None else ""
                                keys_list.append(name)
                                values_list.append(val_str)
                                mappings_dict[name] = val_str
            except Exception:
                pass
        return Mappings(keys_list, values_list, mappings_dict)

    # Graphing Operations
    def graphing(self, chart_attributes: str, series_data: List[str]) -> bytes:
        if chart_attributes is None:
            raise CFRDSError("chart_attributes is required")
        if series_data is None:
            raise CFRDSError("series_data is required")
        args = ["GRAPH", chart_attributes, str(len(series_data))] + series_data
        raw = self._send_rds_command("GRAPHING", args)
        return raw


# Class Alias

