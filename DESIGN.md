---
name: WRECKAGE
description: A condemned concrete wall you play; a grimey synth whose face corrodes as you push it.
colors:
  ground: "#121010"
  concrete: "#3f3c39"
  well: "#1c1917"
  well-floor: "#141110"
  screen: "#16100d"
  slot: "#0f0d0c"
  bone: "#e4ddd0"
  bone-dim: "#b3ab9e"
  engrave-fill: "rgba(178, 169, 158, 0.86)"
  coral: "#e0745a"
  coral-hot: "#ffb7a0"
  coral-deep: "#8a3b29"
  paper: "#c9c1b2"
  permit-paper: "#d8d0c0"
  ink: "#1d1a17"
  disabled: "#6d655b"
typography:
  wordmark:
    fontFamily: "Stencil, Yanone, sans-serif"
    fontSize: "54px"
    fontWeight: 800
    lineHeight: 1
    letterSpacing: "0.05em"
  headline:
    fontFamily: "Yanone, Arial Narrow, sans-serif"
    fontSize: "20px"
    fontWeight: 500
    letterSpacing: "0.2em"
  title:
    fontFamily: "Yanone, Arial Narrow, sans-serif"
    fontSize: "19px"
    fontWeight: 400
    letterSpacing: "0.32em"
  label:
    fontFamily: "Yanone, Arial Narrow, sans-serif"
    fontSize: "15px"
    fontWeight: 500
    letterSpacing: "0.2em"
  label-small:
    fontFamily: "Yanone, Arial Narrow, sans-serif"
    fontSize: "13px"
    fontWeight: 400
    letterSpacing: "0.14em"
  macro:
    fontFamily: "Yanone, Arial Narrow, sans-serif"
    fontSize: "28px"
    fontWeight: 500
    letterSpacing: "0.24em"
  body:
    fontFamily: "Yanone, Arial Narrow, sans-serif"
    fontSize: "15px"
    fontWeight: 400
    letterSpacing: "0.02em"
rounded:
  none: "0px"
  cap: "2px"
  slot: "4px"
  round: "50%"
spacing:
  gap: "4px"
  edge: "5px"
  inset: "10px"
  section-top: "14px"
components:
  slab:
    backgroundColor: "{colors.concrete}"
    rounded: "{rounded.none}"
    padding: "14px 10px 10px"
  well-button:
    backgroundColor: "{colors.well}"
    textColor: "{colors.bone-dim}"
    typography: "{typography.label-small}"
    rounded: "{rounded.none}"
    padding: "3px 7px 2px"
  well-button-active:
    backgroundColor: "{colors.well}"
    textColor: "{colors.coral-hot}"
  action-button:
    backgroundColor: "{colors.well}"
    textColor: "{colors.bone}"
    rounded: "{rounded.none}"
    padding: "6px 10px 5px"
  action-button-hover:
    textColor: "{colors.coral-hot}"
  preset-plate:
    backgroundColor: "{colors.well}"
    textColor: "{colors.bone}"
    typography: "{typography.title}"
    height: "34px"
    width: "228px"
  knob-small:
    size: "38px"
    rounded: "{rounded.round}"
  knob-mid:
    size: "58px"
    rounded: "{rounded.round}"
  knob-big:
    size: "94px"
    rounded: "{rounded.round}"
  knob-huge:
    size: "112px"
    rounded: "{rounded.round}"
  knob-rust:
    size: "176px"
    rounded: "{rounded.round}"
  fader-slot:
    backgroundColor: "{colors.slot}"
    rounded: "{rounded.slot}"
    width: "8px"
    height: "214px"
  tag-chip:
    backgroundColor: "{colors.paper}"
    textColor: "{colors.ink}"
    rounded: "{rounded.none}"
    padding: "2px 8px 1px"
  tag-chip-active:
    backgroundColor: "{colors.coral}"
    textColor: "{colors.ink}"
  search-input:
    backgroundColor: "{colors.well}"
    textColor: "{colors.bone}"
    rounded: "{rounded.none}"
    padding: "6px 10px 5px"
  scope-screen:
    backgroundColor: "{colors.screen}"
    rounded: "{rounded.slot}"
---

# Design System: WRECKAGE

## Overview

**Creative North Star: "The Condemned Panel"**

The instrument is a wall, not a panel with a skin. Every surface is board-formed concrete, cold Owens grey, split into slabs by black shadow gaps. Controls are either cut into that concrete (lettering, wells, fader slots, the scope screen) or bolted onto it (matte black cast knobs, the caged tube, anchor bolts). Nothing floats, nothing glows except what the sound lights.

There is one accent, rust coral, and it only ever means value or signal: knob arcs, fader fills, the tube, the scope trace, the active state. The wall decays on demand. The RUST macro writes a live `--rust` (0 to 1) that raises a corrosion overlay from slab edges, lengthens the weeps under anchor holes, and stretches the drips under the RUST knob. The caged fluorescent tube is the output meter and flashes on transients, and the overhead spill light washes down the wall with it.

