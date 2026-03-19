import SwiftUI

struct DeviceMappingView: View {
    let deviceID: UUID
    @EnvironmentObject var deviceStore: DeviceStore
    @EnvironmentObject var midiManager: MIDIManager

    private var mapping: DeviceMapping {
        deviceStore.known[deviceID]?.mapping ?? DeviceMapping()
    }

    private func update(_ block: (inout DeviceMapping) -> Void) {
        var m = mapping
        block(&m)
        deviceStore.updateMapping(id: deviceID, mapping: m)
    }

    var body: some View {
        Form {
            // MARK: MIDI Channel
            Section("MIDI Channel") {
                Picker("Channel", selection: Binding(
                    get: { mapping.midiChannel },
                    set: { update { $0.midiChannel = $1 } }
                )) {
                    ForEach(1...16, id: \.self) { ch in
                        Text("\(ch)").tag(ch)
                    }
                }
            }

            // MARK: CC Section
            Section("Control Change (CC)") {
                Toggle("Enable CC", isOn: Binding(
                    get: { mapping.midiEnableCC },
                    set: { update { $0.midiEnableCC = $1 } }
                ))
                if mapping.midiEnableCC {
                    Stepper("CC Number: \(mapping.midiCCNumber)",
                            value: Binding(
                                get: { mapping.midiCCNumber },
                                set: { update { $0.midiCCNumber = $1 } }
                            ), in: 0...127)
                }
            }

            // MARK: Notes Section
            Section("Notes") {
                Toggle("Enable Notes", isOn: Binding(
                    get: { mapping.midiEnableNotes },
                    set: { update { $0.midiEnableNotes = $1 } }
                ))
                if mapping.midiEnableNotes {
                    Stepper("Note: \(mapping.midiNoteNumber) (\(noteName(mapping.midiNoteNumber)))",
                            value: Binding(
                                get: { mapping.midiNoteNumber },
                                set: { update { $0.midiNoteNumber = $1 } }
                            ), in: 0...127)

                    HStack {
                        Text("Velocity")
                        Slider(value: Binding(
                            get: { Double(mapping.midiVelocity) },
                            set: { update { $0.midiVelocity = Int($1) } }
                        ), in: 1...127, step: 1)
                        Text("\(mapping.midiVelocity)")
                            .frame(width: 36, alignment: .trailing)
                    }

                    Stepper("Note On Threshold: \(mapping.noteOnThreshold) dBm",
                            value: Binding(
                                get: { mapping.noteOnThreshold },
                                set: { update { $0.noteOnThreshold = $1 } }
                            ), in: -100...0)

                    Stepper("Note Off Threshold: \(mapping.noteOffThreshold) dBm",
                            value: Binding(
                                get: { mapping.noteOffThreshold },
                                set: { update { $0.noteOffThreshold = $1 } }
                            ), in: -100...0)

                    Text("Note triggers when RSSI rises above On threshold and releases when it drops below Off threshold. The gap prevents retriggering.")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }

            // MARK: RSSI Range
            Section("RSSI Range") {
                Stepper("Min: \(mapping.rssiMin) dBm",
                        value: Binding(
                            get: { mapping.rssiMin },
                            set: { update { $0.rssiMin = $1 } }
                        ), in: -120...0)
                Stepper("Max: \(mapping.rssiMax) dBm",
                        value: Binding(
                            get: { mapping.rssiMax },
                            set: { update { $0.rssiMax = $1 } }
                        ), in: -120...0)
                Picker("Curve", selection: Binding(
                    get: { mapping.ccCurve },
                    set: { update { $0.ccCurve = $1 } }
                )) {
                    ForEach(CCCurve.allCases) { curve in
                        Text(curve.rawValue.capitalized).tag(curve)
                    }
                }
            }

            // MARK: Destinations
            Section("MIDI Destinations") {
                Toggle("Send to Virtual Out", isOn: Binding(
                    get: { mapping.midiSendToVirtualOut },
                    set: { update { $0.midiSendToVirtualOut = $1 } }
                ))

                NavigationLink("Select Destinations (\(mapping.midiDestinationUIDs.count))") {
                    MIDIDestinationsPickerView(
                        selectedUIDs: Binding(
                            get: { mapping.midiDestinationUIDs },
                            set: { update { $0.midiDestinationUIDs = $1 } }
                        )
                    )
                }
            }

            // MARK: Local Sound
            Section("Local Sound") {
                Toggle("Enable Local Tone", isOn: Binding(
                    get: { mapping.localSoundEnabled },
                    set: { update { $0.localSoundEnabled = $1 } }
                ))

                if mapping.localSoundEnabled {
                    Picker("Waveform", selection: Binding(
                        get: { mapping.waveform },
                        set: { update { $0.waveform = $1 } }
                    )) {
                        ForEach(Waveform.allCases) { w in
                            Text(w.rawValue.capitalized).tag(w)
                        }
                    }

                    HStack {
                        Text("Gain")
                        Slider(value: Binding(
                            get: { mapping.gain },
                            set: { update { $0.gain = $1 } }
                        ), in: 0...1, step: 0.05)
                        Text(String(format: "%.2f", mapping.gain))
                            .frame(width: 40, alignment: .trailing)
                    }

                    HStack {
                        Text("Pan")
                        Slider(value: Binding(
                            get: { mapping.pan },
                            set: { update { $0.pan = $1 } }
                        ), in: -1...1, step: 0.1)
                        Text(String(format: "%.1f", mapping.pan))
                            .frame(width: 36, alignment: .trailing)
                    }

                    if !mapping.midiEnableNotes {
                        HStack {
                            Text("Base Freq")
                            Slider(value: Binding(
                                get: { mapping.baseFrequencyHz },
                                set: { update { $0.baseFrequencyHz = $1 } }
                            ), in: 20...2000, step: 1)
                            Text("\(Int(mapping.baseFrequencyHz)) Hz")
                                .frame(width: 64, alignment: .trailing)
                        }
                    } else {
                        LabeledContent("Frequency",
                                       value: "\(Int(440.0 * pow(2.0, (Double(mapping.midiNoteNumber) - 69.0) / 12.0))) Hz (from note)")
                    }
                }
            }
        }
        .navigationTitle("Mapping")
    }

    // MARK: - Note name helper

    private func noteName(_ note: Int) -> String {
        let names = ["C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B"]
        let octave = (note / 12) - 1
        let name = names[note % 12]
        return "\(name)\(octave)"
    }
}
