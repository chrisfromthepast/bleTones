import SwiftUI

@main
struct bleTonessApp: App {
    @StateObject private var deviceStore = DeviceStore()
    @StateObject private var bleCentral = BLECentral()
    @StateObject private var midiManager = MIDIManager()
    @StateObject private var audioEngine = AudioEngineManager()
    @State private var coordinator: UpdateCoordinator?

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(deviceStore)
                .environmentObject(bleCentral)
                .environmentObject(midiManager)
                .environmentObject(audioEngine)
                .onAppear {
                    bleCentral.deviceStore = deviceStore
                    if coordinator == nil {
                        coordinator = UpdateCoordinator(
                            deviceStore: deviceStore,
                            bleCentral: bleCentral,
                            midiManager: midiManager,
                            audioEngine: audioEngine
                        )
                    }
                }
        }
    }
}
