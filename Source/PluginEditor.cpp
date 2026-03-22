#include "PluginEditor.h"

//==============================================================================
// Colour palette  (matching the original Electron / Web Audio look)
//==============================================================================
static const juce::Colour kColBg1    { 0xff1a1a2e };   // dark navy
static const juce::Colour kColBg2    { 0xff16213e };   // mid navy
static const juce::Colour kColBg3    { 0xff0f3460 };   // deep blue
static const juce::Colour kColGold   { 0xffe8d5b7 };   // warm gold (title / thumb)
static const juce::Colour kColMuted  { 0xff8e9aaf };   // muted blue-grey (body text)
static const juce::Colour kColBlue   { 0xff64b4ff };   // accent blue (BLE panel)
static const juce::Colour kColPanel  { 0xcc16213e };   // panel fill (semi-transparent)
static const juce::Colour kColPanelB { 0x4d64b4ff };   // panel border

// BLE signal-strength range used in all RSSI normalisation calls
static constexpr float kRssiMin        = -100.0f;  // dBm – barely detectable
static constexpr float kRssiMax        =  -30.0f;  // dBm – very close device

// Animation: one complete sine cycle takes this many timer ticks (15 Hz × 3 s)
static constexpr float kAnimCycleFrames = 45.0f;

// Maximum number of background orbs drawn (one per visible device)
static constexpr int kMaxOrbs = 8;

//==============================================================================
// BLETonesLookAndFeel – warm-gold sliders on dark background
//==============================================================================
class BLETonesLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BLETonesLookAndFeel()
    {
        setColour (juce::Slider::thumbColourId,             kColGold);
        setColour (juce::Slider::trackColourId,             kColPanelB);
        setColour (juce::Slider::backgroundColourId,        kColBg1);
        setColour (juce::Slider::textBoxTextColourId,       kColMuted);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId,               kColMuted);
    }

    void drawLinearSlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos, float /*minPos*/, float /*maxPos*/,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearHorizontal)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, width, height,
                                              sliderPos, 0.0f, 1.0f, style, slider);
            return;
        }

        constexpr float kTrackH = 4.0f;
        constexpr float kThumbR = 8.0f;
        const float trackY  = y + (height - kTrackH) * 0.5f;
        const float thumbCX = sliderPos;
        const float thumbCY = y + height * 0.5f;

        // Track background
        g.setColour (kColBlue.withAlpha (0.2f));
        g.fillRoundedRectangle ((float)x, trackY, (float)width, kTrackH, kTrackH * 0.5f);

        // Filled portion (left of thumb)
        if (thumbCX > (float)x)
        {
            g.setColour (kColGold.withAlpha (0.55f));
            g.fillRoundedRectangle ((float)x, trackY,
                                    thumbCX - (float)x, kTrackH, kTrackH * 0.5f);
        }

        // Thumb
        g.setColour (kColGold);
        g.fillEllipse (thumbCX - kThumbR, thumbCY - kThumbR,
                       kThumbR * 2.0f, kThumbR * 2.0f);

        // Thumb glow ring
        g.setColour (kColGold.withAlpha (0.25f));
        g.drawEllipse (thumbCX - kThumbR - 1.5f, thumbCY - kThumbR - 1.5f,
                       (kThumbR + 1.5f) * 2.0f, (kThumbR + 1.5f) * 2.0f, 1.5f);
    }
};

//==============================================================================
// Constructor / Destructor
//==============================================================================
BLETonesAudioProcessorEditor::BLETonesAudioProcessorEditor (BLETonesAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      customLookAndFeel (std::make_unique<BLETonesLookAndFeel>()),
      volumeAttachment      (p.apvts, "volume",      volumeSlider),
      sensitivityAttachment (p.apvts, "sensitivity", sensitivitySlider)
{
    DBG ("[bleTones] Editor ctor");

    setLookAndFeel (customLookAndFeel.get());
    setSize (520, 400);

    // Volume slider
    volumeLabel.setText ("Volume", juce::dontSendNotification);
    addAndMakeVisible (volumeLabel);
    volumeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 55, 22);
    addAndMakeVisible (volumeSlider);

    // Sensitivity slider
    sensitivityLabel.setText ("Sensitivity", juce::dontSendNotification);
    addAndMakeVisible (sensitivityLabel);
    sensitivitySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    sensitivitySlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 55, 22);
    addAndMakeVisible (sensitivitySlider);

    // Status / device list drawn directly in paint() – hide JUCE label components
    statusLabel.setVisible (false);
    deviceListLabel.setVisible (false);

    startTimerHz (15);
}

