#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

// Everything that makes sound. No JUCE processor types in here so the offline renderer can use it too.
namespace wreck
{
constexpr float twoPi = juce::MathConstants<float>::twoPi;
constexpr int maxVoices = 16;
constexpr int maxUnison = 7;
constexpr int controlRate = 16; // samples between modulation updates

// Snapshot of every parameter, read once per block on the audio thread.
struct Settings
{
    int   wave[2] {}, oct[2] {}, semi[2] {};
    float fine[2] {}, pw[2] {}, level[2] {};
    int   uniVoices = 1;
    float uniDetune = 0, uniSpread = 0;
    float subLevel = 0; int subWave = 0, subOct = -1;
    float noise = 0;
    int   fMode = 1;
    float cutoff = 4000, res = 0, fDrive = 0, fEnv = 0, fKey = 0;
    float aA = .005f, aD = .3f, aS = .8f, aR = .25f;
    float fA = .005f, fD = .4f, fS = .3f, fR = .3f;
    int   lfoWave = 0; float lfoRate = 2; bool lfoSync = false; int lfoDiv = 2;
    float lfoPitch = 0, lfoCutoff = 0, lfoAmp = 0, lfoPw = 0;
    int   wMode = 1;
    float wDrive = .25f, wTone = .6f, wMix = 1;
    int   bits = 16, rate = 1;
    float wow = 0, spSize = .6f, spDamp = .6f, spMix = .15f;
    float rust = 0, glide = 0; bool mono = false; int bend = 2;
    float velocity = .5f, outDb = -3;
};

inline float polyBlep (float t, float dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

inline float osc (int wave, float ph, float dt, float pw)
{
    switch (wave)
    {
        case 0: return 2.0f * ph - 1.0f - polyBlep (ph, dt);
        case 1:
        {
            float v = ph < pw ? 1.0f : -1.0f;
            v += polyBlep (ph, dt);
            float t2 = ph - pw; if (t2 < 0) t2 += 1.0f;
            v -= polyBlep (t2, dt);
            return v * 0.8f;
        }
        case 2: return 1.0f - 4.0f * std::abs (ph - 0.5f);
        case 3: return std::sin (twoPi * ph);
        default:
        {
            // CHIP: deliberately aliased stepped saw, width picks the step count (2..16 levels).
            const int steps = 2 + (int) (pw * 14.0f);
            return (std::floor (ph * (float) steps) / (float) (steps - 1)) * 2.0f - 1.0f;
        }
    }
}

// Cytomic/Zavalishin TPT state variable filter, one channel.
struct Svf
{
    float ic1 = 0, ic2 = 0;
    void reset() { ic1 = ic2 = 0; }
    inline float tick (float v0, float a1, float a2, float a3, float k, int mode)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        if (mode == 2) return v1 * k;            // BP, level-compensated
        if (mode == 3) return v0 - k * v1 - v2;  // HP
        return v2;                               // LP
    }
};

// Linear attack, exponential decay and release. Parameters can change at any moment without
// disturbing the stage a note is in. (juce::ADSR recomputes its release rate from the sustain
// level whenever parameters are set, so on zero-sustain patches a note released mid-decay never ended.)
struct Env
{
    enum Stage { idle, attack, decay, sustain, release };
    Stage st = idle;
    float v = 0, sr = 48000, aInc = 1, dCoef = 0, s = 1, rCoef = 0;

    void set (float a, float d, float sus, float r)
    {
        aInc = 1.0f / juce::jmax (1.0f, a * sr);
        dCoef = std::exp (-6.9f / juce::jmax (1.0f, d * sr)); // -60 dB over the set time
        s = sus;
        rCoef = std::exp (-6.9f / juce::jmax (1.0f, r * sr));
    }
    void on() { st = attack; } // rises from wherever it is, so retriggers don't click
    void off() { if (st != idle) st = release; }
    void kill() { st = idle; v = 0; }
    bool active() const { return st != idle; }
    bool releasing() const { return st == release; }
    inline float next()
    {
        switch (st)
        {
            case attack:  v += aInc; if (v >= 1.0f) { v = 1.0f; st = decay; } break;
            case decay:   v = s + (v - s) * dCoef; if (v - s < 1.0e-4f) { v = s; st = s < 1.0e-4f ? idle : sustain; } break;
            case sustain: v = s; break;
            case release: v *= rCoef; if (v < 1.0e-4f) { v = 0; st = idle; } break;
            case idle:    break;
        }
        return v;
    }
};

inline float fastTanh (float x)
{
    x = juce::jlimit (-3.0f, 3.0f, x);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

struct Voice
{
    int note = -1, key = 60;
    float vel = 0;
    uint64_t age = 0;
    float freq = 440, target = 440;
    std::array<float, maxUnison> phA {}, phB {};
    float phSub = 0;
    Env aenv, fenv;
    Svf f[2][2]; // [stage][channel]
    float a1 = 0, a2 = 0, a3 = 0, k = 2;

    // Stealing: the old sound fades over ~3 ms, then the queued note starts in this voice.
    int pendingNote = -1;
    float pendingVel = 0, fade = 1;
    bool pendingOff = false;

    bool active() const { return aenv.active() || pendingNote >= 0; }
};

class Synth
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        for (auto& v : voices) { v.aenv.sr = sr; v.fenv.sr = sr; }
        fadeStep = 1.0f / (0.003f * sr);
        rng.setSeedRandomly();
    }

