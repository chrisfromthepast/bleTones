import Foundation
import CoreBluetooth
import Combine

/// Scans for BLE peripherals and maintains a throttled, sorted list for the UI.
final class BLECentral: NSObject, ObservableObject {

    // MARK: Published state

    @Published var state: CBManagerState = .unknown
    @Published var isScanning: Bool = false
    @Published var discoveredList: [DiscoveredListItem] = []

    // MARK: Dependencies (set after init)

    var deviceStore: DeviceStore?

    // MARK: Internal state

    private var central: CBCentralManager!
    private var discovered: [UUID: DiscoveredDevice] = [:]
    private let queue = DispatchQueue(label: "ble.central.queue")
    private var throttleTimer: Timer?
    private var dirtyDiscovered = false

    // MARK: - Init

    override init() {
        super.init()
        central = CBCentralManager(delegate: self, queue: queue)
        startThrottleTimer()
    }

    // MARK: - Scan control

    func startScan() {
        guard central.state == .poweredOn else { return }
        central.scanForPeripherals(
            withServices: nil,
            options: [CBCentralManagerScanOptionAllowDuplicatesKey: true]
        )
        DispatchQueue.main.async { self.isScanning = true }
    }

    func stopScan() {
        central.stopScan()
        DispatchQueue.main.async { self.isScanning = false }
    }

    // MARK: - Public accessors

    /// Thread-safe read of latest discovered device data.
    func latestDiscovered(for id: UUID) -> DiscoveredDevice? {
        queue.sync { discovered[id] }
    }

    // MARK: - UI throttle

    private func startThrottleTimer() {
        throttleTimer = Timer.scheduledTimer(withTimeInterval: 0.2, repeats: true) { [weak self] _ in
            self?.rebuildUIList()
        }
    }

    private func rebuildUIList() {
        guard dirtyDiscovered else { return }
        dirtyDiscovered = false

        let snapshot: [DiscoveredDevice] = queue.sync { Array(discovered.values) }
        let store = deviceStore

        let items: [DiscoveredListItem] = snapshot.map { dev in
            let known = store?.known[dev.id]
            let isKnown = known != nil
            let isSelected = known?.isSelected ?? false
            let displayName = known?.userName
                ?? dev.advertisedName
                ?? "Unknown"
            return DiscoveredListItem(
                id: dev.id,
                displayName: displayName,
                rssi: dev.rssi,
                filteredRSSI: dev.filteredRSSI,
                isKnown: isKnown,
                isSelected: isSelected,
                lastSeen: dev.lastSeen
            )
        }

        let sorted = items.sorted { a, b in
            // 1) selected known first
            if a.isSelected != b.isSelected { return a.isSelected }
            // 2) known before unknown
            if a.isKnown != b.isKnown { return a.isKnown }
            // 3) stronger signal first
            if a.rssi != b.rssi { return a.rssi > b.rssi }
            // 4) most recently seen first
            return a.lastSeen > b.lastSeen
        }

        DispatchQueue.main.async {
            self.discoveredList = sorted
        }
    }
}

// MARK: - CBCentralManagerDelegate

extension BLECentral: CBCentralManagerDelegate {

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        DispatchQueue.main.async {
            self.state = central.state
        }
        if central.state != .poweredOn {
            DispatchQueue.main.async { self.isScanning = false }
        }
    }

    func centralManager(_ central: CBCentralManager,
                         didDiscover peripheral: CBPeripheral,
                         advertisementData: [String: Any],
                         rssi RSSI: NSNumber) {
        let id = peripheral.identifier
        let name = advertisementData[CBAdvertisementDataLocalNameKey] as? String
        let rawRSSI = RSSI.intValue

        // Ignore anomalous readings
        guard rawRSSI != 127 else { return }

        if var existing = discovered[id] {
            if let name { existing.advertisedName = name }
            existing.rssi = rawRSSI
            existing.filteredRSSI = existing.filteredRSSI + 0.2 * (Double(rawRSSI) - existing.filteredRSSI)
            existing.lastSeen = Date()
            discovered[id] = existing
        } else {
            discovered[id] = DiscoveredDevice(
                id: id,
                advertisedName: name,
                rssi: rawRSSI,
                filteredRSSI: Double(rawRSSI),
                lastSeen: Date()
            )
        }
        dirtyDiscovered = true
    }
}
