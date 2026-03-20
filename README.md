# bleTones

A native iOS app that scans nearby Bluetooth Low Energy (BLE) devices and converts their proximity (RSSI) into MIDI messages and audio tones.

## Features

- **BLE Scanning** – Discovers nearby BLE peripherals with real-time RSSI readings
- **MIDI CC Output** – Maps device proximity to continuous controller messages
- **MIDI Note Triggers** – Hysteresis-based note on/off from RSSI thresholds
- **Virtual MIDI Source** – Appears as "bleTones MIDI Out" in any CoreMIDI host (AUM, GarageBand, etc.)
- **Per-Device Destinations** – Route MIDI to specific external endpoints in addition to the virtual out
- **Local Audio Synth** – Sine/triangle oscillator per device, amplitude driven by proximity
- **JSON Persistence** – Device names, selections, and mappings saved automatically

## Requirements

- iOS 17.0+
- Xcode 15.0+
- Device with Bluetooth (BLE scanning requires a physical device)

## Getting Started

1. Open `bleTones.xcodeproj` in Xcode
2. Select your development team under Signing & Capabilities
3. Build and run on an iOS device (BLE scanning does not work in Simulator)

For a detailed walkthrough see **[XCODE_SETUP.md](XCODE_SETUP.md)**.

## CI

Every push and PR to `main` is automatically built on a macOS GitHub Actions runner. See `.github/workflows/build.yml`. The CI job compiles for both Simulator and device targets (without code signing).

## Architecture

| Layer | Class | Responsibility |
|-------|-------|---------------|
| Bluetooth | `BLECentral` | CoreBluetooth scanning, throttled UI list, filtered RSSI |
| Persistence | `DeviceStore` | JSON read/write, device naming, selection, mapping storage |
| MIDI | `MIDIManager` | Virtual MIDI source, output port, destination enumeration |
| Audio | `AudioEngineManager` | AVAudioEngine with source node synth (sine/triangle) |
| Coordinator | `UpdateCoordinator` | 20 Hz tick loop driving CC, Notes, and audio for all selected devices |

## Bundle ID

`com.chrisfromthepast.bletones`

## License

See [LICENSE](LICENSE) for details.
