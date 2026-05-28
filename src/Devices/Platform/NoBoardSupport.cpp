#if !defined(LILYGO_T_WATCH_S3) && !defined(EVIL_CROW_CABLE_WIND)
#include "BoardSupport.h"
#include "../../Devices/USB/USBCore.h"
#include "../../Attacks/Ducky/DuckyPayload.h"
#ifdef ARDUINO_ARCH_ESP32
#include "esp_sleep.h"
#endif

static unsigned long lastActivityTime = 0;
static const unsigned long IDLE_TIMEOUT_MS = 5 * 60 * 1000; // 5 minutes

BoardSupport::BoardSupport()
{
}

void BoardSupport::begin(Preferences &prefs)
{
    lastActivityTime = millis();
}

void BoardSupport::loop(Preferences &prefs)
{
#ifdef ARDUINO_ARCH_ESP32
    bool isIdle = true;
    
    // Check if USB is connected and active
    if (Devices::USB::Core.currentDeviceType() != USBDeviceType::None) {
        isIdle = false;
    }

    if (!isIdle) {
        lastActivityTime = millis();
    } else {
        if (millis() - lastActivityTime > IDLE_TIMEOUT_MS) {
            // Idle for 5 minutes, enter light sleep
            esp_light_sleep_start();
            lastActivityTime = millis(); // Reset after waking up
        }
    }
#endif
}
#endif