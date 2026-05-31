---
name: client-ui-development
description: Use when building, editing, or reviewing client UI in this repo — new screens, semantic components, hooks, providers, navigation, or theming under src/client/ui (and the src/ui primitives + hook runtime they build on). Covers the React-shaped retained-C++ patterns, the deferred-mutation discipline, the variant/altitude rules, and where new UI code goes.
---

# Client UI Development

The authoritative guide for writing UI under `src/client/ui` (and the `src/ui` toolkit it
sits on). This UI is **C++20 / SDL3 retained nodes**, but the hook runtime in
`src/ui/runtime/react.h` is **deliberately React-shaped**. Treat it as React with three
hard differences:

1. **A component is a free function** `::ui::UiElement Foo(const FooProps&)`, registered via
   `::ui::component("Foo", props, Foo, key)`. No JSX classes, no `forwardRef`, no `ref`.
2. **"Actions" are deferred writes, not `setState`.** Setters returned by hooks queue a
   closure that runs *after* layout. The declaration pass (`build_ui()` / any component body /
   any hook) must be **pure** — read game/app state, never write it.
3. **Paint is resolved inside the component** from a theme role + a sparse override + live
   interaction state. The renderer never sees the theme.

## Before you touch anything

| You need… | Go to |
| --- | --- |
| `.cppx`/`.hx` syntax, JSX children, `<detail.Host>`, transpiler limits, validate toolchain | **skill `cppx-authoring`** — do not relearn syntax here |
| Generic React doctrine (variants over booleans, compound components, lift state, context interfaces) | **skill `composition-patterns`** — this skill maps each rule onto C++ |
| The single best worked example of *all* the patterns at once | read `src/client/ui/screens/loadout/` (it has its own `CLAUDE.md` calling it "the reference template") |
| Exhaustive API detail | `reference/hook-runtime.md`, `reference/styling-and-theming.md`, `reference/react-patterns-in-cpp.md` (this dir) |

