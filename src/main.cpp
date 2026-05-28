#if ARDUINO_USB_MODE
#warning This sketch should be used when USB is in OTG mode
void setup() {}
void loop() {}
#else

#include <uptime.h>
#include <Adafruit_TinyUSB.h>

#include "Devices/Platform/BoardSupport.h"
#include "Devices/Button/HardwareButton.h"
#include "Devices/LED/HardwareLED.h"
#include "Devices/TFT/HardwareTFT.h"
#include "Devices/Storage/HardwareStorage.h"
#include "Devices/USB/USBCore.h"
#include "Devices/WiFi/HardwareWiFi.h"
#include "Devices/Microphone/HardwareMicrophone.h"
#include "Devices/IR/HardwareIR.h"
#include "Devices/Touch/HardwareTouch.h"

#include "Attacks/Marauder/Marauder.h"
#include "Attacks/Ducky/DuckyPayload.h"
#include "Attacks/Ghost/SilentSentinel.h"
#include "Attacks/Neo/EtherHarvest.h"
#include "Attacks/Trinity/HydraHID.h"
#include "Attacks/Trinity/EapHarvester.h"
#include "Attacks/Shadow/ShadowVolume.h"
#include "Attacks/Agent/Agent.h"

#ifdef ENABLE_RED_TEAM_COVERT_BLE
#include "Attacks/Red/BLETerminal.h"
#endif

#include "Attacks/Red/CaptivePortal.h"

#ifdef ENABLE_BLUE_TEAM_TELEMETRY
#include "Attacks/Blue/Telemetry.h"
#include "Attacks/Blue/NetworkScanner.h"
#endif

#include "Debug/Logging.h"
#include "AuxiliaryComponent/Auxiliary.h"

#include "Utilities/Format.h"
#include "version.h"

#define TAG "main"

static Preferences prefs;
static Auxiliary aux;
static BoardSupport board;
static unsigned long lastHeapCheck = 0;
static const uint32_t LOW_HEAP_THRESHOLD = 20 * 1024; // 20KB

#ifdef ARDUINO_ARCH_ESP32
void networkTask(void *pvParameters) {
  while (true) {
    Devices::WiFi.loop(prefs);
    
#ifdef ENABLE_RED_TEAM_COVERT_BLE
    Attacks::Red::BLE.loop(prefs);
#endif

    Attacks::Red::Portal.loop(prefs);

#ifdef ENABLE_BLUE_TEAM_TELEMETRY
    Attacks::Blue::Logger.loop(prefs);
    Attacks::Blue::Scanner.loop(prefs);
#endif

    vTaskDelay(pdMS_TO_TICKS(10)); // Yield to network watchdog
  }
}
#endif

