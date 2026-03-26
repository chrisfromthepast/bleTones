#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
// Scale interval tables – matching the Electron app's scales object.
// Each array contains semitone offsets from the root within a single octave.
//==============================================================================
static constexpr int kMinorPentatonic[] = { 0, 2, 3, 7, 9 };
static constexpr int kMajorPentatonic[] = { 0, 2, 4, 7, 9 };
static constexpr int kNaturalMinor[]    = { 0, 2, 3, 5, 7, 8, 10 };
static constexpr int kMajorScale[]      = { 0, 2, 4, 5, 7, 9, 11 };
static constexpr int kDorian[]          = { 0, 2, 3, 5, 7, 9, 10 };
static constexpr int kPhrygian[]        = { 0, 1, 3, 5, 7, 8, 10 };
static constexpr int kLydian[]          = { 0, 2, 4, 6, 7, 9, 11 };
static constexpr int kMixolydian[]      = { 0, 2, 4, 5, 7, 9, 10 };
static constexpr int kChromatic[]       = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
// Halloween Spooky scale – uses diminished/tritone intervals for eerie sound
// Classic horror movie intervals: minor 2nd, tritone, minor 6th create tension
static constexpr int kHalloweenSpooky[] = { 0, 1, 3, 6, 8, 9, 11 };

struct ScaleInfo
{
    const int* intervals;
    int        length;
};

static constexpr ScaleInfo kAllScales[] =
{
    { kMinorPentatonic, 5  },
    { kMajorPentatonic, 5  },
    { kNaturalMinor,    7  },
    { kMajorScale,      7  },
    { kDorian,          7  },
    { kPhrygian,        7  },
    { kLydian,          7  },
    { kMixolydian,      7  },
    { kChromatic,       12 },
    { kHalloweenSpooky, 7  },
};

const juce::StringArray& BLETonesAudioProcessor::getScaleNames()
{
    static const juce::StringArray names
    {
        "Minor Pentatonic",
        "Major Pentatonic",
        "Natural Minor",
        "Major",
        "Dorian",
        "Phrygian",
        "Lydian",
        "Mixolydian",
        "Chromatic",
        "Halloween Spooky"
    };
    return names;
}

const int* BLETonesAudioProcessor::getScaleIntervals (ScaleType type, int& outLength)
{
    const int idx = juce::jlimit (0, (int) NumScales - 1, (int) type);
    outLength = kAllScales[idx].length;
    return kAllScales[idx].intervals;
}

//==============================================================================
// Sound-generation tuning constants
//==============================================================================
static constexpr int   kFallbackScaleLen  = 5;      // Fallback scale length (Minor Pentatonic) if lookup fails
static constexpr int   kBaseDegreeOctaves = 3;      // Hash range spans this many scale-octaves
static constexpr float kDeltaToAmpScale   = 6.0f;   // Movement delta → amplitude scaling
static constexpr int   kPanHashRange      = 140;     // Hash range for pan (mapped to −0.7…+0.7)
static constexpr float kPanHashOffset     = 70.0f;   // Centre offset for pan hash
static constexpr float kInitialDelta      = 0.15f;   // Synthetic delta for first-sighting trigger
static constexpr int   kHeartbeatMs       = 3000;    // Trigger a soft note if idle for this long
static constexpr float kHeartbeatAmpFactor= 0.6f;    // Heartbeat notes are softer (60% of initial)

// Named constants for harmonic decay multipliers (used in activateVoice)
static constexpr float kHarmDecayHarp      = 2.5f;  // Celtic Harp: fast harmonic decay
static constexpr float kHarmDecayGuitar    = 3.0f;  // Clean Guitar: very fast decay
static constexpr float kHarmDecayKeys      = 4.0f;  // Warm Keys: fastest for bell overtone

//==============================================================================
// Voice Type names – matching the Electron app's instrument flavors
//==============================================================================
const juce::StringArray& BLETonesAudioProcessor::getEnsembleNames()
{
    static const juce::StringArray names
    {
        "Choral",    // Choir Pad + Celtic Harp + Soft String
        "Airy",      // Native Flute + Low Drone + Wind Pad
        "Shimmer",   // Clean Guitar + Warm Keys + Shimmer Chorus
        "Bells",     // Vibraphone + Marimba + Glockenspiel
        "Strings",   // Soft String + Harp + Vibraphone
        "Drone"      // Choir Pad + Low Drone
    };
    return names;
}

const juce::StringArray& BLETonesAudioProcessor::getInstrumentNames()
{
    static const juce::StringArray names
    {
        // Melodic ensemble
        "Choir Pad",       // Layered vowel-like pad
        "Celtic Harp",     // Bright plucked harp
        "Soft String",     // Warm bowed string
        // Ambient ensemble
        "Native Flute",    // Breathy with vibrato
        "Low Drone",       // Deep sustained tone
        "Wind Pad",        // Airy wind texture
        // Ethereal ensemble
        "Clean Guitar",    // Plucked clean electric
        "Warm Keys",       // Rhodes-like piano
        "Shimmer Chorus",  // Bright shimmery texture
        // Percussive ensemble
        "Vibraphone",      // Metallic with tremolo
        "Marimba",         // Warm woody percussion
        "Glockenspiel"     // Bright bell-like chime
    };
    return names;
}

