#include "PluginEditor.h"

namespace
{
using juce::Colour;

// Procedurally "paints" a Starry-Night-like sky: thousands of short brush strokes
// following a flow field with a few big vortices, then a dark village and cypress.
void paintStarryNight (juce::Image& img)
{
    const int w = img.getWidth(), h = img.getHeight();
    juce::Graphics g (img);

    g.setGradientFill (juce::ColourGradient (Colour (0xff08122f), 0, 0, Colour (0xff1b3a7a), 0, (float) h * 0.85f, false));
    g.fillAll();

    struct V { float x, y, R, s; };
    const std::vector<V> vs {
        { w * 0.60f, h * 0.30f, h * 0.20f,  1.0f },
        { w * 0.34f, h * 0.22f, h * 0.14f, -1.0f },
        { w * 0.82f, h * 0.46f, h * 0.12f,  1.0f },
        { w * 0.12f, h * 0.36f, h * 0.10f, -1.0f } };

    auto field = [&] (float x, float y, float& ix) -> juce::Point<float>
    {
        const float a = 0.5f * std::sin (x * 0.009f + y * 0.004f) + 0.5f * std::cos (y * 0.012f);
        float vx = std::cos (a), vy = std::sin (a) * 0.6f;
        ix = 0.0f;
        for (auto& v : vs)
        {
            const float dx = x - v.x, dy = y - v.y;
            const float d2 = dx * dx + dy * dy;
            const float r = std::sqrt (d2) + 1e-3f;
            const float wgt = std::exp (-d2 / (2.0f * v.R * v.R));
            ix = std::max (ix, wgt);
            vx += v.s * (-dy / r) * wgt * 3.0f;
            vy += v.s * (dx / r) * wgt * 3.0f;
        }
        const float m = std::sqrt (vx * vx + vy * vy) + 1e-4f;
        return { vx / m, vy / m };
    };

    static const juce::uint32 dark[]   = { 0xff0e2257, 0xff16347a, 0xff1f4a9c, 0xff2a5db4 };
    static const juce::uint32 mid[]    = { 0xff3a7ac2, 0xff4a97c9, 0xff5fb0c9 };
    static const juce::uint32 bright[] = { 0xff9fd0e8, 0xffd7e7c4, 0xfff0dc8a, 0xfff6e7b4 };

    juce::Random r (1889);
    const int count = (w * h) / 70;
    for (int i = 0; i < count; ++i)
    {
        float x = r.nextFloat() * (float) w, y = r.nextFloat() * (float) h * 0.88f;
        float ix = 0.0f;
        field (x, y, ix);
        const float t = ix + (r.nextFloat() - 0.5f) * 0.35f;
        Colour col = t > 0.78f ? Colour (bright[r.nextInt (4)])
                   : t > 0.45f ? Colour (mid[r.nextInt (3)])
                   : (r.nextFloat() < 0.18f + 0.3f * (y / (float) h) ? Colour (mid[r.nextInt (3)]) : Colour (dark[r.nextInt (4)]));
        col = col.withAlpha (0.78f + r.nextFloat() * 0.22f);

        juce::Path p;
        p.startNewSubPath (x, y);
        for (int k = 0; k < 14; ++k)
        {
            const auto d = field (x, y, ix);
            x += d.x * 5.5f; y += d.y * 5.5f;
            p.lineTo (x, y);
        }
        g.setColour (col);
        g.strokePath (p, juce::PathStrokeType (3.0f + r.nextFloat() * 3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // hills
    const float hy = (float) h * 0.86f;
    juce::Path hills;
    hills.startNewSubPath (0.0f, (float) h);
    for (float x = 0; x <= (float) w; x += 8.0f)
        hills.lineTo (x, hy + 10.0f * std::sin (x * 0.01f) + 8.0f * std::sin (x * 0.027f + 1.0f));
    hills.lineTo ((float) w, (float) h);
    hills.closeSubPath();
    g.setColour (Colour (0xff060d22));
    g.fillPath (hills);

    // village with lit windows
    for (int i = 0; i < 17; ++i)
    {
        const float x = (float) w * 0.26f + (float) i * (float) w * 0.037f + r.nextFloat() * 8.0f;
        const float bw = 16.0f + r.nextFloat() * 14.0f, bh = 14.0f + r.nextFloat() * 16.0f;
        const float y = hy + 2.0f + 8.0f * std::sin (x * 0.01f) - bh + 14.0f;
        g.setColour (Colour (0xff0a1430));
        g.fillRect (x, y, bw, bh);
        juce::Path roof;
        roof.addTriangle (x - 2.0f, y, x + bw + 2.0f, y, x + bw * 0.5f, y - 8.0f);
        g.fillPath (roof);
        if (r.nextFloat() < 0.7f)
        {
            g.setColour (Colour (0xfff0c85a));
            g.fillRect (x + bw * 0.3f, y + bh * 0.35f, 3.5f, 4.5f);
        }
    }
    juce::Path spire;
    spire.addTriangle ((float) w * 0.64f, hy - 6.0f, (float) w * 0.64f + 14.0f, hy - 6.0f, (float) w * 0.64f + 7.0f, hy - 58.0f);
    g.setColour (Colour (0xff0a1430));
    g.fillPath (spire);
    g.fillRect ((float) w * 0.64f + 1.0f, hy - 8.0f, 12.0f, 24.0f);

    // cypress
    const float cx = (float) w * 0.055f;
    juce::Path cy;
    cy.startNewSubPath (cx - 20.0f, (float) h);
    cy.cubicTo (cx - 36.0f, (float) h * 0.80f, cx - 10.0f, (float) h * 0.58f, cx - 2.0f, (float) h * 0.34f);
    cy.cubicTo (cx + 6.0f, (float) h * 0.56f, cx + 40.0f, (float) h * 0.78f, cx + 22.0f, (float) h);
    cy.closeSubPath();
    g.setColour (Colour (0xff04100f));
    g.fillPath (cy);
    for (int i = 0; i < 40; ++i)
    {
        const float yy = (float) h * (0.40f + r.nextFloat() * 0.58f);
        const float half = 6.0f + 24.0f * ((yy / (float) h) - 0.40f);
        const float xx = cx + (r.nextFloat() - 0.5f) * half * 1.4f;
        juce::Path s;
        s.startNewSubPath (xx, yy);
        s.cubicTo (xx + 4.0f, yy - 14.0f, xx - 5.0f, yy - 26.0f, xx + 2.0f, yy - 40.0f);
        g.setColour (Colour (0xff0f3a33).withAlpha (0.55f));
        g.strokePath (s, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}
} // namespace

// ============================================================================
AtelierEditor::AtelierEditor (AtelierProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), roll (p), pad (p)
{
    setLookAndFeel (&lnf);

    static const char* comboIds[7] = { "key", "mode", "style", "part", "meter", "rate", "bars" };
    for (size_t i = 0; i < 7; ++i)
    {
        auto& row = combos[i];
        row.box.addItemList (proc.apvts.getParameter (comboIds[i])->getAllValueStrings(), 1);
        row.att = std::make_unique<CBA> (proc.apvts, comboIds[i], row.box);
        addAndMakeVisible (row.box);
    }

    static const char* knobIds[6] = { "density", "color", "velocity", "humanize", "legato", "octave" };
    for (size_t i = 0; i < 6; ++i)
    {
        auto& k = knobs[i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        k.slider.onValueChange = [this] { repaint (knobPanel); };
        k.att = std::make_unique<SLA> (proc.apvts, knobIds[i], k.slider);
        addAndMakeVisible (k.slider);
    }

    addAndMakeVisible (roll);
    addAndMakeVisible (pad);
    addAndMakeVisible (newBtn);
    addAndMakeVisible (exportBtn);
    addAndMakeVisible (auditionTgl);
    addAndMakeVisible (soundTgl);
    auditionAtt = std::make_unique<BTA> (proc.apvts, "audition", auditionTgl);
    soundAtt    = std::make_unique<BTA> (proc.apvts, "sound", soundTgl);

    newBtn.onClick = [this] { proc.newSeed(); };
    exportBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Export MIDI",
                     juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile (proc.suggestedFileName()),
                     "*.mid");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                              [&p = proc] (const juce::FileChooser& fc)
                              { const auto f = fc.getResult(); if (f != juce::File()) p.writeMidiFileTo (f); });
    };

    stars = { { 0.36f, 0.060f, 14.0f, 0.0f, 1.3f, false }, { 0.50f, 0.085f, 22.0f, 1.2f, 1.1f, false },
              { 0.58f, 0.045f, 16.0f, 2.1f, 1.7f, false }, { 0.66f, 0.110f, 24.0f, 0.7f, 0.9f, false },
              { 0.74f, 0.050f, 18.0f, 3.3f, 1.4f, false }, { 0.82f, 0.115f, 26.0f, 4.1f, 1.0f, false },
              { 0.38f, 0.968f, 15.0f, 2.6f, 1.2f, false }, { 0.62f, 0.972f, 18.0f, 1.0f, 1.5f, false },
              { 0.945f, 0.090f, 24.0f, 0.4f, 0.6f, true } };

    setSize (980, 620);
    setResizable (false, false);
    proc.clipChanged.addChangeListener (this);
    startTimerHz (24);
}

AtelierEditor::~AtelierEditor()
{
    stopTimer();
    proc.clipChanged.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void AtelierEditor::rebuildBackground()
{
    if (bg.isValid() && bg.getWidth() == getWidth() && bg.getHeight() == getHeight()) return;
    bg = juce::Image (juce::Image::ARGB, getWidth(), getHeight(), true);
    paintStarryNight (bg);
}

void AtelierEditor::resized()
{
    rebuildBackground();

    leftPanel = { 20, 96, 254, 500 };
    rollFrame = { 290, 96, 670, 262 };
    knobPanel = { 290, 370, 670, 146 };
    barPanel  = { 290, 528, 670, 68 };

    for (int i = 0; i < 7; ++i)
        combos[(size_t) i].box.setBounds (36, 110 + i * 62 + 18, 222, 30);

    roll.setBounds (rollFrame.reduced (12));

    const int cellW = (knobPanel.getWidth() - 20) / 6;
    for (int i = 0; i < 6; ++i)
        knobs[(size_t) i].slider.setBounds (knobPanel.getX() + 10 + i * cellW + (cellW - 74) / 2, knobPanel.getY() + 12, 74, 74);

    newBtn.setBounds (302, 540, 170, 44);
    exportBtn.setBounds (480, 540, 110, 44);
    pad.setBounds (598, 538, 206, 48);
    auditionTgl.setBounds (816, 538, 138, 24);
    soundTgl.setBounds (816, 562, 138, 24);

    // star halos change size on resize; refresh the whole thing once
    repaint();
}

void AtelierEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> r, bool gilded)
{
    auto f = r.toFloat();
    g.setColour (juce::Colour (0xb00a1636));
    g.fillRoundedRectangle (f, 14.0f);
    g.setColour (pal::gold().withAlpha (0.55f));
    g.drawRoundedRectangle (f.reduced (0.5f), 14.0f, 1.4f);
    if (gilded)
    {
        g.setColour (pal::gold().withAlpha (0.9f));
        g.drawRoundedRectangle (f.reduced (3.5f), 11.0f, 2.2f);
        g.setColour (pal::cream().withAlpha (0.35f));
        g.drawRoundedRectangle (f.reduced (7.0f), 8.0f, 1.0f);
    }
}

void AtelierEditor::drawStar (juce::Graphics& g, float cx, float cy, float R, float pulse, bool moon)
{
    const auto core = moon ? juce::Colour (0xfffff0a8) : pal::cream();
    juce::ColourGradient grad (core.withAlpha (0.95f * pulse), cx, cy,
                               juce::Colour (0xff5d8fd0).withAlpha (0.0f), cx + R * 2.2f, cy, true);
    grad.addColour (0.25, pal::gold().withAlpha (0.55f * pulse));
    grad.addColour (0.55, juce::Colour (0xff9fd0e8).withAlpha (0.22f));
    g.setGradientFill (grad);
    g.fillEllipse (cx - R * 2.2f, cy - R * 2.2f, R * 4.4f, R * 4.4f);

    for (int i = 1; i <= 3; ++i)
    {
        const float rr = R * (0.7f + 0.42f * (float) i);
        g.setColour (juce::Colour (0xfff0dc8a).withAlpha (0.38f * pulse / (float) i));
        g.drawEllipse (cx - rr, cy - rr, rr * 2.0f, rr * 2.0f, 1.6f);
    }
    g.setColour (core);
    const float cr = R * (moon ? 0.62f : 0.32f);
    g.fillEllipse (cx - cr, cy - cr, cr * 2.0f, cr * 2.0f);
}

void AtelierEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (bg, 0, 0);

    for (auto& s : stars)
    {
        const float pulse = 0.78f + 0.22f * (float) std::sin (starTime * s.speed + s.phase);
        drawStar (g, s.fx * (float) getWidth(), s.fy * (float) getHeight(), s.R, pulse, s.moon);
    }

    drawPanel (g, leftPanel);
    drawPanel (g, rollFrame, true);
    drawPanel (g, knobPanel);
    drawPanel (g, barPanel);

    // title
    const auto serif = juce::Font::getDefaultSerifFontName();
    g.setFont (juce::Font (serif, 40.0f, juce::Font::italic));
    g.setColour (juce::Colour (0xaa000000));
    g.drawText ("Atelier Debussy", 31, 15, 520, 46, juce::Justification::centredLeft);
    g.setColour (pal::gold());
    g.drawText ("Atelier Debussy", 28, 12, 520, 46, juce::Justification::centredLeft);
    g.setFont (juce::Font (serif, 14.0f, juce::Font::italic));
    g.setColour (pal::cream().withAlpha (0.9f));
    g.drawText (juce::String::fromUTF8 ("g\xc3\xa9n\xc3\xa9rateur de MIDI impressionniste"), 30, 58, 520, 22, juce::Justification::centredLeft);

    // left-panel labels
    static const char* labels[7] = { "KEY", "MODE / SCALE", "STYLE", "PART", "METER", "RHYTHM GRID", "LENGTH (BARS)" };
    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.setColour (pal::gold().withAlpha (0.9f));
    for (int i = 0; i < 7; ++i)
        g.drawText (labels[i], 38, 110 + i * 62, 220, 16, juce::Justification::centredLeft);

    g.setFont (juce::Font (serif, 12.5f, juce::Font::italic));
    g.setColour (pal::cream().withAlpha (0.65f));
    g.drawFittedText ("Tip: press the golden button for a new sketch, then drag the MIDI onto a track.",
                      36, 552, 222, 36, juce::Justification::centredLeft, 2);

    // knob captions
    static const char* names[6] = { "DENSITY", "COLOUR", "DYNAMICS", "HUMANISE", "PEDAL", "OCTAVE" };
    const int cellW = (knobPanel.getWidth() - 20) / 6;
    for (int i = 0; i < 6; ++i)
    {
        const int x = knobPanel.getX() + 10 + i * cellW;
        g.setFont (juce::Font (12.0f, juce::Font::bold));
        g.setColour (pal::gold());
        g.drawText (names[i], x, knobPanel.getY() + 92, cellW, 16, juce::Justification::centred);
        auto& s = knobs[(size_t) i].slider;
        const juce::String v = (i == 5) ? juce::String ((int) s.getValue()) : juce::String (juce::roundToInt (s.getValue() * 100.0)) + "%";
        g.setFont (juce::Font (13.0f));
        g.setColour (pal::cream().withAlpha (0.85f));
        g.drawText (v, x, knobPanel.getY() + 110, cellW, 18, juce::Justification::centred);
    }

    g.setFont (juce::Font (serif, 12.0f, juce::Font::italic));
    g.setColour (pal::cream().withAlpha (0.5f));
    g.drawText (juce::String::fromUTF8 ("d'apr\xc3\xa8s Debussy & Ravel"), 0, 600, getWidth(), 16, juce::Justification::centred);
}

void AtelierEditor::timerCallback()
{
    starTime += 1.0 / 24.0;
    for (auto& s : stars)
    {
        const int cx = (int) (s.fx * (float) getWidth()), cy = (int) (s.fy * (float) getHeight());
        const int r = (int) (s.R * 2.4f);
        repaint (cx - r, cy - r, r * 2, r * 2);
    }
    const double ph = proc.playheadBeat.load();
    if (ph != lastPlayhead) { lastPlayhead = ph; roll.repaint(); }
}
