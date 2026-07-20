"""
cfrds - Pure Python implementation of the ColdFusion Remote Development Service (RDS) protocol.
This package is completely standalone and does NOT require libcfrds.so or any native C extension.
"""

from enum import IntEnum
import http.client
import os
import re
import sys
from typing import Optional, List, Dict, Any, Union, Tuple



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

    def __getitem__(self, idx: int) -> Dict[str, Any]:
        return self.items[idx]


class cfrds_file_content:
    def __init__(self, data: bytes, modified: str, permission: str):
        self.data = data
        self.size = len(data)
        self.modified = modified
        self.permission = permission


class cfrds_sql_dsninfo:
    def __init__(self, names: List[str]):
        self.names = names

    def __len__(self) -> int:
        return len(self.names)


class cfrds_sql_tableinfo:
    def __init__(self, items: List[Dict[str, Optional[str]]]):
        self.items = items

    def __len__(self) -> int:
        return len(self.items)


class cfrds_sql_columninfo:
    def __init__(self, items: List[Dict[str, Any]]):
        self.items = items

    def __len__(self) -> int:
        return len(self.items)


class cfrds_sql_primarykeys:
    def __init__(self, items: List[Dict[str, Any]]):
        self.items = items

    def __len__(self) -> int:
        return len(self.items)


class cfrds_sql_foreignkeys:
    def __init__(self, items: List[Dict[str, Any]]):
        self.items = items

    def __len__(self) -> int:
        return len(self.items)


class cfrds_sql_importedkeys:
    def __init__(self, items: List[Dict[str, Any]]):
        self.items = items

    def __len__(self) -> int:
        return len(self.items)


class cfrds_sql_exportedkeys:
    def __init__(self, items: List[Dict[str, Any]]):
        self.items = items

    def __len__(self) -> int:
        return len(self.items)


class cfrds_sql_resultset:
    def __init__(self, columns: int, rows: int, names: List[str], values: List[List[Optional[str]]]):
        self.columns = columns
        self.rows = rows
        self.names = names
        self.values = values


class cfrds_sql_metadata:
    def __init__(self, items: List[Dict[str, Optional[str]]]):
        self.items = items

    def __len__(self) -> int:
        return len(self.items)


class cfrds_sql_supportedcommands:
    def __init__(self, commands: List[str]):
        self.commands = commands

    def __len__(self) -> int:
        return len(self.commands)


class cfrds_debugger_event:
    def __init__(self, event_type: int, data: Dict[str, Any]):
        self.event_type = event_type
        self.data = data


class cfrds_security_analyzer_result:
    def __init__(self, data: Dict[str, Any]):
        self.data = data


class cfrds_adminapi_customtagpaths:
    def __init__(self, paths: List[str]):
        self.paths = paths

    def __len__(self) -> int:
        return len(self.paths)


