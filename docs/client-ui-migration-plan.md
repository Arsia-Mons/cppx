# Client UI Migration Plan (canonical, reconciled)

Lead-architect reconciliation of the per-area specs into ONE internally consistent,
behavior-preserving migration of `src/client/ui` toward
`docs/client-ui-component-architecture-goal.md`.

This plan is the single source of truth. Where two area specs named the same concept
two ways, this document picks one owner and one name; the rejected alternative is noted
inline as "superseded".

## 0. Reconciliation decisions (read first)

These resolve every cross-area naming/variant overlap. They are binding.

1. **Tokens live in ONE shared header `src/client/ui/components/tokens.h`, namespace
   `shooter::tokens`.** It replaces the entire `shooter::theme` namespace in
   `screen_chrome.hx`. It is a plain `.h` (no JSX), header-only (`constexpr` + `inline`),
   NOT in `cppx_transpile`. It exposes named constants + the three visual builders
   `fill_visual` / `panel_visual` / `text_visual` (verbatim semantics). It does NOT export
   recipe functions and does NOT export components.
   - Superseded: per-area private token files (`layout_tokens.h` in
     `shooter::client_layout::detail`, per-component `detail::` tables in `panel.hx`,
     `text_tokens.hx`, screens-simple's `theme_tokens.h` keeping `shooter::theme`).
     Rationale: five private token tables re-fragment the palette and force each
     component to re-derive the same colors; a single shared `tokens.h` is the shadcn
     "design tokens" analogue (one palette, per-component variant→token switch). The
     anti-catch-all rule (goal lines 80, 210) bans bundling *recipe functions + unrelated
     component exports*; a cohesive token header with only constants + 3 generic builders
     is the explicitly-allowed "tightly scoped public concept" / "private implementation
     helper" exception (goal lines 75-76). EXCEPTION: the **loadout** feature keeps a
     small screen-local `screens/loadout/loadout_tokens.h` (`shooter::loadout` namespace)
     for its feature-local colors/sizes that no shared component consumes — co-location
     wins for screen-local-only values; it `#include`s `tokens.h` to reuse the three
     builders. See §3.
2. **`AppButton` is the one button component**; `MenuButton` is deleted (not aliased).
   Variant/size enums are unified to the goal-doc superset:
   `enum class AppButtonVariant { Primary, Secondary, Danger, Ghost }` and
   `enum class AppButtonSize { Md, Sm }`. All variants map to the SAME themed paint at
   baseline (no-op `StylePatch`) so pixels are byte-identical to today; the variant table
   is the single place future polish edits.
   - Superseded: screens-simple's `{Primary,Danger}`/`{Md}` (too narrow — loadout needs
     `Secondary`, actions reserves `Ghost`); keeping `MenuButton` as a preset.
3. **Screen frames: ONE compound `ScreenLayout` with `ScreenLayoutVariant`** in
   `components/layout/screen_layout.{hx,cppx}`. The Box-vs-Dialog choice and modal flag are
   derived from the variant (impl detail), never a caller knob.
   - Superseded: screens-simple's four separate frame files
     (`menu_screen_frame`, `game_screen_frame`, `overlay_screen_frame`,
     `centered_overlay_frame`). Rationale: all four are the same concept (full-viewport
     screen root) differing only by an app-approved appearance bundle — the textbook
     variant case (goal lines 122-149, 151-160). The loadout root Dialog is NOT one of
     these four (dynamic `modal`); it gets its own `LoadoutScreenFrame` (§5/§3, owned by
     loadout).
4. **Panel: ONE `Panel` with `PanelVariant { Hero, Overlay, Sunken }`** (+ optional
   `PanelSize { Auto, Sm, Md, Lg }`) in `components/surfaces/panel.{hx,cppx}`.
   - Superseded: screens-simple's `{Hero,Overlay}`-only panel and the two-file
     `hero_panel`/`overlay_panel`. `Sunken` is required for the loadout "details" column.
5. **Titles: ONE `ScreenTitle` with `ScreenTitleVariant { Hero, Screen, Dialog, Popup }`**
   plus a standalone `ScreenSubtitle`, plus a standalone `BodyText`
   (`BodyTextVariant { Body, Strong, Message, Detail }` × `BodyTextTone { Default, Muted,
   Disabled }`) — all in `components/text/`.
   - `Popup` covers the loadout confirm-dialog title (22px); `BodyText` covers all
     in-panel body/label/detail leaves. Superseded: three-component title trio
     (`HeroTitle`/`ScreenTitle`/`DialogTitle`); screens-simple's `{Hero,Screen,Dialog}`
     (missing Popup) and no `BodyText`.
6. **Loadout tabs: compound `LoadoutTabs` + `LoadoutTabs::Tab`** (struct with nested
   `TabProps` + static `Tab`), in `screens/loadout/components/loadout_tabs.{hx,cppx}`.
   - Superseded: navigation spec's free-function `LoadoutTabList`/`LoadoutTab` pair. The
     compound form matches the goal doc's `<LoadoutTabs />` and the transpiler's dotted-tag
     lowering (`<LoadoutTabs.Tab/>` → `LoadoutTabs::Tab` / `LoadoutTabs::TabProps`,
     `cppx_transpile.py:74-83`). The screen still owns `active_tab` `use_state` and the
     3-action `on_select` closure (where the 2-deferred-write contract lives).
7. **App shell: framework files move to `app_shell/`; game `shooter_provider` STAYS in
   `providers/`.** Nav/action hooks move to `screens/<screen>/<screen>_actions.{h,cpp}`.
   - Superseded: moving `shooter_provider` into `app_shell/` (would muddy the
     game-agnostic seam and add 9 include edits for no gain).
8. **Namespace: everything client-side stays in `namespace shooter`** (components, tokens,
   variant enums) and framework stays in `client::ui` / `client::ui::internal`. No new
   `client_ui`-style sub-namespace. The loadout feature-local tokens use `shooter::loadout`.
   `shooter::tokens` is the shared token sub-namespace.
9. **Each new shared component re-derives its paint from `tokens::` locally** (a
   per-component variant→token `switch`); there is no surviving `theme::` shim and no
   shared recipe layer. After all consumers migrate, `screen_chrome.{hx,cppx}` is deleted.
10. **Umbrella headers**: each shared component family ships an umbrella
    (`actions/actions.h`, `layout/layout.h`, `surfaces/surfaces.h`, `text/text.h`,
    `navigation/navigation.h`) that re-exports the family's generated headers. These are
    the goal-doc-allowed "small index/umbrella header" (line 75). Screens include the
    family umbrellas they use. No single global `components.h` aggregator (keeps include
    graphs honest).

Hard contract (from the task + `tests/shooter_ui_tests.cpp` + `tests/ui_cli_commands.py`):
preserve every screen `debug_name()`, every control ID + offset, retained tree
shape/order/focusability, default focus, modal flags, `previous_focus_before_modal`,
the gear-tab 2-deferred-write count, and `react_error_count()==0`. Paint is free (no test
asserts pixels; smoke tests only assert capture existence/size). All baseline paint values
below reproduce today's pixels so the migration is byte-identical until variants diverge.

---

## 0.5 Binding corrections from adversarial review (OVERRIDE conflicts below)

Four review critics validated this plan against the live transpiler, the test contract,
the goal doc, and CMake. Where these corrections conflict with §1–§8, **THESE WIN**.

