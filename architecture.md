# Architecture Notes

This project is a C++20 / SDL3 game-UI reference. The checked-in app runs
through a retained-mode UI runtime owned by this repo, with React-like
composition and hooks over retained nodes.

## Layers

```text
src/app/        process lifecycle, composition, and frame sequencing
src/platform/   SDL/input/control-mailbox adapters
src/renderer/   SDL drawing and font measurement backends
src/ui/         generic UI runtime, focus, layout, primitives, design tokens
src/client/ui/  client UI shell, providers, hooks, screens, game UI components
src/game/       game rules and state
src/react.*     React-style composition and hooks runtime
```

Allowed dependency direction:

```text
app -> client/ui -> ui -> react
app -> renderer/platform
client/ui -> game
```

`game/` must stay independent from UI, SDL, renderer, and retained-runtime
details. `ui/` must stay generic and free of shooter/game vocabulary.

## Current State

The app/runtime path now builds retained screens only. `UiPipeline` opens the
retained tree frame, updates retained flex layout through Yoga, resolves
retained focus/events, emits retained draw commands, and hands those commands
to the SDL retained renderer. `App` and `GameLoop` do not own a separate UI
layout backend.

The current React-style hook runtime stores hook state per component instance
and already has useful concepts the migration should preserve: component
identity, keyed siblings, providers/context, effects, refs, callbacks, and
per-frame unmount cleanup. Hook fiber identity is an app-owned 64-bit ID built
from the parent fiber and positional or keyed child identity.

`ClientUi` owns retained screens through `ScreenStack`, retained runtime
outputs, focus runtime lifetime, and the deferred mutation queue. That
ownership remains correct. The migration changes the generic UI runtime under
the screens, not the fact that `ClientUi` is the shell that sequences frames
and drains mutations.
`ClientUi` now also owns the retained `UiTree`, retained focus runtime, and
retained draw list. `UiPipeline` now opens the retained tree frame and refreshes
retained flex layout, focus/event state, and draw commands before the render
callback.

## Target Runtime

The retained UI target is:

```text
src/ui/runtime/
  UiTree
    stable app-owned node identity
    keyed children and positional children
    per-frame reconciliation
    lifecycle cleanup for unmounted nodes
    style and computed layout storage

  flex layout adapter
    converts retained styles into a real flexbox engine
    writes computed rects back onto retained nodes

  event routing
    hit testing against retained layout boxes
    pointer/key/gamepad events routed by retained node ID

  focus runtime
    focus scopes, navigation, and callbacks over retained node IDs

  renderer boundary
    retained draw commands consumed by renderer/
```

The runtime boundary is retained-node based: component identity, layout, input
routing, and render output all use repo-owned data structures.

## Flexbox Strategy

Use an existing maintained flexbox engine instead of hand-rolling full flexbox.
The selected migration direction is Yoga:

- Yoga is an embeddable flexbox layout engine with a C++20 implementation and
  CMake build support: https://github.com/facebook/yoga
- It is a direct C/C++ fit for this repo's C++20/CMake/SDL stack.
- Taffy is strong technically, but it is Rust/WASM-first and would add a Rust
  bridge before this repo has any Rust toolchain requirement:
  https://taffylayout.com/
- Stretch is Rust-first and older than Taffy/Yoga for this use case:
  https://github.com/vislyhq/stretch

The retained tree talks to Yoga through `src/ui/runtime/FlexLayoutAdapter`.
That keeps app code independent from Yoga headers while letting tests cover
tree reconciliation and actual flex layout behavior separately.

## JSX-Like Authored UI

The end state should support authored `.cppx` and `.hx` files that transpile to
ordinary C++ before compilation. The transpiler belongs under `tools/` with
golden tests under `tests/`.

The authored model should be React-like C++, not JavaScript:

```cpp
<Panel key="pause" style={Style{.width = Length::points(320)}}>
    <Panel.Header>
        <Text value="Paused" />
    </Panel.Header>
    <Panel.Body>
        <Button id="ResumeButton" onActivate={actions.resume}>
            Resume
        </Button>
    </Panel.Body>
</Panel>
```

Generated C++ should be deterministic, debuggable, and include source line
mapping in diagnostics. The generated code should call retained runtime
builders and normal C++ component functions. It must not introduce a JS runtime,
DOM assumptions, or React imports.

