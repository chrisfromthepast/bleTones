import Foundation

/// Internal mutable record kept by BLECentral for every peripheral seen during scanning.
struct DiscoveredDevice {
    let id: UUID
    var advertisedName: String?
    var rssi: Int
    var filteredRSSI: Double
    var lastSeen: Date
}

/// Lightweight value shown in the SwiftUI list – sorted and throttled.
struct DiscoveredListItem: Identifiable, Equatable {
    let id: UUID
    var displayName: String   // userName if known, else advertised name, else "Unknown"
    var rssi: Int
    var filteredRSSI: Double
    var isKnown: Bool
    var isSelected: Bool
    var lastSeen: Date
}
