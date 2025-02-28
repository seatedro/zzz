#!/bin/bash

set -e

RAYLIB_VERSION="4.5.0"
RAYLIB_DIR="./vendor/raylib"

check_raylib() {
    if [ -d "$RAYLIB_DIR" ] && [ -f "$RAYLIB_DIR/src/libraylib.a" ]; then
        echo "Raylib installation found."
        return 0
    else
        return 1
    fi
}

install_raylib() {
    echo "Raylib not found. Installing Raylib $RAYLIB_VERSION..."

    # Clone raylib repository
    if [ ! -d "$RAYLIB_DIR" ]; then
        git clone https://github.com/raysan5/raylib.git "$RAYLIB_DIR" --branch "$RAYLIB_VERSION" --depth 1
    fi

    cd "$RAYLIB_DIR"/src
    make PLATFORM=PLATFORM_DESKTOP
    cd ../..

    echo "Raylib has been successfully installed!"
}

echo "Building..."

if ! check_raylib; then
    install_raylib
fi

echo "Compiling the program..."
make

echo "Running procedural terrain generator..."
./zzz
