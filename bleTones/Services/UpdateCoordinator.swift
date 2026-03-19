import Foundation
import Combine

/// Runs a 20 Hz timer that drives MIDI CC/Notes and audio for all selected devices.
final class UpdateCoordinator: ObservableObject {

    // MARK: Dependencies

    private let deviceStore: DeviceStore
    private let bleCentral: BLECentral
    private let midiManager: MIDIManager
    private let audioEngine: AudioEngineManager

    // MARK: Per-device state

    private var noteIsOn: [UUID: Bool] = [:]
    private var lastCCSentAt: [UUID: Date] = [:]
    private var lastCCValue: [UUID: Int] = [:]
    private var deviceLastSeen: [UUID: Date] = [:]

    // MARK: Timer

    private var tickTimer: Timer?
    private static let tickInterval: TimeInterval = 1.0 / 20.0  // 20 Hz
    private static let staleTimeout: TimeInterval = 10.0

    // MARK: - Init

    init(deviceStore: DeviceStore,
         bleCentral: BLECentral,
         midiManager: MIDIManager,
         audioEngine: AudioEngineManager) {
        self.deviceStore = deviceStore
        self.bleCentral = bleCentral
        self.midiManager = midiManager
        self.audioEngine = audioEngine
        start()
    }

    func start() {
        tickTimer?.invalidate()
        tickTimer = Timer.scheduledTimer(withTimeInterval: Self.tickInterval,
                                         repeats: true) { [weak self] _ in
            self?.tick()
        }
    }

    func stop() {
        tickTimer?.invalidate()
        tickTimer = nil
    }

    // MARK: - Tick

    private func tick() {
        let now = Date()
        let selectedDevices = deviceStore.known.values.filter { $0.isSelected }

        for device in selectedDevices {
            let id = device.id
            let mapping = device.mapping

            // 1. Fetch latest RSSI
            let rssi: Double
            if let discovered = bleCentral.latestDiscovered(for: id) {
                rssi = discovered.filteredRSSI
                deviceLastSeen[id] = discovered.lastSeen
                deviceStore.updateLastSeen(id: id, rssi: Int(discovered.filteredRSSI), at: discovered.lastSeen)
            } else if let lastRSSI = device.lastRSSI {
                rssi = Double(lastRSSI)
            } else {
                rssi = Double(mapping.rssiMin)
            }

            // Check staleness
            let lastSeen = deviceLastSeen[id] ?? .distantPast
            let stale = now.timeIntervalSince(lastSeen) > Self.staleTimeout

            // 2. Normalize RSSI
            let range = Double(mapping.rssiMax - mapping.rssiMin)
            let raw = range > 0 ? (rssi - Double(mapping.rssiMin)) / range : 0
            let clamped = min(max(raw, 0), 1)
            let normalized = mapping.ccCurve.apply(clamped)

            // 3. Local audio
            if mapping.localSoundEnabled && !stale {
                let freq: Double
                if mapping.midiEnableNotes {
                    freq = noteToHz(mapping.midiNoteNumber)
                } else {
                    freq = mapping.baseFrequencyHz
                }
                let amp = normalized * mapping.gain
                audioEngine.setVoice(id: id, freq: freq, targetAmp: amp,
                                     pan: mapping.pan, waveform: mapping.waveform)
            } else if stale {
                // Ramp amplitude to 0 (engine will smooth)
                audioEngine.setVoice(id: id, freq: 0, targetAmp: 0,
                                     pan: mapping.pan, waveform: mapping.waveform)
            }

            // 4. MIDI CC (rate-limited)
            if mapping.midiEnableCC && !stale {
                let ccValue = Int(round(normalized * 127))
                let lastValue = lastCCValue[id] ?? -1
                let lastSent = lastCCSentAt[id] ?? .distantPast
                let minInterval: TimeInterval = 1.0 / 30.0

                if abs(ccValue - lastValue) >= 1 &&
                    now.timeIntervalSince(lastSent) >= minInterval {
                    midiManager.sendCC(
                        channel: mapping.midiChannel,
                        cc: mapping.midiCCNumber,
                        value: ccValue,
                        sendToVirtualOut: mapping.midiSendToVirtualOut,
                        destinationUIDs: mapping.midiDestinationUIDs
                    )
                    lastCCValue[id] = ccValue
                    lastCCSentAt[id] = now
                }
            }

            // 5. MIDI Notes (hysteresis state machine)
            if mapping.midiEnableNotes && !stale {
                let isOn = noteIsOn[id] ?? false
                let intRSSI = Int(rssi)

                if !isOn && intRSSI >= mapping.noteOnThreshold {
                    midiManager.sendNoteOn(
                        channel: mapping.midiChannel,
                        note: mapping.midiNoteNumber,
                        velocity: mapping.midiVelocity,
                        sendToVirtualOut: mapping.midiSendToVirtualOut,
                        destinationUIDs: mapping.midiDestinationUIDs
                    )
                    noteIsOn[id] = true
                } else if isOn && intRSSI <= mapping.noteOffThreshold {
                    midiManager.sendNoteOff(
                        channel: mapping.midiChannel,
                        note: mapping.midiNoteNumber,
                        sendToVirtualOut: mapping.midiSendToVirtualOut,
                        destinationUIDs: mapping.midiDestinationUIDs
                    )
                    noteIsOn[id] = false
                }
            }

            // 6. Stale cleanup: prevent hung notes
            if stale, noteIsOn[id] == true {
                midiManager.sendNoteOff(
                    channel: mapping.midiChannel,
                    note: mapping.midiNoteNumber,
                    sendToVirtualOut: mapping.midiSendToVirtualOut,
                    destinationUIDs: mapping.midiDestinationUIDs
                )
                noteIsOn[id] = false
            }
        }
    }

    // MARK: - Helpers

    /// Convert MIDI note number to frequency in Hz.
    private func noteToHz(_ note: Int) -> Double {
        440.0 * pow(2.0, (Double(note) - 69.0) / 12.0)
    }
}
