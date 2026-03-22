#pragma once
#include <JuceHeader.h>

//==============================================================================
/** Represents a single BLE device seen by the helper process. */
struct BLEDevice
{
    juce::String name;
    int          rssi       { -100 };
    juce::int64  lastSeenMs { 0 };
};

//==============================================================================
/**
 * bleTones AudioProcessor
 *
 * Listens for OSC messages from the BLE helper on UDP port 9000.
 * Each message has the pattern:  /ble/rssi  <name:string>  <rssi:int32>
 *
 * Received RSSI values are mapped to sine-wave voices:
 *   - RSSI -100 … -30 dBm  →  frequency 110 … 880 Hz
 *   - Amplitude tracks RSSI strength (closer device = louder)
 *
 * Up to 8 simultaneous voices (one per BLE device) are rendered.
 * The plugin also emits MIDI NoteOn/Off for use as a DAW modulation source.
 */
class BLETonesAudioProcessor : public juce::AudioProcessor,
                                public juce::OSCReceiver::Listener<
                                    juce::OSCReceiver::MessageLoopCallback>
{
public:
    BLETonesAudioProcessor();
    ~BLETonesAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;
    bool   acceptsMidi()    const override { return false; }
    bool   producesMidi()   const override { return true; }
    bool   isMidiEffect()   const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==========================================================================
    int  getNumPrograms() override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    /** Called on the message thread when an OSC packet arrives. */
    void oscMessageReceived (const juce::OSCMessage& message) override;

    /** Thread-safe snapshot of the current device table (for the editor). */
    std::vector<BLEDevice> getDevicesCopy() const;

    /** Returns the number of currently active voices (for display). */
    int getActiveVoiceCount() const;

    // RSSI and note range constants used for frequency / velocity mapping
    static constexpr int kMinRSSI     = -100; // dBm – barely detectable
    static constexpr int kMaxRSSI     =  -30; // dBm – very close device
    static constexpr int kMinMIDINote =   36; // C2
    static constexpr int kMaxMIDINote =   96; // C7

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    juce::OSCReceiver oscReceiver;
    static constexpr int kOSCPort = 9000;

    mutable juce::CriticalSection deviceLock;
    std::vector<BLEDevice>        devices;

    //==========================================================================
    /** One sine-wave voice per active BLE device (up to kMaxVoices). */
    struct Voice
    {
        double phase       { 0.0 };
        float  amplitude   { 0.0f };
        float  targetAmp   { 0.0f };
        double frequency   { 440.0 };
        bool   active      { false };
        int    midiNote    { -1 };
    };

    static constexpr int kMaxVoices = 8;
    std::array<Voice, kMaxVoices> voices;
    double sampleRate { 44100.0 };

    //==========================================================================
    /** Map RSSI -100…-30 dBm → MIDI note 36…96, then to Hz. */
    static double rssiToFrequency (int rssi);

    /** Map RSSI -100…-30 dBm → normalised amplitude 0.1…1.0. */
    static float  rssiToVelocity (int rssi);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BLETonesAudioProcessor)
};
