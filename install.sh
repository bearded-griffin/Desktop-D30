#!/bin/bash

#   This file is part of Desktop-D30
#   Copyright (C) 2026 Chris Griffin (bearded-griffin)
# 
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation version 3 of the License.
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Ensure we are running as root
if [ "$EUID" -ne 0 ]
  then echo "Please run as root (sudo ./install.sh)"
  exit
fi

echo "--- Building Desktop-D30 ---"
rm -rf build
mkdir build
cd build
cmake ..
make
cd ..

echo "--- Installing to /opt/Desktop-D30 ---"
# Create the directory
mkdir -p /opt/Desktop-D30

# Copy the binary
cp build/Desktop-D30 /opt/Desktop-D30/

# Copy the assets folder
cp -r assets /opt/Desktop-D30/

# Fix permissions so you can read/write if necessary
chmod -R 755 /opt/Desktop-D30

echo "--- Creating Launcher ---"
# Create a wrapper script that cd's into the dir before running
# This fixes the "Relative Path" issue for assets
echo '#!/bin/bash' > /usr/local/bin/Desktop-D30
echo 'cd /opt/Desktop-D30' >> /usr/local/bin/Desktop-D30
echo './Desktop-D30' >> /usr/local/bin/Desktop-D30

# Make it executable
chmod +x /usr/local/bin/Desktop-D30

echo "--- Registering Desktop Icon ---"
cp Desktop-D30.desktop /usr/share/applications/

echo "--- Setting Bluetooth Permissions ---"
# Allow the binary to access raw bluetooth sockets without sudo
setcap 'cap_net_raw,cap_net_admin+eip' /opt/Desktop-D30/Desktop-D30

echo "Done! You can now launch 'Desktop-D30' from your applications menu."