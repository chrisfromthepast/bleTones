#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    // Auto-launch the embedded BLE helper when running as a Standalone app.
    // The helper is packaged at:  bleTones.app/Contents/Resources/bleTones_helper.app
    // Launch with -g so it does not steal focus / appear in the foreground.
    {
        const juce::File helperApp =
            juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                .getParentDirectory()           // .../bleTones.app/Contents/MacOS
                .getParentDirectory()           // .../bleTones.app/Contents
                .getChildFile ("Resources")
                .getChildFile ("bleTones_helper.app");

        if (helperApp.exists())
        {
            juce::StringArray args;
            args.add ("/usr/bin/open");
            args.add ("-g");
            args.add (helperApp.getFullPathName());

            // 'open' delegates to LaunchServices and exits immediately;
            // the helper runs as its own independent process, so letting
            // launcher go out of scope here is intentional and safe.
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
// OSC message format:  /ble/rssi  <deviceName:string>  <rssi:int32>
void BLETonesAudioProcessor::oscMessageReceived (const juce::OSCMessage& msg)
{
    if (msg.getAddressPattern().toString() != "/ble/rssi")
        return;

    if (msg.size() < 2 || ! (msg[0].isString() && msg[1].isInt32()))
        return;

    const juce::String name = msg[0].getString();
    const int          rssi = msg[1].getInt32();
    const juce::int64  now  = juce::Time::currentTimeMillis();

    {
        juce::ScopedLock lock (deviceLock);

        bool found = false;
        for (auto& d : devices)
        {
            if (d.name == name)
            {
                d.rssi       = rssi;
                d.lastSeenMs = now;
                found        = true;
                break;
            }
        }

        if (! found)
            devices.push_back ({ name, rssi, now });

        // Evict devices not seen for more than 10 seconds
        devices.erase (
            std::remove_if (devices.begin(), devices.end(),
                [now] (const BLEDevice& d) { return (now - d.lastSeenMs) > 10'000; }),
            devices.end());
    }
}

std::vector<BLEDevice> BLETonesAudioProcessor::getDevicesCopy() const
{
    juce::ScopedLock lock (deviceLock);
    return devices;
}

int BLETonesAudioProcessor::getActiveVoiceCount() const
{
    int count = 0;
    for (const auto& v : voices)
        if (v.active)
            ++count;
    return count;
}

//==============================================================================
double BLETonesAudioProcessor::rssiToFrequency (int rssi)
{
    const int midiNote = juce::jlimit (kMinMIDINote, kMaxMIDINote,
        juce::jmap (rssi, kMinRSSI, kMaxRSSI, kMinMIDINote, kMaxMIDINote));
    return 440.0 * std::pow (2.0, (midiNote - 69) / 12.0);
}

float BLETonesAudioProcessor::rssiToVelocity (int rssi)
{
    return juce::jlimit (0.0f, 1.0f,
        juce::jmap (static_cast<float> (rssi),
                    static_cast<float> (kMinRSSI), static_cast<float> (kMaxRSSI),
                    0.1f, 1.0f));
}

//==============================================================================
void BLETonesAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer&         midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const float volume      = *apvts.getRawParameterValue ("volume");
    const float sensitivity = *apvts.getRawParameterValue ("sensitivity");

    // Amplitude ramps – attack faster than decay
    static constexpr float kAttackRate = 0.001f;
    static constexpr float kDecayRate  = 0.0005f;

    // Refresh voice targets from the current device list
    {
        juce::ScopedLock lock (deviceLock);

        int voiceIdx = 0;
        for (int i = 0; i < static_cast<int> (devices.size()) && voiceIdx < kMaxVoices; ++i)
        {
            const auto& d            = devices[static_cast<size_t> (i)];
            voices[voiceIdx].targetAmp  = rssiToVelocity (d.rssi) * sensitivity * volume;
            voices[voiceIdx].frequency  = rssiToFrequency (d.rssi);
            voices[voiceIdx].active     = true;
            ++voiceIdx;
        }

        for (int i = voiceIdx; i < kMaxVoices; ++i)
        {
            voices[i].targetAmp = 0.0f;
            if (voices[i].amplitude < 1.0e-4f)
                voices[i].active = false;
        }
    }

    // Render each active voice as a sine wave
    auto* leftCh  = buffer.getWritePointer (0);
    auto* rightCh = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    const int numSamples = buffer.getNumSamples();

    for (auto& v : voices)
    {
        if (! v.active && v.amplitude < 1.0e-4f)
            continue;

        const double phaseInc = juce::MathConstants<double>::twoPi * v.frequency / sampleRate;

        for (int n = 0; n < numSamples; ++n)
        {
            // Smooth amplitude towards target
            if (v.amplitude < v.targetAmp)
                v.amplitude = std::min (v.amplitude + kAttackRate, v.targetAmp);
            else
                v.amplitude = std::max (v.amplitude - kDecayRate,  v.targetAmp);

            const float sample = v.amplitude * static_cast<float> (std::sin (v.phase));
            leftCh[n] += sample;
            if (rightCh) rightCh[n] += sample;

            v.phase += phaseInc;
            if (v.phase > juce::MathConstants<double>::twoPi)
                v.phase -= juce::MathConstants<double>::twoPi;
        }
    }

    // Attenuate to keep the mix below 0 dBFS for up to kMaxVoices simultaneous voices
    buffer.applyGain (1.0f / static_cast<float> (kMaxVoices));

    juce::ignoreUnused (midiMessages);
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
