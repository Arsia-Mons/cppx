# Render modes: three border-smoothing strategies, side by side

**Branch:** `feat/styling-render-system` · **Status:** implemented, 26/26 green.

## Why

"How does a UI library get an infinitely smooth border radius?" has three
textbook answers. Rather than pick one, this project keeps all three behind a
single runtime switch so they can be compared on the same scene:

| Mode | What it does | Cost | Look |
| --- | --- | --- | --- |
| **SSAA** | Render hard-edged geometry into a 2× offscreen, box-downsample on resolve. AA comes entirely from the downsample. | ~4× fill | smooth but slightly soft |
| **Fringe AA** | Extrude each rounded silhouette into a 1-device-pixel band that fades to transparent (Dear ImGui `AntiAliasedFill`). The core stays a tessellated polygon. | ~1.0× | crisp, faintly faceted on big radii |
| **SDF** | Evaluate the rounded-rect signed-distance field **per pixel** into a coverage mask, blit it tinted. The curve is exact at any size. | mask gen, then cached | the "infinitely smooth" one |

SSAA was the previous full-scene default; Fringe AA was the previous
per-primitive default (they were layered together before this change). They are
now **mutually exclusive, pure** modes so the difference is visible.

## Architecture — one source of truth, two consumers

`RenderMode { Ssaa, FringeAa, Sdf }` (`src/renderer/render_mode.h`) is the single
source of truth — an **explicit variant, not three boolean flags** (the
composition-patterns "no boolean-prop proliferation" rule applied to C++). It is
pure data: no SDL, no game vocabulary. Two pipeline stages each read it through
**one pure mapping function**, so the policy lives in exactly one place:

```
                         RenderMode  (chosen by app/, in GameLoop)
                          /                              \
   frame stage:  supersample_for(mode, density)     primitive stage: execute_draw_commands(... mode ...)
   (UiSurface::begin_frame)                          switch on mode per rounded shape
        |                                                   |
   SSAA -> 2× offscreen + downsample                  Ssaa     -> hard tessellation (feather 0)
   else -> native res                                 FringeAa -> 1px feather tessellation (legacy)
                                                       Sdf      -> sdf_raster.cpp coverage masks
```

This is the React mental model in C++ terms: a discriminated-union "state"
consumed by independent "components" (pipeline stages) via pure selectors —
never a pile of `useSsaa` / `useFringe` / `useSdf` booleans that can contradict
each other.

### What is shared (the DRY boundary)

Only **rounded vector primitives** (`Rect` fill, `Border`+outline, `Gradient`)
branch on the mode. Everything else is mode-independent and shared verbatim:
the component → `VisualStyle` → `DrawCommand` IR pipeline, text (TTF), images,
nine-slice, drop shadows, clipping, and group-opacity layers. Text is
byte-identical across all three modes — proof the boundary holds.

The executor computes the per-mode policy **once** per frame (not per vertex):

```cpp
const bool  use_sdf  = (mode == RenderMode::Sdf);
const float feather  = (mode == RenderMode::FringeAa) ? (1.0f / scale) : 0.0f;
```

so `FringeAa` at `scale==1` reproduces the legacy path **byte for byte** — every
pre-existing golden stayed unchanged; the only new golden is the SDF scene.

## The SDF rasterizer (`src/renderer/sdf_raster.{h,cpp}`)

Per device pixel: `cov = clamp(0.5 - d, 0, 1)` where `d` is the signed distance
(in device px) to the rounded-rect surface (Inigo Quilez `sdRoundBox`). 50%
coverage lands on the nominal edge — same sub-pixel convention as the fringe band.

- **Coverage mask = premultiplied white** `(cov,cov,cov,cov)`. Tint is applied at
  blit via SDL color/alpha-mod, so the mask is **color-independent and cached**
  by device-pixel geometry (`SdfMaskCache`, an LRU owned by `UiSurface`, freed in
  `shutdown()` before the renderer dies). One mask serves every color.
- **Fill** → cached coverage mask, tinted. **Border + outline** → cached ring
  masks (`inside(d) − inside(d+band)`). **Gradient** → per-pixel mask (color
  varies, so uncached).
- Blends under `SDL_BLENDMODE_BLEND_PREMULTIPLIED`, matching the IR contract.

### v1 limitations (deliberate, documented)

- Per-side **border colors/widths collapse to one ring** in SDF mode (uniform
  width = widest side, color = first non-transparent side). The theme uses
  uniform borders, so the live look is faithful. Per-side SDF is a future step.
- Gradient masks are regenerated per frame (gradients are rare); the backing
  buffer is reused (a `static` scratch) so there is no per-frame heap alloc.
  Fills/rings are cached as textures.
- **Mask cache keys** quantize device-pixel radii to 1/4 px (`quant`) and pack
  into a 56-bit key via `make_mask_key`, which `SDL_assert`s its field bounds
  (mask dims < 8192, quantized radii < 16384) so an out-of-range geometry fails
  loudly rather than silently colliding. Masks are capped at `kMaxMaskDim`
  (4096) device px per side; larger fills fall back to a hard rect.
- **SDF gradient vs cached fill precision:** gradient pixels are premultiplied in
  float (`col*cov`), while cached fills/rings tint via SDL color/alpha-mod. These
  can differ by ±1 per channel on edge pixels — absorbed by the golden tolerance
  and imperceptible. Unifying them is a future nicety, not a correctness issue.
- **SSAA at the viewport edge:** geometry lying exactly on the render-target
  boundary is clipped by the rasterizer before the 2× buffer sees it, so it
  can't be anti-aliased on resolve — an inherent SSAA trait. The analytic modes
  (Fringe/SDF) don't have this; UI rarely extends to the very edge.
- A null `SdfMaskCache` is legal everywhere (masks are then transient) — keeps
  the golden harness and any non-`UiSurface` caller simple.

## How to toggle

| Surface | How |
| --- | --- |
| Interactive | **F2** cycles SSAA → Fringe AA → SDF. Active mode shown in the window title. |
| Headless / per process | `UI_RENDER_MODE=ssaa\|fringe\|sdf` env var (read at startup). |
| Scripted / live | control-mailbox op `render_mode` (`tools/ui_cli.py render_mode --mode sdf`), switchable inside a running process. |

## Files

- `src/renderer/render_mode.h` — the enum + pure mapping functions.
- `src/renderer/sdf_raster.{h,cpp}` — the SDF rasterizer + mask cache.
- `src/renderer/draw_executor.{h,cpp}` — per-mode dispatch (the primitive stage).
- `src/renderer/ui_surface.{h,cpp}` — owns the `SdfMaskCache`.
- `src/app/game_loop.{h,cpp}` — owns the active mode; env init, F2 cycle, window
  title, mailbox-driven switch; frame-stage supersample policy.
- `src/platform/control_mailbox.{h,cpp}` — `render_mode` op (forwards the raw
  slug; stays renderer-agnostic).
- `tools/ui_cli.py` — `render_mode` subcommand.
- `tests/renderer_golden_tests.cpp` — SDF golden scene (fill + frame + gradient)
  with semantic probes; golden `tests/fixtures/golden/new_ir_sdf.bmp`.

## Tested

`./build.sh --tests` → 26/26. The SDF golden probes assert the corner is cut, the
fill is solid, the frame is hollow, and the gradient runs the right direction —
independent of the pinned bytes. A one-process live-switch harness confirms the
mailbox op flips modes inside a running binary (the three captures differ).
