# Spec: client-owned theme + uniform sparse-`StylePatch` override on primitives

- **Date:** 2026-05-30
- **Branch:** `feat/styling-render-system`
- **Status:** Approved (autonomous implementation mandate — user delegated full ownership, decide-and-document).
- **Supersedes nothing.** Follow-on to the completed styling/render rewrite (`docs/retained-ui/styling-render-system-design.md`).

## Problem

Two architectural defects were raised and verified against the source:

**A. Theme ownership.** The whole theme machinery lives in the generic toolkit `src/ui/style` (`Theme`/`RoleStyle` types, `ThemeContext`, `use_theme()`, `resolve()`, plus the authored slate palette in `default_theme.cpp`). Two problems:
- The `ThemeContext` provider was specced but **never built** — repo-wide, `ThemeContext` is referenced only at its definition (`theme.h:56`) and its read (`theme.h:62`); nothing pushes it, so `use_theme()` *always* falls through to `default_theme()`.
- The authored slate **values** (palette, accent, radii) are a product design decision sitting in the generic toolkit. The mechanism is legitimately generic; the *values* are not.

**B. Primitives don't accept per-instance styles.** `Button`/`Input`/`Checkbox` derive their entire paint from `use_theme()` and pass `{}` to `resolve()`'s per-instance `variant` slot (`button.cppx:19`). `Box`/`Text`/`Dialog` accept a *dense* `VisualStyle visual` override and bypass the theme entirely. So there are two override flavors and the controls have none. The resolver already has the seam (`resolve(role, variant, state)`); it's just fed `{}`.

**C. Consequence:** `AppButton`'s shadcn-style variant table is inert — `app_button_variant_patch()` returns `{}` and `app_button.cppx:37` discards it with `(void)`. Primary/Secondary/Danger/Ghost render byte-identical.

## Decisions (locked)

1. **Full coherent fix** — all of A + B + C, sequenced.
2. **Converge every primitive onto one sparse `StylePatch` override prop** (`style_override`). `Box`/`Text`/`Dialog` drop the dense `VisualStyle visual` prop.
3. **Two layers:** `ui::Theme` = primitive-level role defaults (client-owned **values**, installed via provider); `shooter::tokens` = the app palette, whose builders return `StylePatch`. (Not unified — the rich variant palette doesn't fit `Theme`'s flat one-role-per-widget shape.)
4. **`ui::default_theme()` becomes a neutral, unopinionated minimal fallback**, used only when no provider is installed (standalone `retained_ui_*` tests). The product slate moves to the client.
5. **`Box`/`Text`/`Dialog` resolve over their theme role** (`theme.box`/`theme.text`/`theme.dialog`), under the caller's sparse patch. Un-patched primitives get theme defaults.
6. **Delete the dead `TextStyleContext`/`TextStyleValue`** (`theme.h:47-57`, zero providers, zero readers).
7. **`AppButton` variants:** Primary = accent fill, Secondary = theme slate (empty patch), Danger = red fill, Ghost = transparent + no border. Exact colors are iterable (no screen uses non-default variants yet).

## Architecture: the two-layer model

```
client owns VALUES + installs ──┐
                                 ▼
  app_theme()  ──provider(&ui::ThemeContext)──►  use_theme()  ──►  resolve(role, style_override, state)
  (slate palette, client)                         (ui mechanism)        ▲
                                                                        │ style_override : StylePatch
  shooter::tokens (app palette) ── *_patch() builders ── semantic comps ┘
```

- **Stays in `src/ui/style`** (generic, vocabulary-free): `Theme`/`RoleStyle`, `ThemeContext`, `use_theme()`, `resolve()`, `StylePatch`/`VisualStyle`, `apply`/`merge`, `patch()` builder. `default_theme()` becomes the **neutral** fallback.
- **New `src/client/ui/app_theme.{h,cpp}`** (plain C++): `const ::ui::Theme& app_theme()` holding the relocated slate palette (verbatim move of current `default_theme.cpp` body). Installed once at the shell's cross-cutting-context seam via the existing generic provider: `::ui::provider("ThemeProvider", &::ui::ThemeContext, ::ui::copy_value(app_theme()), children)`. `copy_value` gives runtime-managed lifetime (no `static`/dangling concern). Install altitude = the `FrameProvider`/`wrap_root` seam (`ui_pipeline.h` / `client_ui.cpp:41-85`), same altitude as `ScreenProvider`/`InteractionProvider`.
- **Every primitive** resolves `resolve(use_theme().<role>, props.style_override, interaction_state(...))`. Box/Text/Dialog pass a default (all-false) `InteractionState` (no hover/focus on a container) → base+patch only.
- **`shooter::tokens`** builders (`fill_visual`/`panel_visual`/`text_visual`) change return type `VisualStyle` → `StylePatch`, renamed `fill_patch`/`panel_patch`/`text_patch`. Semantic components map variant → patch and pass it as `style_override`.
- **`AppButton`** fills `app_button_variant_patch(variant)` and assigns the result to `ButtonProps.style_override` (replacing the `(void)` discard).

