#!/bin/bash
set -e

PROJECT_DIR=$(pwd)
GTEST_DIR="$PROJECT_DIR/libs/googletest"

echo "=== Project extra libraries setup ==="

echo
echo "I. Configuring Google Test..."
mkdir -p "$PROJECT_DIR/libs"
if [ -d "$GTEST_DIR" ]; then
    echo "Google Test directory already exists. Skipping clone."
else
    echo "Cloning Google Test..."
    git clone https://github.com/google/googletest.git "$GTEST_DIR"
    echo "Google Test cloned successfully!"
fi

if [ ! -d "$GTEST_DIR/build" ]; then
    echo "Building Google Test library..."
    mkdir -p "$GTEST_DIR/build"
    cd "$GTEST_DIR/build"
    cmake ..
    make
    echo "Google Test built successfully!"
else
    echo "Google Test build directory already exists."
fi

echo
echo "=== Setup process finished! ==="
