import Foundation

struct MIDIDestinationInfo: Identifiable, Equatable {
    var id: String { uniqueID }
    let name: String
    let uniqueID: String
    // endpoint ref is resolved at send time, not stored
}
