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
static constexpr float kMinDecaySec       = 1.5f;    // Shortest note decay (far devices)
static constexpr float kDecayRangeScale   = 3.0f;    // Additional decay for close devices (total max = 4.5 s)
static constexpr int   kPanHashRange      = 140;     // Hash range for pan (mapped to −0.7…+0.7)
static constexpr float kPanHashOffset     = 70.0f;   // Centre offset for pan hash
static constexpr int   kDetuneHashRange   = 50;      // Hash range for per-voice random detune
static constexpr float kDetuneStep        = 0.0001f; // Per-hash-unit detune (total range 0.001–0.006)
static constexpr float kInitialDelta      = 0.15f;   // Synthetic delta for first-sighting trigger
static constexpr int   kHeartbeatMs       = 3000;    // Trigger a soft note if idle for this long
static constexpr float kHeartbeatAmpFactor= 0.6f;    // Heartbeat notes are softer (60% of initial)

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

    return layout;
}

//==============================================================================
BLETonesAudioProcessor::BLETonesAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    if (! oscReceiver.connect (kOSCPort))
        DBG ("bleTones: failed to bind OSC receiver on port " << kOSCPort);

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

    // ── Decay time: close devices ring longer for more sonic presence ────
    const float baseDecay = kMinDecaySec + normRssi * kDecayRangeScale;

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
        const float decay = baseDecay + (float) i * 0.3f;

        pendingNotes.push_back ({ freq, noteAmp, pan, decay });
    }
}

//==============================================================================
// Voice allocation
//==============================================================================

void BLETonesAudioProcessor::activateVoice (const PendingNote& note)
{
    // Find a free voice, or steal the quietest one
    int bestIdx     = 0;
    float quietest  = 2.0f;

    for (int i = 0; i < kMaxVoices; ++i)
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
    v.frequency    = note.frequency;
    v.amplitude    = note.amplitude;
    v.envelope     = 1.0f;  // Start at full envelope
    v.pan          = note.pan;

    // Per-sample envelope decay: reach ~0.001 in decaySec seconds
    //   envelope *= envDecay  each sample
    //   envDecay^(sr * decaySec) ≈ 0.001
    //   envDecay = 0.001^(1 / (sr * decaySec))
    // Halloween mode: longer decay for spookier, more sustained sounds
    const float decayMultiplier = halloween ? 1.6f : 1.0f;
    const double totalSamples = sampleRate * (double) (note.decaySec * decayMultiplier);
    v.envDecay = (totalSamples > 0.0)
        ? static_cast<float> (std::pow (0.001, 1.0 / totalSamples))
        : 0.0f;

    // Halloween mode: wider detune for eerie dissonance
    // Normal: range ≈ 1.001–1.006
    // Halloween: range ≈ 1.003–1.015 for more detuned, unsettling sound
    if (halloween)
    {
        v.detuneRatio = 1.003f
            + (float) (hashName (juce::String (note.frequency)) % kDetuneHashRange) * kDetuneStep * 3.0f;
        // Spooky tremolo effect - rate varies per voice for eerie wavering
        v.tremRate  = 4.0f + (float) (hashName (juce::String (note.frequency)) % 10) * 0.5f;
        v.tremDepth = 0.25f; // 25% amplitude modulation for ghostly waver
    }
    else
    {
        v.detuneRatio = 1.001f
            + (float) (hashName (juce::String (note.frequency)) % kDetuneHashRange) * kDetuneStep;
        v.tremRate  = 0.0f;
        v.tremDepth = 0.0f;
    }

    // Sub oscillator louder for lower frequencies (warmer bass, thinner treble)
    // Halloween mode: more prominent sub for darker, heavier sound
    const float subBase = halloween ? 0.55f : 0.4f;
    v.subMix = juce::jlimit (0.0f, subBase,
        1.0f - static_cast<float> (note.frequency / 600.0));

    // Low-pass filter coefficient – lower for low notes, higher for high notes
    // Halloween mode: darker tones with lower filter cutoff
    const float filterMin = halloween ? 0.10f : 0.15f;
    const float filterMax = halloween ? 0.45f : 0.6f;
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

        const double phaseInc1   = twoPi * v.frequency / sampleRate;
        const double phaseInc2   = twoPi * v.frequency * (double) v.detuneRatio / sampleRate;
        const double phaseIncSub = twoPi * v.frequency * 0.5 / sampleRate;
        const double phaseIncTrem = twoPi * (double) v.tremRate / sampleRate;

        for (int n = 0; n < numSamples; ++n)
        {
            // Three-oscillator mix: main sine + detuned sine + sub sine
            float sig = static_cast<float> (std::sin (v.phase1)) * 0.45f
                      + static_cast<float> (std::sin (v.phase2)) * 0.30f
                      + static_cast<float> (std::sin (v.phaseSub)) * v.subMix;

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

            // Decay envelope
            v.envelope *= v.envDecay;

            // Equal-power panning
            const float leftGain  = std::sqrt ((1.0f - v.pan) * 0.5f);
            const float rightGain = std::sqrt ((1.0f + v.pan) * 0.5f);

            leftCh[n] += sig * leftGain;
            if (rightCh != nullptr)
                rightCh[n] += sig * rightGain;

            // Advance oscillator phases
            v.phase1   += phaseInc1;
            v.phase2   += phaseInc2;
            v.phaseSub += phaseIncSub;

            if (v.phase1   > twoPi) v.phase1   -= twoPi;
            if (v.phase2   > twoPi) v.phase2   -= twoPi;
            if (v.phaseSub > twoPi) v.phaseSub -= twoPi;
        }

        // Deactivate voices that have decayed to silence
        if (v.envelope < 0.001f)
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
