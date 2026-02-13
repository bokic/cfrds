#!/bin/zsh

# Fail on error
set -e

# Get the directory where the script is located
SCRIPT_DIR=${0:a:h}
FORMULA_PATH="${SCRIPT_DIR}/cfrds.rb"

echo "Installing cfrds from ${FORMULA_PATH}..."

# Uninstall if already installed
if brew list cfrds &>/dev/null; then
    echo "Uninstalling previous version of cfrds..."
    brew uninstall cfrds
fi

# Install from local formula
# --build-from-source to ensure it builds using the definition in the file
brew install --build-from-source "${FORMULA_PATH}"

echo "Installation complete!"
echo "You can verify the installation by running: cfrds --version"