Current implementation: `tools/cppx_transpile.py` lowers `.cppx` / `.hx`
JSX-like lines to returned `UiElement` construction over the hidden current
`UiElementFrame`. Public components receive props/children only; `ClientUi`
binds the frame before building screens, and the reconciler re-binds it while
rendering component bodies. Children become owned frame data in
`.children = children({ ... })` props, text children lower to `text(...)`, and
`return <...>` lowers to a normal returned element expression. The CMake helper in
`cmake/cppx_transpile.cmake` generates build-tree `.cpp` / `.h` outputs, and
golden tests pin output plus diagnostics.
Generated `.cppx` output is compiled against `src/ui/runtime/element.*` and
`src/ui/components/*`, proving the authored syntax path can create returned
descriptors, preserve hook identity through the reconciler, and commit
`HostKind::Box` / `HostKind::Text` output into `UiTree`.
`UiScreen` supports returned-element roots during the app/client migration, and
`ClientUi` commits those roots inside the existing screen/provider frame
boundary. Main menu, options, pause, in-game HUD/action UI, loadout, and
loadout dialogs are now migrated to returned elements. Loadout's provider value
is copied into `UiElementFrame` storage so descendants see stable context when
the reconciler invokes them.
The retained component surface now writes copied node metadata for host role,
semantic role, control id, accessibility metadata, label/value, interaction
state, and text-editing state. The public foundation is the HTML-derived
primitive set (`Box`, `Text`, `Button`, `Input`, `Checkbox`, `Dialog`);
selected cards, tiles, and tabs are client components composed from those
primitives rather than generic UI foundations. Retained controls store typed
activation/focus/text callbacks; `ClientUi` invokes them after retained
focus/event update and before render-command handoff, preserving the same
deferred-mutation frame boundary used by other controls.
The public component props are flat: `key` is retained reconciliation identity,
`id` is the inspectable/control identifier, and `style` is the single style prop.
`Style` mirrors the Yoga v3.2.1 styling surface through repo-owned layout
types, including percent/auto edge values, logical edge aliases, box sizing,
layout direction, flex shorthand, aspect ratio, gap percentages, per-edge
layout borders, and retained baseline/node flags. `UiTree` snapshots retain
Yoga layout readback for x/y/width/height plus direction, overflow, and
computed margin/border/padding. Renderer-owned visual fields such as
background, border color, text color, and font size remain alongside those
layout fields.
`src/ui/runtime/draw_list.*` is the renderer boundary: it walks
retained snapshots after flex layout and emits app-owned rect/text draw
commands from retained metadata.
Draw-list generation uses the visual fields on `Style` for styled panels and
headings while preserving default control styles.
`src/ui/runtime/focus.*` is the retained focus/event boundary: it collects
focusable retained nodes, uses computed `UiTree` layout boxes for spatial
navigation and pointer hit testing, and treats modal retained nodes as active
focus scopes. Focus changes and confirmed retained controls dispatch copied
callbacks from `ClientUi` at the frame boundary, and modal retained scopes
restore the parent focused node when they close.
`renderer/sdl_retained_renderer.*` consumes retained draw commands directly and
the app-owned game loop renders `ClientUi::retained_draw_list()`. `UiPipeline`
owns the retained frame lifecycle, so the list is
computed before renderer handoff rather than through side-car test code.
The control mailbox now reports retained focusables in the same `focusables`
array used by CLI pointer targeting.
The app screens are retained end-to-end. `MainMenuScreen`, `OptionsScreen`,
`PauseScreen`, `ShooterGameScreen`, `HudBand`, `LoadoutScreen`, and
`LoadoutConfirmDialog` emit retained panels, text, buttons, inputs, checkboxes,
focus callbacks, and activation callbacks. Retained overlay screens
consult the `ScreenProvider` top-screen flag before emitting modal nodes, so a
retained overlay does not render or trap input after a higher overlay covers it.
Indexed retained control metadata preserves deterministic CLI targeting for
weapon tiles and other repeated controls.
The old immediate retained helper API has been removed; app/client code now
enters the retained runtime by returning `UiElement` descriptors that the
reconciler commits into `UiTree`.

## Component API Direction

Use React-style composition conventions adapted to C++:

- provider-owned state with explicit `state`, `actions`, and `meta` interfaces;
- compound components for surfaces with shared provider state;
- children slots for composition;
- explicit variants instead of boolean prop sprawl;
- keyed children for reorderable lists;
- named actions from hooks/components instead of exposing mutation sinks broadly.

`$vercel-composition-patterns` is guidance, not a license to import React,
DOM, Next.js, or JavaScript APIs.

## Migration Order

1. Keep this architecture document current.
2. Land the retained tree and flex-layout adapter skeleton with tests.
3. Add the `.cppx`/`.hx` transpiler with golden tests and CMake integration.
4. Port generic primitives to retained nodes.
5. Port screens one coherent slice at a time through `ClientUi`.
6. Move focus/event routing to retained layout boxes.
7. Render retained draw commands through `renderer/`.
8. Remove obsolete layout-runtime dependencies, tests, and stale docs once
   parity is proven.
9. Add guard tests that prevent obsolete UI runtime dependencies from
   re-entering `src/ui`, `src/client/ui`, and `src/react.*`.

## Hard Rules

- Do not mutate game state during UI declaration/rendering. Queue mutations and
  drain them at the frame boundary owned by `ClientUi`.
- Do not move game rules into UI components.
- Do not move screen-local state into `game/`.
- Do not add game vocabulary to `src/ui`.
- Do not keep external layout-runtime semantics behind retained-looking names.
- Do not treat deterministic tests as optional; the CLI control path must
  remain scriptable through `tools/ui_cli.py`.
