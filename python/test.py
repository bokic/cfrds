#!/usr/bin/env python3

import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import cfrds

rds_vars = {k: v for k, v in os.environ.items() if k.startswith("RDS_")}
if rds_vars:
    print("RDS environment variables:")
    for k, v in sorted(rds_vars.items()):
        print(f"  {k}={v}")
else:
    print("No RDS_* environment variables set.")

print("Testing pure Python cfrds module exports...")

# Verify status enums and constants
assert cfrds.CFRDS_STATUS_OK == 0
assert cfrds.cfrds_status.CFRDS_STATUS_OK == 0
assert cfrds.CFRDS_STATUS_COMMAND_FAILED == 6
assert cfrds.cfrds_debugger_type.CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT == 1

# Verify exported C API functions from cfrds.h exist
c_api_functions = [
    "cfrds_server_init",
    "cfrds_server_free",
    "cfrds_command_browse_dir",
    "cfrds_browse_dir_count",
    "cfrds_command_file_read",
    "cfrds_command_file_write",
    "cfrds_command_file_rename",
    "cfrds_command_file_remove_file",
    "cfrds_command_file_remove_dir",
    "cfrds_command_file_exists",
    "cfrds_command_file_create_dir",
    "cfrds_command_file_get_root_dir",
    "cfrds_command_sql_dsninfo",
    "cfrds_command_sql_tableinfo",
    "cfrds_command_sql_columninfo",
    "cfrds_command_sql_primarykeys",
    "cfrds_command_sql_foreignkeys",
    "cfrds_command_sql_importedkeys",
    "cfrds_command_sql_exportedkeys",
    "cfrds_command_sql_sqlstmnt",
    "cfrds_command_sql_sqlmetadata",
    "cfrds_command_sql_getsupportedcommands",
    "cfrds_command_sql_dbdescription",
    "cfrds_command_debugger_start",
    "cfrds_command_debugger_stop",
    "cfrds_command_debugger_get_server_info",
    "cfrds_command_security_analyzer_scan",
    "cfrds_command_ide_default",
]

for func in c_api_functions:
    assert hasattr(cfrds, func), f"Missing exported C API function: {func}"
    assert callable(getattr(cfrds, func)), f"{func} is not callable"

print(f"All {len(c_api_functions)} C API functions successfully verified!")

# Verify server class methods
server_methods = [
    "get_host",
    "get_port",
    "get_username",
    "get_password",
    "close",
    "browse_dir",
    "file_read",
    "file_write",
    "file_rename",
    "file_remove",
    "dir_remove",
    "file_exists",
    "dir_create",
    "cf_root_dir",
    "sql_dsninfo",
    "sql_tableinfo",
    "sql_columninfo",
    "sql_primarykeys",
    "sql_foreignkeys",
    "sql_importedkeys",
    "sql_exportedkeys",
    "sql_sqlstmnt",
    "sql_metadata",
    "sql_getsupportedcommands",
    "sql_dbdescription",
    "debugger_start",
    "debugger_stop",
    "debugger_get_server_info",
    "debugger_breakpoint_on_exception",
    "debugger_breakpoint",
    "debugger_clear_all_breakpoints",
    "debugger_get_debug_events",
    "debugger_all_fetch_flags_enabled",
    "debugger_step_in",
    "debugger_step_over",
    "debugger_step_out",
    "debugger_continue",
    "debugger_watch_expression",
    "debugger_set_variable",
    "debugger_watch_variables",
    "debugger_get_output",
    "debugger_set_scope_filter",
    "security_analyzer_scan",
    "security_analyzer_cancel",
    "security_analyzer_status",
    "security_analyzer_result",
    "security_analyzer_clean",
    "ide_default",
]

for method in server_methods:
    assert hasattr(cfrds.server, method), f"server class missing method: {method}"
    assert callable(getattr(cfrds.server, method)), f"{method} is not callable"

