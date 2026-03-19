import SwiftUI

struct DeviceDetailView: View {
    let deviceID: UUID
    @EnvironmentObject var deviceStore: DeviceStore
    @State private var editedName: String = ""

    private var device: KnownDevice? {
        deviceStore.known[deviceID]
    }

    var body: some View {
        Form {
            if let device {
                Section("Name") {
                    TextField("Device name", text: $editedName)
                        .onSubmit {
                            let trimmed = editedName.trimmingCharacters(in: .whitespaces)
                            if !trimmed.isEmpty {
                                deviceStore.rename(id: deviceID, name: trimmed)
                            }
                        }
                        .onAppear { editedName = device.userName }
                }

                Section("Status") {
                    Toggle("Selected", isOn: Binding(
                        get: { device.isSelected },
                        set: { deviceStore.setSelected(id: deviceID, selected: $0) }
                    ))

                    if let rssi = device.lastRSSI {
                        LabeledContent("Last RSSI", value: "\(rssi) dBm")
                    }
                    if let seen = device.lastSeen {
                        LabeledContent("Last Seen", value: seen, format: .relative(presentation: .named))
                    }
                }

                Section {
                    NavigationLink("MIDI & Audio Mapping") {
                        DeviceMappingView(deviceID: deviceID)
                    }
                }
            } else {
                Section {
                    Text("Device not yet known. Select it from the scan list first.")
                        .foregroundStyle(.secondary)
                }
            }
        }
        .navigationTitle(device?.userName ?? "Device")
    }
}
