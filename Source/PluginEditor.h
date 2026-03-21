#pragma once
#include "PluginProcessor.h"

//==============================================================================
class BLETonesAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit BLETonesAudioProcessorEditor (BLETonesAudioProcessor&);
    ~BLETonesAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    BLETonesAudioProcessor& audioProcessor;

    // Controls
    juce::Slider volumeSlider;
    juce::Slider sensitivitySlider;
    juce::Label  volumeLabel;
    juce::Label  sensitivityLabel;

    // Status / device list
    juce::Label statusLabel;
    juce::Label deviceListLabel;

    // Parameter attachments
    juce::AudioProcessorValueTreeState::SliderAttachment volumeAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment sensitivityAttachment;

    // Cached device snapshot (refreshed at 4 Hz)
    std::vector<BLEDevice> cachedDevices;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BLETonesAudioProcessorEditor)
};
