# Platform Limitations & Feasibility Assessment

## Status: Swift iOS App Abandoned

The Swift/SwiftUI iOS rewrite (PR #15) has been abandoned. iOS background BLE scanning restrictions make continuous RSSI-driven audio generation infeasible as a native mobile app. See the iOS analysis below for the full technical breakdown.

**The path forward is a desktop JUCE VST/AU plugin.** See the [JUCE feasibility assessment](#juce-vstau-plugin-feasibility) below.

---

## iOS & Mobile Background BLE/WiFi Scanning

### The Problem

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

The **ESP32 beacon approach** (Workaround #1) remains viable if iOS is revisited in the future. However, given the complexity of the ESP32 workaround and the foreground-only limitation, **the Swift iOS approach has been abandoned in favor of a JUCE desktop plugin.**

---

## JUCE VST/AU Plugin Feasibility

### The Question

Can [JUCE](https://juce.com/) (a C++ audio framework) build a VST/AU plugin that does BLE scanning and drives generative audio inside a DAW?

### Short Answer

**Yes — with a helper process.** JUCE itself has no BLE APIs, but desktop operating systems impose no background scanning restrictions. The recommended architecture splits BLE scanning into a lightweight companion process that feeds data to the plugin.

### How BLE Access Works on Desktop

Unlike iOS, desktop platforms give native code full access to Bluetooth radios:

| Platform | BLE API | Access from Plugin Process | Permission Model |
|---|---|---|---|
| **macOS** | CoreBluetooth (Obj-C++) | ⚠️ Requires Info.plist in host DAW | System dialog on first use |
| **Windows** | WinRT `Windows.Devices.Bluetooth` | ✅ Works directly | System Bluetooth settings only |
| **Linux** | BlueZ via D-Bus | ✅ Works directly | No special permissions needed |

**No throttling, no MAC obfuscation, no App Store review.** Desktop BLE scanning runs at full speed with real MAC addresses.

### The macOS Permission Problem

On macOS 10.15+, any process using CoreBluetooth must have `NSBluetoothAlwaysUsageDescription` in its Info.plist. For a VST/AU plugin, the **host DAW's** Info.plist must contain this key — not the plugin's.

- Ableton Live, Logic Pro, and most DAWs **do not** include Bluetooth permission in their Info.plist
- You cannot modify a signed DAW's Info.plist without breaking code signing
- Calling CoreBluetooth from inside the plugin process will silently fail or crash on macOS

### Recommended Architecture: Plugin + BLE Helper

Split the system into two processes:

```
┌─────────────────────┐     OSC/UDP      ┌──────────────────────┐
│   BLE Helper App    │ ──────────────▶  │   JUCE VST/AU Plugin │
│  (standalone .app)  │   localhost:9000  │   (inside DAW)       │
│                     │                  │                      │
│  • Python bleak     │                  │  • Receives OSC      │
│  • OR native C++    │                  │  • Maps RSSI → audio │
│  • Full BLE access  │                  │  • Generates sound    │
│  • Own Info.plist   │                  │  • MIDI output        │
└─────────────────────┘                  └──────────────────────┘
```

**BLE Helper** (runs as a separate app with its own Bluetooth permission):
- Scans for BLE devices continuously at full speed
- Sends RSSI data over localhost via OSC (UDP port 9000) or MIDI
- Can be the existing `supercollider/bridge.py` with zero changes — it already outputs OSC

**JUCE Plugin** (runs inside the DAW):
- Listens for OSC messages on a UDP socket (JUCE has built-in `juce::OSCSender`/`juce::OSCReceiver`)
- Maps incoming RSSI data to synthesis parameters
- Generates audio via JUCE's `AudioProcessor` pipeline
- Optionally outputs MIDI to other instruments in the DAW

### Why This Architecture Is Good

1. **Already prototyped.** The `supercollider/bridge.py` → SuperCollider `.scd` pipeline is exactly this pattern. The JUCE plugin replaces the SuperCollider component.
2. **No permission hacks.** The helper app has its own Info.plist with `NSBluetoothAlwaysUsageDescription`. The DAW never touches Bluetooth.
3. **Cross-platform.** JUCE compiles VST3/AU/AAX for macOS, Windows, and Linux from one codebase. The helper app uses Python `bleak` which is also cross-platform.
4. **Distribution.** VST plugins are distributed as files (`.vst3`, `.component`), not through an app store. No review process, no restrictions.
5. **Monetizable.** Sell the plugin + helper as a bundle. Desktop audio plugins routinely sell for $10–$50.

### What JUCE Gives You

- **Audio synthesis:** `AudioProcessor` with sample-accurate buffer rendering
- **MIDI output:** Built-in MIDI message generation and routing
- **OSC support:** `juce::OSCReceiver` for incoming BLE data from the helper
- **GUI:** Cross-platform plugin editor UI
- **Plugin formats:** VST3, AU, AAX, standalone — from one codebase
- **Licensing:** Free for projects under $50K revenue, then paid tiers above that

### Comparison: JUCE Plugin vs. Swift iOS App

| | Swift iOS App | JUCE Desktop Plugin |
|---|---|---|
| Background BLE scanning | ❌ Throttled to ~1 scan/4 min | ✅ Full speed via helper |
| MAC address access | ❌ Obfuscated UUIDs | ✅ Real MAC addresses |
| App Store required | ✅ Yes, with review friction | ❌ No, direct distribution |
| Audio synthesis | AVAudioEngine | JUCE AudioProcessor |
| MIDI output | CoreMIDI | JUCE built-in |
| Target audience | General consumers | Musicians, streamers, creatives |
| Monetization ceiling | ~$5 App Store purchase | $10–$50 plugin sale |

### Remaining Questions

1. **Standalone mode:** JUCE also builds standalone desktop apps (not just plugins). Could skip the DAW entirely and build a menu-bar app with built-in BLE — no helper needed.
2. **Helper bundling:** Should the BLE helper be a Python script (reuse `bridge.py`) or a native Swift/C++ app for cleaner distribution?
3. **MIDI vs. OSC:** OSC is more flexible (float RSSI values, device names), but MIDI CC is universally supported by every DAW without a plugin.

## References

- [Apple CoreBluetooth Background Processing](https://developer.apple.com/library/archive/documentation/NetworkingInternetWeb/Conceptual/CoreBluetooth_concepts/CoreBluetoothBackgroundProcessingForIOSApps/PerformingTasksWhileYourAppIsInTheBackground.html)
- [Apple App Store Review Guidelines §5.1.1](https://developer.apple.com/app-store/review/guidelines/#data-collection-and-storage)
- [Android Bluetooth Permissions (API 31+)](https://developer.android.com/develop/connectivity/bluetooth/bt-permissions)
- [CBCentralManagerScanOptionAllowDuplicatesKey behavior in background](https://developer.apple.com/documentation/corebluetooth/cbcentralmanagerscanoptionallowduplicateskey)
- [JUCE Framework](https://juce.com/)
- [JUCE OSC Module](https://docs.juce.com/master/group__juce__osc.html)
- [JUCE AudioProcessor](https://docs.juce.com/master/classAudioProcessor.html)
