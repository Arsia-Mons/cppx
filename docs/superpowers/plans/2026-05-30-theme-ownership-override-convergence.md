# Theme Ownership + Override Convergence Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans (inline) to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move theme *values* out of `src/ui` into a client-installed `ThemeProvider`, converge every primitive onto one sparse `StylePatch` override prop, and make `AppButton`'s variant table actually paint.

**Architecture:** Two layers. `src/ui/style` keeps the theme *mechanism* (`Theme` types, `ThemeContext`, `use_theme`, `resolve`, `StylePatch`) plus a *neutral* `default_theme()` fallback. The client owns the slate *values* (`app_theme()`) and installs them via `::ui::provider(&ui::ThemeContext, …)`. Every primitive resolves `resolve(use_theme().<role>, props.style_override, interaction_state(…))`. `shooter::tokens` builders return `StylePatch`; semantic components map variant → patch.

**Tech Stack:** C++20, SDL3, custom React-style hook runtime, cppx transpiler (`.hx`→`.h`, `.cppx`→`.cpp`), CMake + ctest.

**Spec:** `docs/superpowers/specs/2026-05-30-theme-ownership-override-convergence-design.md`

**Gate (every commit):** `./build.sh --tests` — the failing set must stay exactly `{renderer_golden_tests}` (pre-existing, synthetic, ~582 px). Everything else green. NOTE: piping build.sh to `tail` masks its nonzero exit; read the ctest summary line, not the shell exit code.

---

## Task 1 — P1: Theme ownership → client

**Files:**
- Modify: `src/ui/style/theme.h` (delete `TextStyleValue`/`TextStyleContext`)
- Modify: `src/ui/style/default_theme.cpp` (slate values → neutral fallback)
- Create: `src/client/ui/app_theme.h`, `src/client/ui/app_theme.cpp` (relocated slate as `app_theme()`)
- Modify: install site (the shell `FrameProvider`/`wrap_root` seam — confirm exact file: `src/app/*` or `src/client/ui/app_shell/ui_pipeline.cpp` / `client_ui.cpp`)
- Modify: `CMakeLists.txt` (add `app_theme.cpp` to `hello` + `shooter_ui_tests` + any client-linking test targets)
- Modify: `tests/ui_style_tests.cpp` (assert neutral fallback shape; was asserting slate)
- Test: `tests/ui_style_tests.cpp` or `tests/ui_pipeline_tests.cpp` (provider-read test)

- [ ] **Step 1: Write failing test — provider read.** In the client-linking test (ui_pipeline_tests or client_ui_tests), assert that under an installed `ThemeProvider(app_theme())`, a `use_theme()` read returns `app_theme()`'s button base (slate), not the neutral fallback. Build the minimal retained frame that installs the provider and reads the theme inside a component.
- [ ] **Step 2: Write failing test — neutral fallback.** In `tests/ui_style_tests.cpp`, assert `ui::default_theme().button.base` equals the *neutral* values (e.g. flat gray, no gradient) — this fails today because default_theme is the slate.
- [ ] **Step 3: Run both, verify they fail.** `./build.sh --target ui_style_tests && ctest --test-dir cmake-build-debug -R ui_style_tests` → FAIL.
- [ ] **Step 4: Create `app_theme.{h,cpp}`.** Move the *current* `default_theme.cpp` body verbatim into `client::ui::app_theme()` returning `const ::ui::Theme&` (keep the `static const Theme` idiom). Header declares `const ::ui::Theme& app_theme();` in the client namespace. Plain C++ — rooted includes.
- [ ] **Step 5: Neutralize `default_theme.cpp`.** Replace the slate body with a minimal unopinionated theme: controls = flat neutral-gray fill + 1px gray border, no gradient; text = a plain default color/size; box = transparent; dialog = a plain panel; focus ring kept (a visible outline) so focusable controls remain testable. No product palette.
- [ ] **Step 6: Delete `TextStyleValue` + `TextStyleContext`** from `theme.h` (verified zero readers/providers).
- [ ] **Step 7: Install the provider.** At the shell cross-cutting seam, wrap the screen tree: `::ui::provider("ThemeProvider", &::ui::ThemeContext, ::ui::copy_value(client::ui::app_theme()), children)`. Use the existing `FrameProvider`/`wrap_root` mechanism (same altitude as ScreenProvider/InteractionProvider).
- [ ] **Step 8: CMake.** Add `src/client/ui/app_theme.cpp` to the `hello` target and every test target that links client UI (`shooter_ui_tests`, `ui_pipeline_tests`, `client_ui_tests` as needed). Rooted-include note: it's plain `.cpp`, not transpiled.
- [ ] **Step 9: Run targeted tests, verify pass.** `./build.sh --target ui_style_tests` + the provider-read target → PASS.
- [ ] **Step 10: Full gate.** `./build.sh --tests` → failing set == `{renderer_golden_tests}` only. Fix any standalone `ui` test that asserted the slate (it now sees the neutral fallback).
- [ ] **Step 11: Capture + view a frame** (capture cmd in spec/memory). Confirm the product slate look is preserved (provider installed correctly). View the PNG.
- [ ] **Step 12: Commit.** `git add -A && git commit` — "Client UI: move theme values into a client-installed ThemeProvider; ui keeps a neutral fallback".

