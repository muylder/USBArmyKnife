#include "CaptivePortal.h"
#include "../../Debug/Logging.h"
#include "../../Devices/WiFi/HardwareWiFi.h"

#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#endif

#define TAG_CAPTIVE "CaptivePortal"

namespace Attacks::Red {
  CaptivePortal Portal;
}

using namespace Attacks::Red;

CaptivePortal::CaptivePortal() {}

void CaptivePortal::begin(Preferences& prefs) {
#ifdef ARDUINO_ARCH_ESP32
  // Start DNS Server on port 53, intercept all queries ("*")
  // and resolve to the ESP32's current IP address
  if (Devices::WiFi.getState() && WiFi.getMode() == WIFI_MODE_AP) {
    dnsServer.start(53, "*", WiFi.softAPIP());
    Debug::Log.info(TAG_CAPTIVE, "Captive Portal DNS Server started.");
    isEnabled = true;
  } else {
    isEnabled = false;
  }
#endif
}

void CaptivePortal::loop(Preferences& prefs) {
#ifdef ARDUINO_ARCH_ESP32
  if (isEnabled) {
    dnsServer.processNextRequest();
  }
#endif
}

void CaptivePortal::end() {
#ifdef ARDUINO_ARCH_ESP32
  if (isEnabled) {
    dnsServer.stop();
    isEnabled = false;
    Debug::Log.info(TAG_CAPTIVE, "Captive Portal DNS Server stopped.");
  }
#endif
}
