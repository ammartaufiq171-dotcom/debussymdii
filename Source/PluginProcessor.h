#pragma once
#include <JuceHeader.h>
#include "Generator.h"

class AtelierProcessor : public juce::AudioProcessor,
                         private juce::AsyncUpdater,
                         private juce::AudioProcessorValueTreeState::Listener
{
public:
    AtelierProcessor();
    ~AtelierProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Atelier Debussy"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- app-facing API
    juce::AudioProcessorValueTreeState apvts;
    juce::ChangeBroadcaster clipChanged;
    std::atomic<double> playheadBeat { -1.0 };

    void newSeed();
    dbs::Clip getClip() const;
    juce::String suggestedFileName() const;
    bool writeMidiFileTo (const juce::File&) const;
    juce::File writeMidiFile() const;   // temp file used for drag & drop

private:
    void handleAsyncUpdate() override;
    void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
    void regenerate();
    dbs::Settings readSettings() const;
    void flushPending (juce::MidiBuffer&, int sample);
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    mutable juce::CriticalSection clipLock;
    dbs::Clip clip;

    struct Pending { int pitch; double beat; };
    std::vector<Pending> pending;

    juce::Synthesiser synth;
    double currentSampleRate = 44100.0;
    double internalPpq = 0.0, lastEnd = 0.0;
    bool wasActive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AtelierProcessor)
};
