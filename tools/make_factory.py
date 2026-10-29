# Source of truth for the factory bank. Run: python3 tools/make_factory.py  -> presets/factory.json
# Params not listed fall back to their defaults (see Source/Params.h).
import json
W = dict(SAW=0, PULSE=1, TRI=2, SINE=3, CHIP=4)
F = dict(LP12=0, LP24=1, BP=2, HP=3)
M = dict(FOLD=0, TAPE=1, FUZZ=2, CRUSH=3)
L = dict(SINE=0, TRI=1, SAW=2, SQUARE=3, SH=4)

def P(name, tags, notes, bank="", author="WRECKAGE", **p):
    for k, v in list(p.items()):
        if k.endswith("_wave") and isinstance(v, str):
            p[k] = (L if k == "lfo_wave" else W)[v]
        if k == "flt_mode" and isinstance(v, str): p[k] = F[v]
        if k == "wrk_mode" and isinstance(v, str): p[k] = M[v]
    return dict(name=name, author=author, tags=tags, notes=notes, bank=bank, version=1, params=p)

bank = [
 # ---- leads: Crystal Castles side
 P("CONDEMNED", ["lead","chip"], "Bit-crushed chip lead. Mod wheel adds rust.",
   oscA_wave="CHIP", oscA_pw=0.3, oscB_wave="PULSE", oscB_pw=0.25, oscB_level=0.5, oscB_fine=12,
   flt_cutoff=6000, flt_res=0.35, wrk_mode="CRUSH", wrk_drive=0.4, crush_bits=8, crush_rate=2,
   aenv_a=0.002, aenv_d=0.2, aenv_s=0.7, aenv_r=0.2, glide=0.04, mono=1, space_mix=0.2,
   lfo_rate=5.5, lfo_pitch=0.03),
 P("BLEACHED TONGUE", ["lead","chip","dirty"], "Thin pulse through fuzz and 6-bit crush.",
   oscA_wave="PULSE", oscA_pw=0.12, crush_bits=6, wrk_mode="FUZZ", wrk_drive=0.5, wrk_tone=0.7,
   flt_cutoff=9000, mono=1, glide=0.06, space_mix=0.18),
 P("NO TRESPASS", ["lead","dirty"], "Screaming unison saw. Push RUST for the scream.",
   uni_voices=5, uni_detune=0.35, oscB_wave="SAW", oscB_level=0.6, oscB_fine=-8,
   flt_cutoff=3500, flt_res=0.5, flt_env=0.5, wrk_mode="FUZZ", wrk_drive=0.7, wrk_tone=0.55,
   lfo_rate=6, lfo_pitch=0.04, space_mix=0.2),
 P("DEAD PIXEL", ["pluck","chip"], "Chip pluck for arps.",
   oscA_wave="CHIP", oscA_pw=0.5, aenv_d=0.15, aenv_s=0, aenv_r=0.1, fenv_d=0.12, fenv_s=0,
   flt_env=0.6, flt_cutoff=1500, crush_bits=7, space_mix=0.25),
 P("KILL SWITCH", ["lead","chip"], "Square lead gated by a synced 1/16 LFO.",
   oscA_wave="PULSE", oscA_pw=0.5, oscB_wave="CHIP", oscB_level=0.35, oscB_oct=1,
   lfo_wave="SQUARE", lfo_sync=1, lfo_div=4, lfo_amp=1, mono=1, flt_cutoff=5000,
   wrk_mode="CRUSH", wrk_drive=0.3, space_mix=0.22),
 P("WRECKEM", ["lead","signature","dirty"], "His. Wide saw with a pulse an octave down, fuzz and a bit of rust.",
   uni_voices=5, uni_detune=0.3, oscB_wave="PULSE", oscB_oct=-1, oscB_level=0.5,
   flt_cutoff=2200, flt_res=0.35, flt_env=0.35, wrk_mode="FUZZ", wrk_drive=0.45, rust=0.35,
   space_mix=0.25),
 P("SIREN", ["fx","lead"], "Slow pitch sweep through fuzz.",
   lfo_wave="TRI", lfo_rate=0.4, lfo_pitch=0.6, wrk_mode="FUZZ", wrk_drive=0.5, mono=1,
   space_mix=0.3, flt_cutoff=5000),

 # ---- pads: Salem side
 P("WITCH HOUSE", ["pad","dark"], "Huge slow saw pad, tape-warped. Play it low.",
   oscA_oct=-1, uni_voices=7, uni_detune=0.5, uni_spread=1, flt_cutoff=1200, flt_res=0.25,
   aenv_a=1.2, aenv_d=2, aenv_s=0.8, aenv_r=3, tape_wow=0.45, wrk_mode="TAPE", wrk_drive=0.3,
   wrk_tone=0.35, space_mix=0.45, space_size=0.9, lfo_rate=0.15, lfo_cutoff=0.2),
 P("BLEACH CHOIR", ["pad"], "Hollow choir pad, bit-faded.",
   oscA_wave="TRI", oscB_wave="SAW", oscB_oct=1, oscB_level=0.3, uni_voices=5, uni_detune=0.4,
   flt_cutoff=2500, flt_res=0.4, aenv_a=2, aenv_s=1, aenv_r=4, space_mix=0.55, tape_wow=0.6,
   crush_bits=12),
 P("DROWNED", ["pad","dark","drone"], "Pitched-down, underwater, smeared.",
   oscA_oct=-2, uni_voices=7, uni_detune=0.7, flt_cutoff=700, tape_wow=0.8, space_mix=0.6,
   space_damp=0.8, wrk_mode="TAPE", wrk_drive=0.4, crush_rate=3, aenv_a=1.5, aenv_r=4),
 P("SODIUM LAMP", ["pad"], "Warm PWM pad that breathes.",
   oscA_wave="PULSE", oscA_pw=0.4, lfo_pw=0.5, lfo_rate=0.3, oscB_wave="SAW", oscB_fine=-9,
   oscB_level=0.6, flt_cutoff=1800, aenv_a=0.8, aenv_r=2.5, space_mix=0.4),
 P("FLOOD LEVEL", ["drone","dark"], "Resonant dark drone with a slow filter tide.",
   oscA_oct=-1, sub_level=0.5, noise_level=0.08, flt_cutoff=500, flt_res=0.6, lfo_rate=0.08,
   lfo_cutoff=0.35, space_mix=0.7, space_size=1, tape_wow=0.3, aenv_a=2, aenv_r=5, out_gain=-8),
 P("DRAPE", ["pad","dark"], "Restrained. Sine and sub, barely any dirt.",
   oscA_wave="SINE", sub_level=0.4, wrk_mode="TAPE", wrk_drive=0.2, aenv_a=0.6, aenv_r=2,
   space_mix=0.35, flt_cutoff=3000),
 P("VEIL", ["pad"], "Folded sine pad, glassy.",
   oscA_wave="SINE", oscB_wave="TRI", oscB_oct=1, oscB_level=0.4, wrk_mode="FOLD", wrk_drive=0.35,
   aenv_a=1, aenv_r=2.5, space_mix=0.5, uni_voices=3, uni_detune=0.2),

 # ---- bass
 P("CROWBAR SUB", ["bass","sub"], "Saturated sub. Mono with a little glide.",
   oscA_wave="SINE", oscA_level=0.6, sub_level=0.9, wrk_mode="TAPE", wrk_drive=0.55, wrk_tone=0.4,
   flt_cutoff=300, flt_env=0, aenv_a=0.003, aenv_d=0.5, aenv_s=0.9, aenv_r=0.15, mono=1,
   glide=0.05, space_mix=0),
 P("RUST BELT", ["bass","dirty"], "Grimy reese with a synced filter wobble.",
   uni_voices=3, uni_detune=0.25, oscB_wave="SAW", oscB_fine=-12, oscB_level=0.7, flt_cutoff=900,
   flt_res=0.3, flt_drive=0.4, wrk_mode="FUZZ", wrk_drive=0.45, wrk_tone=0.45, wrk_mix=0.7,
   lfo_sync=1, lfo_div=2, lfo_cutoff=0.15, mono=1, space_mix=0),
 P("ASBESTOS", ["bass"], "Wavefolded triangle bass.",
   oscA_wave="TRI", wrk_mode="FOLD", wrk_drive=0.6, flt_env=0.4, fenv_d=0.25, fenv_s=0,
   flt_cutoff=1500, mono=1, glide=0.03, space_mix=0),
 P("808 FUNERAL", ["bass","sub"], "Long distorted 808 tail.",
   oscA_wave="SINE", sub_level=0.5, aenv_d=1.4, aenv_s=0, aenv_r=0.6, wrk_mode="TAPE",
   wrk_drive=0.7, wrk_tone=0.5, mono=1, glide=0.08, flt_cutoff=2000, space_mix=0),
 P("CHAIN LINK", ["bass","pluck"], "Resonant pulse pluck bass.",
   oscA_wave="PULSE", oscA_pw=0.3, flt_env=0.7, fenv_d=0.18, fenv_s=0, flt_cutoff=250,
   flt_res=0.55, crush_bits=10, mono=1, space_mix=0),
 P("BLACK DENIM", ["bass"], "Dark square bass, 24dB filter.",
   oscA_wave="PULSE", uni_voices=3, uni_detune=0.15, flt_mode="LP24", flt_cutoff=600,
   wrk_mode="FUZZ", wrk_drive=0.3, mono=1, space_mix=0),

 # ---- keys / plucks
 P("TETANUS", ["pluck","keys"], "Dull metal pluck.",
   oscB_wave="PULSE", oscB_level=0.5, aenv_d=0.35, aenv_s=0, aenv_r=0.3, flt_env=0.6, fenv_d=0.2,
   fenv_s=0, flt_cutoff=800, flt_res=0.4, space_mix=0.35, tape_wow=0.2),
 P("CONCRETE BELL", ["keys","pluck"], "Folded bell, cold.",
   oscA_wave="SINE", oscB_wave="TRI", oscB_oct=2, oscB_semi=7, oscB_level=0.4, wrk_mode="FOLD",
   wrk_drive=0.45, aenv_d=1.5, aenv_s=0, aenv_r=1.2, space_mix=0.5),
 P("GHOST KEYS", ["keys"], "Warped, half-erased keys.",
   oscA_wave="TRI", tape_wow=0.5, crush_bits=10, crush_rate=2, space_mix=0.4, aenv_a=0.01,
   aenv_d=0.8, aenv_s=0.4, aenv_r=0.8),
 P("BROKEN MUSIC BOX", ["keys","chip"], "Wobbly chip music box.",
   oscA_wave="CHIP", oscA_pw=0.8, oscA_oct=1, aenv_d=0.6, aenv_s=0, aenv_r=0.6, tape_wow=0.7,
   space_mix=0.5, crush_bits=9),

 # ---- fx
 P("STATIC FIELD", ["fx","dirty"], "Sample-and-hold noise field.",
   oscA_level=0.2, noise_level=0.8, flt_mode="BP", flt_res=0.7, flt_cutoff=1500, lfo_wave="SH",
   lfo_rate=8, lfo_cutoff=0.6, crush_bits=5, crush_rate=6, space_mix=0.5, out_gain=4),
 P("FLOODLIGHT RISER", ["fx"], "Hold it for six seconds.",
   noise_level=0.5, uni_voices=7, uni_detune=0.6, aenv_a=6, aenv_s=1, aenv_r=2, fenv_a=6,
   fenv_s=1, flt_env=0.8, flt_cutoff=300, space_mix=0.5),
 P("WRECKING BALL", ["fx","dirty"], "Everything at once.",
   uni_voices=5, uni_detune=0.4, wrk_mode="FUZZ", wrk_drive=0.6, rust=0.85, space_mix=0.3),

 # ---- hidden: OCT 29
 P("HAPPY BDAY JACOB", ["keys","chip"], "Happy birthday Jacob. From Austin, Oct 29 2026.", bank="OCT 29", author="AUSTIN",
   oscA_wave="CHIP", oscA_pw=0.7, oscB_wave="TRI", oscB_oct=1, oscB_level=0.3, aenv_d=0.7,
   aenv_s=0.2, aenv_r=0.7, tape_wow=0.55, space_mix=0.45, crush_bits=9, rust=0.2),
 P("CANDLE WAX", ["pad"], "Slow and warm. Make a wish.", bank="OCT 29", author="AUSTIN",
   oscA_wave="PULSE", oscA_pw=0.35, lfo_pw=0.4, lfo_rate=0.25, uni_voices=5, uni_detune=0.35,
   aenv_a=1.4, aenv_r=3, flt_cutoff=1600, tape_wow=0.35, space_mix=0.5),
 P("OCT 29", ["lead","dirty"], "Birthday lead.", bank="OCT 29", author="AUSTIN",
   uni_voices=3, uni_detune=0.25, oscB_wave="CHIP", oscB_level=0.4, oscB_oct=1, wrk_mode="FUZZ",
   wrk_drive=0.5, rust=0.25, mono=1, glide=0.05, space_mix=0.25),
 P("FROM AUSTIN", ["pluck"], "Have a good one.", bank="OCT 29", author="AUSTIN",
   oscA_wave="TRI", oscB_wave="PULSE", oscB_level=0.4, aenv_d=0.4, aenv_s=0, flt_env=0.5,
   fenv_d=0.25, fenv_s=0, flt_cutoff=1200, space_mix=0.4, tape_wow=0.25),
]
json.dump(bank, open("presets/factory.json", "w"), indent=1)
print(len(bank), "presets")
