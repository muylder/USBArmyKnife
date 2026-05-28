#pragma once

#include "../../USBArmyKnifeCapability.h"

namespace Attacks::Blue {

class NetworkScanner : public USBArmyKnifeCapability {
public:
  NetworkScanner();

  virtual void begin(Preferences& prefs);
  virtual void loop(Preferences& prefs);
  virtual void end();

private:
  bool isEnabled = false;
  unsigned long lastScanTime = 0;
  int currentTargetIndex = 0;
  int currentPortIndex = 0;
  
  // Basic scan state machine
  enum ScanState {
      IDLE,
      CONNECTING,
      NEXT_TARGET
  };
  ScanState state = IDLE;
};

extern NetworkScanner Scanner;

} // namespace Attacks::Blue
