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

### macOS

| Tool | Version | Notes |
|------|---------|-------|
| Xcode | 14+ | Install from the [Mac App Store](https://apps.apple.com/app/xcode/id497799835). Includes the compiler, SDK, and AU support. |
| Xcode Command Line Tools | (included with Xcode) | Or install standalone: `xcode-select --install` |
| CMake | 3.22+ | `brew install cmake` or [cmake.org](https://cmake.org/download/) |
| Git | any | Included with Xcode CLT, or `brew install git` |

> **Note:** Full Xcode (not just Command Line Tools) is required to build AU
> plugins and to use the Xcode IDE workflow described in Option A.

### Windows

| Tool | Version | Notes |
|------|---------|-------|
| Visual Studio | 2022 (or 2019) | Include the **Desktop development with C++** workload |
| CMake | 3.22+ | Bundled with Visual Studio, or [cmake.org](https://cmake.org/download/) |
| Git | any | [git-scm.com](https://git-scm.com/download/win) |
| Windows SDK | 10.0.19041+ | Included with the Visual Studio C++ workload |

> **JUCE is fetched automatically** by CMake's `FetchContent` — no manual
> download or submodule initialisation required.

---

## Building

There are two supported workflows: **CMake** (recommended, works in the terminal
and generates projects for any IDE) and **Projucer** (Xcode-only, requires a
manual JUCE installation). Most users should use CMake.

---

### Option A — CMake (recommended)

#### 1 — Clone the repository

```bash
git clone https://github.com/chrisfromthepast/bleTones.git
cd bleTones
```

#### 2 — Configure

The first run downloads JUCE (~400 MB) and may take **2–5 minutes** depending
on your internet connection. Subsequent runs are instant.

**macOS / Linux (Makefile or Ninja build — terminal only):**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

**macOS — generate an Xcode project (open in Xcode IDE):**
```bash
cmake -B build -G Xcode
```
Then open `build/bleTones.xcodeproj` in Xcode, select the **bleTones -
Standalone** or **bleTones - AU** scheme, and press **⌘B** to build.

**Windows — generate a Visual Studio solution:**
```bat
cmake -B build -G "Visual Studio 17 2022"
```
Then open `build\bleTones.sln` in Visual Studio and build the `Release`
configuration.

#### 3 — Build from the terminal

```bash
cmake --build build --config Release
```

> **Tip (macOS):** Pass `-j$(sysctl -n hw.logicalcpu)` to use all CPU cores:
> ```bash
> cmake --build build --config Release -j$(sysctl -n hw.logicalcpu)
> ```

Built artefacts:

| Artefact | Location |
|----------|----------|
| VST3 plugin | `build/bleTones_artefacts/Release/VST3/bleTones.vst3` |
| AU plugin (macOS) | `build/bleTones_artefacts/Release/AU/bleTones.component` |
| Standalone app (macOS) | `build/bleTones_artefacts/Release/Standalone/bleTones.app` |
| Standalone app (Windows) | `build/bleTones_artefacts/Release/Standalone/bleTones.exe` |
| BLE Helper (macOS) | `build/helper/bleTones_helper.app` |
| BLE Helper (Windows) | `build/helper/Release/bleTones_helper.exe` |

#### 4 — Install the plugin

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

After copying an AU, reset the Audio Unit cache so your DAW picks it up:
```bash
killall -9 AudioComponentRegistrar 2>/dev/null; auval -a 2>/dev/null | grep -i bletones
```

**Windows VST3:**
```bat
xcopy /E /I "build\bleTones_artefacts\Release\VST3\bleTones.vst3" ^
      "C:\Program Files\Common Files\VST3\bleTones.vst3"
```

---

### Option B — Projucer / Xcode (macOS only)

The file `bleTones/bleTones.jucer` is a [Projucer](https://juce.com/discover/projucer)
project. Use this only if you already have JUCE installed and prefer working
entirely inside Xcode without CMake.

**Requirements:**
- JUCE 8.0.0+ downloaded and placed at `../JUCE` relative to the repository root
  (i.e. the `JUCE` folder must be a sibling of the `bleTones` repository folder).
- Projucer (ships with JUCE) — open it from `JUCE/extras/Projucer/Builds/MacOSX/build/Release/Projucer.app`.

**Steps:**
1. Open `bleTones/bleTones.jucer` in Projucer.
2. Click **Save and Open in IDE** (⌘S) — this generates the Xcode project under
   `bleTones/Builds/MacOSX/`.
3. Build and run from Xcode.

> **Note:** The Projucer workflow does **not** build the BLE helper. Use the
> CMake workflow if you need the helper process.

---

### Troubleshooting

| Symptom | Fix |
|---------|-----|
| `cmake: command not found` | Install CMake: `brew install cmake` (macOS) or download from cmake.org |
| `xcode-select: error: tool 'xcodebuild' requires Xcode` | Install full Xcode from the Mac App Store (not just CLT) |
| CMake configure hangs for several minutes | Normal — JUCE is being downloaded (~400 MB). Wait it out. |
| `Error: No CMAKE_CXX_COMPILER` (macOS) | Run `sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer` |
| AU plugin not found in DAW after install | Run `killall -9 AudioComponentRegistrar` and rescan in your DAW |
| macOS says "bleTones.app is damaged or incomplete" | The app was built without a code signing identity. Right-click the `.app` and choose **Open** once to bypass Gatekeeper, or run `xattr -cr bleTones.app` in Terminal to strip the quarantine flag. |
| Bluetooth permission denied on first launch | macOS will show a system dialog — click **Allow**. If missed, go to **System Settings → Privacy & Security → Bluetooth** and enable bleTones Helper. |

---

## Running

### macOS Standalone (single click)

When you launch the **Standalone** app (`bleTones.app`) on macOS, it
automatically starts the embedded BLE helper in the background — no manual
step required. The helper is bundled inside the app at:

```
bleTones.app/Contents/Resources/bleTones_helper.app
```

The helper runs silently (no Dock icon). macOS will prompt for Bluetooth
permission on the **first launch** — click **Allow**.

### DAW / plugin usage (VST3 / AU)

The helper must be started manually before opening the plugin in your DAW.

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

### Loading in your DAW (VST3 / AU)

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

