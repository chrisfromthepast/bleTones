# bleTones

A generative sound experience that creates beautiful, organic music through interaction.

## 🎵 Interactive Web Experience

**[Try it live in your browser!](index.html)** - No installation required.

### How to Use

1. Open `index.html` in Chrome (or any modern browser)
2. Click **"Begin Experience"** to enable audio
3. **Move your mouse** or **touch the screen** to create sounds
4. The faster you move, the louder the sound
5. Position on screen affects pitch (top = high, bottom = low)
6. Choose different instruments: Wood Chimes, Deep Log, Hollow Reed, or Kalimba

### Features

- 🎹 **4 Unique Instruments** - Each with distinct wooden/organic character
- 🎨 **Real-time Visualization** - Particle effects respond to your movements
- 📱 **Touch Support** - Works great on mobile devices
- 🔊 **Spatial Audio** - Left/right panning based on horizontal position
- 🎼 **Musical Scale** - Notes are quantized to a minor pentatonic scale

---

## 🔬 Original BLE-to-Sound System

The original version uses Bluetooth Low Energy signals to generate music:

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
