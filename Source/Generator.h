#pragma once
#include <JuceHeader.h>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>

// ============================================================================
//  Debussy-flavoured MIDI generator.
//  Pure logic, no GUI / audio dependencies, deterministic per seed.
// ============================================================================
namespace dbs
{
struct Note { int pitch = 60; double start = 0.0; double length = 1.0; int velocity = 90; };

struct Clip
{
    std::vector<Note> notes;
    double length = 16.0;     // total beats (quarter notes)
    double barBeats = 4.0;
    int tsNum = 4, tsDen = 4;
};

struct Settings
{
    int key = 1, mode = 0, style = 0, part = 6, meter = 0, rate = 1, bars = 4;
    float density = 0.7f, color = 0.55f, velocity = 0.6f, humanize = 0.3f, legato = 0.5f;
    int octave = 0;
    int seed = 1;
};

// ---------------------------------------------------------------- data tables
inline const juce::StringArray& keyNames()
{ static const juce::StringArray a { "C","Db","D","Eb","E","F","F#","G","Ab","A","Bb","B" }; return a; }

inline const juce::StringArray& modeNames()
{ static const juce::StringArray a { "Major (Ionian)","Natural Minor","Dorian","Lydian","Mixolydian",
                                     "Phrygian","Whole Tone","Pentatonic","Octatonic","Acoustic (Lyd-Mixo)" }; return a; }

inline const std::vector<std::vector<int>>& modeTable()
{
    static const std::vector<std::vector<int>> t {
        {0,2,4,5,7,9,11}, {0,2,3,5,7,8,10}, {0,2,3,5,7,9,10}, {0,2,4,6,7,9,11}, {0,2,4,5,7,9,10},
        {0,1,3,5,7,8,10}, {0,2,4,6,8,10},   {0,2,4,7,9},      {0,2,3,5,6,8,9,11}, {0,2,4,6,7,9,10} };
    return t;
}

inline const juce::StringArray& styleNames()
{ static const juce::StringArray a { "Clair de Lune","Une barque sur l'ocean","Jeux d'eau","La cathedrale engloutie" }; return a; }

inline const juce::StringArray& partNames()
{ static const juce::StringArray a { "Arp Up","Arp Down","Arp Wave (cascade)","Main Chords","Motif / Melody","Bass Pedal","Full Texture" }; return a; }

inline const juce::StringArray& meterNames()
{ static const juce::StringArray a { "4/4","3/4","6/8","9/8","12/8" }; return a; }

inline const juce::StringArray& rateNames()
{ static const juce::StringArray a { "1/4","1/8","1/8 Triplet","1/16","1/16 Triplet" }; return a; }

inline const juce::StringArray& barNames()
{ static const juce::StringArray a { "1","2","4","8" }; return a; }

struct MeterDef { double beats; int num; int den; };
inline const std::array<MeterDef, 5>& meters()
{ static const std::array<MeterDef, 5> m {{ {4.0,4,4}, {3.0,3,4}, {3.0,6,8}, {4.5,9,8}, {6.0,12,8} }}; return m; }

inline const std::array<double, 5>& rates()
{ static const std::array<double, 5> r {{ 1.0, 0.5, 1.0/3.0, 0.25, 1.0/6.0 }}; return r; }

struct StyleDef
{
    std::vector<std::vector<int>> progs;   // chord roots as scale degrees (0 = tonic)
    int span;          // arpeggio range in octaves
    int regShift;      // semitone register shift for arpeggios
    float colorFloor;  // minimum chord richness
    float legatoBias;  // pedal feel
    int fullArp;       // which arp shape "Full Texture" uses
    bool planing;      // parallel open-fifth chords (La cathedrale engloutie)
};

inline const std::vector<StyleDef>& styleTable()
{
    static const std::vector<StyleDef> t {
        // Clair de Lune: gentle I-vi-IV-V style motion, rising/falling arpeggios
        { { {0,5,3,4}, {0,2,5,4}, {0,3,1,4} }, 2,  0, 0.35f,  0.15f, 0, false },
        // Une barque sur l'ocean: wave-like modal planing
        { { {0,1,2,1}, {0,2,4,2}, {0,3,0,4} }, 2,  0, 0.50f,  0.10f, 2, false },
        // Jeux d'eau: glittering high 9th-chord cascades over pedal tones
        { { {0,4,0,3}, {0,0,5,4}, {0,1,0,4} }, 3, 12, 0.60f, -0.10f, 2, false },
        // La cathedrale engloutie: parallel fifths / octaves, huge slow chords
        { { {0,1,3,0}, {0,4,3,0}, {0,2,1,0} }, 1,  0, 0.45f,  0.30f, 0, true  } };
    return t;
}

// ----------------------------------------------------------------- generator
class Generator
{
public:
    static Clip generate (const Settings& st) { Generator g (st); return g.run(); }

private:
    Settings s;
    juce::Random rng;
    std::vector<int> iv;
    int n = 7, tonic = 48, bars = 4, stepsPerBar = 8, stepsPerBeat = 2, chordCount = 4;
    int tsNum = 4, tsDen = 4;
    double barBeats = 4.0, stepBeats = 0.5, total = 16.0;
    const StyleDef* style = nullptr;
    float legato = 0.5f, baseVel = 80.0f;
    std::vector<int> prog;
    bool cadence = true;
    std::vector<Note> notes;