    void reset()
    {
        for (auto& v : voices)
        {
            v.aenv.kill(); v.fenv.kill(); v.note = -1; v.pendingNote = -1;
            for (auto& st : v.f) for (auto& c : st) c.reset();
        }
        held.clear();
    }

    void setBpm (double b) { bpm = b > 0 ? (float) b : 120.0f; }
    void pitchWheel (int value14) { bendNorm = ((float) value14 - 8192.0f) / 8192.0f; }
    void modWheel (int cc) { modwheel = (float) cc / 127.0f; }
    float modWheelValue() const { return modwheel; }

    void noteOn (const Settings& s, int note, float velocity)
    {
        held.removeFirstMatchingValue (note);
        held.add (note);
        if (s.mono) { monoOn (s, note, velocity); return; }

        Voice* v = nullptr;
        for (auto& c : voices) if (c.key == note && c.active() && c.pendingNote < 0) { v = &c; break; } // same key: retrigger in place
        if (! v) for (auto& c : voices) if (! c.active()) { v = &c; break; }
        if (! v) v = oldest (true);   // a releasing voice first
        if (! v) v = oldest (false);  // then the oldest held one
        if (v->active() && v->key != note) steal (*v, note, velocity);
        else start (s, *v, note, velocity, true);
    }

    void noteOff (const Settings& s, int note)
    {
        held.removeFirstMatchingValue (note);
        auto& m = voices[0];
        const bool legatoBack = s.mono && m.note == note && m.pendingNote < 0 && ! held.isEmpty();
        if (legatoBack)
        {
            // Legato back to the last key still down.
            m.target = mtof (held.getLast());
            m.note = m.key = held.getLast();
            if (s.glide <= 0.0005f) m.freq = m.target;
        }
        // Every voice, whatever the mode: a preset or MONO switch mid-note must not orphan voices.
        for (auto& v : voices)
        {
            if (legatoBack && &v == &m) continue;
            if (v.note == note) { v.aenv.off(); v.fenv.off(); v.note = -1; }
            if (v.pendingNote == note) v.pendingOff = true;
        }
    }

    void allOff()
    {
        held.clear();
        for (auto& v : voices) { v.aenv.off(); v.fenv.off(); v.note = -1; if (v.pendingNote >= 0) v.pendingOff = true; }
    }

