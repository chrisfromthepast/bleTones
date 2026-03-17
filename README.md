# bleTones

A generative sound experience that creates beautiful, organic music through interaction with Bluetooth devices or mouse/touch input.

## 🚀 Quick Start

### Desktop App (Recommended)

**Best for:** Full Bluetooth scanning and best performance

**One-click start:**
- **Mac/Linux**: Double-click `start.sh` or run `./start.sh`  
- **Windows**: Double-click `start.bat`
- **Manual**: Run `npm install` then `npm start`

**Features:**
- ✅ Full native BLE scanning (no browser restrictions)
- ✅ Automatically detects all nearby Bluetooth devices
- ✅ Better performance and reliability

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
- 🎨 **4 Sound Flavors** - Melodic, Ambient, Percussive, Ethereal (⚙️ Settings menu)
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
python bridge.py
```

Then open one of the SuperCollider flavor files in SuperCollider and evaluate it:

- `woods.scd` - Melodic flavor
- `flavor-ambient.scd` - Ambient with long sustain
- `flavor-percussive.scd` - Sharp, rhythmic sounds
- `flavor-ethereal.scd` - High, dreamy tones
- `sines.scd`, `songly.scd`, `sprites.scd` - Additional experimental presets

**How it works:**
1. `bridge.py` scans for BLE devices and normalizes RSSI values
2. Sends OSC messages to SuperCollider on port 57120
3. SuperCollider `.scd` files generate sounds based on BLE signal data

---

## 📝 Summary

**For most users:** Use the **desktop app** (start.sh/npm start) for the best experience with full BLE support.

**For quick testing:** Open **index.html** in your browser and try Demo Mode.

**For advanced users:** Use **Python + SuperCollider** for custom OSC integration and audio synthesis.
