# cfrds 🚀

[![Language: C](https://shields.io)](https://wikipedia.org)
[![Language: Python](https://shields.io)](https://python.org)
[![Language: TypeScript](https://shields.io)](https://typescriptlang.org)
[![License: LGPL v3](https://shields.io)](https://gnu.org)
[![Arch Linux AUR](https://shields.io)](https://archlinux.org)

A powerful, high-performance, cross-platform CLI tool and library for communicating with ColdFusion servers via the **Adobe RDS (Remote Development Services)** protocol. 

Architected with a core **C shared library** for unmatched speed and portability, `cfrds` also features native, standalone implementations in **Python** and **TypeScript**—making it seamless to integrate into modern Node.js backends or automated scripts.

## ✨ Key Features

- **📂 Remote File Management:** Browse directories, upload, and download files directly through RDS.
- **🗄️ Database Operations:** List data sources, inspect table schemas, and execute remote SQL queries.
- **🛠️ Diagnostics & Admin:** Remote debugging, AdminAPI interaction, security analysis of CFML apps, and charts generation.
- **🤖 Automation Friendly:** Every CLI command supports structured JSON output using the `--json` flag.
- **💻 True Cross-Platform:** Native support for Windows, Linux, and macOS (x86 & ARM).

---

## 🚀 Installation

### 📦 Linux (Pre-built Packages)

#### **Arch Linux (AUR)**
Install the package using your favorite AUR helper:
```bash
yay -S cfrds
```

#### **Ubuntu / Debian (PPA)**
Add the official PPA repository and install via APT:
```bash
sudo add-apt-repository ppa:bbarbulovski-gmail/cfrds
sudo apt-get update
sudo apt-get install cfrds
```

### 🐳 Docker
Run instantly using the built-in `Dockerfile`:
```bash
docker build -t cfrds .
docker run --rm cfrds --help
```

---

## 🛠️ Building from Source

### Prerequisites
Make sure you have `cmake` and a C compiler (`gcc` or `clang`) installed.

### Build Steps
```bash
git clone https://github.com
cd cfrds
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

---

## 💡 Quick Usage Examples

### CLI Command
Connect to a remote server and fetch the database tables:
```bash
cfrds --host cfserver.local --port 8500 --rds-password secret --action list-tables --dsn my_datasource
```

### Get Output in JSON
```bash
cfrds --host cfserver.local --action list-files --path "/var/www/html/" --json
```

---

## 🤝 Contributing & Feedback

Contributions, bug reports, and feature requests are highly welcome! 

1. **Fork** the repository.
2. **Create** your feature branch (`git checkout -b feature/AmazingFeature`).
3. **Commit** your changes (`git commit -m 'Add some AmazingFeature'`).
4. **Push** to the branch (`git push origin feature/AmazingFeature`).
5. **Open a Pull Request**.

## 📄 License

This project is licensed under the **GNU Lesser General Public License v3.0 (LGPL-3.0)** - see the [LICENSE](LICENSE) file for details.

Developed with ❤️ by [Boris Barbulovski (bokic)](https://github.com).
