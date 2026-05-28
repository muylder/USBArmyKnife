#include "NetworkScanner.h"
#include "../../Debug/Logging.h"
#include "Telemetry.h"
#include "../../Devices/USB/USBCore.h"

#ifdef ARDUINO_ARCH_ESP32
#include <WiFiClient.h>
#endif

#define TAG_SCANNER "NetworkScanner"

namespace Attacks::Blue {
  NetworkScanner Scanner;
}

using namespace Attacks::Blue;

static const char* targetIPs[] = {"10.0.0.1", "10.0.0.3"};
static const uint16_t scanPorts[] = {22, 80, 443, 445, 3389, 8080};
static const int numTargets = 2;
static const int numPorts = 6;

NetworkScanner::NetworkScanner() {}

void NetworkScanner::begin(Preferences& prefs) {
  isEnabled = true;
  state = IDLE;
  lastScanTime = millis();
}

void NetworkScanner::loop(Preferences& prefs) {
#ifdef ARDUINO_ARCH_ESP32
  if (!isEnabled) return;
  
  // Only run if NCM is the active USB profile
  if (Devices::USB::Core.currentDeviceType() != USBDeviceType::NCM) return;

  switch(state) {
    case IDLE:
      if (millis() - lastScanTime > 30000) { // Scan every 30 seconds
        state = CONNECTING;
        currentTargetIndex = 0;
        currentPortIndex = 0;
        Debug::Log.info(TAG_SCANNER, "Starting Network Scan...");
      }
      break;

    case CONNECTING:
    {
      WiFiClient client;
      client.setTimeout(1); // 1 second timeout
      const char* ip = targetIPs[currentTargetIndex];
      uint16_t port = scanPorts[currentPortIndex];
      
      if (client.connect(ip, port)) {
        std::string msg = "Open Port Found: " + std::string(ip) + ":" + std::to_string(port);
        Debug::Log.info(TAG_SCANNER, msg);
        Attacks::Blue::Logger.logCommand("network_scanner", msg);
        client.stop();
      }

      currentPortIndex++;
      if (currentPortIndex >= numPorts) {
        currentPortIndex = 0;
        currentTargetIndex++;
        if (currentTargetIndex >= numTargets) {
            state = IDLE;
            lastScanTime = millis();
            Debug::Log.info(TAG_SCANNER, "Network Scan Complete.");
        }
      }
      break;
    }
    
    default:
      state = IDLE;
      break;
  }
#endif
}

void NetworkScanner::end() {
  isEnabled = false;
}
