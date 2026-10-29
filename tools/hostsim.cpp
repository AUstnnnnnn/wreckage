#include "PluginProcessor.h"
int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    for (double sr : { 44100.0, 48000.0 })
        for (int prep : { 512, 256 })
        {
            WreckageProcessor proc;
            proc.setPlayConfigDetails (0, 2, sr, prep);
            proc.prepareToPlay (sr, prep);
            juce::Random rnd (1);
            float peak = 0; bool finite = true; int maxBlock = 0;
            for (int b = 0; b < 400; ++b)
            {
                const int n = 1 + rnd.nextInt (prep * 3);
                maxBlock = juce::jmax (maxBlock, n);
                juce::AudioBuffer<float> buf (2, n);
                juce::MidiBuffer m;
                if (b == 2) m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
                proc.processBlock (buf, m);
                for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) { auto x = buf.getSample (c, i); finite &= std::isfinite (x); peak = juce::jmax (peak, std::abs (x)); }
            }
            std::printf ("sr %.0f prep %d (blocks up to %d): peak %.3f finite %d\n", sr, prep, maxBlock, peak, (int) finite);
        }
}
