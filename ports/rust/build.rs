//! Build script: sets the shared-library soname so the Rust `cdylib` can be a
//! drop-in replacement for the C `libcfrds.so`.

fn main() {
    let version = env!("CARGO_PKG_VERSION");
    // Matches the C build's SONAME convention: libcfrds.so.X.Y.Z
    println!("cargo:rustc-cdylib-link-arg=-Wl,-soname,libcfrds.so.{version}");
}
