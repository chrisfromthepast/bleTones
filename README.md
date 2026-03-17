# bleTones

A generative sound experience that creates beautiful, organic music through interaction.

## 🚀 Quick Start Options

### Option 1: Standalone Desktop App (Recommended for Real BLE)

**Best for:** Full Bluetooth scanning without browser restrictions

1. **Install dependencies:**
   ```bash
   npm install
   ```

2. **Run the app:**
   ```bash
   npm start
   ```

The standalone app provides:
- ✅ **Full native BLE scanning** - No browser restrictions or flags needed
- ✅ **One-click experience** - Just run `npm start`
- ✅ **All nearby devices** - Automatically detects all BLE devices in range
- ✅ **Better performance** - Native BLE access via Electron

### Option 2: Web Browser Experience

**Best for:** Quick testing or if you don't want to install anything

**[Try it live in your browser!](index.html)** - No installation required.

1. Open `index.html` in Chrome (or any modern browser)
2. Click **"Begin Experience"** to enable audio
3. Choose your input mode:
   - **🖱️ Mouse/Touch**: Move your mouse or touch the screen to create sounds
   - **📡 BLE Devices**: Use nearby Bluetooth devices to generate sounds

---

## 🎵 How to Use

### Mouse/Touch Mode

- **Move your mouse** or **touch the screen** to create sounds
- The faster you move, the louder the sound
- Position on screen affects pitch (top = high, bottom = low)
- Choose different instruments: Wood Chimes, Deep Log, Hollow Reed, or Kalimba

### BLE Mode

The experience can use **Bluetooth Low Energy (BLE) data** to generate tones!

#### 🎭 Demo Mode (Best for Web Browser)

Demo Mode generates **simulated BLE signals** so you can experience the sound generation without any Bluetooth hardware or setup:

1. Switch to "📡 BLE Devices" mode
2. Click **"🎭 Demo Mode"**
3. Enjoy the generative music from simulated BLE devices!

This is perfect for:
- Testing the experience on any browser
- Using the app without Bluetooth hardware
- Demonstrating the concept to others

#### 🔍 Real BLE Mode (Web Browser)

> ⚠️ **Important**: Chrome's Web Bluetooth API has significant limitations for privacy reasons. **For the best BLE experience, use the standalone desktop app (Option 1 above).**

For actual Bluetooth device scanning in the browser:

**Option A: Passive Scanning (Experimental - May Not Work)**
1. Navigate to `chrome://flags/#enable-experimental-web-platform-features` in Chrome
2. Enable the flag and **restart Chrome completely**
3. Open `index.html` and click "🔍 Real BLE"
4. If you don't see devices, the API may not be working - **use the standalone app instead**

**Option B: Device Pairing (Limited)**
- Without the experimental flag, Chrome requires pairing with a specific device
- Click "🔍 Real BLE" and select a device from the picker
- **Limitation:** Only monitors the one paired device (not a stream of all nearby devices)

**Requirements for Web BLE Mode:**
- Chrome browser (version 79+) with Web Bluetooth support
- HTTPS connection (or localhost) is required for Web Bluetooth
- Bluetooth turned on in your OS

### Features

- 🎹 **4 Unique Instruments** - Each with distinct wooden/organic character
- 🎨 **4 Sound Flavors** - Melodic, Ambient, Percussive, and Ethereal presets
- ⚙️ **Parameter Control** - Adjust volume, pitch, scale, and sensitivity via settings popup
- 💾 **Persistent Settings** - Your preferences are saved between sessions
- 📡 **BLE Integration** - Generate music from Bluetooth device signals
- 🎭 **Demo Mode** - Experience BLE sound generation without Bluetooth hardware
- 🎨 **Real-time Visualization** - Particle effects respond to your movements
- 📱 **Touch Support** - Works great on mobile devices
- 🔊 **Spatial Audio** - Left/right panning based on horizontal position
- 🎼 **Multiple Scales** - Choose from 9 musical scales (pentatonic, diatonic, modal)

### Sound Flavors

