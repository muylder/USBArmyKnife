#include "Telemetry.h"

#ifdef ENABLE_BLUE_TEAM_TELEMETRY

#include "../../Debug/Logging.h"
#include "../../Devices/WiFi/HardwareWiFi.h"
#include <ArduinoJson.h>

#define TAG_TELEMETRY "BlueTelemetry"

#define TELEMETRY_ENABLED_SETTING "telemetryEnabled"
#define TELEMETRY_ENABLED_DEFAULT true

#define TELEMETRY_HOST_SETTING "telemetryHost"
#define TELEMETRY_HOST_DEFAULT "255.255.255.255"

#define TELEMETRY_PORT_SETTING "telemetryPort"
#define TELEMETRY_PORT_DEFAULT (uint16_t)514

namespace Attacks::Blue {
  Telemetry Logger;
}

Telemetry::Telemetry() {
}

void Telemetry::begin(Preferences &prefs) {
  registerUserConfigurableSetting(CATEGORY_WIFI, TELEMETRY_ENABLED_SETTING, USBArmyKnifeCapability::SettingType::Bool, TELEMETRY_ENABLED_DEFAULT);
  registerUserConfigurableSetting(CATEGORY_WIFI, TELEMETRY_HOST_SETTING, USBArmyKnifeCapability::SettingType::String, TELEMETRY_HOST_DEFAULT);
  registerUserConfigurableSetting(CATEGORY_WIFI, TELEMETRY_PORT_SETTING, USBArmyKnifeCapability::SettingType::UInt16, TELEMETRY_PORT_DEFAULT);

  isTelemetryEnabled = prefs.getBool(TELEMETRY_ENABLED_SETTING, TELEMETRY_ENABLED_DEFAULT);
  
  String host = prefs.getString(TELEMETRY_HOST_SETTING, TELEMETRY_HOST_DEFAULT);
  syslogHost = std::string(host.c_str());
  
  syslogPort = prefs.getUShort(TELEMETRY_PORT_SETTING, TELEMETRY_PORT_DEFAULT);

  Debug::Log.info(TAG_TELEMETRY, "Blue Team Telemetry Logger started. Target: " + syslogHost + ":" + std::to_string(syslogPort));
}

void Telemetry::loop(Preferences &prefs) {
  // Telemetry is trigger-based, loop does not need polling
}

void Telemetry::end() {
  udpClient.stop();
}

void Telemetry::logCommand(const std::string& command, const std::string& status, uint32_t durationMs) {
  if (!isTelemetryEnabled || !Devices::WiFi.getState()) {
    return;
  }

  JsonDocument doc;
  doc["event"] = "command_execution";
  doc["command"] = command;
  doc["status"] = status;
  doc["duration_ms"] = durationMs;
  doc["device"] = "USB_Army_Knife_S3";
  doc["timestamp"] = millis();

  char buffer[256];
  size_t len = serializeJson(doc, buffer);
  sendUdpPacket(buffer, len);
}

void Telemetry::logKeystroke(char key, uint8_t modifier) {
  if (!isTelemetryEnabled || !Devices::WiFi.getState()) {
    return;
  }

  JsonDocument doc;
  doc["event"] = "keystroke_injected";
  char keyStr[2] = {key, 0};
  doc["key"] = keyStr;
  doc["modifier"] = modifier;
  doc["device"] = "USB_Army_Knife_S3";
  doc["timestamp"] = millis();

  char buffer[256];
  size_t len = serializeJson(doc, buffer);
  sendUdpPacket(buffer, len);
}

void Telemetry::sendUdpPacket(const char* jsonMsg, size_t len) {
  // Only transmit if WiFi is connected
  if (Devices::WiFi.getState()) {
    udpClient.beginPacket(syslogHost.c_str(), syslogPort);
    udpClient.write((const uint8_t*)jsonMsg, len);
    udpClient.endPacket();
  }
}

#endif
