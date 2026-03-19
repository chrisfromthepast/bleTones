# bleTones v1.0.0-python Release Notes

**Release Date:** March 19, 2026

This release captures the Python-heavy version of bleTones before a major repository refactor to experiment with a new language implementation.

## 🎯 Overview

bleTones is a generative sound experience that creates beautiful, organic music through interaction with Bluetooth Low Energy (BLE) devices or mouse/touch input. This version features multiple deployment options and comprehensive Python-based BLE scanning capabilities.

## 📦 What's Included

### Python Components

#### 1. **main.py** - Python Desktop App (Recommended)
- Full native BLE scanning using `bleak` library
- Embedded web browser via `eel` for Web Audio synthesis
- No browser restrictions on BLE access
- Can be compiled to single executable with PyInstaller

**Dependencies:**
```bash
pip install eel bleak
```

**Build Instructions:**
- **macOS/Linux:** `pyinstaller --onefile --noconsole --name bleTones --add-data "web:web" --osx-bundle-identifier com.chrisfromthepast.bletones main.py`
- **Windows:** `pyinstaller --onefile --noconsole --name bleTones --add-data "web;web" main.py`
- **macOS:** Copy `Info.plist` to enable Bluetooth permissions

#### 2. **supercollider/bridge.py** - Advanced OSC Bridge
- BLE scanning with RSSI normalization
- Sends OSC messages to SuperCollider on port 57120
- Real-time device detection with signal strength mapping
- Stability filtering to prevent flooding

**Dependencies:**
```bash
pip install bleak python-osc
```

### SuperCollider Sound Synthesis (.scd files)

The `supercollider/` directory contains 7 SuperCollider presets that receive OSC data from bridge.py:

1. **woods.scd** - Melodic flavor with organic wood-like tones
2. **flavor-ambient.scd** - Long sustain, atmospheric sounds
3. **flavor-percussive.scd** - Sharp, rhythmic percussion
4. **flavor-ethereal.scd** - High, dreamy, ethereal tones
5. **sines.scd** - Pure sine wave experiments
6. **songly.scd** - Musical, melodic patterns
7. **sprites.scd** - Playful, sprite-like sounds

### Electron Desktop App

Built with Node.js and Electron for cross-platform desktop deployment:

**Files:**
- `main.js` - Electron main process with BLE scanning via @abandonware/noble
- `preload.js` - Security context bridge
- `package.json` - Node dependencies and build configuration
- `start.sh` / `start.bat` - One-click launcher scripts

**Features:**
- Native BLE scanning on macOS, Windows, Linux
- Automatic dependency installation
- Builds to DMG (macOS), NSIS (Windows), AppImage (Linux)

### Web Audio Interface

**index.html** (114KB) - Complete web-based audio synthesis:
- Web Audio API with custom instrument synthesis
- 4 instrument flavors: Melodic, Ambient, Percussive, Ethereal
- 12 unique instruments across flavors
- Real-time BLE device mapping to musical scales
- Mouse/touch interaction mode
- Demo mode for simulated BLE signals
- Persistent settings via localStorage
- Real-time particle visualization

**Also available as:** `web/index.html` (identical, used by Python app)

## 🎵 Features

### Multiple Deployment Methods
1. **Python + Eel (Recommended)** - Best BLE scanning, single executable
2. **Electron Desktop** - Cross-platform, Node.js based
3. **Web Browser** - No installation, limited BLE support
4. **Python + SuperCollider** - Advanced synthesis, OSC integration

### Audio Capabilities
- 12 unique Web Audio instruments with custom synthesis
- 4 flavors with distinct sonic characteristics
- Adjustable BPM (60-200), volume, key, scale
- Movement sensitivity control (1-20)
- Real-time RSSI-to-pitch mapping for BLE devices

### BLE Integration
- Automatic detection of all nearby Bluetooth devices
- Device-specific pitch assignment (hash-based)
- RSSI strength affects volume/intensity
- Configurable movement threshold
- Demo mode for testing without real BLE devices

### User Experience
- Settings panel with persistent preferences
- Real-time particle effects synchronized to audio
- Mouse/touch mode for interactive sound creation
- Comprehensive documentation (README.md, QUICKSTART.md)