print(f"All {len(server_methods)} server class methods successfully verified!")

# Verify container class ready-to-use array/struct interfaces
dsn_container = cfrds.cfrds_sql_dsninfo(["dsn1", "dsn2"])
assert len(dsn_container) == 2
assert dsn_container[0] == "dsn1"
assert list(dsn_container) == ["dsn1", "dsn2"]
assert dsn_container.to_list() == ["dsn1", "dsn2"]

tbl_container = cfrds.cfrds_sql_tableinfo([{"name": "t1"}])
assert len(tbl_container) == 1
assert tbl_container[0]["name"] == "t1"
assert list(tbl_container) == [{"name": "t1"}]

res_container = cfrds.cfrds_sql_resultset(1, 1, ["col1"], [["val1"]])
assert res_container["columns"] == 1
assert res_container["names"] == ["col1"]
assert res_container["values"] == [["val1"]]
assert res_container[0] == ["val1"]
assert res_container.to_dict()["values"] == [["val1"]]

fc_container = cfrds.cfrds_file_content(b"hello", "ts", "perm")
assert fc_container["data"] == b"hello"
assert fc_container["size"] == 5
assert fc_container.to_dict()["data"] == b"hello"

map_container = cfrds.cfrds_adminapi_mappings({"k1": "v1"})
assert map_container["k1"] == "v1"
assert map_container.to_dict() == {"k1": "v1"}
assert list(map_container) == ["k1"]

print("Container class ready-to-use value tests passed!")

# Verify browse_dir validation (offline tests using http mock)
from unittest.mock import patch, MagicMock

mock_resp = MagicMock()
mock_resp.status = 200
mock_resp.reason = "OK"

mock_conn = MagicMock()
mock_conn.getresponse.return_value = mock_resp

