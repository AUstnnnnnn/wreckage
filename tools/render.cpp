// Renders every factory preset to build/renders/*.wav and fails on silence, NaN/inf or runaway level.
#include "PluginProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 48000;
    const int block = 512;
    auto outDir = juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "build/renders");
    outDir.createDirectory();

    WreckageProcessor proc;
    int failures = 0;
    auto all = proc.presets.list();
    proc.presets.unlock(); // include the hidden bank in the check
    all = proc.presets.list();

    for (auto& p : *all.getArray())
    {
        if (! (bool) p["factory"]) continue;
        proc.presets.load (p["id"].toString());
        proc.prepareToPlay (sr, block);

        const bool low = p["tags"].getArray()->contains ("bass") || p["tags"].getArray()->contains ("sub");
        const int notes[] { low ? 36 : 48, low ? 36 : 55, low ? 43 : 63 };
        juce::AudioBuffer<float> out (2, (int) (sr * 4));
        out.clear();
        juce::AudioBuffer<float> buf (2, block);
        for (int pos = 0; pos < out.getNumSamples(); pos += block)
        {
            juce::MidiBuffer midi;
            if (pos == 0) for (int nn : notes) midi.addEvent (juce::MidiMessage::noteOn (1, nn, (juce::uint8) 100), 0);
            if (pos == block * 4 * 47) for (int nn : notes) midi.addEvent (juce::MidiMessage::noteOff (1, nn), 0); // ~2s
            proc.processBlock (buf, midi);
            const int len = juce::jmin (block, out.getNumSamples() - pos);
            for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, len);
        }

        float peak = 0; double sum = 0; bool finite = true;
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < out.getNumSamples(); ++i)
            {
                const float x = out.getSample (c, i);
                finite &= std::isfinite (x);
                peak = juce::jmax (peak, std::abs (x));
                sum += x * x;
            }
        const double rms = std::sqrt (sum / (2.0 * out.getNumSamples()));
        const bool ok = finite && rms > 0.005 && peak <= 1.0f;
        failures += ok ? 0 : 1;
        std::printf ("%-4s %-22s peak %.3f  rms %.4f%s\n", ok ? "ok" : "FAIL", p["name"].toString().toRawUTF8(), peak, rms, finite ? "" : "  NaN!");

        auto f = outDir.getChildFile (juce::File::createLegalFileName (p["name"].toString()) + ".wav");
        f.deleteFile();
        juce::WavAudioFormat wav;
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0)))
            w->writeFromAudioSampleBuffer (out, 0, out.getNumSamples());
    }
    // Stuck notes: release mid-decay, hammer one key, overflow the voices, then everything must go quiet.
    for (auto& p : *all.getArray())
    {
        proc.presets.load (p["id"].toString());
        proc.prepareToPlay (sr, block);
        juce::AudioBuffer<float> buf (2, block);
        auto run = [&] (juce::MidiBuffer m) { proc.processBlock (buf, m); };
        auto ev = [] (juce::MidiMessage msg) { juce::MidiBuffer m; m.addEvent (msg, 0); return m; };
        run (ev (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100)));
        for (int b = 0; b < 3; ++b) run ({});
        run (ev (juce::MidiMessage::noteOff (1, 60)));
        for (int r = 0; r < 40; ++r) { run (ev (juce::MidiMessage::noteOn (1, 64, (juce::uint8) 90))); run (ev (juce::MidiMessage::noteOff (1, 64))); }
        juce::MidiBuffer chord;
        for (int nn = 0; nn < 24; ++nn) chord.addEvent (juce::MidiMessage::noteOn (1, 36 + nn, (juce::uint8) 100), nn * 7);
        run (chord);
        for (int b = 0; b < 20; ++b) run ({});
        juce::MidiBuffer offs;
        for (int nn = 0; nn < 24; ++nn) offs.addEvent (juce::MidiMessage::noteOff (1, 36 + nn), nn * 3);
        run (offs);
        const float rel = (float) p["params"].getProperty ("aenv_r", 0.25);
        const int tail = (int) ((juce::jmax (12.0f, rel * 2.0f) + 1.0f) * sr / block);
        for (int b = 0; b < tail; ++b) run ({});
        const int left = proc.activeVoices();
        if (left) { ++failures; std::printf ("FAIL %-22s %d voice(s) still sounding after release\n", p["name"].toString().toRawUTF8(), left); }
    }

    // Tiny press: note-on and note-off land on the same sample. Hosts may list the off first; it must still end.
    for (auto& p : *all.getArray())
        for (int order = 0; order < 3; ++order)
        {
            proc.presets.load (p["id"].toString());
            proc.prepareToPlay (sr, block);
            juce::AudioBuffer<float> buf (2, block);
            juce::MidiBuffer m;
            if (order == 0) { m.addEvent (juce::MidiMessage::noteOff (1, 62), 100); m.addEvent (juce::MidiMessage::noteOn (1, 62, (juce::uint8) 100), 100); }
            else if (order == 2)
            {
                // off in a zero-length call, on at offset 0 of the next one: the same instant split across calls
                juce::AudioBuffer<float> none (2, 0);
                juce::MidiBuffer off; off.addEvent (juce::MidiMessage::noteOff (1, 62), 0);
                proc.processBlock (none, off);
                m.addEvent (juce::MidiMessage::noteOn (1, 62, (juce::uint8) 100), 0);
            }
            else            { m.addEvent (juce::MidiMessage::noteOn (1, 62, (juce::uint8) 100), 100); m.addEvent (juce::MidiMessage::noteOff (1, 62), 100); }
            proc.processBlock (buf, m);
            for (int b = 0; b < (int) (14 * sr / block); ++b) { juce::MidiBuffer e; proc.processBlock (buf, e); }
            if (proc.activeVoices()) { ++failures; std::printf ("FAIL tiny press %s (%s first): stuck\n", p["name"].toString().toRawUTF8(), order == 0 ? "off" : order == 1 ? "on" : "split"); }
        }

    // Fuzz: random piano-roll MIDI (same-note overlaps, on/off at one sample, chords, mono/poly flips), every on matched by an off.
    {
        juce::Random rnd (29);
        for (auto& preset : *all.getArray())
            for (int trial = 0; trial < 60; ++trial)
            {
                const auto nameStr = preset["name"].toString();
                const char* name = nameStr.toRawUTF8();
                proc.presets.load (preset["id"].toString());
                proc.prepareToPlay (sr, block);
                juce::AudioBuffer<float> buf (2, block);
                struct Ev { int at, note; bool on; };
                std::vector<Ev> evs;
                for (int k = 0; k < 60; ++k)
                {
                    const int note = 48 + rnd.nextInt (12), at = rnd.nextInt (block * 80), len = rnd.nextInt (3) == 0 ? 0 : rnd.nextInt (block * 12);
                    evs.push_back ({ at, note, true });
                    evs.push_back ({ at + len, note, false });
                }
                std::stable_sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) { return a.at < b.at; });
                size_t e = 0;
                for (int b = 0; b < 100 + (int) (14 * sr / block); ++b)
                {
                    juce::MidiBuffer m;
                    if (b < 90 && rnd.nextInt (12) == 0) proc.apvts.getParameter ("mono")->setValueNotifyingHost (proc.apvts.getParameter ("mono")->getValue() > 0.5f ? 0.0f : 1.0f);
                    while (e < evs.size() && evs[e].at < (b + 1) * block)
                    {
                        auto& ev = evs[e++];
                        m.addEvent (ev.on ? juce::MidiMessage::noteOn (1, ev.note, (juce::uint8) 100) : juce::MidiMessage::noteOff (1, ev.note), ev.at - b * block);
                    }
                    proc.processBlock (buf, m);
                }
                if (const int left = proc.activeVoices())
                {
                    ++failures;
                    std::printf ("FAIL fuzz %s trial %d: %d voice(s) stuck  %s mono=%d\n", name, trial, left, proc.debugState().toRawUTF8(), (int) (proc.apvts.getParameter ("mono")->getValue() > 0.5f));
                    break;
                }
            }
    }

    // CPU: 16 held notes per preset for 10 s of audio, reported as % of real time.
    if (argc > 2 && juce::String (argv[2]) == "--bench")
    {
        double worst = 0;
        for (auto& p : *all.getArray())
        {
            proc.presets.load (p["id"].toString());
            proc.prepareToPlay (sr, block);
            juce::AudioBuffer<float> buf (2, block);
            const int blocks = (int) (sr * 10 / block);
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            for (int b = 0; b < blocks; ++b)
            {
                juce::MidiBuffer midi;
                if (b == 0) for (int nn = 0; nn < 16; ++nn) midi.addEvent (juce::MidiMessage::noteOn (1, 40 + nn * 2, (juce::uint8) 100), 0);
                proc.processBlock (buf, midi);
            }
            const double pct = (juce::Time::getMillisecondCounterHiRes() - t0) / 10000.0 * 100.0;
            worst = juce::jmax (worst, pct);
            std::printf ("cpu %-22s %5.1f%%\n", p["name"].toString().toRawUTF8(), pct);
        }
        std::printf ("worst %.1f%% of one core\n", worst);
    }
    std::printf ("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