## 📋 System Requirements

### For Python Desktop App
- Python 3.8 or higher
- macOS 10.15+, Windows 10+, or modern Linux
- Bluetooth Low Energy capable hardware
- 100MB free disk space

### For Electron App
- Node.js 14 or higher
- npm package manager
- Same OS/Bluetooth requirements as Python

### For Web Browser
- Modern browser (Chrome, Firefox, Safari, Edge)
- Web Audio API support
- Web Bluetooth API (optional, experimental)

## 🔧 Usage

### Quick Start - Python App
```bash
python main.py
```

### Quick Start - Electron App
```bash
npm install
npm start
```
Or double-click `start.sh` (Mac/Linux) or `start.bat` (Windows)

### Quick Start - SuperCollider Integration
```bash
# Terminal 1: Start BLE bridge
python supercollider/bridge.py

# Terminal 2: Open SuperCollider and evaluate any .scd file
# Choose from woods.scd, flavor-ambient.scd, etc.
```

## 📁 File Structure

```
bleTones/
├── main.py                    # Python desktop app entry point
├── index.html                 # Web Audio interface (standalone)
├── main.js                    # Electron main process
├── preload.js                 # Electron preload script
├── package.json               # Node.js dependencies & build config
├── Info.plist                 # macOS Bluetooth permissions
├── start.sh                   # Mac/Linux launcher
├── start.bat                  # Windows launcher
├── LICENSE                    # MIT License
├── README.md                  # Comprehensive documentation
├── QUICKSTART.md              # Quick start guide
├── web/
│   └── index.html             # Web interface (used by Python app)
└── supercollider/
    ├── bridge.py              # BLE → OSC bridge
    ├── woods.scd              # Melodic preset
    ├── flavor-ambient.scd     # Ambient preset
    ├── flavor-percussive.scd  # Percussive preset
    ├── flavor-ethereal.scd    # Ethereal preset
    ├── sines.scd              # Sine wave experiments
    ├── songly.scd             # Musical patterns
    └── sprites.scd            # Playful sounds
```

## 🎨 Instrument Details

### Melodic Flavor
- Choir Pad (warm, sustained pad)
- Celtic Harp (plucked, resonant strings)
- Soft String (gentle string ensemble)

### Ambient Flavor
- Native Flute (breathy, organic wind)
- Low Drone (deep, sustained bass)
- Wind Pad (atmospheric, evolving texture)

### Percussive Flavor
- Vibraphone (metallic, resonant mallet)
- Marimba (wooden, warm percussion)
- Glockenspiel (bright, bell-like chimes)

### Ethereal Flavor
- Clean Guitar (clear, reverberant guitar)
- Warm Keys (soft, pad-like keyboard)
- Shimmer Chorus (lush, shimmering texture)

## 🔐 Security & Privacy

- No data collection or external network requests
- All audio synthesis happens locally
- BLE scanning only detects device MAC addresses and signal strength
- No personal data or device pairing required
- Settings stored locally in browser localStorage

## 🐛 Known Limitations

1. **Web Browser BLE** - Chrome's Web Bluetooth API is intentionally restricted for privacy. Desktop apps recommended for full BLE support.
2. **macOS Permissions** - Requires NSBluetoothAlwaysUsageDescription in Info.plist for BLE access.
3. **Performance** - Complex scenes with many BLE devices may impact audio timing on lower-end hardware.

## 📜 License

MIT License - See LICENSE file for details

## 👤 Author

chrisfromthepast

## 🔮 Future Plans

This Python-heavy version is being archived before a major refactor to experiment with a new language implementation. All functionality documented here represents the stable, feature-complete Python/JavaScript/Node.js version of bleTones.

## 📞 Support

For issues or questions about this release, please refer to:
- README.md for detailed documentation
- QUICKSTART.md for getting started guide
- GitHub issues for bug reports

---

**Version:** 1.0.0-python  
**Branch:** python-archive  
**Commit:** [Will be tagged at release time]  
**Release Type:** Archive/Snapshot before major refactor
