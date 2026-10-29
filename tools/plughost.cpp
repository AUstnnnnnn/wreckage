// Loads the installed WRECKAGE AU and VST3 like a DAW would, plays a note, reports the output level.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::AudioPluginFormatManager fm;
    fm.addDefaultFormats();
    const auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
    const juce::StringArray paths { home.getFullPathName() + "/Library/Audio/Plug-Ins/VST3/WRECKAGE.vst3",
                                    home.getFullPathName() + "/Library/Audio/Plug-Ins/Components/WRECKAGE.component" };
    for (auto* format : fm.getFormats())
        for (auto& path : paths)
        {
            if (! format->fileMightContainThisPluginType (path)) continue;
            juce::OwnedArray<juce::PluginDescription> descs;
            format->findAllTypesForFile (descs, path);
            for (auto* d : descs)
            {
                juce::String err;
                auto inst = fm.createPluginInstance (*d, 44100.0, 512, err);
                if (! inst) { std::printf ("%s: load failed: %s\n", format->getName().toRawUTF8(), err.toRawUTF8()); continue; }
                inst->setPlayConfigDetails (0, 2, 44100.0, 512);
                inst->prepareToPlay (44100.0, 512);
                for (int prog = 0; prog < inst->getNumPrograms(); ++prog)
                {
                    inst->setCurrentProgram (prog);
                    // Every factory preset through the real wrapper: chord on, overlapping same-note retrigger, all off, then silence.
                    float peak = 0, tail = 0;
                    juce::Random rnd (prog);
                    for (int b = 0; b < 1400; ++b)
                    {
                        const int n = b % 7 == 3 ? 0 : 64 + rnd.nextInt (448); // FL-ish: uneven blocks and zero-length calls
                        juce::AudioBuffer<float> buf (2, juce::jmax (1, n));
                        buf.clear();
                        juce::AudioBuffer<float> view (buf.getArrayOfWritePointers(), 2, n);
                        juce::MidiBuffer m;
                        if (b == 5) for (int nn : { 48, 55, 60, 63 }) m.addEvent (juce::MidiMessage::noteOn (1, nn, (juce::uint8) 100), 0);
                        if (b == 30) m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 90), juce::jmax (0, n - 1));
                        if (b == 60) for (int nn : { 48, 55, 60, 63 }) m.addEvent (juce::MidiMessage::noteOff (1, nn), 0);
                        if (b == 61) m.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
                        inst->processBlock (view, m);
                        if (n > 0) { peak = juce::jmax (peak, view.getMagnitude (0, n)); if (b > 1350) tail = juce::jmax (tail, view.getMagnitude (0, n)); }
                    }
                    std::printf ("%s %-20s: peak %.3f tail %.5f  %s\n", format->getName().toRawUTF8(), inst->getProgramName (prog).toRawUTF8(), peak, tail, tail > 0.001f ? "STUCK" : "ok");
                }
            }
        }
    return 0;
}