    explicit Generator (const Settings& st) : s (st), rng ((juce::int64) st.seed * 7919 + 13)
    {
        iv = modeTable()[(size_t) juce::jlimit (0, (int) modeTable().size() - 1, s.mode)];
        n = (int) iv.size();
        tonic = 48 + s.key + 12 * s.octave;
        const auto& m = meters()[(size_t) juce::jlimit (0, 4, s.meter)];
        barBeats = m.beats; tsNum = m.num; tsDen = m.den;
        stepBeats = rates()[(size_t) juce::jlimit (0, 4, s.rate)];
        stepsPerBar = std::max (1, (int) std::floor (barBeats / stepBeats + 1e-4));
        stepsPerBeat = std::max (1, (int) std::lround (1.0 / stepBeats));
        bars = std::max (1, s.bars);
        total = bars * barBeats;
        style = &styleTable()[(size_t) juce::jlimit (0, 3, s.style)];
        legato = juce::jlimit (0.0f, 1.0f, s.legato + style->legatoBias);
        const float c = std::max (s.color, style->colorFloor);
        chordCount = 3 + (int) (c * 3.99f);
        baseVel = 38.0f + 82.0f * s.velocity;
        prog = style->progs[(size_t) rng.nextInt ((int) style->progs.size())];
        cadence = rng.nextFloat() < 0.75f;
    }

    static int floorDiv (int a, int b) { int q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }

    int degToPitch (int deg) const
    {
        const int o = floorDiv (deg, n);
        return tonic + 12 * o + iv[(size_t) (deg - o * n)];
    }

    int chordDeg (int bar) const
    {
        if (bar == bars - 1 && bars > 1 && cadence) return 0;
        return prog[(size_t) bar % prog.size()];
    }

    void add (int pitch, double start, double len, float vel)
    {
        while (pitch > 100) pitch -= 12;
        while (pitch < 28)  pitch += 12;
        if (start >= total - 1e-6) return;
        start = std::max (0.0, start + (rng.nextDouble() - 0.5) * 0.05 * s.humanize);
        len = std::min (len, total - start);
        if (len < 0.04) len = 0.04;
        const double pos = std::fmod (start, barBeats) / barBeats;
        const float swell = 0.86f + 0.14f * (float) std::sin (juce::MathConstants<double>::pi * pos);
        const float v = baseVel * vel * swell + (rng.nextFloat() - 0.5f) * 28.0f * s.humanize;
        Note nt; nt.pitch = pitch; nt.start = start; nt.length = len;
        nt.velocity = juce::jlimit (1, 127, (int) std::lround (v));
        notes.push_back (nt);
    }

