#!/usr/bin/env python3

import os
import sys

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
    "cfrds_command_adminapi_debugging_getlogproperty",
    "cfrds_command_graphing",
]

for func in c_api_functions:
    assert hasattr(cfrds, func), f"Missing exported C API function: {func}"
    assert callable(getattr(cfrds, func)), f"{func} is not callable"

print(f"All {len(c_api_functions)} C API functions successfully verified!")

# Live server integration test if env vars present
if "RDS_HOST" in os.environ and "RDS_PORT" in os.environ:
    host = os.environ["RDS_HOST"]
    port = int(os.environ["RDS_PORT"])
    username = os.environ.get("RDS_USERNAME", "admin")
    password = os.environ.get("RDS_PASSWORD", "")

    print(f"Connecting to live RDS server at {host}:{port}...")
    rds = cfrds.server(host, port, username, password)

    try:
        print("Getting CF root dir...")
        root_dir = rds.cf_root_dir()
        print("CF Root Dir:", root_dir)
    except cfrds.CFRDSError as e:
        print("CF Root Dir error:", e)

    try:
        print("Browsing directory...")
        items = rds.browse_dir("/")
        print(f"Found {len(items)} items in /")
    except cfrds.CFRDSError as e:
        print("Browse dir error:", e)

    try:
        print("Starting Debugger...")
        debugger_session_id = rds.debugger_start()
        print("Debugger session ID:", debugger_session_id)
        rds.debugger_stop(debugger_session_id)
        print("Debugger session stopped.")
    except cfrds.CFRDSError as e:
        print("Debugger notification (expected if CF debugging disabled):", e)

    try:
        print("Testing graphing (bar chart)...")
        chart_attrs = "type=bar;height=400;width=600;title=Test Chart"
        series = ["seriesLabel=Sales;values=10,20,30,40"]
        img_bytes = rds.graphing(chart_attrs, series)
        print(f"Graph rendered successfully ({len(img_bytes)} bytes, starts with: {img_bytes[:8].hex()})")
    except cfrds.CFRDSError as e:
        print("Graphing error:", e)

print("cfrds pure Python test completed successfully!")
