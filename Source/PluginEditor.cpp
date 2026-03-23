#include "PluginEditor.h"
#include <cmath>

//==============================================================================
// Colour palette  (matching the original Electron / Web Audio look)
//==============================================================================
static const juce::Colour kColBg1        { 0xff1a1a2e };  // dark navy
static const juce::Colour kColBg2        { 0xff16213e };  // mid navy
static const juce::Colour kColBg3        { 0xff0f3460 };  // deep blue
static const juce::Colour kColGold       { 0xffe8d5b7 };  // warm gold (title / thumb)
static const juce::Colour kColGoldDark   { 0xffb8a48c };  // darker gold (buttons)
static const juce::Colour kColMuted      { 0xff8e9aaf };  // muted blue-grey (body text)
static const juce::Colour kColBlue       { 0xff64b4ff };  // accent blue (BLE panel)
static const juce::Colour kColBlueDim    { 0xff3d7ab8 };  // dimmer blue
static const juce::Colour kColPanel      { 0xcc16213e };  // panel fill (semi-transparent)
static const juce::Colour kColPanelB     { 0x4d64b4ff };  // panel border
static const juce::Colour kColTextLight  { 0xffe8e8e8 };  // light text
static const juce::Colour kColGreen      { 0xff4caf50 };  // status green

//==============================================================================
// Halloween Colour palette – spooky orange, purple, and dark tones
//==============================================================================
static const juce::Colour kColHalBg1     { 0xff1a0a0a };  // very dark red-black
static const juce::Colour kColHalBg2     { 0xff2a1510 };  // dark brown
static const juce::Colour kColHalBg3     { 0xff3d1a0a };  // deep rust
static const juce::Colour kColHalOrange  { 0xffff6b00 };  // pumpkin orange (main accent)
static const juce::Colour kColHalOrangeDark { 0xffcc5500 };  // darker orange
static const juce::Colour kColHalPurple  { 0xff8b00ff };  // spooky purple (secondary accent)
static const juce::Colour kColHalGreen   { 0xff39ff14 };  // toxic green (status)
static const juce::Colour kColHalMuted   { 0xff9a7b6a };  // muted brown-grey
static const juce::Colour kColHalPanel   { 0xcc2a1510 };  // panel fill (semi-transparent)
static const juce::Colour kColHalPanelB  { 0x4dff6b00 };  // panel border (orange)

// Layout constants
static constexpr int kLeftPanelW    = 270;   // BLE Scanner panel width
static constexpr int kRightPanelW   = 200;   // Sound Stats panel width
static constexpr int kPanelMargin   = 16;    // margin around panels
static constexpr int kBottomBarH    = 60;    // bottom info bar height

// BLE signal-strength range
static constexpr float kRssiMin        = -100.0f;
static constexpr float kRssiMax        =  -30.0f;

// Animation
static constexpr float kAnimCycleFrames = 45.0f;
static constexpr int   kMaxDeviceRows   = 8;

// Musical note names (shared between key combo and stats display)
static const char* const kNoteNames[] = { "C", "C#", "D", "D#", "E", "F",
                                           "F#", "G", "G#", "A", "A#", "B" };

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

        // ComboBox styling
        setColour (juce::ComboBox::backgroundColourId,      kColBg1);
        setColour (juce::ComboBox::textColourId,            kColTextLight);
        setColour (juce::ComboBox::outlineColourId,         kColPanelB);
        setColour (juce::ComboBox::arrowColourId,           kColGold);
        setColour (juce::PopupMenu::backgroundColourId,     kColBg2);
        setColour (juce::PopupMenu::textColourId,           kColTextLight);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, kColBg3);
        setColour (juce::PopupMenu::highlightedTextColourId, kColGold);

        // ToggleButton styling
        setColour (juce::ToggleButton::textColourId,        kColMuted);
        setColour (juce::ToggleButton::tickColourId,        kColGold);
        setColour (juce::ToggleButton::tickDisabledColourId, kColMuted);
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
        constexpr float kThumbR = 7.0f;
        const float trackY  = y + (height - kTrackH) * 0.5f;
        const float thumbCX = sliderPos;
        const float thumbCY = y + height * 0.5f;

        // Track background
        g.setColour (kColBlue.withAlpha (0.15f));
        g.fillRoundedRectangle ((float) x, trackY, (float) width, kTrackH, kTrackH * 0.5f);

        // Filled portion
        if (thumbCX > (float) x)
        {
            g.setColour (kColGold.withAlpha (0.55f));
            g.fillRoundedRectangle ((float) x, trackY,
                                    thumbCX - (float) x, kTrackH, kTrackH * 0.5f);
        }

        // Thumb
        g.setColour (kColGold);
        g.fillEllipse (thumbCX - kThumbR, thumbCY - kThumbR,
                       kThumbR * 2.0f, kThumbR * 2.0f);

        // Thumb glow
        g.setColour (kColGold.withAlpha (0.2f));
        g.drawEllipse (thumbCX - kThumbR - 1.5f, thumbCY - kThumbR - 1.5f,
                       (kThumbR + 1.5f) * 2.0f, (kThumbR + 1.5f) * 2.0f, 1.5f);
    }
};