**C1 — Loadout body fragment must be HAND-WRITTEN C++ (build-breaker fix).** The form
`return ::ui::fragment(::ui::children({ <Tag/>, <Tag>…</Tag> }))` does NOT transpile:
`tools/cppx_transpile.py` only enters JSX mode on a line starting with `<` or `return <`
(`is_jsx_line`). A `return ::ui::fragment(…)` line is emitted verbatim with an EMPTY parser
stack, so bare `<Tag/>` children get `;` terminators (uncompilable) + spurious `text(",")`
nodes (extra siblings). **RULE:** a component body that is a *single JSX root element* →
author `.cppx` (JSX). A body that returns a *fragment* or assembles/branches children in C++
→ author *plain `.cpp`* and build the tree with `::ui::component(name, Props{…}, Fn)` +
`::ui::fragment(::ui::children({…}))`. So `loadout_content` is **plain `.cpp`**
(`LoadoutScreenBody`) hand-assembling: child#1 `::ui::component("LoadoutConfirmDialog", …)`;
child#2 `::ui::component("LoadoutScreenFrame", LoadoutScreenFrameProps{.key="root",
.confirm_open=confirm_open, .children=::ui::children({…title, tabs, body…})}, LoadoutScreenFrame)`.
Sub-components (LoadoutScreenFrame/LoadoutTitle/LoadoutTabs/LoadoutBody/LoadoutWeaponGrid/
LoadoutDetails/weapon_tile/equipment_slot) are single-root → `.cppx`. `confirm_dialog` may
stay plain `.cpp`. **Every authored `.cppx/.hx` MUST be run through
`python3 tools/cppx_transpile.py <f>` and inspected before "done"** (no stray `text(",")`,
no `;` mid-initializer-list).

**C2 — Keep shared component public APIs clean (no host leaks).**
- `AppButtonProps`: **REMOVE** `::ui::SemanticRole role` and `::ui::Style advanced_layout` /
  `has_advanced_layout`. Final public props: `key, control_id, control_offset, variant, size,
  disabled, selected, default_focused, label, on_press` (no-arg), `on_focus` (no-arg),
  `accessibility_label, children`.
- `WeaponTile`, `EquipmentSlot`, and `LoadoutTabs::Tab` adapt `ui::components::Button`
  **directly in their own `.cppx` impls** (raw primitive inside a component impl is allowed,
  goal:117-118,241-243). This keeps leaf geometry byte-exact (weapon tile 190×78 pad10 gap5;
  equipment slot 232×38 pad{10,10,8,8}; tab 132×34 pad{12,12,7,7}; border `selected?2:1`) and
  lets `LoadoutTabs::Tab` set `accessibility={.role=Tab}` internally — no `role` on AppButton.
- `ScreenTitleProps` / `BodyTextProps`: **REMOVE** `reserved_height_override`. `LoadoutTitle`
  is loadout-local, adapting `ui::components::Text` directly (height 30, `kTextTitle`, font 26).
  Add `BodyTextVariant::Summary` (font 14, reserved height 18, `kTextBody`) for the loadout
  summary so the one off-default height is a variant, not a numeric knob.

**C3 — CMake: register every NEW plain `.cpp`.** `${CLIENT_UI_GENERATED}` auto-flows only
`.cppx/.hx` outputs. Every new plain `.cpp` must be added to BOTH `hello` (~L181-193) and
`shooter_ui_tests` (~L621-633): `main_menu_actions.cpp`, `pause_actions.cpp`,
`options_actions.cpp`, `in_game_actions.cpp`, `loadout_actions.cpp`, `loadout_content.cpp`
(+ `confirm_dialog.cpp` if kept plain). `app_button_variant.h` helpers are `inline`
(header-only). After Phases C and E, grep both lists to confirm. tokens include path is
exactly `"client/ui/components/tokens.h"`.

**C4 — Contract-matrix additions (pin these focus facts):**
- Weapon-grid tab partition: weapons tab ⇒ WeaponTile offsets {0,1 in `weapon-row-0`; 2 in
  `weapon-row-1`}; gear tab ⇒ offset 3 in `gear-row` (ui_cli_commands.py:239,247;
  shooter_ui_tests.cpp:321-332,385).
- LoadoutDetails ordered children (frozen): summary, CompareToggle, BuyWeaponButton,
  EquipWeaponButton, BackFromLoadoutButton, slots-title, PrimarySlot, GearSlot — keyboard-down
  from the grid lands on BuyWeaponButton (shooter_ui_tests.cpp:329-332).
- ConfirmLoadoutActionButton / CancelLoadoutActionButton must be byte-exact literals.

**C5 — Sequencing:** `callback_deps.h` is **NOT** moved (stays `src/client/ui/callback_deps.h`)
to cut churn. The **app_shell move is the LAST phase**.

---

## 1. Final target folder tree under `src/client/ui`

```text
src/client/ui/
  app_shell/                                # MOVED framework shell (was loose + navigation/ + internal/ + providers/app_shell)
    client_ui.{h,cpp}                       # MOVED from src/client/ui/client_ui.{h,cpp}
    ui_pipeline.{h,cpp}                     # MOVED from src/client/ui/ui_pipeline.{h,cpp}
    deferred_ui_mutation.h                  # MOVED from src/client/ui/internal/deferred_ui_mutation.h
    app_shell_provider.{h,cpp}              # MOVED+RENAMED from providers/app_shell.{h,cpp}
    navigation/
      screen_stack.{h,cpp}                  # MOVED from navigation/screen_stack.{h,cpp}
      ui_screen.h                           # MOVED from navigation/ui_screen.h
    callback_deps.h                         # MOVED from src/client/ui/callback_deps.h  (see §4 note)

  providers/
    shooter_provider.{h,cpp}                # UNCHANGED (game vocabulary stays here)

  hooks/
    shooter_hud.{h,cpp}                     # UNCHANGED
    shooter_weapons.{h,cpp}                 # UNCHANGED (include path edits only)

  components/                               # shared semantic app components
    tokens.h                                # NEW plain .h: shooter::tokens constants + 3 visual builders
    actions/
      actions.h                             # NEW umbrella
      app_button_variant.h                  # NEW plain .h: AppButtonVariant/Size enums + impl helpers
      app_button.hx / app_button.cppx       # NEW: AppButton (adapts ui::components::Button)
      app_checkbox.hx / app_checkbox.cppx   # NEW: AppCheckbox (adapts ui::components::Checkbox)
      app_input.hx / app_input.cppx         # NEW: AppInput (adapts ui::components::Input)
      action_row.hx / action_row.cppx       # NEW: ActionRow (adapts ui::components::Box)
    layout/
      layout.h                              # NEW umbrella
      screen_layout.hx / screen_layout.cppx # NEW: ScreenLayout + ScreenLayoutVariant
    surfaces/
      surfaces.h                            # NEW umbrella
      panel.hx / panel.cppx                 # NEW: Panel + PanelVariant + PanelSize
    text/
      text.h                                # NEW umbrella
      screen_title.hx / screen_title.cppx   # NEW: ScreenTitle + ScreenTitleVariant
      screen_subtitle.hx / screen_subtitle.cppx  # NEW: ScreenSubtitle
      body_text.hx / body_text.cppx         # NEW: BodyText + BodyTextVariant + BodyTextTone
    hud_band.{h,cpp}                         # UNCHANGED structure; include + call-site edits to tokens::
    # screen_chrome.{hx,cppx}               # DELETED at end of migration

  screens/
    main_menu/
      main_menu_screen.hx / .cppx           # EDIT: compose semantic components; drop nav-hook body+decl
      main_menu_actions.h / .cpp            # NEW: use_exit_to_main_menu()
    pause/
      pause_screen.hx / .cppx               # EDIT
      pause_actions.h / .cpp                # NEW: use_push_pause_screen()
    options/
      options_screen.hx / .cppx             # EDIT
      options_actions.h / .cpp              # NEW: use_push_options_screen()
    in_game/
      in_game_screen.hx / .cppx             # EDIT
      in_game_actions.h / .cpp              # NEW: use_start_match()
    loadout/
      loadout_screen.h / .cpp               # EDIT: trim to LoadoutScreen + LoadoutScreenView; drop hook + monolith body
      loadout_actions.h / .cpp              # NEW: use_push_loadout_screen()
      loadout_state.{h,cpp}                 # UNCHANGED behavior (include path edits only)
      loadout_tokens.h                      # NEW plain .h: shooter::loadout feature-local tokens
      components/
        loadout_content.hx / .cppx          # NEW: LoadoutScreenBody (JSX body; display name preserved)
        loadout_screen_frame.hx / .cppx     # NEW: LoadoutScreenFrame (root Dialog; owns modal=!confirm_open)
        loadout_title.hx / .cppx            # NEW: LoadoutTitle
        loadout_tabs.hx / .cppx             # NEW: LoadoutTabs + LoadoutTabs::Tab
        loadout_body.hx / .cppx             # NEW: LoadoutBody (grid+details row)
        loadout_weapon_grid.hx / .cppx      # NEW: LoadoutWeaponGrid (reads active_tab)
        loadout_details.hx / .cppx          # NEW: LoadoutDetails
        weapon_tile.hx / .cppx              # CONVERTED from weapon_tile.{h,cpp}
        equipment_slot.hx / .cppx           # CONVERTED from equipment_slot.{h,cpp}
        confirm_dialog.hx / .cppx           # CONVERTED from confirm_dialog.{h,cpp}
