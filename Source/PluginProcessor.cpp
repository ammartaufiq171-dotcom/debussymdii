#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
const char* kParamIds[] = { "key","mode","style","part","meter","rate","bars",
                            "density","color","velocity","humanize","legato","octave","seed" };

// A soft, bell-like "piano" so you can audition the MIDI without loading another instrument.
struct PianoSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

struct PianoVoice : public juce::SynthesiserVoice
{
    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<PianoSound*> (s) != nullptr; }

    void startNote (int note, float vel, juce::SynthesiserSound*, int) override
    {
        const double sr = getSampleRate();
        delta = juce::MathConstants<double>::twoPi * 440.0 * std::pow (2.0, (note - 69) / 12.0) / sr;
        phase = 0.0; attack = 0.0f; env = 1.0f; releasing = false;
        level = vel * 0.26f;
        attackInc = (float) (1.0 / (0.004 * sr));
        const double tau = 1.2 + juce::jlimit (0.0, 60.0, 90.0 - (double) note) * 0.045;
        decayCoef = (float) std::exp (-1.0 / (tau * sr));
        relCoef   = (float) std::exp (-1.0 / (0.30 * sr));
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff) releasing = true;
        else { env = 0.0f; clearCurrentNote(); }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        for (int i = 0; i < numSamples; ++i)
        {
            env *= decayCoef;
            if (releasing) env *= relCoef;
            attack = std::min (1.0f, attack + attackInc);
            const float e2 = env * env;
            const float s = (float) (std::sin (phase) + 0.38 * std::sin (2.0 * phase) * e2
                                     + 0.14 * std::sin (3.0 * phase) * e2 * e2);
            const float v = s * env * attack * level;
            for (int ch = 0; ch < out.getNumChannels(); ++ch)
                out.addSample (ch, startSample + i, v);
            phase += delta;
            if (phase > juce::MathConstants<double>::twoPi) phase -= juce::MathConstants<double>::twoPi;
            if (env < 0.0008f) { clearCurrentNote(); break; }
        }
    }

    double phase = 0.0, delta = 0.0;
    float env = 0.0f, attack = 0.0f, attackInc = 0.01f, level = 0.2f, decayCoef = 0.9999f, relCoef = 0.999f;
    bool releasing = false;
};
}

juce::AudioProcessorValueTreeState::ParameterLayout AtelierProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto choice = [&] (const char* id, const char* name, const StringArray& items, int def)
    { layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def)); };
    auto flt = [&] (const char* id, const char* name, float def)
    { layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                                                         NormalisableRange<float> (0.0f, 1.0f, 0.001f), def)); };

    choice ("key",   "Key",    dbs::keyNames(),   1);   // Db: the key of Clair de Lune
    choice ("mode",  "Mode",   dbs::modeNames(),  0);
    choice ("style", "Style",  dbs::styleNames(), 0);
    choice ("part",  "Part",   dbs::partNames(),  6);
    choice ("meter", "Meter",  dbs::meterNames(), 0);
    choice ("rate",  "Rhythm", dbs::rateNames(),  1);
    choice ("bars",  "Bars",   dbs::barNames(),   2);

    flt ("density",  "Density",  0.70f);
    flt ("color",    "Colour",   0.55f);
    flt ("velocity", "Dynamics", 0.60f);
    flt ("humanize", "Humanise", 0.30f);
    flt ("legato",   "Pedal",    0.50f);

    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "octave", 1 }, "Octave", -2, 2, 0));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "seed", 1 }, "Seed", 1, 99999, 1));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "audition", 1 }, "Audition", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "sound", 1 }, "Built-in Sound", true));
    return layout;
}

AtelierProcessor::AtelierProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pending.reserve (2048);
    for (int i = 0; i < 24; ++i) synth.addVoice (new PianoVoice());
    synth.addSound (new PianoSound());

    for (auto* id : kParamIds) apvts.addParameterListener (id, this);
    regenerate();
}

AtelierProcessor::~AtelierProcessor()
{
    cancelPendingUpdate();
    for (auto* id : kParamIds) apvts.removeParameterListener (id, this);
}

void AtelierProcessor::prepareToPlay (double sr, int)
{
    currentSampleRate = sr > 0 ? sr : 44100.0;
    synth.setCurrentPlaybackSampleRate (currentSampleRate);
    pending.clear(); wasActive = false; internalPpq = 0.0;
}

bool AtelierProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

// ------------------------------------------------------------------ generation
dbs::Settings AtelierProcessor::readSettings() const
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    dbs::Settings s;
    static const int barOpts[4] = { 1, 2, 4, 8 };
    s.key    = (int) std::lround (get ("key"));
    s.mode   = (int) std::lround (get ("mode"));
    s.style  = (int) std::lround (get ("style"));
    s.part   = (int) std::lround (get ("part"));
    s.meter  = (int) std::lround (get ("meter"));
    s.rate   = (int) std::lround (get ("rate"));
    s.bars   = barOpts[juce::jlimit (0, 3, (int) std::lround (get ("bars")))];
    s.density = get ("density"); s.color = get ("color"); s.velocity = get ("velocity");
    s.humanize = get ("humanize"); s.legato = get ("legato");
    s.octave = (int) std::lround (get ("octave"));
    s.seed   = (int) std::lround (get ("seed"));
    return s;
}

void AtelierProcessor::regenerate()
{
    auto c = dbs::Generator::generate (readSettings());
    const juce::ScopedLock sl (clipLock);
    clip = std::move (c);
}

