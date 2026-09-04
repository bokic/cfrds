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
    const cfRoot = await server.cfRootDir();
    console.log(`ColdFusion Root: ${cfRoot}`);

    // 2. Browse directory contents
    const items = await server.browseDir("/");
    console.log("Directory listing:", items);

    // 3. Read a remote file
    const fileContent = await server.fileRead("/index.cfm");
    console.log("File size:", fileContent.size, "bytes");
    console.log("Content:\n", fileContent.data.toString("utf-8"));

    // 4. Query Data Sources
    const dsns = await server.sqlDsninfo();
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
const file = await server.fileRead("/var/www/html/app.cfm");

// Upload / write file to remote server
const localBuffer = Buffer.from("<cfoutput>Hello World</cfoutput>");
await server.fileWrite("/var/www/html/hello.cfm", localBuffer);

// Move / Rename
await server.fileRename("/var/www/html/hello.cfm", "/var/www/html/index.cfm");

// Create / Delete directory
await server.dirCreate("/var/www/html/uploads");
await server.dirRemove("/var/www/html/uploads");

// Delete file
await server.fileRemove("/var/www/html/index.cfm");

// Check if file exists
const exists = await server.fileExists("/var/www/html/index.cfm");
```

### Database Operations

```typescript
// List data sources
const dsns = await server.sqlDsninfo();

// Get tables in DSN
const tables = await server.sqlTableinfo("my_dsn");

// Get columns in table
const columns = await server.sqlColumninfo("my_dsn", "users");

// Inspect primary and foreign keys
const pks = await server.sqlPrimarykeys("my_dsn", "users");
const fks = await server.sqlForeignkeys("my_dsn", "users");

// Execute SQL query
const resultSet = await server.sqlSqlstmnt("my_dsn", "SELECT * FROM users WHERE active = 1");
console.log("Columns:", resultSet.columns);
console.log("Names:", resultSet.names);
console.log("Values:", resultSet.values);
```

### Remote Debugger

```typescript
// Start debugging session
const sessionId = await server.debuggerStart();
console.log("Debug session ID:", sessionId);

// Set breakpoint
await server.debuggerBreakpoint(sessionId, "/var/www/html/index.cfm", 15, true);

// Synchronous step operations
const stepInEvt = await server.debuggerSyncStepIn(sessionId, "main");
const stepOverEvt = await server.debuggerSyncStepOver(sessionId, "main");
const stepOutEvt = await server.debuggerSyncStepOut(sessionId, "main");

// Inspect variables
const vars = await server.debuggerGetCfVariables(sessionId, "main");

// Resume execution
await server.debuggerContinue(sessionId, "main");

// Stop debugging session
await server.debuggerStop(sessionId);
```

### Error Handling

All asynchronous methods reject with specific `CFRDSError` subclasses, providing type-safe error catching with status codes and native cause chains:

```typescript
import {
  Server,
  CFRDSError,
  CFRDSNetworkError,
  CFRDSResponseError,
  CFRDSCommandError,
  CFRDSValidationError,
  CFRDS_STATUS,
} from "@bokic/cfrds";

try {
  const server = new Server("cfserver.local", 8500, "admin", "secret");
  const files = await server.browseDir("/nonexistent/dir");
} catch (err) {
  if (err instanceof CFRDSNetworkError) {
    console.error(`Network error: ${err.message}, code: ${err.code}, status: ${err.status}`);
  } else if (err instanceof CFRDSCommandError) {
    console.error(`RDS command rejected by server: ${err.message}`);
  } else if (err instanceof CFRDSResponseError) {
    console.error(`Protocol or response format error: ${err.message}`);
  } else if (err instanceof CFRDSValidationError) {
    console.error(`Input parameter validation failed: ${err.message}`);
  } else if (err instanceof CFRDSError) {
    console.error(`Generic CFRDS error (${err.status}): ${err.message}`);
  }
}
```

---

## API Summary

| Category | Method | Description |
|---|---|---|
| **System** | `cfRootDir()` | Get ColdFusion root installation folder |
| **System** | `ideDefault(version)` | Get CF server info |
| **Files** | `browseDir(path)` | List directory items |
| **Files** | `fileRead(filepath)` | Read file content |
| **Files** | `fileWrite(filepath, content)` | Write file content |
| **Files** | `fileRename(from, to)` | Move or rename file |
| **Files** | `fileRemove(filepath)` | Delete remote file |
| **Files** | `dirCreate(dirpath)` | Create remote directory |
| **Files** | `dirRemove(dirpath)` | Delete remote directory |
| **Files** | `fileExists(filepath)` | Check if file exists |
| **Database** | `sqlDsninfo()` | Enumerate data sources |
| **Database** | `sqlTableinfo(dsn)` | Enumerate tables |
| **Database** | `sqlColumninfo(dsn, table)` | Enumerate table columns |
| **Database** | `sqlPrimarykeys(dsn, table)` | Enumerate primary keys |
| **Database** | `sqlForeignkeys(dsn, table)` | Enumerate foreign keys |
| **Database** | `sqlImportedkeys(dsn, table)` | Enumerate imported keys |
| **Database** | `sqlExportedkeys(dsn, table)` | Enumerate exported keys |
| **Database** | `sqlSqlstmnt(dsn, sql)` | Execute SQL query |
| **Database** | `sqlMetadata(dsn, sql)` | Query SQL metadata |
| **Database** | `sqlGetsupportedcommands()` | Enumerate supported SQL commands |
| **Database** | `sqlDbdescription(dsn)` | Query database description |
| **Debugger** | `debuggerStart()` / `debuggerStop()` | Start/stop debugging session |
| **Debugger** | `debuggerServerStop()` | Stop debugger server |
| **Debugger** | `debuggerBreakpoint(session, file, line, enable)` | Set/clear breakpoint |
| **Debugger** | `debuggerSyncStepIn()` / `StepOver()` / `StepOut()` | Synchronous stepping |
| **Debugger** | `debuggerGetCfVariables(session, thread)` | Get CF variable scopes and values |
| **Admin API** | `adminapiExtensionsGetcustomtagpaths()` | Fetch custom tag paths |
| **Admin API** | `adminapiExtensionsGetmappings()` | Fetch server mappings |
| **Admin API** | `adminapiExtensionsSetmapping(name, path)` | Set server mapping |
| **Admin API** | `adminapiExtensionsDeletemapping(name)` | Delete server mapping |
| **Admin API** | `adminapiDebuggingGetlogproperty(dir)` | Fetch log directory property |
| **Security** | `securityAnalyzerScan()` / `Status()` / `Result()` | Security analyzer scans |

---

## License

[MIT](LICENSE)
