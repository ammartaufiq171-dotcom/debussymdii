#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ============================================================================
//  Palette taken from Van Gogh's "Starry Night"
// ============================================================================
namespace pal
{
    inline juce::Colour gold()   { return juce::Colour (0xffe9c46a); }
    inline juce::Colour cream()  { return juce::Colour (0xfff6e7b4); }
    inline juce::Colour navy()   { return juce::Colour (0xff0b1a3d); }
    inline juce::Colour deep()   { return juce::Colour (0xff070f2a); }
    inline juce::Colour cobalt() { return juce::Colour (0xff2d5aa8); }
    inline juce::Colour sky()    { return juce::Colour (0xff6fa8dc); }
}

// ============================================================================
class ImpressionLNF : public juce::LookAndFeel_V4
{
public:
    ImpressionLNF()
    {
        setColour (juce::ComboBox::textColourId, pal::cream());
        setColour (juce::ComboBox::arrowColourId, pal::gold());
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xf20b1a3d));
        setColour (juce::PopupMenu::textColourId, pal::cream());
        setColour (juce::PopupMenu::highlightedBackgroundColourId, pal::cobalt());
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xffffe9a0));
        setColour (juce::TextButton::textColourOffId, pal::navy());
        setColour (juce::TextButton::textColourOnId, pal::navy());
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xe014295e), 0, 0, juce::Colour (0xe00a1638), 0, (float) h, false));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (pal::gold().withAlpha (box.isEnabled() ? 0.75f : 0.3f));
        g.drawRoundedRectangle (r, 8.0f, 1.2f);
        juce::Path p;
        const float cx = (float) w - 18.0f, cy = (float) h * 0.5f;
        p.addTriangle (cx - 5, cy - 2.5f, cx + 5, cy - 2.5f, cx, cy + 3.5f);
        g.setColour (pal::gold());
        g.fillPath (p);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (15.0f); }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (10, 1, box.getWidth() - 34, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto c = bounds.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);
        const float disc = radius * 0.76f;

        // glow
        juce::ColourGradient glow (pal::gold().withAlpha (0.28f), c.x, c.y, juce::Colours::transparentBlack, c.x + radius * 1.35f, c.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (c.x - radius * 1.35f, c.y - radius * 1.35f, radius * 2.7f, radius * 2.7f);

        // painted disc
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a80d4), c.x - disc * 0.5f, c.y - disc * 0.6f,
                                                 juce::Colour (0xff0a1844), c.x + disc * 0.6f, c.y + disc * 0.8f, false));
        g.fillEllipse (c.x - disc, c.y - disc, disc * 2, disc * 2);

        // swirling brush strokes that rotate with the value
        for (int i = 0; i < 5; ++i)
        {
            const float rr = disc * (0.22f + 0.15f * (float) i);
            const float a0 = angle * 0.35f + (float) i * 1.3f;
            juce::Path sp;
            sp.addCentredArc (c.x, c.y, rr, rr, 0.0f, a0, a0 + 2.4f, true);
            g.setColour (i % 2 ? pal::sky().withAlpha (0.75f) : pal::cream().withAlpha (0.5f));
            g.strokePath (sp, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // track + value arc
        juce::Path track;
        track.addCentredArc (c.x, c.y, radius, radius, 0.0f, startAngle, endAngle, true);
        g.setColour (pal::cream().withAlpha (0.22f));
        g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path val;
        val.addCentredArc (c.x, c.y, radius, radius, 0.0f, startAngle, angle, true);
        g.setColour (pal::gold());
        g.strokePath (val, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // pointer
        const float s = std::sin (angle), co = std::cos (angle);
        g.setColour (pal::cream());
        g.drawLine (c.x + s * disc * 0.35f, c.y - co * disc * 0.35f, c.x + s * disc * 0.95f, c.y - co * disc * 0.95f, 2.6f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::Font (15.0f, juce::Font::bold); }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.0f);
        const float bright = down ? -0.15f : (over ? 0.12f : 0.0f);
        g.setGradientFill (juce::ColourGradient (pal::gold().brighter (0.25f + bright), 0, r.getY(),
                                                 juce::Colour (0xffd08a2e).brighter (bright), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (juce::Colour (0xfffff2b0).withAlpha (0.6f));
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.2f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool) override
    {
        const bool on = b.getToggleState();
        const float h = (float) b.getHeight();
        juce::Rectangle<float> star (2.0f, 1.0f, h - 2.0f, h - 2.0f);
        const auto c = star.getCentre();
        if (on)
        {
            g.setGradientFill (juce::ColourGradient (pal::gold().withAlpha (0.55f), c.x, c.y, juce::Colours::transparentBlack, c.x + h * 0.9f, c.y, true));
            g.fillEllipse (c.x - h * 0.9f, c.y - h * 0.9f, h * 1.8f, h * 1.8f);
        }
        juce::Path st;
        st.addStar (c, 6, h * 0.14f, h * 0.42f, 0.0f);
        g.setColour (on ? pal::cream() : pal::cream().withAlpha (over ? 0.6f : 0.35f));
        if (on) g.fillPath (st); else g.strokePath (st, juce::PathStrokeType (1.4f));
        g.setColour (on ? pal::cream() : pal::cream().withAlpha (0.7f));
        g.setFont (juce::Font (14.0f));
        g.drawText (b.getButtonText(), (int) h + 6, 0, b.getWidth() - (int) h - 6, b.getHeight(), juce::Justification::centredLeft);
    }
};

// ============================================================================
//  Piano-roll preview of the generated clip
// ============================================================================
class RollView : public juce::Component
{
public:
    explicit RollView (AtelierProcessor& p) : proc (p) {}

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0x88040a1c));
        g.fillRoundedRectangle (b, 6.0f);

        const auto clip = proc.getClip();
        if (clip.notes.empty()) return;

        int lo = 127, hi = 0;
        for (auto& n : clip.notes) { lo = std::min (lo, n.pitch); hi = std::max (hi, n.pitch); }
        lo -= 2; hi += 2;
        if (hi - lo < 24) { const int mid = (hi + lo) / 2; lo = mid - 12; hi = mid + 12; }
        const float rowH = b.getHeight() / (float) (hi - lo + 1);
        const float ppb  = b.getWidth() / (float) clip.length;

        for (int p = lo; p <= hi; ++p)
        {
            const int pc = ((p % 12) + 12) % 12;
            if (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10)
            {
                g.setColour (juce::Colour (0x22000000));
                g.fillRect (0.0f, b.getHeight() - (float) (p - lo + 1) * rowH, b.getWidth(), rowH);
            }
        }
        for (double beat = 0; beat <= clip.length + 1e-6; beat += 1.0)
        {
            const bool isBar = std::fmod (beat, clip.barBeats) < 1e-6;
            g.setColour (pal::cream().withAlpha (isBar ? 0.28f : 0.07f));
            g.fillRect ((float) beat * ppb, 0.0f, isBar ? 1.5f : 1.0f, b.getHeight());
        }

        for (auto& n : clip.notes)
        {
            const float x = (float) n.start * ppb;
            const float w = std::max (4.0f, (float) n.length * ppb - 1.0f);
            const float y = (float) (hi - n.pitch) * rowH;
            auto col = juce::Colour (0xff4f86d9).interpolatedWith (juce::Colour (0xffffd86b), (float) n.velocity / 127.0f);
            g.setColour (col.withAlpha (0.28f));
            g.fillRoundedRectangle (x - 1.5f, y - 0.5f, w + 3.0f, rowH + 1.0f, 3.0f);
            g.setColour (col);
            g.fillRoundedRectangle (x, y + 1.0f, w, std::max (2.0f, rowH - 2.0f), 2.5f);
        }

        const double ph = proc.playheadBeat.load();
        if (ph >= 0.0)
        {
            g.setColour (pal::cream().withAlpha (0.9f));
            g.fillRect ((float) ph * ppb, 0.0f, 2.0f, b.getHeight());
        }
    }

