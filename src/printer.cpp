//  This file is part of Desktop-D30
//  Copyright (C) 2026 Chris Griffin (bearded-griffin)
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation version 3 of the License.
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

/*!***************************************************
 * @file     src/printer.cpp
 * @brief    Handles the printer communication
 * @details  Handles the bluetooth connection needed to
 * talk to the printer.
 * @note
 * @date     2026.01.20
 * @author   bearded.griffin
 ****************************************************/

#include "printer.h"
#include <iostream>

#ifdef __linux__
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

/*!***************************************************
 * @brief    Destructor for Printer
 * @details  Ensures the scan thread is joined before destruction
 * @date     2026.01.20
 * @author   bearded.griffin
 ****************************************************/
Printer::~Printer() {
  Disconnect();
  if (scanThread.joinable()) {
    scanThread.join();
  }
}

#ifdef __linux__
// --- LINUX IMPLEMENTATION ---

void Printer::StartScan() {
  if (scanning)
    return; // Already scanning

  scanning = true;
  scanComplete = false;

  if (scanThread.joinable())
    scanThread.join();

  scanThread = std::thread([this]() {
    std::vector<BluetoothDevice> results = this->ScanInternal();
    {
      std::lock_guard<std::mutex> lock(scanMutex);
      lastScanResults = results;
    }
    scanning = false;
    scanComplete = true;
  });
}

bool Printer::HasScanResults() { return scanComplete; }

std::vector<BluetoothDevice> Printer::GetScanResults() {
  std::lock_guard<std::mutex> lock(scanMutex);
  scanComplete = false;
  return lastScanResults;
}

std::vector<BluetoothDevice> Printer::ScanInternal() {
  std::vector<BluetoothDevice> devices;
  int dev_id = hci_get_route(NULL);
  if (dev_id < 0) {
    std::cerr << "[Printer] No Bluetooth Adapter Found." << std::endl;
    return devices;
  }
  int sock = hci_open_dev(dev_id);
  if (sock < 0) {
    std::cerr << "[Printer] Failed to open Bluetooth Adapter." << std::endl;
    return devices;
  }
  int len = 8;
  int max_rsp = 255;
  int flags = IREQ_CACHE_FLUSH;
  inquiry_info *ii = (inquiry_info *)malloc(max_rsp * sizeof(inquiry_info));
  int num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
  if (num_rsp < 0) perror("hci_inquiry");

  for (int i = 0; i < num_rsp; i++) {
    BluetoothDevice dev;
    char addr[19] = {0};
    char name[248] = {0};
    ba2str(&(ii[i].bdaddr), addr);
    dev.address = addr;
    if (hci_read_remote_name(sock, &(ii[i].bdaddr), sizeof(name), name, 0) < 0) {
      dev.name = "[Unknown]";
    } else {
      dev.name = name;
    }
    devices.push_back(dev);
  }
  free(ii);
  close(sock);
  return devices;
}

bool Printer::Connect(const std::string &address) {
  if (connected) Disconnect();
  struct sockaddr_rc addr = {0};
  int status;
  sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  addr.rc_family = AF_BLUETOOTH;
  addr.rc_channel = (uint8_t)1;
  str2ba(address.c_str(), &addr.rc_bdaddr);
  std::cout << "Connecting to " << address << "..." << std::endl;
  status = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
  if (status == 0) {
    connected = true;
    connectedDeviceName = address;
    std::cout << "Connected!" << std::endl;
    return true;
  } else {
    perror("Failed to connect");
    connected = false;
    close(sock);
    sock = -1;
    return false;
  }
}

void Printer::Disconnect() {
  if (sock >= 0) {
    close(sock);
    sock = -1;
  }
  connected = false;
  connectedDeviceName.clear();
}

bool Printer::Write(const std::vector<uint8_t> &data) {
  if (!connected || sock < 0) return false;
  ssize_t bytesSent = write(sock, data.data(), data.size());
  if (bytesSent < 0) {
    perror("[Printer] Write Failed");
    return false;
  }
  return true;
}

#else
// --- WINDOWS / STUB IMPLEMENTATION ---

void Printer::StartScan() {
    std::cout << "[Printer] Scanning not supported on this platform yet." << std::endl;
    scanning = false;
    scanComplete = true;
    lastScanResults.clear();
}

bool Printer::HasScanResults() { return false; }

std::vector<BluetoothDevice> Printer::GetScanResults() { return {}; }

std::vector<BluetoothDevice> Printer::ScanInternal() { return {}; }

bool Printer::Connect(const std::string &address) {
    std::cout << "[Printer] Connection not supported on this platform yet." << std::endl;
    return false;
}

void Printer::Disconnect() {
    connected = false;
}

bool Printer::Write(const std::vector<uint8_t> &data) {
    std::cout << "[Printer] Writing not supported on this platform yet." << std::endl;
    return false;
}

#endif