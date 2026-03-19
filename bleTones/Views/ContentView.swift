import SwiftUI

struct ContentView: View {
    var body: some View {
        TabView {
            ScanView()
                .tabItem {
                    Label("Scan", systemImage: "antenna.radiowaves.left.and.right")
                }

            MappingsListView()
                .tabItem {
                    Label("Mappings", systemImage: "slider.horizontal.3")
                }

            SettingsView()
                .tabItem {
                    Label("Settings", systemImage: "gear")
                }
        }
    }
}
