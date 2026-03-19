import SwiftUI

struct SettingsView: View {
    @EnvironmentObject var midiManager: MIDIManager

    var body: some View {
        NavigationStack {
            Form {
                Section("MIDI") {
                    LabeledContent("Virtual Source", value: "bleTones MIDI Out")

                    Button("Refresh Destinations") {
                        midiManager.refreshDestinations()
                    }

                    ForEach(midiManager.destinations) { dest in
                        LabeledContent(dest.name, value: "ID: \(dest.uniqueID)")
                    }
                }

                Section("About") {
                    LabeledContent("Version", value: "1.0")
                    LabeledContent("Minimum iOS", value: "17.0")
                }
            }
            .navigationTitle("Settings")
        }
    }
}
