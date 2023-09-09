# cfrds Python binding
---
cfrds python module for connecting to ColdFusion RDS service.

TODO:
- [ ] ColdFusion error string passed to python exception.

Examples:
``` Python
# Create ColdFusion RDS connection
rds = cfrds.server(
    hostname="127.0.0.1",
    port=8500,
    username="admin",
    password=""
)

# Browse directory
dir = rds.browse_dir("/var/log")
print(dir)

# Read file
filepath = "/var/log/dpkg.log"
file_content = rds.file_read(filepath)
print(file_content)

# Write file
filepath = "/opt/ColdFusion2021/cfusion/wwwroot/test.cfm"
file_content = bytearray("Test cfm file...\n", 'utf-8')
rds.file_write(filepath, file_content)

# File rename
from_filepath = "/opt/ColdFusion2021/cfusion/wwwroot/test.cfm"
to_filepath =   "/opt/ColdFusion2021/cfusion/wwwroot/test_renamed.cfm"
rds.file_rename(from_filepath, to_filepath)

# File remove
rds.dir_remove("/opt/ColdFusion2021/cfusion/wwwroot/test_renamed.cfm")

# Check if file exists
file_exists = rds.file_exists("/opt/ColdFusion2021/cfusion/wwwroot/test_renamed.cfm")
print(file_exists)

# Create directory
new_dir = "/opt/ColdFusion2021/cfusion/wwwroot/some_dir"
rds.dir_create(new_dir)

# Return ColdFusion installation directory(ex. /opt/ColdFusion2021/cfusion)
cf_dir = rds.cf_root_dir()
print(cf_dir)
```