```

Deleted at end: `src/client/ui/client_ui.{h,cpp}`, `ui_pipeline.{h,cpp}`,
`internal/deferred_ui_mutation.h` (dir removed), `navigation/screen_stack.{h,cpp}`,
`navigation/ui_screen.h` (old `navigation/` dir removed), `providers/app_shell.{h,cpp}`,
`components/screen_chrome.{hx,cppx}`, loadout `components/weapon_tile.{h,cpp}`,
`equipment_slot.{h,cpp}`, `confirm_dialog.{h,cpp}` (replaced by `.hx/.cppx`).

> Transpiler/path mechanic confirmed: `cmake/cppx_transpile.cmake:16-17` builds
> `output_rel` from the FULL source-relative path and always passes `-o "${output_abs}"`,
> so `tools/cppx_transpile.py`'s flat `output_path_for` (line 448) is bypassed in-build.
> Therefore `src/client/ui/components/layout/screen_layout.hx` →
> `<build>/generated/cppx/src/client/ui/components/layout/screen_layout.h`, and the
> include-root `generated/cppx/src` (`CMakeLists.txt:97`) makes the public include
> `"client/ui/components/layout/screen_layout.h"`. Verified against the existing
> `screen_chrome` precedent (`weapon_tile.cpp:8` includes
> `"client/ui/components/screen_chrome.h"`).

---

## 2. Component catalog

For each shared component: file path, kind, semantic Props, enums, the `src/ui` primitive
it adapts to + the adaptation, and the variant→paint(token) baseline table.

Adaptation rules common to all `.cppx` adapters (the contract that preserves retained
identity):

| Semantic prop | ui primitive prop | retained effect |
|---|---|---|
| `control_id` | `.id` | `element.cpp` → `node.control_id` (test lookup key) |
| `control_offset` | `.id_offset` | `node.control_offset` (weapon-tile offset 0..3) |
| `default_focused` | `.autofocus` | `interaction_from_props` → `initial_focus` |
| `disabled` | `.disabled` | `NodeInteraction.disabled` |
| `on_press` (no-arg) | `.on_activate` wrapping a lambda ignoring `ActivationEvent` | retained activation |
| `on_focus` (no-arg) | `.on_focus` wrapping a lambda ignoring `FocusEvent` | tile/slot select-on-focus |
| `key` | `.key` | reconciler identity |

### 2.0 `components/tokens.h` — see §3 (tokens/theme module).

### 2.1 `components/actions/app_button_variant.h` (plain `.h`)

```cpp
#pragma once
#include "ui/components/common.h"   // ::ui::Style, ::ui::StylePatch
namespace shooter {
enum class AppButtonVariant { Primary, Secondary, Danger, Ghost };
enum class AppButtonSize    { Md, Sm };

// impl-only helpers (host detail allowed inside component impl headers, goal :146-149).
::ui::Style       app_button_layout(AppButtonSize size, bool selected);  // w/h/padding/align + border_width
::ui::StylePatch  app_button_variant_patch(AppButtonVariant v);          // baseline: empty patch
} // namespace shooter
```

`app_button_layout` baseline table (reproduces current per-site geometry):

| size | width | height | padding (l,r,t,b) | align/justify | border_width | source |
|---|---|---|---|---|---|---|
| `Md` | 132 | 38 | 14,14,8,8 | Center/Center | `selected?2:1` | `ui Button` default `button.hx:23-29`; every MenuButton + loadout Buy/Equip/Back |
| `Sm` | 132 | 34 | 12,12,7,7 | Center/Center | `selected?2:1` | loadout WeaponsTab/GearTab `loadout_screen.cpp:238-249,287-298` |

`app_button_variant_patch` baseline: ALL four variants → empty `StylePatch{}` (no-op).
`ui::components::Button` resolves all fill/border/gradient/focus-ring from
`use_theme().button` (`button.cppx:18-20`); it has no `visual`/variant prop today, so the
patch is computed and discarded at baseline. Activating per-variant paint is the deferred
`ui`-change open question in §8.

### 2.2 `components/actions/app_button.{hx,cppx}` — `AppButton`

`.hx` Props:
```cpp
struct AppButtonProps {
  const char *key = nullptr;
  const char *control_id = nullptr;        // -> Button.id
  int control_offset = 0;                  // -> Button.id_offset (weapon tiles 0..3)
  AppButtonVariant variant = AppButtonVariant::Primary;
  AppButtonSize size = AppButtonSize::Md;
  bool disabled = false;
  bool selected = false;                   // border-width bump (paint only)
  bool default_focused = false;            // -> Button.autofocus
  const char *label = nullptr;
  ::ui::SemanticRole role = ::ui::SemanticRole::Auto;  // Tab for loadout tabs; Auto->Button
  std::function<void()> on_press = {};     // no event arg
  std::function<void()> on_focus = {};     // no event arg (tile/slot select-on-focus)
  const char *accessibility_label = nullptr;           // a11y label distinct from visible text
  ::ui::Style advanced_layout = {};        // ESCAPE HATCH: tile/slot content box
  bool has_advanced_layout = false;        // gate for advanced_layout
  ::ui::UiChildren children = {};
};
::ui::UiElement AppButton(const AppButtonProps &props);
```

Adapts `ui::components::Button`. `.cppx` body sets `id=control_id`, `idOffset=control_offset`,
`disabled`, `autofocus=default_focused`, `label`,
`accessibility={.role=role, .label = accessibility_label ? accessibility_label : label}`,
`style = has_advanced_layout ? advanced_layout(with border_width=selected?2:1)
: app_button_layout(size, selected)`, wraps `on_press`/`on_focus` into the event-taking
ui callbacks, forwards `children` then `label`-fallback text child. `variant` →
`app_button_variant_patch(variant)` computed and (baseline) discarded. `focusable` left
default — Button forces `focusable=true` (`interaction_from_props(props,true)`), preserving
the focus contract.

Justification for non-doc props (all behavior-preserving, none are `style`/`visual`/event
leaks in ordinary screen paths):
`control_offset` (task-required for 4 tiles), `role` (loadout tabs set `Tab`),
`on_focus` no-arg (tile/slot select-on-focus, current handler ignores the event),
`accessibility_label` (a11y label ≠ visible text on tiles/slots),
`advanced_layout`+`has_advanced_layout` (clearly-named advanced escape hatch for the two
genuinely-bespoke tile/slot geometries, goal :147; only WeaponTile/EquipmentSlot use it).

### 2.3 `components/actions/app_checkbox.{hx,cppx}` — `AppCheckbox`

`.hx` Props: `{ key, control_id, bool checked, const char *label, std::function<void(bool)> on_change }`.
Adapts `ui::components::Checkbox`: `id=control_id`, `checked`, `label`, `on_change` 1:1.
Baseline layout 178×38 (`checkbox.hx:24-32`), paint from `use_theme().checkbox` — unchanged.
No variant enum (single appearance today). Used by options `LargeHudToggle`/`ReducedMotionToggle`
and loadout `CompareToggle`.

### 2.4 `components/actions/app_input.{hx,cppx}` — `AppInput`

`.hx` Props: `{ key, control_id, const char *accessibility_label, const char *value,
std::function<void(const std::string&)> on_change }`.
Adapts `ui::components::Input`: `id=control_id`,
`accessibility={.label=accessibility_label}`, `value`, `on_change` 1:1.
Baseline 220×36 (`input.hx:25-31`), paint from theme. Used by options `NameInput`
(`accessibility_label="Name"`; smoke test reads value transitions, and the a11y label).
No variant enum.

### 2.5 `components/actions/action_row.{hx,cppx}` — `ActionRow`

`.hx` Props: `{ key, children }`. Adapts `ui::components::Box` with
`{ direction=Row, align_items=Start, gap=12 }` (verbatim `screen_chrome.hx:142-148`).
Only consumer: in_game. No variant. (Loadout tabs/dialog-action rows use gap=10 and are
NOT ActionRow — they are local Boxes inside their own components; see §5.)

### 2.6 `components/layout/screen_layout.{hx,cppx}` — `ScreenLayout`

`.hx`:
```cpp
enum class ScreenLayoutVariant { Menu, Game, Overlay, CenteredOverlay };
struct ScreenLayoutProps {
  const char *key = nullptr;
  ScreenLayoutVariant variant = ScreenLayoutVariant::Menu;
  ::ui::UiChildren children = {};
};
::ui::UiElement ScreenLayout(const ScreenLayoutProps &props);
```
Adapts `ui::components::Box` (Menu/Game) OR `ui::components::Dialog` (Overlay/CenteredOverlay).
The `.cppx` branches on variant and emits the matching JSX tag; CenteredOverlay emits NO
`visual=` (transparent), the other three pass a fill visual. Dialog `modal=true` default
(`dialog.hx:21`) is unchanged → modal flag identical to today.

Variant → style + visual baseline (verbatim from `screen_chrome.hx`):

| Variant | primitive | direction | align_items | justify | w/h | padding | gap | bg (straight α) | source |
|---|---|---|---|---|---|---|---|---|---|
| `Menu` | Box | Column | Center | Center | 100/100 | 36 all | — | `{8,14,18,255}` | hx:52-61,151,11 |
| `Game` | Box | Column | (Stretch dflt) | (Start dflt) | 100/100 | 24 all | 18 | `{10,16,18,255}` | hx:63-71,154,12 |
| `Overlay` | Dialog(modal) | Column | Start | (Start dflt) | 100/100 | 24 all | 12 | `{14,22,28,245}` | hx:73-82,157,13 |
| `CenteredOverlay` | Dialog(modal) | Column | Center | Center | 100/100 | — | — | none (no visual) | hx:84-92 |

Implementation composes these via `tokens::fill_visual(tokens::kSurface*)`. The visual is
built inline per-variant (no recipe layer). Border width 0 for all frames (borders belong
to panels).

### 2.7 `components/surfaces/panel.{hx,cppx}` — `Panel`

`.hx`:
```cpp
enum class PanelVariant { Hero, Overlay, Sunken };
enum class PanelSize    { Auto, Sm, Md, Lg };   // Auto = variant's canonical width
struct PanelProps {
  const char *key = nullptr;
  PanelVariant variant = PanelVariant::Overlay;
  PanelSize size = PanelSize::Auto;
  ::ui::UiChildren children = {};
};
::ui::UiElement Panel(const PanelProps &props);
```
Adapts `ui::components::Box`. Variant → style + visual baseline:

| Variant | bg | border | border_width | width(Auto) | padding | gap | align_items | source |
|---|---|---|---|---|---|---|---|---|
| `Hero` | `{18,27,32,245}` | `{83,108,118,255}` | 1.0 | 340 | 24 all | 14 | Center | hx:94-105,160,15,17 |
| `Overlay` | `{17,24,30,255}` | `{78,96,108,255}` | 1.0 | 220 | 18 all | 12 | Center | hx:107-116,163,14,16 |
| `Sunken` | `{22,30,36,255}` | none (a=0) | 0.0 | 260 | 14 all | 10 | **Stretch** | loadout_screen.cpp:330-351 |

`PanelSize` width tiers: `Sm=220`, `Md=260`, `Lg=340` (= the three canonical widths).
CRITICAL: `Sunken` must use `align_items=Stretch` (the loadout details Box inherits the
`Style` default Stretch — `tree.h:241`), while Hero/Overlay use Center — variant-encoded,
never a prop. Visual via `tokens::panel_visual(bg, border)` (Hero/Overlay) and
`tokens::fill_visual(bg)` (Sunken, no border).

### 2.8 `components/text/screen_title.{hx,cppx}` — `ScreenTitle`

`.hx`:
```cpp
enum class ScreenTitleVariant : uint8_t { Hero, Screen, Dialog, Popup };
struct ScreenTitleProps {
  const char *key = nullptr;
  ScreenTitleVariant variant = ScreenTitleVariant::Screen;
  const char *value = "";
  float reserved_height_override = 0.0f;   // 0 => variant default (escape hatch)
};
::ui::UiElement ScreenTitle(const ScreenTitleProps &props);
```
Adapts `ui::components::Text` (color+font_size visual via `tokens::text_visual`,
align/line_height default). Variant → token baseline:

| variant | color | font | reserved height | source |
|---|---|---|---|---|
| `Hero` | `kTextHeroTitle {235,246,242,255}` | 30 | 36 | hx:118-121,166 |
| `Screen` | `kTextTitle {236,246,242,255}` | 26 | 32 | hx:124-128,169 |
| `Dialog` | `kTextTitle {236,246,242,255}` | 28 | 34 | hx:130-134,172 |
| `Popup` | `kTextDialogTitle {240,248,244,255}` | 22 | 24 | confirm_dialog.cpp:84 |

`reserved_height_override` is used by exactly one site — the loadout "Loadout" title
(`Screen` variant but reserved height **30**, not 32). Loadout title becomes
`ScreenTitle(variant=Screen, value="Loadout", reserved_height_override=30)`. (Loadout also
has its own thin `LoadoutTitle` wrapper — see §5 — which calls this with the override; the
title may equivalently be authored directly. Either is byte-identical.)

### 2.9 `components/text/screen_subtitle.{hx,cppx}` — `ScreenSubtitle`

`.hx` Props: `{ key, const char *value }`. Single style (no variant):
`kTextSubtitle {154,177,184,255}`, font 16, reserved height 22 (`hx:136-140,175`).
Adapts `ui::components::Text`.

### 2.10 `components/text/body_text.{hx,cppx}` — `BodyText`

`.hx`:
```cpp
enum class BodyTextVariant : uint8_t { Body, Strong, Message, Detail };
enum class BodyTextTone    : uint8_t { Default, Muted, Disabled };
struct BodyTextProps {
  const char *key = nullptr;
  BodyTextVariant variant = BodyTextVariant::Body;
  BodyTextTone tone = BodyTextTone::Default;
  const char *value = "";
  float reserved_height_override = 0.0f;
};
::ui::UiElement BodyText(const BodyTextProps &props);
```
Variant → font + default reserved height: `Body`=14/16, `Strong`=16/18, `Message`=15/18,
`Detail`=12/16. Tone → color: `Muted`=`kTextBodyMuted {202,218,216,255}`,
`Disabled`=`kTextWeaponDetailOff {142,148,150,255}`, `Default`=per-variant:
`Body`→`kTextBody {226,238,236,255}`, `Strong`→`kTextWeaponName {238,246,244,255}`,
`Message`→`kTextBodyMuted {202,218,216,255}`, `Detail`→`kTextWeaponDetail {184,204,204,255}`.

Exact call-site mapping (each reproduces a current raw `Text` byte-for-byte):

| current site | new call | resolves to (color,font,h) |
|---|---|---|
| weapon name `weapon_tile.cpp:87` | `BodyText{variant=Strong, value=weapon.name}` | {238,246,244,255},16,18 |
| weapon detail `weapon_tile.cpp:99-102` | `BodyText{variant=Detail, tone=disabled?Disabled:Default, value=detail}` | enabled {184,204,204,255} / disabled {142,148,150,255},12,16 |
| equip-slot label `equipment_slot.cpp:64` | `BodyText{variant=Body, value=slot_text}` | {226,238,236,255},14,16 |
| loadout summary `loadout_screen.cpp:365` | `BodyText{variant=Body, value=details, reserved_height_override=18}` | {226,238,236,255},14,**18** |
| "Equipment Slots" `loadout_screen.cpp:466` | `BodyText{variant=Body, tone=Muted, value="Equipment Slots"}` | {202,218,216,255},14,16 |
| confirm message `confirm_dialog.cpp:99` | `BodyText{variant=Message, value=message}` | {202,218,216,255},15,18 |

> Two near-white title/body colors that differ by 2-4/channel
> (`{236,246,242}`↔`{240,248,244}`, `{238,246,244}`↔`{226,238,236}`) are kept DISTINCT
> tokens (byte-exact). HUD text (`{224,238,236,255}`/18px) is NOT migrated to `BodyText`
> here — `HudBand` keeps its inline `tokens::text_visual` (kTextHud/kFontHud); flagged in §8.

---

## 3. Tokens / theme module design

### 3.1 `src/client/ui/components/tokens.h` (plain `.h`, `namespace shooter::tokens`)

Header-only: `constexpr` color/font/dimension constants + three `inline` `VisualStyle`
builders. Depends only on `ui/components/common.h` (for `::ui::Color`/`VisualStyle`/`Style`)
— legal `client/ui → ui`, no game vocab, satisfies `runtime_dependency_guard.py`. NOT in
`cppx_transpile`; flows in via `#include "client/ui/tokens.h"` (reachable because `src/` is
on the include path; note the include uses the `client/ui/...` root form for consistency
with generated headers, and resolves to the plain `src/client/ui/tokens.h`).