`src/client/ui/CLAUDE.md` is the authoritative convention doc — this skill operationalizes it.
(Note: the root `CLAUDE.md` says `queue_deferred_write` and `src/client/ui/CLAUDE.md` references
`docs/client-ui-component-architecture-goal.md`; **both are stale** — the real API is
`queue_deferred_mutation` and that doc does not exist. Don't propagate either.)

## Where does new code go?

Dependency direction is one-way: `client/ui/screens → client/ui → game → ui → ui/runtime/react`.
`src/ui/` is **SDL-free** (the python guard `tests/runtime_dependency_guard.py` enforces it) and
**game-vocabulary-free** by convention (`src/ui/CLAUDE.md`).

| What you're adding | Where | Namespace |
| --- | --- | --- |
| A new screen | `client/ui/screens/<name>/<name>_screen.{hx,cppx}` (+ `components/`, `hooks/`, `providers/`, `lib/` if it grows) | `shooter` |
| Cross-screen semantic component | `client/ui/components/{actions,surfaces,layout,text}/` | `shooter` |
| Cross-screen hook (consumer API) | `client/ui/hooks/` | `client::ui` † |
| Cross-screen provider (owns context) | `client/ui/providers/` | `client::ui` † |
| Screen-local component/hook/provider | the screen's own `components/`/`hooks/`/`providers/` | `shooter` |
| Generic, game-agnostic widget | `src/ui/components/` | `ui::components` |
| Generic runtime primitive | `src/ui/runtime/` | `ui` |
| Product theme **values** (palette) | `client/ui/app_theme.cpp` (installed via `ThemeProvider`) | `client::ui` |

† **Namespace exception:** app-shell capabilities (`use_app`, `use_navigation`) and their
providers are `namespace client::ui`. **Game-domain** hooks/providers that carry game types —
notably `use_server()` / `ServerProvider` (they hold `ShooterGame`) — live in the same folders but
in `namespace shooter`. The folder is by role; the namespace follows whether the code names game
vocabulary.

**Co-locate first.** A component/hook/provider starts in the feature folder; promote to
`client/ui/{components,hooks,providers}/` only when a *real second consumer* appears.

## The load-bearing rules

These are the ones that cause correctness bugs or review rejections. Memorize them.

- **Never mutate during the declaration pass.** No game/app/UI-state write inside `build_ui()`,
  a component body, a hook body, or a layout/focus callback. Mutations go through a named
  setter that queues a deferred closure (see Recipe 3). The declaration pass may *read* state.
- **Hooks are positional — call them unconditionally, in a fixed order, before any early
  return.** Max **8 hooks per fiber**; consolidate many state cells into one `use_state<struct>`.
  An `if`/early-return that skips a hook corrupts slot identity (the runtime diagnoses it).
- **Public component/screen APIs are props, children, hooks, and providers only.** Never expose
  `UiElementFrame`, `Style`/`VisualStyle`, `ActivationEvent`/`FocusEvent`, the mutation sink, or
  raw `use_state` pointers. Only a component's *own implementation* touches host/runtime types.
- **Select appearance with a closed `enum` variant, never boolean mode props.** (Leaf
  host-*state* flags like `disabled`/`default_focused` are fine; mode/host-swapping booleans are not.)
- **Screens pass variants; the component owns the look.** A screen never builds a `LayoutStyle`
  or `StylePatch` — it passes `variant={...}` and semantic props.
- **Navigation is owned by the caller.** The destination exports only its screen *type*; the
  caller composes `use_navigation().push(std::make_unique<Dest>())`. No `*_actions` files.
- **Context lives privately with its provider.** The `ReactContext` is a file-static in the
  provider `.cpp`; export the provider from `providers/` and the consumer hook from `hooks/`
  (implemented in the provider `.cpp` so the context type stays private).
- **Capability hooks, not god hooks.** `use_app()` (quit), `use_navigation()` (stack ops +
  `is_top`/`current_entry_id`), `use_server()` (game state + actions; `namespace shooter`). Don't
  invent a flow hook that knows specific button flows.
- **Hover/press/focus paint is gated on focusability *and* requires a host that resolves *live*
  `InteractionState`.** `Button` is focusable and resolves live. **`Box` is `focusable=false` and
  resolves with empty `{}`** — it never enters the hover candidate set (`focus.cpp` `hovered_enabled`
  iterates only focusables) and its `.hover` slot is dead data. So a hover-reactive node is, by
  construction, an *interactive* (focusable / nav-stop) node. See Recipe 2.
- **Build before you claim done.** Run `./build.sh --tests` and report the real result. Never
  fabricate a "blocked build" excuse. Keep your diff self-contained — ignore unrelated in-flight files.

## Recipe 1 — Add a screen

Pick the base class: **`OverlayScreen`** if it floats over another screen (Pause/Options/Loadout);
**`UiScreen`** if it's an opaque base you `reset_to`. Then three things and you're done: the
`.hx/.cppx` pair, two lines in CMake, and a caller that pushes it.

`screens/credits/credits_screen.hx`:
```cpp
#pragma once
#include "client/ui/app_shell/navigation/ui_screen.h"
#include "ui/runtime/element.h"

namespace shooter {
class CreditsScreen final : public client::ui::OverlayScreen {
public:
  CreditsScreen() = default;
  const char *debug_name() const override { return "Credits"; }
  bool build_element(::ui::UiElementFrame &frame, ::ui::UiElement *out) override;
  void build_ui() override;
};
} // namespace shooter
```

`screens/credits/credits_screen.cppx` (the `*View` free function **is** the screen body):
```cpp
#include "credits_screen.h"
#include <functional>
#include <memory>
#include <stdio.h>
#include "client/ui/callback_deps.h"
#include "client/ui/components/actions/actions.h"
#include "client/ui/components/layout/layout.h"
#include "client/ui/components/text/text.h"
#include "client/ui/hooks/use_navigation.h"
#include "ui/runtime/react.h"

namespace shooter {
using ::ui::children;

struct CreditsScreenProps { uint32_t unused = 0; };

static const char *screen_entry_key(const char *prefix, client::ui::UiScreenEntryId id) {
  char key[64] = {};
  snprintf(key, sizeof(key), "%s-%u", prefix, id);
  return ::ui::copy_string(key);
}

static ::ui::UiElement CreditsScreenView(const CreditsScreenProps &props) {
  (void)props;
  client::ui::Navigation navigation = client::ui::use_navigation();

  // ALL hooks run before the is_top guard. The guard yields the frame to a
  // covering overlay: visible screens build top-first, so a base/overlay screen
  // that keeps rendering paints OVER (and traps input from) the screen above it.
  if (!navigation.is_top)
    return ::ui::empty();

  return <ScreenLayout key="root" variant={ScreenLayoutVariant::CenteredOverlay}>
    <ScreenTitle key="title" variant={ScreenTitleVariant::Screen} value="Credits" />
    <BodyText key="line-1" value="Built with the retained UI runtime." />
    <BodyText key="line-2" value="Thanks for playing." />
    <AppButton key="back" controlId="CreditsBackButton" label="Back" onPress={navigation.pop_current} />
  </ScreenLayout>
}

bool CreditsScreen::build_element(::ui::UiElementFrame &frame, ::ui::UiElement *out) {
  (void)frame;
  if (!out) return false;
  *out = ::ui::component("CreditsScreen", CreditsScreenProps{}, CreditsScreenView,
                         screen_entry_key("credits", entry_id()));
  return true;
}
void CreditsScreen::build_ui() {} // legacy pure-virtual; always empty
} // namespace shooter
```

Wire the caller (e.g. main menu) — caller owns navigation:
```cpp
std::function<void()> open_credits = use_callback(
    [navigation] { if (navigation.push) navigation.push(std::make_unique<CreditsScreen>()); },
    client::ui::callback_deps(navigation.current_entry_id));
// ... <AppButton key="credits" controlId="OpenCreditsButton" label="Credits" onPress={open_credits} />
```

`controlId` is the stable handle the headless CLI/smoke tests address a control by — keep it
unique and descriptive, following the existing `<Action>[From<Source>]Button` convention
(`ResumeButton`, `OpenOptionsFromPauseButton`, `BackFromOptionsButton`). The header may be a plain
`.h` (it has no JSX — `loadout_screen.h` is one) or `.hx`; the *body* is always `.cppx`.

CMake: add the `.cppx`/`.hx` to the `cppx_transpile(CLIENT_UI_GENERATED ...)` call. That's all —
`hello` and `shooter_ui_tests` already consume `${CLIENT_UI_GENERATED}`. There is no screen
registry/enum to update; a screen exists once something `push`es/`reset_to`s it.

## Recipe 2 — Add a semantic component

Two files: a `.hx` public interface (semantic props only) and a `.cppx` impl that switches on
the variant, resolves tokens → `LayoutStyle` + `StylePatch`/`StyleStatePatch`, and forwards to a
primitive. `children` is the **last** struct field and the **last** JSX child.

**First decide: does it need to react to hover / press / focus?** That single question picks the
host primitive — and getting it wrong is the most common bug here (see below).

### 2a — A static surface (no interaction): root on `Box`

`components/surfaces/tag.hx`:
```cpp
#pragma once
#include "ui/components/common.h"

namespace shooter {
enum class TagVariant { Info, Warning };
struct TagProps {
  const char *key = nullptr;
  TagVariant variant = TagVariant::Info;
  ::ui::UiChildren children = {};      // LAST
};
::ui::UiElement Tag(const TagProps &props);
} // namespace shooter
```

`components/surfaces/tag.cppx` — the only place host/runtime types appear. **Note the `using`
lines**: the transpiler emits bare `Box`/`BoxProps`/`children(...)`, which live in `::ui` /
`::ui::components`; without the `using`s the file won't compile (a `cppx-authoring` gotcha):
```cpp
#include "tag.h"
#include "client/ui/components/tokens.h"
#include "ui/components/box.h"

namespace shooter {
using ::ui::children;                  // for {props.children} lowering
using ::ui::components::Box;
using ::ui::components::BoxProps;

namespace {
::ui::StylePatch tag_patch(TagVariant v) {
  switch (v) {
  case TagVariant::Warning: return tokens::panel_patch(tokens::kDanger, tokens::kDangerBorder);
  case TagVariant::Info:
  default:                  return tokens::panel_patch(tokens::kSurfacePanel, tokens::kBorderPanel);
  }
}
} // namespace

::ui::UiElement Tag(const TagProps &props) {
  ::ui::LayoutStyle layout{ .padding = {2.f, 8.f, 2.f, 8.f}, .border_width = tokens::kBorderWidth };
  return <Box key={props.key} layout={layout} style={tag_patch(props.variant)}>
    {props.children}
  </Box>
}
} // namespace shooter
```

A `Box` is `focusable=false` and resolves with an empty `InteractionState{}` — it **cannot
hover/press/focus**, and a `.hover` slot on it is dead data. That's correct for a static label.

### 2b — Needs hover / press / focus: root on `Button`

In this runtime, interaction paint is gated on focusability: `focus.cpp`'s `hovered_enabled`
only considers focusable nodes, and only `Button` resolves with **live** interaction
(`detail::interaction_state(...)`). **So a hover-reactive component is, by construction, an
interactive (focusable / nav-stop) node** — root it on `Button` and supply per-state override
slots. Solid fills must re-emit a flat 2-stop gradient or the Button role's slate gradient bleeds
through (`reference/styling-and-theming.md`):

```cpp
// chip.cppx
#include "chip.h"
#include <functional>
#include "client/ui/components/tokens.h"
#include "ui/components/button.h"

namespace shooter {
using ::ui::components::Button;
using ::ui::components::ButtonProps;

namespace {
::ui::StylePatch solid(::ui::Color fill, ::ui::Color border) {
  return ::ui::patch()
      .background(fill)
      .gradient(::ui::Gradient{.angle_deg = 0.f, .stop_count = 2, .stops = {{0.f, fill}, {1.f, fill}}})
      .border(::ui::Border{{1, 1, 1, 1}, {border, border, border, border}});
}
::ui::StyleStatePatch chip_style(ChipVariant v) {        // base + hover slots
  ::ui::StyleStatePatch s{};
  switch (v) {
  case ChipVariant::Warning:
    s.base  = solid(tokens::kDanger, tokens::kDangerBorder);
    s.hover = solid(tokens::kDangerHover, tokens::kDangerHoverBorder);
    break;
  case ChipVariant::Info:
  default:
    s.base  = solid(tokens::kAccent, tokens::kAccentBorder);
    s.hover = solid(tokens::kAccentHover, tokens::kAccentHoverBorder);
    break;
  }
  return s;
}
} // namespace

::ui::UiElement Chip(const ChipProps &props) {           // ChipProps: key, variant, label, on_press
  std::function<void(const ::ui::ActivationEvent &)> on_activate = {};
  if (props.on_press)
    on_activate = [on_press = props.on_press](const ::ui::ActivationEvent &) { on_press(); };
  ::ui::LayoutStyle layout{ .padding = {2.f, 10.f, 2.f, 10.f}, .border_width = tokens::kBorderWidth };
  // Button resolves use_theme().button with LIVE interaction, so .hover fires.
  return <Button key={props.key} label={props.label} onActivate={on_activate}
                 layout={layout} style={chip_style(props.variant)} />
}
} // namespace shooter
```

**Rules this enforces (each was a real failure mode):**
- **The form you actually use must be the live-interaction one.** If a requirement names hover,
  the *default* form must react — never ship a hover slot on a `Box` and bury the working
  `Button` form behind an opt-in.
- **No host-swapping boolean** (`interactive`/`clickable`/`selectable`) that flips a component
  between `Box` and `Button`. "Sometimes interactive" is **two components** (`Tag` vs `Chip`) or a
  variant that encodes it — not a boolean that doubles the state space and couples element choice
  to focus behavior.
- **Hover and nav are coupled.** A hover-reactive node joins the keyboard/gamepad focus order. If
  a surface genuinely must *not* be a nav stop (e.g. a HUD status indicator), it **can't** hover —
  surface that conflict instead of shipping a dead-hover `Box`.

Add the `.cppx`/`.hx` to `cppx_transpile(CLIENT_UI_GENERATED ...)`; re-export from the family
umbrella (`surfaces.h`/`actions.h`/…) only if truly shared. One consumer → keep it screen-local
under `screens/<screen>/components/` first.

## Recipe 3 — Add a capability (provider + private context + hook)

Pattern: a file-private `ReactContext`, a provider that `copy_value`s its value into the frame
arena and `::ui::provider`s it, and a consumer hook (implemented here) that null-checks
`use_context` and projects the private value into a clean public struct. The example below is
**illustrative** (`Audio`/`AudioState` aren't real files); its shape mirrors the real
`AppProvider` / `ServerProvider`.

```cpp
// providers/audio_provider.cpp
namespace client::ui {
static ReactContext AudioContext = {};   // private to this TU

// AudioState { bool muted; } is the long-lived POD (see "Where does the state live" below); the
// provider value just points at it — mirroring ServerProvider -> ShooterGame. Providers take no
// `key`: they're singletons in the frame-provider stack (match AppProvider/ServerProvider).
::ui::UiElement AudioProvider(const AudioProviderValue &value, ::ui::UiChildren children) {
  const AudioProviderValue *stored = ::ui::copy_value(value);   // outlives the build call
  if (!stored) return ::ui::empty();
  return ::ui::provider("AudioProvider", &AudioContext,
                        const_cast<AudioProviderValue *>(stored), children);
}

Audio use_audio() {                       // return type declared in hooks/use_audio.h
  auto *value = static_cast<AudioProviderValue *>(use_context(&AudioContext));
  if (!value) { react_report_error("client/ui: missing AudioProvider\n"); return {}; }
  AudioState *state = value->state;

  // The setter HIDES the deferred-mutation sink and queues the write — it never mutates
  // synchronously. A plain std::function is the convention for app/server capability setters;
  // loadout's screen-local setters wrap this in use_callback + callback_deps only when a
  // retained control needs a stable handler identity across frames.
  internal::DeferredUiMutationSink mutations = internal::use_deferred_ui_mutations();
  std::function<void()> toggle = [state, mutations] {
    if (state && mutations) mutations.submit([state] { state->muted = !state->muted; });
  };

  return { .muted = state ? state->muted : false, .toggle_muted = toggle };
}
} // namespace client::ui
```

**Where does the state itself live?**
- **Long-lived / cross-screen** (survives screen pops): a POD owned by the composition root
  `app::App`; the provider holds a *raw pointer* to it (mirrors `ServerProvider` → `ShooterGame`).
  Install the provider in the app frame-provider stack. A screen-local `use_state` can't do this.
- **Screen-local UI state** (a tab index, a pending action): hold it in a screen-local provider
  via `use_state<T>` and expose it through a `use_*()` hook (see `loadout_provider.cpp`). **Never**
  a member on the `UiScreen` subclass — screens are rebuilt every frame and don't participate in
  the hook lifecycle.

Ordinary components consume the **named** setter (`use_audio().toggle_muted`), never the sink.

## Common mistakes (grounded in real failures)

| Mistake | Reality / fix |
| --- | --- |
| `.hover` slot on a `Box` and expecting it to react | `Box` is non-focusable and resolves with empty `InteractionState` — hover is dead. Root the *used* form on `Button` (Recipe 2b). |
| `bool interactive`/`clickable`/`selectable` that swaps `Box`↔`Button` | Host-swapping boolean is the anti-pattern. Make it **two components** (`Tag` vs `Chip`) or a variant `enum`; never one boolean coupling element choice to focus. |
| `bool isPrimary`/`isDanger` mode props | Use a closed variant `enum`; the impl switches host + paint internally. |
| Passing `label=` **and** a nested `<Text value=label>` child | Double render. Pick one: pass `label` and rely on the host's `{children.count>0 ? nullptr : props.label}` fallback, or render the child. |
| Mutating game/UI state in the component body / a focus handler | Queue it: a named setter that calls `mutations.submit([=]{ ... })`. Declaration pass stays pure. |
| `is_top` guard *before* the hooks | Run every hook first, then `if (!navigation.is_top) return ::ui::empty();`. Early-returning before hooks corrupts slot identity. |
| A `*_actions` file or destination-owned `open()` helper | Destination exports only its type; caller composes `use_navigation().push(make_unique<Dest>())`. |
| `ReactContext` or the stored value struct declared in a header | Keep both private in the provider `.cpp`; export only the provider + the consumer hook. |
| Screen UI state stored as a `UiScreen` member | Hold it in a screen-local provider via `use_state<T>`; expose via a hook. |
| Calling `ClientUi::queue_deferred_write(...)` | That symbol doesn't exist (stale root `CLAUDE.md`). The method is `queue_deferred_mutation`; product code uses a named hook, not it directly. |
| Claiming "build blocked" without running it / mixing unrelated files into the diff | Run `./build.sh --tests`, report the real output. Keep the diff self-contained. |
| A screen split into `Screen` + reusable `View` for no reuse | The `*View` free function wired through `::ui::component` is the *required* entry point, not a gratuitous split. Don't add a second reusable view unless genuinely reused. |

## Verify

After editing any `.cppx`/`.hx`:
```sh
python3 tools/cppx_format.py --in-place <files>
python3 tools/cppx_transpile.py <file.cppx>
./build.sh --tests
git diff --check
```
Tests: `client_ui_tests` (screen stack + mutation queue), `ui_pipeline_tests` (frame wrapper),
`shooter_ui_tests` (screens), `react_runtime_tests` (hook semantics), plus python CLI smoke tests.

**Reporting a green change on an already-red tree.** If `./build.sh --tests` is red for reasons
unrelated to your change (this branch has a known pre-existing `renderer_golden_tests` failure),
don't fabricate a "blocked build" and don't fold someone else's fix into your diff. Instead prove
*your* slice: confirm the targets that compile/link your new TU (`hello`, `shooter_ui_tests`) build
green and `git diff --check` is clean, and report which pre-existing failures you did **not**
touch. In particular, if an unrelated in-flight change has already drifted a **shared** test file
(`tests/shooter_ui_tests.cpp`, `tests/ui_cli_commands.py` — e.g. focus-walk counts), do **not**
edit that test to chase a green suite; that makes your diff non-self-contained. Note it and leave it.

## Deep reference (this directory)

- `reference/hook-runtime.md` — hooks, fiber identity & keys, `use_callback`/`use_effect` deps,
  context, the per-frame contract, capacity limits, the imperative-vs-retained authoring layers.
- `reference/styling-and-theming.md` — `VisualStyle`/`StylePatch`/`StyleStatePatch`/`resolve()`,
  `InteractionState`, `Theme`/`RoleStyle`, `tokens.h`, the variant→paint chain, the hover-on-Box trap.
- `reference/react-patterns-in-cpp.md` — each `composition-patterns` rule mapped to its C++
  realization in this repo, with the divergences (no `forwardRef`, deferred-write "actions", etc.).
