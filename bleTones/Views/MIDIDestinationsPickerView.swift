import SwiftUI

struct MIDIDestinationsPickerView: View {
    @Binding var selectedUIDs: [String]
    @EnvironmentObject var midiManager: MIDIManager

    var body: some View {
        List {
            if midiManager.destinations.isEmpty {
                Text("No external MIDI destinations found.")
                    .foregroundStyle(.secondary)
            } else {
                ForEach(midiManager.destinations) { dest in
                    HStack {
                        Text(dest.name)
                        Spacer()
                        if selectedUIDs.contains(dest.uniqueID) {
                            Image(systemName: "checkmark")
                                .foregroundStyle(.blue)
                        }
                    }
                    .contentShape(Rectangle())
                    .onTapGesture {
                        toggle(dest.uniqueID)
                    }
                }
            }
        }
        .navigationTitle("MIDI Destinations")
        .toolbar {
            ToolbarItem(placement: .primaryAction) {
                Button("Refresh") {
                    midiManager.refreshDestinations()
                }
            }
        }
    }

    private func toggle(_ uid: String) {
        if let idx = selectedUIDs.firstIndex(of: uid) {
            selectedUIDs.remove(at: idx)
        } else {
            selectedUIDs.append(uid)
        }
    }
}