## Migration surface (exact call sites)

**ui primitives — props + body:**
- `src/ui/components/box.{hx,cppx}` — `visual` prop → `style_override` (`StylePatch`); body resolves `theme.box`.
- `src/ui/components/text.{hx,cppx}` — same, `theme.text`.
- `src/ui/components/dialog.{hx,cppx}` — same, `theme.dialog`.
- `src/ui/components/button.cppx:19` — thread `props.style_override` into the `{}` variant slot (+ add prop in `button.hx`).
- `src/ui/components/input.cppx:216` & `checkbox.cppx:40-41` — same (+ props in `.hx`).

**tokens.h:** `src/client/ui/components/tokens.h:62,70,80` — three builders return `StylePatch`.

**Client `.cppx` (JSX — attribute order = struct field order):**
- `screens/loadout/components/loadout_title.cppx:12` (Text)
- `screens/loadout/components/loadout_screen_frame.cppx:20` (Dialog)
- `components/layout/screen_layout.cppx:31,47,76`
- `components/surfaces/panel.cppx:41-84` (builds a `VisualStyle` then passes — rebuild as `StylePatch`)
- `components/text/body_text.cppx:53`, `screen_subtitle.cppx:13`, `screen_title.cppx:38`

**Client plain `.cpp` (`.visual =` designated initializers — order = struct field order):**
- `screens/loadout/components/weapon_tile.cpp:87,99`
- `screens/loadout/components/confirm_dialog.cpp:53,68,83,98`
- `screens/loadout/components/equipment_slot.cpp:64`
- `components/hud_band.cpp:29,48`

## cppx authoring rules (must follow — see memory `cppx-authoring-gotchas`)

1. JSX mode needs a leading `<` / `return <`; fragment/branching bodies are plain `.cpp`.
2. **Attribute/initializer order must match the target Props struct's field declaration order** (`-Werror=reorder-init-list`). Replacing `visual` with `style_override` *in the same field position* keeps existing call sites' order valid. When *adding* `style_override` to control props, place it deterministically and update `AppButton`'s initializer accordingly.
3. Any `.cppx` emitting children needs `using ::ui::children;`.
4. Plain `.cpp` includes generated headers via the rooted form (`"client/ui/.../foo.h"`).

## Phases (each: implement → build+test gate → commit)

**Gate** = `./build.sh --tests` shows the failing set is exactly `{renderer_golden_tests}` at its baseline 582-px signature (pre-existing, out of scope), everything else green.

- **P0 — baseline:** confirmed 25/26 (only `renderer_golden_tests` pre-existing). ✔
- **P1 — theme ownership:** neutralize `ui::default_theme()`; create client `app_theme.{h,cpp}` with the slate; install `ThemeProvider` at the shell seam; delete `TextStyleContext`/`TextStyleValue`; wire CMake. Update standalone `ui` tests that asserted the slate look. Build+test. Capture+view a frame (product look preserved via provider). Commit.
- **P2 — uniform override:** replace Box/Text/Dialog `visual` prop with `style_override` (`StylePatch`); each primitive resolves `theme.<role>` + override; thread `style_override` through Button/Input/Checkbox; convert `tokens.h` builders to `StylePatch`; migrate all client call sites above. Build+test. Capture+view. Commit.
- **P3 — variant tables:** fill `app_button_variant_patch`; assign into `ButtonProps.style_override`; add unit test proving each variant resolves to a *distinct* `VisualStyle`. Build+test. Commit.
- **P4 — finalize:** full build+test; completion report; update memory.

## Testing (TDD)

- `tests/ui_style_resolve_tests.cpp` / `ui_style_tests.cpp`: assert patch-over-theme-role resolution and the neutral fallback shape.
- Provider read: a test proving `use_theme()` under an installed provider returns the installed theme, not the fallback (extend `ui_pipeline_tests` or `client_ui_tests`).
- `shooter_ui_tests`: assert each `AppButton` variant resolves to a distinct `VisualStyle`.
- Goldens: `renderer_golden_tests` is synthetic/theme-independent — must stay at its baseline signature. Smoke tests assert frame capture only (existence) — verify the live look by capturing + viewing a frame (capture cmd in memory `styling-render-live-swap-state`).

## Risks

- **cppx order breaks** — mitigated by the build gate (`-Werror=reorder-init-list`).
- **Neutral fallback shifts standalone `ui`-test expectations** — update those tests as part of P1.
- **Provider push/pop balance** shares the 16-deep context stack with game contexts — mitigated by reusing the proven generic `provider()` element + `copy_value`.
- **Pre-existing `renderer_golden_tests` failure** — confirmed at baseline; not attributable to this work; left as-is and documented.
