#pragma once

#include "../../USBArmyKnifeCapability.h"

#ifdef ARDUINO_ARCH_ESP32
#include <DNSServer.h>
#endif

namespace Attacks::Red {

class CaptivePortal : public USBArmyKnifeCapability {
public:
  CaptivePortal();

  virtual void begin(Preferences& prefs);
  virtual void loop(Preferences& prefs);
  virtual void end();

private:
#ifdef ARDUINO_ARCH_ESP32
  DNSServer dnsServer;
#endif
  bool isEnabled = true;
};

extern CaptivePortal Portal;

} // namespace Attacks::Red
