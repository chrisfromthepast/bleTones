# bleTones

A generative audio plugin that turns nearby Bluetooth device signal strengths
into music. Each BLE device becomes a voice: the closer it is, the louder and
higher its tone. Works as a **VST3 / AU plugin inside any DAW** or as a
**standalone desktop app**.

---

## Architecture

bleTones uses a two-process design to work around macOS CoreBluetooth
permission restrictions on DAW hosts:

```
┌───────────────────────┐     OSC / UDP      ┌──────────────────────────┐
│   bleTones Helper     │ ───────────────▶   │  bleTones VST3/AU Plugin │
│  (standalone app)     │   localhost:9000    │  (inside DAW or standalone)│
│                       │                    │                          │
│  • Scans BLE devices  │                    │  • Receives OSC          │
│  • Own Info.plist     │                    │  • Maps RSSI → frequency │
│  • Full BLE access    │                    │  • Renders audio         │
└───────────────────────┘                    └──────────────────────────┘
```

The **helper** runs as a background app with its own `Info.plist` (macOS
Bluetooth permission). The **plugin** never touches Bluetooth directly, so the
host DAW's Info.plist is irrelevant.

---

## Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| CMake | 3.22+ | [cmake.org](https://cmake.org/download/) |
| C++ compiler | C++17 | Xcode / MSVC / Clang |
| Git | any | needed by CMake FetchContent |
| macOS SDK | 12+ | for AU and CoreBluetooth helper |
| Windows SDK | 10.0.19041+ | for WinRT BLE helper |

> **JUCE is fetched automatically** by CMake's `FetchContent` — no manual
> download or submodule initialisation required.

---

## Building

### 1 — Clone & configure

```bash
git clone https://github.com/chrisfromthepast/bleTones.git
cd bleTones

# Configure (downloads JUCE on first run — takes ~2 minutes)
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

### 2 — Build plugin + helper

```bash
cmake --build build --config Release
```

Built artefacts:

| Artefact | Location |
|----------|----------|
| VST3 plugin | `build/bleTones_artefacts/Release/VST3/bleTones.vst3` |
| AU plugin (macOS) | `build/bleTones_artefacts/Release/AU/bleTones.component` |
| Standalone app | `build/bleTones_artefacts/Release/Standalone/bleTones` |
| BLE Helper (macOS) | `build/helper/bleTones_helper.app` |
| BLE Helper (Windows) | `build/helper/Release/bleTones_helper.exe` |

### 3 — Install the plugin

**macOS VST3:**
```bash
cp -r "build/bleTones_artefacts/Release/VST3/bleTones.vst3" \
      ~/Library/Audio/Plug-Ins/VST3/
```

**macOS AU:**
```bash
cp -r "build/bleTones_artefacts/Release/AU/bleTones.component" \
      ~/Library/Audio/Plug-Ins/Components/
```

**Windows VST3:**
```
copy build\bleTones_artefacts\Release\VST3\bleTones.vst3
     C:\Program Files\Common Files\VST3\
```

---

## Running

### Step 1 — Launch the BLE Helper

The helper must be running before you open the plugin.

**macOS:**
```bash
open build/helper/bleTones_helper.app
```
macOS will ask for Bluetooth permission on first launch — click **Allow**.
The helper runs silently (no Dock icon).

**Windows:**
```
build\helper\Release\bleTones_helper.exe
```

### Step 2 — Open the plugin

- Load `bleTones` as a VST3 / AU instrument in your DAW, **or**
- Launch the standalone app directly.

The plugin listens on **UDP port 9000** (localhost). Once the helper is
running and nearby BLE devices are visible, they appear in the plugin UI
within a second or two.

---

## Plugin Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Volume | 0 – 1 | 0.7 | Master output level |
| BLE Sensitivity | 0 – 1 | 0.5 | Scales RSSI → amplitude mapping |
| Root Note | 0 – 127 | 60 (C4) | MIDI root note (reserved for scale mapping) |

### RSSI mapping

| BLE RSSI | Frequency | Amplitude |
|----------|-----------|-----------|
| −30 dBm (very close) | 880 Hz (A5) | 1.0 |
| −65 dBm (medium) | ~440 Hz (A4) | ~0.5 |
| −100 dBm (far / barely visible) | 110 Hz (A2) | 0.1 |

Up to **8 simultaneous voices** are rendered (one per device). Devices not
seen for more than 10 seconds are automatically removed.

---

## OSC Protocol

The plugin accepts messages on **UDP port 9000** matching this pattern:

```
/ble/rssi  <deviceName:string>  <rssi:int32>
```

Any OSC-capable program can feed data to the plugin — you are not limited to
the bundled helper.

---

## Project Structure

```
bleTones/
├── CMakeLists.txt          # Top-level CMake: plugin + helper
├── Source/
│   ├── PluginProcessor.h   # AudioProcessor + OSC receiver
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h      # Plugin UI
│   └── PluginEditor.cpp
└── helper/
    ├── CMakeLists.txt      # Helper build (macOS / Windows)
    ├── Info.plist          # macOS Bluetooth permission
    └── Source/
        ├── OSCSender.h     # Self-contained UDP OSC sender
        ├── main.mm         # macOS CoreBluetooth scanner (Obj-C++)
        └── main_win.cpp    # Windows WinRT BLE scanner
```

---

## License

[MIT](LICENSE)

