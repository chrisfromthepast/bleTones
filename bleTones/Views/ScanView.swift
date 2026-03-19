import SwiftUI
import CoreBluetooth

struct ScanView: View {
    @EnvironmentObject var bleCentral: BLECentral
    @EnvironmentObject var deviceStore: DeviceStore

    var body: some View {
        NavigationStack {
            VStack(spacing: 0) {
                // Bluetooth state banner
                if bleCentral.state != .poweredOn {
                    bluetoothBanner
                }

                List {
                    ForEach(bleCentral.discoveredList) { item in
                        NavigationLink(destination: DeviceDetailView(deviceID: item.id)) {
                            ScanRowView(item: item)
                        }
                    }
                }
                .listStyle(.plain)
            }
            .navigationTitle("Scan")
            .toolbar {
                ToolbarItem(placement: .primaryAction) {
                    Button {
                        if bleCentral.isScanning {
                            bleCentral.stopScan()
                        } else {
                            bleCentral.startScan()
                        }
                    } label: {
                        Text(bleCentral.isScanning ? "Stop" : "Start")
                    }
                    .disabled(bleCentral.state != .poweredOn)
                }
            }
        }
    }

    private var bluetoothBanner: some View {
        HStack {
            Image(systemName: "exclamationmark.triangle.fill")
                .foregroundStyle(.yellow)
            Text(bluetoothStateMessage)
                .font(.footnote)
        }
        .padding(8)
        .frame(maxWidth: .infinity)
        .background(Color(.systemGray5))
    }

    private var bluetoothStateMessage: String {
        switch bleCentral.state {
        case .poweredOff: return "Bluetooth is turned off."
        case .unauthorized: return "Bluetooth permission denied."
        case .unsupported: return "Bluetooth not supported on this device."
        default: return "Bluetooth is not available."
        }
    }
}

// MARK: - Row

struct ScanRowView: View {
    let item: DiscoveredListItem
    @EnvironmentObject var deviceStore: DeviceStore

    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 2) {
                Text(item.displayName)
                    .font(.body)
                    .fontWeight(item.isSelected ? .semibold : .regular)
                Text("RSSI: \(item.rssi) dBm")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Spacer()

            Toggle("", isOn: Binding(
                get: { item.isSelected },
                set: { newValue in
                    if newValue {
                        deviceStore.ensureKnownDevice(id: item.id,
                                                      advertisedName: item.displayName)
                    }
                    deviceStore.setSelected(id: item.id, selected: newValue)
                }
            ))
            .labelsHidden()
        }
        .padding(.vertical, 4)
    }
}
