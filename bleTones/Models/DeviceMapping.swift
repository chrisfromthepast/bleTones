import Foundation

// MARK: - Curve type

enum CCCurve: String, Codable, CaseIterable, Identifiable {
    case linear
    case exponential

    var id: String { rawValue }

    func apply(_ x: Double) -> Double {
        switch self {
        case .linear: return x
        case .exponential: return x * x
        }
    }
}

// MARK: - Waveform

enum Waveform: String, Codable, CaseIterable, Identifiable {
    case sine
    case triangle

    var id: String { rawValue }
}

// MARK: - DeviceMapping

struct DeviceMapping: Codable, Equatable {
    var midiChannel: Int = 1           // 1–16
    var midiEnableCC: Bool = true
    var midiCCNumber: Int = 20         // 0–127
    var midiEnableNotes: Bool = false
    var midiNoteNumber: Int = 60       // 0–127
    var midiVelocity: Int = 100        // 0–127
    var noteOnThreshold: Int = -60     // RSSI dBm
    var noteOffThreshold: Int = -66    // RSSI dBm
    var midiSendToVirtualOut: Bool = true
    var midiDestinationUIDs: [String] = []
    var rssiMin: Int = -100
    var rssiMax: Int = -35
    var ccCurve: CCCurve = .linear
    var localSoundEnabled: Bool = true
    var waveform: Waveform = .sine
    var baseFrequencyHz: Double = 220
    var gain: Double = 0.4
    var pan: Double = 0.0              // -1 … 1

    // MARK: OSC scaffold (deferred)
    var oscEnabled: Bool = false
    var oscAddress: String = "/bletones"
    var oscPort: Int = 9000
}
