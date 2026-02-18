#!/bin/bash
# Setup script for Memory Tracer project

set -e

echo "========================================"
echo "  Memory Tracer Project Setup"
echo "========================================"
echo ""

# Check if conan is installed
if ! command -v conan &> /dev/null; then
    echo "Conan is not installed. Please install it first:"
    echo "  pip install conan"
    exit 1
fi

echo "Conan version: $(conan --version)"
echo ""

# Detect default profile
echo "Detecting Conan profile..."
conan profile detect

# Install dependencies
echo ""
echo "Installing dependencies..."
conan install . --build=missing

echo ""
echo "========================================"
echo "  Setup completed successfully!"
echo "========================================"
echo ""
echo "You can now build the project with:"
echo "  bazel build //..."
echo ""
echo "Run the test program with:"
echo "  bazel run //examples/test_program:test_program"
echo ""
