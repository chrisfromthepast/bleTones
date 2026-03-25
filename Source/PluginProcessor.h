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
 * 
 * Voice Types (matching Electron app flavors):
 * - Pad: Slow attack, long sustain (like Enya/Choir)
 * - Pluck: Quick attack, medium decay (like Celtic Harp)
 * - Bell: Quick attack, long shimmering ring (like Vibraphone)
 * - Drone: Very slow attack, very long sustain (ambient)
 * - Flute: Breathy with vibrato (Native Flute style)
 * - Keys: Soft attack, warm piano-like
 * - Guitar: Quick pluck, medium sustain
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
    using juce::AudioProcessor::processBlock;  // Expose double version

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

    //==========================================================================
    // ── Scale definitions (matching the original Electron app) ───────────────

    /** All available musical scales / modes. */
    enum ScaleType
    {
        MinorPentatonic = 0,
        MajorPentatonic,
        NaturalMinor,
        Major,
        Dorian,
        Phrygian,
        Lydian,
        Mixolydian,
        Chromatic,
        HalloweenSpooky,  // Classic spooky diminished/tritone scale
        NumScales
    };

    /** Human-readable names for each scale (in the same order as the enum). */
    static const juce::StringArray& getScaleNames();

    /** Returns the interval array and its length for a given ScaleType. */
    static const int* getScaleIntervals (ScaleType type, int& outLength);

    //==========================================================================
    // ── Ensemble definitions (curated instrument combinations that sound good together) ─

    /** 
     * Ensemble types - each is a curated combination of complementary instruments
     * that work well together musically, preventing fatigue from single-instrument sounds.
     * Based on the original Electron app's flavor system with 3 themed ensembles.
     */
    enum EnsembleType
    {
        EnsembleMelodic = 0,   // Enya-style: Choir Pad + Celtic Harp + Soft String
        EnsembleAmbient,       // Nature: Native Flute + Low Drone + Wind Pad
        EnsembleEthereal,      // Indie/Jazz: Clean Guitar + Warm Keys + Shimmer Chorus
        EnsemblePercussive,    // Mallet: Vibraphone + Marimba + Glockenspiel
        EnsembleChamber,       // Classical: Strings + Harp + Bells
        EnsembleMinimal,       // Simple: Pure sine drones + soft pads
        NumEnsembles
    };

    /** Human-readable names for each ensemble. */
    static const juce::StringArray& getEnsembleNames();

    //==========================================================================
    // ── Individual Instrument definitions (12 distinct instruments) ──────────

    /**
     * Individual instrument types within ensembles.
     * Each has unique oscillator recipes, envelopes, and timbral characteristics.
     */
    enum InstrumentType
    {
        // Melodic ensemble (Enya-inspired)
        InstChoirPad = 0,      // Layered vowel-like pad with formant filtering
        InstCelticHarp,        // Bright plucked harp with ringing harmonics
        InstSoftString,        // Warm sustained string with bowed character
        
        // Ambient ensemble (Native American Flute-inspired)
        InstNativeFlute,       // Breathy sine with vibrato and breath noise
        InstLowDrone,          // Deep sustained tone with sub-octave
        InstWindPad,           // Airy pink noise through resonant filters
        
        // Ethereal ensemble (The Sea and Cake-inspired)
        InstCleanGuitar,       // Plucked clean electric with chorus
        InstWarmKeys,          // Rhodes-like with bell overtone
        InstShimmerChorus,     // Bright shimmery chorus texture
        
        // Percussive ensemble (Mallet instruments)
        InstVibraphone,        // Metallic bar with tremolo
        InstMarimba,           // Warm woody pitched percussion
        InstGlockenspiel,      // Bright bell-like chime
        
        NumInstruments
    };

    /** Human-readable names for each instrument. */
    static const juce::StringArray& getInstrumentNames();

    juce::AudioProcessorValueTreeState apvts;

    /** Returns true if Halloween mode is currently enabled. */
    bool isHalloweenMode() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** Updates reverb parameters based on Halloween mode setting. */
    void updateReverbForHalloweenMode (bool halloween);

    /** Tracks the last Halloween mode state to detect changes. */
    bool lastHalloweenMode { false };

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
        float  attackSec;        // Attack time in seconds
        int    instrumentType;   // InstrumentType enum value
    };

    std::vector<PendingNote> pendingNotes;

    /** Minimum ms between triggers for a single device. */
    static constexpr int kMinStrikeMs = 150;

    /** Generate chord notes for a device based on proximity and movement. */
    void triggerNotesForDevice (const juce::String& id,
                                float normRssi, float delta);

    /** Get the instruments that play together in the current ensemble. */
    std::vector<int> getEnsembleInstruments (int ensembleType) const;

    //==========================================================================
    // ── Polyphonic voice pool ────────────────────────────────────────────────

    struct Voice
    {
        bool   active      { false };
        double phase1      { 0.0 };     // Main oscillator
        double phase2      { 0.0 };     // Detuned/harmonic oscillator
        double phase3      { 0.0 };     // Third oscillator (harmonics)
        double phase4      { 0.0 };     // Fourth oscillator (extra partials)
        double phase5      { 0.0 };     // Fifth oscillator (for choir/shimmer)
        double phaseSub    { 0.0 };     // Sub oscillator (octave below)
        double phaseTrem   { 0.0 };     // Tremolo LFO phase
        double phaseVibrato { 0.0 };    // Vibrato LFO phase
        double frequency   { 440.0 };
        float  amplitude   { 0.0f };    // Peak amplitude
        float  envelope    { 0.0f };    // Current envelope level (0–1)
        float  envDecay    { 0.9999f }; // Per-sample envelope decay
        float  pan         { 0.0f };    // Stereo position (−1…+1)
        
        // Oscillator tuning
        float  detuneRatio1 { 1.003f }; // First detune ratio
        float  detuneRatio2 { 0.997f }; // Second detune ratio
        float  harmonicRatio1 { 2.0f }; // First harmonic ratio
        float  harmonicRatio2 { 3.0f }; // Second harmonic ratio
        float  subMix       { 0.0f };   // Sub oscillator level
        
        // Oscillator mix levels
        float  osc1Mix      { 0.5f };   // Main oscillator mix
        float  osc2Mix      { 0.3f };   // Detuned oscillator mix
        float  osc3Mix      { 0.15f };  // Third oscillator mix
        float  osc4Mix      { 0.0f };   // Fourth oscillator mix
        float  osc5Mix      { 0.0f };   // Fifth oscillator mix
        
        // Filter state
        float  filterState  { 0.0f };   // One-pole LP filter state
        float  filterCoef   { 0.3f };   // LP filter coefficient
        float  filterState2 { 0.0f };   // Second filter for formants
        float  filterCoef2  { 0.5f };   // Formant filter coefficient
        
        // Modulation
        float  tremRate     { 5.0f };   // Tremolo rate Hz
        float  tremDepth    { 0.0f };   // Tremolo depth 0-1
        float  vibratoRate  { 5.0f };   // Vibrato rate Hz
        float  vibratoDepth { 0.0f };   // Vibrato depth in Hz
        
        // ADSR envelope
        enum EnvStage { Attack, Sustain, Release };
        EnvStage envStage   { Attack };
        float  attackRate   { 0.001f }; // Per-sample attack increment
        float  sustainLevel { 1.0f };   // Sustain level (0-1)
        float  sustainTime  { 0.0f };   // Time to hold sustain (samples remaining)
        
        // Instrument type
        int    instrumentType { 0 };    // InstrumentType enum
        
        // Waveform type per oscillator (0=sine, 1=triangle, 2=sawtooth)
        int    waveType1    { 0 };
        int    waveType2    { 0 };
        int    waveType3    { 0 };
        
        // Per-harmonic envelope decay multipliers (for plucked sounds)
        float  harmDecayMult { 1.0f };
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