//==============================================================================
// Halloween LookAndFeel – spooky orange theme
//==============================================================================
class BLETonesHalloweenLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BLETonesHalloweenLookAndFeel()
    {
        setColour (juce::Slider::thumbColourId,             kColHalOrange);
        setColour (juce::Slider::trackColourId,             kColHalPanelB);
        setColour (juce::Slider::backgroundColourId,        kColHalBg1);
        setColour (juce::Slider::textBoxTextColourId,       kColHalMuted);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId,               kColHalMuted);

        // ComboBox styling
        setColour (juce::ComboBox::backgroundColourId,      kColHalBg1);
        setColour (juce::ComboBox::textColourId,            kColTextLight);
        setColour (juce::ComboBox::outlineColourId,         kColHalPanelB);
        setColour (juce::ComboBox::arrowColourId,           kColHalOrange);
        setColour (juce::PopupMenu::backgroundColourId,     kColHalBg2);
        setColour (juce::PopupMenu::textColourId,           kColTextLight);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, kColHalBg3);
        setColour (juce::PopupMenu::highlightedTextColourId, kColHalOrange);

        // ToggleButton styling
        setColour (juce::ToggleButton::textColourId,        kColHalMuted);
        setColour (juce::ToggleButton::tickColourId,        kColHalOrange);
        setColour (juce::ToggleButton::tickDisabledColourId, kColHalMuted);
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
        constexpr float kThumbR = 7.0f;
        const float trackY  = y + (height - kTrackH) * 0.5f;
        const float thumbCX = sliderPos;
        const float thumbCY = y + height * 0.5f;

        // Track background
        g.setColour (kColHalOrange.withAlpha (0.15f));
        g.fillRoundedRectangle ((float) x, trackY, (float) width, kTrackH, kTrackH * 0.5f);

        // Filled portion
        if (thumbCX > (float) x)
        {
            g.setColour (kColHalOrange.withAlpha (0.55f));
            g.fillRoundedRectangle ((float) x, trackY,
                                    thumbCX - (float) x, kTrackH, kTrackH * 0.5f);
        }

        // Thumb
        g.setColour (kColHalOrange);
        g.fillEllipse (thumbCX - kThumbR, thumbCY - kThumbR,
                       kThumbR * 2.0f, kThumbR * 2.0f);

        // Thumb glow (spooky purple)
        g.setColour (kColHalPurple.withAlpha (0.3f));
        g.drawEllipse (thumbCX - kThumbR - 1.5f, thumbCY - kThumbR - 1.5f,
                       (kThumbR + 1.5f) * 2.0f, (kThumbR + 1.5f) * 2.0f, 1.5f);
    }
};

//==============================================================================
// Network constellation node generation (deterministic from dimensions)
//==============================================================================
void BLETonesAudioProcessorEditor::generateNetNodes (int W, int H)
{
    netNodes.clear();
    // Use a simple linear-congruential approach seeded by dimensions
    unsigned seed = (unsigned) (W * 31 + H * 17 + 42);
    auto nextRand = [&seed]() -> float {
        seed = seed * 1103515245u + 12345u;
        return (float) ((seed >> 16) & 0x7FFF) / 32767.0f;
    };

    const int count = 25 + (W * H) / 30000;
    for (int i = 0; i < count; ++i)
    {
        netNodes.push_back ({ nextRand() * (float) W, nextRand() * (float) H });
    }
}

