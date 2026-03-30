# bleTones

A generative audio plugin that turns nearby Bluetooth device signal strengths
into music. Each BLE device becomes a persistent voice with a consistent pitch
(derived from its name). **Movement** — changes in signal strength — triggers
notes: more movement produces louder, richer chords, while proximity determines
how many notes play and how widely they spread across octaves. Works as a
**VST3 / AU plugin inside any DAW** or as a **standalone desktop app**.

> **🆕 First time testing?** See [TESTING_GUIDE.md](TESTING_GUIDE.md) for step-by-step
> instructions on running bleTones and troubleshooting common issues. **No developer
> registration is needed!**

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
│  • Own Info.plist     │                    │  • Detects RSSI movement │
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

The build system is **CMake** — it works in the terminal and can generate projects for any IDE (Xcode, Visual Studio, CLion, etc.).

---

### CMake

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

**Tip:** Double-click a device name in the BLE Scanner panel to give it a
custom alias. Aliases are saved with the plugin state.

---

## Plugin Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Volume | 0 – 1 | 0.7 | Master output level |
| BLE Sensitivity | 0 – 1 | 0.5 | Movement detection threshold: low = only large RSSI changes trigger notes, high = small changes trigger notes |
| Root Note | 0 – 127 | 60 (C4) | MIDI root note for scale mapping |
| Scale / Mode | 10 choices | Minor Pentatonic | Musical scale for pitch mapping |
| Ensemble | 6 choices | Choral | Curated instrument combinations |
| Attack Time | 0.01 – 2.0s | 0.4s | How fast notes fade in |
| Release Time | 0.5 – 12.0s | 6.0s | How long notes ring out |
| Halloween Mode | on/off | off | Secret feature: replaces instruments with 4 spooky timbres and applies Halloween effects. See below. |

### Sound Design: Ensembles

bleTones features a **curated ensemble system** where each selection plays multiple complementary instruments together, creating rich, non-fatiguing textures. Each ensemble was designed based on instruments known to work well together musically.

| Ensemble | Instruments | Character |
|----------|-------------|-----------|
| **Choral** | Choir Pad + Celtic Harp + Soft String | Thick layered chorus with plucked accents |
| **Airy** | Native Flute + Low Drone + Wind Pad | Breathy, floating, spacious |
| **Shimmer** | Clean Guitar + Warm Keys + Shimmer Chorus | Bright, sparkly, detuned textures |
| **Bells** | Vibraphone + Marimba + Glockenspiel | Struck metallic and wooden tones |
| **Strings** | Soft String + Harp + Vibraphone | Sustained bowed and plucked blend |
| **Drone** | Choir Pad + Low Drone | Deep sustained tones |

Each of the **12 distinct instruments** has its own unique:
- **Oscillator recipe**: Specific waveform combinations (sine, triangle, sawtooth)
- **Harmonic ratios**: Including inharmonic partials for bells (2.76, 5.40, 8.93)
- **Envelope characteristics**: From quick plucks (3ms attack) to slow pads (400ms+)
- **Filter settings**: Dark/warm to bright/shimmery
- **Modulation**: Vibrato for flute, tremolo for vibraphone

#### 🎃 Halloween Mode

Activated secretly by **clicking the plugin title 5 times within 3 seconds**. When active:
- All 12 instruments are replaced by 4 spooky timbres cycling across voices:
  1. **Theremin Wail** – pure sine with wide, slow vibrato and ghostly pulsation
  2. **Haunted Organ** – dissonant sawtooth pipe organ with tritone stops and heavy sub-bass
  3. **Ghost Moan** – slow attack filtered noise pad, ethereal and unsettling
  4. **Death Bell** – inharmonic metallic clang with long decaying shimmer
- Reverb becomes large and cavernous (room size 0.92, wet 50%)
- Note decay is extended 2.5× for more sustained, spectral sounds
- UI theme switches to pumpkin orange and spooky purple

> **Tip:** For the full effect, also set Scale / Mode to **Halloween Spooky** (diminished/tritone intervals).

### How BLE Signal Maps to Sound

bleTones uses **movement-based triggering**: sound is generated when a device's
RSSI changes (i.e. it moves closer or further away), not continuously. Each
device is hashed to a **consistent scale degree** so it always plays the same
pitch regardless of signal strength.

| Condition | Effect |
|-----------|--------|
| RSSI changes (movement) | Triggers notes; larger change = higher amplitude (capped at 0.8) |
| Very close (RSSI ≥ −48 dBm) | 4 chord notes; notes spread ±1 octave |
| Close (RSSI −48 to −62 dBm) | 3 chord notes; notes spread upward 1 octave |
| Medium (RSSI −62 to −79 dBm) | 2 chord notes; no octave spread |
| Far (RSSI < −79 dBm) | 1 note; no octave spread |
| Idle for 3 seconds | Soft heartbeat note plays (60% amplitude) |
| Not seen for 10 seconds | Device removed from display |

Every chord note triggers voices for **all instruments in the active ensemble**
simultaneously (2–3 instruments layered), creating rich polyphonic textures.
Up to **32 simultaneous voices** are rendered across all devices and ensembles.

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