BLETonesAudioProcessorEditor::~BLETonesAudioProcessorEditor()
{
    DBG ("[bleTones] Editor dtor");
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
// paint
//==============================================================================
void BLETonesAudioProcessorEditor::paint (juce::Graphics& g)
{
    const int W = getWidth();
    const int H = getHeight();

    // ── Background gradient ──────────────────────────────────────────────────
    juce::ColourGradient bg (kColBg1, 0.0f, 0.0f,
                             kColBg3, 0.0f, (float)H, false);
    bg.addColour (0.5, kColBg2);
    g.setGradientFill (bg);
    g.fillAll();

    // ── Animated orbs – one per active BLE device ────────────────────────────
    for (int i = 0; i < (int)cachedDevices.size() && i < kMaxOrbs; ++i)
    {
        const float rssiNorm = juce::jlimit (0.0f, 1.0f,
            juce::jmap ((float)cachedDevices[(size_t)i].rssi, kRssiMin, kRssiMax, 0.0f, 1.0f));
        const float pulse  = 0.5f + 0.5f * std::sin (animPhase + (float)i * 1.3f);
        const float radius = 40.0f + rssiNorm * 80.0f;
        const float cx = (float)W * ((float)(i + 1) / ((float)cachedDevices.size() + 1.0f));
        const float cy = (float)H * 0.46f;

        juce::ColourGradient orb (kColBlue.withAlpha (0.12f * pulse),
                                   cx, cy,
                                   juce::Colours::transparentBlack,
                                   cx + radius, cy, true);
        g.setGradientFill (orb);
        g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    }

    // ── Title "bleTones" ─────────────────────────────────────────────────────
    // Soft glow pass
    g.setColour (kColGold.withAlpha (0.08f));
    g.setFont (juce::Font (44.0f).withExtraKerningFactor (0.15f));
    g.drawFittedText ("bleTones", 0, 14, W, 52, juce::Justification::centred, 1);

    // Main title
    g.setColour (kColGold);
    g.setFont (juce::Font (38.0f).withExtraKerningFactor (0.15f));
    g.drawFittedText ("bleTones", 0, 18, W, 46, juce::Justification::centred, 1);

    // ── Subtitle ─────────────────────────────────────────────────────────────
    g.setColour (kColMuted);
    g.setFont (juce::Font (10.5f).withExtraKerningFactor (0.2f));
    g.drawFittedText ("GENERATIVE SOUND EXPERIENCE",
                      0, 65, W, 16, juce::Justification::centred, 1);

    // Thin separator
    g.setColour (kColMuted.withAlpha (0.15f));
    g.drawHorizontalLine (84, 20.0f, (float)(W - 20));

    // ── BLE device panel ─────────────────────────────────────────────────────
    const juce::Rectangle<float> panel (12.0f, 91.0f, (float)(W - 24), 190.0f);

    // Panel fill + border
    g.setColour (kColPanel);
    g.fillRoundedRectangle (panel, 10.0f);
    g.setColour (kColPanelB);
    g.drawRoundedRectangle (panel, 10.0f, 1.0f);

    // Panel header "BLE DEVICES"
    g.setColour (kColBlue);
    g.setFont (juce::Font (11.5f).withExtraKerningFactor (0.08f));
    g.drawText ("BLE DEVICES",
                (int)panel.getX() + 14, (int)panel.getY() + 11,
                160, 18, juce::Justification::centredLeft);

    // Pulsing scanning dot (top-right of panel)
    const float scanPulse = 0.45f + 0.55f * (0.5f + 0.5f * std::sin (animPhase * 2.2f));
    g.setColour (kColBlue.withAlpha (scanPulse));
    constexpr float kDotR = 4.0f;
    g.fillEllipse (panel.getRight() - 18.0f - kDotR,
                   panel.getY() + 11.0f + (18.0f - kDotR * 2.0f) * 0.5f,
                   kDotR * 2.0f, kDotR * 2.0f);

    // Separator inside panel
    g.setColour (kColBlue.withAlpha (0.18f));
    g.drawHorizontalLine ((int)panel.getY() + 32,
                           panel.getX() + 8.0f, panel.getRight() - 8.0f);

    if (cachedDevices.empty())
    {
        // Waiting state – pulse the message gently
        const float waitAlpha = 0.35f + 0.3f * (0.5f + 0.5f * std::sin (animPhase * 1.2f));
        g.setColour (kColMuted.withAlpha (waitAlpha));
        g.setFont (juce::Font (12.0f));
        g.drawFittedText ("Waiting for BLE helper on UDP 9000\u2026",
                           (int)panel.getX(), (int)panel.getY() + 42,
                           (int)panel.getWidth(), 44,
                           juce::Justification::centred, 2);

        g.setColour (kColMuted.withAlpha (0.35f));
        g.setFont (juce::Font (10.5f));
        g.drawFittedText ("Launch bleTones Helper to begin scanning",
                           (int)panel.getX(), (int)panel.getY() + 95,
                           (int)panel.getWidth(), 72,
                           juce::Justification::centred, 2);
    }
    else
    {
        // ── Device rows ──────────────────────────────────────────────────────
        constexpr int   kMaxShow = 5;
        constexpr float kRowH    = 29.0f;
        const float     rowY0    = panel.getY() + 37.0f;
        const float     nameX    = panel.getX() + 14.0f;
        const float     nameW    = 165.0f;
        const float     barMaxW  = panel.getWidth() - nameW - 76.0f - 14.0f;
        const float     barX     = nameX + nameW;

        for (int i = 0; i < std::min ((int)cachedDevices.size(), kMaxShow); ++i)
        {
            const auto&  dev     = cachedDevices[(size_t)i];
            const float  rssiN   = juce::jlimit (0.0f, 1.0f,
                juce::jmap ((float)dev.rssi, kRssiMin, kRssiMax, 0.0f, 1.0f));
            const float  rowY    = rowY0 + (float)i * kRowH;
            const float  barW    = barMaxW * rssiN;
            const float  barCtrY = rowY + kRowH * 0.5f;

            // Device name
            g.setColour (kColMuted);
            g.setFont (juce::Font (12.0f));
            g.drawText (dev.name,
                        (int)nameX, (int)rowY, (int)nameW, (int)kRowH,
                        juce::Justification::centredLeft, true);

            // RSSI bar background
            g.setColour (kColBlue.withAlpha (0.12f));
            g.fillRoundedRectangle (barX, barCtrY - 3.0f, barMaxW, 6.0f, 3.0f);

            // RSSI bar filled
            if (barW > 0.0f)
            {
                g.setColour (kColBlue.withAlpha (0.3f + 0.7f * rssiN));
                g.fillRoundedRectangle (barX, barCtrY - 3.0f, barW, 6.0f, 3.0f);
            }

            // RSSI value (monospace)
            g.setColour (kColBlue);
            g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 11.0f, 0));
            g.drawText (juce::String (dev.rssi) + " dB",
                        (int)(panel.getRight() - 70.0f), (int)rowY,
                        64, (int)kRowH,
                        juce::Justification::centredRight);
        }
    }
}

