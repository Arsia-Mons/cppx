# Styling & Render System Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the current fused-`Style` + role-switching draw builder + texture-blit renderer with a from-first-principles, component-resolved styling system and a tagged-union `DrawCommand` IR rendered by a dumb SDL executor.

**Architecture:** Components resolve their own dense `VisualStyle` (theme + variant + interaction) at authoring time and pass it as a prop; `build_draw_list` is a pure transcriber to a POD `DrawCommand` IR; an SDL renderer linearly executes the IR with premultiplied alpha. Layout (`LayoutStyle`) and paint (`VisualStyle`) are split. One injected `MeasureTextFn` drives both Yoga measure and draw (measure==draw).

**Tech Stack:** C++20 (no exceptions/RTTI in ui/runtime), SDL3 + SDL3_ttf, Yoga, a React-style hook runtime (`src/react.h`), CMake + ctest, Python CLI smoke tests.

**Companion spec (canonical, single source of truth for all types/algorithms):** `docs/retained-ui/styling-render-system-design.md`. This plan references spec sections (e.g. "§5.2") for code bodies rather than duplicating them — when a step says "implement per §X", copy the canonical listing from that spec section verbatim. The spec's §15 lists what is **cut**; never implement those.

**Branch:** `feat/styling-render-system` (already created; spec committed). One commit per task/sub-step as noted. Do NOT push.

**Gate discipline (every phase):** the phase is not done until `./build.sh --tests` is fully green under `-Wall -Wextra`. Overflow paths must increment `error_count` and fail the frame — never clamp/truncate.

---

## File Structure (from spec §12.7)

**New (`src/ui/style/` — generic, SDL-free, game-free):**
- `visual_style.h` — `Color, Vec2, SideWidths, SideColors, Border, Outline, GradientStop, Gradient, BackgroundImage, Shadow, TextAlign, TextWrap, TextVisual, VisualStyle, LineRun` (§2.2)
- `style_patch.h` — `Opt<T>, opt(), StylePatch, apply(), merge(), patch()` (§3)
- `interaction.h` — `InteractionState` incl. `active` (§5.1)
- `theme.h` — `RoleStyle, Theme, ThemeContext, TextStyleContext, use_theme()` (§4)
- `resolve.{h,cpp}` — `resolve(const RoleStyle&, const StylePatch& variant, const InteractionState&)` (§5.2)
- `text_measure.h` — `TextMetricsQuery/Result, MeasureTextFn, set_text_measurer` (§10.2)
- `default_theme.{h,cpp}` — `default_theme()` (§4.5)

**New (`src/ui/runtime/`):**
- `interaction.{h,cpp}` — `InteractionSnapshot, InteractionContext, use_host_id`, the four hooks (§7)

**Edited:**
- `src/ui/runtime/tree.h` — `Style`→`LayoutStyle`; include `style/visual_style.h`; `Node::visual`; `UI_RETAINED_TEXT_ARENA_BYTES`; text as `(offset,len)` (§2.1, §10.3)
- `src/ui/runtime/element.{h,cpp}` — `HostProps.style`→`HostProps.layout` + add `.visual`; commit `.visual`; text via measurer (§2.4, §11.1)
- `src/ui/runtime/focus.{h,cpp}` — `hovered_id`, `focus_hovered_id`, `focus_pressed_id`, `focus_source_is_visible` (§7.2)
- `src/ui/runtime/draw_list.{h,cpp}` — REWRITTEN to the IR + pure transcriber (§8)
- `src/ui/runtime/yoga_flex_layout.cpp` — text `MeasureFn` shim calls the global measurer (§10.1)
- `src/ui/components/*` — migrate to resolve-and-pass-via-props (§11)
- `src/renderer/sdl_retained_renderer.{h,cpp}` — linear executor over the IR; provide the real `MeasureTextFn` (§9)
- `src/renderer/font_registry.{h,cpp}` — per-`(face,size)` `TTF_Font*` map; back `MeasureTextFn` (§10.6)
- `src/client/ui/client_ui.cpp` — publish `InteractionSnapshot` (§7); migrate client screens/components (§11)
- `src/app/app.cpp` — install the renderer-backed `MeasureTextFn` via `ui::set_text_measurer` (§12.3)
- `tests/runtime_dependency_guard.py` — add SDL/TTF-under-`src/ui/` scan (§12.1)
- `CMakeLists.txt` — new test targets + `src/ui/style/*.cpp` into source lists (§13.1)
- `architecture.md` — NEW canonical boundary doc (§12.6)