The stage is a fixed 1200 x 750 artboard scaled whole to the plugin window. Density is high but every control is large and decisive; the giant RUST knob and the largest CUTOFF knob anchor the hierarchy.

**Key Characteristics:**
- Board-formed concrete slabs on a near-black ground, 4px shadow gaps.
- Engraved lettering: bone fill clipped through the concrete texture, dark top lip, light bottom lip.
- One accent, rust coral, reserved for value and signal.
- Matte black cast knobs from a single raster plate; coral SVG arcs and bone needles drawn on top.
- Material that reacts: tube level and flash, overhead spill, RUST corrosion.

## Colors

A cold, nearly colourless concrete-and-bone world with a single rust coral that carries every live value.

### Primary
- **Rust Coral** (coral): value arcs, fader fills (fading to Oxide), the scope trace and graticule, selected-state underlines, focus outlines, the caret, prev/next arrows.
- **Hot Coral** (coral-hot): live numeric values inside labels, hovered arrows and steppers, selected text in wells and the current shelf entry.
- **Oxide** (coral-deep): the dark end of fader fills; the permit headline.

### Neutral
- **Night Ground** (ground): the gaps between slabs and the stage floor.
- **Owens Concrete** (concrete): slab base under the concrete plate, cooled by a 42% blue-grey wash (rgba(42, 43, 46, 0.42)) in the background itself.
- **Cut Well** (well to well-floor): the vertical gradient of every recessed control: preset plate, buttons, segments, inputs.
- **Phosphor Glass** (screen) and **Slot Black** (slot): the scope screen and fader slots.
- **Bone** (bone) and **Dust Bone** (bone-dim): live text and needles; inactive or secondary text.
- **Engrave Fill** (engrave-fill): the bone tint laid over the concrete texture for engraved lettering.
- **Tag Paper** (paper), **Permit Paper** (permit-paper), **Ink** (ink): stuck-on paper tags and the about permit.
- **Dead Stock** (disabled): disabled actions and unset favourites.

### Named Rules
**The One Rust Rule.** Coral marks value, signal, or selection. It is never decoration, never a panel fill, never a heading colour.

**The Wash-Not-Filter Rule.** Concrete is cooled and darkened inside its own background layers, never with a CSS filter on the slab, because a slab filter dulls the knobs and lettering bolted to it.

## Typography

**Display Font:** Big Shoulders Stencil Display (local "Stencil"), wordmark only.
**Body Font:** Yanone Kaffeesatz (local "Yanone", with Arial Narrow fallback), everything else.

**Character:** A stencil wordmark sprayed through a template, and a tall condensed grotesk set in widely tracked caps that reads like stamped plant signage.

### Hierarchy
- **Wordmark** (800, 54px, line-height 1, 0.05em): WRECKAGE, top left, engraved with a deeper lip than other lettering.
- **Headline** (500, 20px, 0.2em): slab names (OSC A, FILTER, WRECK), with a cut rule beneath.
- **Title** (400, 19px, 0.32em): the preset name on the plate and shelf names (0.16em there).
- **Label** (500, 15px, 0.2em): knob labels; fader letters run 16px at 0.1em; bench tabs 16px at 0.3em.
- **Label Small** (13px, 0.12 to 0.14em): small-knob labels, segments, toggles; tightened to 0.06 to 0.08em only where a narrow slab forces it.
- **Macro** (500, 28px, 0.24em): RUST alone.
- **Body** (15px, 0.02em): preset notes in the crates drawer, the only running text.

### Named Rules
**The Engraved Rule.** Lettering on concrete is cut, not printed: engrave-fill over the concrete plate (300px tile) clipped to the glyphs, text fill transparent, a 1px dark drop-shadow above (rgba(0,0,0,0.7)) and a 1px light one below (rgba(236,228,214,0.14)). Applies to the wordmark, slab heads, knob and fader labels, bench tabs and selector positions. Live values inside a label break out in hot coral.

**The Lit-Glass Exception.** Labels on the dark scope screen are lit, not cut: plain dust bone with a black text shadow.

**The Tracked Caps Rule.** All UI text is uppercase and tracked; tracking grows with importance (0.1em controls, 0.2em labels, 0.3em tabs and preset name). Lowercase appears only in user-entered content: tags, notes, search.

## Layout

A fixed 1200 x 750 stage, transformed as a whole to fit the window; nothing reflows. Three bands of slabs separated by 4px gaps with a 5px outer edge: a 109px top band (wordmark, preset plate over the tube, tag), a 326px row of seven section slabs whose widths follow their importance (FILTER widest at 194px), and a 293px bottom row split between the bench (scope) and the WRECK slab. Inside a slab, content stacks centred under the heading (14px top, 10px sides) and secondary knobs pin to the bottom in a two-column pair grid. Panels that open (the crates drawer, the permit) overlay the wall rather than pushing it.

