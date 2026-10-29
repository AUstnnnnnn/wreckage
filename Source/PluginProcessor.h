#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "Engine.h"
#include "Params.h"
#include "Presets.h"

class WreckageProcessor : public juce::AudioProcessor
{
public:
    WreckageProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "WRECKAGE"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    // Factory presets double as host programs so FL's own preset arrows step through them.
    int getNumPrograms() override { return juce::jmax (1, presets.numFactory()); }
    int getCurrentProgram() override
    {
        const auto id = presets.current()["id"].toString();
        return id.startsWith ("factory:") ? id.fromFirstOccurrenceOf (":", false, false).getIntValue() : 0;
    }
    void setCurrentProgram (int i) override { if (juce::isPositiveAndBelow (i, presets.numFactory())) presets.load ("factory:" + juce::String (i)); }
    const juce::String getProgramName (int i) override { return presets.factoryAt (i)["name"].toString(); }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    Presets presets;

    // UI keyboard and the editor's meters/scope. Written on the audio thread, read on the message thread.
    juce::MidiMessageCollector uiMidi;
    std::atomic<float> peakL { 0 }, peakR { 0 }, rustNow { 0 };
    static constexpr int scopeSize = 2048;
    std::array<float, scopeSize> scope {};
    std::atomic<int> scopePos { 0 };
    int activeVoices() const { return synth.activeVoices(); }
    juce::String debugState() const { return synth.debugState(); }

private:
    wreck::Settings readSettings() const;
    void handleMidi (const wreck::Settings&, const juce::MidiMessage&);

    wreck::Synth synth;
    wreck::Fx fx;
    std::vector<std::atomic<float>*> raw;
    int preparedBlock = 0;

    juce::AudioBuffer<float> work;
    juce::MidiBuffer uiBuf;
    std::array<juce::int64, 128> orphanOff { [] { std::array<juce::int64, 128> a; a.fill (-1); return a; }() };
    juce::int64 sampleClock = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WreckageProcessor)
};
