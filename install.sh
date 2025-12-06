#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PREFIX=${PREFIX:-/usr/local}

echo "Installing neye to $PREFIX..."

cd "$SCRIPT_DIR"

if command -v make >/dev/null 2>&1; then
    make clean
    make all
    sudo make install PREFIX="$PREFIX"
else
    echo "Error: make is required but not installed"
    exit 1
fi

echo "Installation completed successfully!"
echo "You can now run 'neye' from anywhere in the system."
echo ""
echo "Usage examples:"
echo "  neye /path/to/project"
echo "  neye -e '.c .h' -t 20 -r 'make' /path/to/project"
echo "  neye -c ~/.config/neye/config.ini"