    // ---- arpeggios: mode 0 = up, 1 = down, 2 = wave (ping-pong cascade)
    void arp (int mode, float vs, int regShift)
    {
        const int span = style->span;
        for (int b = 0; b < bars; ++b)
        {
            const int d = chordDeg (b);
            const int lowP = degToPitch (d);
            std::vector<int> ladder;
            for (int o = 0; o <= span; ++o)
                for (int i = 0; i < chordCount; ++i)
                    ladder.push_back (degToPitch (d + 2 * i + n * o));
            std::sort (ladder.begin(), ladder.end());
            ladder.erase (std::unique (ladder.begin(), ladder.end()), ladder.end());
            ladder.erase (std::remove_if (ladder.begin(), ladder.end(),
                          [&] (int p) { return p > lowP + 12 * span + 2; }), ladder.end());
            const int m = (int) ladder.size();
            if (m < 2) continue;

            for (int st = 0; st < stepsPerBar; ++st)
            {
                if (st > 0 && rng.nextFloat() < (1.0f - s.density) * 0.45f) continue;
                int idx;
                if (mode == 0)      idx = st % m;
                else if (mode == 1) idx = m - 1 - (st % m);
                else { const int period = 2 * (m - 1); const int p = st % period; idx = p < m ? p : period - p; }

                const double start = b * barBeats + st * stepBeats;
                const double len = stepBeats * (0.8 + legato * 3.2);
                const float accent = (st % stepsPerBeat == 0) ? 1.0f : 0.72f + 0.1f * rng.nextFloat();
                add (ladder[(size_t) idx] + regShift + style->regShift, start, len, vs * accent);
            }
        }
    }

    // ---- block / planed chords
    void chords (float vs)
    {
        const double strum = 0.012 + 0.035 * s.humanize;
        for (int b = 0; b < bars; ++b)
        {
            const int d = chordDeg (b);
            std::vector<int> tones;
            if (style->planing)
            {
                tones = { degToPitch (d - n), degToPitch (d), degToPitch (d + 4), degToPitch (d + n) };
                if (s.color > 0.45f) tones.push_back (degToPitch (d + n + 2));
                if (s.color > 0.75f) tones.push_back (degToPitch (d + n + 4));
            }
            else
            {
                for (int i = 0; i < chordCount; ++i) tones.push_back (degToPitch (d + 2 * i));
                const int inv = rng.nextInt (3);
                for (int k = 0; k < inv && k < (int) tones.size() - 2; ++k) tones[(size_t) k] += 12;
                std::sort (tones.begin(), tones.end());
                tones.insert (tones.begin(), degToPitch (d - n));
            }

            std::vector<int> hits { 0 };
            if (s.density >= 0.7f)
                for (int st = stepsPerBeat; st < stepsPerBar; st += stepsPerBeat) hits.push_back (st);
            else if (s.density >= 0.34f && stepsPerBar > 1)
                hits.push_back (stepsPerBar / 2);

            for (size_t h = 0; h < hits.size(); ++h)
            {
                const double t0 = b * barBeats + hits[h] * stepBeats;
                const double t1 = (h + 1 < hits.size()) ? b * barBeats + hits[h + 1] * stepBeats
                                                        : (b + 1) * barBeats;
                const double len = (t1 - t0) * (0.55 + 0.45 * legato);
                for (size_t k = 0; k < tones.size(); ++k)
                    add (tones[k], t0 + (double) k * strum, len, vs * (k == 0 ? 1.0f : 0.88f));
            }
        }
    }