void setup()
{
  prefs.begin("usbarmyknife");

  // First set up our core components / hw
  Debug::Log.begin(prefs);

  // set up underlying platform hardware
  board.begin(prefs);

  Devices::Storage.begin(prefs);
  // ESP32 Marauder uses a BT library that gets stuck in an infinite loop if it
  // fails to init. We init Marauder early as this means we should have as few tasks
  // as possible up in the air
  // If you plug in and don't see any LEDs, try commenting this line out
  Attacks::Marauder.begin(prefs);

  Devices::TFT.begin(prefs);
  Devices::LED.begin(prefs);
  Devices::Button.begin(prefs);
  Devices::Mic.begin(prefs);
  
  Devices::USB::Core.begin(prefs);
  Devices::WiFi.begin(prefs);
  Devices::IR.begin(prefs);
  Devices::Touch.begin(prefs);

  Attacks::Ducky.begin(prefs);
  Attacks::Ghost.begin(prefs);
  Attacks::Neo.begin(prefs);
  Attacks::Trinity.begin(prefs);
  Attacks::Eap.begin(prefs);
  Attacks::GhostStorage.begin(prefs);
  Attacks::Agent.begin(prefs);

#ifdef ENABLE_RED_TEAM_COVERT_BLE
  Attacks::Red::BLE.begin(prefs);
#endif

  Attacks::Red::Portal.begin(prefs);

#ifdef ENABLE_BLUE_TEAM_TELEMETRY
  Attacks::Blue::Logger.begin(prefs);
  Attacks::Blue::Scanner.begin(prefs);
#endif

  if (!Devices::Storage.isRunning())
  {
#ifndef NO_SD
    AskFormatSD(prefs);
#endif
  }
  else if (!Attacks::Marauder.isRunning())
  {
    // Most users won't see this error as devices without an SD won't have a screen either
    Devices::TFT.display(0, 0, "Error FlashFS invalid");
    for (int x = 0; x < 5; x++)
    {
      // They might see/report some debug output
      Devices::LED.changeLEDState(true, 0, 100, 100, x % 2 == 0 ? 255 : 0); // flash RED led
      Debug::Log.error(TAG, "Flash filesystem is invalid, upload new FS image");
      delay(1000);
    }
#ifdef ARDUINO_ARCH_ESP32
    // Other platforms don't implement ESP32 Marauder so we don't have to worry about a pi
    ESP.restart();
#endif
  }

  Devices::TFT.display(0, 0, "Device now running");
  Debug::Log.info(TAG, "Running!");

  if (Devices::USB::Core.currentDeviceType() == USBDeviceType::Serial)
  {
    Devices::TFT.display(0, 8, "USB MODE: Serial");
  }
  else if (Devices::USB::Core.currentDeviceType() == USBDeviceType::NCM)
  {
    Devices::TFT.display(0, 8, "USB MODE: NCM");
  }
  else
  {
    Devices::TFT.display(0, 8, "USB MODE: Disabled");
  }

  if (Devices::USB::Core.currentClassType() == USBClassType::HID)
  {
    Devices::TFT.display(0, 8+8, "USB CLASS: HID");
  }
  else if (Devices::USB::Core.currentClassType() == USBClassType::Storage)
  {
    Devices::TFT.display(0, 8+8, "USB CLASS: Storage");
  }
  else
  {
    Devices::TFT.display(0, 8+8, "USB CLASS: None");
  }

  Debug::Log.info(TAG, DEVICE_MAKE_MODEL);

  auto versionStr = std::string("Version: ")+GIT_COMMIT_HASH;
  Devices::TFT.display(0, 8+8+8, versionStr);
  Debug::Log.info(TAG, versionStr);

  aux.begin(prefs);

#ifdef ARDUINO_ARCH_ESP32
  xTaskCreatePinnedToCore(
    networkTask,      /* Task function. */
    "NetworkTask",    /* name of task. */
    8192,             /* Stack size of task */
    NULL,             /* parameter of the task */
    1,                /* priority of the task */
    NULL,             /* Task handle to keep track of created task */
    0);               /* pin task to core 0 */
#endif
}

void loop()
{
  uptime::calculateUptime();
  board.loop(prefs);

  Debug::Log.loop(prefs);

  Devices::USB::Core.loop(prefs);
#ifndef ARDUINO_ARCH_ESP32
  Devices::WiFi.loop(prefs);
#endif
  Devices::Storage.loop(prefs);
  Devices::Button.loop(prefs);
  Devices::LED.loop(prefs);
  Devices::TFT.loop(prefs);
  Devices::Mic.loop(prefs);
  Devices::IR.loop(prefs);
  Devices::Touch.loop(prefs);

  Attacks::Ducky.loop(prefs);
  Attacks::Marauder.loop(prefs);
  Attacks::Ghost.loop(prefs);
  Attacks::Neo.loop(prefs);
  Attacks::Trinity.loop(prefs);
  Attacks::Eap.loop(prefs);
  Attacks::GhostStorage.loop(prefs);
  Attacks::Agent.loop(prefs);

#ifndef ARDUINO_ARCH_ESP32
#ifdef ENABLE_RED_TEAM_COVERT_BLE
  Attacks::Red::BLE.loop(prefs);
#endif

#ifdef ENABLE_BLUE_TEAM_TELEMETRY
  Attacks::Blue::Logger.loop(prefs);
#endif
#endif

  aux.loop(prefs);
  
  // Robustness: Heap Monitoring
  if (millis() - lastHeapCheck > 60000) // Check every minute
  {
     lastHeapCheck = millis();
     uint32_t freeHeap = ESP.getFreeHeap();
     if (freeHeap < LOW_HEAP_THRESHOLD)
     {
        Debug::Log.warning(TAG, "Low Heap Warning: " + std::to_string(freeHeap) + " bytes free");
     }
  }
}

#endif /* ARDUINO_USB_MODE */