#pragma once

#include "../../USBArmyKnifeCapability.h"

#ifdef ENABLE_RED_TEAM_COVERT_BLE

#include <NimBLEDevice.h>

class BLETerminal : public USBArmyKnifeCapability, public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks {
public:
  BLETerminal();

  virtual void begin(Preferences &prefs);
  virtual void loop(Preferences &prefs);
  virtual void end();

  // Callbacks
  void onConnect(NimBLEServer* pServer) override;
  void onDisconnect(NimBLEServer* pServer) override;
  void onWrite(NimBLECharacteristic* pCharacteristic) override;

private:
  void processCommand(const std::string& cmd);
  void sendResponse(const std::string& msg);

  NimBLEServer* pServer = nullptr;
  NimBLECharacteristic* pTxCharacteristic = nullptr;
  NimBLECharacteristic* pRxCharacteristic = nullptr;
  bool deviceConnected = false;
  std::string inputBuffer;
};

namespace Attacks::Red {
  extern BLETerminal BLE;
}

#endif
