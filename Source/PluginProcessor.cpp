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
// Decay times increased for more ambient, less staccato sound (matching Electron app)
static constexpr float kMinDecaySec       = 3.0f;    // Shortest note decay (far devices) - was 1.5s
static constexpr float kDecayRangeScale   = 5.0f;    // Additional decay for close devices (total max = 8.0 s)
static constexpr int   kPanHashRange      = 140;     // Hash range for pan (mapped to −0.7…+0.7)
static constexpr float kPanHashOffset     = 70.0f;   // Centre offset for pan hash
static constexpr int   kDetuneHashRange   = 50;      // Hash range for per-voice random detune
static constexpr float kDetuneStep        = 0.0001f; // Per-hash-unit detune (total range 0.001–0.006)
static constexpr float kInitialDelta      = 0.15f;   // Synthetic delta for first-sighting trigger
static constexpr int   kHeartbeatMs       = 3000;    // Trigger a soft note if idle for this long
static constexpr float kHeartbeatAmpFactor= 0.6f;    // Heartbeat notes are softer (60% of initial)

//==============================================================================
// Voice Type names – matching the Electron app's instrument flavors
//==============================================================================
const juce::StringArray& BLETonesAudioProcessor::getVoiceTypeNames()
{
    static const juce::StringArray names
    {
        "Pad (Enya)",         // Slow attack, long sustain - like Choir Pad / Soft String
        "Pluck (Harp)",       // Quick attack, medium decay - like Celtic Harp
        "Bell (Vibes)",       // Quick attack, long shimmering ring - like Vibraphone
        "Drone (Ambient)",    // Very slow attack, very long sustain - like Low Drone
        "Flute (Breathy)",    // Breathy with vibrato - like Native Flute
        "Keys (Warm)",        // Soft attack, warm piano-like - like Electric Piano
        "Guitar (Clean)"      // Quick pluck attack, medium sustain - like Clean Guitar
    };
    return names;
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

    // Voice type selection - matching Electron app's instrument flavors
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "voiceType",
        "Voice Type",
        getVoiceTypeNames(),
        0));  // Default: Pad (Enya)

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
    
    // Get voice type and envelope parameters from plugin state
    const int voiceType = static_cast<int> (*apvts.getRawParameterValue ("voiceType"));
    const float userAttack = *apvts.getRawParameterValue ("attackTime");
    const float userRelease = *apvts.getRawParameterValue ("releaseTime");

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
    const float amp = juce::jlimit (0.15f, 0.8f, delta * kDeltaToAmpScale);

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

        // Amplitude tapers for upper chord tones
        const float noteAmp = amp * (1.0f - (float) i * 0.15f);

        // Panning via hash for consistent spatial placement (range ≈ −0.7…+0.7)
        const float pan = ((float) (hashName (id + juce::String (i)) % kPanHashRange)
                          - kPanHashOffset) / 100.0f;

        // Slightly different decay per note for organic feel
        const float decay = baseDecay + (float) i * 0.5f;
        
        // Slightly different attack per note for organic stagger
        const float attack = userAttack * (1.0f + (float) i * 0.1f);

        pendingNotes.push_back ({ freq, noteAmp, pan, decay, attack, voiceType });
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
    v.phaseSub     = 0.0;
    v.phaseTrem    = 0.0;
    v.phaseVibrato = 0.0;
    v.phase3       = 0.0;
    v.frequency    = note.frequency;
    v.amplitude    = note.amplitude;
    v.pan          = note.pan;
    v.voiceType    = note.voiceType;
    
    // ── Set up ADSR envelope based on voice type ─────────────────────────────
    // Start in Attack stage with envelope at 0, ramping up
    v.envStage = Voice::Attack;
    v.envelope = 0.0f;
    
    // Calculate attack rate (per-sample increment to reach 1.0)
    // attackTime is in seconds, we need per-sample increment
    const double attackSamples = sampleRate * (double) note.attackSec;
    v.attackRate = (attackSamples > 0.0)
        ? static_cast<float> (1.0 / attackSamples)
        : 1.0f;  // Instant attack if 0
    
    // Sustain level varies by voice type (matching Electron app characteristics)
    // Calculate sustain time (time to hold after attack before release)
    // and decay rate based on voice type
    float sustainTimeSec = 0.0f;
    
    switch (static_cast<VoiceType> (note.voiceType))
    {
        case VoicePad:      // Slow attack, long sustain (Enya/Choir)
            v.sustainLevel = 0.85f;
            sustainTimeSec = note.decaySec * 0.5f;  // Half of total time is sustain
            break;
            
        case VoicePluck:    // Quick attack, medium decay (Celtic Harp)
            v.sustainLevel = 0.6f;
            sustainTimeSec = note.decaySec * 0.1f;  // Short sustain, mostly decay
            break;
            
        case VoiceBell:     // Quick attack, long shimmering ring (Vibraphone)
            v.sustainLevel = 0.7f;
            sustainTimeSec = note.decaySec * 0.3f;
            break;
            
        case VoiceDrone:    // Very slow attack, very long sustain
            v.sustainLevel = 0.9f;
            sustainTimeSec = note.decaySec * 0.7f;  // Most of time is sustain
            break;
            
        case VoiceFlute:    // Breathy with vibrato
            v.sustainLevel = 0.75f;
            sustainTimeSec = note.decaySec * 0.4f;
            v.vibratoRate  = 5.5f + (float) (hashName (juce::String (note.frequency)) % 20) * 0.1f;
            v.vibratoDepth = 4.0f + note.frequency * 0.003f;  // Subtle pitch vibrato
            break;
            
        case VoiceKeys:     // Soft attack, warm piano-like
            v.sustainLevel = 0.65f;
            sustainTimeSec = note.decaySec * 0.25f;
            break;
            
        case VoiceGuitar:   // Quick pluck, medium sustain
            v.sustainLevel = 0.55f;
            sustainTimeSec = note.decaySec * 0.15f;
            break;

        case NumVoiceTypes:
        default:
            v.sustainLevel = 0.8f;
            sustainTimeSec = note.decaySec * 0.3f;
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

    // ── Voice type specific oscillator settings ──────────────────────────────
    // Halloween mode: wider detune for eerie dissonance
    if (halloween)
    {
        v.detuneRatio = 1.003f
            + (float) (hashName (juce::String (note.frequency)) % kDetuneHashRange) * kDetuneStep * 3.0f;
        v.tremRate  = 4.0f + (float) (hashName (juce::String (note.frequency)) % 10) * 0.5f;
        v.tremDepth = 0.25f;
    }
    else
    {
        // Detune varies by voice type
        switch (static_cast<VoiceType> (note.voiceType))
        {
            case VoicePad:
                // Rich chorus-like detuning for pads
                v.detuneRatio = 1.002f + (float) (hashName (juce::String (note.frequency)) % kDetuneHashRange) * kDetuneStep * 2.0f;
                break;
            case VoiceBell:
                // Slight inharmonicity for bell-like quality
                v.detuneRatio = 1.0f + (float) (hashName (juce::String (note.frequency)) % 30) * 0.0003f;
                v.harmonicMix = 0.3f;  // Add harmonic content
                break;
            case VoiceFlute:
                // Minimal detuning for purer tone
                v.detuneRatio = 1.0005f;
                break;
            case VoicePluck:
            case VoiceDrone:
            case VoiceKeys:
            case VoiceGuitar:
            case NumVoiceTypes:
            default:
                v.detuneRatio = 1.001f + (float) (hashName (juce::String (note.frequency)) % kDetuneHashRange) * kDetuneStep;
                break;
        }
        v.tremRate  = 0.0f;
        v.tremDepth = 0.0f;
    }

    // Sub oscillator level varies by voice type
    float subBase = halloween ? 0.55f : 0.4f;
    switch (static_cast<VoiceType> (note.voiceType))
    {
        case VoiceDrone:
            subBase = halloween ? 0.7f : 0.6f;  // More sub for drones
            break;
        case VoiceBell:
        case VoiceFlute:
            subBase = 0.15f;  // Less sub for bright voices
            break;
        case VoiceGuitar:
        case VoicePluck:
            subBase = 0.25f;  // Moderate sub for plucked
            break;
        case VoicePad:
        case VoiceKeys:
        case NumVoiceTypes:
        default:
            break;
    }
    v.subMix = juce::jlimit (0.0f, subBase,
        1.0f - static_cast<float> (note.frequency / 600.0));

    // Low-pass filter coefficient based on voice type
    float filterMin, filterMax;
    if (halloween)
    {
        filterMin = 0.10f;
        filterMax = 0.45f;
    }
    else
    {
        switch (static_cast<VoiceType> (note.voiceType))
        {
            case VoicePad:
            case VoiceDrone:
                filterMin = 0.12f;
                filterMax = 0.5f;  // Warmer, darker
                break;
            case VoiceBell:
                filterMin = 0.25f;
                filterMax = 0.85f;  // Brighter for shimmer
                break;
            case VoiceFlute:
                filterMin = 0.2f;
                filterMax = 0.7f;
                break;
            case VoicePluck:
            case VoiceKeys:
            case VoiceGuitar:
            case NumVoiceTypes:
            default:
                filterMin = 0.15f;
                filterMax = 0.6f;
                break;
        }
    }
    v.filterCoef = juce::jlimit (filterMin, filterMax,
        static_cast<float> (note.frequency / 1200.0));
    v.filterState = 0.0f;
}

