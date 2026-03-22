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

    // Custom look-and-feel (concrete type defined in .cpp)
    std::unique_ptr<juce::LookAndFeel> customLookAndFeel;

    // Controls
    juce::Slider volumeSlider;
    juce::Slider sensitivitySlider;
    juce::Label  volumeLabel;
    juce::Label  sensitivityLabel;

    // Status / device list labels (kept for layout; drawing done in paint())
    juce::Label statusLabel;
    juce::Label deviceListLabel;

    // Parameter attachments
    juce::AudioProcessorValueTreeState::SliderAttachment volumeAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment sensitivityAttachment;

    // Cached device snapshot (refreshed at 15 Hz)
    std::vector<BLEDevice> cachedDevices;

    // Animation phase (radians, advanced each timer tick)
    float animPhase { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BLETonesAudioProcessorEditor)
};