class cfrds_adminapi_mappings:
    def __init__(self, mappings: Dict[str, str]):
        self.mappings = mappings
        self.keys = list(mappings.keys())
        self.values = list(mappings.values())

    def __len__(self) -> int:
        return len(self.keys)


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
        raw = _send_rds_command(self._ctx, "BROWSEDIR", [path, ""])
        total, offset = _parse_number(raw, 0)
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
        raw = _send_rds_command(self._ctx, "FILEIO", [filepath, "READ", ""])
        total, offset = _parse_number(raw, 0)
        data_bytes, offset = _parse_bytearray(raw, offset)
        modified, offset = _parse_string(raw, offset)
        permission, offset = _parse_string(raw, offset)
        return bytearray(data_bytes)

    def file_write(self, filepath: str, content: Union[bytes, bytearray, str]) -> None:
        if isinstance(content, str):
            data_bytes = content.encode("utf-8")
        else:
            data_bytes = bytes(content)
        _send_rds_command(self._ctx, "FILEIO", [filepath, "WRITE", "", data_bytes])

    def file_rename(self, filepath_from: str, filepath_to: str) -> None:
        _send_rds_command(self._ctx, "FILEIO", [filepath_from, "RENAME", "", filepath_to])

    def file_remove(self, filepath: str) -> None:
        _send_rds_command(self._ctx, "FILEIO", [filepath, "REMOVE", "", "F"])

    def dir_remove(self, dirpath: str) -> None:
        _send_rds_command(self._ctx, "FILEIO", [dirpath, "REMOVE", "", "D"])

    def file_exists(self, pathname: str) -> bool:
        try:
            _send_rds_command(self._ctx, "FILEIO", [pathname, "EXISTENCE", "", ""])
            return True
        except CFRDSError as e:
            if e.status == CFRDS_STATUS_COMMAND_FAILED:
                return False
            raise

    def dir_create(self, dirpath: str) -> None:
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
            if item.startswith('"') and item.endswith('"'):
                item = item[1:-1]
            dsns.append(item)
        return dsns

    def sql_tableinfo(self, connection_name: str) -> List[Dict[str, Optional[str]]]:
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
        raw = _send_rds_command(self._ctx, "DBFUNCS", [connection_name, "SQLSTMNT", sql])
        cnt, offset = _parse_number(raw, 0)
        rows = max(0, cnt - 1)
        if cnt <= 0:
            return {"columns": 0, "rows": 0, "names": [], "data": []}

        col_str, offset = _parse_string(raw, offset)
        names = _parse_string_list_item(col_str)
        cols = len(names)

        data_rows: List[List[str]] = []
        for _ in range(rows):
            r_str, offset = _parse_string(raw, offset)
            r_vals = _parse_string_list_item(r_str)
            data_rows.append(r_vals)

        return {"columns": cols, "rows": rows, "names": names, "data": data_rows}

    def sql_metadata(self, connection_name: str, sql: str) -> List[Dict[str, Optional[str]]]:
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
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_STOP", session_name])

    def debugger_get_server_info(self, session_name: Optional[str] = None) -> int:
        auto_session = False
        if session_name is None:
            session_name = self.debugger_start()
            auto_session = True
        try:
            raw = _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_GET_DEBUG_SERVER_INFO", session_name])
            _, offset = _parse_number(raw, 0)
            wddx_xml, _ = _parse_string(raw, offset)
            m = re.search(r"<var name=['\"]DEBUG_SERVER_PORT['\"]>\s*<number>(\d+(?:\.\d+)?)</number>", wddx_xml, re.IGNORECASE)
            return int(float(m.group(1))) if m else 0
        finally:
            if auto_session:
                self.debugger_stop(session_name)

    def debugger_breakpoint_on_exception(self, session_name: str, enable: bool) -> None:
        val = "true" if enable else "false"
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SESSION_BREAK_ON_EXCEPTION</string></var><var name='BREAK_ON_EXCEPTION'><boolean value='{val}'/></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_breakpoint(self, session_name: str, filepath: str, line: int, enable: bool) -> None:
        cmd = "SET_BREAKPOINT" if enable else "UNSET_BREAKPOINT"
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>{cmd}</string></var><var name='FILE'><string>{filepath}</string></var><var name='Y'><number>{line}</number></var><var name='SEQ'><number>1.0</number></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_clear_all_breakpoints(self, session_name: str) -> None:
        wddx = "<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>UNSET_ALL_BREAKPOINTS</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_get_debug_events(self, session_name: str) -> Optional[Dict[str, Any]]:
        raw = _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_EVENTS", session_name])
        evt_type, offset = _parse_number(raw, 0)
        if evt_type == CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT_SET:
            pathname, offset = _parse_string(raw, offset)
            req_line_str, offset = _parse_string(raw, offset)
            act_line_str, offset = _parse_string(raw, offset)
            return {
                "pathname": pathname,
                "req_line": int(req_line_str) if req_line_str.isdigit() else 0,
                "act_line": int(act_line_str) if act_line_str.isdigit() else 0,
            }
        elif evt_type in (CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT, CFRDS_DEBUGGER_EVENT_TYPE_STEP):
            source, offset = _parse_string(raw, offset)
            line_str, offset = _parse_string(raw, offset)
            thread_name, offset = _parse_string(raw, offset)
            return {
                "source": source,
                "line": int(line_str) if line_str.isdigit() else 0,
                "thread_name": thread_name,
            }
        return None

    def debugger_all_fetch_flags_enabled(
        self, session_name: str, threads: bool, watch: bool, scopes: bool, cf_trace: bool, java_trace: bool
    ) -> None:
        def b(v: bool) -> str:
            return "true" if v else "false"
        wddx = (f"<wddxPacket version='1.0'><header/><data><struct type='java.util.HashMap'>"
                f"<var name='THREADS'><boolean value='{b(threads)}'/></var>"
                f"<var name='WATCH'><boolean value='{b(watch)}'/></var>"
                f"<var name='SCOPES'><boolean value='{b(scopes)}'/></var>"
                f"<var name='CF_TRACE'><boolean value='{b(cf_trace)}'/></var>"
                f"<var name='JAVA_TRACE'><boolean value='{b(java_trace)}'/></var>"
                f"</struct></data></wddxPacket>")
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_EVENTS", session_name, wddx])

    def debugger_step_in(self, session_name: str, thread_name: str) -> None:
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_IN</string></var><var name='THREAD'><string>{thread_name}</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_step_over(self, session_name: str, thread_name: str) -> None:
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_OVER</string></var><var name='THREAD'><string>{thread_name}</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_step_out(self, session_name: str, thread_name: str) -> None:
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>STEP_OUT</string></var><var name='THREAD'><string>{thread_name}</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_continue(self, session_name: str, thread_name: str) -> None:
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>CONTINUE</string></var><var name='THREAD'><string>{thread_name}</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_watch_expression(self, session_name: str, thread_name: str, expression: str) -> None:
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>GET_SINGLE_CF_VARIABLE</string></var><var name='VARIABLE_NAME'><string>{expression}</string></var><var name='THREAD'><string>{thread_name}</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_set_variable(self, session_name: str, thread_name: str, variable: str, value: str) -> None:
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_VARIABLE_VALUE</string></var><var name='VARIABLE_NAME'><string>{variable}</string></var><var name='VARIABLE_VALUE'><string>{value}</string></var><var name='THREAD'><string>{thread_name}</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_watch_variables(self, session_name: str, variables: str) -> None:
        vars_list = [v.strip() for v in variables.split(",") if v.strip()]
        var_tags = "".join([f"<var name='WATCH_{i}'><string>{v}</string></var>" for i, v in enumerate(vars_list)])
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_WATCH_VARIABLES</string></var>{var_tags}</struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_get_output(self, session_name: str, thread_name: str) -> None:
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='BODY_ONLY'><boolean value='true'/></var><var name='THREAD'><string>{thread_name}</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    def debugger_set_scope_filter(self, session_name: str, filter_str: str) -> None:
        wddx = f"<wddxPacket version='1.0'><header/><data><array length='1'><struct type='java.util.HashMap'><var name='COMMAND'><string>SET_SCOPE_FILTER</string></var><var name='FILTER'><string>{filter_str}</string></var></struct></array></data></wddxPacket>"
        _send_rds_command(self._ctx, "DBGREQUEST", ["DBG_REQUEST", session_name, wddx])

    # Security Analyzer Operations
    def security_analyzer_scan(self, pathnames: str, recursively: bool = True, cores: int = 1) -> int:
        raw = _send_rds_command(
            self._ctx, "SECURITYANALYZER", ["scan", pathnames, "1" if recursively else "0", str(cores)]
        )
        cmd_str, _ = _parse_string(raw, 0)
        return int(cmd_str) if cmd_str.isdigit() else 0

    def security_analyzer_cancel(self, command_id: int) -> None:
        _send_rds_command(self._ctx, "SECURITYANALYZER", ["cancel", str(command_id)])

    def security_analyzer_status(self, command_id: int) -> Dict[str, Any]:
        raw = _send_rds_command(self._ctx, "SECURITYANALYZER", ["status", str(command_id)])
        tot_str, offset = _parse_string(raw, 0)
        vis_str, offset = _parse_string(raw, offset)
        pct_str, offset = _parse_string(raw, offset)
        upd_str, offset = _parse_string(raw, offset)
        return {
            "totalfiles": int(tot_str) if tot_str.isdigit() else 0,
            "filesvisitedcount": int(vis_str) if vis_str.isdigit() else 0,
            "percentage": int(pct_str) if pct_str.isdigit() else 0,
            "lastupdated": int(upd_str) if upd_str.isdigit() else 0,
        }

    def security_analyzer_result(self, command_id: int) -> Optional[Dict[str, Any]]:
        raw = _send_rds_command(self._ctx, "SECURITYANALYZER", ["result", str(command_id)])
        res_str, _ = _parse_string(raw, 0)
        import json
        try:
            return json.loads(res_str)
        except Exception:
            return {"raw": res_str}

    def security_analyzer_clean(self, command_id: int) -> None:
        _send_rds_command(self._ctx, "SECURITYANALYZER", ["clean", str(command_id)])

    # IDE Default
    def ide_default(self, version: int = 1) -> Dict[str, Any]:
        raw = _send_rds_command(self._ctx, "IDE_DEFAULT", ["", str(version)])
        n1, offset = _parse_string(raw, 0)
        s_ver, offset = _parse_string(raw, offset)
        c_ver, offset = _parse_string(raw, offset)
        n2, offset = _parse_string(raw, offset)
        n3, offset = _parse_string(raw, offset)
        return {
            "num1": int(n1) if n1.isdigit() else 0,
            "server_version": s_ver,
            "client_version": c_ver,
            "num2": int(n2) if n2.isdigit() else 0,
            "num3": int(n3) if n3.isdigit() else 0,
        }

    # Admin API Operations
    def adminapi_debugging_getlogproperty(self, logdirectory: str) -> Optional[str]:
        raw = _send_rds_command(self._ctx, "ADMINAPI", ["cfide.adminapi.debugging", "getlogproperty", logdirectory])
        prop_str, _ = _parse_string(raw, 0)
        return prop_str

    def adminapi_extensions_getcustomtagpaths(self) -> List[str]:
        raw = _send_rds_command(self._ctx, "ADMINAPI", ["cfide.adminapi.extensions", "getcustomtagpaths"])
        cnt, offset = _parse_number(raw, 0)
        paths: List[str] = []
        for _ in range(cnt):
            p, offset = _parse_string(raw, offset)
            paths.append(p)
        return paths

    def adminapi_extensions_setmapping(self, name: str, path: str) -> None:
        # The C code serializes name+path into a WDDX struct before sending
        wddx = f"<wddxPacket version='1.0'><header/><data><struct><var name='{name}'><string>{path}</string></var></struct></data></wddxPacket>"
        _send_rds_command(self._ctx, "ADMINAPI", ["cfide.adminapi.extensions", "setmappings", wddx])

    def adminapi_extensions_deletemapping(self, mapping: str) -> None:
        _send_rds_command(self._ctx, "ADMINAPI", ["cfide.adminapi.extensions", "deleltemappings", mapping])

    def adminapi_extensions_getmappings(self) -> Dict[str, str]:
        raw = _send_rds_command(self._ctx, "ADMINAPI", ["cfide.adminapi.extensions", "getmappings"])
        cnt, offset = _parse_number(raw, 0)
        mappings: Dict[str, str] = {}
        for _ in range(cnt):
            k, offset = _parse_string(raw, offset)
            v, offset = _parse_string(raw, offset)
            mappings[k] = v
        return mappings

    # Graphing Operations
    def graphing(self, chart_attributes: str, series_data: List[str]) -> bytes:
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
def cfrds_command_browse_dir(srv: cfrds_server, path: str, out_ptr: List[Any]) -> int:
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
    return val.items[ndx]["kind"]

