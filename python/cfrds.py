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



class cfrds_status(IntEnum):
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


CFRDS_STATUS_OK = cfrds_status.CFRDS_STATUS_OK
CFRDS_STATUS_MEMORY_ERROR = cfrds_status.CFRDS_STATUS_MEMORY_ERROR
CFRDS_STATUS_PARAM_IS_NULL = cfrds_status.CFRDS_STATUS_PARAM_IS_NULL
CFRDS_STATUS_SERVER_IS_NULL = cfrds_status.CFRDS_STATUS_SERVER_IS_NULL
CFRDS_STATUS_INVALID_INPUT_PARAMETER = cfrds_status.CFRDS_STATUS_INVALID_INPUT_PARAMETER
CFRDS_STATUS_INDEX_OUT_OF_BOUNDS = cfrds_status.CFRDS_STATUS_INDEX_OUT_OF_BOUNDS
CFRDS_STATUS_COMMAND_FAILED = cfrds_status.CFRDS_STATUS_COMMAND_FAILED
CFRDS_STATUS_RESPONSE_ERROR = cfrds_status.CFRDS_STATUS_RESPONSE_ERROR
CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND = cfrds_status.CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND
CFRDS_STATUS_DIR_ALREADY_EXISTS = cfrds_status.CFRDS_STATUS_DIR_ALREADY_EXISTS
CFRDS_STATUS_SOCKET_HOST_NOT_FOUND = cfrds_status.CFRDS_STATUS_SOCKET_HOST_NOT_FOUND
CFRDS_STATUS_SOCKET_CREATION_FAILED = cfrds_status.CFRDS_STATUS_SOCKET_CREATION_FAILED
CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED = cfrds_status.CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED
CFRDS_STATUS_WRITING_TO_SOCKET_FAILED = cfrds_status.CFRDS_STATUS_WRITING_TO_SOCKET_FAILED
CFRDS_STATUS_PARTIALLY_WRITE_TO_SOCKET = cfrds_status.CFRDS_STATUS_PARTIALLY_WRITE_TO_SOCKET
CFRDS_STATUS_READING_FROM_SOCKET_FAILED = cfrds_status.CFRDS_STATUS_READING_FROM_SOCKET_FAILED
CFRDS_STATUS_RESPONSE_TOO_LARGE = cfrds_status.CFRDS_STATUS_RESPONSE_TOO_LARGE


class cfrds_debugger_type(IntEnum):
    CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT_SET = 0
    CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT = 1
    CFRDS_DEBUGGER_EVENT_TYPE_STEP = 2
    CFRDS_DEBUGGER_EVENT_UNKNOWN = 3


CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT_SET = cfrds_debugger_type.CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT_SET
CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT = cfrds_debugger_type.CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT
CFRDS_DEBUGGER_EVENT_TYPE_STEP = cfrds_debugger_type.CFRDS_DEBUGGER_EVENT_TYPE_STEP
CFRDS_DEBUGGER_EVENT_UNKNOWN = cfrds_debugger_type.CFRDS_DEBUGGER_EVENT_UNKNOWN


class CFRDSError(Exception):
    """Exception raised for ColdFusion RDS protocol errors."""
    def __init__(self, status: int, message: str = ""):
        self.status = status
        try:
            self.status_enum = cfrds_status(status)
            status_name = self.status_enum.name
        except ValueError:
            status_name = f"STATUS_{status}"

        full_msg = f"{status_name} ({status})"
        if message:
            full_msg += f": {message}"
        super().__init__(full_msg)


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


# Pure Python Server Context Object
class cfrds_server:
    def __init__(self, host: str, port: int, username: str, password: str):
        self.host = host
        self.port = port
        self.username = username
        self.orig_password = password
        self.encoded_password = _encode_password(password) if password else ""
        self.error_code: int = 1
        self.error: Optional[str] = None

    def clear_error(self) -> None:
        self.error_code = 1
        self.error = None

    def set_error(self, code: int, msg: str) -> None:
        self.error_code = code
        self.error = msg


# Response Parsing Utilities
def _parse_number(data: bytes, offset: int) -> Tuple[int, int]:
    colon_pos = data.find(b":", offset)
    if colon_pos == -1:
        raise CFRDSError(CFRDS_STATUS_RESPONSE_ERROR, "Failed to parse number: missing ':' delimiter")
    try:
        val = int(data[offset:colon_pos].decode("utf-8"))
    except ValueError:
        raise CFRDSError(CFRDS_STATUS_RESPONSE_ERROR, "Failed to parse number: non-integer value")
    return val, colon_pos + 1


def _parse_string(data: bytes, offset: int) -> Tuple[str, int]:
    size, offset = _parse_number(data, offset)
    if size < 0 or offset + size > len(data):
        raise CFRDSError(CFRDS_STATUS_RESPONSE_ERROR, "Failed to parse string: bounds error")
    raw_str = data[offset:offset + size].decode("utf-8", errors="replace")
    return raw_str, offset + size


def _parse_bytearray(data: bytes, offset: int) -> Tuple[bytes, int]:
    size, offset = _parse_number(data, offset)
    if size < 0 or offset + size > len(data):
        raise CFRDSError(CFRDS_STATUS_RESPONSE_ERROR, "Failed to parse bytearray: bounds error")
    raw_bytes = data[offset:offset + size]
    return raw_bytes, offset + size


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


# Network Transport Function
def _send_rds_command(server_ctx: cfrds_server, command: str, args: List[Union[str, bytes]]) -> bytes:
    server_ctx.clear_error()

    all_items: List[bytes] = []
    for a in args:
        if isinstance(a, str):
            all_items.append(a.encode("utf-8"))
        else:
            all_items.append(a)

    if server_ctx.username is not None:
        all_items.append(server_ctx.username.encode("utf-8"))
    if server_ctx.orig_password is not None:
        all_items.append(server_ctx.encoded_password.encode("utf-8"))

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
        "Connection": "close",
    }

    try:
        conn = http.client.HTTPConnection(server_ctx.host, server_ctx.port, timeout=30)
        conn.request("POST", path, body=payload, headers=headers)
        resp = conn.getresponse()
        if resp.status != 200:
            server_ctx.set_error(CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND, f"HTTP {resp.status} {resp.reason}")
            raise CFRDSError(CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND, f"HTTP {resp.status} {resp.reason}")
        body = resp.read()
        conn.close()
    except Exception as e:
        if isinstance(e, CFRDSError):
            raise
        server_ctx.set_error(CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED, str(e))
        raise CFRDSError(CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED, str(e))

    # Validate response status code at start of body
    try:
        err_code, offset = _parse_number(body, 0)
    except Exception as e:
        server_ctx.set_error(CFRDS_STATUS_RESPONSE_ERROR, str(e))
        raise CFRDSError(CFRDS_STATUS_RESPONSE_ERROR, str(e))

    server_ctx.error_code = err_code
    if err_code < 0:
        err_msg = body[offset:].decode("utf-8", errors="replace")
        server_ctx.set_error(CFRDS_STATUS_COMMAND_FAILED, err_msg)
        raise CFRDSError(CFRDS_STATUS_COMMAND_FAILED, err_msg)

    return body


# Struct Result Container Objects for C-API Compatibility
class cfrds_browse_dir:
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
        return f"cfrds_browse_dir({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_browse_dir):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False


class cfrds_file_content:
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
        return f"cfrds_file_content({self.to_dict()!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_file_content):
            return self.to_dict() == other.to_dict()
        if isinstance(other, dict):
            return self.to_dict() == other
        return False


class cfrds_sql_dsninfo:
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
        return f"cfrds_sql_dsninfo({self.names!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_sql_dsninfo):
            return self.names == other.names
        if isinstance(other, list):
            return self.names == other
        return False


class cfrds_sql_tableinfo:
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
        return f"cfrds_sql_tableinfo({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_sql_tableinfo):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False


class cfrds_sql_columninfo:
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
        return f"cfrds_sql_columninfo({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_sql_columninfo):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False


class cfrds_sql_primarykeys:
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
        return f"cfrds_sql_primarykeys({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_sql_primarykeys):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False


class cfrds_sql_foreignkeys:
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
        return f"cfrds_sql_foreignkeys({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_sql_foreignkeys):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False


class cfrds_sql_importedkeys:
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
        return f"cfrds_sql_importedkeys({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_sql_importedkeys):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False


class cfrds_sql_exportedkeys:
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
        return f"cfrds_sql_exportedkeys({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_sql_exportedkeys):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False


class cfrds_sql_resultset:
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
        return f"cfrds_sql_resultset({self.to_dict()!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_sql_resultset):
            return self.to_dict() == other.to_dict()
        if isinstance(other, dict):
            return self.to_dict() == other
        return False


class cfrds_sql_metadata:
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
        return f"cfrds_sql_metadata({self.items!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_sql_metadata):
            return self.items == other.items
        if isinstance(other, list):
            return self.items == other
        return False


class cfrds_sql_supportedcommands:
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
        return f"cfrds_sql_supportedcommands({self.commands!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_sql_supportedcommands):
            return self.commands == other.commands
        if isinstance(other, list):
            return self.commands == other
        return False


class cfrds_debugger_event:
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
        return f"cfrds_debugger_event({self.to_dict()!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_debugger_event):
            return self.to_dict() == other.to_dict()
        if isinstance(other, dict):
            return self.to_dict() == other
        return False


class cfrds_security_analyzer_result:
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
        return f"cfrds_security_analyzer_result({self.data!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_security_analyzer_result):
            return self.data == other.data
        if isinstance(other, dict):
            return self.data == other
        return False


