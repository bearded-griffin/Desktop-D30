# Installation Guide

Desktop-D30 is available for both Linux and Windows.

## Windows

1. Download the latest `Desktop-D30.exe` from the releases page.
2. Ensure the `assets/` folder is located in the same directory as the `.exe` file.
3. Simply double-click the `.exe` to run. No installation is required.

## Linux

### Debian/Ubuntu (.deb)
1. Download the `.deb` package.
2. Install using your package manager:
   ```bash
   sudo apt install ./Desktop-D30-1.0.0-Linux.deb
   ```

### Generic Linux (.tar.gz)
1. Extract the archive to a folder of your choice.
2. Run the `Desktop-D30` executable directly.

### Building from Source
If you wish to build the application yourself, you will need:
- CMake (3.14+)
- A C++17 compiler (GCC or Clang)
- Bluetooth development headers (`libbluetooth-dev` on Ubuntu)

```bash
git clone https://forgejo.example.com/bearded-griffin/Desktop-D30
cd Desktop-D30
make build
./build/Desktop-D30
```
