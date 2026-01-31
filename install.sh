#!/bin/bash

# Ensure we are running as root
if [ "$EUID" -ne 0 ]
  then echo "Please run as root (sudo ./install.sh)"
  exit
fi

echo "--- Building LabelForge ---"
rm -rf build
mkdir build
cd build
cmake ..
make
cd ..

echo "--- Installing to /opt/LabelForge ---"
# Create the directory
mkdir -p /opt/LabelForge

# Copy the binary
cp build/LabelForge /opt/LabelForge/

# Copy the assets folder
cp -r assets /opt/LabelForge/

# Fix permissions so you can read/write if necessary
chmod -R 755 /opt/LabelForge

echo "--- Creating Launcher ---"
# Create a wrapper script that cd's into the dir before running
# This fixes the "Relative Path" issue for assets
echo '#!/bin/bash' > /usr/local/bin/labelforge
echo 'cd /opt/LabelForge' >> /usr/local/bin/labelforge
echo './LabelForge' >> /usr/local/bin/labelforge

# Make it executable
chmod +x /usr/local/bin/labelforge

echo "--- Registering Desktop Icon ---"
cp LabelForge.desktop /usr/share/applications/

echo "--- Setting Bluetooth Permissions ---"
# Allow the binary to access raw bluetooth sockets without sudo
setcap 'cap_net_raw,cap_net_admin+eip' /opt/LabelForge/LabelForge

echo "Done! You can now launch 'LabelForge' from your applications menu."