# cfrds 🚀

[![C](https://img.shields.io/badge/C-00599C?logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Python](https://img.shields.io/badge/Python-3776AB?logo=python&logoColor=white)](https://python.org)
[![TypeScript](https://img.shields.io/badge/TypeScript-3178C6?logo=typescript&logoColor=white)](https://typescriptlang.org)
[![License: LGPL v3](https://img.shields.io/badge/License-LGPL_v3-blue?logo=gnu&logoColor=white)](https://www.gnu.org/licenses/lgpl-3.0)
[![AUR version](https://img.shields.io/aur/version/cfrds?logo=arch-linux)](https://aur.archlinux.org/packages/cfrds)
[![Windows Installer / ZIP](https://img.shields.io/badge/Windows-Installer%20%2F%20ZIP-0078D6?logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA4OCA4OCI+PHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTAgMTIuNDAybDM1LjY4Ny00Ljg2LjAxNiAzNC40MjMtMzUuNjcuMjAzem0zNS42NyAzMy41MjlSLjAyOCAzNC40NTNMLjAyOCA3NS40OC4wMDEgNDUuNzI4em00LjMyNi0zOS4wMjdMODcuOTE0IDB2NDEuNTI3bC00Ny45MTguMzc2em00Ny45MjkgNDUuMDM0Vjg4TDM5Ljk5NiA3Ni4wNDdsLjAyNC0zNC42MnoiLz48L3N2Zz4=)](https://github.com/bokic/cfrds/releases/latest)

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

### 🪟 Windows (Installer & Portable ZIP)
Download the installer (`.exe`) or portable archive (`.zip`) from the [Releases](https://github.com/bokic/cfrds/releases/latest) page.

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