---

## Task 2 — P2a: `tokens.h` builders return `StylePatch`

**Files:**
- Modify: `src/client/ui/components/tokens.h:62-85` (`fill_visual`/`panel_visual`/`text_visual`)

- [ ] **Step 1: Convert builders.** Change return type `::ui::VisualStyle` → `::ui::StylePatch`, rename `fill_visual`→`fill_patch`, `panel_visual`→`panel_patch`, `text_visual`→`text_patch`. Body uses `::ui::patch().background(...)` / `.border(...)` / `.text(...)` so only the intended fields are `set`. (Do NOT set fields the old dense builder left default — e.g. `fill_patch` sets only `background`.)
- [ ] **Step 2: Build will break at call sites** — that's expected; Tasks 3–4 fix them. Do not commit alone; fold into the P2 commit after call sites compile. (Verification deferred to Task 4 Step N.)

---

## Task 3 — P2b: Converge the `ui` primitives

**Files:**
- Modify: `src/ui/components/box.{hx,cppx}`, `text.{hx,cppx}`, `dialog.{hx,cppx}`
- Modify: `src/ui/components/button.{hx,cppx}`, `input.{hx,cppx}`, `checkbox.{hx,cppx}`
- Test: `tests/retained_ui_primitives_tests.cpp` (or `ui_components_tests` — whichever asserts resolved `node.visual`)