    // Adds voices into L/R.
    void render (const Settings& s, float* L, float* R, int n)
    {
        const float lfoHz = s.lfoSync ? (bpm / 60.0f) / divBeats (s.lfoDiv) : s.lfoRate;
        const float glideCoef = s.glide > 0.0005f ? 1.0f - std::exp (-1.0f / (s.glide * 0.35f * sr)) : 1.0f;
        const int uni = juce::jlimit (1, maxUnison, s.uniVoices);
        const float uniGain = 1.0f / std::sqrt ((float) uni);
        const float k = 2.0f - 1.96f * std::pow (s.res, 0.6f);
        const float driveIn = 1.0f + s.fDrive * 9.0f;
        const float driveOut = 1.0f / std::sqrt (driveIn);
        const bool oscOn[2] { s.level[0] > 0.0001f, s.level[1] > 0.0001f };

        // Per-block constants shared by every voice.
        float uniPanL[maxUnison], uniPanR[maxUnison], mul[2][maxUnison];
        for (int u = 0; u < uni; ++u)
        {
            const float off = uni == 1 ? 0.0f : ((float) u / (float) (uni - 1)) * 2.0f - 1.0f;
            const float pan = 0.5f + 0.5f * off * s.uniSpread;
            uniPanL[u] = std::cos (pan * juce::MathConstants<float>::halfPi) * uniGain;
            uniPanR[u] = std::sin (pan * juce::MathConstants<float>::halfPi) * uniGain;
            const float detune = std::exp2 (off * s.uniDetune * 0.5f / 12.0f);
            for (int o = 0; o < 2; ++o)
                mul[o][u] = detune * std::exp2 ((float) s.oct[o] + ((float) s.semi[o] + s.fine[o] * 0.01f) / 12.0f) / sr;
        }
        const float subMul = std::exp2 ((float) s.subOct) / sr;

        for (auto& v : voices)
        {
            v.aenv.set (s.aA, s.aD, s.aS, s.aR);
            v.fenv.set (s.fA, s.fD, s.fS, s.fR);
        }

        for (int i0 = 0; i0 < n; i0 += controlRate)
        {
            const int len = juce::jmin (controlRate, n - i0);
            lfoValue = advanceLfo (s.lfoWave, lfoHz * (float) len / sr);
            const float bendMul = std::exp2 ((bendNorm * (float) s.bend + lfoValue * s.lfoPitch * 2.0f) / 12.0f);
            const float pwMod = lfoValue * s.lfoPw * 0.4f;
            const float pw[2] { juce::jlimit (0.05f, 0.95f, s.pw[0] + pwMod), juce::jlimit (0.05f, 0.95f, s.pw[1] + pwMod) };
            const float ampLfo = 1.0f - s.lfoAmp * 0.5f * (1.0f - lfoValue);

            for (auto& v : voices)
            {
                if (! v.active()) continue;

                const float fenvNow = v.fenv.next();
                for (int skip = 1; skip < len; ++skip) v.fenv.next();
                const float oct = s.fEnv * fenvNow * 7.0f + lfoValue * s.lfoCutoff * 3.0f + s.fKey * ((float) v.key - 60.0f) / 12.0f;
                const float fc = juce::jlimit (20.0f, sr * 0.45f, s.cutoff * std::exp2 (oct));
                const float g = std::tan (juce::MathConstants<float>::pi * fc / sr);
                v.k = k;
                v.a1 = 1.0f / (1.0f + g * (g + k));
                v.a2 = g * v.a1;
                v.a3 = g * v.a2;
                const float velGain = (1.0f - s.velocity) + s.velocity * v.vel;

                for (int i = i0; i < i0 + len; ++i)
                {
                    v.freq += (v.target - v.freq) * glideCoef;
                    const float f0 = v.freq * bendMul;
                    float l = 0, r = 0;

                    for (int o = 0; o < 2; ++o)
                    {
                        if (! oscOn[o]) continue;
                        auto& ph = o == 0 ? v.phA : v.phB;
                        const float lev = s.level[o];
                        for (int u = 0; u < uni; ++u)
                        {
                            const float dt = juce::jmin (0.49f, f0 * mul[o][u]);
                            auto& p = ph[(size_t) u];
                            const float x = osc (s.wave[o], p, dt, pw[o]) * lev;
                            p += dt; if (p >= 1.0f) p -= 1.0f;
                            l += x * uniPanL[u];
                            r += x * uniPanR[u];
                        }
                    }
                    if (s.subLevel > 0.0001f)
                    {
                        const float x = (s.subWave == 0 ? std::sin (twoPi * v.phSub) : (v.phSub < 0.5f ? 0.7f : -0.7f)) * s.subLevel * 0.7071f;
                        v.phSub += f0 * subMul; if (v.phSub >= 1.0f) v.phSub -= 1.0f;
                        l += x; r += x;
                    }
                    if (s.noise > 0.0001f)
                    {
                        const float x = (rng.nextFloat() * 2.0f - 1.0f) * s.noise * 0.3535f;
                        l += x; r += x;
                    }

                    l = fastTanh (l * driveIn) * driveOut;
                    r = fastTanh (r * driveIn) * driveOut;
                    const int fm = s.fMode == 1 ? 0 : s.fMode;
                    l = v.f[0][0].tick (l, v.a1, v.a2, v.a3, v.k, fm);
                    r = v.f[0][1].tick (r, v.a1, v.a2, v.a3, v.k, fm);
                    if (s.fMode == 1)
                    {
                        l = v.f[1][0].tick (l, v.a1, v.a2, v.a3, v.k, 0);
                        r = v.f[1][1].tick (r, v.a1, v.a2, v.a3, v.k, 0);
                    }

                    float amp = v.aenv.next() * velGain * ampLfo * 0.3f;
                    if (v.pendingNote >= 0)
                    {
                        v.fade -= fadeStep;
                        if (v.fade <= 0.0f) { startPending (s, v); amp = 0.0f; }
                        else amp *= v.fade;
                    }
                    L[i] += l * amp;
                    R[i] += r * amp;
                }
                if (! v.active()) v.note = -1;
            }
        }
    }

