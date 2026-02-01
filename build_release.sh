#!/bin/bash
set -e  # Stop script immediately if any command fails

echo "========================================"
echo "    Desktop-D30 Release Builder 🚀"
echo "========================================"

# 1. Clean previous builds to ensure no debug symbols remain
if [ -d "build" ]; then
    echo "[1/4] Cleaning old build directory..."
    rm -rf build
fi
mkdir build
cd build

# 2. Configure CMake in Release Mode
# -DCMAKE_BUILD_TYPE=Release optimizes the code and strips out debug info
echo "[2/4] Configuring Project (Release Mode)..."
cmake -DCMAKE_BUILD_TYPE=Release ..

# 3. Compile the project
# -j$(nproc) uses all available CPU cores for speed
echo "[3/4] Compiling Source Code..."
make -j$(nproc)

# 4. Generate Installers using CPack
echo "[4/4] Packaging Installers..."
cpack -G "DEB;TGZ"

echo "========================================"
echo "    ✅ BUILD SUCCESSFUL"
echo "========================================"
echo "Installers available in build/:"
ls -lh Desktop-D30-*.deb Desktop-D30-*.tar.gz