//==============================================================================
// Constructor / Destructor
//==============================================================================
BLETonesAudioProcessorEditor::BLETonesAudioProcessorEditor (BLETonesAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      customLookAndFeel (std::make_unique<BLETonesLookAndFeel>()),
      halloweenLookAndFeel (std::make_unique<BLETonesHalloweenLookAndFeel>()),
      volumeAttachment      (p.apvts, "volume",      volumeSlider),
      sensitivityAttachment (p.apvts, "sensitivity", sensitivitySlider)
{
    // Check initial Halloween mode state
    cachedHalloweenMode = p.isHalloweenMode();
    setLookAndFeel (cachedHalloweenMode ? halloweenLookAndFeel.get() : customLookAndFeel.get());
    setSize (900, 640);

    // Halloween mode toggle
    halloweenToggle.setButtonText ("Halloween Mode");
    addAndMakeVisible (halloweenToggle);
    halloweenAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        p.apvts, "halloweenMode", halloweenToggle);

    // Volume slider
    volumeLabel.setText ("Volume", juce::dontSendNotification);
    addAndMakeVisible (volumeLabel);
    volumeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible (volumeSlider);

    // Sensitivity slider
    sensitivityLabel.setText ("Sensitivity", juce::dontSendNotification);
    addAndMakeVisible (sensitivityLabel);
    sensitivitySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    sensitivitySlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible (sensitivitySlider);

    // Scale / mode combo box – items must be added BEFORE the attachment
    scaleLabel.setText ("Scale", juce::dontSendNotification);
    addAndMakeVisible (scaleLabel);
    {
        const auto& names = BLETonesAudioProcessor::getScaleNames();
        for (int i = 0; i < names.size(); ++i)
            scaleCombo.addItem (names[i], i + 1);  // ComboBox IDs are 1-based
    }
    addAndMakeVisible (scaleCombo);
    scaleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        p.apvts, "scaleType", scaleCombo);

    // Key / root note combo box
    keyLabel.setText ("Key", juce::dontSendNotification);
    addAndMakeVisible (keyLabel);
    {
        // Populate with musical notes from C1 (MIDI 24) to C5 (MIDI 72)
        // matching the Electron app's root note slider range
        int itemId = 1;
        for (int midi = 24; midi <= 72; ++midi)
        {
            const int octave = (midi / 12) - 1;
            const juce::String label = juce::String (kNoteNames[midi % 12])
                                     + juce::String (octave);
            keyCombo.addItem (label, itemId++);
        }

        // Set the initial selection from the parameter (default MIDI 60 = C4)
        const int currentMidi = static_cast<int> (*p.apvts.getRawParameterValue ("rootNote"));
        const int comboIdx = juce::jlimit (0, 48, currentMidi - 24);
        keyCombo.setSelectedId (comboIdx + 1, juce::dontSendNotification);

        keyCombo.onChange = [this, &p]()
        {
            const int selectedIdx = keyCombo.getSelectedId() - 1;   // 0-based
            const int midiNote    = selectedIdx + 24;               // MIDI 24–72
            p.apvts.getParameter ("rootNote")->setValueNotifyingHost (
                p.apvts.getParameter ("rootNote")->convertTo0to1 ((float) midiNote));
        };
    }
    addAndMakeVisible (keyCombo);

    generateNetNodes (900, 640);
    startTimerHz (15);
}

BLETonesAudioProcessorEditor::~BLETonesAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
// Helper to update look-and-feel when Halloween mode changes
//==============================================================================
void BLETonesAudioProcessorEditor::updateLookAndFeelForMode (bool halloween)
{
    setLookAndFeel (halloween ? halloweenLookAndFeel.get() : customLookAndFeel.get());
}