void AtelierProcessor::handleAsyncUpdate()
{
    regenerate();
    clipChanged.sendChangeMessage();
}

void AtelierProcessor::newSeed()
{
    if (auto* p = apvts.getParameter ("seed"))
    {
        const int v = juce::Random::getSystemRandom().nextInt (99999) + 1;
        p->setValueNotifyingHost (p->convertTo0to1 ((float) v));
    }
}

dbs::Clip AtelierProcessor::getClip() const
{
    const juce::ScopedLock sl (clipLock);
    return clip;
}

// ------------------------------------------------------------------ MIDI files
juce::String AtelierProcessor::suggestedFileName() const
{
    const auto s = readSettings();
    auto name = "Debussy " + dbs::keyNames()[s.key] + " " + dbs::modeNames()[s.mode] + " "
              + dbs::partNames()[s.part] + " " + juce::String (s.seed) + ".mid";
    return juce::File::createLegalFileName (name).replaceCharacter (' ', '_');
}

bool AtelierProcessor::writeMidiFileTo (const juce::File& f) const
{
    const auto c = getClip();
    if (c.notes.empty()) return false;

    const int tpq = 960;
    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (c.tsNum, c.tsDen));
    seq.addEvent (juce::MidiMessage::tempoMetaEvent (500000));   // 120 bpm; your DAW will follow its own tempo
    for (auto& n : c.notes)
    {
        auto on = juce::MidiMessage::noteOn (1, n.pitch, (juce::uint8) n.velocity);
        on.setTimeStamp (n.start * tpq);
        auto off = juce::MidiMessage::noteOff (1, n.pitch);
        off.setTimeStamp ((n.start + n.length) * tpq - 1.0);
        seq.addEvent (on);
        seq.addEvent (off);
    }
    seq.sort();
    seq.updateMatchedPairs();

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (seq);

    f.getParentDirectory().createDirectory();
    f.deleteFile();
    juce::FileOutputStream os (f);
    if (! os.openedOk()) return false;
    return mf.writeTo (os);
}

juce::File AtelierProcessor::writeMidiFile() const
{
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory)
                 .getChildFile ("AtelierDebussy").getChildFile (suggestedFileName());
    return writeMidiFileTo (f) ? f : juce::File();
}

// ------------------------------------------------------------------ real-time
void AtelierProcessor::flushPending (juce::MidiBuffer& midi, int sample)
{
    for (auto& p : pending) midi.addEvent (juce::MidiMessage::noteOff (1, p.pitch), sample);
    pending.clear();
}

void AtelierProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    buffer.clear();
    const int numSamples = buffer.getNumSamples();

    const bool audition = apvts.getRawParameterValue ("audition")->load() > 0.5f;
    const bool sound    = apvts.getRawParameterValue ("sound")->load() > 0.5f;

    double bpm = 120.0, hostPpq = 0.0;
    bool hostPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto p = pos->getPpqPosition()) { hostPpq = *p; hostPlaying = pos->getIsPlaying(); }
        }

    const double bps = bpm / 60.0 / currentSampleRate;   // beats per sample
    bool active = false;
    double start = 0.0;
    if (hostPlaying)   { active = true; start = hostPpq; }
    else if (audition) { active = true; start = internalPpq; }
    const double end = start + numSamples * bps;

    if (! active)
    {
        flushPending (midi, 0);
        wasActive = false;
        internalPpq = 0.0;
        playheadBeat.store (-1.0);
    }
    else
    {
        if (! wasActive || std::abs (start - lastEnd) > 0.02)
            flushPending (midi, 0);

        const juce::ScopedTryLock sl (clipLock);
        if (sl.isLocked() && ! clip.notes.empty() && clip.length > 0.0)
        {
            const double L = clip.length;
            const double base = std::floor (start / L) * L;
            for (int k = 0; k < 2; ++k)
                for (auto& n : clip.notes)
                {
                    const double t = base + k * L + n.start;
                    if (t < start || t >= end) continue;
                    const int off = juce::jlimit (0, numSamples - 1, (int) ((t - start) / bps));

                    // retrigger of a still-sounding pitch: close the old one first
                    for (size_t i = 0; i < pending.size();)
                        if (pending[i].pitch == n.pitch && pending[i].beat >= t)
                        { midi.addEvent (juce::MidiMessage::noteOff (1, n.pitch), off); pending.erase (pending.begin() + (long) i); }
                        else ++i;

                    midi.addEvent (juce::MidiMessage::noteOn (1, n.pitch, (juce::uint8) n.velocity), off);
                    pending.push_back ({ n.pitch, t + n.length });
                }
            playheadBeat.store (std::fmod (start, L));
        }

        for (size_t i = 0; i < pending.size();)
            if (pending[i].beat < end)
            {
                const int off = juce::jlimit (0, numSamples - 1, (int) ((pending[i].beat - start) / bps));
                midi.addEvent (juce::MidiMessage::noteOff (1, pending[i].pitch), off);
                pending.erase (pending.begin() + (long) i);
            }
            else ++i;

        if (! hostPlaying) internalPpq = end;
        lastEnd = end;
        wasActive = true;
    }

    if (sound) synth.renderNextBlock (buffer, midi, 0, numSamples);
}

// ------------------------------------------------------------------ state / editor
juce::AudioProcessorEditor* AtelierProcessor::createEditor() { return new AtelierEditor (*this); }

void AtelierProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void AtelierProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
    triggerAsyncUpdate();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AtelierProcessor(); }
