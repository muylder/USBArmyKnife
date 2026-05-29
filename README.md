<a href="https://github.com/i-am-shodan/USBArmyKnife/blob/master/LICENSE"><img alt="License" src="https://img.shields.io/github/license/mashape/apistatus.svg"></a>
[![PlatformIO CI](https://github.com/i-am-shodan/USBArmyKnife/actions/workflows/main.yml/badge.svg)](https://github.com/i-am-shodan/USBArmyKnife/actions/workflows/main.yml)
[![.NET](https://github.com/i-am-shodan/USBArmyKnife/actions/workflows/dotnet.yml/badge.svg)](https://github.com/i-am-shodan/USBArmyKnife/actions/workflows/dotnet.yml)

# USBArmyKnife 🔪 (Next-Gen Version)

<div align="center">
  <h3>The Ultimate Cyber-Swiss-Army Knife for Red & Blue Teams</h3>
  <img src="./docs/images/t-dongle-s3-side.png" width="400px" alt="USB Army Knife Device">
</div>

---

## 🧐 What is the USBArmyKnife?

The USBArmyKnife is a compact, highly versatile hardware implant and penetration testing platform. Built on the ESP32-S3, it empowers you to execute sophisticated Red Team operations and Blue Team auditing that typically require multiple discrete devices. 

By leveraging its dual-core architecture, it simultaneously handles raw USB emulation (HID, Mass Storage, NCM Ethernet) while running a full suite of network exploits, scripting engines, and wireless attacks in the background.

---

## ⚡ Core Functionalities & New Features

### 🔴 Red Team Suite (Offensive)
- **AutoPwn Scripting Engine**: A lightweight embedded engine that reads `/autopwn.txt` from the SD card to automate multi-stage attack workflows (inject keystrokes, wait, launch network attacks) immediately on boot.
- **On-Device LLMNR Poisoner (Responder)**: Emulates `Responder` directly on the dongle's NCM Ethernet interface. Spoofs LLMNR queries and captures NetNTLMv2 hashes over SMB (port 445) to the SD card for offline cracking.
- **Covert HID Exfiltration**: Steals data from heavily locked-down, air-gapped machines by monitoring the target OS's Keyboard LED signals (Caps Lock / Scroll Lock toggles).
- **DNS Captive Portal**: Hosts a fake Wi-Fi portal (e.g., "Free Airport WiFi") that harvests user credentials and saves them to the SD card.
- **Covert BLE C2 Terminal**: Exposes a hidden Bluetooth Low Energy UART terminal for triggering DuckyScript payloads via a smartphone without touching the host machine.
- **ESP32 Marauder**: Full integration for Wi-Fi/Bluetooth deauths, PMKID capturing, and wardriving.

### 🔵 Blue Team Suite (Auditing)
- **Real-Time Telemetry Dashboard**: A standalone Node.js dashboard (`c2_dashboard`) that listens to UDP telemetry from the dongle. Visualizes intercepted keystrokes, honeypot alerts, and network captures in real-time.
- **USB Honeypot Monitoring**: Audits unauthorized data access by triggering high-priority alerts whenever specific decoy files on the emulated mass storage are read or modified.
- **NCM Port Scanner**: Automatically audits the target host's network defenses by scanning common TCP ports (22, 80, 443, 445) over the USB Ethernet interface.

---

## 🖥️ Supported Hardware

| Device | Recommended Use | Features |
|:---|:---|:---|
| **LilyGo T-Dongle S3** | **Primary / Best Experience** | LCD, Button, MicroSD slot, WIFI/BT |
| **Evil Crow Cable Wind** | Covert Physical Implants | Stealthy USB cable form-factor |
| **M5Stack AtomS3U** | Compact Operations | Button, WIFI/BT |
| **T-Watch S3** | Wearable Auditing | Screen, Watch form-factor |

---

## 🚀 Installation & Flashing Guide

To compile and flash the USBArmyKnife to your hardware, you will need **PlatformIO**.

### Prerequisites
1. Install [Visual Studio Code](https://code.visualstudio.com/).
2. Install the **PlatformIO IDE** extension in VS Code.
3. Clone this repository: `git clone https://github.com/muylder/USBArmyKnife.git`

### Flashing Steps
1. Open the cloned folder in VS Code.
2. Insert your ESP32-S3 device (e.g., LILYGO T-Dongle S3) into your computer's USB port.
3. Open the PlatformIO Core CLI (or use the VS Code terminal) and run the following command to build and flash:
   ```bash
   pio run -e LILYGO-T-Dongle-S3 -t upload
   ```
   *(Change the `-e` environment to match your specific board if you are not using the T-Dongle S3).*
4. Once flashed, safely eject and re-insert the device to reboot it into USBArmyKnife mode.

---

## 📖 Detailed How-To Use Guide

### 1. Initial Setup & Connection
Before using the USBArmyKnife, ensure it has been flashed with the latest firmware via PlatformIO (using `pio run -e LILYGO-T-Dongle-S3 -t upload` for the recommended T-Dongle S3).
1. **Plug the device** into a USB port on the host machine.
2. **Connect to its WiFi Access Point**:
   - **SSID**: `iPhone14`
   - **Password**: `password`
3. **Access the Web Interface**: Once connected, open a browser and navigate to `http://4.3.2.1:8080`.

### 2. Using the Web Interface
The web interface is your command center for configuring and triggering payloads:
- **Status Dashboard**: Check the uptime, storage usage (SD card or Internal Flash), and the status of any currently running attacks.
- **File Manager**: Upload, download, edit, or delete DuckyScript payloads directly on the device's storage.
- **Payload Execution**: Browse your scripts and run them on-demand directly from the browser.
- **Marauder Control**: Access the WiFi attack subsystem to perform wardriving or deauth attacks.
- **Agent Output**: View results from commands sent to a compromised host via the Agent C2 channel.

### 3. DuckyScript & AutoPwn Execution
The device can run payloads automatically upon insertion or manually via the Web UI or Bluetooth.

**AutoPwn (Plug-and-Play Attacks)**
To run a script automatically the moment the USB is inserted:
1. Create a script named `autopwn.txt` on the root of your MicroSD card.
2. Insert the card into the USBArmyKnife.
3. Plug the device into the target. The script will execute immediately.

**Covert BLE C2 Terminal**
Trigger payloads without touching the target machine:
1. Download a BLE UART app (e.g., nRF Connect) on your smartphone.
2. Connect to `USBArmyKnife_C2` via Bluetooth.
3. Open the UART terminal and type commands such as `exec my_payload.txt`.

### 4. Advanced HID & Automation Features
The USBArmyKnife supports standard DuckyScript along with powerful custom extensions for advanced stealth and interaction.

**Important Custom Commands:**
- `RAW_HID [0xXX] [modifier]`: Send precise raw HID keyboard codes.
- `MOUSE_MOVE [x] [y]`: Manipulate the mouse cursor.
- `MOUSE_JIGGLE`: Prevent the host machine from sleeping.
- `CALC`: Instantly launch the system calculator.
- `KEYBOARD_LAYOUT [layout]`: Switch keyboard layouts dynamically to match the target OS.

**Stealth & Triggers:**
- `RANDOM_DELAY [min] [max]`: Introduces random wait times to evade behavioral analysis.
- `WAIT_FOR_CAPS_CHANGE`, `WAIT_FOR_CAPS_ON`, `WAIT_FOR_CAPS_OFF`: Wait for the target to toggle Caps Lock before proceeding.
- Similar triggers are available for Num Lock (`WAIT_FOR_NUM_CHANGE`) and Scroll Lock (`WAIT_FOR_SCROLL_CHANGE`).

**Mass Storage (MSC):**
- `USB_MOUNT_DISK_READ_ONLY [image.img]`: Mount a virtual disk image as a USB Drive.
- `USB_MOUNT_CDROM_READ_ONLY [image.iso]`: Mount an ISO file to impersonate a CD-ROM.
- `WAIT_FOR_USB_STORAGE_ACTIVITY`: Pause script execution until the host OS starts reading from the USB drive.

### 5. Network & Captive Portal Attacks

**LLMNR Poisoner (Responder)**
Capture NetNTLMv2 hashes for offline cracking:
1. Ensure the USBArmyKnife is configured in NCM Ethernet mode.
2. The onboard Responder module runs automatically. When a Windows target attempts to resolve a non-existent local hostname, the dongle replies and forces SMB authentication.
3. Hashes are saved to `smb_capture.bin` on the SD card.

**DNS Captive Portal**
Steal credentials via fake Wi-Fi:
- The device can host a rogue Wi-Fi network with a Captive Portal.
- Users connecting will be prompted to enter credentials which are harvested and saved to the SD card.

**Blue Team Auditing**
- Launch the real-time telemetry dashboard on your auditing machine:
  ```bash
  cd c2_dashboard
  npm install
  node server.js
  ```
- Open `http://localhost:3002` to monitor intercepted keystrokes, honeypot access logs, and port scanning results.

### 6. WiFi Attacks (ESP32 Marauder)
The integrated ESP32 Marauder engine allows for robust wireless attacks. You can control this from the Web Interface or trigger it via DuckyScript:
- `ESP32_MARAUDER [command]`: Pass native Marauder commands directly.
- `WIFI_ON` / `WIFI_OFF`: Toggle the device's management Access Point.
- Use the Marauder engine to sniff beacons, capture PMKIDs, or execute Deauthentication attacks.

### 7. Agent C2 Interaction
The device can establish a covert Command & Control channel over the USB Serial interface:
- `AGENT_RUN [command]`: Send a background shell command to the victim machine.
- `WAIT_FOR_AGENT_RUN_RESULT`: Pause payload execution until the output from the victim machine is received.
- Use the Web Interface's **Agent Interaction** tab to view the terminal output of compromised hosts.

---

## 🤝 Contributing & Support

Forked from **https://github.com/i-am-shodan/USBArmyKnife**
and **https://github.com/bob-reis/USBArmyKnife_PP_Tech_Version**

## 📄 License

This project is licensed under the MIT License - see the **[LICENSE](LICENSE)** file for details.