//==============================================================================
// paint
//==============================================================================
void BLETonesAudioProcessorEditor::paint (juce::Graphics& g)
{
    const int W = getWidth();
    const int H = getHeight();

    paintBackground      (g, W, H);
    paintNetworkLines    (g, W, H);
    paintBLEScannerPanel (g, W, H);
    paintCenterContent   (g, W, H);
    paintSoundStatsPanel (g, W, H);
    paintBottomBar       (g, W, H);
}

//==============================================================================
// paintBackground
//==============================================================================
void BLETonesAudioProcessorEditor::paintBackground (juce::Graphics& g, int W, int H)
{
    if (cachedHalloweenMode)
    {
        // Halloween: dark red-brown gradient
        juce::ColourGradient bg (kColHalBg1, 0.0f, 0.0f,
                                 kColHalBg3, (float) W, (float) H, false);
        bg.addColour (0.5, kColHalBg2);
        g.setGradientFill (bg);
    }
    else
    {
        // Normal: dark navy gradient
        juce::ColourGradient bg (kColBg1, 0.0f, 0.0f,
                                 kColBg3, (float) W, (float) H, false);
        bg.addColour (0.5, kColBg2);
        g.setGradientFill (bg);
    }
    g.fillAll();
}

//==============================================================================
// paintNetworkLines – decorative constellation lines like the Electron version
//==============================================================================
void BLETonesAudioProcessorEditor::paintNetworkLines (juce::Graphics& g, int W, int H)
{
    juce::ignoreUnused (W, H);

    if (netNodes.empty())
        return;

    const float maxDist = 160.0f;
    const auto lineCol = cachedHalloweenMode ? kColHalOrange : kColBlue;

    // Draw connections
    for (size_t i = 0; i < netNodes.size(); ++i)
    {
        for (size_t j = i + 1; j < netNodes.size(); ++j)
        {
            const float dx   = netNodes[i].x - netNodes[j].x;
            const float dy   = netNodes[i].y - netNodes[j].y;
            const float dist = std::sqrt (dx * dx + dy * dy);

            if (dist < maxDist)
            {
                float alpha = (1.0f - dist / maxDist) * 0.08f;
                // Subtle pulse based on animation
                alpha *= 0.7f + 0.3f * (0.5f + 0.5f * std::sin (animPhase + (float) i * 0.3f));
                g.setColour (lineCol.withAlpha (alpha));
                g.drawLine (netNodes[i].x, netNodes[i].y,
                            netNodes[j].x, netNodes[j].y, 0.8f);
            }
        }
    }

    // Draw nodes
    const auto nodeCol = cachedHalloweenMode ? kColHalPurple : kColBlue;
    for (size_t i = 0; i < netNodes.size(); ++i)
    {
        const float pulse = 0.5f + 0.5f * std::sin (animPhase * 0.8f + (float) i * 0.7f);
        const float r = 1.2f + pulse * 1.0f;
        g.setColour (nodeCol.withAlpha (0.1f + pulse * 0.08f));
        g.fillEllipse (netNodes[i].x - r, netNodes[i].y - r, r * 2.0f, r * 2.0f);
    }
}

