//! A minimal `ls`/`cat`-style CLI demonstrating the cfrds Rust library.
//!
//! ```sh
//! RDS_HOST=127.0.0.1 RDS_PORT=8500 RDS_USERNAME=admin RDS_PASSWORD=secret \
//!   cargo run --example fs -- ls /
//! RDS_HOST=... cargo run --example fs -- cat /path/to/file.txt
//! ```

use cfrds::{Server, Status};

fn server() -> Server {
    let host = std::env::var("RDS_HOST").unwrap_or_else(|_| "127.0.0.1".into());
    let port = std::env::var("RDS_PORT")
        .ok()
        .and_then(|v| v.parse().ok())
        .unwrap_or(8500);
    let username = std::env::var("RDS_USERNAME").unwrap_or_else(|_| "admin".into());
    let password = std::env::var("RDS_PASSWORD").unwrap_or_default();
    Server::new(&host, port, &username, &password).expect("failed to create server context")
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 2 {
        eprintln!("usage: {} <ls|cat> <path>", args[0]);
        std::process::exit(2);
    }

    let server = server();

    let result = match args[1].as_str() {
        "ls" => {
            let path = args.get(2).map(String::as_str).unwrap_or("/");
            list_dir(&server, path)
        }
        "cat" => {
            let path = args.get(2).map(String::as_str).unwrap_or("/");
            print_file(&server, path)
        }
        "root" => match server.file_get_root_dir() {
            Ok(root) => {
                println!("{root}");
                Ok(())
            }
            Err(e) => Err(e),
        },
        other => {
            eprintln!("unknown command: {other}");
            std::process::exit(2);
        }
    };

    if let Err(e) = result {
        eprintln!("error: {e}");
        std::process::exit(1);
    }
}

fn list_dir(server: &Server, path: &str) -> Result<(), cfrds::Error> {
    let dir = server.browse_dir(path)?;
    for item in dir.iter() {
        println!("{}{}", item.kind, item.name);
    }
    Ok(())
}

fn print_file(server: &Server, path: &str) -> Result<(), cfrds::Error> {
    let content = server.file_read(path)?;
    std::io::Write::write_all(&mut std::io::stdout(), content.data())
        .map_err(|e| cfrds::Error::new(Status::CommandFailed, e.to_string()))?;
    Ok(())
}
