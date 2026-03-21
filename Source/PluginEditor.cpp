#include "PluginEditor.h"

//==============================================================================
BLETonesAudioProcessorEditor::BLETonesAudioProcessorEditor (BLETonesAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      volumeAttachment      (p.apvts, "volume",      volumeSlider),
      sensitivityAttachment (p.apvts, "sensitivity", sensitivitySlider)
{
    DBG ("[bleTones] Editor ctor");

    setSize (400, 320);

    // Volume slider
    volumeLabel.setText ("Volume", juce::dontSendNotification);
    addAndMakeVisible (volumeLabel);
    volumeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible (volumeSlider);

    // Sensitivity slider
    sensitivityLabel.setText ("BLE Sensitivity", juce::dontSendNotification);
    addAndMakeVisible (sensitivityLabel);
    sensitivitySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    sensitivitySlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible (sensitivitySlider);

    // Status line
    statusLabel.setJustificationType (juce::Justification::centred);
    statusLabel.setText ("Waiting for BLE helper on UDP 9000…",
                         juce::dontSendNotification);
    addAndMakeVisible (statusLabel);

    // Device list (multi-line)
    deviceListLabel.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (deviceListLabel);

    startTimerHz (4); // Refresh UI 4 times per second
}

BLETonesAudioProcessorEditor::~BLETonesAudioProcessorEditor()
{
    DBG ("[bleTones] Editor dtor");
    stopTimer();
}

//==============================================================================
void BLETonesAudioProcessorEditor::paint (juce::Graphics& g)
{
    DBG ("[bleTones] paint");

    g.fillAll (juce::Colour (0xff1a1a2e));

    g.setColour (juce::Colour (0xff00d4ff));
    g.setFont (18.0f);
    g.drawFittedText ("bleTones",
                      getLocalBounds().removeFromTop (40),
                      juce::Justification::centred, 1);
}

void BLETonesAudioProcessorEditor::resized()
{
    DBG ("[bleTones] resized: " + juce::String (getWidth()) + "x" + juce::String (getHeight()));

    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (40); // title

    auto makeRow = [&] (int height) { return area.removeFromTop (height).reduced (0, 2); };

    auto volRow = makeRow (30);
    volumeLabel.setBounds  (volRow.removeFromLeft (130));
    volumeSlider.setBounds (volRow);

    auto senRow = makeRow (30);
    sensitivityLabel.setBounds  (senRow.removeFromLeft (130));
    sensitivitySlider.setBounds (senRow);

    area.removeFromTop (8);
    statusLabel.setBounds (area.removeFromTop (20));

    area.removeFromTop (8);
    deviceListLabel.setBounds (area);
}

//==============================================================================
void BLETonesAudioProcessorEditor::timerCallback()
{
    cachedDevices = audioProcessor.getDevicesCopy();

    if (cachedDevices.empty())
    {
        statusLabel.setText ("Waiting for BLE helper on UDP 9000…",
                             juce::dontSendNotification);
        deviceListLabel.setText ("No BLE devices detected.\nMake sure bleTones Helper is running.",
                                 juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText (
            juce::String (static_cast<int> (cachedDevices.size())) + " device(s) active",
            juce::dontSendNotification);

        juce::String txt;
        for (const auto& d : cachedDevices)
            txt += d.name + "  " + juce::String (d.rssi) + " dBm\n";

        deviceListLabel.setText (txt.trimEnd(), juce::dontSendNotification);
    }

    repaint();
}