//==============================================================================
// paintBLEScannerPanel – left sidebar
//==============================================================================
void BLETonesAudioProcessorEditor::paintBLEScannerPanel (juce::Graphics& g, int /*W*/, int H)
{
    const float px = (float) kPanelMargin;
    const float py = (float) kPanelMargin;
    const float pw = (float) kLeftPanelW;
    const float ph = (float) (H - kPanelMargin * 2 - kBottomBarH);
    const juce::Rectangle<float> panel (px, py, pw, ph);

    // Choose colors based on Halloween mode
    const auto panelBg  = cachedHalloweenMode ? kColHalPanel : kColPanel;
    const auto panelBdr = cachedHalloweenMode ? kColHalPanelB : kColPanelB;
    const auto accent   = cachedHalloweenMode ? kColHalOrange : kColBlue;
    const auto muted    = cachedHalloweenMode ? kColHalMuted : kColMuted;
    const auto status   = cachedHalloweenMode ? kColHalGreen : kColGreen;

    // Panel background
    g.setColour (panelBg);
    g.fillRoundedRectangle (panel, 12.0f);
    g.setColour (panelBdr);
    g.drawRoundedRectangle (panel, 12.0f, 1.0f);

    // Header: "BLE Scanner" or "Ghost Scanner" for Halloween
    const float hdrY = py + 14.0f;
    g.setColour (accent);
    g.setFont (juce::Font (15.0f).boldened());
    g.drawText (cachedHalloweenMode ? "Ghost Scanner" : "BLE Scanner",
                (int) (px + 14), (int) hdrY, (int) pw - 28, 22,
                juce::Justification::centredLeft);

    // Scanning status with pulsing dot
    const float statusY = hdrY + 28.0f;
    const float dotPulse = 0.4f + 0.6f * (0.5f + 0.5f * std::sin (animPhase * 2.0f));

    // Pulsing dot
    g.setColour (accent.withAlpha (dotPulse));
    g.fillEllipse (px + 16.0f, statusY + 4.0f, 7.0f, 7.0f);

    g.setColour (muted);
    g.setFont (juce::Font (11.5f));
    g.drawText (cachedHalloweenMode ? "Detecting spirits..." : "Listening on UDP 9000...",
                (int) (px + 28), (int) statusY, (int) pw - 44, 16,
                juce::Justification::centredLeft);

    // Separator
    const float sepY = statusY + 24.0f;
    g.setColour (accent.withAlpha (0.15f));
    g.drawHorizontalLine ((int) sepY, px + 10.0f, px + pw - 10.0f);

    // Device list
    const float rowY0  = sepY + 10.0f;
    constexpr float kRowH = 26.0f;

    if (cachedDevices.empty())
    {
        const float waitAlpha = 0.35f + 0.25f * (0.5f + 0.5f * std::sin (animPhase * 1.2f));
        g.setColour (muted.withAlpha (waitAlpha));
        g.setFont (juce::Font (12.0f));
        g.drawFittedText (cachedHalloweenMode ? "Waiting for apparitions..." : "Waiting for devices...",
                          (int) (px + 14), (int) rowY0, (int) pw - 28, 60,
                          juce::Justification::centred, 2);
    }
    else
    {
        const int maxShow = std::min ((int) cachedDevices.size(), kMaxDeviceRows);
        for (int i = 0; i < maxShow; ++i)
        {
            const auto& dev = cachedDevices[(size_t) i];
            const float rowY = rowY0 + (float) i * kRowH;

            // Device name
            g.setColour (kColTextLight);
            g.setFont (juce::Font (12.0f));
            g.drawText (dev.name,
                        (int) (px + 16), (int) rowY, 150, (int) kRowH,
                        juce::Justification::centredLeft, true);

            // RSSI value (right-aligned, monospace)
            g.setColour (accent);
            g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 11.5f, 0));
            g.drawText (juce::String (dev.rssi) + "  dBm",
                        (int) (px + pw - 90), (int) rowY, 76, (int) kRowH,
                        juce::Justification::centredRight);
        }
    }

    // Bottom status message in panel
    const float bottomMsgY = panel.getBottom() - 52.0f;
    g.setColour (accent.withAlpha (0.12f));
    g.drawHorizontalLine ((int) bottomMsgY, px + 10.0f, px + pw - 10.0f);

    if (! cachedDevices.empty())
    {
        g.setColour (status.withAlpha (0.85f));
        g.setFont (juce::Font (10.5f));
        g.drawFittedText (cachedHalloweenMode 
                              ? "Paranormal activity detected!" 
                              : "Receiving BLE data - detecting nearby devices!",
                          (int) (px + 12), (int) (bottomMsgY + 6),
                          (int) pw - 24, 36,
                          juce::Justification::centredLeft, 2);
    }
    else
    {
        g.setColour (muted.withAlpha (0.5f));
        g.setFont (juce::Font (10.5f));
        g.drawFittedText ("Launch bleTones Helper to begin scanning",
                          (int) (px + 12), (int) (bottomMsgY + 6),
                          (int) pw - 24, 36,
                          juce::Justification::centredLeft, 2);
    }
}

