# Render quality overhaul — anti-aliasing, HiDPI, text path

**Date:** 2026-05-30
**Branch:** `feat/styling-render-system`
**Trigger:** user reported jagged corners on borders/focus ring; mandate to fix
the fundamental SDL3 rendering assumptions (correctness + performance).

This document is the durable trail for an autonomous, unsupervised session. It
records the diagnosis, the design decisions (made without the user since they
were asleep), and per-phase progress. Each phase is gated on `./build.sh --tests`
green and committed separately.

---

## 1. Diagnosis (systematic-debugging Phase 1, evidence-backed)

Captured a real frame (`tools/ui_cli.py smoke`, software renderer) and zoomed 10×
on the focused "Start Match" button corner. The focus ring and border show a
**hard pixel staircase** — zero anti-aliasing. Axis-aligned integer edges (the
square panel border) look clean; every curve/sub-pixel edge aliases.

Three fundamentals are wrong:

1. **No anti-aliasing.** `SDL_RenderGeometry` hard-rasterizes triangles. The
   tessellation in `geometry.cpp` produces a geometrically smooth curve, but SDL
   fills each triangle with no edge coverage/AA, so the silhouette staircases.
   *This is the user's literal complaint.*

2. **No HiDPI.** `platform/sdl/window.cpp` creates the window without
   `SDL_WINDOW_HIGH_PIXEL_DENSITY`. On a Retina (2×) Mac the renderer draws at
   logical resolution and macOS upscales 2× → blurry, and the staircase is
   smeared. The UI is rendered at half the native resolution.

3. **Text path: per-frame texture churn + a color-space bug.**
   - `font_registry` creates a `TTF_TextEngine` (the cached glyph-atlas path) but
     `draw_executor::render_text` ignores it and does
     `TTF_RenderText_Blended` + `SDL_CreateTextureFromSurface` +
     `SDL_DestroyTexture` **every frame for every string**. Wasteful.
   - The transcriber premultiplies the text color (`push_text_line` →
     `premul(...)`), but text is rendered via `TTF_RenderText_Blended` (which
     expects STRAIGHT alpha) under a straight-alpha BLEND texture. Opaque text
     (a=255) is unaffected (premul is a no-op), but translucent/disabled text
     double-darkens.

---

## 2. Design decision

The correct, performant answer for an immediate-geometry UI on SDL's 2D renderer
(no portable custom shaders, no MSAA on the default target) is **analytic fringe
anti-aliasing** — the Dear ImGui approach: extrude the silhouette by ~1 device
pixel into a band that fades to premultiplied-transparent `(0,0,0,0)`, so the GPU
blends a 1px coverage ramp at the edge. Rendered at **native device resolution**
this is the gold standard: crisp, cheap (a thin band of extra triangles, no extra
render targets, no fill-rate blow-up), renderer-agnostic, and unit-testable.

Considered and rejected: whole-frame **supersampling (SSAA)**. Simpler (no
geometry surgery) but blunter (box-downsample vs analytic 1px coverage), 4–9×
fill, and it still needs device-resolution glyphs. Kept as the documented
fallback if fringe AA proves too risky (systematic-debugging Phase 4.5).

**Key properties of the fringe design:**
- Feather only **curved** silhouettes (corner_radius > 0.5). Pure axis-aligned
  rectangles stay hard quads — they don't alias at integer positions and we don't
  want to soften crisp UI edges. (Square borders/fills unchanged.)
- The feather is **1 device pixel** wide. Geometry is tessellated in UI points;
  the executor passes `feather = 1/scale` points so the band is exactly 1px after
  the device scale. Default `feather = 0` ⇒ no fringe ⇒ existing call sites and
  unit tests are byte-for-byte unchanged; only the executor opts in.
- Sub-pixel-accurate: the solid core is inset by feather/2 and the fringe extends
  feather/2 outward, so the 50%-coverage line lands on the nominal edge.
- Reuses the existing paired-ring machinery (shared `seg`, `build_ring_seg`,
  `emit_band`) that already survived adversarial review for topology matching.

---

## 3. Execution plan (each phase: implement → `./build.sh --tests` green → commit → screenshot)

- **Phase 1 — Fringe AA** (geometry.cpp + executor `feather`). The headline;
  provable in the headless software capture. Adversarial-review the geometry diff.
- **Phase 2 — HiDPI device-resolution rendering + text path overhaul** (window
  flag; thread device `scale` through the executor for geometry/clip/feather;
  render glyphs at device size via the text engine / a texture cache; fix the
  straight-vs-premultiplied text color).
- **Phase 3 — Final audit, full tests, report, hand-off.**

---

## 4. Progress log