    // ---- motif: one rhythmic/melodic cell, sequenced over the chord changes
    void motif (float vs, int extraOct)
    {
        struct M { int step; int off; int len; };
        std::vector<M> m;
        std::vector<int> onsets { 0 };
        const float p = 0.30f + 0.55f * s.density;
        for (int st = 1; st < stepsPerBar; ++st)
        {
            const float pr = p * ((st % stepsPerBeat == 0) ? 1.0f : 0.55f);
            if (rng.nextFloat() < pr) onsets.push_back (st);
        }
        if (onsets.size() < 2 && stepsPerBar > 1) onsets.push_back (stepsPerBar / 2);

        int off = 0;
        for (size_t i = 0; i < onsets.size(); ++i)
        {
            if (i > 0)
            {
                const float r = rng.nextFloat();
                const int mv = r < 0.10f ? -3 : r < 0.28f ? -2 : r < 0.50f ? -1 : r < 0.58f ? 0
                             : r < 0.80f ? 1 : r < 0.92f ? 2 : 3;
                off = juce::jlimit (-3, 5, off + mv);
            }
            const int next = (i + 1 < onsets.size()) ? onsets[i + 1] : stepsPerBar;
            m.push_back ({ onsets[i], off, next - onsets[i] });
        }

        for (int b = 0; b < bars; ++b)
        {
            const int d = chordDeg (b);
            const int anchor = d + ((b % 2 == 0) ? 4 : 2) + n * (1 + extraOct);
            const bool lastBar = (b == bars - 1 && bars > 1);
            for (size_t i = 0; i < m.size(); ++i)
            {
                int o = m[i].off;
                if (b % 2 == 1 && rng.nextFloat() < 0.35f) o += rng.nextBool() ? 1 : -1;
                int pitch = degToPitch (anchor + o);
                double len = m[i].len * stepBeats * (0.55 + 0.45 * legato);
                const double start = b * barBeats + m[i].step * stepBeats;
                if (lastBar && i == m.size() - 1)
                {
                    pitch = degToPitch (n * (1 + extraOct));   // resolve to tonic
                    len = (b + 1) * barBeats - start;
                }
                const float accent = (m[i].step % stepsPerBeat == 0) ? 1.0f : 0.82f;
                add (pitch, start, len, vs * accent);
            }
        }
    }

    // ---- bass pedal
    void bass (float vs)
    {
        for (int b = 0; b < bars; ++b)
        {
            const int d = chordDeg (b);
            const int root = degToPitch (d - n);
            std::vector<std::pair<double, int>> ev { { 0.0, root } };
            if (s.density > 0.35f) ev.push_back ({ 0.5, degToPitch (d - n + 4) });
            if (s.density > 0.75f) ev.push_back ({ 0.75, root + 12 });
            for (size_t i = 0; i < ev.size(); ++i)
            {
                const double next = (i + 1 < ev.size()) ? ev[i + 1].first : 1.0;
                const double len = (next - ev[i].first) * barBeats * (0.6 + 0.4 * legato);
                add (ev[i].second, b * barBeats + ev[i].first * barBeats, len, vs);
            }
        }
    }

    Clip run()
    {
        switch (s.part)
        {
            case 0:  arp (0, 1.0f, 0); break;
            case 1:  arp (1, 1.0f, 0); break;
            case 2:  arp (2, 1.0f, 0); break;
            case 3:  chords (1.0f);    break;
            case 4:  motif (1.1f, 0);  break;
            case 5:  bass (1.0f);      break;
            default: bass (0.85f); arp (style->fullArp, 0.5f, 0); motif (1.1f, 1); break;
        }
        std::sort (notes.begin(), notes.end(), [] (const Note& a, const Note& b)
                   { return a.start < b.start || (a.start == b.start && a.pitch < b.pitch); });
        Clip c;
        c.notes = std::move (notes);
        c.length = total; c.barBeats = barBeats; c.tsNum = tsNum; c.tsDen = tsDen;
        return c;
    }
};
} // namespace dbs
