# Completion Report — Client-owned theme + sparse-`StylePatch` override convergence

**Date:** 2026-05-30 · **Branch:** `feat/styling-render-system` · **Status:** complete, green, committed (not pushed).

Spec: `docs/superpowers/specs/2026-05-30-theme-ownership-override-convergence-design.md`
Plan: `docs/superpowers/plans/2026-05-30-theme-ownership-override-convergence.md`

## What this fixed

Two architectural defects raised by the user, plus the inert variant table they implied:

- **A — Theme ownership.** The whole theme machinery (mechanism *and* the authored slate palette) lived in the generic toolkit `src/ui/style`, and the `ThemeContext` provider was specced but never installed (`use_theme()` always hit the `default_theme()` fallback).
- **B — Primitives didn't accept per-instance styles.** `Button`/`Input`/`Checkbox` fed `resolve()`'s per-instance slot a literal `{}`; `Box`/`Text`/`Dialog` took a *dense* `VisualStyle` override and bypassed the theme. Two override flavors, none for controls.
- **C — Inert variants.** `AppButton`'s cva table returned `{}` and was `(void)`-discarded; Primary/Secondary/Danger/Ghost were byte-identical.

## The model now (two layers)

- `src/ui/style` keeps the **mechanism** (`Theme`/`RoleStyle` types, `ThemeContext`, `use_theme()`, `resolve()`, `StylePatch`, `apply`/`merge`, `patch()`) and a **neutral, flat** `default_theme()` fallback.
- The **product values** live in `src/client/ui/app_theme.cpp` (`client::ui::app_theme()` — the slate palette, moved verbatim) and are installed via `client::ui::ThemeProvider`, which pushes `ThemeContext` at the app-shell frame-provider seam (`app.cpp`), the same altitude as `ShooterProvider`/`AppShellProvider`. `shooter_ui_tests`' `run_pipeline_frame` installs it identically, so tests stay faithful to production.
- Every primitive resolves `resolve(use_theme().<role>, props.style_override, interaction_state(...))`. The override prop is a uniform sparse `::ui::StylePatch style_override` on all six primitives; `Box`/`Text`/`Dialog` dropped their dense `VisualStyle visual` prop.
- `shooter::tokens` builders return `StylePatch` (`fill_patch`/`panel_patch`/`text_patch`); semantic components map variant → patch.
- `AppButton`'s `app_button_variant_patch()` is filled (Primary = accent fill, Secondary = `{}` = theme slate, Danger = red, Ghost = transparent) and assigned to `ButtonProps.style_override`.

## Decisions (locked, autonomous)

1. Full coherent fix (A+B+C). 2. Converge to one sparse `StylePatch` override everywhere. 3. Two layers (theme = primitive defaults; `tokens` = app palette). 4. `default_theme()` = neutral flat fallback. 5. Box/Text/Dialog resolve over their theme role. 6. Deleted dead `TextStyleContext`/`TextStyleValue`. 7. Variant looks as above.
8. **Default `AppButtonProps.variant` = `Secondary`** (not `Primary`) — so existing screens keep the slate baseline; accent is opt-in. (Review caught that defaulting to Primary would silently turn every screen button accent-blue.)
9. **`ThemeProvider` hands the context a pointer to the function-static `app_theme()` directly** (no `copy_value`) — stable lifetime, avoids a per-frame copy of the Theme. Documented divergence from the spec's `copy_value` suggestion.

## Verification

- `./build.sh --tests`: **25/26**. The single failure is the **pre-existing** `renderer_golden_tests` (scene) — a synthetic, theme-independent IR golden (582 px > tol, max delta 56, first at 75,4), unchanged from the P0 baseline and **out of scope** for this work. Everything else green.
- New/strengthened tests: `test_theme_ownership` (values moved: slate has a gradient, neutral fallback is flat); `test_theme_provider_delivers_slate` (a component reading `use_theme()` *under* the installed `ThemeProvider` sees `app_theme()`, not the fallback — proves the provider path end-to-end); `test_app_button_variants_distinct` (variants differ both as patches and after `ui::resolve` over the slate base; Secondary collapses to the base). `ui_style_tests` / `retained_ui_draw_command_tests` / `ui_pipeline_tests` updated to the neutral fallback (they exercise `src/ui` with no provider).
- Visual: captured + viewed the **main menu** (buttons render slate, accent focus ring, hero panel + text intact) and drove to the **pause screen** (the `CenteredOverlay` is transparent — the scene shows through; only the centered panel paints). Both confirm the provider delivers the slate and the two regressions below are fixed.

## Adversarial review (parallel, 3 lenses) — findings addressed

- **MAJOR (fixed):** `CenteredOverlay` `<Dialog>` was un-patched and resolved `theme.dialog`'s opaque slate panel (the pause overlay regression). Now passes an explicit transparent `style_override` (clears background + border), preserving the prior empty-visual look. Verified by capture.
- **MAJOR (fixed):** default variant flipped every button accent-blue → defaulted to `Secondary`. Verified by capture.
- **Nits (addressed):** renamed `panel.cppx` local `override` → `overlay` (contextual-keyword shadow); documented the Ghost interaction-state behavior and the static-pointer provider choice; hardened the two tests (above).

## Known limitations / deferred (not blocking)

- A `style_override` patch overrides base/variant only; the theme's hover/pressed/disabled deltas still layer on top. So a hovered **Ghost** reverts to slate hover chrome, and Primary/Danger fills are tinted by the slate hover gradient. A per-state-transparent variant would need its own `RoleStyle` — deferred (no screen uses Ghost/Danger yet). Documented on `app_button_variant.h`.
- The pre-existing `renderer_golden_tests` failure predates this branch's convergence work (it is a synthetic renderer-IR golden, untouched here). Left as-is; not regenerated blindly.

## Commit trail (on `feat/styling-render-system`, not pushed)

`227bfc7` spec+plan · `59b77a8` P1 theme ownership · `588066e` P2 converge primitives · `2ea3311` P3 variant table · `b805fdc` review fixes (+ this docs commit).
