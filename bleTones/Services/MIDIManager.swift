import Foundation
import CoreMIDI

/// Manages a CoreMIDI virtual source ("bleTones MIDI Out"), an output port,
/// and destination enumeration for per-device routing.
final class MIDIManager: ObservableObject {

    // MARK: Published

    @Published var destinations: [MIDIDestinationInfo] = []

    // MARK: CoreMIDI refs

    private var client: MIDIClientRef = 0
    private var virtualSource: MIDIEndpointRef = 0
    private var outputPort: MIDIPortRef = 0

    // MARK: - Init

    init() {
        setupMIDI()
        refreshDestinations()
    }

    // MARK: - Setup

    private func setupMIDI() {
        var status = MIDIClientCreateWithBlock("bleTones" as CFString, &client) { [weak self] _ in
            // Refresh destinations when MIDI setup changes
            DispatchQueue.main.async { self?.refreshDestinations() }
        }
        guard status == noErr else {
            print("[MIDIManager] client create error: \(status)")
            return
        }

        status = MIDISourceCreateWithProtocol(
            client,
            "bleTones MIDI Out" as CFString,
            ._1_0,
            &virtualSource
        )
        if status != noErr {
            print("[MIDIManager] virtual source create error: \(status)")
        }

        status = MIDIOutputPortCreateWithProtocol(
            client,
            "bleTones Output" as CFString,
            ._1_0,
            &outputPort
        )
        if status != noErr {
            print("[MIDIManager] output port create error: \(status)")
        }
    }

    // MARK: - Destination enumeration

    func refreshDestinations() {
        var list: [MIDIDestinationInfo] = []
        let count = MIDIGetNumberOfDestinations()
        for i in 0..<count {
            let endpoint = MIDIGetDestination(i)
            var cfName: Unmanaged<CFString>?
            let nameStatus = MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &cfName)
            let name: String
            if nameStatus == noErr, let cf = cfName {
                name = cf.takeUnretainedValue() as String
            } else {
                name = "Unknown"
            }
            var uniqueID: Int32 = 0
            MIDIObjectGetIntegerProperty(endpoint, kMIDIPropertyUniqueID, &uniqueID)
            list.append(MIDIDestinationInfo(name: name, uniqueID: String(uniqueID)))
        }
        DispatchQueue.main.async {
            self.destinations = list
        }
    }

    // MARK: - Resolve per-device destinations

    func resolveDestinations(uids: [String]) -> [MIDIEndpointRef] {
        var results: [MIDIEndpointRef] = []
        let count = MIDIGetNumberOfDestinations()
        for i in 0..<count {
            let endpoint = MIDIGetDestination(i)
            var uniqueID: Int32 = 0
            MIDIObjectGetIntegerProperty(endpoint, kMIDIPropertyUniqueID, &uniqueID)
            if uids.contains(String(uniqueID)) {
                results.append(endpoint)
            }
        }
        return results
    }

    // MARK: - Send MIDI

    /// Send a CC message.
    func sendCC(channel: Int, cc: Int, value: Int,
                sendToVirtualOut: Bool, destinationUIDs: [String]) {
        let statusByte = UInt8(0xB0 | ((channel - 1) & 0x0F))
        let bytes: [UInt8] = [statusByte, UInt8(cc & 0x7F), UInt8(value & 0x7F)]
        send(bytes: bytes, sendToVirtualOut: sendToVirtualOut, destinationUIDs: destinationUIDs)
    }

    /// Send a NoteOn message.
    func sendNoteOn(channel: Int, note: Int, velocity: Int,
                    sendToVirtualOut: Bool, destinationUIDs: [String]) {
        let statusByte = UInt8(0x90 | ((channel - 1) & 0x0F))
        let bytes: [UInt8] = [statusByte, UInt8(note & 0x7F), UInt8(velocity & 0x7F)]
        send(bytes: bytes, sendToVirtualOut: sendToVirtualOut, destinationUIDs: destinationUIDs)
    }

    /// Send a NoteOff message.
    func sendNoteOff(channel: Int, note: Int,
                     sendToVirtualOut: Bool, destinationUIDs: [String]) {
        let statusByte = UInt8(0x80 | ((channel - 1) & 0x0F))
        let bytes: [UInt8] = [statusByte, UInt8(note & 0x7F), 0]
        send(bytes: bytes, sendToVirtualOut: sendToVirtualOut, destinationUIDs: destinationUIDs)
    }

    // MARK: - Internal send (modern MIDIEventList API)

    private func send(bytes: [UInt8], sendToVirtualOut: Bool, destinationUIDs: [String]) {
        guard bytes.count >= 1 else { return }

        // Build UMP word for MIDI 1.0 channel voice message (message type 2, group 0).
        // Format: [mt:4][group:4][status:8][data1:8][data2:8]
        var word: UInt32 = 0x20000000
        word |= UInt32(bytes[0]) << 16
        if bytes.count > 1 { word |= UInt32(bytes[1]) << 8 }
        if bytes.count > 2 { word |= UInt32(bytes[2]) }

        var eventList = MIDIEventList()
        var packet = MIDIEventListInit(&eventList, ._1_0)
        packet = MIDIEventListAdd(&eventList,
                                  MemoryLayout<MIDIEventList>.size,
                                  packet,
                                  0,  // timestamp: now
                                  1,  // wordCount
                                  &word)

        if sendToVirtualOut && virtualSource != 0 {
            MIDIReceivedEventList(virtualSource, &eventList)
        }

        let endpoints = resolveDestinations(uids: destinationUIDs)
        for endpoint in endpoints {
            MIDISendEventList(outputPort, endpoint, &eventList)
        }
    }
}
