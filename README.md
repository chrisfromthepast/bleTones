# bleTones

A generative sound experience that creates beautiful, organic music through interaction with Bluetooth devices or mouse/touch input.

> **📦 Python Version Archive:** This repository contains the Python-heavy version (v1.0.0-python). For accessing the archived release, see [PYTHON_ARCHIVE.md](PYTHON_ARCHIVE.md) and [RELEASE_NOTES.md](RELEASE_NOTES.md).

> **⚠️ Mobile Platform Note:** iOS and Android severely restrict background BLE/WiFi scanning. See [PLATFORM_LIMITATIONS.md](PLATFORM_LIMITATIONS.md) for technical details and viable workarounds (ESP32 beacon, desktop app, smart home integration).

## 🚀 Quick Start

### Standalone Python Desktop App (Recommended)

**Best for:** Full Bluetooth scanning, best performance, and single-file distribution

**Requirements:**
- Python 3.8+
- Install dependencies: `pip install eel bleak`

**Run from source:**
```bash
python main.py
```

**Build a single executable (no console window):**

macOS / Linux:
```bash
pyinstaller --onefile --noconsole --name bleTones --add-data "web:web" --osx-bundle-identifier com.chrisfromthepast.bletones main.py
```
On macOS, copy the included `Info.plist` into the built `.app` bundle to enable Bluetooth permissions:
```bash
cp Info.plist dist/bleTones.app/Contents/Info.plist
```
Windows:
```bash
pyinstaller --onefile --noconsole --name bleTones --add-data "web;web" main.py
```
The compiled binary will appear in the `dist/` folder.

**Required directory structure:**
```
bleTones/
├── main.py
└── web/
    └── index.html
```

**Features:**
- ✅ Full native BLE scanning via Python `bleak` (no browser restrictions)
- ✅ Automatically detects all nearby Bluetooth devices
- ✅ Compiles to a single standalone executable
- ✅ Web Audio synthesis rendered in an embedded browser window via `eel`

### Electron Desktop App (Legacy)

**Best for:** Users already set up with Node.js

**One-click start:**
- **Mac/Linux**: Double-click `start.sh` or run `./start.sh`  
- **Windows**: Double-click `start.bat`
- **Manual**: Run `npm install` then `npm start`

### Web Browser

**Best for:** Quick testing without installation

1. Open `index.html` in any modern browser
2. Click **"Begin Experience"** to enable audio
3. Choose your mode:
   - **🖱️ Mouse/Touch**: Create sounds by moving your mouse
   - **📡 BLE Devices**: Use Demo Mode or real Bluetooth devices

**Note:** Real BLE scanning in browsers is limited due to privacy restrictions. Use the desktop app for full BLE support.

---

## 🎵 How to Use

### Mouse/Touch Mode

- Move your mouse or touch the screen to create sounds
- Faster movement = louder sounds
- Vertical position affects pitch (top = high notes, bottom = low notes)
- Choose from 4 instruments: Wood Chimes, Deep Log, Hollow Reed, or Kalimba

### BLE Mode

#### 🎭 Demo Mode
Generates simulated BLE signals for a full experience without real Bluetooth hardware:
1. Switch to "📡 BLE Devices" mode
2. Click **"🎭 Demo Mode"**
3. Enjoy generative music from simulated devices!

#### 🔍 Real BLE Mode
- **Desktop App**: Click "🔍 Real BLE" to scan all nearby Bluetooth devices automatically
- **Web Browser**: Limited support (see Troubleshooting below)

### Settings & Features

- 🎹 **4 Unique Instruments** - Wood Chimes, Deep Log, Hollow Reed, Kalimba
- 🎨 **3 Sound Flavors** - Melodic, Ambient, Ethereal (⚙️ Settings menu)
- ⚙️ **Parameter Control** - Adjust volume, pitch scale, and sensitivity
- 💾 **Persistent Settings** - Preferences saved between sessions
- 🎨 **Real-time Visualization** - Particle effects respond to your movements

---

## 🔧 Troubleshooting

### "Not seeing Bluetooth devices"

**Best Solution:** Use the desktop app!
- Run `npm install` then `npm start`
- Or double-click `start.sh` (Mac/Linux) or `start.bat` (Windows)

**For web browsers:**
- Try **Demo Mode** first to verify audio is working
- Enable Bluetooth in your system settings
- Chrome only: Try enabling `chrome://flags/#enable-experimental-web-platform-features`
- Note: Chrome's Web Bluetooth API is intentionally restricted for privacy

### "App won't start"

- Ensure Node.js is installed from https://nodejs.org/
- Run `npm install` in the project directory
- Check terminal for error messages

---

## 🔬 Advanced: Python + SuperCollider

For the original setup with more control and OSC integration:

### Requirements
- Python 3 with `bleak` and `python-osc` packages
- SuperCollider audio synthesis software

### Setup

```bash
# Install Python dependencies
pip install bleak python-osc

# Run the BLE bridge (scans Bluetooth and sends OSC messages)
python supercollider/bridge.py
```

Then open one of the SuperCollider flavor files (in the `supercollider/` folder) in SuperCollider and evaluate it:

- `supercollider/woods.scd` - Melodic flavor
- `supercollider/flavor-ambient.scd` - Ambient with long sustain
- `supercollider/flavor-percussive.scd` - Sharp, rhythmic sounds
- `supercollider/flavor-ethereal.scd` - High, dreamy tones
- `supercollider/sines.scd`, `supercollider/songly.scd`, `supercollider/sprites.scd` - Additional experimental presets

**How it works:**
1. `supercollider/bridge.py` scans for BLE devices and normalizes RSSI values
2. Sends OSC messages to SuperCollider on port 57120
3. SuperCollider `.scd` files in the `supercollider/` folder generate sounds based on BLE signal data

---

## 📝 Summary

**For most users:** Use the **desktop app** (start.sh/npm start) for the best experience with full BLE support.

**For quick testing:** Open **index.html** in your browser and try Demo Mode.

**For advanced users:** Use **Python + SuperCollider** for custom OSC integration and audio synthesis.
