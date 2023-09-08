import cfrds


rds = cfrds.server("localhost", 8500, "admin", "")


dir = rds.browse_dir("/var/log")
print(dir)
