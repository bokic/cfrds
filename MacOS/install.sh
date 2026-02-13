#!/bin/zsh

# Fail on error
set -e

# Capture script name for usage
SCRIPT_NAME=${0:t}

# Usage function
usage() {
    echo "Usage: $SCRIPT_NAME [-f|--force] [-h|--help]"
    echo "  -f, --force    Uninstall previous version without confirmation"
    echo "  -h, --help     Show this help message"
    exit 1
}

# Parse arguments
FORCE=false
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -f|--force) FORCE=true ;;
        -h|--help) usage ;;
        *) echo "Unknown parameter passed: $1"; usage ;;
    esac
    shift
done

# Check dependencies
if ! command -v brew &> /dev/null; then
    echo "Error: Homebrew is not installed. Please install it first."
    exit 1
fi

if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed. Please install it first."
    echo "You can install it with: brew install cmake"
    exit 1
fi

# Get the directory where the script is located
SCRIPT_DIR=${0:a:h}
FORMULA_PATH="${SCRIPT_DIR}/cfrds.rb"

# Uninstall if already installed
if brew list cfrds &>/dev/null; then
    if [ "$FORCE" = true ]; then
        echo "Uninstalling previous version of cfrds..."
        brew uninstall cfrds
    else
        read "response?cfrds is already installed. Do you want to uninstall and reinstall? [y/N] "
        if [[ "$response" =~ ^[Yy]$ ]]; then
            echo "Uninstalling previous version of cfrds..."
            brew uninstall cfrds
        else
            echo "Aborting installation."
            exit 0
        fi
    fi
fi

# Get absolute path to the project root (parent of MacOS dir)
PROJECT_ROOT=$(cd "${SCRIPT_DIR}/.." && pwd)

# Get absolute path to the project root (parent of MacOS dir)
PROJECT_ROOT=$(cd "${SCRIPT_DIR}/.." && pwd)
BUILD_DIR="${PROJECT_ROOT}/build"

echo "Building and installing cfrds from local source at ${PROJECT_ROOT}..."

# Clean build directory
rm -rf "${BUILD_DIR}"

# Configure
# We check if ninja is available for faster builds, otherwise default
GENERATOR=""
if command -v ninja &> /dev/null; then
    GENERATOR="-G Ninja"
fi

# Ensure dependencies are linked/found. Homebrew prefix.
if command -v brew &> /dev/null; then
    BREW_PREFIX=$(brew --prefix)
    CMAKE_PREFIX_PATH="$BREW_PREFIX"
else
    BREW_PREFIX="/usr/local"
fi

echo "Configuring CMake..."
cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${BREW_PREFIX}" \
    -DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH}" \
    -DCMAKE_INSTALL_RPATH="${BREW_PREFIX}/lib" \
    $GENERATOR

echo "Building..."
cmake --build "${BUILD_DIR}" --config Release

echo "Installing..."
# Use sudo for install if prefix is not writable
if [ -w "${BREW_PREFIX}" ]; then
    cmake --install "${BUILD_DIR}" --config Release
    
    # Manually fix RPATH to be safe
    echo "Fixing RPATH..."
    install_name_tool -add_rpath "${BREW_PREFIX}/lib" "${BREW_PREFIX}/bin/cfrds" 2>/dev/null || true
else
    echo "Installation to ${BREW_PREFIX} requires sudo privileges."
    sudo cmake --install "${BUILD_DIR}" --config Release
    
    # Manually fix RPATH with sudo
    echo "Fixing RPATH..."
    sudo install_name_tool -add_rpath "${BREW_PREFIX}/lib" "${BREW_PREFIX}/bin/cfrds" 2>/dev/null || true
fi

echo "Installation complete!"
echo "You can verify the installation by running: cfrds --version"