with patch("http.client.HTTPConnection", return_value=mock_conn):
    srv_mock = cfrds.server("127.0.0.1", 8500, "admin", "admin")
    
    # Test 1: total = 0 (divisible by 5) -> should succeed
    mock_resp.read.return_value = b"0:"
    items0 = srv_mock.browse_dir("/")
    assert len(items0) == 0, "browse_dir with total = 0 should return empty list"
    
    # Test 2: total = 3 (not divisible by 5) -> should throw CFRDSError with CFRDS_STATUS_RESPONSE_ERROR
    mock_resp.read.return_value = b"3:"
    threw = False
    try:
        srv_mock.browse_dir("/")
    except cfrds.CFRDSError as e:
        if "Invalid total items count" in str(e):
            threw = True
        else:
            raise Exception(f"Unexpected error: {e}")
    assert threw, "browse_dir should throw CFRDSError containing Invalid total items count when total is 3"
    
    print("Offline browse_dir validation tests passed!")

    # Test 3: WDDX XML escaping in debugger_breakpoint
    mock_conn.request.reset_mock()
    mock_resp.read.return_value = b"0:"
    srv_mock.debugger_breakpoint("mysession", "foo<bar>&baz", 42, True)
    call_body = mock_conn.request.call_args[1].get("body", b"")
    if isinstance(call_body, bytes):
        call_body = call_body.decode("utf-8")
    assert "<string>foo&lt;bar&gt;&amp;baz</string>" in call_body, "debugger_breakpoint filepath should be XML-escaped"

    # Test 4: WDDX XML escaping in debugger_watch_expression
    mock_conn.request.reset_mock()
    mock_resp.read.return_value = b"0:"
    srv_mock.debugger_watch_expression("mysession", "thread<1>", "expr&val")
    call_body = mock_conn.request.call_args[1].get("body", b"")
    if isinstance(call_body, bytes):
        call_body = call_body.decode("utf-8")
    assert "<string>expr&amp;val</string>" in call_body, "debugger_watch_expression expression should be XML-escaped"
    assert "<string>thread&lt;1&gt;</string>" in call_body, "debugger_watch_expression thread_name should be XML-escaped"

    # Test 5: WDDX XML escaping in adminapi_extensions_setmapping
    mock_conn.request.reset_mock()
    mock_resp.read.return_value = b"0:"
    srv_mock.adminapi_extensions_setmapping("map'name", "path\"val")
    call_body = mock_conn.request.call_args[1].get("body", b"")
    if isinstance(call_body, bytes):
        call_body = call_body.decode("utf-8")
    assert "<var name='map&apos;name'>" in call_body, "adminapi_extensions_setmapping name should be XML-escaped"

    # Test 5b: adminapi_extensions_getmappings returns structured cfrds_adminapi_mappings (offline test)
    mock_conn.request.reset_mock()
    xml_data = "<wddxPacket version='1.0'><header/><data><struct><var name='k1'><string>v1</string></var><var name='k2'><string>v2</string></var><var name='k1'><string>v3</string></var></struct></data></wddxPacket>"
    resp_body = f"1:{len(xml_data)}:{xml_data}".encode("utf-8")
    mock_resp.read.return_value = resp_body
    mappings = srv_mock.adminapi_extensions_getmappings()
    assert isinstance(mappings, cfrds.cfrds_adminapi_mappings)
    assert len(mappings) == 3
    assert mappings.keys == ["k1", "k2", "k1"]
    assert mappings.values == ["v1", "v2", "v3"]
    assert mappings.mappings == {"k1": "v3", "k2": "v2"}
    assert mappings["k2"] == "v2"
    assert mappings[1] == ("k2", "v2")
    # Test 6: _wddx_deserialize offline validation
    wddx = """<wddxPacket version='1.0'>
      <header/>
      <data>
        <struct>
          <var name='nullVal'><null/></var>
          <var name='boolTrue'><boolean value='true'/></var>
          <var name='boolFalse'><boolean value='false'/></var>
          <var name='numberVal'><number>42.5</number></var>
          <var name='stringVal'><string>hello &lt;world&gt; &amp; &quot;everyone&quot;</string></var>
          <var name='cdataVal'><string><![CDATA[cdata <test> & val]]></string></var>
          <var name='arrayVal'>
            <array length='2'>
              <string>item1</string>
              <struct>
                <var name='nestedKey'><string>nestedVal</string></var>
              </struct>
            </array>
          </var>
        </struct>
      </data>
    </wddxPacket>"""
    parsed = cfrds._wddx_deserialize(wddx)
    assert parsed is not None, "parsed should not be None"
    assert parsed["nullVal"] is None, "nullVal should be None"
    assert parsed["boolTrue"] is True, "boolTrue should be True"
    assert parsed["boolFalse"] is False, "boolFalse should be False"
    assert parsed["numberVal"] == 42.5, "numberVal should be 42.5"
    assert parsed["stringVal"] == 'hello <world> & "everyone"', "stringVal should have unescaped XML entities"
    assert parsed["cdataVal"] == 'cdata <test> & val', "cdataVal should parse CDATA literally"
    assert isinstance(parsed["arrayVal"], list), "arrayVal should be a list"
    assert len(parsed["arrayVal"]) == 2, "arrayVal length should be 2"
    assert parsed["arrayVal"][0] == 'item1', "arrayVal[0] should be item1"
    assert parsed["arrayVal"][1]["nestedKey"] == 'nestedVal', "arrayVal[1]['nestedKey'] should be nestedVal"
    print("Offline _wddx_deserialize tests passed!")

    # Test 7: Debugger event accessors offline validation
    mock_event = cfrds.cfrds_debugger_event(
        cfrds.cfrds_debugger_type.CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT,
        {
            "source": "/app/index.cfm",
            "line": 42,
            "thread_name": "my-thread",
            "SCOPES": ["Variables", "Session"],
            "THREADS": ["my-thread", "other-thread"],
            "WATCH": ["expr1", "expr2"],
            "CF_TRACE": ["trace1", "trace2"],
            "JAVA_TRACE": ["jtrace1", "jtrace2"]
        }
    )

    assert cfrds.cfrds_debugger_event_get_type(mock_event) == cfrds.cfrds_debugger_type.CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT, "get_type mismatch"
    assert cfrds.cfrds_debugger_event_breakpoint_get_source(mock_event) == "/app/index.cfm", "get_source mismatch"
    assert cfrds.cfrds_debugger_event_breakpoint_get_line(mock_event) == 42, "get_line mismatch"
    assert cfrds.cfrds_debugger_event_breakpoint_get_thread_name(mock_event) == "my-thread", "get_thread_name mismatch"
    
    scopes = cfrds.cfrds_debugger_event_breakpoint_get_scopes(mock_event)
    assert isinstance(scopes, list) and scopes[0] == "Variables", "get_scopes mismatch"

    assert cfrds.cfrds_debugger_event_get_scopes_count(mock_event) == 2, "scopes count mismatch"
    assert cfrds.cfrds_debugger_event_get_scopes_item(mock_event, 0) == "Variables", "scopes item 0 mismatch"
    assert cfrds.cfrds_debugger_event_get_scopes_item(mock_event, 1) == "Session", "scopes item 1 mismatch"
    assert cfrds.cfrds_debugger_event_get_scopes_item(mock_event, 2) is None, "scopes item out of bounds mismatch"

    assert cfrds.cfrds_debugger_event_get_threads_count(mock_event) == 2, "threads count mismatch"
    assert cfrds.cfrds_debugger_event_get_threads_item(mock_event, 0) == "my-thread", "threads item 0 mismatch"

    assert cfrds.cfrds_debugger_event_get_watch_count(mock_event) == 2, "watch count mismatch"
    assert cfrds.cfrds_debugger_event_get_watch_item(mock_event, 0) == "expr1", "watch item 0 mismatch"

    assert cfrds.cfrds_debugger_event_get_cf_trace_count(mock_event) == 2, "cf_trace count mismatch"
    assert cfrds.cfrds_debugger_event_get_cf_trace_item(mock_event, 0) == "trace1", "cf_trace item 0 mismatch"

    assert cfrds.cfrds_debugger_event_get_java_trace_count(mock_event) == 2, "java_trace count mismatch"
    assert cfrds.cfrds_debugger_event_get_java_trace_item(mock_event, 0) == "jtrace1", "java_trace item 0 mismatch"

    print("Offline debugger event accessors tests passed!")

    # Test 8: Transport error mapping offline validation
    import socket
    import errno

    mock_conn_err = MagicMock()
    mock_exception = None
    
    def mock_request_raise(*args, **kwargs):
        raise mock_exception

    mock_conn_err.request.side_effect = mock_request_raise

    def test_error_mapping(exception_to_raise, expected_msg_substr):
        global mock_exception
        mock_exception = exception_to_raise
        threw = False
        try:
            srv_mock.browse_dir("/")
        except cfrds.CFRDSError as e:
            if expected_msg_substr in str(e):
                threw = True
            else:
                raise Exception(f"Unexpected error for exception {exception_to_raise}: {e}")
        assert threw, f"Should throw CFRDSError containing '{expected_msg_substr}' for {exception_to_raise}"

    with patch("http.client.HTTPConnection", return_value=mock_conn_err):
        # Test socket.gaierror -> SOCKET_HOST_NOT_FOUND
        test_error_mapping(socket.gaierror(8, "hostname lookup failed"), "Socket host not found")
        test_error_mapping(socket.herror(1, "host error"), "Socket host not found")

        # Test specific OSErrors -> SOCKET_CREATION_FAILED
        test_error_mapping(PermissionError(errno.EACCES, "Permission denied"), "Socket creation failed")
        test_error_mapping(PermissionError(errno.EPERM, "Operation not permitted"), "Socket creation failed")
        test_error_mapping(OSError(errno.EADDRNOTAVAIL, "Address not available"), "Socket creation failed")
        test_error_mapping(OSError(errno.EMFILE, "Too many open files"), "Socket creation failed")
        test_error_mapping(OSError(errno.ENFILE, "Too many open files in system"), "Socket creation failed")

        # Test general OSError/connection errors -> CONNECTION_TO_SERVER_FAILED
        test_error_mapping(ConnectionRefusedError(errno.ECONNREFUSED, "Connection refused"), "Connection to server failed")
        test_error_mapping(TimeoutError(errno.ETIMEDOUT, "Connection timed out"), "Connection to server failed")
        test_error_mapping(OSError(errno.EIO, "Input/output error"), "Connection to server failed")
        test_error_mapping(Exception("Some other error"), "Connection to server failed")

    print("Offline transport error mapping tests passed!")

    print("Offline WDDX escaping validation tests passed!")

