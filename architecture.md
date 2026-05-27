# Architecture Notes

This project is a C++20 / SDL3 game-UI reference. The current checked-in app is
still Clay-backed, but the active migration target is a retained-mode UI
runtime owned by this repo. The goal is to preserve the React-like programming
model while replacing Clay as the layout/runtime dependency.

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

The current runtime still uses Clay for:

- element identity and immediate layout blocks;
- flex-like layout and element bounds;
- pointer hit testing;
- render command emission;
- text measurement integration.

The current React-style hook runtime stores hook state per component instance
and already has useful concepts the migration should preserve: component
identity, keyed siblings, providers/context, effects, refs, callbacks, and
per-frame unmount cleanup. Hook fiber identity is now an app-owned 64-bit ID;
Clay-backed components still emit separate Clay IDs only for the temporary Clay
layout path.

`ClientUi` owns retained screens through `ScreenStack`, focus runtime lifetime,
and the deferred mutation queue. That ownership remains correct. The migration
changes the generic UI runtime under the screens, not the fact that `ClientUi`
is the shell that sequences frames and drains mutations.

## Target Runtime

The retained UI target is:

```text
src/ui/retained/
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

Clay-specific IDs, `CLAY(...)` layout calls, and Clay render command arrays are
temporary implementation details. They must not be preserved behind renamed
facades.

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

The retained tree talks to Yoga through `src/ui/retained/FlexLayoutAdapter`.
That keeps app code independent from Yoga headers while letting tests cover
tree reconciliation and actual flex layout behavior separately.

## JSX-Like Authored UI

The end state should support authored `.cppx` and `.hx` files that transpile to
ordinary C++ before compilation. The transpiler belongs under `tools/` with
golden tests under `tests/`.

The authored model should be React-like C++, not JavaScript:

```cpp
<Panel key="pause">
    <Panel.Header>
        <Text value="Paused" />
    </Panel.Header>
    <Panel.Body>
        <Button id="ResumeButton" onConfirm={actions.resume}>
            Resume
        </Button>
    </Panel.Body>
</Panel>
```

Generated C++ should be deterministic, debuggable, and include source line
mapping in diagnostics. The generated code should call retained runtime
builders and normal C++ component functions. It must not introduce a JS runtime,
DOM assumptions, or React imports.

Current implementation: `tools/cppx_transpile.py` lowers `.cppx` / `.hx` JSX-like
lines to normal C++ component calls with prop initializers and synchronous child
lambdas. The CMake helper in `cmake/cppx_transpile.cmake` generates build-tree
`.cpp` / `.h` outputs, and golden tests pin output plus diagnostics.
Generated `.cppx` output is now also compiled against generic retained
components in `src/ui/retained/components.*`, proving the authored syntax path
can create `UiTree` nodes and use retained hook identity without Clay layout.

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
4. Port generic primitives from Clay to retained nodes.
5. Port screens one coherent slice at a time through `ClientUi`.
6. Move focus/event routing from Clay element data to retained layout boxes.
7. Replace Clay renderer glue with retained draw commands consumed by
   `renderer/`.
8. Remove Clay dependencies, Clay tests, and stale Clay docs once parity is
   proven.
9. Add guard tests that prevent Clay from re-entering `src/ui`, `src/client/ui`,
   and `src/react.*`.

## Hard Rules

- Do not mutate game state during UI declaration/rendering. Queue mutations and
  drain them at the frame boundary owned by `ClientUi`.
- Do not move game rules into UI components.
- Do not move screen-local state into `game/`.
- Do not add game vocabulary to `src/ui`.
- Do not keep Clay semantics behind retained-looking names.
- Do not treat deterministic tests as optional; the CLI control path must
  remain scriptable through `tools/ui_cli.py`.