- **Phase 1 — Fringe AA: DONE & green (26/26).** Added an opt-in `feather`
  parameter to `tessellate_rect_fill` / `gradient_fill_colors` /
  `tessellate_frame` (default 0 ⇒ legacy output unchanged). Feathered fills emit
  a centroid-fanned solid core inset by feather/2 plus a transparent fringe ring
  (`emit_fill`); feathered bands add outer+inner fringes around the solid core
  (`emit_band` + `bridge_rings`), with the half-feather clamped to the thinnest
  side so thin borders never invert. Square (radius ≤ 0.5) shapes stay hard quads.
  The executor passes `feather = 1.0` (1 device px at scale 1). New geometry unit
  tests assert fringe presence + core containment + square-ignores-feather +
  overflow. Golden fixtures regenerated. Verified visually: the focus-ring corner
  staircase is now a smooth ramp (`/tmp/ui_aa/compare_corner.png`).
- **Phase 2a — HiDPI scale plumbing: DONE & green (26/26).** Window requests
  `SDL_WINDOW_HIGH_PIXEL_DENSITY`; a device `scale` (from
  `SDL_GetWindowPixelDensity`) threads through `execute_draw_commands` — geometry
  verts, text font-size + dst, image dst, and clip rects all multiply by `scale`,
  and the AA feather is `1/scale` so the fringe stays exactly 1 device pixel.
  Layout stays in points (visual sizes unchanged). New golden scene 4 renders
  scene 1 at scale=2 with semantic probes + a golden lock; the other 3 goldens
  are byte-identical (scale==1 unchanged). **Diagnostic:** the local display is
  1920×1080, density 1.0 (NOT Retina) — so the user's jaggies were purely the
  no-AA issue, and HiDPI is a correct no-op here (a real win only on Retina).
- **Phase 2b — Full-scene supersampling (SSAA): DONE & green (26/26).** Because
  this user is at density 1.0, native 1px AA still steps slightly; the crisp fix
  is SSAA, reusing the Phase-2a `scale` knob. `UiSurface::begin_frame` binds a
  cached offscreen target sized `output * supersample`, the UI renders into it at
  `scale = density * supersample`, and `resolve_frame` box-downsamples it onto
  the window with a linear filter (anti-aliasing everything — curves, gradients,
  text — on top of the per-primitive feather). Policy: 2× on standard displays,
  1× on Retina (already native). Target lifecycle owned by `UiSurface` (freed in
  `App::shutdown` before the renderer dies). Verified headless (density 1 × 2 →
  renders 1600×1000 → downsamples to 800×500): the focus-ring corner is now a
  clean arc (`/tmp/ui_ssaa/compare3.png`: before → fringe AA → AA+SSAA).
- **Phase 3 — Text path perf + color fix: DONE & green (26/26).** (1) Perf: the
  executor rebuilt a `TTF_RenderText_Blended` surface + GPU texture EVERY frame
  for EVERY string (and the `TTF_TextEngine` FontRegistry created was unused dead
  code). Added a fixed-capacity LRU texture cache in `FontRegistry`
  (`cached_text_texture`, keyed by string+pixel_size+straight-color); a label
  that repeats across frames is now rasterized + uploaded ONCE. Removed the dead
  engine. (2) Color: the IR text color is premultiplied (by the color-space
  contract — the transcriber + its tests are unchanged), but `render_text` fed it
  to TTF as if straight, double-darkening translucent/disabled text. Now
  un-premultiplied at render time. New golden-suite cache checks assert
  hit-reuse + per-field key distinction (skip gracefully without a system font).
- **Phase 4 — Adversarial verification: DONE, zero confirmed defects.** Ran a
  multi-agent review workflow over the whole diff (241b3b3..HEAD) across four
  dimensions (geometry-AA, executor-scale, ssaa-surface, text-cache); every
  finding was then handed to an independent skeptic told to refute it. Result: 3
  findings raised, **all 3 refuted**, 0 confirmed. The geometry-AA concern
  (possible degenerate fringe quads) was disproved by a verifier that numerically
  reimplemented the tessellation and swept box sizes / border widths / radii /
  feather — zero reversed-winding quads, `clamp_half_feather` keeps the core
  strictly positive (≥0.05px even at 0.5px borders). The other two were
  speculative exception-safety suggestions, refuted against the no-exceptions
  policy + the no-throw/no-alloc hot path.

---

## 5. Summary (what shipped)

The user's "jagged corners" were `SDL_RenderGeometry` doing no anti-aliasing.
Four green-gated commits on `feat/styling-render-system` (each `./build.sh
--tests` = 26/26):

1. **Fringe AA** (`2d0c106`) — analytic 1-device-pixel edge feather on curved
   silhouettes (fills, gradients, borders, focus rings). Fixes the jaggies.
2. **HiDPI scale plumbing** (`16f7898`) — `SDL_WINDOW_HIGH_PIXEL_DENSITY` + a
   device `scale` through the executor so the UI renders at native resolution on
   Retina (correct no-op on the local 1080p display).
3. **Full-scene SSAA** (`d472ed2`) — render the UI 2× into an offscreen target
   and box-downsample, for crisp edges on standard-density displays (the actual
   win for this 1080p user).
4. **Text path** (`b3ee7f6`) — kill per-frame glyph re-rasterization with an LRU
   texture cache in `FontRegistry`; fix premultiplied-vs-straight text color
   (translucent text was double-darkening); remove the dead text engine.

Not pushed (per the established branch workflow). Independent adversarial review:
clean.