    float lfoNow() const { return lfoValue; }
    juce::String debugState() const
    {
        juce::String r = "held[" + juce::String (held.size()) + "] ";
        for (size_t i = 0; i < voices.size(); ++i)
            if (voices[i].active())
                r << "v" << (int) i << "(note " << voices[i].note << " key " << voices[i].key << " pend " << voices[i].pendingNote
                  << " st " << (int) voices[i].aenv.st << " lvl " << voices[i].aenv.v << ") ";
        return r;
    }
    bool isHeld (int note) const { return held.contains (note); }
    int activeVoices() const { int n = 0; for (auto& v : voices) n += v.active() ? 1 : 0; return n; }

private:
    static float divBeats (int d)
    {
        static const float b[] { 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 2.0f / 3.0f, 1.0f / 3.0f, 1.0f / 6.0f };
        return b[juce::jlimit (0, 7, d)];
    }

    float advanceLfo (int wave, float inc)
    {
        lfoPhase += inc;
        if (lfoPhase >= 1.0f) { lfoPhase -= std::floor (lfoPhase); sh = rng.nextFloat() * 2.0f - 1.0f; }
        switch (wave)
        {
            case 0: return std::sin (twoPi * lfoPhase);
            case 1: return 1.0f - 4.0f * std::abs (lfoPhase - 0.5f);
            case 2: return 1.0f - 2.0f * lfoPhase;
            case 3: return lfoPhase < 0.5f ? 1.0f : -1.0f;
            default: return sh;
        }
    }

    static float mtof (int note) { return 440.0f * std::exp2 (((float) note - 69.0f) / 12.0f); }

    Voice* oldest (bool releasingOnly)
    {
        Voice* best = nullptr;
        for (auto& c : voices)
            if (c.pendingNote < 0 && (! releasingOnly || c.aenv.releasing()) && (! best || c.age < best->age)) best = &c;
        return best ? best : &voices[0];
    }

    void steal (Voice& v, int note, float velocity)
    {
        if (v.pendingNote < 0) v.fade = 1.0f;
        v.pendingNote = note;
        v.pendingVel = velocity;
        v.pendingOff = false;
        v.note = -1;
        v.age = ++clock;
    }

    void startPending (const Settings& s, Voice& v)
    {
        const int n = v.pendingNote;
        const bool off = v.pendingOff;
        v.pendingNote = -1;
        v.fade = 1.0f;
        v.aenv.kill(); v.fenv.kill();
        for (auto& st : v.f) for (auto& c : st) c.reset();
        start (s, v, n, v.pendingVel, true);
        if (off) { v.aenv.off(); v.fenv.off(); v.note = -1; }
    }

    // Glide is legato only: it bends from the previous note while another key is still held.
    void start (const Settings& s, Voice& v, int note, float velocity, bool retrigger)
    {
        v.target = mtof (note);
        const bool slide = s.glide > 0.0005f && held.size() > 1 && lastFreq > 0;
        v.freq = slide ? lastFreq : v.target;
        lastFreq = v.target;
        v.note = v.key = note;
        v.vel = velocity;
        v.age = ++clock;
        if (retrigger)
        {
            if (! v.aenv.active())
            {
                // Random start phases keep unison wide.
                for (auto& p : v.phA) p = rng.nextFloat();
                for (auto& p : v.phB) p = rng.nextFloat();
            }
            v.aenv.on();
            v.fenv.on();
        }
    }

    void monoOn (const Settings& s, int note, float velocity)
    {
        auto& v = voices[0];
        const bool legato = v.aenv.active() && ! v.aenv.releasing() && held.size() > 1;
        start (s, v, note, velocity, ! legato);
    }

