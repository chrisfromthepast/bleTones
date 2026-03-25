# Testing bleTones — Complete Guide

This guide helps you get bleTones running and verify that it's producing sound.
**No developer registration or special accounts are needed** — just the built
plugin/app and a few nearby Bluetooth devices.

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Quick Start (Standalone App)](#quick-start-standalone-app)
3. [Testing with a DAW (VST3/AU)](#testing-with-a-daw-vst3au)
4. [Testing Without BLE Hardware](#testing-without-ble-hardware)
5. [Troubleshooting: No BLE Data](#troubleshooting-no-ble-data)
6. [Troubleshooting: No Sound](#troubleshooting-no-sound)
7. [FAQ](#faq)

---

## Prerequisites

| Requirement            | Details |
|------------------------|---------|
| **macOS**              | macOS 10.15+ (Catalina or later) |
| **Windows**            | Windows 10 build 1903+ or Windows 11 |
| **Linux**              | ⚠️ BLE helper not yet implemented on Linux — see [Testing Without BLE Hardware](#testing-without-ble-hardware) |
| **Bluetooth devices**  | Any nearby Bluetooth/BLE devices (phones, watches, headphones, etc.) |
| **Built artifacts**    | Plugin + helper must be built — see [README.md](README.md#building) |

**You do NOT need:**
- An Apple Developer account
- Windows developer mode
- Any paid software or registration
- Any special permissions beyond Bluetooth access

---

## Quick Start (Standalone App)

The **easiest** way to test bleTones:

### macOS

1. **Build the project** (if not already done):
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ```

2. **Launch the Standalone app**:
   ```bash
   open build/bleTones_artefacts/Release/Standalone/bleTones.app
   ```
   
   > On macOS Standalone, the BLE helper starts **automatically** — no extra step needed.

3. **Grant Bluetooth permission** when macOS prompts you. If you missed the prompt:
   - Go to **System Settings → Privacy & Security → Bluetooth**
   - Enable both `bleTones` and `bleTones Helper`

4. **Check the UI**:
   - The left panel ("BLE Scanner") should show "Listening on UDP 9000..."
   - After a few seconds, nearby BLE devices appear with their RSSI values
   - Orbs appear in the center; you should hear ambient tones

5. **No devices showing?** Make sure:
   - Bluetooth is turned ON in System Settings
   - You have BLE devices nearby (phones, watches, wireless headphones)
   - The helper was launched (check Activity Monitor for `bleTones_helper`)

### Windows

1. **Build the project**:
   ```bat
   cmake -B build -G "Visual Studio 17 2022"
   cmake --build build --config Release
   ```

2. **Launch the helper first**:
   ```bat
   build\helper\Release\bleTones_helper.exe
   ```
   
   Leave it running in a console window.

3. **Launch the Standalone app**:
   ```bat
   build\bleTones_artefacts\Release\Standalone\bleTones.exe
   ```

4. **Check the UI** — same as macOS above.

---

## Testing with a DAW (VST3/AU)

When using bleTones as a plugin in your DAW, **you must start the helper manually**.

### macOS

```bash
# Start the BLE helper (it runs silently — no window)
open build/helper/bleTones_helper.app

# Now open your DAW and load bleTones as an instrument
```

### Windows

```bat
:: Start the BLE helper (shows a console window with scan output)
build\helper\Release\bleTones_helper.exe

:: Now open your DAW and load bleTones as an instrument
```

### In Your DAW

1. Add an instrument track
2. Load `bleTones` as a VST3 or AU instrument
3. The plugin UI should show "Listening on UDP 9000..."
4. Enable the track output and turn up the volume
5. BLE devices should appear within seconds

---

## Testing Without BLE Hardware

If you don't have Bluetooth devices nearby, or you're on Linux, or you want to
test the sound engine without BLE, you can **send fake OSC data** to the plugin.

bleTones listens on **UDP port 9000** for messages:
```
/ble/rssi  <deviceName:string>  <rssi:int32>
```

### Using the OSC Test Script

A test script is provided for quick testing:

```bash
# Python test script (requires Python 3)
python3 tools/osc_test_sender.py
```

This script simulates 4 BLE devices with varying RSSI values, so you can see
the visual feedback and hear sound without any real Bluetooth devices.

### Using `sendosc` (if installed)

```bash
# Install sendosc (macOS)
brew install liblo

# Send a test device
sendosc localhost 9000 /ble/rssi s "TestPhone" i -45

# Send multiple devices
sendosc localhost 9000 /ble/rssi s "Phone" i -40
sendosc localhost 9000 /ble/rssi s "Watch" i -55
sendosc localhost 9000 /ble/rssi s "Headphones" i -70
```

### Using Python directly

```python
import socket
import struct

def send_osc_rssi(device_name: str, rssi: int, host="127.0.0.1", port=9000):
    """Send a /ble/rssi OSC message to bleTones."""
    def osc_string(s):
        b = s.encode('utf-8') + b'\x00'
        while len(b) % 4 != 0:
            b += b'\x00'
        return b

    packet = osc_string("/ble/rssi")
    packet += osc_string(",si")
    packet += osc_string(device_name)
    packet += struct.pack(">i", rssi)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto(packet, (host, port))
    sock.close()

# Test it
send_osc_rssi("FakePhone", -45)
send_osc_rssi("FakeWatch", -60)
```

---

## Troubleshooting: No BLE Data

### Symptom: "Waiting for devices..." never changes

**Check 1: Is the helper running?**

- **macOS**: Run `pgrep -l bleTones_helper` — if empty, the helper isn't running.
- **Windows**: Check Task Manager for `bleTones_helper.exe`.

**Fix**: Launch the helper manually:
```bash
# macOS
open build/helper/bleTones_helper.app

# Windows
build\helper\Release\bleTones_helper.exe
```

---

**Check 2: Bluetooth permission (macOS)**

- Go to **System Settings → Privacy & Security → Bluetooth**
- Make sure `bleTones Helper` is listed and enabled
- If not listed, the helper may not have requested permission yet — relaunch it

---

**Check 3: Bluetooth is ON**

- macOS: Bluetooth icon in menu bar should be enabled
- Windows: Settings → Bluetooth & devices → Bluetooth ON

---

**Check 4: Port 9000 conflict**

Another app might be using UDP port 9000.

```bash
# macOS/Linux — check what's using port 9000
lsof -i :9000

# Windows
netstat -aon | findstr 9000
```

If another app is using port 9000, close it first, then restart bleTones.

---

**Check 5: Firewall blocking UDP**

- Your firewall might be blocking local UDP traffic
- Temporarily disable the firewall or add an exception for localhost:9000

---

## Troubleshooting: No Sound

### Symptom: Devices appear but no sound is heard

**Check 1: Volume slider**

- The Volume slider (bottom of the UI) might be at 0
- Drag it to at least 0.5

---

**Check 2: DAW output routing**

- If using a DAW, make sure the instrument track output is routed to master
- Make sure the DAW is playing (some DAWs mute instruments when stopped)
- Enable monitoring or arm the track for recording

---

**Check 3: Audio output device**

- Make sure your system audio output is set correctly
- In the Standalone app, go to **Options → Audio Settings** and verify the output device

---

**Check 4: Device RSSI values are too low**

- Devices with RSSI below -100 dBm are considered "too far" and produce very quiet tones
- Move closer to your BLE devices or use the test script with higher RSSI values

---

**Check 5: Movement-based triggering**

bleTones triggers notes based on **RSSI changes (movement)**, not absolute values.
If devices are stationary, you'll hear less activity.

- Walk around with your phone to create RSSI variations
- Use the test script which simulates moving devices:
  ```bash
  python3 tools/osc_test_sender.py
  ```

---

## FAQ

### Do I need an Apple Developer account?

**No.** The plugin and helper are built locally. macOS may show a Gatekeeper
warning ("app is from an unidentified developer") — right-click the `.app` and
choose "Open" to bypass this, or run:
```bash
xattr -cr bleTones.app
xattr -cr "bleTones Helper.app"
```

### Do I need to register anything on Windows?

**No.** The helper uses standard WinRT BLE APIs available on Windows 10+. No
developer mode, no signing, no registration required.

### Can I test on Linux?

The BLE helper is **not yet implemented** for Linux (BlueZ support pending).
However, you can:
1. Build the plugin on Linux
2. Send fake OSC data using the test script (see [Testing Without BLE Hardware](#testing-without-ble-hardware))

### Why does the helper run as a separate process?

On macOS, DAW hosts can't request Bluetooth permission on behalf of plugins
(the host's `Info.plist` doesn't include the Bluetooth usage description).
Running a separate helper with its own `Info.plist` solves this.

### How can I see what the helper is doing?

**macOS**: Launch from Terminal to see console output:
```bash
./build/helper/bleTones_helper.app/Contents/MacOS/bleTones_helper
```

**Windows**: The helper runs in a console window and prints discovered devices.

### The helper says "Bluetooth unavailable"

- macOS: Make sure Bluetooth is ON in System Settings
- Windows: Make sure Bluetooth is enabled in Settings → Bluetooth & devices
- Try restarting Bluetooth or the helper

### Can I contribute to Linux support?

Yes! The Linux BlueZ implementation is pending. See `helper/CMakeLists.txt`
line 47-51 for the placeholder. PRs welcome!

---

## Getting Help

If you're still stuck:

1. **Check the build output** — make sure both the plugin AND helper built successfully
2. **Run the test script** to verify the sound engine works independently of BLE
3. **Open an issue** on GitHub with:
   - Your OS and version
   - Build output / error messages
   - What you see in the UI
   - Whether the helper is running
