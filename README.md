# bleTones

A generative sound experience that creates beautiful, organic music through interaction.

## 🎵 Interactive Web Experience

**[Try it live in your browser!](index.html)** - No installation required.

### How to Use

1. Open `index.html` in Chrome (or any modern browser)
2. Click **"Begin Experience"** to enable audio
3. Choose your input mode:
   - **🖱️ Mouse/Touch**: Move your mouse or touch the screen to create sounds
   - **📡 BLE Devices**: Use nearby Bluetooth devices to generate sounds

### Mouse/Touch Mode

- **Move your mouse** or **touch the screen** to create sounds
- The faster you move, the louder the sound
- Position on screen affects pitch (top = high, bottom = low)
- Choose different instruments: Wood Chimes, Deep Log, Hollow Reed, or Kalimba

### BLE Mode

The web experience can use **Bluetooth Low Energy (BLE) data** to generate tones, just like the original project!

#### 🎭 Demo Mode (Recommended for Most Users)

Demo Mode generates **simulated BLE signals** so you can experience the sound generation without any Bluetooth hardware or setup:

1. Switch to "📡 BLE Devices" mode
2. Click **"🎭 Demo Mode"**
3. Enjoy the generative music from simulated BLE devices!

This is perfect for:
- Testing the experience on any browser
- Using the app without Bluetooth hardware
- Demonstrating the concept to others

#### 🔍 Real BLE Mode

For actual Bluetooth device scanning:

**Option A: Passive Scanning (Experimental)**
- Enable `chrome://flags/#enable-experimental-web-platform-features` in Chrome
- This allows passive scanning without pairing
- Best experience: detects all nearby BLE devices automatically

**Option B: Device Pairing (Standard)**
- Without the experimental flag, Chrome requires pairing with a specific device
- Click "🔍 Real BLE" and select a device from the picker
- Limited to monitoring only the paired device

> ⚠️ **Note on Chrome BLE Limitations**: The Web Bluetooth API does not support passive RSSI scanning without user interaction for privacy reasons. For unrestricted BLE scanning, use the Python bridge with SuperCollider (see below).

**Requirements for Real BLE Mode:**
- Chrome browser (version 79+) with Web Bluetooth support
- HTTPS connection (or localhost) is required for Web Bluetooth

### Features

- 🎹 **4 Unique Instruments** - Each with distinct wooden/organic character
- 📡 **BLE Integration** - Generate music from Bluetooth device signals
- 🎭 **Demo Mode** - Experience BLE sound generation without Bluetooth hardware
- 🎨 **Real-time Visualization** - Particle effects respond to your movements
- 📱 **Touch Support** - Works great on mobile devices
- 🔊 **Spatial Audio** - Left/right panning based on horizontal position
- 🎼 **Musical Scale** - Notes are quantized to a minor pentatonic scale

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

Then open one of the SuperCollider files (`.scd`) in SuperCollider and evaluate.

### Architecture

1. `bridge.py` - Scans for BLE devices and sends RSSI data via OSC
2. SuperCollider `.scd` files - Generate sounds based on BLE signal data
