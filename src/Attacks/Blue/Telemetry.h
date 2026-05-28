#pragma once

#include "../../USBArmyKnifeCapability.h"

#ifdef ENABLE_BLUE_TEAM_TELEMETRY

#include <string>
#include <WiFiUdp.h>

class Telemetry : public USBArmyKnifeCapability {
public:
  Telemetry();

  virtual void begin(Preferences &prefs);
  virtual void loop(Preferences &prefs);
  virtual void end();

  // Logging & Telemetry APIs
  void logCommand(const std::string& command, const std::string& status, uint32_t durationMs = 0);
  void logKeystroke(char key, uint8_t modifier);
  
private:
  void sendUdpPacket(const char* jsonMsg, size_t len);

  WiFiUDP udpClient;
  std::string syslogHost = "255.255.255.255"; // Default to broadcast
  uint16_t syslogPort = 514;                  // Standard syslog / logging port
  bool isTelemetryEnabled = true;
};

namespace Attacks::Blue {
  extern Telemetry Logger;
}

#endif