class cfrds_adminapi_customtagpaths:
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
        return f"cfrds_adminapi_customtagpaths({self.paths!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_adminapi_customtagpaths):
            return self.paths == other.paths
        if isinstance(other, list):
            return self.paths == other
        return False


class cfrds_adminapi_mappings:
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
        return f"cfrds_adminapi_mappings({self.mappings!r})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, cfrds_adminapi_mappings):
            return self.mappings == other.mappings
        if isinstance(other, dict):
            return self.mappings == other
        return False


# High-Level Server API
class server:
    def __init__(self, hostname: str = "127.0.0.1", port: int = 8500, username: str = "admin", password: str = ""):
        self._ctx = cfrds_server(hostname, port, username, password)

    def close(self) -> None:
        pass

    def __enter__(self) -> "server":
        return self

    def __exit__(self, exc_type: Any, exc_val: Any, exc_tb: Any) -> None:
        self.close()

    def get_host(self) -> str:
        return self._ctx.host

    def get_port(self) -> int:
        return self._ctx.port

    def get_username(self) -> str:
        return self._ctx.username

    def get_password(self) -> str:
        return self._ctx.orig_password

    # Browse Directory
    def browse_dir(self, path: str) -> List[Dict[str, Any]]:
        if path is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "path is required")
        raw = _send_rds_command(self._ctx, "BROWSEDIR", [path, ""])
        total, offset = _parse_number(raw, 0)
        if total < 0 or (total != 0 and total % 5 != 0):
            raise CFRDSError(CFRDS_STATUS_RESPONSE_ERROR, "Invalid total items count in browse_dir response")
        cnt = total // 5
        items: List[Dict[str, Any]] = []
        for _ in range(cnt):
            str_kind, offset = _parse_string(raw, offset)
            filename, offset = _parse_string(raw, offset)
            str_perms, offset = _parse_string(raw, offset)
            str_size, offset = _parse_string(raw, offset)
            str_ts, offset = _parse_string(raw, offset)

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

            perms_str = list("-----")
            if kind == 'D':
                perms_str[0] = 'D'
            if perms_num & 0x01:
                perms_str[1] = 'R'
            if perms_num & 0x02:
                perms_str[2] = 'H'
            if perms_num & 0x10:
                perms_str[3] = 'A'
            if perms_num & 0x80:
                perms_str[4] = 'N'

            items.append({
                "kind": kind,
                "name": filename,
                "permissions": "".join(perms_str),
                "size": size,
                "modified": modified,
            })
        return items

    # File Operations
    def file_read(self, filepath: str) -> bytearray:
        if filepath is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "filepath is required")
        raw = _send_rds_command(self._ctx, "FILEIO", [filepath, "READ", ""])
        total, offset = _parse_number(raw, 0)
        data_bytes, offset = _parse_bytearray(raw, offset)
        modified, offset = _parse_string(raw, offset)
        permission, offset = _parse_string(raw, offset)
        return bytearray(data_bytes)

    def file_write(self, filepath: str, content: Union[bytes, bytearray, str]) -> None:
        if filepath is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "filepath is required")
        if content is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "content is required")
        if isinstance(content, str):
            data_bytes = content.encode("utf-8")
        else:
            data_bytes = bytes(content)
        _send_rds_command(self._ctx, "FILEIO", [filepath, "WRITE", "", data_bytes])

    def file_rename(self, filepath_from: str, filepath_to: str) -> None:
        if filepath_from is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "filepath_from is required")
        if filepath_to is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "filepath_to is required")
        _send_rds_command(self._ctx, "FILEIO", [filepath_from, "RENAME", "", filepath_to])

    def file_remove(self, filepath: str) -> None:
        if filepath is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "filepath is required")
        _send_rds_command(self._ctx, "FILEIO", [filepath, "REMOVE", "", "F"])

    def dir_remove(self, dirpath: str) -> None:
        if dirpath is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "dirpath is required")
        _send_rds_command(self._ctx, "FILEIO", [dirpath, "REMOVE", "", "D"])

    def file_exists(self, pathname: str) -> bool:
        if pathname is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "pathname is required")
        try:
            _send_rds_command(self._ctx, "FILEIO", [pathname, "EXISTENCE", "", ""])
            return True
        except CFRDSError as e:
            if e.status == CFRDS_STATUS_COMMAND_FAILED:
                return False
            raise

    def dir_create(self, dirpath: str) -> None:
        if dirpath is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "dirpath is required")
        _send_rds_command(self._ctx, "FILEIO", [dirpath, "CREATE", "", ""])

    def cf_root_dir(self) -> Optional[str]:
        raw = _send_rds_command(self._ctx, "FILEIO", ["", "CF_DIRECTORY"])
        _, offset = _parse_number(raw, 0)
        path_str, _ = _parse_string(raw, offset)
        return path_str

    # SQL Operations
    def sql_dsninfo(self) -> List[str]:
        raw = _send_rds_command(self._ctx, "DBFUNCS", ["", "DSNINFO"])
        cnt, offset = _parse_number(raw, 0)
        dsns: List[str] = []
        for _ in range(cnt):
            item, offset = _parse_string(raw, offset)
            fields = _parse_string_list_item(item)
            name = fields[0] if fields else item
            dsns.append(name)
        return dsns

    def sql_tableinfo(self, connection_name: str) -> List[Dict[str, Optional[str]]]:
        if connection_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "connection_name is required")
        raw = _send_rds_command(self._ctx, "DBFUNCS", [connection_name, "TABLEINFO"])
        cnt, offset = _parse_number(raw, 0)
        tables: List[Dict[str, Optional[str]]] = []
        for _ in range(cnt):
            item, offset = _parse_string(raw, offset)
            fields = _parse_string_list_item(item)
            f1 = fields[0] if len(fields) > 0 else ""
            f2 = fields[1] if len(fields) > 1 else ""
            f3 = fields[2] if len(fields) > 2 else ""
            f4 = fields[3] if len(fields) > 3 else ""
            tables.append({"unknown": f1, "schema": f2, "name": f3, "type": f4})
        return tables

    def sql_columninfo(self, connection_name: str, table_name: str) -> List[Dict[str, Any]]:
        if connection_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "connection_name is required")
        if table_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "table_name is required")
        raw = _send_rds_command(self._ctx, "DBFUNCS", [connection_name, "COLUMNINFO", table_name])
        cnt, offset = _parse_number(raw, 0)
        cols: List[Dict[str, Any]] = []
        for _ in range(cnt):
            item, offset = _parse_string(raw, offset)
            fields = _parse_string_list_item(item)
            cols.append({
                "schema": fields[0] if len(fields) > 0 else "",
                "owner": fields[1] if len(fields) > 1 else "",
                "table": fields[2] if len(fields) > 2 else "",
                "name": fields[3] if len(fields) > 3 else "",
                "type": int(fields[4]) if len(fields) > 4 and fields[4].isdigit() else 0,
                "typeStr": fields[5] if len(fields) > 5 else "",
                "precision": int(fields[6]) if len(fields) > 6 and fields[6].isdigit() else 0,
                "length": int(fields[7]) if len(fields) > 7 and fields[7].isdigit() else 0,
                "scale": int(fields[8]) if len(fields) > 8 and fields[8].isdigit() else 0,
                "radix": int(fields[9]) if len(fields) > 9 and fields[9].isdigit() else 0,
                "nullable": int(fields[10]) if len(fields) > 10 and fields[10].isdigit() else 0,
            })
        return cols

    def sql_primarykeys(self, connection_name: str, table_name: str) -> List[Dict[str, Any]]:
        if connection_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "connection_name is required")
        if table_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "table_name is required")
        raw = _send_rds_command(self._ctx, "DBFUNCS", [connection_name, "PRIMARYKEYS", table_name])
        cnt, offset = _parse_number(raw, 0)
        keys: List[Dict[str, Any]] = []
        for _ in range(cnt):
            item, offset = _parse_string(raw, offset)
            fields = _parse_string_list_item(item)
            keys.append({
                "catalog": fields[0] if len(fields) > 0 else "",
                "owner": fields[1] if len(fields) > 1 else "",
                "table": fields[2] if len(fields) > 2 else "",
                "column": fields[3] if len(fields) > 3 else "",
                "key_sequence": int(fields[4]) if len(fields) > 4 and fields[4].isdigit() else 0,
            })
        return keys

    def sql_foreignkeys(self, connection_name: str, table_name: str) -> List[Dict[str, Any]]:
        if connection_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "connection_name is required")
        if table_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "table_name is required")
        raw = _send_rds_command(self._ctx, "DBFUNCS", [connection_name, "FOREIGNKEYS", table_name])
        cnt, offset = _parse_number(raw, 0)
        keys: List[Dict[str, Any]] = []
        for _ in range(cnt):
            item, offset = _parse_string(raw, offset)
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
                "key_sequence": int(fields[8]) if len(fields) > 8 and fields[8].isdigit() else 0,
                "updaterule": int(fields[9]) if len(fields) > 9 and fields[9].isdigit() else 0,
                "deleterule": int(fields[10]) if len(fields) > 10 and fields[10].isdigit() else 0,
            })
        return keys

    def sql_importedkeys(self, connection_name: str, table_name: str) -> List[Dict[str, Any]]:
        if connection_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "connection_name is required")
        if table_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "table_name is required")
        raw = _send_rds_command(self._ctx, "DBFUNCS", [connection_name, "IMPORTEDKEYS", table_name])
        cnt, offset = _parse_number(raw, 0)
        keys: List[Dict[str, Any]] = []
        for _ in range(cnt):
            item, offset = _parse_string(raw, offset)
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
                "key_sequence": int(fields[8]) if len(fields) > 8 and fields[8].isdigit() else 0,
                "updaterule": int(fields[9]) if len(fields) > 9 and fields[9].isdigit() else 0,
                "deleterule": int(fields[10]) if len(fields) > 10 and fields[10].isdigit() else 0,
            })
        return keys

    def sql_exportedkeys(self, connection_name: str, table_name: str) -> List[Dict[str, Any]]:
        if connection_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "connection_name is required")
        if table_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "table_name is required")
        raw = _send_rds_command(self._ctx, "DBFUNCS", [connection_name, "EXPORTEDKEYS", table_name])
        cnt, offset = _parse_number(raw, 0)
        keys: List[Dict[str, Any]] = []
        for _ in range(cnt):
            item, offset = _parse_string(raw, offset)
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
                "key_sequence": int(fields[8]) if len(fields) > 8 and fields[8].isdigit() else 0,
                "updaterule": int(fields[9]) if len(fields) > 9 and fields[9].isdigit() else 0,
                "deleterule": int(fields[10]) if len(fields) > 10 and fields[10].isdigit() else 0,
            })
        return keys

    def sql_sqlstmnt(self, connection_name: str, sql: str) -> Dict[str, Any]:
        if connection_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "connection_name is required")
        if sql is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "sql is required")
        raw = _send_rds_command(self._ctx, "DBFUNCS", [connection_name, "SQLSTMNT", sql])
        cnt, offset = _parse_number(raw, 0)
        rows = max(0, cnt - 1)
        if cnt <= 0:
            return {"columns": 0, "rows": 0, "names": [], "values": []}

        col_str, offset = _parse_string(raw, offset)
        names = _parse_string_list_item(col_str)
        cols = len(names)

        data_rows: List[List[str]] = []
        for _ in range(rows):
            r_str, offset = _parse_string(raw, offset)
            r_vals = _parse_string_list_item(r_str)
            data_rows.append(r_vals)

        return {"columns": cols, "rows": rows, "names": names, "values": data_rows}

    def sql_metadata(self, connection_name: str, sql: str) -> List[Dict[str, Optional[str]]]:
        if connection_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "connection_name is required")
        if sql is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "sql is required")
        raw = _send_rds_command(self._ctx, "DBFUNCS", [connection_name, "SQLMETADATA", sql])
        cnt, offset = _parse_number(raw, 0)
        meta: List[Dict[str, Optional[str]]] = []
        for _ in range(cnt):
            item, offset = _parse_string(raw, offset)
            fields = _parse_string_list_item(item)
            meta.append({
                "name": fields[0] if len(fields) > 0 else "",
                "type": fields[1] if len(fields) > 1 else "",
                "jtype": fields[2] if len(fields) > 2 else "",
            })
        return meta

    def sql_getsupportedcommands(self) -> List[str]:
        raw = _send_rds_command(self._ctx, "DBFUNCS", ["", "SUPPORTEDCOMMANDS"])
        cnt, offset = _parse_number(raw, 0)
        cmds: List[str] = []
        for _ in range(cnt):
            cmd_str, offset = _parse_string(raw, offset)
            cmds.extend(_parse_string_list_item(cmd_str))
        return cmds

    def sql_dbdescription(self, connection_name: str) -> Optional[str]:
        if connection_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "connection_name is required")
        raw = _send_rds_command(self._ctx, "DBFUNCS", [connection_name, "DBDESCRIPTION"])
        _, offset = _parse_number(raw, 0)
        item, _ = _parse_string(raw, offset)
        fields = _parse_string_list_item(item)
        return fields[0] if fields else item

    # Debugger Operations
    def debugger_start(self) -> str:
        raw = _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_START", ""])
        _, offset = _parse_number(raw, 0)
        session_id, _ = _parse_string(raw, offset)
        return session_id

    def debugger_stop(self, session_name: str) -> None:
        if session_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_STOP", session_name])

    def debugger_get_server_info(self, session_name: str) -> int:
        if session_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        raw = _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_GET_DEBUG_SERVER_INFO", session_name])
        _, offset = _parse_number(raw, 0)
        wddx_xml, _ = _parse_string(raw, offset)
        parsed = _wddx_deserialize(wddx_xml)
        if parsed and isinstance(parsed, dict) and "DEBUG_SERVER_PORT" in parsed:
            try:
                return int(float(parsed["DEBUG_SERVER_PORT"]))
            except (ValueError, TypeError):
                pass
        return 0

    def debugger_breakpoint_on_exception(self, session_name: str, enable: bool) -> None:
        if session_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        if enable is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "enable is required")
        val = "true" if enable else "false"
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SESSION_BREAK_ON_EXCEPTION</string></var><var name='BREAK_ON_EXCEPTION'><boolean value='{val}'/></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_breakpoint(self, session_name: str, filepath: str, line: int, enable: bool) -> None:
        if session_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        if filepath is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "filepath is required")
        if line is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "line is required")
        if enable is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "enable is required")
        cmd = "SET_BREAKPOINT" if enable else "UNSET_BREAKPOINT"
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>{cmd}</string></var><var name='FILE'><string>{_escape_xml(filepath)}</string></var><var name='Y'><number>{line}</number></var><var name='SEQ'><number>1.0</number></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_clear_all_breakpoints(self, session_name: str) -> None:
        if session_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        wddx = "<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>UNSET_ALL_BREAKPOINTS</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def _parse_debugger_event(self, raw: bytes) -> Optional[Dict[str, Any]]:
        if not raw:
            return None
        _, offset = _parse_number(raw, 0)
        wddx_xml, _ = _parse_string(raw, offset)
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
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        raw = _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_EVENTS", session_name])
        return self._parse_debugger_event(raw)

    def debugger_all_fetch_flags_enabled(
        self, session_name: str, threads: bool, watch: bool, scopes: bool, cf_trace: bool, java_trace: bool
    ) -> Optional[Dict[str, Any]]:
        """
        Configures fetch flags and waits for debugger events.
        NOTE: This is a long-polling request on the ColdFusion server that blocks until a debugger event occurs or times out.
        """
        if session_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        if threads is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "threads is required")
        if watch is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "watch is required")
        if scopes is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "scopes is required")
        if cf_trace is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "cf_trace is required")
        if java_trace is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "java_trace is required")
        def b(v: bool) -> str:
            return "true" if v else "false"
        wddx = (f"<wddxPacket version='1.0'><header/><data><struct type='java.util.HashMap'>"
                f"<var name='THREADS'><boolean value='{b(threads)}'/></var>"
                f"<var name='WATCH'><boolean value='{b(watch)}'/></var>"
                f"<var name='SCOPES'><boolean value='{b(scopes)}'/></var>"
                f"<var name='CF_TRACE'><boolean value='{b(cf_trace)}'/></var>"
                f"<var name='JAVA_TRACE'><boolean value='{b(java_trace)}'/></var>"
                f"</struct></data></wddxPacket>")
        raw = _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_EVENTS", session_name, wddx])
        return self._parse_debugger_event(raw)

    def debugger_step_in(self, session_name: str, thread_name: str) -> None:
        if session_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        if thread_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "thread_name is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_IN</string></var><var name='THREAD'><string>{_escape_xml(thread_name)}</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_step_over(self, session_name: str, thread_name: str) -> None:
        if session_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        if thread_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "thread_name is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_OVER</string></var><var name='THREAD'><string>{_escape_xml(thread_name)}</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_step_out(self, session_name: str, thread_name: str) -> None:
        if session_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        if thread_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "thread_name is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_OUT</string></var><var name='THREAD'><string>{_escape_xml(thread_name)}</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_continue(self, session_name: str, thread_name: str) -> None:
        if session_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        if thread_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "thread_name is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>CONTINUE</string></var><var name='THREAD'><string>{_escape_xml(thread_name)}</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_watch_expression(self, session_name: str, thread_name: str, expression: str) -> None:
        if session_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        if thread_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "thread_name is required")
        if expression is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "expression is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>GET_SINGLE_CF_VARIABLE</string></var><var name='VARIABLE_NAME'><string>{_escape_xml(expression)}</string></var><var name='THREAD'><string>{_escape_xml(thread_name)}</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_set_variable(self, session_name: str, thread_name: str, variable: str, value: str) -> None:
        if session_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        if thread_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "thread_name is required")
        if variable is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "variable is required")
        if value is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "value is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_VARIABLE_VALUE</string></var><var name='VARIABLE_NAME'><string>{_escape_xml(variable)}</string></var><var name='VARIABLE_VALUE'><string>{_escape_xml(value)}</string></var><var name='THREAD'><string>{_escape_xml(thread_name)}</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_watch_variables(self, session_name: str, variables: str) -> None:
        if session_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        if variables is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "variables is required")
        vars_list = [v.strip() for v in variables.split(",") if v.strip()]
        var_tags = "".join([f"<string>{_escape_xml(v)}</string>" for v in vars_list])
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_WATCH_VARIABLES</string></var><var name='WATCH'><array length='{len(vars_list)}'>{var_tags}</array></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_get_output(self, session_name: str, thread_name: str) -> str:
        if session_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        if thread_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "thread_name is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>GET_OUTPUT</string></var><var name='BODY_ONLY'><boolean value='true'/></var><var name='THREAD'><string>{_escape_xml(thread_name)}</string></var></struct></array></data></wddxPacket>"
        raw = _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])
        if not raw:
            return ""
        _, offset = _parse_number(raw, 0)
        wddx_xml, _ = _parse_string(raw, offset)
        if not wddx_xml:
            return ""
        parsed = _wddx_deserialize(wddx_xml)
        if parsed and isinstance(parsed, dict) and "VALUE" in parsed:
            val = parsed["VALUE"]
            return str(val) if val is not None else ""
        return ""

    def debugger_set_scope_filter(self, session_name: str, filter_str: str) -> None:
        if session_name is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "session_name is required")
        if filter_str is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "filter_str is required")
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_SCOPE_FILTER</string></var><var name='FILTER'><string>{_escape_xml(filter_str)}</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    # Security Analyzer Operations
    def security_analyzer_scan(self, pathnames: str, recursively: bool = True, cores: int = 1) -> int:
        if pathnames is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "pathnames is required")
        raw = _send_rds_command(
            self._ctx, "SECURITYANALYZER", ["scan", pathnames, "true" if recursively else "false", str(cores)]
        )
        _, offset = _parse_number(raw, 0)
        json_str, _ = _parse_string(raw, offset)
        import json
        try:
            data = json.loads(json_str)
            return int(data.get("id", 0))
        except Exception:
            return 0

    def security_analyzer_cancel(self, command_id: int) -> None:
        if command_id is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "command_id is required")
        _send_rds_command(self._ctx, "SECURITYANALYZER", ["cancel", str(command_id)])

    def security_analyzer_status(self, command_id: int) -> Dict[str, Any]:
        if command_id is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "command_id is required")
        raw = _send_rds_command(self._ctx, "SECURITYANALYZER", ["status", str(command_id)])
        _, offset = _parse_number(raw, 0)
        json_str, _ = _parse_string(raw, offset)
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
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "command_id is required")
        raw = _send_rds_command(self._ctx, "SECURITYANALYZER", ["result", str(command_id)])
        _, offset = _parse_number(raw, 0)
        json_str, _ = _parse_string(raw, offset)
        import json
        try:
            return json.loads(json_str)
        except Exception:
            return {"raw": json_str}

    def security_analyzer_clean(self, command_id: int) -> None:
        if command_id is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "command_id is required")
        _send_rds_command(self._ctx, "SECURITYANALYZER", ["clean", str(command_id)])

    # IDE Default
    def ide_default(self, version: int = 1) -> Dict[str, Any]:
        if version is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "version is required")
        raw = _send_rds_command(self._ctx, "IDE_DEFAULT", ["", f"{version},"])
        _, offset = _parse_number(raw, 0)
        n1, offset = _parse_string(raw, offset)
        s_ver, offset = _parse_string(raw, offset)
        c_ver, offset = _parse_string(raw, offset)
        n2, offset = _parse_string(raw, offset)
        n3, offset = _parse_string(raw, offset)
        def safe_int(s: str) -> int:
            try: return int(s)
            except ValueError: return 0
        return {
            "num1": safe_int(n1),
            "server_version": s_ver,
            "client_version": c_ver,
            "num2": safe_int(n2),
            "num3": safe_int(n3),
        }

    # Admin API Operations
    def adminapi_debugging_getlogproperty(self, logdirectory: str) -> Optional[str]:
        if logdirectory is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "logdirectory is required")
        raw = _send_rds_command(self._ctx, "ADMINAPI", ["cfide.adminapi.debugging", "getlogproperty", logdirectory])
        _, offset = _parse_number(raw, 0)
        prop_str, _ = _parse_string(raw, offset)
        return prop_str

    def adminapi_extensions_getcustomtagpaths(self) -> List[str]:
        raw = _send_rds_command(self._ctx, "ADMINAPI", ["cfide.adminapi.extensions", "getcustomtagpaths"])
        _, offset = _parse_number(raw, 0)
        xml_str, _ = _parse_string(raw, offset)
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
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "name is required")
        if path is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "path is required")
        # The C code serializes name+path into a WDDX struct before sending
        wddx = f"<wddxPacket version='1.0'><header/><data><struct><var name='{_escape_xml(name)}'><string>{_escape_xml(path)}</string></var></struct></data></wddxPacket>"
        _send_rds_command(self._ctx, "ADMINAPI", ["cfide.adminapi.extensions", "setmappings", wddx])

    def adminapi_extensions_deletemapping(self, mapping: str) -> None:
        if mapping is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "mapping is required")
        _send_rds_command(self._ctx, "ADMINAPI", ["cfide.adminapi.extensions", "deleltemappings", mapping])

    def adminapi_extensions_getmappings(self) -> cfrds_adminapi_mappings:
        raw = _send_rds_command(self._ctx, "ADMINAPI", ["cfide.adminapi.extensions", "getmappings"])
        _, offset = _parse_number(raw, 0)
        xml_str, _ = _parse_string(raw, offset)
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
        return cfrds_adminapi_mappings(keys_list, values_list, mappings_dict)

    # Graphing Operations
    def graphing(self, chart_attributes: str, series_data: List[str]) -> bytes:
        if chart_attributes is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "chart_attributes is required")
        if series_data is None:
            raise CFRDSError(CFRDS_STATUS_PARAM_IS_NULL, "series_data is required")
        args = ["GRAPH", chart_attributes, str(len(series_data))] + series_data
        raw = _send_rds_command(self._ctx, "GRAPHING", args)
        return raw