std::vector<int> BLETonesAudioProcessor::getEnsembleInstruments (int ensembleType) const
{
    // Each ensemble returns 2-3 complementary instruments that play together
    switch (static_cast<EnsembleType> (ensembleType))
    {
        case EnsembleMelodic:
            return { InstChoirPad, InstCelticHarp, InstSoftString };
        case EnsembleAmbient:
            return { InstNativeFlute, InstLowDrone, InstWindPad };
        case EnsembleEthereal:
            return { InstCleanGuitar, InstWarmKeys, InstShimmerChorus };
        case EnsemblePercussive:
            return { InstVibraphone, InstMarimba, InstGlockenspiel };
        case EnsembleChamber:
            return { InstSoftString, InstCelticHarp, InstVibraphone };
        case EnsembleMinimal:
        case NumEnsembles:
        default:
            return { InstChoirPad, InstLowDrone };
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
    BLETonesAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "volume",
        "Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.7f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "sensitivity",
        "BLE Sensitivity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.5f));

    layout.add (std::make_unique<juce::AudioParameterInt> (
        "rootNote",
        "Root Note (MIDI)",
        0, 127, 60));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "scaleType",
        "Scale / Mode",
        getScaleNames(),
        0));  // Default: Minor Pentatonic

    layout.add (std::make_unique<juce::AudioParameterBool> (
        "halloweenMode",
        "Halloween Mode",
        false));  // Default: off

    // Ensemble type selection - curated combinations of complementary instruments
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "ensembleType",
        "Ensemble",
        getEnsembleNames(),
        0));  // Default: Choral

    // Attack time control (0.01s to 2.0s) - allows user to control how fast notes fade in
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "attackTime",
        "Attack Time",
        juce::NormalisableRange<float> (0.01f, 2.0f, 0.01f, 0.5f),  // Skewed for fine control at low end
        0.4f));  // Default: 400ms (matching Electron's Pad voice)

    // Release/decay time control (0.5s to 12.0s) - longer values = more ambient, less staccato
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "releaseTime",
        "Release Time",
        juce::NormalisableRange<float> (0.5f, 12.0f, 0.1f, 0.5f),
        6.0f));  // Default: 6.0s (matching Electron's Pad voice)

    return layout;
}

//==============================================================================
BLETonesAudioProcessor::BLETonesAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    if (! oscReceiver.connect (kOSCPort))
    {
        DBG ("bleTones: failed to bind OSC receiver on port " << kOSCPort);
    }

    oscReceiver.addListener (this);

#if JucePlugin_Build_Standalone && JUCE_MAC
    {
        const juce::File helperApp =
            juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                .getParentDirectory()
                .getParentDirectory()
                .getChildFile ("Resources")
                .getChildFile ("bleTones_helper.app");

        if (helperApp.exists())
        {
            juce::StringArray args;
            args.add ("/usr/bin/open");
            args.add ("-g");
            args.add (helperApp.getFullPathName());

            juce::ChildProcess launcher;
            if (! launcher.start (args))
                DBG ("bleTones: failed to launch helper at " << helperApp.getFullPathName());
        }
        else
        {
            DBG ("bleTones: helper not found at " << helperApp.getFullPathName());
        }
    }
#endif
}

BLETonesAudioProcessor::~BLETonesAudioProcessor()
{
    oscReceiver.removeListener (this);
    oscReceiver.disconnect();
}

//==============================================================================
const juce::String BLETonesAudioProcessor::getName() const { return "bleTones"; }

//==============================================================================
void BLETonesAudioProcessor::prepareToPlay (double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;

    for (auto& v : voices)
        v = {};

    // Configure reverb - will be updated in processBlock based on Halloween mode
    updateReverbForHalloweenMode (*apvts.getRawParameterValue ("halloweenMode") > 0.5f);
}

void BLETonesAudioProcessor::updateReverbForHalloweenMode (bool halloween)
{
    juce::Reverb::Parameters rp;
    if (halloween)
    {
        // Spooky reverb: larger cavernous room, more echo, ethereal wet signal
        rp.roomSize   = 0.92f;   // Very large haunted space
        rp.damping    = 0.25f;   // Less damping = more echo
        rp.wetLevel   = 0.50f;   // More reverb for ghostly effect
        rp.dryLevel   = 0.50f;
        rp.width      = 1.0f;
        rp.freezeMode = 0.0f;
    }
    else
    {
        // Normal spacious ambient sound
        rp.roomSize   = 0.75f;
        rp.damping    = 0.4f;
        rp.wetLevel   = 0.30f;
        rp.dryLevel   = 0.70f;
        rp.width      = 1.0f;
        rp.freezeMode = 0.0f;
    }
    reverb.setParameters (rp);
    reverb.reset();  // Clear internal state to avoid audio artifacts when switching modes
}

void BLETonesAudioProcessor::releaseResources() {}

//==============================================================================
bool BLETonesAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
}

//==============================================================================
// Musical helpers
//==============================================================================

float BLETonesAudioProcessor::normalizeRSSI (int rssi)
{
    // -100 dBm → 0.0,  -30 dBm → 1.0
    return juce::jlimit (0.0f, 1.0f, (static_cast<float> (rssi) + 100.0f) / 70.0f);
}

int BLETonesAudioProcessor::hashName (const juce::String& name)
{
    // Simple hash to get a consistent small integer from a device identifier
    unsigned int h = 0;
    for (int i = 0; i < name.length(); ++i)
    {
        h = ((h << 5) - h) + static_cast<unsigned int> (name[i]);
    }
    return static_cast<int> (h % 32);  // 0–31 range
}

double BLETonesAudioProcessor::degreeToFreq (int degree, int rootMidiNote) const
{
    // Look up the currently selected scale
    const auto scaleIdx = static_cast<ScaleType> (
        static_cast<int> (*apvts.getRawParameterValue ("scaleType")));
    int scaleLen = 0;
    const int* scale = getScaleIntervals (scaleIdx, scaleLen);

    if (scaleLen <= 0) { scaleLen = kFallbackScaleLen; }  // Safety guard

    const double rootHz = 440.0 * std::pow (2.0, (rootMidiNote - 69) / 12.0);
    const int octave      = degree / scaleLen;
    // Normalise modulo to [0, scaleLen) even for negative degrees
    const int withinScale = ((degree % scaleLen) + scaleLen) % scaleLen;
    const int semitones   = scale[withinScale] + octave * 12;
    return rootHz * std::pow (2.0, semitones / 12.0);
}

//==============================================================================
// OSC message → movement detection → chord triggering
//==============================================================================

