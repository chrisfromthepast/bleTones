import SwiftUI

/// Lists all known (selected) devices and links to their mapping screens.
struct MappingsListView: View {
    @EnvironmentObject var deviceStore: DeviceStore

    private var selectedDevices: [KnownDevice] {
        deviceStore.known.values
            .filter { $0.isSelected }
            .sorted { $0.userName < $1.userName }
    }

    var body: some View {
        NavigationStack {
            List {
                if selectedDevices.isEmpty {
                    Text("No devices selected. Go to the Scan tab to add devices.")
                        .foregroundStyle(.secondary)
                } else {
                    ForEach(selectedDevices) { device in
                        NavigationLink(destination: DeviceMappingView(deviceID: device.id)) {
                            VStack(alignment: .leading, spacing: 2) {
                                Text(device.userName)
                                    .font(.body)
                                HStack(spacing: 12) {
                                    if device.mapping.midiEnableCC {
                                        Label("CC \(device.mapping.midiCCNumber)", systemImage: "slider.horizontal.3")
                                            .font(.caption)
                                    }
                                    if device.mapping.midiEnableNotes {
                                        Label("Note \(device.mapping.midiNoteNumber)", systemImage: "music.note")
                                            .font(.caption)
                                    }
                                    Text("Ch \(device.mapping.midiChannel)")
                                        .font(.caption)
                                        .foregroundStyle(.secondary)
                                }
                            }
                        }
                    }
                }
            }
            .navigationTitle("Mappings")
        }
    }
}
