#!/bin/bash

set -e

RAYLIB_VERSION="5.5.0"
RAYLIB_DIR="./vendor/raylib"
RAYGUI_VERSION="4.0"
RAYGUI_DIR="./vendor/raygui"

check_raylib() {
    if [ -d "$RAYLIB_DIR" ] && [ -f "$RAYLIB_DIR/src/libraylib.a" ]; then
        echo "Raylib installation found."
    else
        return 1
    fi
    if [ -d "$RAYGUI_DIR" ] && [ -f "$RAYGUI_DIR/src/raygui.h" ]; then
        echo "RayGUI installation found."
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
    cd ../../..

    echo "Raylib has been successfully installed!"
}

install_raygui() {
    echo "RayGUI not found. Installing raygui $RAYGUI_VERSION..."

    # Clone raylib repository
    if [ ! -d "$RAYGUI_DIR" ]; then
        git clone https://github.com/raysan5/raygui.git "$RAYGUI_DIR" --branch "$RAYGUI_VERSION" --depth 1
    fi

    echo "RayGUI has been successfully installed!"
}

echo "Building..."

if ! check_raylib; then
    install_raylib
    install_raygui
fi

echo "Compiling the program..."
make

echo "Running procedural terrain generator..."
./zzz