# Class Alias
Server = server


# Low-level C API Function Wrappers Matching cfrds.h Exported Signatures
def cfrds_buffer_cleanup(buf: Any) -> None:
    pass

def cfrds_buffer_data(buf: Any) -> Optional[bytes]:
    return getattr(buf, "data", None)

def cfrds_buffer_data_size(buf: Any) -> int:
    data = getattr(buf, "data", None)
    return len(data) if data else 0

def cfrds_file_content_cleanup(buf: Any) -> None:
    pass

def cfrds_str_cleanup(buf: Any) -> None:
    pass

def cfrds_server_init(server_ptr: List[Any], host: str, port: int, username: str, password: str) -> bool:
    try:
        srv = cfrds_server(host, port, username, password)
        if isinstance(server_ptr, list):
            server_ptr[0] = srv
        return True
    except Exception:
        return False

def cfrds_server_free(srv: Optional[cfrds_server]) -> None:
    pass

def cfrds_server_cleanup(server_ptr: List[Any]) -> None:
    if isinstance(server_ptr, list):
        server_ptr[0] = None

def cfrds_server_clear_error(srv: Optional[cfrds_server]) -> None:
    if srv:
        srv.clear_error()

def cfrds_server_get_error(srv: Optional[cfrds_server]) -> Optional[str]:
    return srv.error if srv else None

