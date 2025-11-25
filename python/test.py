import os

import cfrds

host = os.environ["RDS_HOST"]
port = int(os.environ["RDS_PORT"])
username = os.environ["RDS_USERNAME"]
password = os.environ["RDS_PASSWORD"]

rds = cfrds.server(host, port, username, password)

# print(rds.browse_dir("/var/log"))
# print(rds.sql_dsninfo())
# print(rds.sql_tableinfo("datasource"))
# print(rds.sql_columninfo("datasource", "table"))
# print(rds.sql_primarykeys("datasource", "table"))
# print(rds.sql_foreignkeys("datasource", "table"))
# print(rds.sql_importedkeys("datasource", "table"))
# print(rds.sql_exportedkeys("datasource", "table"))
# print(rds.sql_sqlstmnt("datasource", "SELECT 1 as id, 2 as id2 LIMIT 1"))
# print(rds.sql_metadata("datasource", "SELECT 1 as id, 2 as id2 LIMIT 1"))
# print(rds.sql_getsupportedcommands())
# print(rds.sql_dbdescription("datasource"))

debugger_session_id = rds.debugger_start()
print("Debugger session ID:", debugger_session_id)

#print(rds.debugger_get_server_info())
print(rds.debugger_breakpoint_on_exception(debugger_session_id, True))

rds.debugger_stop(debugger_session_id)
print("Debugger session stopped")
