/*!***************************************************
 * @file     printer.h
 * @brief    Handles the printer communication
 * @details  Handles the bluetooth connection neede to
 * talk to the printer.
 * @note
 * @date     2026.01.20
 * @author   bearded.griffin
 ****************************************************/

#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct BluetoothDevice {
  std::string name;
  std::string address;
};

class Printer {
public:
  // Scans for devices (Blocking for simplicity in this step)
  // In a real app, you'd run this in a thread.
  static std::vector<BluetoothDevice> ScanDevices();

  // Starts the scan in a background thread
  void StartScan();

  // Returns true if the thread is still working
  bool IsScanning() const { return scanning; }

  // Returns true if the scan finished and we have new data
  bool HasScanResults();

  // Retrieves the results (clears the 'HasScanResults' flag)
  std::vector<BluetoothDevice> GetScanResults();

  // Connect to a specific address (RFCOMM)
  bool Connect(const std::string &address);

  // Disconnect
  void Disconnect();

  // Check status
  bool IsConnected() const { return connected; }
  std::string GetConnectedName() const { return connectedDeviceName; }

  // Send it to the printer
  bool Write(const std::vector<uint8_t> &data);

  // Singleton access for the UI to use
  static Printer &Get() {
    static Printer instance;
    return instance;
  }

private:
  Printer() = default;
  ~Printer();
  int sock = -1;
  bool connected = false;
  std::string connectedDeviceName;

  // Threading members
  std::atomic<bool> scanning{false};
  std::atomic<bool> scanComplete{false};
  std::vector<BluetoothDevice> lastScanResults;
  std::mutex scanMutex;
  std::thread scanThread;

  // The actual heavy lifting function (private now)
  std::vector<BluetoothDevice> ScanInternal();
};