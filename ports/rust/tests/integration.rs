//! Integration tests against a live ColdFusion RDS server.
//!
//! These mirror `test.sh` from the C project. They are skipped (with a notice)
//! when no server is configured.
//!
//! Configuration via environment variables (matching `test.sh`):
//! - `RDS_HOST` — hostname (or an `rds://user:pass@host:port` URI)
//! - `RDS_PORT`, `RDS_USERNAME`, `RDS_PASSWORD`
//! - `RDS_DSN`, `RDS_DSN_TABLE`

use cfrds::Server;

struct TestConfig {
    host: String,
    port: u16,
    username: String,
    password: String,
}

fn config() -> Option<TestConfig> {
    let host_raw = std::env::var("RDS_HOST").ok()?;
    if host_raw.is_empty() {
        return None;
    }

    if let Some(rest) = host_raw.strip_prefix("rds://") {
        let (userinfo, hostport) = match rest.find('@') {
            Some(i) => (&rest[..i], &rest[i + 1..]),
            None => ("", rest),
        };
        let (user, pass) = match userinfo.find(':') {
            Some(i) => (&userinfo[..i], &userinfo[i + 1..]),
            None => (userinfo, ""),
        };
        let (host, port) = match hostport.find(':') {
            Some(i) => (&hostport[..i], &hostport[i + 1..]),
            None => (hostport, "8500"),
        };
        return Some(TestConfig {
            host: host.to_string(),
            port: port.parse().unwrap_or(8500),
            username: user.to_string(),
            password: pass.to_string(),
        });
    }

    Some(TestConfig {
        host: host_raw,
        port: std::env::var("RDS_PORT")
            .ok()
            .and_then(|v| v.parse().ok())
            .unwrap_or(8500),
        username: std::env::var("RDS_USERNAME").unwrap_or_else(|_| "admin".into()),
        password: std::env::var("RDS_PASSWORD").unwrap_or_else(|_| "admin".into()),
    })
}

fn server() -> Option<Server> {
    let cfg = config()?;
    Server::new(&cfg.host, cfg.port, &cfg.username, &cfg.password).ok()
}

fn dsn_and_table() -> (Option<String>, Option<String>) {
    (
        std::env::var("RDS_DSN").ok().filter(|s| !s.is_empty()),
        std::env::var("RDS_DSN_TABLE")
            .ok()
            .filter(|s| !s.is_empty()),
    )
}

#[test]
fn file_operations() {
    let Some(server) = server() else {
        println!("SKIP: RDS_HOST not configured; skipping live-server test");
        return;
    };

    let _ = server.file_remove_file("/tmp/rs_testfile.txt");
    let _ = server.file_remove_file("/tmp/rs_testfile2.txt");
    let _ = server.file_remove_dir("/tmp/rs_test_dir");

    // create directory
    server
        .file_create_dir("/tmp/rs_test_dir")
        .expect("create dir should succeed");

    // write + read
    server
        .file_write("/tmp/rs_testfile.txt", b"some file content from rust test")
        .expect("file_write should succeed");
    let content = server
        .file_read("/tmp/rs_testfile.txt")
        .expect("file_read should succeed");
    assert_eq!(
        content.to_string_lossy(),
        "some file content from rust test"
    );
    assert_eq!(content.size(), 32);

    // exists
    assert!(server.file_exists("/tmp/rs_testfile.txt").expect("exists"));
    assert!(!server
        .file_exists("/tmp/rs_missing_file.txt")
        .expect("missing"));

    // rename
    server
        .file_rename("/tmp/rs_testfile.txt", "/tmp/rs_testfile2.txt")
        .expect("file_rename should succeed");
    assert!(server
        .file_exists("/tmp/rs_testfile2.txt")
        .expect("renamed"));

    // browse dir
    let dir = server
        .browse_dir("/tmp")
        .expect("browse_dir should succeed");
    assert!(dir.count() > 0);
    assert!(dir.iter().any(|i| i.name == "rs_testfile2.txt"));

    // root dir
    let root = server.file_get_root_dir().expect("cf root dir");
    assert!(!root.is_empty());

    // cleanup
    server
        .file_remove_file("/tmp/rs_testfile2.txt")
        .expect("remove file");
    server
        .file_remove_dir("/tmp/rs_test_dir")
        .expect("remove dir");
}