def cfrds_server_get_host(srv: Optional[cfrds_server]) -> Optional[str]:
    return srv.host if srv else None

def cfrds_server_get_port(srv: Optional[cfrds_server]) -> int:
    return srv.port if srv else 0

def cfrds_server_get_username(srv: Optional[cfrds_server]) -> Optional[str]:
    return srv.username if srv else None

def cfrds_server_get_password(srv: Optional[cfrds_server]) -> Optional[str]:
    return srv.orig_password if srv else None

# Low-level Command Wrappers
def cfrds_command_browse_dir(srv: Optional[cfrds_server], path: Optional[str], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or path is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        items = s.browse_dir(path)
        if isinstance(out_ptr, list):
            out_ptr[0] = cfrds_browse_dir(items)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_browse_dir_free(val: Any) -> None:
    pass

def cfrds_browse_dir_cleanup(val: Any) -> None:
    pass

def cfrds_browse_dir_count(val: cfrds_browse_dir) -> int:
    return len(val) if val else 0

def cfrds_browse_dir_item_get_kind(val: cfrds_browse_dir, ndx: int) -> str:
    return val.items[ndx]["kind"] if val else "F"

def cfrds_browse_dir_item_get_name(val: cfrds_browse_dir, ndx: int) -> Optional[str]:
    return val.items[ndx]["name"] if val else None

def cfrds_browse_dir_item_get_permissions(val: cfrds_browse_dir, ndx: int) -> int:
    if not val:
        return 0
    perms_str = val.items[ndx]["permissions"]
    res = 0
    if len(perms_str) > 1 and perms_str[1] == 'R':
        res |= 0x01
    if len(perms_str) > 2 and perms_str[2] == 'H':
        res |= 0x02
    if len(perms_str) > 3 and perms_str[3] == 'A':
        res |= 0x10
    if len(perms_str) > 4 and perms_str[4] == 'N':
        res |= 0x80
    return res

def cfrds_browse_dir_item_get_size(val: cfrds_browse_dir, ndx: int) -> int:
    return val.items[ndx]["size"] if val else 0

def cfrds_browse_dir_item_get_modified(val: cfrds_browse_dir, ndx: int) -> int:
    return val.items[ndx]["modified"] if val else 0

def cfrds_command_file_read(srv: Optional[cfrds_server], pathname: Optional[str], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or pathname is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        data = s.file_read(pathname)
        if isinstance(out_ptr, list):
            out_ptr[0] = cfrds_file_content(bytes(data), "", "")
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_file_content_free(val: Any) -> None:
    pass

def cfrds_file_content_get_data(val: cfrds_file_content) -> Optional[bytes]:
    return val.data if val else None

def cfrds_file_content_get_size(val: cfrds_file_content) -> int:
    return val.size if val else 0

def cfrds_file_content_get_modified(val: cfrds_file_content) -> Optional[str]:
    return val.modified if val else None

def cfrds_file_content_get_permission(val: cfrds_file_content) -> Optional[str]:
    return val.permission if val else None

def cfrds_command_file_write(srv: Optional[cfrds_server], pathname: Optional[str], data: Optional[bytes], length: Optional[int]) -> int:
    if srv is None or pathname is None or data is None or length is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.file_write(pathname, data[:length])
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_file_rename(srv: Optional[cfrds_server], current_pathname: Optional[str], new_pathname: Optional[str]) -> int:
    if srv is None or current_pathname is None or new_pathname is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.file_rename(current_pathname, new_pathname)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_file_remove_file(srv: Optional[cfrds_server], pathname: Optional[str]) -> int:
    if srv is None or pathname is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.file_remove(pathname)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_file_remove_dir(srv: Optional[cfrds_server], path: Optional[str]) -> int:
    if srv is None or path is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.dir_remove(path)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_file_exists(srv: Optional[cfrds_server], pathname: Optional[str], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or pathname is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        exists = s.file_exists(pathname)
        if isinstance(out_ptr, list):
            out_ptr[0] = exists
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_file_create_dir(srv: Optional[cfrds_server], path: Optional[str]) -> int:
    if srv is None or path is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.dir_create(path)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_file_get_root_dir(srv: Optional[cfrds_server], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        root = s.cf_root_dir()
        if isinstance(out_ptr, list):
            out_ptr[0] = root
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

# SQL Low-level wrappers
def cfrds_command_sql_dsninfo(srv: Optional[cfrds_server], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        names = s.sql_dsninfo()
        if isinstance(out_ptr, list):
            out_ptr[0] = cfrds_sql_dsninfo(names)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_sql_dsninfo_free(val: Any) -> None:
    pass

def cfrds_sql_dsninfo_cleanup(val: Any) -> None:
    pass

def cfrds_sql_dsninfo_count(val: cfrds_sql_dsninfo) -> int:
    return len(val) if val else 0

def cfrds_sql_dsninfo_item_get_name(val: cfrds_sql_dsninfo, ndx: int) -> Optional[str]:
    return val.names[ndx] if val and ndx < len(val.names) else None

def cfrds_command_sql_tableinfo(srv: Optional[cfrds_server], connection_name: Optional[str], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or connection_name is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        tbls = s.sql_tableinfo(connection_name)
        if isinstance(out_ptr, list):
            out_ptr[0] = cfrds_sql_tableinfo(tbls)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_sql_tableinfo_free(val: Any) -> None:
    pass

def cfrds_sql_tableinfo_cleanup(val: Any) -> None:
    pass

def cfrds_sql_tableinfo_count(val: cfrds_sql_tableinfo) -> int:
    return len(val) if val else 0

def cfrds_sql_tableinfo_get_column_unknown(val: cfrds_sql_tableinfo, col: int) -> Optional[str]:
    return val.items[col]["unknown"] if val and col < len(val.items) else None

def cfrds_sql_tableinfo_get_column_schema(val: cfrds_sql_tableinfo, col: int) -> Optional[str]:
    return val.items[col]["schema"] if val and col < len(val.items) else None

def cfrds_sql_tableinfo_get_column_name(val: cfrds_sql_tableinfo, col: int) -> Optional[str]:
    return val.items[col]["name"] if val and col < len(val.items) else None

def cfrds_sql_tableinfo_get_column_type(val: cfrds_sql_tableinfo, col: int) -> Optional[str]:
    return val.items[col]["type"] if val and col < len(val.items) else None

def cfrds_command_sql_columninfo(srv: Optional[cfrds_server], connection_name: Optional[str], table_name: Optional[str], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or connection_name is None or table_name is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        cols = s.sql_columninfo(connection_name, table_name)
        if isinstance(out_ptr, list):
            out_ptr[0] = cfrds_sql_columninfo(cols)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_sql_columninfo_free(val: Any) -> None:
    pass

def cfrds_sql_columninfo_cleanup(val: Any) -> None:
    pass

def cfrds_sql_columninfo_count(val: cfrds_sql_columninfo) -> int:
    return len(val) if val else 0

def cfrds_sql_columninfo_get_schema(val: cfrds_sql_columninfo, col: int) -> Optional[str]:
    return val.items[col]["schema"] if val and col < len(val.items) else None

def cfrds_sql_columninfo_get_owner(val: cfrds_sql_columninfo, col: int) -> Optional[str]:
    return val.items[col]["owner"] if val and col < len(val.items) else None

def cfrds_sql_columninfo_get_table(val: cfrds_sql_columninfo, col: int) -> Optional[str]:
    return val.items[col]["table"] if val and col < len(val.items) else None

def cfrds_sql_columninfo_get_name(val: cfrds_sql_columninfo, col: int) -> Optional[str]:
    return val.items[col]["name"] if val and col < len(val.items) else None

def cfrds_sql_columninfo_get_type(val: cfrds_sql_columninfo, col: int) -> int:
    return val.items[col]["type"] if val and col < len(val.items) else 0

def cfrds_sql_columninfo_get_typeStr(val: cfrds_sql_columninfo, col: int) -> Optional[str]:
    return val.items[col]["typeStr"] if val and col < len(val.items) else None

def cfrds_sql_columninfo_get_precision(val: cfrds_sql_columninfo, col: int) -> int:
    return val.items[col]["precision"] if val and col < len(val.items) else 0

def cfrds_sql_columninfo_get_length(val: cfrds_sql_columninfo, col: int) -> int:
    return val.items[col]["length"] if val and col < len(val.items) else 0

def cfrds_sql_columninfo_get_scale(val: cfrds_sql_columninfo, col: int) -> int:
    return val.items[col]["scale"] if val and col < len(val.items) else 0

def cfrds_sql_columninfo_get_radix(val: cfrds_sql_columninfo, col: int) -> int:
    return val.items[col]["radix"] if val and col < len(val.items) else 0

def cfrds_sql_columninfo_get_nullable(val: cfrds_sql_columninfo, col: int) -> int:
    return val.items[col]["nullable"] if val and col < len(val.items) else 0

def cfrds_command_sql_primarykeys(srv: Optional[cfrds_server], connection_name: Optional[str], table_name: Optional[str], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or connection_name is None or table_name is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        keys = s.sql_primarykeys(connection_name, table_name)
        if isinstance(out_ptr, list):
            out_ptr[0] = cfrds_sql_primarykeys(keys)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_sql_primarykeys_free(val: Any) -> None:
    pass

def cfrds_sql_primarykeys_cleanup(val: Any) -> None:
    pass

def cfrds_sql_primarykeys_count(val: cfrds_sql_primarykeys) -> int:
    return len(val) if val else 0

def cfrds_sql_primarykeys_get_catalog(val: cfrds_sql_primarykeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["catalog"] if val and ndx < len(val.items) else None

def cfrds_sql_primarykeys_get_owner(val: cfrds_sql_primarykeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["owner"] if val and ndx < len(val.items) else None

def cfrds_sql_primarykeys_get_table(val: cfrds_sql_primarykeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["table"] if val and ndx < len(val.items) else None

def cfrds_sql_primarykeys_get_column(val: cfrds_sql_primarykeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["column"] if val and ndx < len(val.items) else None

def cfrds_sql_primarykeys_get_key_sequence(val: cfrds_sql_primarykeys, ndx: int) -> int:
    return val.items[ndx]["key_sequence"] if val and ndx < len(val.items) else 0

def cfrds_command_sql_foreignkeys(srv: Optional[cfrds_server], connection_name: Optional[str], table_name: Optional[str], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or connection_name is None or table_name is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        keys = s.sql_foreignkeys(connection_name, table_name)
        if isinstance(out_ptr, list):
            out_ptr[0] = cfrds_sql_foreignkeys(keys)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_sql_foreignkeys_free(val: Any) -> None:
    pass

def cfrds_sql_foreignkeys_cleanup(val: Any) -> None:
    pass

def cfrds_sql_foreignkeys_count(val: cfrds_sql_foreignkeys) -> int:
    return len(val) if val else 0

def cfrds_sql_foreignkeys_get_pkcatalog(val: cfrds_sql_foreignkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pkcatalog"] if val and ndx < len(val.items) else None

def cfrds_sql_foreignkeys_get_pkowner(val: cfrds_sql_foreignkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pkowner"] if val and ndx < len(val.items) else None

def cfrds_sql_foreignkeys_get_pktable(val: cfrds_sql_foreignkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pktable"] if val and ndx < len(val.items) else None

def cfrds_sql_foreignkeys_get_pkcolumn(val: cfrds_sql_foreignkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pkcolumn"] if val and ndx < len(val.items) else None

def cfrds_sql_foreignkeys_get_fkcatalog(val: cfrds_sql_foreignkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkcatalog"] if val and ndx < len(val.items) else None

def cfrds_sql_foreignkeys_get_fkowner(val: cfrds_sql_foreignkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkowner"] if val and ndx < len(val.items) else None

def cfrds_sql_foreignkeys_get_fktable(val: cfrds_sql_foreignkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fktable"] if val and ndx < len(val.items) else None

def cfrds_sql_foreignkeys_get_fkcolumn(val: cfrds_sql_foreignkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkcolumn"] if val and ndx < len(val.items) else None

def cfrds_sql_foreignkeys_get_key_sequence(val: cfrds_sql_foreignkeys, ndx: int) -> int:
    return val.items[ndx]["key_sequence"] if val and ndx < len(val.items) else 0

def cfrds_sql_foreignkeys_get_updaterule(val: cfrds_sql_foreignkeys, ndx: int) -> int:
    return val.items[ndx]["updaterule"] if val and ndx < len(val.items) else 0

def cfrds_sql_foreignkeys_get_deleterule(val: cfrds_sql_foreignkeys, ndx: int) -> int:
    return val.items[ndx]["deleterule"] if val and ndx < len(val.items) else 0

def cfrds_command_sql_importedkeys(srv: Optional[cfrds_server], connection_name: Optional[str], table_name: Optional[str], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or connection_name is None or table_name is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        keys = s.sql_importedkeys(connection_name, table_name)
        if isinstance(out_ptr, list):
            out_ptr[0] = cfrds_sql_importedkeys(keys)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_sql_importedkeys_free(val: Any) -> None:
    pass

def cfrds_sql_importedkeys_cleanup(val: Any) -> None:
    pass

def cfrds_sql_importedkeys_count(val: cfrds_sql_importedkeys) -> int:
    return len(val) if val else 0

def cfrds_sql_importedkeys_get_pkcatalog(val: cfrds_sql_importedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pkcatalog"] if val and ndx < len(val.items) else None

def cfrds_sql_importedkeys_get_pkowner(val: cfrds_sql_importedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pkowner"] if val and ndx < len(val.items) else None

def cfrds_sql_importedkeys_get_pktable(val: cfrds_sql_importedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pktable"] if val and ndx < len(val.items) else None

def cfrds_sql_importedkeys_get_pkcolumn(val: cfrds_sql_importedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pkcolumn"] if val and ndx < len(val.items) else None

def cfrds_sql_importedkeys_get_fkcatalog(val: cfrds_sql_importedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkcatalog"] if val and ndx < len(val.items) else None

def cfrds_sql_importedkeys_get_fkowner(val: cfrds_sql_importedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkowner"] if val and ndx < len(val.items) else None

def cfrds_sql_importedkeys_get_fktable(val: cfrds_sql_importedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fktable"] if val and ndx < len(val.items) else None

def cfrds_sql_importedkeys_get_fkcolumn(val: cfrds_sql_importedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkcolumn"] if val and ndx < len(val.items) else None

def cfrds_sql_importedkeys_get_key_sequence(val: cfrds_sql_importedkeys, ndx: int) -> int:
    return val.items[ndx]["key_sequence"] if val and ndx < len(val.items) else 0

def cfrds_sql_importedkeys_get_updaterule(val: cfrds_sql_importedkeys, ndx: int) -> int:
    return val.items[ndx]["updaterule"] if val and ndx < len(val.items) else 0

def cfrds_sql_importedkeys_get_deleterule(val: cfrds_sql_importedkeys, ndx: int) -> int:
    return val.items[ndx]["deleterule"] if val and ndx < len(val.items) else 0

def cfrds_command_sql_exportedkeys(srv: Optional[cfrds_server], connection_name: Optional[str], table_name: Optional[str], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or connection_name is None or table_name is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        keys = s.sql_exportedkeys(connection_name, table_name)
        if isinstance(out_ptr, list):
            out_ptr[0] = cfrds_sql_exportedkeys(keys)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_sql_exportedkeys_free(val: Any) -> None:
    pass

def cfrds_sql_exportedkeys_cleanup(val: Any) -> None:
    pass

def cfrds_sql_exportedkeys_count(val: cfrds_sql_exportedkeys) -> int:
    return len(val) if val else 0

def cfrds_sql_exportedkeys_get_pkcatalog(val: cfrds_sql_exportedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pkcatalog"] if val and ndx < len(val.items) else None

def cfrds_sql_exportedkeys_get_pkowner(val: cfrds_sql_exportedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pkowner"] if val and ndx < len(val.items) else None

def cfrds_sql_exportedkeys_get_pktable(val: cfrds_sql_exportedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pktable"] if val and ndx < len(val.items) else None

def cfrds_sql_exportedkeys_get_pkcolumn(val: cfrds_sql_exportedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pkcolumn"] if val and ndx < len(val.items) else None

def cfrds_sql_exportedkeys_get_fkcatalog(val: cfrds_sql_exportedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkcatalog"] if val and ndx < len(val.items) else None

def cfrds_sql_exportedkeys_get_fkowner(val: cfrds_sql_exportedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkowner"] if val and ndx < len(val.items) else None

def cfrds_sql_exportedkeys_get_fktable(val: cfrds_sql_exportedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fktable"] if val and ndx < len(val.items) else None

def cfrds_sql_exportedkeys_get_fkcolumn(val: cfrds_sql_exportedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkcolumn"] if val and ndx < len(val.items) else None

def cfrds_sql_exportedkeys_get_key_sequence(val: cfrds_sql_exportedkeys, ndx: int) -> int:
    return val.items[ndx]["key_sequence"] if val and ndx < len(val.items) else 0

def cfrds_sql_exportedkeys_get_updaterule(val: cfrds_sql_exportedkeys, ndx: int) -> int:
    return val.items[ndx]["updaterule"] if val and ndx < len(val.items) else 0

def cfrds_sql_exportedkeys_get_deleterule(val: cfrds_sql_exportedkeys, ndx: int) -> int:
    return val.items[ndx]["deleterule"] if val and ndx < len(val.items) else 0

def cfrds_command_sql_sqlstmnt(srv: Optional[cfrds_server], connection_name: Optional[str], sql: Optional[str], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or connection_name is None or sql is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        rs = s.sql_sqlstmnt(connection_name, sql)
        if isinstance(out_ptr, list):
            out_ptr[0] = cfrds_sql_resultset(rs["columns"], rs["rows"], rs["names"], rs["values"])
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_sql_resultset_free(val: Any) -> None:
    pass

def cfrds_sql_resultset_cleanup(val: Any) -> None:
    pass

def cfrds_sql_resultset_columns(val: cfrds_sql_resultset) -> int:
    return val.columns if val else 0

def cfrds_sql_resultset_rows(val: cfrds_sql_resultset) -> int:
    return val.rows if val else 0

def cfrds_sql_resultset_column_name(val: cfrds_sql_resultset, column: int) -> Optional[str]:
    return val.names[column] if val and column < len(val.names) else None

def cfrds_sql_resultset_value(val: cfrds_sql_resultset, row: int, column: int) -> Optional[str]:
    if val and row < len(val.values) and column < len(val.values[row]):
        return val.values[row][column]
    return None

def cfrds_command_sql_sqlmetadata(srv: Optional[cfrds_server], connection_name: Optional[str], sql: Optional[str], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or connection_name is None or sql is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        meta = s.sql_metadata(connection_name, sql)
        if isinstance(out_ptr, list):
            out_ptr[0] = cfrds_sql_metadata(meta)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_sql_metadata_free(val: Any) -> None:
    pass

def cfrds_sql_metadata_cleanup(val: Any) -> None:
    pass

def cfrds_sql_metadata_count(val: cfrds_sql_metadata) -> int:
    return len(val) if val else 0

def cfrds_sql_metadata_get_name(val: cfrds_sql_metadata, ndx: int) -> Optional[str]:
    return val.items[ndx]["name"] if val and ndx < len(val.items) else None

def cfrds_sql_metadata_get_type(val: cfrds_sql_metadata, ndx: int) -> Optional[str]:
    return val.items[ndx]["type"] if val and ndx < len(val.items) else None

def cfrds_sql_metadata_get_jtype(val: cfrds_sql_metadata, ndx: int) -> Optional[str]:
    return val.items[ndx]["jtype"] if val and ndx < len(val.items) else None

def cfrds_command_sql_getsupportedcommands(srv: Optional[cfrds_server], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        cmds = s.sql_getsupportedcommands()
        if isinstance(out_ptr, list):
            out_ptr[0] = cfrds_sql_supportedcommands(cmds)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_sql_supportedcommands_free(val: Any) -> None:
    pass

def cfrds_sql_supportedcommands_cleanup(val: Any) -> None:
    pass

def cfrds_sql_supportedcommands_count(val: cfrds_sql_supportedcommands) -> int:
    return len(val) if val else 0

def cfrds_sql_supportedcommands_get(val: cfrds_sql_supportedcommands, ndx: int) -> Optional[str]:
    return val.commands[ndx] if val and ndx < len(val.commands) else None

def cfrds_command_sql_dbdescription(srv: Optional[cfrds_server], connection_name: Optional[str], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or connection_name is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        desc = s.sql_dbdescription(connection_name)
        if isinstance(out_ptr, list):
            out_ptr[0] = desc
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

# Debugger Low-level Wrappers
def cfrds_command_debugger_start(srv: Optional[cfrds_server], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        sess_id = s.debugger_start()
        if isinstance(out_ptr, list):
            out_ptr[0] = sess_id
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_stop(srv: Optional[cfrds_server], session_id: Optional[str]) -> int:
    if srv is None or session_id is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_stop(session_id)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_get_server_info(srv: Optional[cfrds_server], session_id: Optional[str], out_port: Optional[List[Any]]) -> int:
    if srv is None or session_id is None or out_port is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        p = s.debugger_get_server_info(session_id)
        if isinstance(out_port, list):
            out_port[0] = p
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_breakpoint_on_exception(srv: Optional[cfrds_server], session_id: Optional[str], value: Optional[bool]) -> int:
    if srv is None or session_id is None or value is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_breakpoint_on_exception(session_id, value)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_breakpoint(srv: Optional[cfrds_server], session_id: Optional[str], filepath: Optional[str], line: Optional[int], enable: Optional[bool]) -> int:
    if srv is None or session_id is None or filepath is None or line is None or enable is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_breakpoint(session_id, filepath, line, enable)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_clear_all_breakpoints(srv: Optional[cfrds_server], session_id: Optional[str]) -> int:
    if srv is None or session_id is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_clear_all_breakpoints(session_id)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_get_debug_events(srv: Optional[cfrds_server], session_id: Optional[str], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or session_id is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        evt_dict = s.debugger_get_debug_events(session_id)
        if evt_dict and isinstance(out_ptr, list):
            evt_type = evt_dict.get("type", CFRDS_DEBUGGER_EVENT_UNKNOWN)
            out_ptr[0] = cfrds_debugger_event(evt_type, evt_dict.get("data", {}))
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_all_fetch_flags_enabled(
    srv: Optional[cfrds_server], session_id: Optional[str], threads: Optional[bool], watch: Optional[bool], scopes: Optional[bool], cf_trace: Optional[bool], java_trace: Optional[bool], out_ptr: Optional[List[Any]]
) -> int:
    if srv is None or session_id is None or threads is None or watch is None or scopes is None or cf_trace is None or java_trace is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        evt_dict = s.debugger_all_fetch_flags_enabled(session_id, threads, watch, scopes, cf_trace, java_trace)
        if evt_dict and isinstance(out_ptr, list):
            evt_type = evt_dict.get("type", CFRDS_DEBUGGER_EVENT_UNKNOWN)
            out_ptr[0] = cfrds_debugger_event(evt_type, evt_dict.get("data", {}))
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_debugger_event_free(evt: Any) -> None:
    pass

def cfrds_debugger_event_cleanup(evt: Any) -> None:
    pass

def cfrds_debugger_event_get_type(evt: cfrds_debugger_event) -> int:
    return evt.event_type if evt else CFRDS_DEBUGGER_EVENT_UNKNOWN

def cfrds_debugger_event_breakpoint_get_source(evt: cfrds_debugger_event) -> Optional[str]:
    return evt.data.get("source") if evt and evt.data else None

def cfrds_debugger_event_breakpoint_get_line(evt: cfrds_debugger_event) -> int:
    return evt.data.get("line", 0) if evt and evt.data else 0

def cfrds_debugger_event_breakpoint_get_scopes(evt: cfrds_debugger_event) -> Any:
    return evt.data.get("SCOPES") if evt and evt.data else None

def cfrds_debugger_event_breakpoint_get_thread_name(evt: cfrds_debugger_event) -> Optional[str]:
    return evt.data.get("thread_name") if evt and evt.data else None

def cfrds_debugger_event_breakpoint_set_get_pathname(evt: cfrds_debugger_event) -> Optional[str]:
    return evt.data.get("pathname") if evt and evt.data else None

def cfrds_debugger_event_breakpoint_set_get_req_line(evt: cfrds_debugger_event) -> int:
    return evt.data.get("req_line", 0) if evt and evt.data else 0

def cfrds_debugger_event_breakpoint_set_get_act_line(evt: cfrds_debugger_event) -> int:
    return evt.data.get("act_line", 0) if evt and evt.data else 0

def cfrds_debugger_event_get_scopes_count(evt: cfrds_debugger_event) -> int:
    if evt and isinstance(evt.data, dict):
        scopes = evt.data.get("SCOPES")
        return len(scopes) if isinstance(scopes, list) else 0
    return 0

def cfrds_debugger_event_get_scopes_item(evt: cfrds_debugger_event, ndx: int) -> Optional[str]:
    if evt and isinstance(evt.data, dict):
        scopes = evt.data.get("SCOPES")
        if isinstance(scopes, list) and 0 <= ndx < len(scopes):
            return scopes[ndx]
    return None

def cfrds_debugger_event_get_threads_count(evt: cfrds_debugger_event) -> int:
    if evt and isinstance(evt.data, dict):
        threads = evt.data.get("THREADS")
        return len(threads) if isinstance(threads, list) else 0
    return 0

def cfrds_debugger_event_get_threads_item(evt: cfrds_debugger_event, ndx: int) -> Optional[str]:
    if evt and isinstance(evt.data, dict):
        threads = evt.data.get("THREADS")
        if isinstance(threads, list) and 0 <= ndx < len(threads):
            return threads[ndx]
    return None

def cfrds_debugger_event_get_watch_count(evt: cfrds_debugger_event) -> int:
    if evt and isinstance(evt.data, dict):
        watch = evt.data.get("WATCH")
        return len(watch) if isinstance(watch, list) else 0
    return 0

def cfrds_debugger_event_get_watch_item(evt: cfrds_debugger_event, ndx: int) -> Optional[str]:
    if evt and isinstance(evt.data, dict):
        watch = evt.data.get("WATCH")
        if isinstance(watch, list) and 0 <= ndx < len(watch):
            return watch[ndx]
    return None

def cfrds_debugger_event_get_cf_trace_count(evt: cfrds_debugger_event) -> int:
    if evt and isinstance(evt.data, dict):
        cf_trace = evt.data.get("CF_TRACE")
        return len(cf_trace) if isinstance(cf_trace, list) else 0
    return 0

def cfrds_debugger_event_get_cf_trace_item(evt: cfrds_debugger_event, ndx: int) -> Optional[str]:
    if evt and isinstance(evt.data, dict):
        cf_trace = evt.data.get("CF_TRACE")
        if isinstance(cf_trace, list) and 0 <= ndx < len(cf_trace):
            return cf_trace[ndx]
    return None

def cfrds_debugger_event_get_java_trace_count(evt: cfrds_debugger_event) -> int:
    if evt and isinstance(evt.data, dict):
        java_trace = evt.data.get("JAVA_TRACE")
        return len(java_trace) if isinstance(java_trace, list) else 0
    return 0

def cfrds_debugger_event_get_java_trace_item(evt: cfrds_debugger_event, ndx: int) -> Optional[str]:
    if evt and isinstance(evt.data, dict):
        java_trace = evt.data.get("JAVA_TRACE")
        if isinstance(java_trace, list) and 0 <= ndx < len(java_trace):
            return java_trace[ndx]
    return None

def cfrds_command_debugger_step_in(srv: Optional[cfrds_server], session_id: Optional[str], thread_name: Optional[str]) -> int:
    if srv is None or session_id is None or thread_name is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_step_in(session_id, thread_name)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_step_over(srv: Optional[cfrds_server], session_id: Optional[str], thread_name: Optional[str]) -> int:
    if srv is None or session_id is None or thread_name is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_step_over(session_id, thread_name)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_step_out(srv: Optional[cfrds_server], session_id: Optional[str], thread_name: Optional[str]) -> int:
    if srv is None or session_id is None or thread_name is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_step_out(session_id, thread_name)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_continue(srv: Optional[cfrds_server], session_id: Optional[str], thread_name: Optional[str]) -> int:
    if srv is None or session_id is None or thread_name is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_continue(session_id, thread_name)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_watch_expression(srv: Optional[cfrds_server], session_id: Optional[str], thread_name: Optional[str], expression: Optional[str]) -> int:
    if srv is None or session_id is None or thread_name is None or expression is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_watch_expression(session_id, thread_name, expression)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_set_variable(srv: Optional[cfrds_server], session_id: Optional[str], thread_name: Optional[str], variable: Optional[str], value: Optional[str]) -> int:
    if srv is None or session_id is None or thread_name is None or variable is None or value is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_set_variable(session_id, thread_name, variable, value)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_watch_variables(srv: Optional[cfrds_server], session_id: Optional[str], variables: Optional[str]) -> int:
    if srv is None or session_id is None or variables is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_watch_variables(session_id, variables)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_get_output(srv: Optional[cfrds_server], session_id: Optional[str], thread_name: Optional[str], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or session_id is None or thread_name is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        res = s.debugger_get_output(session_id, thread_name)
        if isinstance(out_ptr, list):
            out_ptr[0] = res
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_set_scope_filter(srv: Optional[cfrds_server], session_id: Optional[str], filter_str: Optional[str]) -> int:
    if srv is None or session_id is None or filter_str is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_set_scope_filter(session_id, filter_str)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

# Security Analyzer Low-level Wrappers
def cfrds_command_security_analyzer_scan(srv: Optional[cfrds_server], pathnames: Optional[str], recursively: Optional[bool], cores: Optional[int], out_id: Optional[List[Any]]) -> int:
    if srv is None or pathnames is None or recursively is None or cores is None or out_id is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        cmd_id = s.security_analyzer_scan(pathnames, recursively, cores)
        if isinstance(out_id, list):
            out_id[0] = cmd_id
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_security_analyzer_cancel(srv: Optional[cfrds_server], command_id: Optional[int]) -> int:
    if srv is None or command_id is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.security_analyzer_cancel(command_id)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_security_analyzer_status(
    srv: Optional[cfrds_server], command_id: Optional[int], total_ptr: Optional[List[Any]], visited_ptr: Optional[List[Any]], pct_ptr: Optional[List[Any]], upd_ptr: Optional[List[Any]]
) -> int:
    if srv is None or command_id is None or total_ptr is None or visited_ptr is None or pct_ptr is None or upd_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        st_dict = s.security_analyzer_status(command_id)
        if isinstance(total_ptr, list):
            total_ptr[0] = st_dict.get("totalfiles", 0)
        if isinstance(visited_ptr, list):
            visited_ptr[0] = st_dict.get("filesvisitedcount", 0)
        if isinstance(pct_ptr, list):
            pct_ptr[0] = st_dict.get("percentage", 0)
        if isinstance(upd_ptr, list):
            upd_ptr[0] = st_dict.get("lastupdated", 0)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_security_analyzer_result(srv: Optional[cfrds_server], command_id: Optional[int], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or command_id is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        res_dict = s.security_analyzer_result(command_id)
        if isinstance(out_ptr, list) and res_dict:
            out_ptr[0] = cfrds_security_analyzer_result(res_dict)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_security_analyzer_clean(srv: Optional[cfrds_server], command_id: Optional[int]) -> int:
    if srv is None or command_id is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.security_analyzer_clean(command_id)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_security_analyzer_result_free(val: Any) -> None:
    pass

def cfrds_security_analyzer_result_cleanup(val: Any) -> None:
    pass

def cfrds_security_analyzer_result_totalfiles(val: cfrds_security_analyzer_result) -> int:
    return val.data.get("totalfiles", 0) if val and val.data else 0

def cfrds_security_analyzer_result_filesvisitedcount(val: cfrds_security_analyzer_result) -> int:
    return val.data.get("filesvisitedcount", 0) if val and val.data else 0

def cfrds_security_analyzer_result_errorsdescription_count(val: cfrds_security_analyzer_result) -> int:
    return 0

def cfrds_security_analyzer_result_filesscanned_count(val: cfrds_security_analyzer_result) -> int:
    return len(val.data.get("filesscanned", [])) if val and val.data else 0

def cfrds_security_analyzer_result_filesscanned_item_result(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    scanned = val.data.get("filesscanned", []) if val and val.data else []
    return scanned[ndx].get("result") if ndx < len(scanned) else None

def cfrds_security_analyzer_result_filesscanned_item_filename(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    scanned = val.data.get("filesscanned", []) if val and val.data else []
    return scanned[ndx].get("filename") if ndx < len(scanned) else None

def cfrds_security_analyzer_result_filesnotscanned_count(val: cfrds_security_analyzer_result) -> int:
    not_scanned = val.data.get("filesnotscanned", []) if val and val.data else []
    return len(not_scanned)

def cfrds_security_analyzer_result_filesnotscanned_item_reason(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    not_scanned = val.data.get("filesnotscanned", []) if val and val.data else []
    return not_scanned[ndx].get("reason") if ndx < len(not_scanned) else None

def cfrds_security_analyzer_result_filesnotscanned_item_filename(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    not_scanned = val.data.get("filesnotscanned", []) if val and val.data else []
    return not_scanned[ndx].get("filename") if ndx < len(not_scanned) else None

def cfrds_security_analyzer_result_executorservice(val: cfrds_security_analyzer_result) -> Optional[str]:
    return val.data.get("executorservice") if val and val.data else None

def cfrds_security_analyzer_result_percentage(val: cfrds_security_analyzer_result) -> int:
    return val.data.get("percentage", 0) if val and val.data else 0

def cfrds_security_analyzer_result_files_count(val: cfrds_security_analyzer_result) -> int:
    return 0

def cfrds_security_analyzer_result_files_value(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    return None

def cfrds_security_analyzer_result_lastupdated(val: cfrds_security_analyzer_result) -> int:
    return val.data.get("lastupdated", 0) if val and val.data else 0

def cfrds_security_analyzer_result_filesvisited_count(val: cfrds_security_analyzer_result) -> int:
    return val.data.get("filesvisitedcount", 0) if val and val.data else 0

def cfrds_security_analyzer_result_filesnotscannedcount(val: cfrds_security_analyzer_result) -> int:
    return len(val.data.get("filesnotscanned", [])) if val and val.data else 0

def cfrds_security_analyzer_result_filesscannedcount(val: cfrds_security_analyzer_result) -> int:
    return len(val.data.get("filesscanned", [])) if val and val.data else 0

def cfrds_security_analyzer_result_id(val: cfrds_security_analyzer_result) -> int:
    return val.data.get("id", 0) if val and val.data else 0

def cfrds_security_analyzer_result_errors_count(val: cfrds_security_analyzer_result) -> int:
    return len(val.data.get("errors", [])) if val and val.data else 0

def cfrds_security_analyzer_result_errors_item_errormessage(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    errs = val.data.get("errors", []) if val and val.data else []
    return errs[ndx].get("errormessage") if ndx < len(errs) else None

def cfrds_security_analyzer_result_errors_item_endline(val: cfrds_security_analyzer_result, ndx: int) -> int:
    errs = val.data.get("errors", []) if val and val.data else []
    return errs[ndx].get("endline", 0) if ndx < len(errs) else 0

def cfrds_security_analyzer_result_errors_item_path(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    errs = val.data.get("errors", []) if val and val.data else []
    return errs[ndx].get("path") if ndx < len(errs) else None

def cfrds_security_analyzer_result_errors_item_vulnerablecode(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    errs = val.data.get("errors", []) if val and val.data else []
    return errs[ndx].get("vulnerablecode") if ndx < len(errs) else None

def cfrds_security_analyzer_result_errors_item_filename(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    errs = val.data.get("errors", []) if val and val.data else []
    return errs[ndx].get("filename") if ndx < len(errs) else None

def cfrds_security_analyzer_result_errors_item_beginline(val: cfrds_security_analyzer_result, ndx: int) -> int:
    errs = val.data.get("errors", []) if val and val.data else []
    return errs[ndx].get("beginline", 0) if ndx < len(errs) else 0

def cfrds_security_analyzer_result_errors_item_column(val: cfrds_security_analyzer_result, ndx: int) -> int:
    errs = val.data.get("errors", []) if val and val.data else []
    return errs[ndx].get("column", 0) if ndx < len(errs) else 0

def cfrds_security_analyzer_result_errors_item_error(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    errs = val.data.get("errors", []) if val and val.data else []
    return errs[ndx].get("error") if ndx < len(errs) else None

def cfrds_security_analyzer_result_errors_item_begincolumn(val: cfrds_security_analyzer_result, ndx: int) -> int:
    errs = val.data.get("errors", []) if val and val.data else []
    return errs[ndx].get("begincolumn", 0) if ndx < len(errs) else 0

def cfrds_security_analyzer_result_errors_item_type(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    errs = val.data.get("errors", []) if val and val.data else []
    return errs[ndx].get("type") if ndx < len(errs) else None

def cfrds_security_analyzer_result_errors_item_endcolumn(val: cfrds_security_analyzer_result, ndx: int) -> int:
    errs = val.data.get("errors", []) if val and val.data else []
    return errs[ndx].get("endcolumn", 0) if ndx < len(errs) else 0

def cfrds_security_analyzer_result_errors_item_referencetype(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    errs = val.data.get("errors", []) if val and val.data else []
    return errs[ndx].get("referencetype") if ndx < len(errs) else None

def cfrds_security_analyzer_result_status(val: cfrds_security_analyzer_result) -> Optional[str]:
    return val.data.get("status") if val and val.data else None

# IDE Default Low-level Wrapper
def cfrds_command_ide_default(
    srv: Optional[cfrds_server], version: Optional[int], num1_ptr: Optional[List[Any]], s_ver_ptr: Optional[List[Any]], c_ver_ptr: Optional[List[Any]], num2_ptr: Optional[List[Any]], num3_ptr: Optional[List[Any]]
) -> int:
    if srv is None or version is None or num1_ptr is None or s_ver_ptr is None or c_ver_ptr is None or num2_ptr is None or num3_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        info = s.ide_default(version)
        if isinstance(num1_ptr, list):
            num1_ptr[0] = info.get("num1", 0)
        if isinstance(s_ver_ptr, list):
            s_ver_ptr[0] = info.get("server_version")
        if isinstance(c_ver_ptr, list):
            c_ver_ptr[0] = info.get("client_version")
        if isinstance(num2_ptr, list):
            num2_ptr[0] = info.get("num2", 0)
        if isinstance(num3_ptr, list):
            num3_ptr[0] = info.get("num3", 0)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

# Admin API Low-level Wrappers
def cfrds_command_adminapi_debugging_getlogproperty(srv: Optional[cfrds_server], logdirectory: Optional[str], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or logdirectory is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        res = s.adminapi_debugging_getlogproperty(logdirectory)
        if isinstance(out_ptr, list):
            out_ptr[0] = res
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_adminapi_extensions_getcustomtagpaths(srv: Optional[cfrds_server], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        paths = s.adminapi_extensions_getcustomtagpaths()
        if isinstance(out_ptr, list):
            out_ptr[0] = cfrds_adminapi_customtagpaths(paths)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_adminapi_customtagpaths_free(val: Any) -> None:
    pass

def cfrds_adminapi_customtagpaths_cleanup(val: Any) -> None:
    pass

def cfrds_adminapi_customtagpaths_count(val: cfrds_adminapi_customtagpaths) -> int:
    return len(val) if val else 0

def cfrds_adminapi_customtagpaths_at(val: cfrds_adminapi_customtagpaths, ndx: int) -> Optional[str]:
    return val.paths[ndx] if val and ndx < len(val.paths) else None

def cfrds_command_adminapi_extensions_setmapping(srv: Optional[cfrds_server], name: Optional[str], path: Optional[str]) -> int:
    if srv is None or name is None or path is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.adminapi_extensions_setmapping(name, path)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_adminapi_extensions_deletemapping(srv: Optional[cfrds_server], mapping: Optional[str]) -> int:
    if srv is None or mapping is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.adminapi_extensions_deletemapping(mapping)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_adminapi_extensions_getmappings(srv: Optional[cfrds_server], out_ptr: Optional[List[Any]]) -> int:
    if srv is None or out_ptr is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        mappings = s.adminapi_extensions_getmappings()
        if isinstance(out_ptr, list):
            out_ptr[0] = mappings
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_adminapi_mappings_free(val: Any) -> None:
    pass

def cfrds_adminapi_mappings_cleanup(val: Any) -> None:
    pass

def cfrds_adminapi_mappings_count(val: cfrds_adminapi_mappings) -> int:
    return len(val) if val else 0

def cfrds_adminapi_mappings_key(val: cfrds_adminapi_mappings, ndx: int) -> Optional[str]:
    return val.keys[ndx] if val and ndx < len(val.keys) else None

def cfrds_adminapi_mappings_value(val: cfrds_adminapi_mappings, ndx: int) -> Optional[str]:
    return val.values[ndx] if val and ndx < len(val.values) else None

# Graphing Low-level Wrapper
def cfrds_command_graphing(
    srv: Optional[cfrds_server], out_buffer_ptr: Optional[List[Any]], chart_attributes: Optional[str], num_series: Optional[int], series_data: Optional[List[str]]
) -> int:
    if srv is None or out_buffer_ptr is None or chart_attributes is None or num_series is None or series_data is None:
        return CFRDS_STATUS_PARAM_IS_NULL
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        res_bytes = s.graphing(chart_attributes, series_data[:num_series])
        if isinstance(out_buffer_ptr, list):
            out_buffer_ptr[0] = type("cfrds_buffer", (), {"data": res_bytes})
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED
