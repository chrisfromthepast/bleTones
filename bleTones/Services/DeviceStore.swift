import Foundation
import Combine

/// Persists known devices to Application Support/known_devices.json.
/// Immediate save for user-visible changes; debounced flush for RSSI/lastSeen.
final class DeviceStore: ObservableObject {

    @Published var known: [UUID: KnownDevice] = [:]

    private let fileURL: URL
    private var flushTimer: Timer?
    private var isDirty = false
    private var nextChannelIndex = 0

    // MARK: - Init

    init() {
        let appSupport = FileManager.default.urls(for: .applicationSupportDirectory,
                                                  in: .userDomainMask).first!
        let dir = appSupport.appendingPathComponent("bleTones", isDirectory: true)
        try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        fileURL = dir.appendingPathComponent("known_devices.json")
        load()
        startFlushTimer()
    }

    // MARK: - Public API

    /// Returns (or creates) a KnownDevice for the given peripheral UUID.
    @discardableResult
    func ensureKnownDevice(id: UUID, advertisedName: String?) -> KnownDevice {
        if let existing = known[id] { return existing }
        let channel = nextFreeChannel()
        let ccNumber = 20 + (nextChannelIndex % 20)
        nextChannelIndex += 1
        var mapping = DeviceMapping()
        mapping.midiChannel = channel
        mapping.midiCCNumber = ccNumber
        let device = KnownDevice(id: id,
                                 userName: advertisedName ?? "Unnamed",
                                 isSelected: true,
                                 mapping: mapping,
                                 lastRSSI: nil,
                                 lastSeen: nil)
        known[id] = device
        saveNow()
        return device
    }

    func rename(id: UUID, name: String) {
        known[id]?.userName = name
        saveNow()
    }

    func setSelected(id: UUID, selected: Bool) {
        known[id]?.isSelected = selected
        saveNow()
    }

    func updateMapping(id: UUID, mapping: DeviceMapping) {
        known[id]?.mapping = mapping
        saveNow()
    }

    func updateLastSeen(id: UUID, rssi: Int, at date: Date) {
        known[id]?.lastRSSI = rssi
        known[id]?.lastSeen = date
        isDirty = true   // flushed by timer
    }

    // MARK: - Channel assignment

    private func nextFreeChannel() -> Int {
        let used = Set(known.values.map { $0.mapping.midiChannel })
        for ch in 1...16 where !used.contains(ch) { return ch }
        return 1
    }

    // MARK: - Persistence

    private func load() {
        guard FileManager.default.fileExists(atPath: fileURL.path) else { return }
        do {
            let data = try Data(contentsOf: fileURL)
            let decoder = JSONDecoder()
            decoder.dateDecodingStrategy = .iso8601
            let devices = try decoder.decode([KnownDevice].self, from: data)
            for d in devices { known[d.id] = d }
            nextChannelIndex = devices.count
        } catch {
            print("[DeviceStore] load error: \(error)")
        }
    }

    private func saveNow() {
        isDirty = false
        persistToDisk()
    }

    private func persistToDisk() {
        do {
            let encoder = JSONEncoder()
            encoder.dateEncodingStrategy = .iso8601
            encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
            let data = try encoder.encode(Array(known.values))
            try data.write(to: fileURL, options: .atomic)
        } catch {
            print("[DeviceStore] save error: \(error)")
        }
    }

    private func startFlushTimer() {
        flushTimer = Timer.scheduledTimer(withTimeInterval: 5.0, repeats: true) { [weak self] _ in
            guard let self, self.isDirty else { return }
            self.isDirty = false
            self.persistToDisk()
        }
    }

    deinit {
        flushTimer?.invalidate()
        flushTimer = nil
        if isDirty { persistToDisk() }
    }
}