- [ ] **Step 1: Write failing test — patch over theme role.** Assert that a `Box` declared with `style_override = patch().background({1,2,3,255})` produces a node whose resolved `visual.background == {1,2,3,255}`, and an un-patched `Box` resolves `theme.box.base`. (Match the harness used by the existing primitives test.)
- [ ] **Step 2: Run, verify fail** (prop doesn't exist yet). FAIL to compile/assert.
- [ ] **Step 3: Box/Text/Dialog `.hx`** — replace the `::ui::VisualStyle visual = {}` field with `::ui::StylePatch style_override = {}` **in the same field position** (keeps call-site attribute order valid).
- [ ] **Step 4: Box/Text/Dialog `.cppx`** — replace `visual={props.visual}` with `visual={::ui::resolve(::ui::use_theme().box, props.style_override, {})}` (role per primitive: box/text/dialog; `{}` = default all-false InteractionState).
- [ ] **Step 5: Button/Input/Checkbox `.hx`** — add `::ui::StylePatch style_override = {};` as a new field (deterministic position; update AppButton initializer in Task 5 to match).
- [ ] **Step 6: Button/Input/Checkbox `.cppx`** — replace the `{}` 2nd arg to `resolve(...)` with `props.style_override` (checkbox: thread into the body resolve; mark resolve stays `{}` unless a mark override is wanted — keep `{}`).
- [ ] **Step 7: Run primitive test, verify pass** (after Task 4 makes the tree compile). PASS.
- [ ] (Commit folded into Task 4.)

---

## Task 4 — P2c: Migrate client call sites + verify P2

**Files (JSX `.cppx`, order-sensitive):** `loadout_title.cppx`, `loadout_screen_frame.cppx`, `layout/screen_layout.cppx`, `surfaces/panel.cppx`, `text/body_text.cppx`, `text/screen_subtitle.cppx`, `text/screen_title.cppx`
**Files (plain `.cpp`):** `weapon_tile.cpp`, `confirm_dialog.cpp`, `equipment_slot.cpp`, `hud_band.cpp`

- [ ] **Step 1: `.cppx` sites** — change `visual={tokens::*_visual(...)}` → `style_override={tokens::*_patch(...)}`, keeping the attribute in the same position. `panel.cppx`: rebuild its switch to assemble a `::ui::StylePatch` (via `tokens::*_patch`) instead of a `VisualStyle`, pass `style_override={…}`.
- [ ] **Step 2: plain `.cpp` sites** — change `.visual = tokens::*_visual(...)` → `.style_override = tokens::*_patch(...)` (designated-initializer position unchanged since the field replaces `visual` in place).
- [ ] **Step 3: Build.** `./build.sh` — fix any `-Werror=reorder-init-list` order errors (attribute order must match the new struct field order).
- [ ] **Step 4: Full gate.** `./build.sh --tests` → failing set == `{renderer_golden_tests}` only.
- [ ] **Step 5: Capture + view a frame.** Confirm screens look unchanged (tokens patches over theme roles reproduce prior pixels; text now also inherits theme.text defaults where unset).
- [ ] **Step 6: Commit.** "Client UI: converge primitives onto one sparse StylePatch override; tokens builders emit patches".

---

## Task 5 — P3: AppButton variant table

**Files:**
- Modify: `src/client/ui/components/actions/app_button_variant.h:51-53` (`app_button_variant_patch`)
- Modify: `src/client/ui/components/actions/app_button.cppx:37,42-60` (assign into `.style_override`)
- Test: `tests/shooter_ui_tests.cpp` (variant distinctness)

- [ ] **Step 1: Write failing test.** Assert the resolved `node.visual` of an `AppButton{variant=Danger}` differs from `{variant=Primary}` and from `{variant=Secondary}` (e.g. distinct `background`). FAIL today (all identical).
- [ ] **Step 2: Run, verify fail.** PASS would be the bug; expect FAIL.
- [ ] **Step 3: Fill `app_button_variant_patch`.** Switch on variant returning real `::ui::StylePatch` (via `::ui::patch()`): Primary → accent fill (`tokens` accent bg + matching gradient/border or flat accent), Secondary → `{}` (theme slate default), Danger → red fill, Ghost → transparent bg + zero border. Use `shooter::tokens` colors; add a couple of accent/danger tokens to `tokens.h` if missing.
- [ ] **Step 4: Wire it.** In `app_button.cppx`, replace `(void)app_button_variant_patch(props.variant);` and set `.style_override = app_button_variant_patch(props.variant)` in the `ButtonProps{…}` initializer (correct field-order position).
- [ ] **Step 5: Run test, verify pass.** PASS — variants resolve to distinct visuals.
- [ ] **Step 6: Full gate.** `./build.sh --tests` → failing set == `{renderer_golden_tests}` only.
- [ ] **Step 7: Capture + view.** (Optional: a screen forcing each variant — only if quick.) Confirm no regression on default screens.
- [ ] **Step 8: Commit.** "Client UI: fill AppButton variant table; variants now paint distinctly".

---

## Task 6 — P4: Finalize

- [ ] **Step 1: Clean full build+test.** `./build.sh --tests` from a clean configure if needed; confirm failing set == `{renderer_golden_tests}` only.
- [ ] **Step 2: Update `architecture.md` / `src/ui/CLAUDE.md`** where they describe theme ownership (the stale "Design tokens — none yet" line; the theme-in-ui description). Keep edits minimal and accurate.
- [ ] **Step 3: Write completion report** (decisions, what moved, the pre-existing golden caveat, verification evidence) — short doc or commit body.
- [ ] **Step 4: Update memory** `theme-ownership-override-convergence` with final status + commit range.
- [ ] **Step 5: Commit** the docs.

---

## Self-review (coverage vs spec)

- Decision 1 (full fix): Tasks 1–5. ✔
- Decision 2 (converge to one StylePatch): Task 3 (prop swap) + Task 4 (call sites). ✔
- Decision 3 (two layers): Task 1 (mechanism stays, values move) + Task 2 (tokens emit patches). ✔
- Decision 4 (neutral fallback): Task 1 Steps 5, 2. ✔
- Decision 5 (Box/Text/Dialog over theme role): Task 3 Step 4. ✔
- Decision 6 (delete TextStyleContext): Task 1 Step 6. ✔
- Decision 7 (variant values): Task 5 Step 3. ✔
- Tests: provider read (T1), neutral fallback (T1), patch-over-role (T3), variant distinctness (T5). ✔
- Golden caveat documented (T6). ✔

**Open item resolved at execution:** exact install-site file for the `ThemeProvider` (FrameProvider seam) — locate the current `set_frame_provider`/`wrap_root` caller in `src/app` or shell setup before Task 1 Step 7.
