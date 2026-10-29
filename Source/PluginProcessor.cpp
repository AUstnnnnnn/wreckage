#include "PluginProcessor.h"
#ifndef WRECKAGE_HEADLESS
 #include "PluginEditor.h"
#endif

WreckageProcessor::WreckageProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "WRECKAGE", params::layout()),
      presets (apvts)
{
    for (auto& d : params::all())
        raw.push_back (apvts.getRawParameterValue (d.id));
    if (presets.numFactory() > 0)
        presets.load ("factory:0");
}


bool WreckageProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void WreckageProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.prepare (sampleRate);
    synth.reset();
    // Some hosts announce a tiny (or zero) maximum and then send more; size generously and chunk anything beyond.
    preparedBlock = juce::jmax (2048, samplesPerBlock);
    work.setSize (2, preparedBlock);
    uiBuf.ensureSize (4096);
    fx.prepare (sampleRate, preparedBlock);
    uiMidi.reset (sampleRate);
    setLatencySamples (fx.latency());
}

wreck::Settings WreckageProcessor::readSettings() const
{
    // raw[] follows params::all() order.
    int i = 0;
    auto f = [&] { return raw[(size_t) i++]->load(); };
    auto n = [&] { return (int) std::round (raw[(size_t) i++]->load()); };
    wreck::Settings s;
    for (int o = 0; o < 2; ++o)
    {
        s.wave[o] = n(); s.oct[o] = n(); s.semi[o] = n(); s.fine[o] = f(); s.pw[o] = f(); s.level[o] = f();
    }
    s.uniVoices = n(); s.uniDetune = f(); s.uniSpread = f();
    s.subLevel = f(); s.subWave = n(); s.subOct = n(); s.noise = f();
    s.fMode = n(); s.cutoff = f(); s.res = f(); s.fDrive = f(); s.fEnv = f(); s.fKey = f();
    s.aA = f(); s.aD = f(); s.aS = f(); s.aR = f();
    s.fA = f(); s.fD = f(); s.fS = f(); s.fR = f();
    s.lfoWave = n(); s.lfoRate = f(); s.lfoSync = n() != 0; s.lfoDiv = n();
    s.lfoPitch = f(); s.lfoCutoff = f(); s.lfoAmp = f(); s.lfoPw = f();
    s.wMode = n(); s.wDrive = f(); s.wTone = f(); s.wMix = f();
    s.bits = n(); s.rate = n(); s.wow = f();
    s.spSize = f(); s.spDamp = f(); s.spMix = f();
    s.rust = f(); s.glide = f(); s.mono = n() != 0; s.bend = n(); s.velocity = f(); s.outDb = f();
    jassert ((size_t) i == raw.size());
    return s;
}

void WreckageProcessor::handleMidi (const wreck::Settings& s, const juce::MidiMessage& m)
{
    if (m.isNoteOn()) synth.noteOn (s, m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff()) synth.noteOff (s, m.getNoteNumber());
    else if (m.isPitchWheel()) synth.pitchWheel (m.getPitchWheelValue());
    else if (m.isControllerOfType (1)) synth.modWheel (m.getControllerValue());
    else if (m.isAllNotesOff() || m.isAllSoundOff()) synth.allOff();
}

void WreckageProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n == 0)
    {
        // Zero-length calls still carry events (FL flushes note-offs this way): apply them, render nothing.
        const auto s0 = readSettings();
        for (const auto meta : midi)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOff() && ! synth.isHeld (msg.getNoteNumber())) { orphanOff[(size_t) msg.getNoteNumber()] = sampleClock; continue; }
            handleMidi (s0, msg);
            if (msg.isNoteOn() && orphanOff[(size_t) msg.getNoteNumber()] == sampleClock)
            {
                orphanOff[(size_t) msg.getNoteNumber()] = -1;
                handleMidi (s0, juce::MidiMessage::noteOff (msg.getChannel(), msg.getNoteNumber()));
            }
        }
        return;
    }
    if (preparedBlock == 0) prepareToPlay (getSampleRate() > 0 ? getSampleRate() : 44100.0, n);

    // Some hosts send more samples than prepareToPlay promised; the FX buffers are sized to that promise.
    if (n > preparedBlock && preparedBlock > 0)
    {
        for (int start = 0; start < n; start += preparedBlock)
        {
            const int len = juce::jmin (preparedBlock, n - start);
            juce::AudioBuffer<float> part (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), start, len);
            juce::MidiBuffer partMidi;
            partMidi.addEvents (midi, start, len, -start);
            processBlock (part, partMidi);
        }
        return;
    }
    buffer.clear();

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm()) synth.setBpm (*bpm);

    uiBuf.clear();
    uiMidi.removeNextBlockOfMessages (uiBuf, n);
    if (! uiBuf.isEmpty()) midi.addEvents (uiBuf, 0, n, 0);

    const auto s = readSettings();
    // Always render stereo into our own buffer, then hand the host whatever channel count it asked for.
    juce::AudioBuffer<float> out (work.getArrayOfWritePointers(), 2, n);
    out.clear();
    auto* L = out.getWritePointer (0);
    auto* R = out.getWritePointer (1);

    // Sample-accurate MIDI: render up to each event, then apply it.
    // A very short press can put note-on and note-off on the same sample, and some hosts (FL) list the off first.
    // A note-off for a key that isn't down is remembered; if that key's note-on lands on the same sample it is
    // played and released at once instead of hanging forever.
    int pos = 0;
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, n, meta.samplePosition);
        if (at > pos) { synth.render (s, L + pos, R + pos, at - pos); pos = at; }
        const auto msg = meta.getMessage();
        const auto when = sampleClock + at;
        if (msg.isNoteOff() && ! synth.isHeld (msg.getNoteNumber())) { orphanOff[(size_t) msg.getNoteNumber()] = when; continue; }
        handleMidi (s, msg);
        if (msg.isNoteOn() && orphanOff[(size_t) msg.getNoteNumber()] == when)
        {
            orphanOff[(size_t) msg.getNoteNumber()] = -1;
            handleMidi (s, juce::MidiMessage::noteOff (msg.getChannel(), msg.getNoteNumber()));
        }
    }
    if (pos < n) synth.render (s, L + pos, R + pos, n - pos);

    const float rustEff = juce::jmin (1.0f, s.rust + synth.modWheelValue());
    fx.process (s, rustEff, out);
    const int outCh = buffer.getNumChannels();
    if (outCh == 1) { buffer.copyFrom (0, 0, out.getReadPointer (0), n, 0.5f); buffer.addFrom (0, 0, out.getReadPointer (1), n, 0.5f); }
    for (int c = 0; c < juce::jmin (2, outCh); ++c) if (outCh > 1) buffer.copyFrom (c, 0, out, c, 0, n);

    // Meters and scope for the UI.
    float pl = peakL.load() * 0.85f, pr = peakR.load() * 0.85f;
    int wp = scopePos.load();
    for (int i = 0; i < n; ++i)
    {
        pl = juce::jmax (pl, std::abs (L[i]));
        pr = juce::jmax (pr, std::abs (R[i]));
        scope[(size_t) wp] = 0.5f * (L[i] + R[i]);
        wp = (wp + 1) % scopeSize;
    }
    peakL = pl; peakR = pr; scopePos = wp; rustNow = rustEff;
    sampleClock += n;
}

void WreckageProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("PRESET"), nullptr);
    state.appendChild (presets.toState(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void WreckageProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto t = juce::ValueTree::fromXml (*xml);
        if (! t.hasType (apvts.state.getType())) return;
        presets.fromState (t.getChildWithName ("PRESET"));
        t.removeChild (t.getChildWithName ("PRESET"), nullptr);
        apvts.replaceState (t);
    }
}

juce::AudioProcessorEditor* WreckageProcessor::createEditor()
{
#ifdef WRECKAGE_HEADLESS
    return nullptr;
#else
    return new WreckageEditor (*this);
#endif
}

#ifndef WRECKAGE_HEADLESS
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new WreckageProcessor(); }
#endif
