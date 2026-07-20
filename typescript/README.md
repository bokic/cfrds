# @bokic/cfrds

> Pure TypeScript client implementation of the ColdFusion RDS (Remote Development Service) protocol for Node.js and modern JavaScript runtimes. Zero runtime dependencies.

[![npm version](https://img.shields.io/npm/v/@bokic/cfrds.svg)](https://www.npmjs.com/package/@bokic/cfrds)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

---

## Features

- **File System Operations**: Browse remote server directories, read, upload, download, move, rename, delete files, and create/remove directories.
- **Database Services**: Enumerate data sources (DSNs), inspect tables, columns, primary & foreign keys, execute SQL queries, and inspect SQL metadata.
- **Remote Debugger**: Connect to ColdFusion remote debugger, control execution (step into, step over, resume), set breakpoints, and listen to events.
- **Admin API & Server Info**: Fetch ColdFusion installation directory (`cfroot`), inspect server info, retrieve custom tag paths, and manage CF mappings.
- **Security Analyzer**: Run remote CFML security analyzer scans.
- **Zero External Dependencies**: Pure TypeScript, built on native Node.js HTTP/HTTPS primitives.

---

## Installation

```bash
npm install @bokic/cfrds
```

---

## Quick Start

```typescript
import { Server } from "@bokic/cfrds";

async function main() {
  // Initialize server connection
  const server = new Server("127.0.0.1", 8500, "admin", "your_rds_password");

  try {
    // 1. Get ColdFusion installation directory
    const cfRoot = await server.getCFRoot();
    console.log(`ColdFusion Root: ${cfRoot}`);

    // 2. Browse directory contents
    const items = await server.browseDir("/");
    console.log("Directory listing:", items);

    // 3. Read a remote file
    const fileContent = await server.cat("/index.cfm");
    console.log("File size:", fileContent.size, "bytes");
    console.log("Content:\n", fileContent.content.toString("utf-8"));

    // 4. Query Data Sources
    const dsns = await server.getDsnInfo();
    console.log("Available DSNs:", dsns);
  } catch (err) {
    console.error("RDS Error:", err);
  }
}

main();
```

---

## Usage Examples

### File Operations

```typescript
// Browse directory
const files = await server.browseDir("/var/www/html");

// Read remote file as Buffer
const file = await server.cat("/var/www/html/app.cfm");

// Upload local file to remote server
const localBuffer = Buffer.from("<cfoutput>Hello World</cfoutput>");
await server.uploadFile("/var/www/html/hello.cfm", localBuffer);

// Download remote file
const content = await server.downloadFile("/var/www/html/hello.cfm");

// Move / Rename
await server.renameFile("/var/www/html/hello.cfm", "/var/www/html/index.cfm");

// Create / Delete directory
await server.mkdir("/var/www/html/uploads");
await server.rmdir("/var/www/html/uploads");

// Delete file
await server.removeFile("/var/www/html/index.cfm");
```

### Database Operations

```typescript
// List data sources
const dsns = await server.getDsnInfo();

// Get tables in DSN
const tables = await server.getTableInfo("my_dsn");

// Get columns in table
const columns = await server.getColumnInfo("my_dsn", "users");

// Execute SQL query
const resultSet = await server.execSql("my_dsn", "SELECT * FROM users WHERE active = 1");
console.log("Columns:", resultSet.columns);
console.log("Rows:", resultSet.data);
```

### Remote Debugger

```typescript
// Start debugging session
const session = await server.debuggerStart();
console.log("Debug session ID:", session.sessionId);

// Set breakpoint
await server.debuggerSetBreakpoint(session.sessionId, "/var/www/html/index.cfm", 15);

// Stop debugging session
await server.debuggerStop(session.sessionId);
```

---

## API Summary

| Category | Method | Description |
|---|---|---|
| **System** | `getCFRoot()` | Get ColdFusion root installation folder |
| **System** | `ideDefault(version)` | Get CF server info |
| **Files** | `browseDir(path)` | List directory items |
| **Files** | `cat(path)` | Read file content |
| **Files** | `uploadFile(path, content)` | Upload file content |
| **Files** | `downloadFile(path)` | Download file content |
| **Files** | `renameFile(oldPath, newPath)` | Move or rename file |
| **Files** | `removeFile(path)` | Delete remote file |
| **Files** | `mkdir(path)` | Create remote directory |
| **Files** | `rmdir(path)` | Delete remote directory |
| **Database** | `getDsnInfo()` | Enumerate data sources |
| **Database** | `getTableInfo(dsn)` | Enumerate tables |
| **Database** | `getColumnInfo(dsn, table)` | Enumerate table columns |
| **Database** | `execSql(dsn, sql)` | Execute SQL query |
| **Admin API** | `getCustomTagPaths()` | Fetch custom tag paths |
| **Admin API** | `getMappings()` | Fetch server mappings |

---

## License

[MIT](LICENSE)
