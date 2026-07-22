# cfrds Python Package

Pure Python interface for connecting to Adobe ColdFusion Remote Development Service (RDS).

## Features

- **Pure Python** implementation with zero external PyPI dependencies.
- Full parity with `libcfrds` C library exported API (`cfrds.h`).
- Convenient high-level Pythonic `server` API.
- Support for directory browsing, file read/write/rename/delete, directory creation/deletion.
- Database metadata queries (DSN info, tables, columns, primary keys, foreign keys, imported/exported keys, SQL execution, metadata, DB description).
- ColdFusion remote debugger session management and breakpoint control.
- Security analyzer and Admin API operations.
- Cross-platform support (Linux, macOS, Windows).

## Installation

```bash
pip install cfrds
```

## Quick Start

```python
import cfrds

# Create ColdFusion RDS server connection
rds = cfrds.server(
    hostname="127.0.0.1",
    port=8500,
    username="admin",
    password="secretpassword"
)

# Browse directory
files = rds.browse_dir("/var/log")
for item in files:
    print(f"{item['kind']} {item['name']} ({item['size']} bytes)")

# Read file content
content = rds.file_read("/var/log/dpkg.log")
print(content.data.decode("utf-8", errors="ignore"))

# Write file content
data = b"ColdFusion RDS test content\n"
rds.file_write("/opt/ColdFusion2021/cfusion/wwwroot/test.cfm", data)

# File exists check
if rds.file_exists("/opt/ColdFusion2021/cfusion/wwwroot/test.cfm"):
    print("File exists!")

# Remove file
rds.file_remove("/opt/ColdFusion2021/cfusion/wwwroot/test.cfm")

# Database datasources
datasources = rds.sql_dsninfo()
print("Data Sources:", datasources)

# Close connection
rds.close()
```

## Context Manager Usage

```python
import cfrds

with cfrds.server("127.0.0.1", 8500, "admin", "password") as rds:
    print("ColdFusion Root Dir:", rds.cf_root_dir())
```

## API Compatibility

The Python package provides the same API surface as the `libcfrds` C library (`cfrds.h`), but is implemented entirely in pure Python with no native dependencies.

## License

MIT License.
