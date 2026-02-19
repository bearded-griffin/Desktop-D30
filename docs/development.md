# Development & Build Guide

This guide is for developers who want to compile Desktop-D30 from source or contribute to the project.

## Prerequisites

Regardless of your operating system, you will need:
* **CMake** (3.14 or higher)
* **C++17 Compiler** (GCC 9+, Clang 10+, or MSVC 2019+)
* **Git** (to clone the repository and dependencies)

### Linux Dependencies
On Debian-based systems (Ubuntu, Mint, etc.), install the following:
```bash
sudo apt update
sudo apt install build-essential cmake git libbluetooth-dev libasound2-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev libxkbcommon-dev
```

## Compiling on Linux (Native)

We provide a `Makefile` wrapper for common CMake commands.

### Debug Build
```bash
make build
```
This will generate the executable in `build/Desktop-D30`.

### Running the App
```bash
make run
```

### Running Tests
```bash
make run_tests
```

## Cross-Compiling for Windows from Linux

You can generate a Windows `.exe` directly from a Linux machine using the MinGW-w64 toolchain.

1. **Install the toolchain**:
   ```bash
   sudo apt install mingw-w64
   ```
2. **Run the cross-compile command**:
   ```bash
   make cross-windows
   ```
This will create a `build_win_cross/` directory containing `Desktop-D30.exe`.

## Compiling on Windows (Native)

### Using MinGW-w64
1. Install [w64devkit](https://github.com/skeeto/w64devkit) or a similar MinGW distribution.
2. Open your terminal and navigate to the project folder.
3. Run:
   ```bash
   mkdir build
   cd build
   cmake -G "MinGW Makefiles" ..
   make
   ```

## Packaging for Release

### Generating Linux Installers
To create `.deb` and `.tar.gz` packages for Linux:
```bash
./build_release.sh
```
The installers will be placed in the `build/` directory. This script uses **CPack** internally.

## Project Structure

* `src/`: C++ source files.
* `include/`: Header files.
* `assets/`: Built-in icons, borders, and fonts.
* `docs/`: Documentation source (Markdown).
* `tests/`: Unit tests using GoogleTest.
* `cmake/`: Custom CMake toolchains and scripts.
