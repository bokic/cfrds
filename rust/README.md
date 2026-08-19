# cfrds (Rust)

A Rust port of the C11 [`libcfrds`](..) library — a ColdFusion RDS protocol
client. It communicates with a ColdFusion server via the Adobe RDS protocol
over plain HTTP, exposing file, SQL, debugger, security-analyzer and admin API
operations.

This crate is a faithful, idiomatic port of the C library and shares its
behaviour and protocol quirks (including the hardcoded Adobe password
obfuscation scheme and the `deleltemappings` admin typo).

## Features

- File operations: list directories, read/write/rename/remove files and
  directories, check existence, retrieve the CF root directory.
- SQL operations: DSN info, table/column info, primary/foreign/imported/
  exported keys, statement resultsets, statement metadata, supported commands
  and database descriptions.
- Debugger operations: start/stop sessions, breakpoints, step/continue,
  watch expressions and variables, output retrieval and scope filters.
- Security analyzer: scan, cancel, status, result and clean.
- Admin API: log properties, custom tag paths, mappings, IDE handshake and
  chart rendering (`graphing`).
- WDDX packet parsing and serialization (the RDS wire format).

## Usage

Add the crate to your `Cargo.toml`:

```toml
[dependencies]
cfrds = { path = "rust" }
```

### Example

```rust,no_run
use cfrds::Server;

fn main() -> cfrds::Result<()> {
    let server = Server::new("127.0.0.1", 8500, "admin", "secret")?;

    let root = server.file_get_root_dir()?;
    println!("ColdFusion root: {root}");

    let dir = server.browse_dir("/")?;
    for item in dir.iter() {
        println!("{} {}", item.kind, item.name);
    }

    // Run a query against a datasource.
    if let Ok(dsns) = server.sql_dsninfo() {
        if let Some(dsn) = dsns.name(0) {
            let rs = server.sql_sqlstmnt(dsn, "SELECT 1")?;
            println!("{} rows x {} columns", rs.rows(), rs.columns());
        }
    }

    Ok(())
}
```

## Build

```sh
cd rust
cargo build            # debug
./build.sh             # release build; syncs Cargo.toml version from the latest git tag
cargo test             # unit tests (no server required)
```

`build.sh` derives the package version from the most recent `X.Y.Z` git tag
(via `git describe --tags --abbrev=0`, matching the C build) and only rewrites
`Cargo.toml` when the version actually differs. That version feeds both
`cfrds::VERSION` and the shared-library soname.

The release profile is size-optimized (`opt-level = "z"`, `lto`, `codegen-units = 1`,
`panic = "abort"`, `strip = true`); `target/release/libcfrds.so` is ~600 KB.
For a further ~30 KB reduction, disable unwind tables (not applied by default
because it affects the whole workspace):

```sh
RUSTFLAGS="-C force-unwind-tables=no" cargo build --release
```

The crate builds both an `rlib` (for Rust consumers) and a `cdylib`
(`libcfrds.so`) that exports the exact same 254-symbol C ABI as the C
library, so it can be dropped in place of the C-built shared library.

## Versioning

The crate version is derived from the most recent git tag (`X.Y.Z`) by
`build.sh`, mirroring the C library's CMake behaviour. Tags that don't match
`X.Y.Z` are rejected by the build script.

Integration tests run against a live ColdFusion server. They are skipped when
`RDS_HOST` is unset:

```sh
RDS_HOST=rds://user:pass@host:port cargo test --test integration
# or:
RDS_HOST=127.0.0.1 RDS_PORT=8500 RDS_USERNAME=admin RDS_PASSWORD=secret \
  cargo test --test integration
```

See `tests/integration.rs` for the full configuration.

## Notes

- **TLS is not supported.** Like the C library, the transport layer uses plain
  HTTP over TCP. `cfrds` targets remote ColdFusion development and debugging,
  not production environments.
- **Not thread-safe.** A [`Server`] must not be shared across threads; each
  thread should create its own instance.
- String fields from the server are decoded as lossy UTF-8; raw file data is
  exposed as bytes.