def cfrds_browse_dir_item_get_name(val: cfrds_browse_dir, ndx: int) -> Optional[str]:
    return val.items[ndx]["name"]

def cfrds_browse_dir_item_get_permissions(val: cfrds_browse_dir, ndx: int) -> int:
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
    return val.items[ndx]["size"]

def cfrds_browse_dir_item_get_modified(val: cfrds_browse_dir, ndx: int) -> int:
    return val.items[ndx]["modified"]

def cfrds_command_file_read(srv: cfrds_server, pathname: str, out_ptr: List[Any]) -> int:
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

def cfrds_command_file_write(srv: cfrds_server, pathname: str, data: bytes, length: int) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.file_write(pathname, data[:length])
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_file_rename(srv: cfrds_server, current_pathname: str, new_pathname: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.file_rename(current_pathname, new_pathname)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_file_remove_file(srv: cfrds_server, pathname: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.file_remove(pathname)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_file_remove_dir(srv: cfrds_server, path: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.dir_remove(path)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_file_exists(srv: cfrds_server, pathname: str, out_ptr: List[Any]) -> int:
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

def cfrds_command_file_create_dir(srv: cfrds_server, path: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.dir_create(path)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_file_get_root_dir(srv: cfrds_server, out_ptr: List[Any]) -> int:
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
def cfrds_command_sql_dsninfo(srv: cfrds_server, out_ptr: List[Any]) -> int:
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
    return val.names[ndx]

def cfrds_command_sql_tableinfo(srv: cfrds_server, connection_name: str, out_ptr: List[Any]) -> int:
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
    return val.items[col]["unknown"]

def cfrds_sql_tableinfo_get_column_schema(val: cfrds_sql_tableinfo, col: int) -> Optional[str]:
    return val.items[col]["schema"]

def cfrds_sql_tableinfo_get_column_name(val: cfrds_sql_tableinfo, col: int) -> Optional[str]:
    return val.items[col]["name"]

def cfrds_sql_tableinfo_get_column_type(val: cfrds_sql_tableinfo, col: int) -> Optional[str]:
    return val.items[col]["type"]

def cfrds_command_sql_columninfo(srv: cfrds_server, connection_name: str, table_name: str, out_ptr: List[Any]) -> int:
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
    return val.items[col]["schema"]

def cfrds_sql_columninfo_get_owner(val: cfrds_sql_columninfo, col: int) -> Optional[str]:
    return val.items[col]["owner"]

def cfrds_sql_columninfo_get_table(val: cfrds_sql_columninfo, col: int) -> Optional[str]:
    return val.items[col]["table"]

def cfrds_sql_columninfo_get_name(val: cfrds_sql_columninfo, col: int) -> Optional[str]:
    return val.items[col]["name"]

def cfrds_sql_columninfo_get_type(val: cfrds_sql_columninfo, col: int) -> int:
    return val.items[col]["type"]

def cfrds_sql_columninfo_get_typeStr(val: cfrds_sql_columninfo, col: int) -> Optional[str]:
    return val.items[col]["typeStr"]

def cfrds_sql_columninfo_get_precision(val: cfrds_sql_columninfo, col: int) -> int:
    return val.items[col]["precision"]

def cfrds_sql_columninfo_get_length(val: cfrds_sql_columninfo, col: int) -> int:
    return val.items[col]["length"]

def cfrds_sql_columninfo_get_scale(val: cfrds_sql_columninfo, col: int) -> int:
    return val.items[col]["scale"]

def cfrds_sql_columninfo_get_radix(val: cfrds_sql_columninfo, col: int) -> int:
    return val.items[col]["radix"]

def cfrds_sql_columninfo_get_nullable(val: cfrds_sql_columninfo, col: int) -> int:
    return val.items[col]["nullable"]

def cfrds_command_sql_primarykeys(srv: cfrds_server, connection_name: str, table_name: str, out_ptr: List[Any]) -> int:
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
    return val.items[ndx]["catalog"]

def cfrds_sql_primarykeys_get_owner(val: cfrds_sql_primarykeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["owner"]

def cfrds_sql_primarykeys_get_table(val: cfrds_sql_primarykeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["table"]

def cfrds_sql_primarykeys_get_column(val: cfrds_sql_primarykeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["column"]

def cfrds_sql_primarykeys_get_key_sequence(val: cfrds_sql_primarykeys, ndx: int) -> int:
    return val.items[ndx]["key_sequence"]

def cfrds_command_sql_foreignkeys(srv: cfrds_server, connection_name: str, table_name: str, out_ptr: List[Any]) -> int:
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
    return val.items[ndx]["pkcatalog"]

def cfrds_sql_foreignkeys_get_pkowner(val: cfrds_sql_foreignkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pkowner"]

def cfrds_sql_foreignkeys_get_pktable(val: cfrds_sql_foreignkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pktable"]

def cfrds_sql_foreignkeys_get_pkcolumn(val: cfrds_sql_foreignkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pkcolumn"]

def cfrds_sql_foreignkeys_get_fkcatalog(val: cfrds_sql_foreignkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkcatalog"]

def cfrds_sql_foreignkeys_get_fkowner(val: cfrds_sql_foreignkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkowner"]

def cfrds_sql_foreignkeys_get_fktable(val: cfrds_sql_foreignkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fktable"]

def cfrds_sql_foreignkeys_get_fkcolumn(val: cfrds_sql_foreignkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkcolumn"]

def cfrds_sql_foreignkeys_get_key_sequence(val: cfrds_sql_foreignkeys, ndx: int) -> int:
    return val.items[ndx]["key_sequence"]

def cfrds_sql_foreignkeys_get_updaterule(val: cfrds_sql_foreignkeys, ndx: int) -> int:
    return val.items[ndx]["updaterule"]

def cfrds_sql_foreignkeys_get_deleterule(val: cfrds_sql_foreignkeys, ndx: int) -> int:
    return val.items[ndx]["deleterule"]

def cfrds_command_sql_importedkeys(srv: cfrds_server, connection_name: str, table_name: str, out_ptr: List[Any]) -> int:
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
    return val.items[ndx]["pkcatalog"]

def cfrds_sql_importedkeys_get_pkowner(val: cfrds_sql_importedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pkowner"]

def cfrds_sql_importedkeys_get_pktable(val: cfrds_sql_importedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pktable"]

def cfrds_sql_importedkeys_get_pkcolumn(val: cfrds_sql_importedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pkcolumn"]

def cfrds_sql_importedkeys_get_fkcatalog(val: cfrds_sql_importedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkcatalog"]

def cfrds_sql_importedkeys_get_fkowner(val: cfrds_sql_importedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkowner"]

def cfrds_sql_importedkeys_get_fktable(val: cfrds_sql_importedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fktable"]

def cfrds_sql_importedkeys_get_fkcolumn(val: cfrds_sql_importedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkcolumn"]

def cfrds_sql_importedkeys_get_key_sequence(val: cfrds_sql_importedkeys, ndx: int) -> int:
    return val.items[ndx]["key_sequence"]

def cfrds_sql_importedkeys_get_updaterule(val: cfrds_sql_importedkeys, ndx: int) -> int:
    return val.items[ndx]["updaterule"]

def cfrds_sql_importedkeys_get_deleterule(val: cfrds_sql_importedkeys, ndx: int) -> int:
    return val.items[ndx]["deleterule"]

def cfrds_command_sql_exportedkeys(srv: cfrds_server, connection_name: str, table_name: str, out_ptr: List[Any]) -> int:
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
    return val.items[ndx]["pkcatalog"]

def cfrds_sql_exportedkeys_get_pkowner(val: cfrds_sql_exportedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pkowner"]

def cfrds_sql_exportedkeys_get_pktable(val: cfrds_sql_exportedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pktable"]

def cfrds_sql_exportedkeys_get_pkcolumn(val: cfrds_sql_exportedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["pkcolumn"]

def cfrds_sql_exportedkeys_get_fkcatalog(val: cfrds_sql_exportedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkcatalog"]

def cfrds_sql_exportedkeys_get_fkowner(val: cfrds_sql_exportedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkowner"]

def cfrds_sql_exportedkeys_get_fktable(val: cfrds_sql_exportedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fktable"]

def cfrds_sql_exportedkeys_get_fkcolumn(val: cfrds_sql_exportedkeys, ndx: int) -> Optional[str]:
    return val.items[ndx]["fkcolumn"]

def cfrds_sql_exportedkeys_get_key_sequence(val: cfrds_sql_exportedkeys, ndx: int) -> int:
    return val.items[ndx]["key_sequence"]

def cfrds_sql_exportedkeys_get_updaterule(val: cfrds_sql_exportedkeys, ndx: int) -> int:
    return val.items[ndx]["updaterule"]

def cfrds_sql_exportedkeys_get_deleterule(val: cfrds_sql_exportedkeys, ndx: int) -> int:
    return val.items[ndx]["deleterule"]

def cfrds_command_sql_sqlstmnt(srv: cfrds_server, connection_name: str, sql: str, out_ptr: List[Any]) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        rs = s.sql_sqlstmnt(connection_name, sql)
        if isinstance(out_ptr, list):
            out_ptr[0] = cfrds_sql_resultset(rs["columns"], rs["rows"], rs["names"], rs["data"])
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

def cfrds_command_sql_sqlmetadata(srv: cfrds_server, connection_name: str, sql: str, out_ptr: List[Any]) -> int:
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
    return val.items[ndx]["name"]

def cfrds_sql_metadata_get_type(val: cfrds_sql_metadata, ndx: int) -> Optional[str]:
    return val.items[ndx]["type"]

def cfrds_sql_metadata_get_jtype(val: cfrds_sql_metadata, ndx: int) -> Optional[str]:
    return val.items[ndx]["jtype"]

def cfrds_command_sql_getsupportedcommands(srv: cfrds_server, out_ptr: List[Any]) -> int:
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
    return val.commands[ndx]

def cfrds_command_sql_dbdescription(srv: cfrds_server, connection_name: str, out_ptr: List[Any]) -> int:
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
def cfrds_command_debugger_start(srv: cfrds_server, out_ptr: List[Any]) -> int:
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

def cfrds_command_debugger_stop(srv: cfrds_server, session_id: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_stop(session_id)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_get_server_info(srv: cfrds_server, session_id: str, out_port: List[Any]) -> int:
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

def cfrds_command_debugger_breakpoint_on_exception(srv: cfrds_server, session_id: str, value: bool) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_breakpoint_on_exception(session_id, value)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_breakpoint(srv: cfrds_server, session_id: str, filepath: str, line: int, enable: bool) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_breakpoint(session_id, filepath, line, enable)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_clear_all_breakpoints(srv: cfrds_server, session_id: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_clear_all_breakpoints(session_id)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_get_debug_events(srv: cfrds_server, session_id: str, out_ptr: List[Any]) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        evt_dict = s.debugger_get_debug_events(session_id)
        if evt_dict and isinstance(out_ptr, list):
            evt_type = CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT_SET if "pathname" in evt_dict else CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT
            out_ptr[0] = cfrds_debugger_event(evt_type, evt_dict)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_all_fetch_flags_enabled(
    srv: cfrds_server, session_id: str, threads: bool, watch: bool, scopes: bool, cf_trace: bool, java_trace: bool, out_ptr: List[Any]
) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_all_fetch_flags_enabled(session_id, threads, watch, scopes, cf_trace, java_trace)
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
    return evt.data.get("source") if evt else None

def cfrds_debugger_event_breakpoint_get_line(evt: cfrds_debugger_event) -> int:
    return evt.data.get("line", 0) if evt else 0

def cfrds_debugger_event_breakpoint_get_scopes(evt: cfrds_debugger_event) -> Any:
    return None

def cfrds_debugger_event_breakpoint_get_thread_name(evt: cfrds_debugger_event) -> Optional[str]:
    return evt.data.get("thread_name") if evt else None

def cfrds_debugger_event_breakpoint_set_get_pathname(evt: cfrds_debugger_event) -> Optional[str]:
    return evt.data.get("pathname") if evt else None

def cfrds_debugger_event_breakpoint_set_get_req_line(evt: cfrds_debugger_event) -> int:
    return evt.data.get("req_line", 0) if evt else 0

def cfrds_debugger_event_breakpoint_set_get_act_line(evt: cfrds_debugger_event) -> int:
    return evt.data.get("act_line", 0) if evt else 0

def cfrds_debugger_event_get_scopes_count(evt: cfrds_debugger_event) -> int:
    return 0

def cfrds_debugger_event_get_scopes_item(evt: cfrds_debugger_event, ndx: int) -> Optional[str]:
    return None

def cfrds_debugger_event_get_threads_count(evt: cfrds_debugger_event) -> int:
    return 0

def cfrds_debugger_event_get_threads_item(evt: cfrds_debugger_event, ndx: int) -> Optional[str]:
    return None

def cfrds_debugger_event_get_watch_count(evt: cfrds_debugger_event) -> int:
    return 0

def cfrds_debugger_event_get_watch_item(evt: cfrds_debugger_event, ndx: int) -> Optional[str]:
    return None

def cfrds_debugger_event_get_cf_trace_count(evt: cfrds_debugger_event) -> int:
    return 0

def cfrds_debugger_event_get_cf_trace_item(evt: cfrds_debugger_event, ndx: int) -> Optional[str]:
    return None

def cfrds_debugger_event_get_java_trace_count(evt: cfrds_debugger_event) -> int:
    return 0

def cfrds_debugger_event_get_java_trace_item(evt: cfrds_debugger_event, ndx: int) -> Optional[str]:
    return None

def cfrds_command_debugger_step_in(srv: cfrds_server, session_id: str, thread_name: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_step_in(session_id, thread_name)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_step_over(srv: cfrds_server, session_id: str, thread_name: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_step_over(session_id, thread_name)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_step_out(srv: cfrds_server, session_id: str, thread_name: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_step_out(session_id, thread_name)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_continue(srv: cfrds_server, session_id: str, thread_name: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_continue(session_id, thread_name)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_watch_expression(srv: cfrds_server, session_id: str, thread_name: str, expression: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_watch_expression(session_id, thread_name, expression)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_set_variable(srv: cfrds_server, session_id: str, thread_name: str, variable: str, value: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_set_variable(session_id, thread_name, variable, value)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_watch_variables(srv: cfrds_server, session_id: str, variables: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_watch_variables(session_id, variables)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_get_output(srv: cfrds_server, session_id: str, thread_name: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.debugger_get_output(session_id, thread_name)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_debugger_set_scope_filter(srv: cfrds_server, session_id: str, filter_str: str) -> int:
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
def cfrds_command_security_analyzer_scan(srv: cfrds_server, pathnames: str, recursively: bool, cores: int, out_id: List[Any]) -> int:
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

def cfrds_command_security_analyzer_cancel(srv: cfrds_server, command_id: int) -> int:
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
    srv: cfrds_server, command_id: int, total_ptr: List[Any], visited_ptr: List[Any], pct_ptr: List[Any], upd_ptr: List[Any]
) -> int:
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

def cfrds_command_security_analyzer_result(srv: cfrds_server, command_id: int, out_ptr: List[Any]) -> int:
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

def cfrds_command_security_analyzer_clean(srv: cfrds_server, command_id: int) -> int:
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
    return val.data.get("totalfiles", 0) if val else 0

def cfrds_security_analyzer_result_filesvisitedcount(val: cfrds_security_analyzer_result) -> int:
    return val.data.get("filesvisitedcount", 0) if val else 0

def cfrds_security_analyzer_result_errorsdescription_count(val: cfrds_security_analyzer_result) -> int:
    return 0

def cfrds_security_analyzer_result_filesscanned_count(val: cfrds_security_analyzer_result) -> int:
    return len(val.data.get("filesscanned", [])) if val else 0

def cfrds_security_analyzer_result_filesscanned_item_result(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    scanned = val.data.get("filesscanned", []) if val else []
    return scanned[ndx].get("result") if ndx < len(scanned) else None

def cfrds_security_analyzer_result_filesscanned_item_filename(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    scanned = val.data.get("filesscanned", []) if val else []
    return scanned[ndx].get("filename") if ndx < len(scanned) else None

def cfrds_security_analyzer_result_filesnotscanned_count(val: cfrds_security_analyzer_result) -> int:
    not_scanned = val.data.get("filesnotscanned", []) if val else []
    return len(not_scanned)

def cfrds_security_analyzer_result_filesnotscanned_item_reason(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    not_scanned = val.data.get("filesnotscanned", []) if val else []
    return not_scanned[ndx].get("reason") if ndx < len(not_scanned) else None

def cfrds_security_analyzer_result_filesnotscanned_item_filename(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    not_scanned = val.data.get("filesnotscanned", []) if val else []
    return not_scanned[ndx].get("filename") if ndx < len(not_scanned) else None

def cfrds_security_analyzer_result_executorservice(val: cfrds_security_analyzer_result) -> Optional[str]:
    return val.data.get("executorservice") if val else None

def cfrds_security_analyzer_result_percentage(val: cfrds_security_analyzer_result) -> int:
    return val.data.get("percentage", 0) if val else 0

def cfrds_security_analyzer_result_files_count(val: cfrds_security_analyzer_result) -> int:
    return 0

def cfrds_security_analyzer_result_files_value(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    return None

def cfrds_security_analyzer_result_lastupdated(val: cfrds_security_analyzer_result) -> int:
    return val.data.get("lastupdated", 0) if val else 0

def cfrds_security_analyzer_result_filesvisited_count(val: cfrds_security_analyzer_result) -> int:
    return val.data.get("filesvisitedcount", 0) if val else 0

def cfrds_security_analyzer_result_filesnotscannedcount(val: cfrds_security_analyzer_result) -> int:
    return len(val.data.get("filesnotscanned", [])) if val else 0

def cfrds_security_analyzer_result_filesscannedcount(val: cfrds_security_analyzer_result) -> int:
    return len(val.data.get("filesscanned", [])) if val else 0

def cfrds_security_analyzer_result_id(val: cfrds_security_analyzer_result) -> int:
    return val.data.get("id", 0) if val else 0

def cfrds_security_analyzer_result_errors_count(val: cfrds_security_analyzer_result) -> int:
    return len(val.data.get("errors", [])) if val else 0

def cfrds_security_analyzer_result_errors_item_errormessage(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    errs = val.data.get("errors", []) if val else []
    return errs[ndx].get("errormessage") if ndx < len(errs) else None

def cfrds_security_analyzer_result_errors_item_endline(val: cfrds_security_analyzer_result, ndx: int) -> int:
    errs = val.data.get("errors", []) if val else []
    return errs[ndx].get("endline", 0) if ndx < len(errs) else 0

def cfrds_security_analyzer_result_errors_item_path(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    errs = val.data.get("errors", []) if val else []
    return errs[ndx].get("path") if ndx < len(errs) else None

def cfrds_security_analyzer_result_errors_item_vulnerablecode(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    errs = val.data.get("errors", []) if val else []
    return errs[ndx].get("vulnerablecode") if ndx < len(errs) else None

def cfrds_security_analyzer_result_errors_item_filename(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    errs = val.data.get("errors", []) if val else []
    return errs[ndx].get("filename") if ndx < len(errs) else None

def cfrds_security_analyzer_result_errors_item_beginline(val: cfrds_security_analyzer_result, ndx: int) -> int:
    errs = val.data.get("errors", []) if val else []
    return errs[ndx].get("beginline", 0) if ndx < len(errs) else 0

def cfrds_security_analyzer_result_errors_item_column(val: cfrds_security_analyzer_result, ndx: int) -> int:
    errs = val.data.get("errors", []) if val else []
    return errs[ndx].get("column", 0) if ndx < len(errs) else 0

def cfrds_security_analyzer_result_errors_item_error(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    errs = val.data.get("errors", []) if val else []
    return errs[ndx].get("error") if ndx < len(errs) else None

def cfrds_security_analyzer_result_errors_item_begincolumn(val: cfrds_security_analyzer_result, ndx: int) -> int:
    errs = val.data.get("errors", []) if val else []
    return errs[ndx].get("begincolumn", 0) if ndx < len(errs) else 0

def cfrds_security_analyzer_result_errors_item_type(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    errs = val.data.get("errors", []) if val else []
    return errs[ndx].get("type") if ndx < len(errs) else None

def cfrds_security_analyzer_result_errors_item_endcolumn(val: cfrds_security_analyzer_result, ndx: int) -> int:
    errs = val.data.get("errors", []) if val else []
    return errs[ndx].get("endcolumn", 0) if ndx < len(errs) else 0

def cfrds_security_analyzer_result_errors_item_referencetype(val: cfrds_security_analyzer_result, ndx: int) -> Optional[str]:
    errs = val.data.get("errors", []) if val else []
    return errs[ndx].get("referencetype") if ndx < len(errs) else None

def cfrds_security_analyzer_result_status(val: cfrds_security_analyzer_result) -> Optional[str]:
    return val.data.get("status") if val else None

# IDE Default Low-level Wrapper
def cfrds_command_ide_default(
    srv: cfrds_server, version: int, num1_ptr: List[Any], s_ver_ptr: List[Any], c_ver_ptr: List[Any], num2_ptr: List[Any], num3_ptr: List[Any]
) -> int:
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
def cfrds_command_adminapi_debugging_getlogproperty(srv: cfrds_server, logdirectory: str, out_ptr: List[Any]) -> int:
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

def cfrds_command_adminapi_extensions_getcustomtagpaths(srv: cfrds_server, out_ptr: List[Any]) -> int:
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

def cfrds_command_adminapi_extensions_setmapping(srv: cfrds_server, name: str, path: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.adminapi_extensions_setmapping(name, path)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_adminapi_extensions_deletemapping(srv: cfrds_server, mapping: str) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        s.adminapi_extensions_deletemapping(mapping)
        return CFRDS_STATUS_OK
    except CFRDSError as e:
        return e.status
    except Exception as e:
        srv.set_error(CFRDS_STATUS_COMMAND_FAILED, str(e))
        return CFRDS_STATUS_COMMAND_FAILED

def cfrds_command_adminapi_extensions_getmappings(srv: cfrds_server, out_ptr: List[Any]) -> int:
    try:
        s = server(srv.host, srv.port, srv.username, srv.orig_password)
        mappings = s.adminapi_extensions_getmappings()
        if isinstance(out_ptr, list):
            out_ptr[0] = cfrds_adminapi_mappings(mappings)
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
    srv: cfrds_server, out_buffer_ptr: List[Any], chart_attributes: str, num_series: int, series_data: List[str]
) -> int:
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
