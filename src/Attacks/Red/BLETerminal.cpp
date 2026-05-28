#include "BLETerminal.h"

#ifdef ENABLE_RED_TEAM_COVERT_BLE

#include "../../Debug/Logging.h"
#include "../../Devices/Storage/HardwareStorage.h"
#include "../Ducky/DuckyPayload.h"

#define TAG_BLE "BLE_C2"

// Standard Nordic UART Service (NUS) UUIDs
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

namespace Attacks::Red {
  BLETerminal BLE;
}

BLETerminal::BLETerminal() {
}

void BLETerminal::begin(Preferences &prefs) {
  Debug::Log.info(TAG_BLE, "Starting BLE UART Covert C2 Service...");

  NimBLEDevice::init("Device UART");
  
  // High tx power for stable C2 connection
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(this);

  NimBLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        NIMBLE_PROPERTY::NOTIFY
                      );

  pRxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
                      );

  pRxCharacteristic->setCallbacks(this);

  pService->start();

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Debug::Log.info(TAG_BLE, "BLE UART Covert C2 service is advertising.");
}

void BLETerminal::loop(Preferences &prefs) {
  // BLE handles most operations asynchronously via callbacks
}

void BLETerminal::end() {
  if (pServer) {
    NimBLEDevice::deinit(true);
    pServer = nullptr;
    deviceConnected = false;
  }
}

void BLETerminal::onConnect(NimBLEServer* pServer) {
  deviceConnected = true;
  Debug::Log.info(TAG_BLE, "C2 client connected over BLE.");
  sendResponse("\r\n=== USB Army Knife Covert BLE C2 ===\r\nType 'help' for commands.\r\n\r\nknife> ");
}

void BLETerminal::onDisconnect(NimBLEServer* pServer) {
  deviceConnected = false;
  Debug::Log.info(TAG_BLE, "C2 client disconnected.");
  NimBLEDevice::startAdvertising();
}

void BLETerminal::onWrite(NimBLECharacteristic* pCharacteristic) {
  std::string rxValue = pCharacteristic->getValue();

  if (rxValue.length() > 0) {
    for (int i = 0; i < rxValue.length(); i++) {
      char c = rxValue[i];

      // Handle backspace
      if (c == 0x08 || c == 0x7F) {
        if (!inputBuffer.empty()) {
          inputBuffer.pop_back();
          // Echo backspace to terminal
          sendResponse("\b \b");
        }
      }
      // Handle carriage return or line feed
      else if (c == '\r' || c == '\n') {
        sendResponse("\r\n");
        if (!inputBuffer.empty()) {
          processCommand(inputBuffer);
          inputBuffer.clear();
        } else {
          sendResponse("knife> ");
        }
      }
      // General character input
      else if (c >= 32 && c <= 126) {
        inputBuffer += c;
        // Echo back character
        char echoStr[2] = {c, 0};
        sendResponse(echoStr);
      }
    }
  }
}

void BLETerminal::sendResponse(const std::string& msg) {
  if (deviceConnected && pTxCharacteristic != nullptr) {
    // Send in chunks of 20 bytes (standard BLE MTU limit compatibility)
    size_t offset = 0;
    while (offset < msg.length()) {
      size_t len = std::min((size_t)20, msg.length() - offset);
      pTxCharacteristic->setValue((uint8_t*)msg.c_str() + offset, len);
      pTxCharacteristic->notify();
      offset += len;
      delay(10); // short delay to ensure throughput ordering
    }
  }
}

void BLETerminal::processCommand(const std::string& cmd) {
  std::string cleanCmd = cmd;
  // Trim spaces
  cleanCmd.erase(0, cleanCmd.find_first_not_of(" \t\r\n"));
  cleanCmd.erase(cleanCmd.find_last_not_of(" \t\r\n") + 1);

  if (cleanCmd == "help") {
    sendResponse("Commands:\r\n");
    sendResponse("  help                 Show this help menu\r\n");
    sendResponse("  ls                   List payload files (.ds) on SD card\r\n");
    sendResponse("  payload <file>       Load and execute a DuckyScript payload\r\n");
    sendResponse("  status               Get current payload execution state\r\n");
    sendResponse("  stop                 Immediately stop the active payload\r\n");
  } 
  else if (cleanCmd == "ls") {
    sendResponse("Available Payloads:\r\n");
    std::vector<std::string> files = Devices::Storage.listFiles();
    int count = 0;
    for (const auto& file : files) {
      if (file.length() >= 3 && file.substr(file.length() - 3) == ".ds") {
        sendResponse("  - " + file + "\r\n");
        count++;
      }
    }
    if (count == 0) {
      sendResponse("  (No .ds payloads found)\r\n");
    }
  } 
  else if (cleanCmd == "status") {
    std::string runningStatus = Attacks::Ducky.getPayloadRunningStatus();
    sendResponse("Status: " + runningStatus + "\r\n");
  } 
  else if (cleanCmd == "stop") {
    Attacks::Ducky.setPayloadCmdLine("");
    Attacks::Ducky.setPayload("");
    sendResponse("Payload execution halted.\r\n");
  } 
  else if (cleanCmd.rfind("payload ", 0) == 0) {
    std::string filename = cleanCmd.substr(8);
    filename.erase(0, filename.find_first_not_of(" \t"));
    filename.erase(filename.find_last_not_of(" \t") + 1);
    
    if (filename.empty()) {
      sendResponse("Usage: payload <filename.ds>\r\n");
    } else {
      if (!Devices::Storage.doesFileExist(filename)) {
        sendResponse("Error: File '" + filename + "' does not exist on SD card.\r\n");
      } else {
        sendResponse("Executing payload '" + filename + "'...\r\n");
        Attacks::Ducky.setPayload(filename);
      }
    }
  } 
  else {
    sendResponse("Unknown command: " + cleanCmd + "\r\n");
  }

  sendResponse("\r\nknife> ");
}

#endif