//==============================================================================
// paintCenterContent – title, subtitle, status
//==============================================================================
void BLETonesAudioProcessorEditor::paintCenterContent (juce::Graphics& g, int W, int H)
{
    const int centerX = kPanelMargin + kLeftPanelW + kPanelMargin;
    const int centerW = W - centerX - kRightPanelW - kPanelMargin * 2;

    // Colors based on mode
    const auto orbCol   = cachedHalloweenMode ? kColHalOrange : kColBlue;
    const auto titleCol = cachedHalloweenMode ? kColHalOrange : kColGold;
    const auto muted    = cachedHalloweenMode ? kColHalMuted : kColMuted;
    const auto pillCol  = cachedHalloweenMode ? kColHalOrangeDark : kColGoldDark;

    // ── Animated orbs behind title ──────────────────────────────────────────
    for (int i = 0; i < (int) cachedDevices.size() && i < 6; ++i)
    {
        const float rssiNorm = juce::jlimit (0.0f, 1.0f,
            juce::jmap ((float) cachedDevices[(size_t) i].rssi, kRssiMin, kRssiMax, 0.0f, 1.0f));
        const float pulse  = 0.5f + 0.5f * std::sin (animPhase + (float) i * 1.5f);
        const float radius = 30.0f + rssiNorm * 60.0f;
        const float cx     = (float) centerX + (float) centerW * ((float) (i + 1) / ((float) cachedDevices.size() + 1.0f));
        const float cy     = (float) H * 0.38f;

        // Halloween mode: alternate between orange and purple orbs
        const auto thisOrbCol = (cachedHalloweenMode && (i % 2 == 1)) ? kColHalPurple : orbCol;
        juce::ColourGradient orb (thisOrbCol.withAlpha (0.12f * pulse),
                                   cx, cy,
                                   juce::Colours::transparentBlack,
                                   cx + radius, cy, true);
        g.setGradientFill (orb);
        g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    }

    // ── Title ────────────────────────────────────────────────────
    const float titleY = (float) H * 0.28f;
    const juce::String titleText = cachedHalloweenMode ? "b o o T o n e s" : "b l e T o n e s";

    // Glow pass
    g.setColour (titleCol.withAlpha (0.06f));
    g.setFont (juce::Font (52.0f).withExtraKerningFactor (0.25f));
    g.drawFittedText (titleText,
                      centerX, (int) (titleY - 4), centerW, 60,
                      juce::Justification::centred, 1);

    // Main title
    g.setColour (titleCol);
    g.setFont (juce::Font (48.0f).withExtraKerningFactor (0.25f));
    g.drawFittedText (titleText,
                      centerX, (int) titleY, centerW, 56,
                      juce::Justification::centred, 1);

    // ── Subtitle ─────────────────────────────────────────────────────────────
    g.setColour (muted);
    g.setFont (juce::Font (10.0f).withExtraKerningFactor (0.22f));
    g.drawFittedText (cachedHalloweenMode 
                          ? "S P O O K Y   S O U N D   E X P E R I E N C E" 
                          : "GENERATIVE  SOUND  EXPERIENCE",
                      centerX, (int) (titleY + 62), centerW, 16,
                      juce::Justification::centred, 1);

    // ── Status indicator ────────────────────────────────────────────────────
    const float statusY = titleY + 110.0f;
    const float statusW = 180.0f;
    const float statusH = 42.0f;
    const float statusX = (float) centerX + ((float) centerW - statusW) / 2.0f;

    // Button-like status pill
    if (! cachedDevices.empty())
    {
        // Playing state – themed filled pill
        g.setColour (pillCol.withAlpha (0.35f));
        g.fillRoundedRectangle (statusX, statusY, statusW, statusH, statusH / 2.0f);
        g.setColour (titleCol.withAlpha (0.5f));
        g.drawRoundedRectangle (statusX, statusY, statusW, statusH, statusH / 2.0f, 1.0f);

        const float playPulse = 0.7f + 0.3f * (0.5f + 0.5f * std::sin (animPhase * 1.5f));
        g.setColour (titleCol.withAlpha (playPulse));
        g.setFont (juce::Font (14.0f).withExtraKerningFactor (0.15f).boldened());
        g.drawFittedText (cachedHalloweenMode ? "HAUNTING..." : "PLAYING...",
                          (int) statusX, (int) statusY, (int) statusW, (int) statusH,
                          juce::Justification::centred, 1);
    }
    else
    {
        // Idle state
        g.setColour (muted.withAlpha (0.2f));
        g.fillRoundedRectangle (statusX, statusY, statusW, statusH, statusH / 2.0f);
        g.setColour (muted.withAlpha (0.3f));
        g.drawRoundedRectangle (statusX, statusY, statusW, statusH, statusH / 2.0f, 1.0f);

        g.setColour (muted.withAlpha (0.6f));
        g.setFont (juce::Font (14.0f).withExtraKerningFactor (0.15f));
        g.drawFittedText (cachedHalloweenMode ? "DORMANT" : "IDLE",
                          (int) statusX, (int) statusY, (int) statusW, (int) statusH,
                          juce::Justification::centred, 1);
    }
}

