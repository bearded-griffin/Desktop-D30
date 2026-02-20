#!/bin/bash

# Stop on error
set -e

echo "========================================"
echo "    Desktop-D30 AppImage Builder 📦"
echo "========================================"

# 1. Clean and Build
rm -rf build_appimage
mkdir build_appimage
cd build_appimage

echo "[1/5] Configuring Project (Release Mode)..."
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=OFF ..

echo "[2/5] Compiling..."
make -j$(nproc)

# 2. Prepare AppDir
echo "[3/5] Installing to AppDir..."
make install DESTDIR=AppDir

# 3. Download linuxdeploy
echo "[4/5] Downloading linuxdeploy..."
if [ ! -f linuxdeploy-x86_64.AppImage ]; then
    wget -N https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
fi
if [ ! -f linuxdeploy-plugin-appimage-x86_64.AppImage ]; then
    wget -N https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage
fi

chmod +x linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-plugin-appimage-x86_64.AppImage

# 4. Generate AppImage
echo "[5/5] Generating AppImage..."

export ARCH=x86_64
export OUTPUT="Desktop-D30-x86_64.AppImage"

./linuxdeploy-x86_64.AppImage --appdir AppDir \
    --output appimage

echo "========================================"
echo "    ✅ APPIMAGE GENERATED"
echo "========================================"
mv Desktop-D30*.AppImage ..
cd ..
echo "AppImage available at: $(ls Desktop-D30*.AppImage)"
