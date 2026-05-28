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
3. Clone this repository: `git clone https://github.com/muylder/USBArmyKnife_PP_Tech_Version.git`

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

## 📖 Mini-Tutorials: Using the Tools

### 1. Using the AutoPwn Engine
The AutoPwn engine allows you to script attacks that run the moment the USB is plugged in.
1. Insert your MicroSD card into your computer.
2. Create a file named `autopwn.txt` in the root directory.
3. Write your payload sequence. Example:
   ```text
   WAIT 2000
   INJECT powershell -w hidden -c "echo 'Pwned' > C:\pwn.txt"
   START_RESPONDER
   ```
4. Insert the SD card into the dongle and plug it into the target. The script will execute automatically.

### 2. Launching the Blue Team C2 Dashboard
To view live telemetry, honeypot alerts, and captured credentials:
1. Ensure Node.js is installed on your auditing laptop.
2. Navigate to the `c2_dashboard` folder:
   ```bash
   cd c2_dashboard
   npm install
   node server.js
   ```
3. Open your browser to `http://localhost:3002`.
4. Connect the USBArmyKnife to the machine. As it performs network scans or detects honeypot tampering, the dashboard will light up with real-time logs.

### 3. Activating the LLMNR Poisoner
To capture NetNTLMv2 hashes from a Windows machine:
1. Ensure the dongle is configured to enumerate as a USB Ethernet interface (NCM mode).
2. The Responder module runs automatically in the background. If the target Windows machine tries to resolve a local hostname that doesn't exist (e.g., a typo in a network share), the dongle will intercept it.
3. The captured negotiation packets are saved to `smb_capture.bin` on the SD card for offline cracking with Hashcat.

### 4. Setting up Covert HID Exfiltration
To steal data from an air-gapped machine without a network connection:
1. Plug the USBArmyKnife into the target machine.
2. Run a script on the target machine that reads the sensitive file and translates the binary data into Keyboard LED toggles.
   - Example mechanism: Toggle `Caps Lock` to represent a `1` or `0`, and toggle `Scroll Lock` to signal the "Clock" (telling the dongle to read the bit).
3. The dongle will silently reconstruct the flashing LEDs into binary data and save it directly to `/exfil.bin` on the SD card.

### 5. Using the Covert BLE Terminal
To trigger DuckyScript payloads from across the room:
1. Download a BLE UART app (like nRF Connect or Adafruit Bluefruit LE Connect) on your smartphone.
2. Connect to the Bluetooth device named `USBArmyKnife_C2`.
3. Open the UART terminal and send commands like `exec ducky_script.txt` to trigger physical keystroke injections on the target machine while you are safely out of sight.

---

## 🤝 Contributing & Support

Forked from **https://github.com/i-am-shodan/USBArmyKnife**
and **https://github.com/bob-reis/USBArmyKnife_PP_Tech_Version**

## 📄 License

This project is licensed under the MIT License - see the **[LICENSE](LICENSE)** file for details.