//==============================================================================
// paintSoundStatsPanel – right sidebar
//==============================================================================
void BLETonesAudioProcessorEditor::paintSoundStatsPanel (juce::Graphics& g, int W, int /*H*/)
{
    const float px = (float) (W - kPanelMargin - kRightPanelW);
    const float py = (float) kPanelMargin;
    const float pw = (float) kRightPanelW;
    const float ph = 210.0f;
    const juce::Rectangle<float> panel (px, py, pw, ph);

    // Colors based on mode
    const auto panelBg  = cachedHalloweenMode ? kColHalPanel : kColPanel;
    const auto panelBdr = cachedHalloweenMode ? kColHalPanelB : kColPanelB;
    const auto muted    = cachedHalloweenMode ? kColHalMuted : kColMuted;

    // Panel background
    g.setColour (panelBg);
    g.fillRoundedRectangle (panel, 12.0f);
    g.setColour (panelBdr);
    g.drawRoundedRectangle (panel, 12.0f, 1.0f);

    // Header
    g.setColour (kColTextLight);
    g.setFont (juce::Font (14.0f).boldened());
    g.drawText (cachedHalloweenMode ? "Spooky Stats" : "Sound Stats",
                (int) (px + 14), (int) (py + 12), (int) pw - 28, 20,
                juce::Justification::centredLeft);

    // Separator
    g.setColour (muted.withAlpha (0.15f));
    g.drawHorizontalLine ((int) (py + 38), px + 10.0f, px + pw - 10.0f);

    // Stats rows
    const float rowX  = px + 14.0f;
    const float valX  = px + pw - 14.0f;
    constexpr float rowH = 26.0f;
    float rowY = py + 48.0f;

    auto drawStatRow = [&] (const juce::String& label, const juce::String& value)
    {
        g.setColour (muted);
        g.setFont (juce::Font (12.0f));
        g.drawText (label, (int) rowX, (int) rowY, 120, (int) rowH,
                    juce::Justification::centredLeft);

        g.setColour (kColTextLight);
        g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 13.0f, 0));
        g.drawText (value, (int) rowX, (int) rowY, (int) (valX - rowX), (int) rowH,
                    juce::Justification::centredRight);
        rowY += rowH;
    };

    drawStatRow (cachedHalloweenMode ? "Active Spirits:" : "Active Voices:", juce::String (cachedActiveVoices));
    drawStatRow (cachedHalloweenMode ? "Entities:" : "Devices:", juce::String ((int) cachedDevices.size()));
    drawStatRow ("OSC Port:", "9000");

    // Current key display
    {
        const int midi   = static_cast<int> (*audioProcessor.apvts.getRawParameterValue ("rootNote"));
        const int octave = (midi / 12) - 1;
        drawStatRow ("Key:", juce::String (kNoteNames[midi % 12]) + juce::String (octave));
    }

    // Current scale display
    {
        const int scaleIdx = static_cast<int> (*audioProcessor.apvts.getRawParameterValue ("scaleType"));
        const auto& names  = BLETonesAudioProcessor::getScaleNames();
        drawStatRow ("Scale:", names[juce::jlimit (0, names.size() - 1, scaleIdx)]);
    }
}