> Path note: `components/tokens.h` is included as `"client/ui/components/tokens.h"` (it sits
> in `src/client/ui/components/`). Use that exact path at all consumer sites.

Constants (all STRAIGHT alpha; IR premultiplies at emit):

```cpp
namespace shooter::tokens {
// ---- Surface backgrounds ----
constexpr ::ui::Color kSurfaceMenu         = {8, 14, 18, 255};
constexpr ::ui::Color kSurfaceGame         = {10, 16, 18, 255};
constexpr ::ui::Color kSurfaceOverlay      = {14, 22, 28, 245};
constexpr ::ui::Color kSurfacePanel        = {17, 24, 30, 255};
constexpr ::ui::Color kSurfaceHeroPanel    = {18, 27, 32, 245};
constexpr ::ui::Color kSurfaceHudBand      = {18, 24, 28, 245};   // hud_band.cpp:48
// ---- Borders ----
constexpr ::ui::Color kBorderPanel         = {78, 96, 108, 255};
constexpr ::ui::Color kBorderHeroPanel     = {83, 108, 118, 255};
constexpr ::ui::Color kBorderHudBand       = {82, 106, 118, 255}; // hud_band.cpp:48
// ---- Text ----
constexpr ::ui::Color kTextTitle           = {236, 246, 242, 255};
constexpr ::ui::Color kTextHeroTitle       = {235, 246, 242, 255};
constexpr ::ui::Color kTextSubtitle        = {154, 177, 184, 255};
constexpr ::ui::Color kTextDialogTitle     = {240, 248, 244, 255};
constexpr ::ui::Color kTextBody            = {226, 238, 236, 255};
constexpr ::ui::Color kTextBodyMuted       = {202, 218, 216, 255};
constexpr ::ui::Color kTextWeaponName      = {238, 246, 244, 255};
constexpr ::ui::Color kTextWeaponDetail    = {184, 204, 204, 255};
constexpr ::ui::Color kTextWeaponDetailOff = {142, 148, 150, 255};
constexpr ::ui::Color kTextHud             = {224, 238, 236, 255}; // hud_band.cpp:29

// ---- Font sizes (uint16_t) ----
constexpr uint16_t kFontHeroTitle   = 30;
constexpr uint16_t kFontScreenTitle = 26;
constexpr uint16_t kFontDialogTitle = 28;   // ScreenTitle::Dialog
constexpr uint16_t kFontPopupTitle  = 22;   // ScreenTitle::Popup (confirm dialog)
constexpr uint16_t kFontSubtitle    = 16;
constexpr uint16_t kFontHud         = 18;
constexpr uint16_t kFontStrong      = 16;   // weapon name
constexpr uint16_t kFontMessage     = 15;   // confirm message
constexpr uint16_t kFontBody        = 14;
constexpr uint16_t kFontDetail      = 12;

// ---- Border widths ----
constexpr float kBorderWidth         = 1.0f;
constexpr float kBorderWidthSelected = 2.0f;

// ---- Builders (verbatim semantics from screen_chrome.hx:27-50) ----
inline ::ui::VisualStyle fill_visual(::ui::Color background);
inline ::ui::VisualStyle panel_visual(::ui::Color bg, ::ui::Color border,
                                      float border_width = 1.0f);
inline ::ui::VisualStyle text_visual(::ui::Color color, uint16_t font_size);
} // namespace shooter::tokens
```

