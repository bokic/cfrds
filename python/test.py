import cfrds
import os


host = os.environ['RDS_HOST']
port = int(os.environ['RDS_PORT'])
username = os.environ['RDS_USERNAME']
password = os.environ['RDS_PASSWORD']

rds = cfrds.server(host, port, username, password)

print(rds.browse_dir("/var/log"))
print(rds.sql_dnsinfo())