//==============================================================================
// paintBottomBar
//==============================================================================
void BLETonesAudioProcessorEditor::paintBottomBar (juce::Graphics& g, int W, int H)
{
    const float barY = (float) (H - kBottomBarH);
    const auto muted = cachedHalloweenMode ? kColHalMuted : kColMuted;

    // Subtle top border
    g.setColour (muted.withAlpha (0.1f));
    g.drawHorizontalLine ((int) barY, 0.0f, (float) W);

    // Info text
    g.setColour (muted.withAlpha (0.6f));
    g.setFont (juce::Font (11.0f));
    g.drawFittedText (cachedHalloweenMode 
                          ? "Spirits manifest sounds as they drift through the airwaves..."
                          : "BLE devices generate sounds based on signal strength changes",
                      0, (int) barY + 10, W, 16, juce::Justification::centred, 1);

    g.setColour (muted.withAlpha (0.45f));
    g.setFont (juce::Font (10.5f));
    g.drawFittedText ("Receives OSC from bleTones Helper on localhost:9000",
                      0, (int) barY + 30, W, 16, juce::Justification::centred, 1);
}

//==============================================================================
// resized
//==============================================================================
void BLETonesAudioProcessorEditor::resized()
{
    const int W = getWidth();
    const int H = getHeight();

    // Sliders and combos live in the center column, below the title area
    const int centerX = kPanelMargin + kLeftPanelW + kPanelMargin;
    const int centerW = W - centerX - kRightPanelW - kPanelMargin * 2;

    // Position controls in the lower-center area
    const int sliderAreaY = (int) ((float) H * 0.60f);
    const int ctrlW = std::min (centerW - 20, 320);
    const int ctrlX = centerX + (centerW - ctrlW) / 2;
    constexpr int kLabelW = 90;
    constexpr int kRowH   = 28;
    constexpr int kGap    = 8;

    auto makeRow = [&] (int y) {
        return juce::Rectangle<int> (ctrlX, y, ctrlW, kRowH);
    };

    int y = sliderAreaY;

    // Halloween Mode toggle row (centered)
    halloweenToggle.setBounds (ctrlX + (ctrlW - 180) / 2, y, 180, kRowH);
    y += kRowH + kGap;

    // Key combo row
    auto keyRow = makeRow (y);
    keyLabel.setBounds (keyRow.removeFromLeft (kLabelW));
    keyCombo.setBounds (keyRow);
    y += kRowH + kGap;

    // Scale combo row
    auto scaleRow = makeRow (y);
    scaleLabel.setBounds (scaleRow.removeFromLeft (kLabelW));
    scaleCombo.setBounds (scaleRow);
    y += kRowH + kGap;

    // Volume slider row
    auto volRow = makeRow (y);
    volumeLabel.setBounds (volRow.removeFromLeft (kLabelW));
    volumeSlider.setBounds (volRow);
    y += kRowH + kGap;

    // Sensitivity slider row
    auto senRow = makeRow (y);
    sensitivityLabel.setBounds (senRow.removeFromLeft (kLabelW));
    sensitivitySlider.setBounds (senRow);

    // Regenerate network nodes if window was resized
    generateNetNodes (W, H);
}

//==============================================================================
// timerCallback  (15 Hz)
//==============================================================================
void BLETonesAudioProcessorEditor::timerCallback()
{
    cachedDevices      = audioProcessor.getDevicesCopy();
    cachedActiveVoices = audioProcessor.getActiveVoiceCount();

    // Check if Halloween mode changed
    const bool halloween = audioProcessor.isHalloweenMode();
    if (halloween != cachedHalloweenMode)
    {
        cachedHalloweenMode = halloween;
        updateLookAndFeelForMode (halloween);
    }

    // Advance animation phase (~3-second cycle at 15 Hz)
    animPhase += juce::MathConstants<float>::twoPi / kAnimCycleFrames;
    if (animPhase > juce::MathConstants<float>::twoPi)
        animPhase -= juce::MathConstants<float>::twoPi;

    repaint();
}
