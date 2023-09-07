import cfrds


rds = cfrds.server()

rds.host = "localhost"
rds.username = "admin"
rds.password = ""

dir = rds.browse_dir("/var/log")
print(dir)