## Elevation & Depth

Depth is physical and mostly carved. Slabs sit proud of the ground with a faint top highlight, an inner bottom shade and a short 3px drop. Recessed things (wells, slots, screen) use inset shadows plus a hairline light lip below. Bolted-on things (knobs, fader caps, the selector grip) cast short soft drop shadows. Only overlays (crates drawer, permit) cast large shadows. Light comes from overhead: every lip and shadow assumes it.

### Shadow Vocabulary
- **Slab** (`inset 0 1px 0 rgba(255,255,255,0.06), inset 0 -2px 6px rgba(0,0,0,0.35), 0 3px 6px rgba(0,0,0,0.6)`): every concrete slab.
- **Well** (`inset 0 1px 2px rgba(0,0,0,0.8)`, 3px blur on the preset plate): all recessed controls.
- **Cast knob** (`drop-shadow(0 4px 5px rgba(0,0,0,0.55))`): knob plates.
- **Overlay** (`10px 0 30px rgba(0,0,0,0.75)` drawer, `0 10px 26px rgba(0,0,0,0.7)` permit): things laid over the wall.

### Named Rules
**The Overhead Light Rule.** Top edges of cuts are dark, bottom edges catch light. Never invert it, never light from the side.

## Shapes

Hard-edged and square. Slabs, wells, buttons, chips and the drawer have no radius; the 4px slot radius belongs only to fader slots and the scope screen bezel, 2px to fader caps and fills. Knobs and anchor holes are the only circles. Paper (tags, permit) is set slightly crooked: chips take a deterministic per-tag rotation of up to about 2deg, the permit sits at -1.2deg.

## Components

### Knobs
Bolted-on, heavy, few sizes. One raster plate (rust-knob.png) at five sizes: small 38, mid 58, big 94, huge 112 (CUTOFF), RUST 176. A coral value arc (stroke 3.6 in a 100-unit viewbox, butt caps) sweeps from the rest angle; bipolar knobs sweep from centre. A bone needle (2.6, round caps) shows position. Vertical drag; RUST drags slower. Focus draws a 1px coral ring 10px off the dial.

### Faders
An 8px slot cut 214px deep, a coral-to-oxide fill, a ribbed metal cap (26 x 30) with a bone index line. Letter label below, engraved.

### Wells (segments, toggles, cycle, action buttons, preset plate)
Recessed in the cut-well gradient. Rest text dust bone (bone on action buttons), hover bone (hot coral on action buttons), selected text hot coral with a 2px coral inset rule along the bottom.

### Tabs
Engraved words on the bench under the screen; the selected tab keeps its bone fill and gains a 2px coral rule beneath.

### Tag Chips
Paper scraps stuck on the drawer: tag paper, ink text, a soft 1px shadow, slight rotation, 0.86 opacity until hovered. Selected chip turns rust coral.

### Crates Drawer
A darker concrete sheet that slides from the left over the section row (0.32s, cubic-bezier(0.16, 1, 0.3, 1); none under reduced motion). Search input in a well with coral caret and focus outline; the shelf lists presets with the current one in a coral wash fading to the right.

### Tube Meter (signature)
The caged fluorescent tube plate sits twice: a desaturated dark copy, and a lit copy clipped from the right by `--level`. On a transient `--flash` brightens and desaturates the lit copy and the overhead spill blooms (screen blend). Together with the scope phosphor it is the only light in the world.

### Scope
A dark phosphor screen in a 5px bezel with a coral graticule (10 x 6). The trace is drawn twice, a wide faint glow and a 2.4px coral core, with short persistence.

### RUST Corrosion (signature)
`--rust` drives corrosion overlay opacity on every slab (mirrored per slab so no two match), weep length under anchor holes (70px to 180px), and drip length under the RUST knob.

## Do's and Don'ts

### Do:
- **Do** cut every word that sits on concrete using the Engraved Rule; break live values out in hot coral.
- **Do** keep coral for values, signal and selection only.
- **Do** mark a selected control with hot coral text or a 2px coral rule, never with a filled coral block (paper chips excepted).
- **Do** drive any new decay or reaction from `--rust`, `--level` and `--flash` rather than new timers.
- **Do** cool and darken concrete in its background layers, keeping filters off slabs.
- **Do** keep everything on the fixed 1200 x 750 stage and scale the whole.

### Don't:
- **Don't** print flat light text for wall lettering (slab heads, control labels, tabs); it reads as a skin, which is what the world refuses. Lists and inputs inside the crates drawer and labels on the scope glass are read, not carved, and stay flat.
- **Don't** add a second accent hue or tint panels with coral.
- **Don't** round slabs, wells or buttons.
- **Don't** add light sources beyond the tube, its overhead spill and the scope phosphor.
- **Don't** set the wordmark face anywhere but the wordmark.