# Live server integration test if env vars present
if "RDS_HOST" in os.environ and "RDS_PORT" in os.environ:
    host = os.environ["RDS_HOST"]
    port = int(os.environ["RDS_PORT"])
    username = os.environ.get("RDS_USERNAME", "admin")
    password = os.environ.get("RDS_PASSWORD", "")

    print(f"Connecting to live RDS server at {host}:{port}...")
    rds = cfrds.server(host, port, username, password)

    # --- File & Directory Operations ---
    print("\n--- Validating File & Directory Operations ---")
    cf_root = ""
    try:
        cf_root = rds.cf_root_dir()
        print(f"  cf_root_dir(): {cf_root}")
    except cfrds.CFRDSError as e:
        print(f"  cf_root_dir error: {e}")

    try:
        target_path = cf_root or "/"
        items = rds.browse_dir(target_path)
        print(f"  browse_dir('{target_path}'): Found {len(items)} items")
        if len(items) > 0:
            import datetime
            sample = items[:3]
            print("  browse_dir sample items:")
            for item in sample:
                dt = datetime.datetime.fromtimestamp(item['modified'] / 1000, tz=datetime.timezone.utc)
                print(f"    - {item['name']} ({item['kind']}, size: {item['size']}, modified: {dt.isoformat()})")
                assert item['modified'] > 0, "parsed modified date should be positive/valid"
    except cfrds.CFRDSError as e:
        print(f"  browse_dir error: {e}")

    test_dir = f"/tmp/py_test_dir_{int(time.time() * 1000)}"
    test_file = f"{test_dir}/test.txt"
    test_file_renamed = f"{test_dir}/test_renamed.txt"

    try:
        rds.dir_create(test_dir)
        print(f"  dir_create('{test_dir}'): success")

        exists_dir = rds.file_exists(test_dir)
        print(f"  file_exists('{test_dir}'): {exists_dir}")
        assert exists_dir is True, "file_exists for created directory should be True"

        file_data = b"Hello ColdFusion RDS Python Test!"
        rds.file_write(test_file, file_data)
        print(f"  file_write('{test_file}'): success")

        exists_file = rds.file_exists(test_file)
        print(f"  file_exists('{test_file}'): {exists_file}")

        read_bytes = rds.file_read(test_file)
        print(f"  file_read('{test_file}'): {len(read_bytes)} bytes read ({read_bytes})")
        assert read_bytes == file_data, "file_read data should match file_write data"

        rds.file_rename(test_file, test_file_renamed)
        print(f"  file_rename('{test_file}' -> '{test_file_renamed}'): success")

        rds.file_remove(test_file_renamed)
        print(f"  file_remove('{test_file_renamed}'): success")

        rds.dir_remove(test_dir)
        print(f"  dir_remove('{test_dir}'): success")
    except cfrds.CFRDSError as e:
        print(f"  File/Directory operation error: {e}")
        try: rds.file_remove(test_file)
        except Exception: pass
        try: rds.file_remove(test_file_renamed)
        except Exception: pass
        try: rds.dir_remove(test_dir)
        except Exception: pass

    # --- SQL Operations ---
    print("\n--- Validating SQL Operations ---")
    try:
        supported_cmds = rds.sql_getsupportedcommands()
        print(f"  sql_getsupportedcommands(): {len(supported_cmds)} commands ({', '.join(supported_cmds)})")
    except cfrds.CFRDSError as e:
        print(f"  sql_getsupportedcommands error: {e}")

    dsns = []
    try:
        dsns = rds.sql_dsninfo()
        print(f"  sql_dsninfo(): {len(dsns)} DSNs found ({', '.join(dsns)})")
    except cfrds.CFRDSError as e:
        print(f"  sql_dsninfo error: {e}")

    target_dsn = os.environ.get("RDS_DSN", dsns[0] if dsns else "")
    if target_dsn:
        try:
            db_desc = rds.sql_dbdescription(target_dsn)
            print(f"  sql_dbdescription('{target_dsn}'): {db_desc}")
        except cfrds.CFRDSError as e:
            print(f"  sql_dbdescription error: {e}")

        tables = []
        try:
            tables = rds.sql_tableinfo(target_dsn)
            print(f"  sql_tableinfo('{target_dsn}'): {len(tables)} tables found")
        except cfrds.CFRDSError as e:
            print(f"  sql_tableinfo error: {e}")

        target_table = os.environ.get("RDS_DSN_TABLE", tables[0]["name"] if tables else "")
        if target_table:
            try:
                cols = rds.sql_columninfo(target_dsn, target_table)
                print(f"  sql_columninfo('{target_dsn}', '{target_table}'): {len(cols)} columns")
            except cfrds.CFRDSError as e:
                print(f"  sql_columninfo error: {e}")

            try:
                pks = rds.sql_primarykeys(target_dsn, target_table)
                print(f"  sql_primarykeys('{target_dsn}', '{target_table}'): {len(pks)} primary keys")
            except cfrds.CFRDSError as e:
                print(f"  sql_primarykeys error: {e}")

            try:
                fks = rds.sql_foreignkeys(target_dsn, target_table)
                print(f"  sql_foreignkeys('{target_dsn}', '{target_table}'): {len(fks)} foreign keys")
            except cfrds.CFRDSError as e:
                print(f"  sql_foreignkeys error: {e}")

            try:
                imp_keys = rds.sql_importedkeys(target_dsn, target_table)
                print(f"  sql_importedkeys('{target_dsn}', '{target_table}'): {len(imp_keys)} imported keys")
            except cfrds.CFRDSError as e:
                print(f"  sql_importedkeys error: {e}")

            try:
                exp_keys = rds.sql_exportedkeys(target_dsn, target_table)
                print(f"  sql_exportedkeys('{target_dsn}', '{target_table}'): {len(exp_keys)} exported keys")
            except cfrds.CFRDSError as e:
                print(f"  sql_exportedkeys error: {e}")

        try:
            sql_res = rds.sql_sqlstmnt(target_dsn, "SELECT 1")
            print(f"  sql_sqlstmnt('{target_dsn}', 'SELECT 1'): {sql_res['rows']} rows, {sql_res['columns']} columns")
        except cfrds.CFRDSError as e:
            print(f"  sql_sqlstmnt error: {e}")

        try:
            meta_res = rds.sql_metadata(target_dsn, "SELECT 1")
            print(f"  sql_metadata('{target_dsn}', 'SELECT 1'): {len(meta_res)} metadata items")
        except cfrds.CFRDSError as e:
            print(f"  sql_metadata error: {e}")
    else:
        print("  Skipping DSN-specific SQL tests (no DSN available)")

    # --- Security Analyzer Operations ---
    print("\n--- Validating Security Analyzer Operations ---")
    try:
        scan_path = cf_root or "/"
        cmd_id = rds.security_analyzer_scan(scan_path, True, 1)
        print(f"  security_analyzer_scan('{scan_path}'): command_id={cmd_id}")

        if cmd_id > 0:
            status = rds.security_analyzer_status(cmd_id)
            print(f"  security_analyzer_status({cmd_id}): visited={status.get('filesvisitedcount', 0)}/{status.get('totalfiles', 0)}")

            result = rds.security_analyzer_result(cmd_id)
            print(f"  security_analyzer_result({cmd_id}): {result}")

            rds.security_analyzer_cancel(cmd_id)
            print(f"  security_analyzer_cancel({cmd_id}): success")

            rds.security_analyzer_clean(cmd_id)
            print(f"  security_analyzer_clean({cmd_id}): success")
    except cfrds.CFRDSError as e:
        print(f"  Security Analyzer error: {e}")

    # --- IDE Default ---
    print("\n--- Validating IDE Default ---")
    try:
        ide_res = rds.ide_default(1)
        print(f"  ide_default(1): server_version={ide_res['server_version']}, client_version={ide_res['client_version']}")
    except cfrds.CFRDSError as e:
        print(f"  ide_default error: {e}")

    # --- Debugging Operations ---
    print("\n--- Debugging Operations ---")
    try:
        print("  Starting Debugger...")
        session_id = rds.debugger_start()
        print(f"  debugger_start(): session_id={session_id}")
        assert isinstance(session_id, str) and len(session_id) > 0, "debugger_start should return non-empty session_id string"

        try:
            try:
                dbg_port = rds.debugger_get_server_info(session_id)
                print(f"  debugger_get_server_info('{session_id}'): port={dbg_port}")
            except cfrds.CFRDSError as e:
                print(f"  debugger_get_server_info error: {e}")

            try:
                rds.debugger_set_scope_filter(session_id, "VARIABLES,SESSION")
                print(f"  debugger_set_scope_filter('{session_id}'): success")
            except cfrds.CFRDSError as e:
                print(f"  debugger_set_scope_filter error: {e}")

            try:
                rds.debugger_watch_variables(session_id, "VARIABLES.A")
                print(f"  debugger_watch_variables('{session_id}'): success")
            except cfrds.CFRDSError as e:
                print(f"  debugger_watch_variables error: {e}")

            try:
                rds.debugger_clear_all_breakpoints(session_id)
                print(f"  debugger_clear_all_breakpoints('{session_id}'): success")
            except cfrds.CFRDSError as e:
                print(f"  debugger_clear_all_breakpoints error: {e}")

            test_cfm_content = (
                "<cfset a = 10>\n"
                "<cfset b = 20>\n"
                "<cfset c = a + b>\n"
                "<cfoutput>Debug Test Page: #c#</cfoutput>\n"
            ).encode("utf-8")

            app_cfm_path = "/app/test_debug.cfm"
            wwwroot_cfm_path = f"{cf_root}/wwwroot/test_debug.cfm" if cf_root else "/opt/coldfusion/cfusion/wwwroot/test_debug.cfm"

            try:
                rds.file_write(app_cfm_path, test_cfm_content)
            except Exception:
                pass
            try:
                rds.file_write(wwwroot_cfm_path, test_cfm_content)
            except Exception:
                pass

            target_bp_path = app_cfm_path
            try:
                rds.debugger_breakpoint(session_id, target_bp_path, 2, True)
                print(f"  debugger_breakpoint('{session_id}', '{target_bp_path}', line 2): success")
            except cfrds.CFRDSError as e:
                print(f"  debugger_breakpoint error: {e}")

            if wwwroot_cfm_path != app_cfm_path:
                try:
                    rds.debugger_breakpoint(session_id, wwwroot_cfm_path, 2, True)
                except Exception:
                    pass

            try:
                rds.debugger_breakpoint_on_exception(session_id, True)
                print(f"  debugger_breakpoint_on_exception('{session_id}'): success")
            except cfrds.CFRDSError as e:
                print(f"  debugger_breakpoint_on_exception error: {e}")

            try:
                rds.debugger_all_fetch_flags_enabled(session_id, True, True, True, True, True)
                print(f"  debugger_all_fetch_flags_enabled('{session_id}'): success")
            except cfrds.CFRDSError as e:
                print(f"  debugger_all_fetch_flags_enabled error: {e}")

            # Fetch Event 1 (Dummy / Breakpoint Registration Event)
            evt1 = None
            try:
                evt1 = rds.debugger_get_debug_events(session_id)
                print(f"  >>> EVENT 1 (Dummy / Registration Event): {evt1}")
            except cfrds.CFRDSError as e:
                print(f"  debugger_get_debug_events (Event 1) error: {e}")

            print("\n" + "=" * 72)
            print("[DEBUGGER READY]")
            print(f"  Created CFML file: {app_cfm_path} (and {wwwroot_cfm_path})")
            print(f"  Breakpoint set on line 2")
            print(f"  Please open http://{host}:{port}/test_debug.cfm in your browser.")
            print("  Waiting for Event 2 (Browser Hit Event - script will block until requested)...")
            print("=" * 72 + "\n")

            # Fetch Event 2 (Actual Browser Hit Event)
            evt2 = None
            try:
                evt2 = rds.debugger_get_debug_events(session_id)
                print(f"  >>> EVENT 2 (Browser Hit Event): {evt2}")
            except cfrds.CFRDSError as e:
                print(f"  debugger_get_debug_events (Event 2) error: {e}")

            thread_name = "main"
            if evt2 and isinstance(evt2, dict) and "data" in evt2 and isinstance(evt2["data"], dict):
                thread_name = evt2["data"].get("thread_name") or evt2["data"].get("thread_id") or "main"
            elif evt1 and isinstance(evt1, dict) and "data" in evt1 and isinstance(evt1["data"], dict):
                thread_name = evt1["data"].get("thread_name") or evt1["data"].get("thread_id") or "main"

            try:
                rds.debugger_watch_expression(session_id, thread_name, "arrayNew(1)")
                print(f"  debugger_watch_expression('{session_id}', '{thread_name}'): success")
            except cfrds.CFRDSError as e:
                print(f"  debugger_watch_expression error: {e}")

            try:
                rds.debugger_set_variable(session_id, thread_name, "VARIABLES.A", "200")
                print(f"  debugger_set_variable('{session_id}', '{thread_name}'): success")
            except cfrds.CFRDSError as e:
                print(f"  debugger_set_variable error: {e}")

            try:
                output = rds.debugger_get_output(session_id, thread_name)
                print(f"  debugger_get_output('{session_id}', '{thread_name}'): success, output='{output}'")
            except cfrds.CFRDSError as e:
                print(f"  debugger_get_output error: {e}")

            try:
                rds.debugger_step_over(session_id, thread_name)
                print(f"  debugger_step_over('{session_id}', '{thread_name}'): success")
            except cfrds.CFRDSError as e:
                print(f"  debugger_step_over error: {e}")

            try:
                rds.debugger_continue(session_id, thread_name)
                print(f"  debugger_continue('{session_id}', '{thread_name}'): success")
            except cfrds.CFRDSError as e:
                print(f"  debugger_continue error: {e}")

            try:
                rds.debugger_breakpoint(session_id, target_bp_path, 2, False)
                print(f"  debugger_breakpoint disable('{session_id}'): success")
            except cfrds.CFRDSError as e:
                print(f"  debugger_breakpoint disable error: {e}")
        finally:
            rds.debugger_stop(session_id)
            print(f"  debugger_stop('{session_id}'): success")
    except cfrds.CFRDSError as e:
        print(f"  Debugger session failed to start (skipping remaining debugger tests): {e}")

    rds.close()

print("cfrds pure Python test completed successfully!")