void BLETonesAudioProcessor::oscMessageReceived (const juce::OSCMessage& msg)
{
    if (msg.getAddressPattern().toString() != "/ble/rssi")
        return;

    if (msg.size() < 2 || ! (msg[0].isString() && msg[1].isInt32()))
        return;

    const juce::String name = msg[0].getString();
    const int          rssi = msg[1].getInt32();
    const juce::int64  now  = juce::Time::currentTimeMillis();

    const float normRssi = normalizeRSSI (rssi);

    {
        juce::ScopedLock lock (deviceLock);

        // ── Update or insert device ──────────────────────────────────────────
        bool found = false;
        float delta = 0.0f;

        for (auto& d : devices)
        {
            if (d.name == name)
            {
                delta       = std::abs (normRssi - normalizeRSSI (d.rssi));
                d.prevRssi  = d.rssi;
                d.rssi      = rssi;
                d.lastSeenMs = now;
                found       = true;
                break;
            }
        }

        bool isNewDevice = false;
        if (! found)
        {
            devices.push_back ({ name, rssi, rssi, now });
            // First sighting – initialise device state
            if (deviceStateMap.find (name) == deviceStateMap.end())
            {
                // Hash device name to a base scale degree spanning a few octaves.
                // The range adapts to the current scale length.
                const auto scaleIdx = static_cast<ScaleType> (
                    static_cast<int> (*apvts.getRawParameterValue ("scaleType")));
                int scaleLen = 0;
                getScaleIntervals (scaleIdx, scaleLen);
                if (scaleLen <= 0) { scaleLen = kFallbackScaleLen; }

                DeviceState ds;
                ds.baseDegree = hashName (name) % (scaleLen * kBaseDegreeOctaves);
                deviceStateMap[name] = ds;
                isNewDevice = true;
            }
            // Use a synthetic delta for new devices to trigger an intro note
            delta = kInitialDelta;
        }

        // Evict devices not seen for more than 10 seconds
        devices.erase (
            std::remove_if (devices.begin(), devices.end(),
                [now] (const BLEDevice& d) { return (now - d.lastSeenMs) > 10'000; }),
            devices.end());

        // ── Movement detection ───────────────────────────────────────────────
        // The sensitivity parameter controls the movement threshold:
        //   sensitivity 0.0 → threshold 0.30 (very insensitive)
        //   sensitivity 1.0 → threshold 0.01 (very sensitive)
        const float sensitivity = *apvts.getRawParameterValue ("sensitivity");
        const float threshold   = 0.30f - sensitivity * 0.29f;

        auto& ds = deviceStateMap[name];

        // Determine if we should trigger:
        // 1. New device → always trigger with initial delta
        // 2. Movement detected → trigger if delta exceeds threshold
        // 3. Heartbeat → trigger a soft note if device has been idle too long (only if no movement)
        const bool movementTrigger = (delta > threshold) && (now - ds.lastTriggeredMs) > kMinStrikeMs;
        // Heartbeat only fires if there's no movement trigger (to avoid duplicates)
        const bool heartbeatTrigger = !movementTrigger && (now - ds.lastTriggeredMs) > kHeartbeatMs;

        if (isNewDevice || movementTrigger || heartbeatTrigger)
        {
            // Use a softer delta for heartbeat to create ambient background
            const float effectiveDelta = heartbeatTrigger && !isNewDevice
                                       ? kInitialDelta * kHeartbeatAmpFactor
                                       : delta;
            triggerNotesForDevice (name, normRssi, effectiveDelta);
            ds.lastTriggeredMs = now;
        }
    }
}

//==============================================================================
// Generate chord notes based on proximity and movement
//==============================================================================

void BLETonesAudioProcessor::triggerNotesForDevice (const juce::String& id,
                                                     float normRssi,
                                                     float delta)
{
    // Called under deviceLock

    const int rootNote = static_cast<int> (*apvts.getRawParameterValue ("rootNote"));
    auto&     ds       = deviceStateMap[id];
    const int baseDeg  = ds.baseDegree;
    
    // Get ensemble type and envelope parameters from plugin state
    const int ensembleType = static_cast<int> (*apvts.getRawParameterValue ("ensembleType"));
    const float userAttack = *apvts.getRawParameterValue ("attackTime");
    const float userRelease = *apvts.getRawParameterValue ("releaseTime");
    
    // Get the instruments in this ensemble
    const auto instruments = getEnsembleInstruments (ensembleType);

    // ── Number of chord notes: proximity drives richness ─────────────────
    //    Far (low RSSI):  1 note
    //    Medium:          2 notes
    //    Close:           3–4 notes spread across octaves
    int numNotes;
    if (normRssi < 0.3f)
        numNotes = 1;
    else if (normRssi < 0.55f)
        numNotes = 2;
    else if (normRssi < 0.75f)
        numNotes = 3;
    else
        numNotes = 4;

    // ── Amplitude: driven by movement delta, NOT proximity ───────────────
    //    More movement = louder, capped at 0.8 to stay pleasant
    const float baseAmp = juce::jlimit (0.15f, 0.8f, delta * kDeltaToAmpScale);

    // ── Decay time: use user's release time setting, modulated by proximity
    //    Close devices ring longer for more sonic presence
    const float baseDecay = userRelease * (0.6f + normRssi * 0.6f);

    // ── Octave spread: close devices spread wider ────────────────────────
    //    Far:   all in same octave
    //    Close: notes span −1 to +1 octave offsets
    const int octaveSpread = (normRssi > 0.6f) ? 2 : (normRssi > 0.35f ? 1 : 0);

    // ── Generate chord notes ─────────────────────────────────────────────
    // Scale degree offsets for chord tones (root, 3rd, 5th, extended)
    static constexpr int chordOffsets[] = { 0, 2, 4, 7 };
    // Octave offset pattern for spread
    static constexpr int octavePattern[] = { 0, 1, -1, 1 };

    // Look up current scale length for octave jumps
    const auto scaleIdx = static_cast<ScaleType> (
        static_cast<int> (*apvts.getRawParameterValue ("scaleType")));
    int scaleLen = 0;
    getScaleIntervals (scaleIdx, scaleLen);
    if (scaleLen <= 0) { scaleLen = kFallbackScaleLen; }

    // For each chord note, trigger voices from multiple instruments in the ensemble
    for (int i = 0; i < numNotes; ++i)
    {
        const int degree = baseDeg + chordOffsets[i];
        int octOff = 0;
        if (octaveSpread > 0 && i > 0)
        {
            octOff = octavePattern[i % 4];
            if (octaveSpread < 2)
                octOff = std::max (octOff, 0); // Only spread upward for medium
        }

        const double freq = degreeToFreq (degree + octOff * scaleLen, rootNote);

        // Panning via hash for consistent spatial placement (range ≈ −0.7…+0.7)
        const float basePan = ((float) (hashName (id + juce::String (i)) % kPanHashRange)
                              - kPanHashOffset) / 100.0f;
        
        // Each ensemble instrument plays this note with slightly different characteristics
        // This creates a rich, layered sound instead of one-note fatigue
        for (size_t instIdx = 0; instIdx < instruments.size(); ++instIdx)
        {
            const int instType = instruments[instIdx];
            
            // Amplitude varies by instrument position in ensemble
            // Primary instrument (first) is loudest, others complement
            float instAmpMult = 1.0f;
            if (instIdx == 1) instAmpMult = 0.6f;       // Second instrument softer
            else if (instIdx == 2) instAmpMult = 0.4f;  // Third instrument softest
            
            // Taper amplitude for upper chord tones
            const float noteAmp = baseAmp * instAmpMult * (1.0f - (float) i * 0.12f);
            
            // Slight pan spread across instruments for width
            const float panOffset = (float) instIdx * 0.15f - 0.15f;
            const float pan = juce::jlimit (-0.9f, 0.9f, basePan + panOffset);
            
            // Slightly different decay per instrument for organic layering
            const float decay = baseDecay + (float) instIdx * 0.8f + (float) i * 0.3f;
            
            // Stagger attack times slightly for a more natural ensemble feel
            const float attack = userAttack * (1.0f + (float) instIdx * 0.15f + (float) i * 0.05f);

            pendingNotes.push_back ({ freq, noteAmp, pan, decay, attack, instType });
        }
    }
}

//==============================================================================
// Voice allocation
//==============================================================================

void BLETonesAudioProcessor::activateVoice (const PendingNote& note)
{
    // Find a free voice, or steal the quietest one
    size_t bestIdx  = 0;
    float quietest  = 2.0f;

    for (size_t i = 0; i < kMaxVoices; ++i)
    {
        if (! voices[i].active)
        {
            bestIdx = i;
            break;
        }

        const float level = voices[i].envelope * voices[i].amplitude;
        if (level < quietest)
        {
            quietest = level;
            bestIdx  = i;
        }
    }

    const bool halloween = *apvts.getRawParameterValue ("halloweenMode") > 0.5f;

    auto& v        = voices[bestIdx];
    v.active       = true;
    v.phase1       = 0.0;
    v.phase2       = 0.0;
    v.phase3       = 0.0;
    v.phase4       = 0.0;
    v.phase5       = 0.0;
    v.phaseSub     = 0.0;
    v.phaseTrem    = 0.0;
    v.phaseVibrato = 0.0;
    v.frequency    = note.frequency;
    v.amplitude    = note.amplitude;
    v.pan          = note.pan;
    v.instrumentType = note.instrumentType;
    
    // Reset modulation
    v.vibratoDepth = 0.0f;
    v.tremDepth = halloween ? 0.15f : 0.0f;
    v.tremRate = halloween ? 4.5f : 0.0f;
    
    // ── Set up ADSR envelope based on instrument type ────────────────────────
    v.envStage = Voice::Attack;
    v.envelope = 0.0f;
    
    // Calculate attack rate (per-sample increment to reach 1.0)
    const double attackSamples = sampleRate * (double) note.attackSec;
    v.attackRate = (attackSamples > 0.0)
        ? static_cast<float> (1.0 / attackSamples)
        : 1.0f;
    
    // ── Configure each instrument type with its unique characteristics ───────
    // Based on the original Electron app's Web Audio implementations
    float sustainTimeSec = 0.0f;
    
    // Set defaults
    v.waveType1 = 0;  // sine
    v.waveType2 = 0;  // sine
    v.waveType3 = 0;  // sine
    v.harmDecayMult = 1.0f;
    v.filterCoef2 = 0.5f;
    v.filterState2 = 0.0f;
    
    const int freqHash = hashName (juce::String (note.frequency));
    
    switch (static_cast<InstrumentType> (note.instrumentType))
    {
        // ══════════════════════════════════════════════════════════════════════
        // MELODIC ENSEMBLE (Enya-inspired)
        // ══════════════════════════════════════════════════════════════════════
        
        case InstChoirPad:
            // Layered vowel-like pad with formant filtering
            // Multiple detuned oscillators for thick choir effect
            // Detunes: 0, +3, -3, +7, -7 cents for chorus
            v.sustainLevel = 0.85f;
            sustainTimeSec = note.decaySec * 0.5f;
            v.waveType1 = 0;  // sine
            v.waveType2 = 1;  // triangle
            v.waveType3 = 0;  // sine
            v.detuneRatio1 = 1.0f + 3.0f / 1200.0f;   // +3 cents
            v.detuneRatio2 = 1.0f - 3.0f / 1200.0f;   // -3 cents
            v.harmonicRatio1 = 1.0f + 7.0f / 1200.0f; // +7 cents
            v.harmonicRatio2 = 1.0f - 7.0f / 1200.0f; // -7 cents
            v.osc1Mix = 0.30f;  // Main
            v.osc2Mix = 0.25f;  // Detuned triangle
            v.osc3Mix = 0.20f;  // Detuned sine
            v.osc4Mix = 0.15f;  // Wide detune
            v.osc5Mix = 0.10f;  // Widest detune
            v.subMix = 0.15f;
            // Formant-like filtering for vowel quality
            v.filterCoef = juce::jlimit (0.1f, 0.4f, (float) note.frequency / 1500.0f);
            v.filterCoef2 = 0.6f;  // Bandpass-like character
            break;
            
        case InstCelticHarp:
            // Bright plucked harp with ringing harmonics
            // Fundamental + harmonics that decay at different rates
            // Harmonics: 1, 2, 3, 4.01, 5.02 with fast decay
            v.sustainLevel = 0.4f;
            sustainTimeSec = note.decaySec * 0.08f;  // Very short sustain, mostly decay
            v.waveType1 = 1;  // triangle for fundamental
            v.waveType2 = 0;  // sine for harmonics
            v.waveType3 = 0;  // sine
            v.harmonicRatio1 = 2.0f;
            v.harmonicRatio2 = 3.0f;
            v.detuneRatio1 = 4.01f;  // Slightly detuned 4th harmonic
            v.detuneRatio2 = 5.02f;  // Slightly detuned 5th harmonic
            v.osc1Mix = 0.50f;  // Strong fundamental
            v.osc2Mix = 0.25f;  // 2nd harmonic
            v.osc3Mix = 0.12f;  // 3rd harmonic
            v.osc4Mix = 0.08f;  // 4th harmonic
            v.osc5Mix = 0.05f;  // 5th harmonic
            v.subMix = 0.0f;
            v.harmDecayMult = kHarmDecayHarp;  // Harmonics decay faster than fundamental
            v.filterCoef = juce::jlimit (0.3f, 0.8f, (float) note.frequency / 800.0f);
            break;
            
        case InstSoftString:
            // Warm sustained string with bowed character
            // Sawtooth filtered for string warmth, slight detune for ensemble
            v.sustainLevel = 0.80f;
            sustainTimeSec = note.decaySec * 0.4f;
            v.waveType1 = 2;  // sawtooth
            v.waveType2 = 2;  // sawtooth (detuned)
            v.waveType3 = 0;  // sine sub
            v.detuneRatio1 = 1.003f;  // Slight detune for ensemble effect
            v.detuneRatio2 = 0.997f;
            v.harmonicRatio1 = 1.0f;
            v.harmonicRatio2 = 0.5f;  // Sub-octave
            v.osc1Mix = 0.24f;
            v.osc2Mix = 0.24f;
            v.osc3Mix = 0.12f;
            v.osc4Mix = 0.0f;
            v.osc5Mix = 0.0f;
            v.subMix = 0.10f;
            // Gentle low-pass for warm, not buzzy tone
            v.filterCoef = juce::jlimit (0.08f, 0.35f, (float) note.frequency / 2000.0f);
            break;
            
        // ══════════════════════════════════════════════════════════════════════
        // AMBIENT ENSEMBLE (Native American Flute-inspired)
        // ══════════════════════════════════════════════════════════════════════
        
        case InstNativeFlute:
            // Breathy sine with vibrato and filtered noise breath component
            // Pure tone with subtle vibrato (4.5 Hz, narrow depth)
            v.sustainLevel = 0.75f;
            sustainTimeSec = note.decaySec * 0.35f;
            v.waveType1 = 0;  // Pure sine
            v.waveType2 = 0;  // sine
            v.waveType3 = 0;
            v.detuneRatio1 = 1.0f;
            v.detuneRatio2 = 2.0f;  // Octave above (breath character)
            v.harmonicRatio1 = 1.0f;
            v.harmonicRatio2 = 1.0f;
            v.osc1Mix = 0.55f;   // Strong fundamental
            v.osc2Mix = 0.08f;   // Subtle breath harmonic
            v.osc3Mix = 0.0f;
            v.osc4Mix = 0.0f;
            v.osc5Mix = 0.0f;
            v.subMix = 0.0f;
            // Vibrato settings
            v.vibratoRate = 4.5f + (float) (freqHash % 10) * 0.1f;
            v.vibratoDepth = (float) note.frequency * 0.008f;  // Narrow depth
            v.filterCoef = juce::jlimit (0.15f, 0.6f, (float) note.frequency / 1200.0f);
            break;
            
        case InstLowDrone:
            // Deep sustained tone with sub-octave and perfect fifth
            // Very slow attack, very long sustain
            v.sustainLevel = 0.92f;
            sustainTimeSec = note.decaySec * 0.65f;
            v.waveType1 = 0;  // sine
            v.waveType2 = 1;  // triangle for sub
            v.waveType3 = 0;  // sine for fifth
            v.detuneRatio1 = 0.5f;   // Sub-octave
            v.detuneRatio2 = 1.5f;   // Perfect fifth
            v.harmonicRatio1 = 0.5f;
            v.harmonicRatio2 = 1.5f;
            v.osc1Mix = 0.35f;
            v.osc2Mix = 0.30f;  // Sub-octave
            v.osc3Mix = 0.10f;  // Fifth
            v.osc4Mix = 0.0f;
            v.osc5Mix = 0.0f;
            v.subMix = 0.25f;
            // Dark filtering for drone
            v.filterCoef = juce::jlimit (0.06f, 0.25f, (float) note.frequency / 1500.0f);
            break;
            
        case InstWindPad:
            // Airy, nature-inspired wind texture
            // Pink noise-like through resonant filters (simulated with detuned oscillators)
            v.sustainLevel = 0.70f;
            sustainTimeSec = note.decaySec * 0.45f;
            v.waveType1 = 0;  // sine
            v.waveType2 = 1;  // triangle
            v.waveType3 = 0;  // sine
            // Wide detuning for shimmery, airy quality
            v.detuneRatio1 = 1.015f;
            v.detuneRatio2 = 0.985f;
            v.harmonicRatio1 = 2.01f;
            v.harmonicRatio2 = 3.02f;
            v.osc1Mix = 0.20f;
            v.osc2Mix = 0.18f;
            v.osc3Mix = 0.15f;
            v.osc4Mix = 0.12f;
            v.osc5Mix = 0.10f;
            v.subMix = 0.08f;
            // Resonant filtering for wind character
            v.filterCoef = juce::jlimit (0.12f, 0.45f, (float) note.frequency / 1000.0f);
            break;
            
        // ══════════════════════════════════════════════════════════════════════
        // ETHEREAL ENSEMBLE (The Sea and Cake-inspired: indie jazz)
        // ══════════════════════════════════════════════════════════════════════
        
        case InstCleanGuitar:
            // Plucked clean electric guitar with gentle chorus
            // Karplus-Strong-inspired: triangle wave with fast decay harmonics
            v.sustainLevel = 0.45f;
            sustainTimeSec = note.decaySec * 0.12f;
            v.waveType1 = 1;  // triangle
            v.waveType2 = 0;  // sine (2nd harmonic)
            v.waveType3 = 1;  // triangle (chorus copy)
            v.detuneRatio1 = 2.0f;      // 2nd harmonic
            v.detuneRatio2 = 1.004f;    // Slight chorus
            v.harmonicRatio1 = 2.0f;
            v.harmonicRatio2 = 1.0f;
            v.osc1Mix = 0.40f;
            v.osc2Mix = 0.15f;
            v.osc3Mix = 0.20f;  // Chorus
            v.osc4Mix = 0.0f;
            v.osc5Mix = 0.0f;
            v.subMix = 0.0f;
            v.harmDecayMult = kHarmDecayGuitar;  // Fast harmonic decay
            // Filter sweep: starts bright, decays to mellow
            v.filterCoef = juce::jlimit (0.25f, 0.75f, (float) note.frequency / 600.0f);
            break;
            
        case InstWarmKeys:
            // Rhodes-like electric piano with bell overtone
            // Sine fundamental + bell-like inharmonic overtone at 7×
            v.sustainLevel = 0.60f;
            sustainTimeSec = note.decaySec * 0.20f;
            v.waveType1 = 0;  // sine
            v.waveType2 = 0;  // sine (2nd)
            v.waveType3 = 0;  // sine (bell)
            v.detuneRatio1 = 2.0f;      // Octave
            v.detuneRatio2 = 7.0f;      // Inharmonic bell (characteristic Rhodes tine)
            v.harmonicRatio1 = 2.0f;
            v.harmonicRatio2 = 7.0f;
            v.osc1Mix = 0.45f;
            v.osc2Mix = 0.18f;
            v.osc3Mix = 0.12f;  // Bell overtone
            v.osc4Mix = 0.0f;
            v.osc5Mix = 0.0f;
            v.subMix = 0.08f;
            v.harmDecayMult = kHarmDecayKeys;  // Bell decays faster
            v.filterCoef = juce::jlimit (0.15f, 0.55f, (float) note.frequency / 1000.0f);
            break;
            
        case InstShimmerChorus:
            // Bright, shimmering indie texture with wide stereo
            // Multiple detuned oscillators for shimmery chorus
            v.sustainLevel = 0.72f;
            sustainTimeSec = note.decaySec * 0.35f;
            v.waveType1 = 1;  // triangle
            v.waveType2 = 0;  // sine
            v.waveType3 = 1;  // triangle
            // Detunes: -8, -3, 0, +3, +8 cents for shimmer
            v.detuneRatio1 = 1.0f + 3.0f / 1200.0f;
            v.detuneRatio2 = 1.0f - 3.0f / 1200.0f;
            v.harmonicRatio1 = 1.0f + 8.0f / 1200.0f;
            v.harmonicRatio2 = 1.0f - 8.0f / 1200.0f;
            v.osc1Mix = 0.22f;
            v.osc2Mix = 0.20f;
            v.osc3Mix = 0.18f;
            v.osc4Mix = 0.16f;
            v.osc5Mix = 0.14f;
            v.subMix = 0.0f;
            // Bright bandpass for shimmer quality
            v.filterCoef = juce::jlimit (0.25f, 0.75f, (float) note.frequency / 800.0f);
            break;
            
        // ══════════════════════════════════════════════════════════════════════
        // PERCUSSIVE ENSEMBLE (Mallet instruments)
        // ══════════════════════════════════════════════════════════════════════
        
        case InstVibraphone:
            // Metallic bar with motor-like tremolo
            // Inharmonic partials: 1, 3.99, 9.02 (characteristic vibraphone ratios)
            v.sustainLevel = 0.55f;
            sustainTimeSec = note.decaySec * 0.25f;
            v.waveType1 = 0;  // sine
            v.waveType2 = 0;  // sine
            v.waveType3 = 0;  // sine
            v.detuneRatio1 = 3.99f;     // Characteristic vibraphone partial
            v.detuneRatio2 = 9.02f;     // Higher inharmonic partial
            v.harmonicRatio1 = 3.99f;
            v.harmonicRatio2 = 9.02f;
            v.osc1Mix = 0.50f;
            v.osc2Mix = 0.22f;
            v.osc3Mix = 0.08f;
            v.osc4Mix = 0.0f;
            v.osc5Mix = 0.0f;
            v.subMix = 0.0f;
            // Vibraphone tremolo (simulates rotating motor disc)
            v.tremRate = 5.5f + (float) (freqHash % 10) * 0.1f;
            v.tremDepth = halloween ? 0.35f : 0.20f;
            v.filterCoef = juce::jlimit (0.30f, 0.85f, (float) note.frequency / 600.0f);
            break;
            
        case InstMarimba:
            // Warm, woody pitched percussion
            // Strong fundamental, gentle 2nd partial, soft higher (characteristic bar ratio 3.98)
            v.sustainLevel = 0.40f;
            sustainTimeSec = note.decaySec * 0.10f;
            v.waveType1 = 0;  // sine
            v.waveType2 = 0;  // sine
            v.waveType3 = 1;  // triangle for body
            v.detuneRatio1 = 3.98f;     // Characteristic marimba bar ratio
            v.detuneRatio2 = 2.0f;      // Octave for body resonance
            v.harmonicRatio1 = 3.98f;
            v.harmonicRatio2 = 2.0f;
            v.osc1Mix = 0.55f;
            v.osc2Mix = 0.12f;
            v.osc3Mix = 0.20f;  // Body resonance
            v.osc4Mix = 0.0f;
            v.osc5Mix = 0.0f;
            v.subMix = 0.0f;
            // Warm low-pass for woody character
            v.filterCoef = juce::jlimit (0.15f, 0.50f, (float) note.frequency / 1000.0f);
            break;
            
        case InstGlockenspiel:
            // Bright, bell-like metallic chime
            // Inharmonic partials: 1, 2.76, 5.40, 8.93 for metallic bell character
            v.sustainLevel = 0.50f;
            sustainTimeSec = note.decaySec * 0.18f;
            v.waveType1 = 0;  // sine
            v.waveType2 = 0;  // sine
            v.waveType3 = 0;  // sine
            v.detuneRatio1 = 2.76f;     // Inharmonic partial
            v.detuneRatio2 = 5.40f;     // Higher inharmonic partial
            v.harmonicRatio1 = 2.76f;
            v.harmonicRatio2 = 8.93f;   // Highest partial
            v.osc1Mix = 0.50f;
            v.osc2Mix = 0.25f;
            v.osc3Mix = 0.12f;
            v.osc4Mix = 0.05f;
            v.osc5Mix = 0.0f;
            v.subMix = 0.0f;
            // Bright but not harsh
            v.filterCoef = juce::jlimit (0.40f, 0.90f, (float) note.frequency / 500.0f);
            break;

        case NumInstruments:
        default:
            // Fallback to soft pad
            v.sustainLevel = 0.75f;
            sustainTimeSec = note.decaySec * 0.4f;
            v.osc1Mix = 0.40f;
            v.osc2Mix = 0.30f;
            v.osc3Mix = 0.15f;
            v.osc4Mix = 0.0f;
            v.osc5Mix = 0.0f;
            v.subMix = 0.15f;
            v.detuneRatio1 = 1.002f;
            v.detuneRatio2 = 0.998f;
            v.harmonicRatio1 = 2.0f;
            v.harmonicRatio2 = 3.0f;
            v.filterCoef = 0.3f;
            break;
    }
    
    v.sustainTime = static_cast<float> (sampleRate * sustainTimeSec);
    
    // Calculate release/decay rate
    // Halloween mode: longer decay for spookier, more sustained sounds
    const float decayMultiplier = halloween ? 1.6f : 1.0f;
    const float releaseTimeSec = (note.decaySec - sustainTimeSec) * decayMultiplier;
    const double releaseSamples = sampleRate * (double) releaseTimeSec;
    v.envDecay = (releaseSamples > 0.0)
        ? static_cast<float> (std::pow (0.001, 1.0 / releaseSamples))
        : 0.0f;
    
    // Reset filter state
    v.filterState = 0.0f;
    v.filterState2 = 0.0f;
}

//==============================================================================
// processBlock – render polyphonic voices with envelopes + reverb
//==============================================================================

// Named constants for harmonic decay multipliers
// Helper function to generate waveforms
// Phase is expected to be in range [0, 2π) and properly wrapped by caller
static inline float generateWaveform (double phase, int waveType, double twoPi)
{
    // Normalize phase to [0, 1)
    const float t = static_cast<float> (phase / twoPi);
    
    switch (waveType)
    {
        case 0:  // Sine
            return static_cast<float> (std::sin (phase));
            
        case 1:  // Triangle
        {
            // Triangle wave: rises linearly from -1 to +1 in first half,
            // then falls from +1 to -1 in second half.
            // Formula: 4 * |t - floor(t + 0.5)| - 1 for t in [0,1)
            // Or equivalently: 1 - 4 * |0.5 - fmod(t, 1)|
            const float triPhase = t - std::floor (t + 0.5f);
            return 4.0f * std::abs (triPhase) - 1.0f;
        }
        
        case 2:  // Sawtooth
        {
            // Sawtooth: rises from -1 to +1 over the period
            // Properly wrapped to avoid discontinuity at boundaries
            const float sawPhase = t - std::floor (t);
            return 2.0f * sawPhase - 1.0f;
        }
        
        default:
            return static_cast<float> (std::sin (phase));
    }
}

// Fast approximation for pow(x, n) where n is a small integer
// Much faster than std::pow in the audio loop
static inline float fastPow (float x, float n)
{
    if (n <= 1.0f) return x;
    if (n <= 2.0f) return x * x;
    if (n <= 3.0f) return x * x * x;
    if (n <= 4.0f) { float x2 = x * x; return x2 * x2; }
    // Fallback for non-standard values
    return std::pow (x, n);
}

void BLETonesAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer&         midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Check if Halloween mode changed and update reverb if needed
    const bool halloween = *apvts.getRawParameterValue ("halloweenMode") > 0.5f;
    if (halloween != lastHalloweenMode)
    {
        updateReverbForHalloweenMode (halloween);
        lastHalloweenMode = halloween;
    }
    buffer.clear();

    const float volume = *apvts.getRawParameterValue ("volume");

    // ── Pick up pending notes from the OSC thread ────────────────────────
    {
        juce::ScopedLock lock (deviceLock);

        for (const auto& note : pendingNotes)
        {
            activateVoice (note);
        }

        pendingNotes.clear();
    }

    // ── Render each active voice ─────────────────────────────────────────
    auto* leftCh  = buffer.getWritePointer (0);
    auto* rightCh = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    const int numSamples = buffer.getNumSamples();

    const double twoPi = juce::MathConstants<double>::twoPi;

    for (auto& v : voices)
    {
        if (! v.active)
            continue;

        // Base phase increments for all oscillators
        const double basePhaseInc = twoPi * v.frequency / sampleRate;
        const double phaseInc2 = basePhaseInc * (double) v.detuneRatio1;
        const double phaseInc3 = basePhaseInc * (double) v.detuneRatio2;
        const double phaseInc4 = basePhaseInc * (double) v.harmonicRatio1;
        const double phaseInc5 = basePhaseInc * (double) v.harmonicRatio2;
        const double phaseIncSub = basePhaseInc * 0.5;
        const double phaseIncTrem = twoPi * (double) v.tremRate / sampleRate;
        const double phaseIncVibrato = twoPi * (double) v.vibratoRate / sampleRate;

        for (int n = 0; n < numSamples; ++n)
        {
            // ── Update ADSR envelope ─────────────────────────────────────
            switch (v.envStage)
            {
                case Voice::Attack:
                    v.envelope += v.attackRate;
                    if (v.envelope >= 1.0f)
                    {
                        v.envelope = 1.0f;
                        v.envStage = Voice::Sustain;
                    }
                    break;
                    
                case Voice::Sustain:
                    // Gradually approach sustain level during sustain phase
                    if (v.envelope > v.sustainLevel)
                        v.envelope = v.envelope * 0.9999f + v.sustainLevel * 0.0001f;
                    
                    v.sustainTime -= 1.0f;
                    if (v.sustainTime <= 0.0f)
                    {
                        v.envStage = Voice::Release;
                    }
                    break;
                    
                case Voice::Release:
                    v.envelope *= v.envDecay;
                    break;
            }
            
            // ── Apply vibrato (pitch modulation) ─────────────────────────
            double effectivePhaseInc1 = basePhaseInc;
            if (v.vibratoDepth > 0.0f)
            {
                const double vibrato = v.vibratoDepth * std::sin (v.phaseVibrato);
                effectivePhaseInc1 = twoPi * (v.frequency + vibrato) / sampleRate;
                v.phaseVibrato += phaseIncVibrato;
                if (v.phaseVibrato > twoPi) v.phaseVibrato -= twoPi;
            }
            
            // ── Harmonic decay for plucked/percussive instruments ────────
            // Harmonics decay faster than the fundamental for plucked sounds
            float harmEnv = v.envelope;
            if (v.harmDecayMult > 1.0f)
            {
                // Apply exponential decay multiplier to harmonics
                // Using fastPow for performance in audio loop
                harmEnv = fastPow (v.envelope, v.harmDecayMult);
            }
            
            // ── Generate oscillators with appropriate waveforms ──────────
            // Osc1: Fundamental
            const float osc1 = generateWaveform (v.phase1, v.waveType1, twoPi);
            
            // Osc2: First detuned/harmonic
            const float osc2 = generateWaveform (v.phase2, v.waveType2, twoPi);
            
            // Osc3: Second detuned/harmonic
            const float osc3 = generateWaveform (v.phase3, v.waveType3, twoPi);
            
            // Osc4 & Osc5: Additional harmonics (always sine for smoothness)
            const float osc4 = static_cast<float> (std::sin (v.phase4));
            const float osc5 = static_cast<float> (std::sin (v.phase5));
            
            // Sub oscillator (always sine)
            const float oscSub = static_cast<float> (std::sin (v.phaseSub));
            
            // ── Mix oscillators based on instrument configuration ────────
            float sig = osc1 * v.osc1Mix
                      + osc2 * v.osc2Mix * harmEnv
                      + osc3 * v.osc3Mix * harmEnv
                      + osc4 * v.osc4Mix * harmEnv
                      + osc5 * v.osc5Mix * harmEnv
                      + oscSub * v.subMix;

            // ── Apply one-pole low-pass filter for warmth ────────────────
            v.filterState += v.filterCoef * (sig - v.filterState);
            sig = v.filterState;
            
            // Optional second filter stage for instruments that need it
            // (e.g., choir pad formant, sawtooth softening)
            if (v.filterCoef2 < 0.95f)
            {
                v.filterState2 += v.filterCoef2 * (sig - v.filterState2);
                sig = v.filterState2;
            }

            // ── Apply tremolo for vibraphone/Halloween effect ────────────
            if (v.tremDepth > 0.0f)
            {
                const float tremMod = 1.0f - v.tremDepth * (0.5f + 0.5f * static_cast<float> (std::sin (v.phaseTrem)));
                sig *= tremMod;
                v.phaseTrem += phaseIncTrem;
                if (v.phaseTrem > twoPi) v.phaseTrem -= twoPi;
            }

            // ── Apply envelope and amplitude ─────────────────────────────
            sig *= v.envelope * v.amplitude;

            // ── Equal-power panning ──────────────────────────────────────
            const float leftGain  = std::sqrt ((1.0f - v.pan) * 0.5f);
            const float rightGain = std::sqrt ((1.0f + v.pan) * 0.5f);

            leftCh[n] += sig * leftGain;
            if (rightCh != nullptr)
                rightCh[n] += sig * rightGain;

            // ── Advance oscillator phases ────────────────────────────────
            v.phase1 += effectivePhaseInc1;
            v.phase2 += phaseInc2;
            v.phase3 += phaseInc3;
            v.phase4 += phaseInc4;
            v.phase5 += phaseInc5;
            v.phaseSub += phaseIncSub;

            if (v.phase1   > twoPi) v.phase1   -= twoPi;
            if (v.phase2   > twoPi) v.phase2   -= twoPi;
            if (v.phase3   > twoPi) v.phase3   -= twoPi;
            if (v.phase4   > twoPi) v.phase4   -= twoPi;
            if (v.phase5   > twoPi) v.phase5   -= twoPi;
            if (v.phaseSub > twoPi) v.phaseSub -= twoPi;
        }

        // Deactivate voices that have decayed to silence
        if (v.envelope < 0.001f && v.envStage == Voice::Release)
        {
            v.active = false;
        }
    }

    // ── Apply master volume ──────────────────────────────────────────────
    buffer.applyGain (volume);

    // ── Apply reverb ─────────────────────────────────────────────────────
    if (buffer.getNumChannels() >= 2)
    {
        reverb.processStereo (buffer.getWritePointer (0),
                              buffer.getWritePointer (1),
                              numSamples);
    }
    else
    {
        reverb.processMono (buffer.getWritePointer (0), numSamples);
    }

    juce::ignoreUnused (midiMessages);
}

//==============================================================================
std::vector<BLEDevice> BLETonesAudioProcessor::getDevicesCopy() const
{
    juce::ScopedLock lock (deviceLock);
    return devices;
}

int BLETonesAudioProcessor::getActiveVoiceCount() const
{
    int count = 0;
    for (const auto& v : voices)
    {
        if (v.active)
            ++count;
    }
    return count;
}

bool BLETonesAudioProcessor::isHalloweenMode() const
{
    return *apvts.getRawParameterValue ("halloweenMode") > 0.5f;
}

//==============================================================================
void BLETonesAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void BLETonesAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessorEditor* BLETonesAudioProcessor::createEditor()
{
    return new BLETonesAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BLETonesAudioProcessor();
}
