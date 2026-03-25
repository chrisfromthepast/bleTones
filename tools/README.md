# bleTones Tools

Utilities for testing and debugging bleTones.

## osc_test_sender.py

Simulates BLE devices by sending fake OSC data to bleTones. Useful for:
- Testing bleTones without Bluetooth hardware
- Debugging the sound engine
- Testing on Linux (where the BLE helper isn't implemented yet)
- Reproducing specific scenarios

### Quick Start

```bash
# Run with defaults (4 devices, continuous sending)
python3 osc_test_sender.py

# Send a single snapshot
python3 osc_test_sender.py --once

# Simulate more devices
python3 osc_test_sender.py --devices 8 --verbose
```

### Options

| Option | Default | Description |
|--------|---------|-------------|
| `--host` | `127.0.0.1` | Target host |
| `--port` | `9000` | Target UDP port |
| `--devices` | `4` | Number of simulated devices |
| `--interval` | `0.1` | Seconds between sends |
| `--once` | (flag) | Send one batch and exit |
| `-v, --verbose` | (flag) | Print each message |

### OSC Protocol

The script sends OSC messages matching bleTones' expected format:

```
/ble/rssi  <deviceName:string>  <rssi:int32>
```

Where:
- `deviceName`: Human-readable device name (e.g., "iPhone", "AirPods Pro")
- `rssi`: Signal strength in dBm (-100 to -30)
