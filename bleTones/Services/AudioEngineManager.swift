import Foundation
import AVFoundation

/// Per-device synth voice state used by the audio render callback.
struct SynthVoice {
    var frequency: Double = 220
    var targetAmplitude: Double = 0
    var currentAmplitude: Double = 0
    var pan: Double = 0        // -1 … 1
    var waveform: Waveform = .sine
    var phase: Double = 0
}

/// Manages an AVAudioEngine with a source node that mixes all active voices.
final class AudioEngineManager: ObservableObject {

    private let engine = AVAudioEngine()
    private var sourceNode: AVAudioSourceNode?
    private let sampleRate: Double = 44100

    /// Voices keyed by device UUID. Mutated from main thread, read from audio thread.
    /// Access is protected by a lock for thread safety.
    private var voices: [UUID: SynthVoice] = [:]
    private let voiceLock = NSLock()

    // MARK: - Init

    init() {
        setupAudioSession()
        setupEngine()
    }

    // MARK: - Audio session

    private func setupAudioSession() {
        #if os(iOS)
        let session = AVAudioSession.sharedInstance()
        do {
            try session.setCategory(.playback, options: .mixWithOthers)
            try session.setActive(true)
        } catch {
            print("[AudioEngineManager] session error: \(error)")
        }
        #endif
    }

    // MARK: - Engine setup

    private func setupEngine() {
        let format = AVAudioFormat(standardFormatWithSampleRate: sampleRate, channels: 2)!

        sourceNode = AVAudioSourceNode(format: format) { [weak self] _, _, frameCount, bufferList -> OSStatus in
            guard let self else { return noErr }
            let ablPointer = UnsafeMutableAudioBufferListPointer(bufferList)
            guard ablPointer.count >= 2 else { return noErr }

            let leftBuf = ablPointer[0].mData!.assumingMemoryBound(to: Float.self)
            let rightBuf = ablPointer[1].mData!.assumingMemoryBound(to: Float.self)
            let frames = Int(frameCount)

            // Zero buffers
            for i in 0..<frames {
                leftBuf[i] = 0
                rightBuf[i] = 0
            }

            self.voiceLock.lock()
            var voicesCopy = self.voices
            self.voiceLock.unlock()

            for (id, var voice) in voicesCopy {
                let phaseInc = voice.frequency / self.sampleRate
                let smoothing = 0.002  // amplitude smoothing per sample

                for i in 0..<frames {
                    // Smooth amplitude
                    let diff = voice.targetAmplitude - voice.currentAmplitude
                    voice.currentAmplitude += diff * smoothing

                    // Generate sample
                    let sample: Float
                    switch voice.waveform {
                    case .sine:
                        sample = Float(sin(voice.phase * 2.0 * .pi) * voice.currentAmplitude)
                    case .triangle:
                        let t = voice.phase - floor(voice.phase)
                        let tri = 4.0 * abs(t - 0.5) - 1.0
                        sample = Float(tri * voice.currentAmplitude)
                    }

                    // Stereo pan (constant power approximation)
                    let panNorm = (voice.pan + 1.0) / 2.0  // 0…1
                    let leftGain = Float(cos(panNorm * .pi / 2.0))
                    let rightGain = Float(sin(panNorm * .pi / 2.0))

                    leftBuf[i] += sample * leftGain
                    rightBuf[i] += sample * rightGain

                    voice.phase += phaseInc
                    if voice.phase >= 1.0 { voice.phase -= 1.0 }
                }

                voicesCopy[id] = voice
            }

            // Write back updated phases and amplitudes
            self.voiceLock.lock()
            for (id, voice) in voicesCopy {
                self.voices[id]?.phase = voice.phase
                self.voices[id]?.currentAmplitude = voice.currentAmplitude
            }
            self.voiceLock.unlock()

            return noErr
        }

        engine.attach(sourceNode!)
        engine.connect(sourceNode!, to: engine.mainMixerNode, format: format)

        do {
            try engine.start()
        } catch {
            print("[AudioEngineManager] engine start error: \(error)")
        }
    }

    // MARK: - Voice control

    /// Called from UpdateCoordinator on each tick for each active device.
    func setVoice(id: UUID, freq: Double, targetAmp: Double, pan: Double, waveform: Waveform) {
        voiceLock.lock()
        if var voice = voices[id] {
            voice.frequency = freq
            voice.targetAmplitude = targetAmp
            voice.pan = pan
            voice.waveform = waveform
            voices[id] = voice
        } else {
            voices[id] = SynthVoice(frequency: freq,
                                    targetAmplitude: targetAmp,
                                    currentAmplitude: 0,
                                    pan: pan,
                                    waveform: waveform,
                                    phase: 0)
        }
        voiceLock.unlock()
    }

    /// Remove a voice (device no longer active).
    func removeVoice(id: UUID) {
        voiceLock.lock()
        voices.removeValue(forKey: id)
        voiceLock.unlock()
    }
}