**CMake test-target template:** copy the `retained_ui_draw_list_tests` block (`CMakeLists.txt:359-383`): `add_executable` + `target_include_directories` + `target_compile_options(-Wall -Wextra)` + `target_link_libraries` + `add_test`. Hermetic targets link only the minimal source set (no SDL/Yoga where possible). `${UI_COMPONENT_SOURCES}` is defined at `CMakeLists.txt:93`.

---

## P0 — Land the IR + style types, inert (spec §14-P0, §2, §3, §8)

**Goal:** all style/IR types exist and compile with their `static_assert`s; no behavior change; existing tests stay green by mapping old draw fields onto new arms.

### Task P0.1: `visual_style.h` — the dense paint types

**Files:** Create `src/ui/style/visual_style.h`.

- [ ] **Step 1 — Write the type header.** Implement every type in §2.2 verbatim (`Color, Vec2, SideWidths, SideColors, Border, Outline, GradientStop, Gradient` with `UI_MAX_GRADIENT_STOPS=8`, `BackgroundImage, Shadow, TextAlign, TextWrap, TextVisual, VisualStyle`) plus `LineRun` from §10.2. Namespace `ui`. Include guards/`#pragma once`. End with the two `static_assert`s (`is_aggregate_v<VisualStyle>`, `is_trivially_copyable_v<VisualStyle>`).
- [ ] **Step 2 — Move `Color`.** Delete `struct Color` from `tree.h:229` and `#include "../style/visual_style.h"` in `tree.h` so there is one `Color`. (Adjust the include path to match `tree.h`'s location: `../style/visual_style.h`.)
- [ ] **Step 3 — Write a compile/assert test** `tests/ui_style_types_tests.cpp`: a `main()` that `static_assert`s `is_aggregate_v` + `is_trivially_copyable_v` for `VisualStyle`, `Border`, `Outline`, `Gradient`, `BackgroundImage`, `Shadow`, `TextVisual`, and asserts `Color{} == {0,0,0,0}` defaults and `sizeof(VisualStyle)` prints (record baseline). Use the `CHECK` mold from `tests/retained_ui_draw_list_tests.cpp:9`.
- [ ] **Step 4 — Register** `ui_style_types_tests` in `CMakeLists.txt` (template above; sources: just the test .cpp + headers; no SDL/Yoga).
- [ ] **Step 5 — Build+gate.** Run `./build.sh --tests`. Expected: all green (new target passes; nothing else changed because only `Color`'s definition location moved).
- [ ] **Step 6 — Commit:** `git add -A && git commit -m "P0: add VisualStyle paint types (src/ui/style/visual_style.h)"`

### Task P0.2: `style_patch.h` — Opt<T> + StylePatch + apply/merge + builder

**Files:** Create `src/ui/style/style_patch.h`. Test `tests/ui_style_patch_tests.cpp`.

- [ ] **Step 1 — Write the failing test.** Cases: `Opt<Color>{}.set==false`; `opt(Color{0,0,0,0}).set==true`; `apply()` writes only set fields (a `StylePatch` setting only `background` leaves a dense `VisualStyle`'s `corner_radius`/`border` untouched); `apply(opt(Color{0,0,0,0}))` sets the field to transparent (set-flag, not value); `merge(dst,src)` — src's set fields win, unset src fields leave dst; `patch().background(c).corner_radius(r)` produces a `StylePatch` with exactly those two `.set==true`.
- [ ] **Step 2 — Run, verify it fails** (header doesn't exist). Build the new target only.
- [ ] **Step 3 — Implement** `style_patch.h` per §3.1–§3.5 verbatim (`Opt<T>`, `opt()`, `StylePatch`, `apply()`, `merge()`, `StylePatchBuilder`/`patch()`), plus the two `static_assert`s (§3.2).
- [ ] **Step 4 — Register** `ui_style_patch_tests` (hermetic). Run `./build.sh --tests`. Expected: PASS.
- [ ] **Step 5 — Commit:** `git commit -am "P0: add StylePatch + Opt<T> + apply/merge/patch (no sentinels)"`

### Task P0.3: `interaction.h` + `theme.h` + `text_measure.h` types

**Files:** Create `src/ui/style/interaction.h` (§5.1 `InteractionState` incl. `active`), `src/ui/style/theme.h` (§4.1 `RoleStyle`, `Theme`; §4.2 `ThemeContext`, `use_theme()` — but `default_theme()` is P1 so forward-declare it), `src/ui/style/text_measure.h` (§10.2 `TextMetricsQuery/Result`, `MeasureTextFn`, `set_text_measurer`/`text_measurer`). `text_measure.h` needs `set_text_measurer`/`text_measurer` defined in a small `text_measure.cpp` (a single static function pointer).

- [ ] **Step 1 — Write headers** verbatim from §4.1, §4.2, §5.1, §10.2. `theme.h` includes `style_patch.h`+`visual_style.h`; `ReactContext ThemeContext`/`TextStyleContext` declared (definitions in a .cpp). Add `src/ui/style/text_measure.cpp` (the fn-ptr store) and `src/ui/style/theme.cpp` (context object definitions + `use_theme()` if not inline).
- [ ] **Step 2 — Compile-assert test** `tests/ui_style_meta_tests.cpp`: `static_assert(is_trivially_copyable_v<Theme>)`, `is_trivially_copyable_v<InteractionState>`, `is_trivially_copyable_v<TextMetricsResult>`; `set_text_measurer(nullptr); CHECK(text_measurer()==nullptr)`.
- [ ] **Step 3 — Register** `ui_style_meta_tests`. Build+gate green.
- [ ] **Step 4 — Commit:** `git commit -am "P0: add InteractionState, Theme/RoleStyle, MeasureTextFn seam types"`

### Task P0.4: `draw_list.h` — the tagged-union IR (replaces 2-arm enum)

**Files:** Rewrite `src/ui/runtime/draw_list.h`. Test `tests/ui_draw_ir_tests.cpp`.

- [ ] **Step 1 — Write the IR** per §8.1 verbatim: `DrawRect`, `DrawCommandKind` (full enum incl. reserved `Custom`), the POD arms (`RectData…LayerData`, no `CustomData`), `union DrawPayload` (first arm `RectData rect{}` with DMI), `struct DrawCommand`. Then §8.2 `static_assert`s (set `sizeof(DrawCommand) <= 96`, `sizeof(DrawPayload) <= 56` — tighten to measured at first build per §8.2 note). Then `DrawList` per §8.7 (arenas, cursors, `push`/`push_text`/`push_stops`/`reset`) and the budget `static_assert` (§8.7). Include capacity constants §8.3/§8.7 (`UI_DRAW_TEXT_ARENA_BYTES`, `UI_DRAW_GRAD_STOPS`, `UI_TEXT_LINE_CAP`, `UI_MAX_*`).
- [ ] **Step 2 — Implement `DrawList` methods** in `draw_list.cpp` (§8.6 `reset`, §8.8 `push`/`push_text`/`push_stops` with bounds-check→`error_count`). Keep the OLD `build_draw_list`/`append_*` temporarily but adapt their `DrawCommand{...}` initializers to the new arms (map old `fill`→`payload.rect.fill`, old `border`/`border_width`→a `Border` arm, old text→`TextData` via a temporary inline byte copy) so existing emitters compile. This is the mechanical, behavior-preserving bridge (§14-P0 "existing emitters keep compiling").
- [ ] **Step 3 — Write IR tests** `tests/ui_draw_ir_tests.cpp`: `static_assert` aggregate+trivially_copyable for `DrawCommand`; `DrawList::push` past `UI_MAX_DRAW_COMMANDS` returns false + bumps `error_count`; `push_text` past arena returns false + bumps `error_count` (no clamp); `reset()` zeroes cursors only. Print `sizeof(DrawCommand)`/`sizeof(DrawList)`; then tighten the `static_assert` bounds in `draw_list.h` to the measured values.
- [ ] **Step 4 — Register** `ui_draw_ir_tests`; ensure `retained_ui_draw_list_tests` still builds (adapt its expectations if it inspects `DrawCommand` fields — see Step 2 bridge). Build+gate.
- [ ] **Step 5 — Commit:** `git commit -am "P0: tagged-union DrawCommand IR + arenas + bounds-checked push (inert bridge)"`

**P0 GATE:** `./build.sh --tests` fully green; `static_assert` set passes; `Opt<Color>{}.set==false`. No visual change.

---

## P1 — resolve() + Theme + ThemeContext + interaction hooks, unit-tested in isolation (spec §14-P1, §4, §5, §7)

**Goal:** the cascade and interaction plumbing exist and are unit-tested with no `UiTree`/renderer wiring yet.

### Task P1.1: `resolve()`

**Files:** Create `src/ui/style/resolve.{h,cpp}`. Test `tests/ui_style_resolve_tests.cpp`.

- [ ] **Step 1 — Write failing tests** per §13.2: precedence low→high; `active` lands between `checked` and `disabled`; variant before state; set-flag survival (`opt(Color{0,0,0,0})` applies; `Opt<Color>{false,{255,255,255,255}}` does not); disabled wins last; `resolve(role,{},{})==role.base` (memcmp); `focused && !focus_visible` does not apply `focus_visible`.
- [ ] **Step 2 — Run, verify fail.**
- [ ] **Step 3 — Implement** `resolve()` exactly as §5.2 (single authoritative body).
- [ ] **Step 4 — Register** `ui_style_resolve_tests` (hermetic: links `resolve.cpp` only). Build+gate PASS.
- [ ] **Step 5 — Commit:** `git commit -am "P1: resolve() cascade with locked precedence + tests"`

### Task P1.2: `default_theme()`

**Files:** Create `src/ui/style/default_theme.{h,cpp}`. Test in `ui_style_resolve_tests` (extend).

- [ ] **Step 1 — Implement** `default_theme()` per §4.5 verbatim (seed_control, focus_visible outline patch, checkbox/checkbox_mark, box/text/dialog, global tokens). Values are the new look (no parity).
- [ ] **Step 2 — Test:** a focusable role's `focus_visible` patch sets a non-zero `Outline` (so the focus ring exists); `resolve(default_theme().button, {}, {.disabled=true})` yields the dimmed fill; `resolve(default_theme().checkbox_mark, {}, {.checked=true}).background.a != 0` and `{.checked=false}` ⇒ `a==0`.
- [ ] **Step 3 — Build+gate. Commit:** `git commit -am "P1: default_theme() with focus-ring wiring (new look)"`

### Task P1.3: focus runtime additions

**Files:** Edit `src/ui/runtime/focus.{h,cpp}`. Test extend `tests/retained_ui_focus_tests.cpp`.

- [ ] **Step 1 — Write failing test:** after a `focus_update` with a pointer over node X, `focus_hovered_id(rt)==X`; `focus_pressed_id(rt)` returns `pointer_press_origin`; `focus_source_is_visible(Keyboard)==true`, `(Mouse)==false`.
- [ ] **Step 2 — Implement** per §7.2: add `NodeId hovered_id` to `FocusRuntime`; populate it in `focus_update` from the existing `hovered_enabled` computation; add getters `focus_hovered_id`/`focus_pressed_id` and `focus_source_is_visible`. No new stored `pressed_id`.
- [ ] **Step 3 — Build+gate. Commit:** `git commit -am "P1: expose hovered/pressed + focus_source_is_visible on FocusRuntime"`

### Task P1.4: interaction snapshot + hooks

**Files:** Create `src/ui/runtime/interaction.{h,cpp}`. Test `tests/ui_interaction_hooks_tests.cpp`.

- [ ] **Step 1 — Implement** per §7.2/§7.3/§7.4: `InteractionSnapshot`, `InteractionContext` (ReactContext), `use_host_id()` (deterministic id = the same derivation `begin_node` uses — call the shared helper `make_child_id`/`react_make_instance_fiber_id` path; reuse `tree.h`'s `make_child_id` logic or expose it), `use_hovered/use_pressed/use_focused/use_focus_visible`.
- [ ] **Step 2 — Write tests** (hermetic, real hook runtime, no SDL): provide an `InteractionSnapshot` with `hovered==H` at the root context; a component whose `use_host_id()==H` reads `use_hovered()==true`, a sibling reads false; `use_focus_visible()` true only when focused AND source∈{Keyboard,Gamepad,Programmatic}.
- [ ] **Step 3 — Register** `ui_interaction_hooks_tests` (links `interaction.cpp`, `react.cpp`, `tree.cpp`, `element.cpp`). Build+gate PASS.
- [ ] **Step 4 — Commit:** `git commit -am "P1: InteractionSnapshot + every-frame interaction hooks"`

**P1 GATE:** `ui_style_resolve_tests`, `ui_interaction_hooks_tests`, focus tests green; precedence + `active` slot + disabled-wins + set-flag-survival all pinned. No visual change, builder not yet wired.

---

## P2 — Split HostProps; components resolve-and-pass; builder becomes a transcriber (spec §14-P2, §2.4, §6, §8.5, §11)

**Goal:** `HostProps` carries `.layout`+`.visual`; components resolve and pass `.visual`; `build_draw_list` is a pure transcriber; focus-ring injection and `inherited_disabled` deleted.

### Task P2.1: split `Style`→`LayoutStyle`; add `Node::visual`, `HostProps.visual`

**Files:** Edit `tree.h` (§2.1 `LayoutStyle`, drop paint tail; `Node::visual`; `NodeSnapshot::visual`), `element.h` (`HostProps.layout`+`.visual`, §2.4), `element.cpp` (commit `.visual`), `yoga_flex_layout.cpp` (read `LayoutStyle`), all call sites passing `Style`.

- [ ] **Step 1 — Mechanical rename+split.** `Style`→`LayoutStyle` (drop `background/border/text/border_width/font_size`). Add `VisualStyle visual` to `Node`, `NodeSnapshot`, `HostProps`. Update `begin_node`/`set_metadata`/reconciler commit to copy `.visual`. Update `tree.h` factories (`text(...)` overload §11.1). Fix every compile error from the rename across `src/`.
- [ ] **Step 2 — Build (no test yet)** to flush compile errors: `./build.sh`. Iterate until it compiles. (Components still set old colors via the bridge — they'll move in P2.2.)
- [ ] **Step 3 — Commit:** `git commit -am "P2: split Style into LayoutStyle + VisualStyle; Node/HostProps carry both"`

### Task P2.2: migrate `src/ui/components/*` to resolve-and-pass

**Files:** Edit `src/ui/components/common.h` (delete `control_style` + `kControl*`), each generic component (`Button`, `Input`, `Checkbox`, `Text`, `Box`). Test `tests/ui_components_tests.cpp` (extend).

- [ ] **Step 1 — Write failing component tests:** a `Button` under a root `ThemeProvider(default_theme())` commits a `Node.visual` whose `background == theme.button.base.background`; a focused button (snapshot focus_visible=true via the InteractionContext) resolves a non-zero `outline`; a disabled button resolves the dimmed fill; an unstyled `Box` commits a `VisualStyle` that paints nothing (`background.a==0`).
- [ ] **Step 2 — Implement** each component per §11.5–§11.7 (`use_theme()`, build `InteractionState` from hooks + props, `resolve()`, pass `.visual`; `Text` reads `TextStyleContext` per §6.2; `with_text_default` helper §6.3). Delete `control_style`/`kControl*` from `common.h`.
- [ ] **Step 3 — Build+gate.** Component tests PASS. Commit: `git commit -am "P2: components resolve VisualStyle via theme/hooks and pass as .visual"`

### Task P2.3: `build_draw_list` becomes a pure transcriber

**Files:** Rewrite `src/ui/runtime/draw_list.cpp` (delete the P0 bridge + `append_*` + `k*Fill`/`kFocusBorder`/`inherited_disabled`/`has_color`/`kCharWidth`). Test `tests/ui_draw_list_emit_tests.cpp`.

- [ ] **Step 1 — Write failing emission tests** (subset of §13.4 that doesn't need the measurer yet — text lines come in P4; here use single-line via a trivial inline measure or defer text asserts): a bordered box emits one fused `Border` frame command + a `Rect` fill; an unstyled box emits nothing; `hidden==true` ⇒ no paint; bracket integrity for nested `Overflow::Hidden` (balanced `ClipPush`/`ClipPop`); premultiplied-at-emit (`premul` applied). Use the `assert_balanced_brackets` helper (§13.4).
- [ ] **Step 2 — Implement** the transcriber per §8.5 (`transcribe()` DFS, hierarchical-z, emit order Shadow→fill/gradient/image→children→fused frame; clip/layer brackets; `premul` at emit §8.4). `build_draw_list` loses the `focused_id` parameter. Resolve nothing.
- [ ] **Step 3 — Delete** the focus-ring injection, role switch, constants, `inherited_disabled` thread. Update all `build_draw_list` callers (drop `focused_id` arg) — `client_ui.cpp`, pipeline, tests.
- [ ] **Step 4 — Build+gate.** `ui_draw_list_emit_tests` PASS; existing tests green (rebaseline `retained_ui_draw_list_tests` expectations to the new IR/transcriber). Commit: `git commit -am "P2: build_draw_list is a pure transcriber; delete focus-ring injection + role switch"`

### Task P2.4: publish `InteractionSnapshot` from the client

**Files:** Edit `src/client/ui/client_ui.cpp` (publish snapshot from focus getters into `InteractionContext` at the root each frame, from the previous frame's focus results, §7.2).

- [ ] **Step 1 — Implement** the per-frame publish: build `InteractionSnapshot{focused=focus_focused_id, hovered=focus_hovered_id, pressed=focus_pressed_id, source=focus_source}` and provide it at the tree root. Migrate client screens/components (`src/client/ui/...`) to the new authoring shape where they set colors/focus.
- [ ] **Step 2 — Build+gate.** CLI smoke green. Commit: `git commit -am "P2: publish InteractionSnapshot; migrate client UI to resolved .visual"`

**P2 GATE:** all hermetic + component + client tests green; CLI smoke green (this output is the new baseline). A focused Button resolves a non-zero outline; transcriber emits one fused frame per bordered box. The renderer still reads via the bridge until P3 — verify `hello` still launches (P3 rewrites rendering).

---

## P3 — Renderer rewrite behind NEW goldens (spec §14-P3, §9)

**Goal:** SDL renderer is a dumb linear executor over the IR; premultiplied; uniform rounded rect; co-feathered frame; clip stack; signed-offset focus ring. First real visual goldens.

### Task P3.1: golden harness

**Files:** Create `tests/golden_util.h` (BMP read/`compare_bmp(actual,golden,tol,&report)` with ±2 tolerance, `UI_GOLDEN_REGEN` env regen, `*_actual.bmp`/`*_diff.bmp` dump), `tests/golden/` dir. Test `tests/renderer_golden_tests.cpp`.

- [ ] **Step 1 — Implement** `golden_util.h` per §13.5 (software/dummy SDL drivers like `tests/ui_cli_smoke.py`). Register `renderer_golden_tests` (SDL target). Build (no cases yet).
- [ ] **Step 2 — Commit:** `git commit -am "P3: golden BMP test harness (fresh, no parity)"`

### Task P3.2: executor + rect + frame + clip + focus ring

**Files:** Rewrite `src/renderer/sdl_retained_renderer.{h,cpp}` per §9.1–§9.5, §9.9, §9.11, §9.12.

- [ ] **Step 1 — Write failing goldens** (regen first run): rounded-rect topology (§13.5), co-feathered fill+border no-seam, signed-offset focus ring inset+outset+default. Mark expected behavior in asserts.
- [ ] **Step 2 — Implement** the executor: `render()` linear loop; premultiplied init (§9.2); `exec_rect` uniform rounded fill (§9.3); `exec_border` fused frame co-feathered single pass (§9.4); `clip_push/pop` integer round-out stack (§9.9); signed-offset outline (§9.11); `texture_id`→`SDL_Texture*` map stub. Switch `DrawList::reset` callers to cursor-only.
- [ ] **Step 3 — Regen+gate.** `UI_GOLDEN_REGEN=1 ctest -R renderer_golden_tests` writes goldens; re-run without regen → PASS. Visually sanity-check a dumped BMP if possible. `./build.sh --tests` green; balanced clip asserted.
- [ ] **Step 4 — Commit:** `git commit -am "P3: SDL linear executor — rounded rect, co-feathered frame, clip, focus ring + goldens"`

**P3 GATE:** `renderer_golden_tests` green vs fresh BMPs; CLI smoke green; `hello` launches and renders the new look.

---

## P4 — Text measure seam + per-line emission + TTF_Text cache + lifted storage (spec §14-P4, §10)

**Goal:** one `MeasureTextFn` for layout+draw (measure==draw); per-line emission; caret from advances; text in shared arena (cap lifted); retained `TTF_Text` cache.

### Task P4.1: lifted text storage in the runtime

**Files:** Edit `tree.h` (`UI_RETAINED_TEXT_ARENA_BYTES`, text as `(offset,len)`, §10.3), `tree.cpp` (arena bump + `report_error` on overflow, delete `strncpy` clamp for renderable text), `element.cpp`.

- [ ] **Step 1 — Write failing test:** a node with >95-byte text round-trips its full bytes via the arena; text exceeding `UI_RETAINED_TEXT_ARENA_BYTES` sets `error_count()` (no clamp).
- [ ] **Step 2 — Implement** the shared string arena + `(offset,len)` storage. Keep `accessibility_label`/control `value`/`composition` as fixed caps (§10.3).
- [ ] **Step 3 — Build+gate. Commit:** `git commit -am "P4: lift node text into shared arena (offset,len); overflow=failed frame"`

### Task P4.2: the `MeasureTextFn` seam + Yoga shim + mock

**Files:** `text_measure.{h}` already from P0; create the mock in `tests/`; edit `yoga_flex_layout.cpp` text `MeasureFn` to call the global measurer (§10.1).

- [ ] **Step 1 — Write `ui_draw_list_emit_tests` measure cases** per §13.4 with a deterministic mock (`set_text_measurer(mock)`): measure==draw caret (`x == origin + kAdvance*col`, never `8*col`); 3-line wrap emits 3 `Text` commands ascending `y`; alignment pre-baked; line cap > 8 ⇒ `error_count`; selection from advances.
- [ ] **Step 2 — Implement** per-line emission in `build_draw_list` (§10.4): call `text_measurer()(query)`, emit one `Text` per `LineRun`, copy slice into the draw text arena, caret/selection as `Rect`s from advances (§10.5). Wire the Yoga text `MeasureFn` shim to the global measurer.
- [ ] **Step 3 — Build+gate.** Emission + caret tests PASS. Commit: `git commit -am "P4: per-line text emission + caret/selection from measured advances (kills strlen*8)"`

### Task P4.3: renderer-side real measurer + TTF_Text cache + per-(face,size) fonts

**Files:** Edit `font_registry.{h,cpp}` (per-`(face,size)` `TTF_Font*` map; back `MeasureTextFn` returning `LineRun`s, §10.6), `sdl_retained_renderer.cpp` (`exec_text` via retained `TTF_Text` cache, §10.6), `app.cpp` (install the real measurer via `ui::set_text_measurer`).

- [ ] **Step 1 — Write `renderer_text_cache_tests`** per §13.6: steady-state 1 create/0 SetTextString; reflow leaks zero; font-size change rebuilds once; measured==drawn (±1px).
- [ ] **Step 2 — Implement** the per-`(face,size)` font map (replace single `default_font_`+`TTF_SetFontSize`), the retained `TTF_Text` cache keyed `(node_id,line_index,face,size,style)` with content-fingerprint change-gate + mark-and-sweep eviction, premultiplied glyph upload (§9.2), and `exec_text` drawing under the active clip (§10.7). Install the real `MeasureTextFn` in `app.cpp`.
- [ ] **Step 3 — Write clipped-text golden** (§13.5: zero glyph pixels outside clip). Regen+gate.
- [ ] **Step 4 — Build+gate.** All green. Commit: `git commit -am "P4: TTF_Text cache + per-(face,size) fonts + real MeasureTextFn (measure==draw)"`

**P4 GATE:** caret == measured advance; multi-line count == measured `LineRun` count; over-budget text fails the frame; text cache steady-state/reflow/resize correct; clipped-text golden green.

---

## P5 — Images, gradients, nine-slice, drop shadow (spec §14-P5, §9.6–§9.8)

**Goal:** the remaining fill/effect arms.

### Task P5.1: gradient

**Files:** `sdl_retained_renderer.cpp` `exec_gradient` (§9.6); transcriber emits `Gradient` from `VisualStyle.gradient` (stops→`grad_arena`).
- [ ] Test (golden): 2-stop + 8-stop, angle 0/90, monotonicity probe (§13.5). Implement §9.6. `stop_count==0`⇒none. Build+gate. Commit `git commit -am "P5: linear gradient (per-vertex)"`

### Task P5.2: image + tint + nine-slice

**Files:** `sdl_retained_renderer.cpp` `exec_image` (§9.7) + `texture_id`→`SDL_Texture*` cache with premultiplied upload; transcriber emits `Image`.
- [ ] Test (golden): tinted nine-slice panel (corners unscaled, center stretched); plain tinted rect (colormod restored to white after). Implement §9.7 (plain/nine-slice/rounded paths; nine-slice+rounded cut). Build+gate. Commit `git commit -am "P5: image + tint + nine-slice"`

### Task P5.3: drop shadow + re-derive capacity

**Files:** `sdl_retained_renderer.cpp` `exec_shadow` (§9.8); transcriber emits `Shadow` before fill; re-derive `UI_MAX_DRAW_COMMANDS` (§8.7) now a node can emit shadow+fill+frame.
- [ ] Test (golden): drop shadow feathered, on offset side, fading out; arena-overflow tests assert failed frames. Implement §9.8 (`color.a==0`⇒none; no gaussian cache). Build+gate. Commit `git commit -am "P5: drop shadow (feathered quads) + re-derived capacity"`

**P5 GATE:** gradient/nine-slice/shadow goldens green; arena-overflow tests fail the frame.

---

## P6 — Group opacity (spec §14-P6, §9.10)

**Goal:** `opacity<1` group composite via a single naive transient render-to-target.

### Task P6.1: layer push/pop

**Files:** `sdl_retained_renderer.cpp` `layer_push`/`layer_pop` (§9.10); transcriber emits `LayerPush`/`LayerPop` for `visual.opacity<1` (already in §8.5 `transcribe`).
- [ ] **Step 1 — Test:** transcriber test asserts balanced `LayerPush`/`LayerPop` for an `opacity<1` subtree (extend §13.4 bracket test). Golden: a half-opaque subtree with overlapping translucent children composites uniformly (no double-darkening).
- [ ] **Step 2 — Implement** §9.10 exactly: transient target sized to the `LayerPush` header rect, created/destroyed within the bracket, premultiplied composite at `opacity`, full SDL state save/restore. NO pool/hysteresis/budget/fallback.
- [ ] **Step 3 — Regen+gate. Commit:** `git commit -am "P6: group opacity via single naive render-to-target composite"`

**P6 GATE:** group-opacity golden green; balanced `LayerPush`/`LayerPop` asserted.

---

## Finalization

### Task F.1: boundary guard + architecture.md

**Files:** `tests/runtime_dependency_guard.py` (add `sdl`/`SDL`/`TTF_`-under-`src/ui/` to BLOCKED, §12.1), create `architecture.md` (§12.6 outline), `tests/ui_style_invariants_guard.py` (§13.7: no value-space sentinels in authoring; whitelisted resolved-output checks).
- [ ] Implement both guards + `architecture.md`. Register the python guards. Build+gate (guards pass — confirms no SDL leaked into `ui/` and no authoring sentinels). Commit `git commit -am "Finalize: boundary + no-sentinel guards; create architecture.md"`

### Task F.2: full verification + completion report

- [ ] **Step 1 — Full clean build+test:** `./build.sh --tests` from a clean `cmake-build-debug` (delete it first). Capture the full ctest summary. Every target must pass.
- [ ] **Step 2 — Run the app:** launch `hello` headless via `tools/ui_cli.py` (as the CLI smoke does) and capture a BMP; confirm it renders.
- [ ] **Step 3 — Self-review vs spec §1–§15:** confirm each section has landed; note anything deferred.
- [ ] **Step 4 — Write `docs/retained-ui/IMPLEMENTATION-REPORT.md`:** what shipped per phase, every autonomous decision made + rationale, test results (paste ctest summary), anything deferred/known-incomplete, and the exact commands to verify. Update the memory file. Commit `git commit -am "Finalize: implementation report + verification"`.
- [ ] **Step 5 — Leave for review.** Do NOT push. Summarize on the branch for the user.

---

## Self-review (writing-plans checklist)

- **Spec coverage:** P0(§2,3,8) · P1(§4,5,7) · P2(§2.4,6,8.5,11) · P3(§9.1-5,9,11) · P4(§10) · P5(§9.6-8) · P6(§9.10) · F(§12,13). §15 (cut list) is the "do not implement" guard, enforced by F.1. All spec sections map to a phase.
- **Placeholder scan:** code bodies are referenced to spec sections (DRY, single source of truth) — not "TODO"; every task has exact files, the specific test assertions, the run command, and the commit. Acceptable per DRY since the spec is the committed companion.
- **Type consistency:** all type/function names (`VisualStyle`, `StylePatch`, `resolve`, `apply`/`merge`, `DrawCommand`, `InteractionSnapshot`, `use_host_id`, `MeasureTextFn`, `default_theme`) are used identically here and in the spec's canonical signatures.
