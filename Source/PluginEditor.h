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
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

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
    std::unique_ptr<juce::LookAndFeel> halloweenLookAndFeel;

    // Controls
    juce::Slider    volumeSlider;
    juce::Slider    sensitivitySlider;
    juce::Slider    attackSlider;        // Attack time control
    juce::Slider    releaseSlider;       // Release time control
    juce::Label     volumeLabel;
    juce::Label     sensitivityLabel;
    juce::Label     attackLabel;
    juce::Label     releaseLabel;
    juce::ComboBox  scaleCombo;
    juce::Label     scaleLabel;
    juce::ComboBox  keyCombo;
    juce::Label     keyLabel;
    juce::ComboBox  voiceTypeCombo;      // Voice type selection
    juce::Label     voiceTypeLabel;
    juce::ToggleButton halloweenToggle;  // Hidden – toggled via secret gesture

    // Parameter attachments
    juce::AudioProcessorValueTreeState::SliderAttachment   volumeAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment   sensitivityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> voiceTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   halloweenAttachment;

    // Cached device snapshot (refreshed at 15 Hz)
    std::vector<BLEDevice> cachedDevices;
    int cachedActiveVoices { 0 };
    bool cachedHalloweenMode { false };

    // Animation phase (radians, advanced each timer tick)
    float animPhase { 0.0f };

    // Pseudo-random node positions for network constellation (seeded once)
    struct NetNode { float x, y; };
    std::vector<NetNode> netNodes;
    void generateNetNodes (int W, int H);

    // Helper to update look-and-feel when Halloween mode changes
    void updateLookAndFeelForMode (bool halloween);

    // ── Secret Halloween activation (5 rapid title clicks) ───────────────────
    int    secretClickCount { 0 };
    juce::int64 lastSecretClickMs { 0 };
    juce::Rectangle<float> getTitleClickArea() const;

    // ── Device rename helper ─────────────────────────────────────────────────
    /** Returns the index of the device at a given click position, or -1. */
    int getDeviceIndexAtPosition (juce::Point<int> pos) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BLETonesAudioProcessorEditor)
};
