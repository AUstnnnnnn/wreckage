# Product

<!-- impeccable:product-schema 1 -->

## Platform

web

The UI is HTML/CSS/JS hosted in a JUCE WebView inside a VST3 / AU / Standalone audio plugin. Fixed-size plugin window, mouse + keyboard, no mobile.

## Stack
delegated: JUCE 8.0.10 (C++17, CMake) for DSP and plugin wrappers, JUCE WebBrowserComponent (WebView2 on Windows, WKWebView on macOS) for the UI. Chosen for VST3 + AU from one codebase, free Starter licence, and a web UI that can carry shader-driven material. Windows builds via GitHub Actions; macOS builds locally.

## Users
One person: Jacob, producer name WRECKEM. Makes dark electronic music in FL Studio on Windows. Designs his own Serum presets and wants to do the same here. Into Crystal Castles, Salem (witch house), Rick Owens.

## Product Purpose
WRECKAGE is a birthday gift (Oct 29, 2026, from Austin): a grimey polyphonic synth with a heavy distortion chain and a deep bank of factory presets, built for his sound. Success: he loads it in FL Studio, the first preset hits, and he starts making and tagging his own patches the same night.

## Positioning
Made for one producer. The sound targets his three references (CC chip-crush leads, Salem pitched-down drowned pads, Owens-weight restraint), and the instrument visibly decays as you push it.

## Operating Context
Inside FL Studio's plugin wrapper on a Windows desktop, often late at night, in a dim room. Used in sessions: audition presets, tweak, save his own, tag them, come back to them later.

## Capabilities and Constraints
- Synth: 2 oscillators + sub + noise, filter with drive, 2 envelopes, LFO, unison, glide, WRECK distortion chain (fold / tape / fuzz / crush), tape wow, dark reverb, one RUST macro.
- Presets: factory bank + user presets saved as plain files in Documents/WRECKAGE. User can save, overwrite, rename, delete, tag (free-form tags), author, notes, favourite, search and filter by tag, step prev/next, import/export files.
- Hidden birthday bank and easter eggs, dated Oct 29 2026, "for Jacob, from Austin".
- macOS build is unsigned (Gatekeeper workaround acceptable; he's on Windows).

## Brand Commitments
- Name: WRECKAGE. His tag: WRECKEM.
- Pinned world from the brief: rusty, abandoned warehouse.
- References he loves: Crystal Castles, Salem, Rick Owens.

## Evidence on Hand
No logo, artwork, or audio from him yet. His Instagram (wreckm._) pending a read. Do not fabricate his handwriting, real artwork, or quotes.

## Product Principles
1. Sound first: every preset must be usable in a track, not a demo.
2. Making his own presets is a first-class job, never buried.
3. Grime is earned by the controls: the UI decays because the sound does.
4. Few controls, each one big and decisive.