private:
    AtelierProcessor& proc;
};

// ============================================================================
//  Drag this onto any MIDI track in your DAW
// ============================================================================
class DragPad : public juce::Component
{
public:
    explicit DragPad (AtelierProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.5f);
        g.setColour (juce::Colour (0x55e9c46a));
        g.fillRoundedRectangle (r, 10.0f);
        const float dashes[] = { 6.0f, 4.0f };
        juce::Path rp; rp.addRoundedRectangle (r, 10.0f);
        juce::Path dashed;
        juce::PathStrokeType (1.6f).createDashedStroke (dashed, rp, dashes, 2);
        g.setColour (pal::gold());
        g.fillPath (dashed);

        // tiny note glyphs
        for (int i = 0; i < 4; ++i)
        {
            g.setColour (pal::cream().withAlpha (0.9f));
            g.fillRoundedRectangle (14.0f + (float) i * 9.0f, 30.0f - (float) ((i * 7) % 15), 6.0f, 4.0f, 1.5f);
        }
        g.setColour (pal::cream());
        g.setFont (juce::Font (15.0f, juce::Font::bold));
        g.drawText ("Drag MIDI to DAW", 56, 6, getWidth() - 62, 22, juce::Justification::centredLeft);
        g.setFont (juce::Font (12.0f));
        g.setColour (pal::cream().withAlpha (0.7f));
        g.drawText ("drop onto any track", 56, 26, getWidth() - 62, 18, juce::Justification::centredLeft);
    }

    void mouseDown (const juce::MouseEvent&) override { dragging = false; }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 6) return;
        dragging = true;
        const auto f = proc.writeMidiFile();
        if (f.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }

private:
    AtelierProcessor& proc;
    bool dragging = false;
};

// ============================================================================
class AtelierEditor : public juce::AudioProcessorEditor,
                      private juce::Timer,
                      private juce::ChangeListener
{
public:
    explicit AtelierEditor (AtelierProcessor&);
    ~AtelierEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override { roll.repaint(); }
    void drawPanel (juce::Graphics&, juce::Rectangle<int>, bool gilded = false);
    void drawStar (juce::Graphics&, float cx, float cy, float R, float pulse, bool moon);
    void rebuildBackground();

    AtelierProcessor& proc;
    ImpressionLNF lnf;

    using CBA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SLA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BTA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct ComboRow { juce::ComboBox box; std::unique_ptr<CBA> att; };
    struct KnobBox  { juce::Slider slider; std::unique_ptr<SLA> att; };
    std::array<ComboRow, 7> combos;
    std::array<KnobBox, 6> knobs;

    RollView roll;
    DragPad pad;
    juce::TextButton newBtn { "Nouvelle esquisse" }, exportBtn { "Export .mid" };
    juce::ToggleButton auditionTgl { "Audition loop" }, soundTgl { "Built-in piano" };
    std::unique_ptr<BTA> auditionAtt, soundAtt;
    std::unique_ptr<juce::FileChooser> chooser;

    struct Star { float fx, fy, R, phase, speed; bool moon; };
    std::vector<Star> stars;
    double starTime = 0.0, lastPlayhead = -2.0;

    juce::Image bg;
    juce::Rectangle<int> leftPanel, rollFrame, knobPanel, barPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AtelierEditor)
};
