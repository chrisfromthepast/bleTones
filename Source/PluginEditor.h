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

    // ── Paint helpers ────────────────────────────────────────────────────────
    void paintBackground           (juce::Graphics&, int W, int H);
    void paintNetworkLines         (juce::Graphics&, int W, int H);
    void paintBLEScannerPanel      (juce::Graphics&, int W, int H);
    void paintCenterContent        (juce::Graphics&, int W, int H);
    void paintSoundStatsPanel      (juce::Graphics&, int W, int H);
    void paintBottomBar            (juce::Graphics&, int W, int H);

    BLETonesAudioProcessor& audioProcessor;

    // Custom look-and-feel (concrete type defined in .cpp)
    std::unique_ptr<juce::LookAndFeel> customLookAndFeel;

    // Controls
    juce::Slider    volumeSlider;
    juce::Slider    sensitivitySlider;
    juce::Label     volumeLabel;
    juce::Label     sensitivityLabel;
    juce::ComboBox  scaleCombo;
    juce::Label     scaleLabel;
    juce::ComboBox  keyCombo;
    juce::Label     keyLabel;

    // Parameter attachments
    juce::AudioProcessorValueTreeState::SliderAttachment   volumeAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment   sensitivityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleAttachment;

    // Cached device snapshot (refreshed at 15 Hz)
    std::vector<BLEDevice> cachedDevices;
    int cachedActiveVoices { 0 };

    // Animation phase (radians, advanced each timer tick)
    float animPhase { 0.0f };

    // Pseudo-random node positions for network constellation (seeded once)
    struct NetNode { float x, y; };
    std::vector<NetNode> netNodes;
    void generateNetNodes (int W, int H);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BLETonesAudioProcessorEditor)
};