#[test]
fn sql_operations() {
    let Some(server) = server() else {
        println!("SKIP: RDS_HOST not configured; skipping live-server test");
        return;
    };

    let supported = server
        .sql_getsupportedcommands()
        .expect("supportedcommands");
    assert!(supported.count() > 0);

    let dsn_list = server.sql_dsninfo().expect("dsninfo");
    if dsn_list.count() == 0 {
        println!("SKIP: no DSNs configured on server");
        return;
    }

    let (dsn, table) = dsn_and_table();
    let dsn = dsn.unwrap_or_else(|| dsn_list.name(0).unwrap_or("").to_string());
    if dsn.is_empty() {
        return;
    }

    let _ = server.sql_dbdescription(&dsn);
    let _ = server.sql_sqlstmnt(&dsn, "SELECT 1");
    let _ = server.sql_sqlmetadata(&dsn, "SELECT 1");

    if let Some(table) = table {
        let _ = server.sql_columninfo(&dsn, &table);
        let _ = server.sql_primarykeys(&dsn, &table);
        let _ = server.sql_foreignkeys(&dsn, &table);
        let _ = server.sql_importedkeys(&dsn, &table);
        let _ = server.sql_exportedkeys(&dsn, &table);
    } else {
        let tables = server.sql_tableinfo(&dsn).expect("tableinfo");
        if tables.count() > 0 {
            if let Some(t) = tables.get(0) {
                let _ = server.sql_columninfo(&dsn, &t.name);
                let _ = server.sql_primarykeys(&dsn, &t.name);
            }
        }
    }
}

#[test]
fn ide_default_and_admin() {
    let Some(server) = server() else {
        println!("SKIP: RDS_HOST not configured; skipping live-server test");
        return;
    };

    let ide = server.ide_default(1).expect("ide_default");
    assert!(!ide.server_version.is_empty() || !ide.client_version.is_empty());

    let _ = server.adminapi_extensions_getcustomtagpaths();
    let _ = server.adminapi_extensions_getmappings();
    let _ = server.adminapi_extensions_setmapping("rs_test_map", "/tmp");
    let _ = server.adminapi_extensions_deletemapping("rs_test_map");
}

#[test]
fn debugger_smoke() {
    // Debugger tests are opt-in: they mutate server-side debugger state
    // (breakpoints/sessions), which can wedge the server if a session is not
    // cleanly stopped. Only run with RDS_TEST_DEBUGGER=1 against a scratch
    // server.
    if std::env::var("RDS_TEST_DEBUGGER").as_deref() != Ok("1") {
        println!("SKIP: debugger test is opt-in (set RDS_TEST_DEBUGGER=1)");
        return;
    }
    let Some(server) = server() else {
        println!("SKIP: RDS_HOST not configured; skipping live-server test");
        return;
    };

    // Start a session and always stop it, even on mid-step failures.
    match server.debugger_start() {
        Ok(session_id) => {
            println!("debugger session started: {session_id}");
            let results: Vec<cfrds::Result<()>> = [
                server.debugger_get_server_info(&session_id).map(|_| ()),
                server.debugger_breakpoint_on_exception(&session_id, true),
                server.debugger_clear_all_breakpoints(&session_id),
            ]
            .into_iter()
            .collect();
            for r in &results {
                if let Err(e) = r {
                    eprintln!("debugger step failed (continuing cleanup): {e}");
                }
            }
            if let Err(e) = server.debugger_stop(&session_id) {
                eprintln!("debugger_stop failed: {e}");
            }
        }
        Err(e) => {
            println!("SKIP: debugger unavailable on server ({e})");
        }
    }
}
