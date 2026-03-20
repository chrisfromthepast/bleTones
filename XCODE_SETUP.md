# Xcode Setup Guide — bleTones

A quick walkthrough to get bleTones building on your Mac and running on your iPhone or iPad.

---

## Prerequisites

| Requirement | Minimum |
|---|---|
| macOS | Ventura 13.5+ |
| Xcode | 15.0+ (free from the Mac App Store) |
| iOS device | iPhone or iPad running iOS 17.0+ |
| Apple ID | Any — a free account works for personal-device testing |

> **Simulator note:** The app compiles in the Simulator but BLE scanning requires a physical device with Bluetooth hardware.

---

## 1 — Open the Project

```
git clone https://github.com/chrisfromthepast/bleTones.git
cd bleTones
open bleTones.xcodeproj
```

Xcode will index the project. Wait until the spinner in the toolbar stops.

---

## 2 — Set Your Signing Team

1. In the **Project Navigator** (left sidebar), click the top-level **bleTones** project (blue icon).
2. Select the **bleTones** target.
3. Go to the **Signing & Capabilities** tab.
4. Check **Automatically manage signing**.
5. Under **Team**, choose your Apple ID (or add one via *Xcode → Settings → Accounts*).
6. If the Bundle Identifier (`com.chrisfromthepast.bletones`) shows a conflict, change it to something unique, like `com.yourname.bletones`.

> **Free Apple ID limitation:** Apps signed with a free account expire after 7 days and must be re-installed. A paid Apple Developer account ($99/year) removes this limit and is required for App Store distribution.

---

## 3 — Select a Run Destination

In the toolbar at the top of Xcode:

- Click the **device dropdown** (next to the play ▶ button).
- Plug in your iPhone/iPad via USB (or set up wireless debugging in *Window → Devices and Simulators*).
- Select your physical device from the list.

If this is your first time, Xcode may ask you to **trust the device** — follow the on-screen prompts on both Mac and device.

---

## 4 — Build & Run

Press **⌘R** (or click ▶).

- Xcode compiles all Swift files, bundles assets, signs the app, and installs it on the device.
- The first build takes about 30–60 seconds; incremental builds are much faster.
- Once installed, bleTones will launch automatically.

### First Launch

On your device you will see a Bluetooth permission prompt:

> *"bleTones" Would Like to Use Bluetooth*

Tap **OK**. The app will begin scanning for nearby BLE peripherals.

---

## 5 — Troubleshooting

| Problem | Fix |
|---|---|
| **"Untrusted Developer"** dialog on device | Go to *Settings → General → VPN & Device Management*, tap your profile, and tap **Trust**. |
| **No devices show up in scan** | Make sure Bluetooth is enabled on the device and that there are BLE peripherals nearby (almost any Bluetooth device counts). |
| **"No signing identity found"** | Make sure you selected a Team in Signing & Capabilities (step 2). |
| **Build fails with "No such module"** | Do a clean build: *Product → Clean Build Folder* (⇧⌘K), then rebuild. |
| **Provisioning profile errors** | Change the Bundle Identifier to something unique (step 2.6). |

---

## 6 — Building for Release / Archive

To create a release build (.ipa) that can be distributed via TestFlight or the App Store:

1. Set the destination to **Any iOS Device (arm64)** in the toolbar.
2. Go to *Product → Archive*.
3. When the archive completes, the **Organizer** window opens.
4. Click **Distribute App** and choose your distribution method:
   - **TestFlight & App Store** — uploads to App Store Connect
   - **Ad Hoc** — creates an .ipa for sideloading to registered devices
   - **Development** — for testing on your own registered devices

> You need a paid Apple Developer account for TestFlight/App Store distribution.

---

## 7 — CI Builds with GitHub Actions

This repo includes a GitHub Actions workflow (`.github/workflows/build.yml`) that automatically:

- Builds the project on every push/PR to `main`
- Verifies the project compiles for both Simulator and device targets
- Does **not** code-sign (CI builds are verification-only)

### Do I need to build on each OS?

**No.** Since bleTones is an iOS-only app, all builds happen on a macOS runner using Xcode. There is no Windows or Linux build needed. The GitHub Actions `macos-15` runner includes Xcode 16, which builds for all iOS device architectures (arm64) from a single machine.

For App Store submission, you do need to **archive and sign** on a Mac with your Apple Developer certificates — either your local machine or a self-hosted macOS runner with credentials configured.

---

## Project Structure

```
bleTones.xcodeproj         ← Xcode project file
bleTones/
├── bleTonesApp.swift      ← App entry point (@main)
├── Info.plist             ← Bluetooth usage descriptions
├── Assets.xcassets/       ← App icon and colors
├── Preview Content/       ← SwiftUI preview assets
├── Models/
│   ├── DeviceMapping.swift    ← Per-device MIDI/audio config
│   ├── KnownDevice.swift      ← Persisted device state
│   ├── DiscoveredDevice.swift  ← Live BLE scan result
│   └── MIDIDestinationInfo.swift
├── Services/
│   ├── BLECentral.swift       ← CoreBluetooth scanning
│   ├── DeviceStore.swift      ← JSON persistence
│   ├── MIDIManager.swift      ← CoreMIDI virtual source + output
│   ├── AudioEngineManager.swift ← AVAudioEngine synth
│   └── UpdateCoordinator.swift  ← 20Hz update loop
└── Views/
    ├── ContentView.swift          ← Tab bar (Scan / Mappings / Settings)
    ├── ScanView.swift             ← BLE device list
    ├── DeviceDetailView.swift     ← Device name + status
    ├── DeviceMappingView.swift    ← MIDI CC/Notes/Audio config
    ├── MIDIDestinationsPickerView.swift
    ├── MappingsListView.swift
    └── SettingsView.swift
```

---

## Quick Reference

| Action | Shortcut |
|---|---|
| Build & Run | ⌘R |
| Stop | ⌘. |
| Clean Build | ⇧⌘K |
| Archive (release) | Product → Archive |
| Devices | ⇧⌘2 |

Happy hacking! 🎵📡
