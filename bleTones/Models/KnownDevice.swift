import Foundation

struct KnownDevice: Codable, Identifiable, Equatable {
    let id: UUID
    var userName: String
    var isSelected: Bool
    var mapping: DeviceMapping
    var lastRSSI: Int?
    var lastSeen: Date?
}
