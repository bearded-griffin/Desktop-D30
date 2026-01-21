/*!***************************************************
 * @file     printer.cpp
 * @brief    Handles the printer communication
 * @details  Handles the bluetooth connection needed to
 * talk to the printer.
 * @note     
 * @date     2026.01.20
 * @author   bearded.griffin
 ****************************************************/

#include "printer.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

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

/*!***************************************************
 * @brief    Start Non-Blocking Scan
 * @details  Launches the scan in a background thread
 * @return   void
 * @date     2026.01.20
 ****************************************************/
void Printer::StartScan() {
    if (scanning) return; // Already scanning

    scanning = true;
    scanComplete = false;

    // Join previous thread if it exists but finished
    if (scanThread.joinable()) scanThread.join();

    // Launch new thread
    scanThread = std::thread([this]() {
        std::vector<BluetoothDevice> results = this->ScanInternal();
        
        // Lock and save results safely
        {
            std::lock_guard<std::mutex> lock(scanMutex);
            lastScanResults = results;
        }
        
        scanning = false;
        scanComplete = true;
    });
}

/*!***************************************************
 * @brief    Check for new results
 * @return   bool
 ****************************************************/
bool Printer::HasScanResults() {
    return scanComplete;
}

/*!***************************************************
 * @brief    Get Results
 * @details  Returns the results and resets the "New Data" flag
 * @return   std::vector<BluetoothDevice>
 ****************************************************/
std::vector<BluetoothDevice> Printer::GetScanResults() {
    std::lock_guard<std::mutex> lock(scanMutex);
    scanComplete = false; // Reset flag so we don't re-read old data
    return lastScanResults;
}

/*!***************************************************
 * @brief    Internal Scan Logic
 * @details  The actual blocking HCI calls (run on thread)
 * @return   std::vector<BluetoothDevice>
 ****************************************************/
std::vector<BluetoothDevice> Printer::ScanInternal() {
    std::vector<BluetoothDevice> devices;

    // 1. Get the ID of the first available bluetooth adapter
    int dev_id = hci_get_route(NULL);
    if (dev_id < 0) {
        std::cerr << "[Printer] No Bluetooth Adapter Found." << std::endl;
        return devices;
    }

    // 2. Open the adapter
    int sock = hci_open_dev(dev_id);
    if (sock < 0) {
        std::cerr << "[Printer] Failed to open Bluetooth Adapter. (Permission Issue? Try sudo setcap)" << std::endl;
        return devices;
    }

    // 3. Scan for devices (8 seconds standard scan)
    int len = 8; 
    int max_rsp = 255;
    int flags = IREQ_CACHE_FLUSH;
    
    inquiry_info* ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
    
    // std::cout << "Scanning for devices..." << std::endl;
    int num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
    
    if (num_rsp < 0) perror("hci_inquiry");

    // 4. Resolve Names
    for (int i = 0; i < num_rsp; i++) {
        BluetoothDevice dev;
        char addr[19] = { 0 };
        char name[248] = { 0 };

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

/*!***************************************************
 * @brief    Connect
 * @details  Connects to the found device
 * @param    address std::string& 
 * @return   bool If it is connected or not
 * @date     2026.01.20
 * @author   bearded.griffin
 ****************************************************/
bool Printer::Connect(const std::string& address) {
    if (connected) Disconnect();

    struct sockaddr_rc addr = { 0 };
    int status;

    // allocate socket
    sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) 1; // D30 usually listens on Channel 1
    str2ba(address.c_str(), &addr.rc_bdaddr);

    // connect to server
    std::cout << "Connecting to " << address << "..." << std::endl;
    status = connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    if (status == 0) {
        connected = true;
        connectedDeviceName = address; // Ideally update this with real name
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

/*!***************************************************
 * @brief    Disconnect
 * @details  Disconnect from the printer.
 * @date     2026.01.20
 * @author   bearded.griffin
 ****************************************************/
void Printer::Disconnect() {
    if (sock >= 0) {
        close(sock);
        sock = -1;
    }
    connected = false;
    connectedDeviceName.clear();
}

/*!***************************************************
 * @brief    Sends the bytes down the sock
 * @details  Writes the data to the printer.
 * @param    data const std::vector<uint8_t>&
 * @return   bool
 * @note     
 * @date     2026.01.20
 * @author   bearded.griffin
 ****************************************************/
bool Printer::Write(const std::vector<uint8_t>& data) {
    if (!connected || sock < 0) return false;

    // write() returns the number of bytes sent
    ssize_t bytesSent = write(sock, data.data(), data.size());
    
    if (bytesSent < 0) {
        perror("[Printer] Write Failed");
        return false;
    }
    
    // Optional: Check if bytesSent == data.size() to ensure partial writes didn't happen
    return true;
}