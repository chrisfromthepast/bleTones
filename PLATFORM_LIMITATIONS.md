# Platform Limitations: iOS & Mobile Background BLE/WiFi Scanning

## The Problem

bleTones needs continuous BLE RSSI data to drive generative music in real time. On desktop (Python `bleak`, Electron `noble`), this works seamlessly because the OS gives apps full access to the Bluetooth radio. **On iOS and Android, this is heavily restricted.**

## Why a Native iOS/Android App Cannot Do This

### 1. MAC Address Obfuscation

Apple and Google block third-party apps from seeing raw MAC addresses of surrounding BLE and Wi-Fi devices. Starting with iOS 14 and Android 10, the OS randomizes MAC addresses exposed to apps. CoreBluetooth returns opaque `CBPeripheral.identifier` UUIDs that change per-device and per-app — you cannot reliably track a specific person's phone across sessions.

### 2. Background Execution Throttling

iOS aggressively manages background apps to preserve battery life:

- **CoreBluetooth background mode** (`bluetooth-central`) allows scanning in the background, but with severe restrictions:
  - Scan intervals are throttled from ~10 Hz (foreground) to roughly once every ~3–4 minutes
  - `CBCentralManagerScanOptionAllowDuplicatesKey` is **ignored** in background — you receive one discovery event per device, not continuous RSSI updates
  - The system may suspend or kill your app at any time to reclaim resources
- **Wi-Fi scanning** (`CNCopyCurrentNetworkInfo`, `NEHotspotHelper`) requires explicit entitlements that Apple gates behind a manual review process. Passive Wi-Fi RSSI enumeration of nearby APs is not available to third-party apps at all.
- **Background App Refresh** runs at most once every 15–30 minutes at the OS's discretion — far too slow for real-time audio triggers.

### 3. App Store Review Guidelines

Apple's [App Store Review Guidelines §5.1.1](https://developer.apple.com/app-store/review/guidelines/#data-collection-and-storage) require that apps collecting location or device-proximity data must:

- Provide clear, visible user consent at runtime
- Justify why the data is needed
- Not operate as passive surveillance or background network scanners

An app whose primary function is ambient background scanning of nearby BLE devices would face significant App Store review friction.

## What Still Works on iOS

Despite these restrictions, CoreBluetooth is **not useless** — it's just limited:

| Capability | Foreground | Background |
|---|---|---|
| Scan for BLE peripherals | ✅ Full speed, duplicates allowed | ⚠️ Throttled, no duplicates |
| Read RSSI of **connected** peripherals | ✅ `peripheral.readRSSI()` on demand | ✅ Works if connection is maintained |
| Receive data from **connected** peripherals | ✅ Full speed | ✅ Via `notify` characteristics |
| Passive Wi-Fi AP scanning | ❌ Not available to apps | ❌ Not available to apps |

**Key takeaway:** If bleTones connects to a known BLE peripheral (like an ESP32 beacon), it can poll RSSI in the background via `readRSSI()` on an active connection. It **cannot** passively discover and track unknown devices in the background.

## Viable Workarounds

### 1. Dedicated Hardware Beacon (ESP32)

Use the ESP32 as the scanner and push data to the phone:

- ESP32 continuously scans BLE and Wi-Fi with no OS restrictions
- Sends RSSI data to bleTones iOS app via a BLE GATT `notify` characteristic
- iOS app maintains a single BLE connection to the ESP32 (works in background)
- The phone becomes a **display and sound engine**, not a scanner

This is the most realistic path to an iOS app that works in the background.

### 2. Desktop App or VST Plugin

Desktop operating systems (macOS, Windows, Linux) provide unrestricted access to Bluetooth and Wi-Fi radios. The existing Python (`bleak`) and Electron (`noble`) implementations already work:

- **macOS/Windows menu bar app** — ambient monitoring with system-level BLE access
- **VST/AU plugin for DAWs** (Ableton, Logic) — BLE RSSI as a modulation source
- **Target audience:** Streamers, musicians, and developers who want audio cues when someone enters their space

### 3. Smart Home Integration (Home Assistant / MQTT)

Package the ESP32 scanner as a Home Assistant integration:

- ESP32 publishes RSSI data to MQTT
- Home Assistant automation triggers events
- A companion desktop app bridges MQTT → OSC/MIDI for audio synthesis
- Leverages the existing `supercollider/bridge.py` OSC architecture

### 4. Foreground-Only iOS Mode

Accept the foreground limitation and design the iOS app around it:

- Full-speed BLE scanning while the app is open and screen is on
- Use the `audio` background mode to keep the sound engine running
- Accept that scanning pauses when the user switches apps
- Position it as an interactive art/music tool, not a background monitor

## Recommendation

The **ESP32 beacon approach** (Workaround #1) is the strongest path for a shipping iOS product. It sidesteps every OS restriction by moving the scanning to dedicated hardware, while the phone handles synthesis and output. The existing desktop implementations remain the best option for unrestricted, real-time BLE scanning.

## References

- [Apple CoreBluetooth Background Processing](https://developer.apple.com/library/archive/documentation/NetworkingInternetWeb/Conceptual/CoreBluetooth_concepts/CoreBluetoothBackgroundProcessingForIOSApps/PerformingTasksWhileYourAppIsInTheBackground.html)
- [Apple App Store Review Guidelines §5.1.1](https://developer.apple.com/app-store/review/guidelines/#data-collection-and-storage)
- [Android Bluetooth Permissions (API 31+)](https://developer.android.com/develop/connectivity/bluetooth/bt-permissions)
- [CBCentralManagerScanOptionAllowDuplicatesKey behavior in background](https://developer.apple.com/documentation/corebluetooth/cbcentralmanagerscanoptionallowduplicateskey)