Spacing/dimension layout tokens (paddings/gaps/widths/heights) are NOT centralized here —
each component encodes its own geometry in its variant table (the shadcn "variants live in
the component" rule). This avoids dead tokens and keeps the header to palette + builders.
Border-width tokens are the one shared layout exception because `selected?2:1` recurs.

### 3.2 `src/client/ui/screens/loadout/loadout_tokens.h` (plain `.h`, `namespace shooter::loadout`)

Feature-local. Holds the loadout-only colors/sizes that no shared component consumes (root
dialog bg `{12,20,24,245}`, details bg `{22,30,36,255}`, confirm scrim `{0,0,0,160}`,
confirm panel `{18,26,32,255}`/`{92,116,126,255}`, tab/tile/slot/grid/details geometry,
the `selection_border_width(bool)` helper). It `#include "client/ui/components/tokens.h"`
and reuses `tokens::fill_visual/panel_visual/text_visual`. Shared text colors used by the
loadout (title `kTextTitle`, summary `kTextBody`, slots-heading `kTextBodyMuted`, tile/slot
text) come from `tokens::`; loadout-only surfaces are local. This honors co-location (the
loadout CLAUDE.md) without re-deriving the shared palette.

### 3.3 Variant→token resolution model

Each shared component owns its variant→token `switch` inside its own `.cppx` (or its
`*_variant.h` impl helper). No shared recipe layer, no surviving `theme::` shim. `src/ui`
controls (Button/Checkbox/Input/Dialog) continue to self-resolve their chrome from
`use_theme()`; `tokens::` governs ONLY app-authored surfaces (Box/Dialog backgrounds) and
all `Text` paint.

---

## 4. app_shell relocation (paths, includes, CMake)

Pure file-move + rename. Zero API/behavior change; all namespaces stay `client::ui` /
`client::ui::internal`. No `.hx`/`.cppx` involved.

### 4.1 Moves

| New path | From |
|---|---|
| `app_shell/client_ui.{h,cpp}` | `client_ui.{h,cpp}` |
| `app_shell/ui_pipeline.{h,cpp}` | `ui_pipeline.{h,cpp}` |
| `app_shell/deferred_ui_mutation.h` | `internal/deferred_ui_mutation.h` |
| `app_shell/app_shell_provider.{h,cpp}` | `providers/app_shell.{h,cpp}` (renamed) |
| `app_shell/navigation/screen_stack.{h,cpp}` | `navigation/screen_stack.{h,cpp}` |
| `app_shell/navigation/ui_screen.h` | `navigation/ui_screen.h` |
| `app_shell/callback_deps.h` | `callback_deps.h` |

> `callback_deps.h` is added to the move set (it lives at `src/client/ui/callback_deps.h`
> and is included by client_ui consumers + nav hooks). Moving it into `app_shell/` keeps the
> shell's helpers together. Its consumers' include paths update with the others. (If the
> integrator prefers to leave `callback_deps.h` at `src/client/ui/`, that is acceptable —
> it is framework glue with no game vocab; this plan moves it for cohesion but flags it as
> the one low-stakes optional move.)

### 4.2 Internal include rewrites (within moved files)

- `app_shell/client_ui.h`: `"navigation/screen_stack.h"` unchanged (co-moved sibling);
  all `"../../react.h"` → `"../../../react.h"`, all `"../../ui/..."` → `"../../../ui/..."`
  (one directory deeper).
- `app_shell/client_ui.cpp`: `"client_ui.h"` unchanged; `"internal/deferred_ui_mutation.h"`
  → `"deferred_ui_mutation.h"`; `"callback_deps.h"` unchanged (co-moved sibling).
- `app_shell/deferred_ui_mutation.h`: `"../client_ui.h"` → `"client_ui.h"`.
- `app_shell/ui_pipeline.h`: `"client_ui.h"` unchanged; `"../../ui/..."` → `"../../../ui/..."`.
- `app_shell/ui_pipeline.cpp`: `"ui_pipeline.h"` unchanged.
- `app_shell/navigation/screen_stack.h`: `"../../../ui/span.h"` → `"../../../../ui/span.h"`;
  `"ui_screen.h"` unchanged.
- `app_shell/navigation/ui_screen.h`: `"../../../ui/runtime/element.h"` →
  `"../../../../ui/runtime/element.h"`.
- `app_shell/navigation/screen_stack.cpp`: `"screen_stack.h"` unchanged.
- `app_shell/app_shell_provider.h`: `"../../../ui/runtime/element.h"` unchanged (providers/
  and app_shell/ are both 3 levels deep).
- `app_shell/app_shell_provider.cpp`: `"app_shell.h"` → `"app_shell_provider.h"`;
  `"../../../react.h"` unchanged.
- `app_shell/callback_deps.h`: depth unchanged (was 3 deep, now 3 deep) — verify and adjust
  any `"../../..."` includes if present (it is a small header; treat its relative includes
  the same as `app_shell_provider.h`).

### 4.3 External consumer include rewrites (exact)

A. `client_ui.h` (`client/ui/client_ui.h` → `client/ui/app_shell/client_ui.h`):
- `screens/in_game/in_game_screen.cppx:7`, `screens/pause/pause_screen.cppx:7`,
  `screens/main_menu/main_menu_screen.cppx:7`, `screens/options/options_screen.cppx:8`:
  `"client/ui/client_ui.h"` → `"client/ui/app_shell/client_ui.h"`.
- `screens/loadout/loadout_screen.cpp:9`: `"../../client_ui.h"` →
  `"../../app_shell/client_ui.h"`.
- `hooks/shooter_weapons.cpp:6`: `"../client_ui.h"` → `"../app_shell/client_ui.h"`.
- `tests/client_ui_tests.cpp:1`, `tests/shooter_ui_tests.cpp:1`:
  `"client/ui/client_ui.h"` → `"client/ui/app_shell/client_ui.h"`.

B. `ui_pipeline.h`:
- `src/app/app.h:3`, `src/app/game_loop.h:3`, `src/platform/control_mailbox.h:10`:
  `"../client/ui/ui_pipeline.h"` → `"../client/ui/app_shell/ui_pipeline.h"`.
- `tests/shooter_ui_tests.cpp:10`, `tests/ui_pipeline_tests.cpp:1`:
  `"client/ui/ui_pipeline.h"` → `"client/ui/app_shell/ui_pipeline.h"`.

C. `navigation/ui_screen.h` → `app_shell/navigation/ui_screen.h`:
- `screens/in_game/in_game_screen.hx:5`, `screens/pause/pause_screen.hx:5`,
  `screens/main_menu/main_menu_screen.hx:5`, `screens/options/options_screen.hx:5`:
  `"client/ui/navigation/ui_screen.h"` → `"client/ui/app_shell/navigation/ui_screen.h"`.
- `screens/loadout/loadout_screen.h:5`: `"../../navigation/ui_screen.h"` →
  `"../../app_shell/navigation/ui_screen.h"`.
- `tests/ui_pipeline_tests.cpp:3`: `"client/ui/navigation/ui_screen.h"` →
  `"client/ui/app_shell/navigation/ui_screen.h"`.

D. `providers/app_shell.h` → `app_shell/app_shell_provider.h`:
- `src/app/app.cpp:6`: `"../client/ui/providers/app_shell.h"` →
  `"../client/ui/app_shell/app_shell_provider.h"`.
- `screens/main_menu/main_menu_screen.cppx:9`: `"client/ui/providers/app_shell.h"` →
  `"client/ui/app_shell/app_shell_provider.h"`.
- `tests/shooter_ui_tests.cpp:2`: `"client/ui/providers/app_shell.h"` →
  `"client/ui/app_shell/app_shell_provider.h"`.

E. `internal/deferred_ui_mutation.h` → `app_shell/deferred_ui_mutation.h`:
- `screens/in_game/in_game_screen.cppx:10`: `"client/ui/internal/deferred_ui_mutation.h"`
  → `"client/ui/app_shell/deferred_ui_mutation.h"`.
- `screens/loadout/loadout_state.cpp:5`: `"../../internal/deferred_ui_mutation.h"` →
  `"../../app_shell/deferred_ui_mutation.h"`.
- `hooks/shooter_weapons.cpp:7`: `"../internal/deferred_ui_mutation.h"` →
  `"../app_shell/deferred_ui_mutation.h"`.

F. `callback_deps.h` → `app_shell/callback_deps.h` (consumers; verify exact lines during
   integration — `main_menu_screen.cppx:6`, `pause_screen.cppx:6`, `options_screen.cppx:7`,
   `in_game_screen.cppx:6`, `loadout_screen.cpp:8`, plus the new `*_actions.cpp`). Each
   rooted form `"client/ui/callback_deps.h"` → `"client/ui/app_shell/callback_deps.h"`;
   each relative form re-pointed by depth.

G. `providers/shooter_provider.h` — NOT moved. No include changes
   (`app.cpp:7`, `in_game_screen.cppx:11`, `loadout_screen.cpp:12`, `equipment_slot.cpp:10`,
   `confirm_dialog.cpp:11`, `weapon_tile.cpp:10`, `shooter_hud.cpp:3`,
   `shooter_weapons.cpp:8`, `tests/shooter_ui_tests.cpp:3` all unchanged).

### 4.4 CMakeLists.txt edits for app_shell (per target, line ranges)

`.h`-only files are not listed in CMake; only `.cpp` paths move. The
`cppx_transpile(CLIENT_UI_GENERATED ...)` block (82-93) is unchanged BY THE app_shell move
(no `.cppx`/`.hx` move here). Edits:

- `hello` target (176-217):
  - L181 `src/client/ui/client_ui.cpp` → `src/client/ui/app_shell/client_ui.cpp`
  - L185 `src/client/ui/providers/app_shell.cpp` → `src/client/ui/app_shell/app_shell_provider.cpp`
  - L187 `src/client/ui/navigation/screen_stack.cpp` → `src/client/ui/app_shell/navigation/screen_stack.cpp`
  - L188 `src/client/ui/ui_pipeline.cpp` → `src/client/ui/app_shell/ui_pipeline.cpp`
  - L186 `src/client/ui/providers/shooter_provider.cpp` UNCHANGED
- `client_ui_tests` target (557-570):
  - L560 `client_ui.cpp` → `app_shell/client_ui.cpp`
  - L561 `navigation/screen_stack.cpp` → `app_shell/navigation/screen_stack.cpp`
- `ui_pipeline_tests` target (587-601):
  - L590 `client_ui.cpp` → `app_shell/client_ui.cpp`
  - L591 `navigation/screen_stack.cpp` → `app_shell/navigation/screen_stack.cpp`
  - L592 `ui_pipeline.cpp` → `app_shell/ui_pipeline.cpp`
- `shooter_ui_tests` target (618-646):
  - L621 `client_ui.cpp` → `app_shell/client_ui.cpp`
  - L625 `providers/app_shell.cpp` → `app_shell/app_shell_provider.cpp`
  - L627 `navigation/screen_stack.cpp` → `app_shell/navigation/screen_stack.cpp`
  - L628 `ui_pipeline.cpp` → `app_shell/ui_pipeline.cpp`
  - L626 `providers/shooter_provider.cpp` UNCHANGED

`add_dependencies(... cppx_generated_client_ui)` (228, 657) unchanged. Include dirs
(`target_include_directories ... src`) unchanged (all rooted/relative includes resolve from
`src`).

---

## 5. Per-screen target composition

`debug_name()` strings live on the `UiScreen` subclasses — UNTOUCHED everywhere. Each
screen `.hx` loses its nav-hook decl (moved to `<screen>_actions.h`) and gains
`#include "<screen>_actions.h"` indirectly via its `.cppx`. The `screen_entry_key` helper,
`build_element`/`component(...)` display-name registration, and the `if(!is_top) return
::ui::empty();` guard are byte-identical.

### 5.1 main_menu — `MainMenuScreen` ("MainMenu")

Preserved IDs (order frozen): `StartMatchButton`, `OpenOptionsFromMainMenuButton`,
`QuitButton`. New `main_menu/main_menu_actions.{h,cpp}` holds `use_exit_to_main_menu()`
(constructs `MainMenuScreen`; consumed by pause). `.cppx` includes `in_game/in_game_actions.h`
(`use_start_match`), `options/options_actions.h`, and `app_shell/app_shell_provider.h`
(`use_request_quit`).

```cpp
return <ScreenLayout key="root" variant={ScreenLayoutVariant::Menu}>
  <Panel key="panel" variant={PanelVariant::Hero}>
    <ScreenTitle key="title" variant={ScreenTitleVariant::Hero} value="Reference Shooter" />
    <ScreenSubtitle key="subtitle" value="SDL3 / retained UI flow" />
    <AppButton key="start"   controlId="StartMatchButton"             label="Start Match" onPress={start_match} />
    <AppButton key="options" controlId="OpenOptionsFromMainMenuButton" label="Options"     onPress={open_options} />
    <AppButton key="quit"    controlId="QuitButton" disabled={!request_quit} label="Quit"  onPress={request_quit} />
  </Panel>
</ScreenLayout>
```
`onPress={request_quit}` — AppButton null-checks internally; `disabled={!request_quit}`
preserved.

### 5.2 pause — `PauseScreen : OverlayScreen` ("Pause")

Preserved IDs (order frozen): `ResumeButton`, `OpenOptionsFromPauseButton`,
`OpenLoadoutFromPauseButton`, `ExitToMainMenuButton`. New `pause/pause_actions.{h,cpp}`
holds `use_push_pause_screen()` (consumed by in_game). `.cppx` includes
`options/options_actions.h`, `loadout/loadout_actions.h`, `main_menu/main_menu_actions.h`.

```cpp
return <ScreenLayout key="root" variant={ScreenLayoutVariant::CenteredOverlay}>
  <Panel key="panel" variant={PanelVariant::Overlay}>
    <ScreenTitle key="title" variant={ScreenTitleVariant::Dialog} value="Paused" />
    <AppButton key="resume"       controlId="ResumeButton"               label="Resume"       onPress={nav.pop_current} />
    <AppButton key="options"      controlId="OpenOptionsFromPauseButton"  label="Options"      onPress={open_options} />
    <AppButton key="loadout"      controlId="OpenLoadoutFromPauseButton"  label="Loadout"      onPress={open_loadout} />
    <AppButton key="exit-to-menu" controlId="ExitToMainMenuButton"        label="Exit To Menu" onPress={exit_to_main_menu} />
  </Panel>
</ScreenLayout>
```

### 5.3 options — `OptionsScreen : OverlayScreen` ("Options")

Preserved IDs (order frozen): `LargeHudToggle`, `ReducedMotionToggle`,
`BackFromOptionsButton`, `NameInput` (tree order: checkbox, checkbox, button, input). State
(`use_state_int`/`use_state<std::string>("Ace")`) and the
`if(!is_top || !player_name) return empty();` guard verbatim. New
`options/options_actions.{h,cpp}` holds `use_push_options_screen()` (consumed by
main_menu+pause). Uses semantic `AppCheckbox`/`AppInput`.

```cpp
return <ScreenLayout key="root" variant={ScreenLayoutVariant::Overlay}>
  <ScreenTitle key="title" variant={ScreenTitleVariant::Screen} value="Options" />
  <AppCheckbox key="large-hud"      controlId="LargeHudToggle"     label="Large HUD"
    checked={large_hud && *large_hud != 0}
    onChange={[large_hud](bool e){ if (large_hud) *large_hud = e ? 1 : 0; }} />
  <AppCheckbox key="reduced-motion" controlId="ReducedMotionToggle" label="Reduce Motion"
    checked={reduced_motion && *reduced_motion != 0}
    onChange={[reduced_motion](bool e){ if (reduced_motion) *reduced_motion = e ? 1 : 0; }} />
  <AppButton key="back" controlId="BackFromOptionsButton" label="Back" onPress={nav.pop_current} />
  <AppInput key="name" controlId="NameInput" accessibilityLabel="Name"
    value={player_name->c_str()}
    onChange={[player_name](const std::string &v){ if (player_name) *player_name = v; }} />
</ScreenLayout>
```

### 5.4 in_game — `ShooterGameScreen` ("ShooterGame")

Preserved IDs (order frozen): `OpenPauseButton`, `OpenLoadoutButton` (inside ActionRow,
after HudBand). New `in_game/in_game_actions.{h,cpp}` holds `use_start_match()` (verbatim
body: `use_shooter_game()` + `use_deferred_ui_mutations()`; queues `game->reset()`;
`nav.reset_to(ShooterGameScreen)`; deps unchanged; consumed by main_menu). `.cppx` includes
`pause/pause_actions.h`, `loadout/loadout_actions.h`, plus `components/hud_band.h`.

```cpp
return <ScreenLayout key="root" variant={ScreenLayoutVariant::Game}>
  <HudBand />
  <ActionRow key="actions">
    <AppButton key="pause"   controlId="OpenPauseButton"   label="Pause"   onPress={open_pause} />
    <AppButton key="loadout" controlId="OpenLoadoutButton" label="Loadout" onPress={open_loadout} />
  </ActionRow>
</ScreenLayout>
```

### 5.5 loadout — full decomposition

`loadout_screen.cpp` is trimmed to: `LoadoutScreen` class, `LoadoutScreenView` (the
`use_state` slots + `use_loadout_context_value` + `LoadoutProvider` wrap — STAYS plain C++,
no JSX), `screen_entry_key`. It calls
`::ui::component("LoadoutScreenBody", LoadoutScreenBodyProps{}, LoadoutScreenBody)` verbatim
(display name "LoadoutScreenBody" preserved → fiber identity unchanged). `weapon_grid_children`
and the monolith `LoadoutScreenBody` body MOVE to `components/loadout_content.cppx`.
`use_push_loadout_screen()` MOVES to `loadout/loadout_actions.{h,cpp}`;
`loadout_screen.h` re-includes `loadout_actions.h` so `pause_screen` (which includes
`loadout_screen.h` → calls `use_push_loadout_screen`) keeps compiling without editing pause.

`loadout_content.cppx` — `LoadoutScreenBody`: keeps ALL current hook reads verbatim
(`use_screen_is_top`, `use_screen_navigator`, `use_shooter_weapon_count`,
`use_selected_weapon_tile`, `use_state<int>(LOADOUT_TAB_WEAPONS)`, seed clamping,
`use_weapon_read`, `can_buy/can_equip`, two `use_select_weapon`, `use_set_selected_weapon_tile`,
`use_set_pending_loadout_action`, `use_pending_loadout_action`, `use_text_storage`,
`use_compare_enabled`, `use_set_compare_enabled`), the early `empty()` guards, builds the
closures, then returns:

```cpp
bool confirm_open = pending.action != LOADOUT_ACTION_NONE;
return ::ui::fragment(::ui::children({
  <LoadoutConfirmDialog />,
  <LoadoutScreenFrame key="root" confirmOpen={confirm_open}>
    <LoadoutTitle key="title" value="Loadout" />
    <LoadoutTabs key="tabs">
      <LoadoutTabs.Tab key="weapons" controlId="WeaponsTab" label="Weapons" onSelect={weapons_on_select} />
      <LoadoutTabs.Tab key="gear"    controlId="GearTab"    label="Gear"    onSelect={gear_on_select} />
    </LoadoutTabs>
    <LoadoutBody key="body">
      <LoadoutWeaponGrid key="weapon-grid" activeTab={*active_tab} />
      <LoadoutDetails key="details"
        summary={details} canBuy={can_buy} canEquip={can_equip}
        compareEnabled={compare_enabled} selectedIndex={selected_index_seed}
        onCompareChange={set_compare_enabled} onBuy={on_buy} onEquip={on_equip} onBack={on_back} />
    </LoadoutBody>
  </LoadoutScreenFrame>,
}));
```

> CRITICAL ordering: `LoadoutConfirmDialog` is fragment child #1, `LoadoutScreenFrame`
> (root) is #2 — matching today's `fragment(children({ LoadoutConfirmDialog, Dialog "root"
> }))`. This + `modal=!confirm_open` produces `previous_focus_before_modal`. Must not flip.
> Use the EXPLICIT `::ui::fragment(::ui::children({ <A/>, <B>...</B> }))` form — the
> transpiler has no `<>` fragment syntax (confirmed: no empty-tag handling). JSX tags nest
> inside a `children({...})` arg (confirmed against `screen_chrome.cppx` + the
> collect-jsx-source line scanner).

`weapons_on_select`/`gear_on_select` reproduce the current tab `on_activate` bodies
verbatim: `*active_tab = TAB; set_selected_tile(first_weapon_for_tab(TAB)); select_*_tab_weapon();`
— write #1 (`*active_tab`) is synchronous local-state; #2/#3 are the two deferred writes →
GearTab still queues exactly 2. `on_buy`/`on_equip`/`on_back` reproduce the current pending
payload + `nav.pop_current` closures.

New loadout component files (one export each):

| File | Component | Adapts | Reproduces |
|---|---|---|---|
| `loadout_screen_frame.{hx,cppx}` | `LoadoutScreenFrame {key, bool confirm_open, children}` | `Dialog` (`modal=!confirm_open`) | root Dialog: 100/100, pad 24, gap 18, `fill_visual(loadout::kRootBg {12,20,24,245})` (loadout_screen.cpp:161-172) |
| `loadout_title.{hx,cppx}` | `LoadoutTitle {key, value}` | `Text` | height 30, `text_visual(kTextTitle, kFontScreenTitle)` (lines 176-188) — equivalently `ScreenTitle{Screen, reserved_height_override=30}` |
| `loadout_tabs.{hx,cppx}` | `LoadoutTabs {key, children}` (Box: Row/Start/gap10) + `LoadoutTabs::Tab {key, control_id, label, on_select}` (adapts `Button`: role=Tab, 132×34 pad{12,12,7,7}, wraps on_select into on_activate) | tabs row + WeaponsTab/GearTab (lines 189-301) |
| `loadout_body.{hx,cppx}` | `LoadoutBody {key, children}` | `Box` (Row/Start/gap18) | body row (lines 302-313) |
| `loadout_weapon_grid.{hx,cppx}` | `LoadoutWeaponGrid {key, int active_tab}` | `Box` (width 400, gap 10) + rows | grid + keyed rows weapon-row-0/1 / gear-row + WeaponTile@index (lines 316-329 + weapon_grid_children) |
| `loadout_details.{hx,cppx}` | `LoadoutDetails {key, summary, can_buy, can_equip, compare_enabled, selected_index, on_compare_change, on_buy, on_equip, on_back}` | `Panel variant=Sunken` (or local Box) + ordered children | details panel + 8 ordered children (lines 330-494) |
| `weapon_tile.{hx,cppx}` | `WeaponTile {key, int index}` (+ tab constants/helpers) | `AppButton` (advanced_layout) or `Button` | id=WEAPON_TILE_CONTROL_ID, id_offset=index, autofocus=selected, on_focus/on_activate, 190×78 pad10 gap5, border selected?2:1, name+detail BodyText children (weapon_tile.cpp) |
| `equipment_slot.{hx,cppx}` | `EquipmentSlot {key, id, label, int weapon_index}` | `AppButton` (advanced_layout) or `Button` | key=id, id, autofocus=selected, on_focus/on_activate, 232×38 pad{10,10,8,8}, border selected?2:1, label child (equipment_slot.cpp) |
| `confirm_dialog.{hx,cppx}` | `LoadoutConfirmDialog {unused}` (gate, reads pending, keyed body `confirm_body_key(generation)`) | `Dialog`(scrim)+`Box`(panel)+`Text`×2+`Box`(actions)+`Button`×2 | scrim/panel/title/message/actions + ConfirmLoadoutActionButton (autofocus) + CancelLoadoutActionButton (confirm_dialog.cpp) |

`LoadoutDetails` ordered children (frozen): summary `BodyText{Body, reserved_height_override=18}`,
`AppCheckbox controlId="CompareToggle"`, `AppButton controlId="BuyWeaponButton" disabled={!can_buy}`,
`AppButton controlId="EquipWeaponButton" disabled={!can_equip}`,
`AppButton controlId="BackFromLoadoutButton"`, slots-title `BodyText{Body, tone=Muted}`,
`EquipmentSlot id="PrimarySlot" weaponIndex=0`, `EquipmentSlot id="GearSlot" weaponIndex=3`.

`LoadoutTabs::Tab` lowering (transpiler): `<LoadoutTabs.Tab .../>` →
`::ui::component("LoadoutTabs.Tab", LoadoutTabs::TabProps{...}, LoadoutTabs::Tab)`. Provide
exactly: a `struct LoadoutTabs` with nested `struct TabProps` and
`static ::ui::UiElement Tab(const TabProps&)`, plus a free `LoadoutTabsProps` +
`::ui::UiElement LoadoutTabs(const LoadoutTabsProps&)`. (If the nested-struct form fights the
transpiler during integration, fall back to a free `LoadoutTab` component — contract-neutral,
same nodes.)

`WeaponTile`/`EquipmentSlot` invoked as JSX tags from grid/details `.cppx` lower to
`::ui::component("WeaponTile", WeaponTileProps{...}, WeaponTile)` — identical display name +
key to today's manual registration, so fiber identity is preserved.

`loadout_state.{h,cpp}` is UNCHANGED in behavior (the setters that produce the deferred
writes stay exactly as in `loadout_state.cpp`; only an include path edits for
`deferred_ui_mutation.h`).

---

## 6. CONTRACT PRESERVATION MATRIX

Screen `debug_name()` (each on the `UiScreen` subclass `.hx`/`.h`, untouched):

| debug_name | class | file | status |
|---|---|---|---|
| "MainMenu" | MainMenuScreen | main_menu_screen.hx:13 | unchanged |
| "ShooterGame" | ShooterGameScreen | in_game_screen.hx | unchanged |
| "Options" | OptionsScreen | options_screen.hx | unchanged |
| "Pause" | PauseScreen | pause_screen.hx | unchanged |
| "Loadout" | LoadoutScreen | loadout_screen.h:13 | unchanged |

Control IDs + offsets (produced where; `id`/`id_offset` flow to `node.control_id`/`offset`):

| Control ID | offset | new producer | notes |
|---|---|---|---|
| StartMatchButton | 0 | main_menu `<AppButton controlId=...>` | |
| OpenOptionsFromMainMenuButton | 0 | main_menu AppButton | |
| QuitButton | 0 | main_menu AppButton (disabled={!request_quit}) | |
| ResumeButton | 0 | pause AppButton (onPress=nav.pop_current) | |
| OpenOptionsFromPauseButton | 0 | pause AppButton | |
| OpenLoadoutFromPauseButton | 0 | pause AppButton | |
| ExitToMainMenuButton | 0 | pause AppButton | |
| OpenPauseButton | 0 | in_game AppButton (smoke-test ID, not in brief) | |
| OpenLoadoutButton | 0 | in_game AppButton (smoke-test ID) | |
| BackFromOptionsButton | 0 | options AppButton (smoke ID) | |
| LargeHudToggle | 0 | options AppCheckbox (smoke ID) | |
| ReducedMotionToggle | 0 | options AppCheckbox (smoke ID) | |
| NameInput | 0 | options AppInput (smoke + C++ ID, accessibilityLabel="Name") | |
| WeaponsTab | 0 | loadout `LoadoutTabs::Tab` (role=Tab, size Sm) | |
| GearTab | 0 | loadout `LoadoutTabs::Tab` (role=Tab, size Sm) | |
| PrimarySlot | 0 | loadout `EquipmentSlot` (key=id, autofocus=selected) | |
| GearSlot | 0 | loadout `EquipmentSlot` | |
| BuyWeaponButton | 0 | loadout `LoadoutDetails` AppButton (disabled={!can_buy}) | |
| EquipWeaponButton | 0 | loadout `LoadoutDetails` AppButton (disabled={!can_equip}) | |
| BackFromLoadoutButton | 0 | loadout `LoadoutDetails` AppButton | |
| CompareToggle | 0 | loadout `LoadoutDetails` AppCheckbox | NOT a button |
| ConfirmLoadoutActionButton | 0 | loadout `confirm_dialog.cppx` Button (autofocus=true) | full literal replaces split string |
| CancelLoadoutActionButton | 0 | loadout `confirm_dialog.cppx` Button | |
| WEAPON_TILE_CONTROL_ID ("WeaponTile") | 0..3 | loadout `WeaponTile` via grid, control_offset=index | shared id, 4 offsets |

Loadout focus/modal/deferred-write semantics:

| Semantic | Test assertion | Preserved by |
|---|---|---|
| Default focus = weapon tile 0 on open | shooter_ui_tests:317-318 | WeaponTile index 0 `selected==true` → `autofocus`/`default_focused` → `initial_focus` |
| Focus right tile0→1→2 then down→Buy | :321-332 | Unchanged grid row keys (weapon-row-0/1), tile order, Button focusability; LoadoutDetails order |
| Focus on tile is pure-UI (no game write) | :324-327 | on_focus calls only `set_selected` (UI write); game `select` only on activate |
| Buy opens modal, focus→Confirm, previous_focus_before_modal==buy_id | :339-341 | `LoadoutScreenFrame modal=!confirm_open` toggle + fragment order (ConfirmDialog #1, root #2) + Confirm `autofocus=true` |
| Confirm dialog default focus = Confirm | :357-358 | ConfirmLoadoutActionButton `autofocus=true` |
| GearTab select queues EXACTLY 2 pending writes | :444-448 | `gear_on_select` runs `set_selected_tile` (#2 deferred) + `select_gear_tab_weapon` (#3 deferred); `*active_tab` (#1) synchronous |
| Pointer-hit centers GearTab/PrimarySlot/GearSlot | :379-396 | Focusable node remains the Button with the id, sized 132×34 / 232×38 |
| react_error_count()==0 | :421 | No new hooks added in adapters (AppButton/Panel/etc. call no hooks); LoadoutProvider/state unchanged |

`previous_focus_before_modal`, modal flags, and `react_error_count` are produced by the
shell (`app_shell/`) + screen `build_ui` + `ScreenStack` ordering — all moved byte-for-byte
or composed with identical retained nodes.

---

## 7. ORDERED IMPLEMENTATION PHASES

Designed for a human-in-the-loop integrator who owns CMake + builds. "PARALLEL (disjoint
new files)" steps create only NEW files that nothing yet includes — safe to fan out to
multiple authors; they do not build on their own but cause no breakage. "SEQUENTIAL
integration" steps wire CMake, edit existing files, and must build+test green at the stated
gate.

### Phase 0 — Baseline gate (sequential)
`./build.sh --tests` green on `feat/styling-render-system`. Record the baseline. Nothing
changes yet.

### Phase 1 — Author shared tokens + components (PARALLEL, disjoint new files)
No CMake, no existing-file edits. Safe to author concurrently:
- 1a `components/tokens.h`
- 1b `components/actions/{app_button_variant.h, app_button.hx/.cppx, app_checkbox.hx/.cppx,
  app_input.hx/.cppx, action_row.hx/.cppx, actions.h}`
- 1c `components/layout/{screen_layout.hx/.cppx, layout.h}`
- 1d `components/surfaces/{panel.hx/.cppx, surfaces.h}`
- 1e `components/text/{screen_title.hx/.cppx, screen_subtitle.hx/.cppx, body_text.hx/.cppx,
  text.h}`
All consume `tokens.h`. They reference only `ui/components/*` + `tokens.h` (legal layering).
Not yet referenced by any screen, not yet in CMake → no build impact.
**Gate: none (these don't compile in isolation until Phase 2 wires them).**

### Phase 2 — Wire shared components into CMake + transpile-verify (SEQUENTIAL)
Append to `cppx_transpile(CLIENT_UI_GENERATED ...)` (CMakeLists 82-93) the new `.cppx`/`.hx`
pairs from 1b/1c/1d/1e (12 `.cppx` + 12 `.hx`). The plain `.h` files (`tokens.h`,
`app_button_variant.h`, all `*.h` umbrellas) need no CMake entry (header-only, pulled
transitively). `${CLIENT_UI_GENERATED}` already flows into `hello` + `shooter_ui_tests`
(+ via `target_use_ui_components`). Add a throwaway translation unit OR temporarily
reference one component to force compilation, OR rely on the next phase's screen edits to
exercise them. Simplest: proceed to Phase 3 which references them.
**Gate: `./build.sh` configures; the new generated headers appear under
`generated/cppx/src/client/ui/components/...`. Build need not be green until Phase 3 (the
components are unreferenced).**

> Integrator note: keep `screen_chrome.{hx,cppx}` listed and intact through Phases 2-5; it
> is still consumed by un-migrated screens/loadout. Do NOT delete it yet.

### Phase 3 — Migrate the four simple screens (SEQUENTIAL, one screen per build)
For each of main_menu, in_game, options, pause (in this order; each is independent so order
is flexible, but build+test after EACH):
1. Edit `<screen>_screen.cppx` to the §5 semantic composition; swap
   `#include "client/ui/components/screen_chrome.h"` → the needed family umbrellas
   (`layout/layout.h`, `surfaces/surfaces.h`, `text/text.h`, `actions/actions.h`).
2. `on_activate=[](const ActivationEvent&){...}` lambdas become `onPress={[]{...}}`
   (drop the arg).
**Gate after each screen: `./build.sh --tests` green** (shooter_ui_tests + ui_cli_commands
exercise these screens). screen_chrome still compiles (loadout + any not-yet-migrated screen
still use it).

### Phase 4 — Author loadout new files (PARALLEL, disjoint new files)
No CMake, no existing-file edits yet:
- 4a `screens/loadout/loadout_tokens.h`
- 4b `screens/loadout/loadout_actions.{h,cpp}` (move body of `use_push_loadout_screen`)
- 4c `screens/loadout/components/{loadout_screen_frame, loadout_title, loadout_tabs,
  loadout_body, loadout_weapon_grid, loadout_details, loadout_content}.{hx,cppx}`
- 4d converted `screens/loadout/components/{weapon_tile, equipment_slot, confirm_dialog}.{hx,cppx}`
These reference `tokens.h`, `loadout_tokens.h`, the shared components (Phase 1/2), and the
loadout hooks. Not yet in CMake / not yet referenced by `loadout_screen.cpp`.

### Phase 5 — Integrate loadout (SEQUENTIAL)
1. CMake: append the loadout `.cppx`/`.hx` (4c+4d, 10 `.cppx` + 10 `.hx`) to
   `cppx_transpile(CLIENT_UI_GENERATED ...)`. REMOVE the now-converted plain sources from
   the `hello` (189-191) + `shooter_ui_tests` (629-631) source lists:
   `screens/loadout/components/confirm_dialog.cpp`, `equipment_slot.cpp`, `weapon_tile.cpp`
   (they become generated outputs, flowing via `${CLIENT_UI_GENERATED}`). ADD
   `screens/loadout/loadout_actions.cpp` to both lists (near `loadout_state.cpp`).
   `loadout_screen.cpp` + `loadout_state.cpp` stay listed.
2. Edit `loadout_screen.cpp`: trim to `LoadoutScreen` + `LoadoutScreenView` +
   `screen_entry_key`; remove `weapon_grid_children` + the monolith `LoadoutScreenBody`
   (now in `loadout_content.cppx`) + `use_push_loadout_screen` (now in `loadout_actions.cpp`);
   swap `#include "client/ui/components/screen_chrome.h"` → loadout component umbrellas +
   `loadout_tokens.h`. Make `loadout_screen.h` `#include "loadout_actions.h"` so
   `pause_screen` keeps resolving `use_push_loadout_screen` with no pause edit.
3. The `loadout_content.cppx` body must keep the fragment ordering + closures exactly (§5.5).
**Gate: `./build.sh --tests` green** — especially shooter_ui_tests loadout cases
(focus order, modal/previous_focus, 2-write count) + ui_cli_commands loadout flow.

### Phase 6 — Migrate HudBand off screen_chrome (SEQUENTIAL, small)
Edit `components/hud_band.cpp`: `#include "client/ui/components/screen_chrome.h"` →
`#include "client/ui/components/tokens.h"`; change `theme::text_visual`/`theme::panel_visual`
→ `tokens::text_visual`/`tokens::panel_visual` with `tokens::kTextHud`/`kFontHud`/
`kSurfaceHudBand`/`kBorderHudBand`. Values identical.
**Gate: `./build.sh --tests` green.**

### Phase 7 — Delete screen_chrome (SEQUENTIAL)
Once NO file includes `client/ui/components/screen_chrome.h` (verify with a repo grep),
delete `screen_chrome.{hx,cppx}` and remove their two lines from
`cppx_transpile(CLIENT_UI_GENERATED ...)` (83-84).
**Gate: `./build.sh --tests` green; `grep -r screen_chrome src tests` returns nothing.**

### Phase 8 — app_shell relocation (SEQUENTIAL, independent of 1-7; can run before or after)
This is a pure move and is orthogonal to the component migration. Recommended LAST to avoid
churning include paths while screens are being rewritten, but it can equally be Phase 1.5.
1. Move the 7 file groups (§4.1); rewrite internal includes (§4.2).
2. Rewrite all external consumer includes (§4.3 A-F); `shooter_provider` untouched (G).
3. Apply the CMake source-path edits (§4.4).
4. Update docs: `src/client/ui/CLAUDE.md`, `src/CLAUDE.md:37`, `architecture.md:75` prose;
   `screens/loadout/CLAUDE.md:7` (point at `loadout_actions.*`).
**Gate: `./build.sh --tests` green; runtime_dependency_guard passes.**

> Build-green invariant between phases: after Phases 0, 3 (each screen), 5, 6, 7, 8 the tree
> MUST be green. Phases 1 and 4 author unreferenced files (no gate). Phase 2 only adds
> transpile rules (configure-clean). The `<screen>_actions.*` extraction (Phase 3/5) and the
> app_shell move (Phase 8) are the two steps that touch many existing includes — do each as
> one atomic commit so a failed build is easy to bisect.

### What can be fanned out vs must be serial — summary
- PARALLEL safe (new disjoint files): all of Phase 1 (5 author streams: tokens+actions /
  layout / surfaces / text), all of Phase 4 (loadout authoring). Different authors can own
  actions vs layout vs surfaces vs text vs loadout concurrently because the files don't
  include each other (only `tokens.h`, which is authored first/standalone).
- SERIAL (shared CMake + existing-file edits + build gates): Phase 2 (CMake), Phase 3
  (screen rewrites — each its own build), Phase 5 (loadout CMake + loadout_screen.cpp +
  source-list removals), Phase 6 (hud_band), Phase 7 (delete), Phase 8 (app_shell move).
  CMake is a single shared file → all CMake edits are serialized through the integrator.

---

## 8. Risks + mitigations

1. **Transpiler dotted-tag (`LoadoutTabs::Tab`) ergonomics.** Risk: the nested-struct +
   static-method shape may be awkward or hit a transpiler edge. Mitigation: the lowering is
   confirmed (`cppx_transpile.py:74-83` joins dotted parts with `::` for both the props type
   and the callee). Fallback: a free `LoadoutTab` component (`<LoadoutTab/>`), contract-neutral
   (same nodes/order/IDs). Decide at Phase 4; either passes the contract.
2. **JSX fragment syntax.** Risk: authoring `<>...</>`. Mitigation: the transpiler has NO
   empty-tag rule — author the loadout body as explicit
   `::ui::fragment(::ui::children({ <LoadoutConfirmDialog/>, <LoadoutScreenFrame ...>...
   </LoadoutScreenFrame> }))`. Confirmed JSX nests inside `children({...})`.
3. **Fragment child ORDER / modal flag.** Risk: flipping ConfirmDialog vs root, or losing
   `modal=!confirm_open`, breaks `previous_focus_before_modal`. Mitigation: §5.5 pins the
   order and the `LoadoutScreenFrame` owns the modal toggle; the contract matrix (§6) calls
   it out; shooter_ui_tests:339-341 gates it at Phase 5.
4. **GearTab 2-write count.** Risk: moving the tab into `LoadoutTabs::Tab` could drop or add
   a deferred write. Mitigation: the Tab only wraps `on_select`→`on_activate`; the 3-action
   closure (1 sync + 2 deferred) is built in the screen body and passed in unchanged.
   shooter_ui_tests:444-448 gates it.
5. **Default focus via positional first-focusable.** Risk: simple screens rely on tree order
   + focusability, not `autofocus`. Mitigation: AppButton forces `focusable=true` (Button
   default), order is preserved 1:1, no `default_focused` set on simple screens. Gated by
   shooter_ui_tests main-menu down/down/confirm → Quit and shooter confirm → first button.
6. **AppButton variant paint is inert.** Risk: someone expects `Danger`/`Ghost` to repaint.
   Mitigation: documented as no-op baseline; activating per-variant paint needs an additive
   `ui::components::Button` change (optional `StylePatch variant` layered after the theme
   base in `resolve`) — DEFERRED, out of this behavior-preserving slice. Flagged for review.
7. **`react_error_count()` perturbation.** Risk: a new component that calls hooks could
   shift sibling hook IDs. Mitigation: all new shared adapters (AppButton/AppCheckbox/AppInput/
   ActionRow/ScreenLayout/Panel/ScreenTitle/ScreenSubtitle/BodyText) call NO hooks — they are
   pure element factories returning one primitive element, adding zero hook slots. Loadout
   components that DO read hooks (WeaponTile/EquipmentSlot/confirm body/LoadoutContent) keep
   the EXACT same hook reads in the same order as today. Gated by shooter_ui_tests:421.
8. **Byte-exact paint vs near-duplicate colors.** Risk: collapsing
   `{236,246,242}`↔`{240,248,244}` etc. would change captured BMP bytes. Mitigation: keep
   distinct tokens; smoke tests only assert capture existence/size, but byte-exact is the
   conservative default. Light-polish unification is a follow-up that edits only `tokens.h`
   + the variant tables.
9. **HudBand text not migrated to BodyText.** Risk: inconsistency. Mitigation: HudBand is
   HUD body (18px) outside the titles/subtitle/in-panel-body scope; it keeps inline
   `tokens::text_visual(kTextHud, kFontHud)`. A future `BodyText` 18px variant could absorb
   it; flagged, not done here.
10. **CMake source-list removal timing.** Risk: removing `weapon_tile.cpp` etc. from source
    lists before the `.cppx` is transpiled, or vice-versa, breaks the build. Mitigation:
    Phase 5 does the add (cppx list) and remove (plain source list) in one atomic CMake edit
    + commit, then builds. The generated `.cpp` and the deleted plain `.cpp` are the same TU
    content (converted), so symbols don't double-define.
11. **`callback_deps.h` move.** Risk: it is widely included; a missed re-point breaks build.
    Mitigation: it is the one optional move (§4.1 note) — if risk-averse, leave it at
    `src/client/ui/callback_deps.h` (framework glue, no game vocab, no guard issue) and skip
    its include edits. The plan otherwise stands.
12. **Two screens define a nav hook consumed by another.** Risk: extracting hooks to
    `<screen>_actions.*` could break the consumer's include. Mitigation: each consumer
    includes the TARGET screen's `<target>_actions.h` (e.g. main_menu includes
    `in_game/in_game_actions.h` for `use_start_match`); `loadout_screen.h` re-includes
    `loadout_actions.h` so pause needs no edit. The include graph is enumerated in §5 +
    navigation area spec.

---

## Appendix: superseded area-spec proposals (for traceability)

- theme-tokens: `tokens.h` in `shooter::tokens` ADOPTED; its spacing/dimension token block
  is PARTIALLY DROPPED (only border-width tokens kept shared; per-component geometry lives
  in each component's variant table) to avoid dead tokens.
- layout: `ScreenLayout`+variant ADOPTED; its private `layout_tokens.h` in
  `shooter::client_layout::detail` SUPERSEDED by shared `tokens.h` + inline per-variant
  visual build.
- surfaces: `Panel`+`PanelVariant{Hero,Overlay,Sunken}`+`PanelSize` ADOPTED; its private
  `detail::PanelTokens` table SUPERSEDED by shared `tokens.h`.
- text: `ScreenTitle`(+Popup)/`ScreenSubtitle`/`BodyText` ADOPTED; `text_tokens.hx`
  SUPERSEDED by shared `tokens.h`.
- actions: `AppButton`/`ActionRow` ADOPTED; ADDED `AppCheckbox`/`AppInput` (from
  screens-simple) into the actions family; variant superset unified.
- navigation: `<screen>_actions.*` convention ADOPTED; `LoadoutTabList`/`LoadoutTab` free
  pair SUPERSEDED by compound `LoadoutTabs`/`LoadoutTabs::Tab` (from loadout spec).
- app_shell: framework-to-`app_shell/`, game-`shooter_provider`-stays ADOPTED;
  `callback_deps.h` move ADDED (optional).
- loadout: full decomposition ADOPTED as the loadout owner of record; its `LoadoutTitle` and
  `loadout_tokens.h` co-exist with the shared `ScreenTitle`/`tokens.h`.
- screens-simple: per-screen compositions ADOPTED; its four separate frame files and
  two-file panel/title trios SUPERSEDED by the variant components above.
```