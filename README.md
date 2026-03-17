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

### BLE Mode (Chrome with Web Bluetooth)

The web experience can now use **real Bluetooth Low Energy (BLE) data** to generate tones, just like the original project!

1. Switch to "📡 BLE Devices" mode
2. Click "Start BLE Scan" to scan for nearby BLE devices
3. As BLE devices move or their signal strength changes, sounds are generated
4. Closer devices produce higher pitches
5. Signal strength changes affect amplitude

**Requirements for BLE Mode:**
- Chrome browser (version 79+) with Web Bluetooth support
- For best experience, enable `chrome://flags/#enable-experimental-web-platform-features`
- HTTPS connection (or localhost) is required for Web Bluetooth

### Features

- 🎹 **4 Unique Instruments** - Each with distinct wooden/organic character
- 📡 **BLE Integration** - Generate music from Bluetooth device signals
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