//==============================================================================
// resized
//==============================================================================
void BLETonesAudioProcessorEditor::resized()
{
    // Title (y=18–64), subtitle (y=65), separator (y=84), and the BLE panel
    // (y=91, h=190, bottom=281) are all drawn in paint() without JUCE components.
    // Controls live below the panel with a small gap.

    auto area = getLocalBounds().reduced (12);

    // Skip over the painted header + panel area (absolute y=295 → offset 283).
    area.removeFromTop (283);

    auto makeRow = [&] (int h) { return area.removeFromTop (h).reduced (0, 2); };

    auto volRow = makeRow (32);
    volumeLabel.setBounds  (volRow.removeFromLeft (100));
    volumeSlider.setBounds (volRow);

    area.removeFromTop (6);

    auto senRow = makeRow (32);
    sensitivityLabel.setBounds  (senRow.removeFromLeft (100));
    sensitivitySlider.setBounds (senRow);
}

//==============================================================================
// timerCallback  (15 Hz)
//==============================================================================
void BLETonesAudioProcessorEditor::timerCallback()
{
    cachedDevices = audioProcessor.getDevicesCopy();

    // Advance animation phase (~3-second cycle at 15 Hz)
    animPhase += juce::MathConstants<float>::twoPi / kAnimCycleFrames;
    if (animPhase > juce::MathConstants<float>::twoPi)
        animPhase -= juce::MathConstants<float>::twoPi;

    repaint();
}
