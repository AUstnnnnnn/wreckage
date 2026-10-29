#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// One table drives the APVTS, the UI and the preset files. IDs are stable: never rename.
namespace params
{
inline const juce::StringArray waves    { "SAW", "PULSE", "TRI", "SINE", "CHIP" };
inline const juce::StringArray subWaves { "SINE", "SQUARE" };
inline const juce::StringArray fModes   { "LP12", "LP24", "BP", "HP" };
inline const juce::StringArray lfoWaves { "SINE", "TRI", "SAW", "SQUARE", "S&H" };
inline const juce::StringArray lfoDivs  { "1/1", "1/2", "1/4", "1/8", "1/16", "1/4T", "1/8T", "1/16T" };
inline const juce::StringArray wModes   { "FOLD", "TAPE", "FUZZ", "CRUSH" };

struct Def
{
    const char* id;
    const char* name;
    float min, max, def, skewCentre; // skewCentre <= 0 means linear
    const juce::StringArray* choices = nullptr;
    bool toggle = false;
};

// clang-format off
inline const std::vector<Def>& all()
{
    static const std::vector<Def> d {
        { "oscA_wave",  "Osc A Wave",   0, 4, 0, 0, &waves },
        { "oscA_oct",   "Osc A Octave", -3, 2, 0, 0 },
        { "oscA_semi",  "Osc A Semi",   -12, 12, 0, 0 },
        { "oscA_fine",  "Osc A Fine",   -100, 100, 0, 0 },
        { "oscA_pw",    "Osc A Width",  0.05f, 0.95f, 0.5f, 0 },
        { "oscA_level", "Osc A Level",  0, 1, 0.8f, 0 },

        { "oscB_wave",  "Osc B Wave",   0, 4, 1, 0, &waves },
        { "oscB_oct",   "Osc B Octave", -3, 2, 0, 0 },
        { "oscB_semi",  "Osc B Semi",   -12, 12, 0, 0 },
        { "oscB_fine",  "Osc B Fine",   -100, 100, 7, 0 },
        { "oscB_pw",    "Osc B Width",  0.05f, 0.95f, 0.5f, 0 },
        { "oscB_level", "Osc B Level",  0, 1, 0.0f, 0 },

        { "uni_voices", "Unison",       1, 7, 1, 0 },
        { "uni_detune", "Detune",       0, 1, 0.2f, 0 },
        { "uni_spread", "Spread",       0, 1, 0.6f, 0 },

        { "sub_level",  "Sub Level",    0, 1, 0, 0 },
        { "sub_wave",   "Sub Wave",     0, 1, 0, 0, &subWaves },
        { "sub_oct",    "Sub Octave",   -2, -1, -1, 0 },
        { "noise_level","Noise",        0, 1, 0, 0 },

        { "flt_mode",   "Filter Mode",  0, 3, 1, 0, &fModes },
        { "flt_cutoff", "Cutoff",       20, 20000, 4000, 800 },
        { "flt_res",    "Resonance",    0, 1, 0.2f, 0 },
        { "flt_drive",  "Filter Drive", 0, 1, 0.1f, 0 },
        { "flt_env",    "Filter Env",   -1, 1, 0.3f, 0 },
        { "flt_key",    "Key Track",    0, 1, 0.3f, 0 },

        { "aenv_a",     "Amp Attack",   0.001f, 8, 0.005f, 0.4f },
        { "aenv_d",     "Amp Decay",    0.005f, 8, 0.3f, 0.6f },
        { "aenv_s",     "Amp Sustain",  0, 1, 0.8f, 0 },
        { "aenv_r",     "Amp Release",  0.005f, 12, 0.25f, 0.8f },
        { "fenv_a",     "Flt Attack",   0.001f, 8, 0.005f, 0.4f },
        { "fenv_d",     "Flt Decay",    0.005f, 8, 0.4f, 0.6f },
        { "fenv_s",     "Flt Sustain",  0, 1, 0.3f, 0 },
        { "fenv_r",     "Flt Release",  0.005f, 12, 0.3f, 0.8f },

        { "lfo_wave",   "LFO Wave",     0, 4, 0, 0, &lfoWaves },
        { "lfo_rate",   "LFO Rate",     0.03f, 30, 2, 3 },
        { "lfo_sync",   "LFO Sync",     0, 1, 0, 0, nullptr, true },
        { "lfo_div",    "LFO Division", 0, 7, 2, 0, &lfoDivs },
        { "lfo_pitch",  "LFO > Pitch",  0, 1, 0, 0 },
        { "lfo_cutoff", "LFO > Cutoff", 0, 1, 0, 0 },
        { "lfo_amp",    "LFO > Amp",    0, 1, 0, 0 },
        { "lfo_pw",     "LFO > Width",  0, 1, 0, 0 },

        { "wrk_mode",   "Wreck Mode",   0, 3, 1, 0, &wModes },
        { "wrk_drive",  "Wreck Drive",  0, 1, 0.25f, 0 },
        { "wrk_tone",   "Wreck Tone",   0, 1, 0.6f, 0 },
        { "wrk_mix",    "Wreck Mix",    0, 1, 1, 0 },
        { "crush_bits", "Bits",         2, 16, 16, 0 },
        { "crush_rate", "Downsample",   1, 32, 1, 0 },
        { "tape_wow",   "Wow",          0, 1, 0, 0 },
        { "space_size", "Space Size",   0, 1, 0.6f, 0 },
        { "space_damp", "Space Damp",   0, 1, 0.6f, 0 },
        { "space_mix",  "Space Mix",    0, 1, 0.15f, 0 },

        { "rust",       "RUST",         0, 1, 0, 0 },
        { "glide",      "Glide",        0, 1.5f, 0, 0.25f },
        { "mono",       "Mono",         0, 1, 0, 0, nullptr, true },
        { "bend",       "Bend Range",   1, 24, 2, 0 },
        { "velocity",   "Velocity",     0, 1, 0.5f, 0 },
        { "out_gain",   "Output",       -24, 6, -3, 0 },
    };
    return d;
}
// clang-format on

inline bool isInt (const Def& d)
{
    return d.choices || d.toggle || juce::String (d.id).endsWith ("_oct") || juce::String (d.id).endsWith ("_semi")
        || juce::String (d.id) == "uni_voices" || juce::String (d.id) == "crush_bits" || juce::String (d.id) == "crush_rate"
        || juce::String (d.id) == "bend";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    for (auto& d : all())
    {
        juce::ParameterID pid { d.id, 1 };
        if (d.toggle)
            l.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.def > 0.5f));
        else if (d.choices)
            l.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name, *d.choices, (int) d.def));
        else if (isInt (d))
            l.add (std::make_unique<juce::AudioParameterInt> (pid, d.name, (int) d.min, (int) d.max, (int) d.def));
        else
        {
            juce::NormalisableRange<float> r (d.min, d.max);
            if (d.skewCentre > 0)
                r.setSkewForCentre (d.skewCentre);
            l.add (std::make_unique<juce::AudioParameterFloat> (pid, d.name, r, d.def));
        }
    }
    return l;
}
} // namespace params