//==============================================================================
// processBlock – render polyphonic voices with envelopes + reverb
//==============================================================================

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

        // Base phase increments
        double phaseInc1   = twoPi * v.frequency / sampleRate;
        const double phaseInc2   = twoPi * v.frequency * (double) v.detuneRatio / sampleRate;
        const double phaseIncSub = twoPi * v.frequency * 0.5 / sampleRate;
        const double phaseIncTrem = twoPi * (double) v.tremRate / sampleRate;
        const double phaseIncVibrato = twoPi * (double) v.vibratoRate / sampleRate;
        // Third oscillator for bell/keys harmonics (2x frequency)
        const double phaseInc3 = twoPi * v.frequency * 2.003 / sampleRate;

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
                    // Uses exponential smoothing: env = 0.9999*env + 0.0001*target
                    // At 44100 Hz, this smooths over ~4400 samples (~100ms) for natural feel
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
            
            // ── Apply vibrato for Flute voice (pitch modulation) ─────────
            double effectivePhaseInc1 = phaseInc1;
            if (v.vibratoDepth > 0.0f)
            {
                const double vibrato = v.vibratoDepth * std::sin (v.phaseVibrato);
                effectivePhaseInc1 = twoPi * (v.frequency + vibrato) / sampleRate;
                v.phaseVibrato += phaseIncVibrato;
                if (v.phaseVibrato > twoPi) v.phaseVibrato -= twoPi;
            }
            
            // ── Voice type specific oscillator mixing ────────────────────
            float sig;
            const auto voiceType = static_cast<VoiceType> (v.voiceType);
            
            switch (voiceType)
            {
                case VoicePad:
                    // Rich layered pad: main + detuned + sub, all smooth sines
                    sig = static_cast<float> (std::sin (v.phase1)) * 0.40f
                        + static_cast<float> (std::sin (v.phase2)) * 0.35f
                        + static_cast<float> (std::sin (v.phaseSub)) * v.subMix;
                    break;
                    
                case VoicePluck:
                    // Harp-like: brighter attack, harmonics that decay
                    {
                        const float harmDecay = v.envelope * v.envelope;  // Harmonics decay faster
                        sig = static_cast<float> (std::sin (v.phase1)) * 0.50f
                            + static_cast<float> (std::sin (v.phase2)) * 0.20f * harmDecay
                            + static_cast<float> (std::sin (v.phase3)) * 0.15f * harmDecay * harmDecay;
                    }
                    break;
                    
                case VoiceBell:
                    // Bell: inharmonic partials for shimmer
                    // Ratios 2.76 and 1.51 approximate vibraphone bar modes (not integer harmonics)
                    // creating the characteristic metallic bell timbre. Based on analysis of
                    // struck bar instruments where partials are non-harmonic.
                    {
                        const float bellRing = 0.5f + 0.5f * v.envelope;  // Sustains shimmer
                        sig = static_cast<float> (std::sin (v.phase1)) * 0.45f
                            + static_cast<float> (std::sin (v.phase2 * 2.76)) * 0.25f * bellRing
                            + static_cast<float> (std::sin (v.phase3 * 1.51)) * 0.20f * bellRing;
                    }
                    break;
                    
                case VoiceDrone:
                    // Deep drone: emphasize sub, slow beating
                    sig = static_cast<float> (std::sin (v.phase1)) * 0.35f
                        + static_cast<float> (std::sin (v.phase2)) * 0.30f
                        + static_cast<float> (std::sin (v.phaseSub)) * v.subMix * 1.3f
                        + static_cast<float> (std::sin (v.phaseSub * 0.5)) * v.subMix * 0.4f;
                    break;
                    
                case VoiceFlute:
                    // Breathy: pure sine with subtle breath noise implied by filter
                    sig = static_cast<float> (std::sin (v.phase1)) * 0.65f
                        + static_cast<float> (std::sin (v.phase2)) * 0.15f;
                    break;
                    
                case VoiceKeys:
                    // Warm keys: fundamental + soft harmonics
                    {
                        const float keyDecay = std::sqrt (v.envelope);
                        sig = static_cast<float> (std::sin (v.phase1)) * 0.50f
                            + static_cast<float> (std::sin (v.phase2)) * 0.25f
                            + static_cast<float> (std::sin (v.phase3)) * 0.12f * keyDecay;
                    }
                    break;
                    
                case VoiceGuitar:
                    // Clean guitar: triangle-ish wave with quick harmonic decay
                    // Triangle wave formula: 2*|2*(phase/2pi) - 1| - 1 maps phase [0,2pi] to [-1,+1]
                    // in a linear up-down shape that sounds brighter than sine
                    {
                        const float guitarDecay = v.envelope * v.envelope * v.envelope;
                        // Generate triangle wave from phase: linear ramp up then down
                        const float normalizedPhase = static_cast<float> (v.phase1 / twoPi);
                        const float tri = 2.0f * std::abs (2.0f * normalizedPhase - 1.0f) - 1.0f;
                        sig = tri * 0.40f
                            + static_cast<float> (std::sin (v.phase1)) * 0.30f
                            + static_cast<float> (std::sin (v.phase3)) * 0.15f * guitarDecay;
                    }
                    break;

                case NumVoiceTypes:
                default:
                    sig = static_cast<float> (std::sin (v.phase1)) * 0.45f
                        + static_cast<float> (std::sin (v.phase2)) * 0.30f
                        + static_cast<float> (std::sin (v.phaseSub)) * v.subMix;
                    break;
            }

            // Apply one-pole low-pass filter for warmth
            v.filterState += v.filterCoef * (sig - v.filterState);
            sig = v.filterState;

            // Apply tremolo for spooky wavering effect (Halloween mode)
            if (v.tremDepth > 0.0f)
            {
                const float tremMod = 1.0f - v.tremDepth * (0.5f + 0.5f * static_cast<float> (std::sin (v.phaseTrem)));
                sig *= tremMod;
                v.phaseTrem += phaseIncTrem;
                if (v.phaseTrem > twoPi) v.phaseTrem -= twoPi;
            }

            // Apply envelope and amplitude
            sig *= v.envelope * v.amplitude;

            // Equal-power panning
            const float leftGain  = std::sqrt ((1.0f - v.pan) * 0.5f);
            const float rightGain = std::sqrt ((1.0f + v.pan) * 0.5f);

            leftCh[n] += sig * leftGain;
            if (rightCh != nullptr)
                rightCh[n] += sig * rightGain;

            // Advance oscillator phases
            v.phase1   += effectivePhaseInc1;
            v.phase2   += phaseInc2;
            v.phaseSub += phaseIncSub;
            v.phase3   += phaseInc3;

            if (v.phase1   > twoPi) v.phase1   -= twoPi;
            if (v.phase2   > twoPi) v.phase2   -= twoPi;
            if (v.phaseSub > twoPi) v.phaseSub -= twoPi;
            if (v.phase3   > twoPi) v.phase3   -= twoPi;
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