The **Settings** popup (⚙️ button in top-left) allows you to switch between 4 distinct sound flavors:

1. **🎵 Melodic** - Balanced, musical tones (C3, minor pentatonic)
   - LogDrum, Claves, Kalimba
   - Moderate sustain, clear articulation

2. **🌌 Ambient** - Spacious, atmospheric sounds (C2, major pentatonic)
   - Pad, Shimmer, Bell
   - Long sustain, heavy reverb, soft dynamics

3. **🥁 Percussive** - Sharp, rhythmic sounds (C3, minor pentatonic)
   - Kick, Snare, Hi-hat
   - Short decay, tight reverb, punchy attacks

4. **✨ Ethereal** - High, dreamy tones (C4, lydian mode)
   - Crystal, Glass, Wind
   - Very long sustain, maximum reverb, delicate character

Each flavor comes with optimized parameters that can be further customized via the settings panel.

---

---

## 🔧 Troubleshooting

### "Not seeing a stream of Bluetooth devices in Chrome"

This is a **known limitation** of Chrome's Web Bluetooth API. Here's what to do:

1. **Use the Standalone Desktop App (Recommended)**
   - Run `npm install` then `npm start`
   - Or double-click `start.sh` (Mac/Linux) or `start.bat` (Windows)
   - This provides unrestricted native BLE scanning

2. **Verify Chrome Experimental Flag is Enabled**
   - Open: `chrome://flags/#enable-experimental-web-platform-features`
   - Set to "Enabled"
   - **Restart Chrome completely** (close all Chrome windows)
   - Open `index.html` and try again
   - Open DevTools Console (F12) to see debugging logs

3. **Check Browser Console for Errors**
   - Open DevTools (F12) → Console tab
   - Look for BLE-related errors
   - You should see logs like "Starting passive BLE scan..." and "Advertisement received"
   - If you see "requestLEScan is not a function", the experimental API isn't available

4. **Try Demo Mode**
   - Click "🎭 Demo Mode" to verify the audio system works
   - This confirms the issue is with BLE, not the audio engine

5. **Check System Bluetooth**
   - Ensure Bluetooth is turned ON in your operating system
   - Verify other BLE devices are nearby (phones, watches, headphones)
   - Try the Python bridge (`python bridge.py`) to confirm devices are detectable

### Known Chrome Limitations

- **Privacy Restrictions**: Chrome's Web Bluetooth API intentionally limits passive scanning
- **Experimental API**: `requestLEScan()` may not work in all Chrome versions
- **HTTPS Required**: Web Bluetooth only works on HTTPS or localhost
- **No Background Scanning**: Browser tabs must be active

**Solution**: Use the standalone desktop app for reliable BLE scanning!

---

## 🔬 Original BLE-to-Sound System (Python + SuperCollider)

For the full original experience with more control, use the Python/SuperCollider setup:

### Requirements
- Python 3 with `bleak` and `python-osc` packages
- SuperCollider

### Setup

```bash
# Install Python dependencies
pip install bleak python-osc

# Run the BLE bridge
python bridge.py
```

Then open one of the SuperCollider flavor files (`.scd`) in SuperCollider and evaluate:

- **woods.scd** - Melodic flavor with balanced tones
- **flavor-ambient.scd** - Ambient flavor with long sustain and spacious reverb
- **flavor-percussive.scd** - Percussive flavor with sharp, rhythmic sounds
- **flavor-ethereal.scd** - Ethereal flavor with high, dreamy tones

Each `.scd` file contains configurable parameters at the top:
- `masterVol` - Overall output level (0.0 to 1.0)
- `rootNote` - Base pitch (MIDI note number)
- `scale` - Musical scale (minorPentatonic, majorPentatonic, minor, major, etc.)
- `movementThreshold` - How much RSSI must change to trigger a note
- `deviceMap` - Assign specific instruments to device MAC addresses

### Architecture

1. `bridge.py` - Scans for BLE devices and sends RSSI data via OSC
2. SuperCollider `.scd` files - Generate sounds based on BLE signal data
