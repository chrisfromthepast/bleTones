#pragma once
#include <JuceHeader.h>

//==============================================================================
/** Represents a single BLE device seen by the helper process. */
struct BLEDevice
{
    juce::String name;
    int          rssi       { -100 };
    int          prevRssi   { -100 };   // For RSSI delta (movement) tracking
    juce::int64  lastSeenMs { 0 };
};

//==============================================================================
/**
 * bleTones AudioProcessor
 *
 * Listens for OSC messages from the BLE helper on UDP port 9000.
 * Each message has the pattern:  /ble/rssi  <name:string>  <rssi:int32>
 *
 * Sound generation is driven by **movement** (RSSI changes), not absolute
 * signal strength.  Each device is hashed to a consistent scale degree so
 * it always plays the same pitch.  Closer devices generate richer chords
 * (more notes, wider octave spread) rather than being louder or higher.
 *
 * Up to 32 simultaneous polyphonic voices with attack/decay envelopes,
 * multi-oscillator synthesis, and reverb for a spacious, musical result.
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

    static constexpr int kMinRSSI = -100; // dBm – barely detectable
    static constexpr int kMaxRSSI =  -30; // dBm – very close device

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    juce::OSCReceiver oscReceiver;
    static constexpr int kOSCPort = 9000;

    mutable juce::CriticalSection deviceLock;
    std::vector<BLEDevice>        devices;

    //==========================================================================
    // ── Musical scale & pitch ────────────────────────────────────────────────

    /** Convert a scale degree (can span multiple octaves) to Hz. */
    double degreeToFreq (int degree, int rootMidiNote) const;

    /** Normalise raw RSSI (-100…-30 dBm) to 0.0…1.0. */
    static float normalizeRSSI (int rssi);

    /** Deterministic hash of device name → small int for pitch assignment. */
    static int hashName (const juce::String& name);

    //==========================================================================
    // ── Per-device persistent state (protected by deviceLock) ────────────────

    struct DeviceState
    {
        int         baseDegree      { 0 };  // Hashed scale degree
        juce::int64 lastTriggeredMs { 0 };  // Rate-limit triggers
    };

    std::map<juce::String, DeviceState> deviceStateMap;

    //==========================================================================
    // ── Note event queue (OSC thread → audio thread, under deviceLock) ───────

    struct PendingNote
    {
        double frequency;
        float  amplitude;
        float  pan;
        float  decaySec;
    };

    std::vector<PendingNote> pendingNotes;

    /** Minimum ms between triggers for a single device. */
    static constexpr int kMinStrikeMs = 150;

    /** Generate chord notes for a device based on proximity and movement. */
    void triggerNotesForDevice (const juce::String& id,
                                float normRssi, float delta);

    //==========================================================================
    // ── Polyphonic voice pool ────────────────────────────────────────────────

    struct Voice
    {
        bool   active      { false };
        double phase1      { 0.0 };     // Main sine oscillator
        double phase2      { 0.0 };     // Detuned sine oscillator
        double phaseSub    { 0.0 };     // Sub oscillator (octave below)
        double frequency   { 440.0 };
        float  amplitude   { 0.0f };    // Peak amplitude
        float  envelope    { 0.0f };    // Current envelope level (0–1)
        float  envDecay    { 0.9999f }; // Per-sample envelope decay
        float  pan         { 0.0f };    // Stereo position (−1…+1)
        float  detuneRatio { 1.003f };  // Ratio for the detuned oscillator
        float  subMix      { 0.0f };    // Sub oscillator level (0–1)
        float  filterState { 0.0f };    // One-pole LP filter state
        float  filterCoef  { 0.3f };    // LP filter coefficient
    };

    static constexpr int kMaxVoices = 32;
    std::array<Voice, kMaxVoices> voices;
    double sampleRate { 44100.0 };

    /** Find a free (or quietest) voice slot and initialise it. */
    void activateVoice (const PendingNote& note);

    //==========================================================================
    // ── Reverb ───────────────────────────────────────────────────────────────

    juce::Reverb reverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BLETonesAudioProcessor)
};
