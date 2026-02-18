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
#include <iomanip>
#include <sstream>

#ifdef __linux__
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include "win_fix.h"
#include <winsock2.h>
#include <ws2bth.h>
#include <bluetoothapis.h>

// Helper to manage Winsock lifetime
class WSASession {
public:
    WSASession() {
        WSAData data;
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            std::cerr << "[Printer] WSAStartup failed." << std::endl;
        }
    }
    ~WSASession() {
        WSACleanup();
    }
};

static WSASession globalWSASession;

namespace {
std::string BthAddrToString(BTH_ADDR addr) {
    std::stringstream ss;
    for (int i = 5; i >= 0; i--) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)((addr >> (i * 8)) & 0xff);
        if (i > 0) ss << ":";
    }
    std::string s = ss.str();
    for (auto& c : s) c = toupper(c);
    return s;
}

BTH_ADDR StringToBthAddr(const std::string& s) {
    BTH_ADDR addr = 0;
    unsigned int bytes[6];
    if (sscanf(s.c_str(), "%x:%x:%x:%x:%x:%x", &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5]) == 6) {
        for (int i = 0; i < 6; i++) {
            addr = (addr << 8) + (bytes[i] & 0xff);
        }
    }
    return addr;
}
}
#endif

std::vector<BluetoothDevice> Printer::ScanDevices() {
    return Get().ScanInternal();
}

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
// --- WINDOWS IMPLEMENTATION ---

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

  WSAQUERYSET querySet = {0};
  querySet.dwSize = sizeof(WSAQUERYSET);
  querySet.dwNameSpace = NS_BTH;

  HANDLE hLookup;
  if (WSALookupServiceBegin(&querySet, LUP_CONTAINERS | LUP_RETURN_NAME | LUP_RETURN_ADDR | LUP_FLUSHCACHE, &hLookup) != 0) {
    std::cerr << "[Printer] WSALookupServiceBegin failed: " << WSAGetLastError() << std::endl;
    return devices;
  }

  union {
    CHAR buf[2048];
    WSAQUERYSET res;
  } queryResult;

  DWORD bufSize = sizeof(queryResult);
  while (WSALookupServiceNext(hLookup, LUP_RETURN_NAME | LUP_RETURN_ADDR, &bufSize, &queryResult.res) == 0) {
    BluetoothDevice dev;
    if (queryResult.res.lpszServiceInstanceName) {
        dev.name = queryResult.res.lpszServiceInstanceName;
    } else {
        dev.name = "[Unknown]";
    }

    CSADDR_INFO* addrInfo = (CSADDR_INFO*)queryResult.res.lpcsaBuffer;
    BTH_ADDR bthAddr = ((SOCKADDR_BTH*)addrInfo->RemoteAddr.lpSockaddr)->btAddr;
    dev.address = BthAddrToString(bthAddr);
    
    devices.push_back(dev);
    bufSize = sizeof(queryResult);
  }

  WSALookupServiceEnd(hLookup);
  return devices;
}

bool Printer::Connect(const std::string &address) {
  if (connected) Disconnect();

  sock = (int)socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
  if (sock == (int)INVALID_SOCKET) {
    std::cerr << "[Printer] Failed to create socket: " << WSAGetLastError() << std::endl;
    return false;
  }

  SOCKADDR_BTH addr = {0};
  addr.addressFamily = AF_BTH;
  addr.btAddr = StringToBthAddr(address);
  addr.port = 1; // RFCOMM channel 1

  std::cout << "Connecting to " << address << " (Windows)..." << std::endl;
  if (connect((SOCKET)sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
    connected = true;
    connectedDeviceName = address;
    std::cout << "Connected!" << std::endl;
    return true;
  } else {
    std::cerr << "[Printer] Connection failed: " << WSAGetLastError() << std::endl;
    closesocket((SOCKET)sock);
    sock = -1;
    connected = false;
    return false;
  }
}

void Printer::Disconnect() {
  if (sock != -1) {
    closesocket((SOCKET)sock);
    sock = -1;
  }
  connected = false;
  connectedDeviceName.clear();
}

bool Printer::Write(const std::vector<uint8_t> &data) {
  if (!connected || sock == -1) return false;
  int bytesSent = send((SOCKET)sock, (const char*)data.data(), (int)data.size(), 0);
  if (bytesSent == SOCKET_ERROR) {
    std::cerr << "[Printer] Write Failed: " << WSAGetLastError() << std::endl;
    return false;
  }
  return true;
}

#endif