    std::array<Voice, maxVoices> voices;
    juce::Array<int> held;
    juce::Random rng;
    float sr = 48000, bpm = 120, lfoPhase = 0, lfoValue = 0, sh = 0, bendNorm = 0, modwheel = 0, lastFreq = 0, fadeStep = 0.007f;
    uint64_t clock = 0;
};

// The WRECK chain: shaper (2x oversampled) -> tone -> bitcrush/downsample -> tape wow -> dust -> space -> out.
class Fx
{
public:
    void prepare (double sampleRate, int maxBlock)
    {
        sr = (float) sampleRate;
        os.initProcessing ((size_t) maxBlock);
        os.reset();
        delay.assign ((size_t) (sr * 0.05f) + 4, 0.0f);
        delayR.assign (delay.size(), 0.0f);
        reverb.setSampleRate (sampleRate);
        reverb.reset();
        for (auto* sm : { &drive, &tone, &mix, &wow, &spMix, &out })
            sm->reset (sampleRate, 0.05);
        dry.setSize (2, maxBlock);
        rng.setSeedRandomly();
    }

    int latency() const { return (int) os.getLatencyInSamples(); }

    // rustEff is the RUST knob plus the mod wheel. Returns nothing; writes in place.
    void process (const Settings& s, float rustEff, juce::AudioBuffer<float>& buf)
    {
        const int n = buf.getNumSamples();
        auto* L = buf.getWritePointer (0);
        auto* R = buf.getWritePointer (1);

        drive.setTargetValue (juce::jmin (1.0f, s.wDrive + rustEff * 0.5f));
        tone.setTargetValue (juce::jlimit (0.0f, 1.0f, s.wTone - rustEff * 0.25f));
        mix.setTargetValue (s.wMix);
        wow.setTargetValue (juce::jmin (1.0f, s.wow + rustEff * 0.45f));
        spMix.setTargetValue (juce::jmin (1.0f, s.spMix + rustEff * 0.2f));
        out.setTargetValue (juce::Decibels::decibelsToGain (s.outDb));

        for (int c = 0; c < 2; ++c) dry.copyFrom (c, 0, buf, c, 0, n);

        // 1. Shaper at 2x.
        {
            juce::dsp::AudioBlock<float> block (buf);
            auto up = os.processSamplesUp (block);
            const int un = (int) up.getNumSamples();
            auto* uL = up.getChannelPointer (0);
            auto* uR = up.getChannelPointer (1);
            const float d0 = drive.getCurrentValue(), d1 = drive.getTargetValue();
            for (int i = 0; i < un; ++i)
            {
                const float d = d0 + (d1 - d0) * (float) i / (float) un;
                uL[i] = shape (s.wMode, uL[i], d);
                uR[i] = shape (s.wMode, uR[i], d);
            }
            os.processSamplesDown (block);
            drive.skip (n);
        }

        const int bits = juce::jmax (2, s.bits - (int) std::round (rustEff * 7.0f));
        const int hold = juce::jmax (1, s.rate + (int) std::round (rustEff * rustEff * 10.0f));
        const float q = bits >= 16 ? 0.0f : std::exp2 ((float) bits - 1.0f);
        const int dlen = (int) delay.size();
        const float dust = rustEff * rustEff;

        for (int i = 0; i < n; ++i)
        {
            // Tone: one-pole lowpass after the shaper, then wet/dry.
            if ((i & 15) == 0) toneA = 1.0f - std::exp (-twoPi * 400.0f * std::pow (45.0f, tone.getCurrentValue()) / sr);
            tone.getNextValue();
            const float a = toneA;
            lpL += a * (L[i] - lpL);
            lpR += a * (R[i] - lpR);
            const float m = mix.getNextValue();
            // Shapers normalise to ~unity, so trim to leave headroom for chords, reverb and the output knob.
            float l = (dry.getSample (0, i) * (1.0f - m) + lpL * m) * 0.35f;
            float r = (dry.getSample (1, i) * (1.0f - m) + lpR * m) * 0.35f;

            // 2. Crush.
            if (++holdCount >= hold) { holdCount = 0; heldL = l; heldR = r; }
            l = heldL; r = heldR;
            if (q > 0) { l = std::round (l * q) / q; r = std::round (r * q) / q; }

            // 3. Tape wow and flutter through a short modulated delay.
            const float w = wow.getNextValue();
            wowPh += 0.55f / sr; if (wowPh >= 1) wowPh -= 1;
            flPh += 6.3f / sr; if (flPh >= 1) flPh -= 1;
            delay[(size_t) wpos] = l; delayR[(size_t) wpos] = r;
            const float dms = 6.0f + w * (4.0f * std::sin (twoPi * wowPh) + 0.6f * std::sin (twoPi * flPh));
            float rp = (float) wpos - dms * 0.001f * sr;
            while (rp < 0) rp += (float) dlen;
            const int i0 = (int) rp; const float fr = rp - (float) i0; const int i1 = (i0 + 1) % dlen;
            const float wl = delay[(size_t) i0] + fr * (delay[(size_t) i1] - delay[(size_t) i0]);
            const float wr = delayR[(size_t) i0] + fr * (delayR[(size_t) i1] - delayR[(size_t) i0]);
            if (++wpos >= dlen) wpos = 0;
            const float wmix = juce::jmin (1.0f, w * 4.0f);
            l = l + (wl - l) * wmix;
            r = r + (wr - r) * wmix;

            // 4. Dust: hiss and the odd crackle as RUST climbs. It rides the signal's envelope so it dies with the note
            //    instead of hissing forever after everything is released.
            const float lvl = std::abs (l) + std::abs (r);
            dustEnv = lvl > dustEnv ? dustEnv + (lvl - dustEnv) * 0.01f : dustEnv * 0.99985f;
            if (dust > 0.0001f && dustEnv > 1.0e-4f)
            {
                const float g = juce::jmin (1.0f, dustEnv * 4.0f) * dust;
                const float h = (rng.nextFloat() * 2.0f - 1.0f) * g * 0.01f;
                const float crack = rng.nextFloat() < dust * 0.0004f ? (rng.nextFloat() - 0.5f) * g * 0.35f : 0.0f;
                l += h + crack; r += h - crack * 0.6f;
            }
            L[i] = l; R[i] = r;
        }

        // 5. Space.
        juce::Reverb::Parameters p;
        p.roomSize = 0.3f + s.spSize * 0.68f;
        p.damping = s.spDamp;
        p.width = 1.0f;
        const float sm = spMix.getNextValue(); spMix.skip (n - 1);
        p.wetLevel = sm * 0.6f;
        p.dryLevel = 1.0f - sm * 0.5f;
        reverb.setParameters (p);
        reverb.processStereo (L, R, n);

        // 6. DC block, output, soft safety clip.
        for (int i = 0; i < n; ++i)
        {
            const float g = out.getNextValue();
            float l = L[i] - dcL + 0.995f * dcOutL; dcL = L[i]; dcOutL = l;
            float r = R[i] - dcR + 0.995f * dcOutR; dcR = R[i]; dcOutR = r;
            L[i] = softClip (l * g);
            R[i] = softClip (r * g);
        }
    }

private:
    static float softClip (float x)
    {
        const float a = std::abs (x);
        if (a < 0.8f) return x;
        return std::copysign (0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f), x);
    }

    static float shape (int mode, float x, float d)
    {
        switch (mode)
        {
            case 0: { const float g = 1.0f + d * 7.0f; return std::sin (juce::MathConstants<float>::halfPi * x * g) / (1.0f + d * 0.6f); }
            case 1: { const float g = 1.0f + d * 12.0f, b = 0.15f * d; return (std::tanh (g * x + b) - std::tanh (b)) / (1.0f + d * 0.8f); }
            case 2:
            {
                const float g = 1.0f + d * 70.0f;
                const float y = x > 0 ? 1.0f - std::exp (-g * x) : -0.8f * (1.0f - std::exp (g * 1.3f * x));
                return y / (1.0f + d * 1.2f);
            }
            default:
            {
                const float g = 1.0f + d * 20.0f;
                const float y = juce::jlimit (-1.0f, 1.0f, g * x);
                const float q = std::exp2 (12.0f - d * 9.0f);
                return std::round (y * q) / q / (1.0f + d * 1.2f);
            }
        }
    }

    float sr = 48000;
    juce::dsp::Oversampling<float> os { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::SmoothedValue<float> drive, tone, mix, wow, spMix, out;
    juce::AudioBuffer<float> dry;
    juce::Reverb reverb;
    std::vector<float> delay, delayR;
    juce::Random rng;
    int wpos = 0, holdCount = 0;
    float toneA = 0.5f, lpL = 0, lpR = 0, heldL = 0, heldR = 0, wowPh = 0, flPh = 0;
    float dcL = 0, dcR = 0, dcOutL = 0, dcOutR = 0, dustEnv = 0;
};
} // namespace wreck
