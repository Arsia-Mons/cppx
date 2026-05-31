# Architecture

The canonical mental model for this project. Read this before proposing structural
changes. It is the source of truth for *where new code goes* and *which directions
dependencies may flow*. The per-directory `CLAUDE.md` files defer to this document.

This UI Reference is a C++20 / SDL3 retained-mode UI toolkit driven by a React-style
hook runtime, plus a shooter sample that doubles as a stress test (focus navigation,
retained screens, post-layout writes, a deterministic headless CLI).

---

## 1. Layering

```text
                          app/            process lifecycle + per-frame loop
                           │  composes everything; the only layer that
                           │  knows about every subsystem at once
        ┌──────────────────┼─────────────────────┐
        ▼                  ▼                     ▼
   client/ui/         platform/             renderer/
   (game screens,    (SDL window/input,    (SDL draw executor,
    shell, focus      control mailbox)      fonts, textures)
    glue, mutation         │                     │
    queue)                 │                     │ executes the IR;
        │                  │                     │ owns SDL_ttf
        ▼                  └─────────┐           │
      game/        (OS/library adapters,         │
   (rules + state)  UI-shaped input, never       │
        │            SDL types upward)           │
        ▼                                        │
       ui/   ◄─────────── consumes ──────────────┘
   (generic retained toolkit: UiTree, reconciler,
    flex layout, focus, style types, the tagged-union
    DrawCommand IR, SDL-free geometry)
        │
        ▼
   react.{h,cpp}   (React-style hook runtime over component trees)
```

**Allowed dependency direction:**

```text
client/ui/screens → client/ui → game → ui → react
```

- `app/` composes everything. It is the only directory permitted to know about every
  subsystem at once — it constructs SDL, fonts, the `UiSurface`, the `UiPipeline`,
  the `ShooterGame`, and the `ControlMailbox`, and runs the per-frame loop.
- `platform/` and `renderer/` are **siblings** consumed by `app/`. `platform/` adapts
  OS/SDL input into UI-shaped frames; `renderer/` executes the UI's draw commands into
  SDL. Neither depends on `client/`, `game/`, or `app/`.
- `ui/` is the generic toolkit. It must not depend on `client/`, `game/`, `platform/`,
  `renderer/`, or `app/`, and it must contain **no SDL/SDL_ttf code and no game
  vocabulary** (see §7).
- `game/` holds rules and state. It must not include SDL or UI headers.

`src/main.cpp` is a ~15-line entrypoint: build `AppOptions`, hand off to `app::App`.

### Source map

```text
src/app/        App::initialize/run/shutdown; GameLoop::tick() (the per-frame body)
src/platform/   sdl/window, sdl/input, control_mailbox (headless control + capture)
src/renderer/   draw_executor, font_registry, texture_registry, ui_surface,
                text_measure_impl (the SDL_ttf-backed MeasureTextFn)
src/ui/         input.h, span.h
  ui/style/       visual_style.h, style_patch.h, theme.h, default_theme.cpp,
                  resolve.{h,cpp}, interaction.h, text_measure.{h,cpp}
  ui/runtime/     tree, element (reconciler), flex_layout/yoga_flex_layout, focus,
                  interaction_hooks, draw_command (IR), draw_command_builder
                  (transcriber), geometry (SDL-free tessellation)
  ui/components/  generic element-returning widgets (box/button/checkbox/dialog/
                  input/text), authored in the .cppx/.hx dialect
src/client/ui/  ClientUi shell, UiPipeline, screen stack, providers, hooks,
                screens/* (the game's screens), screen-local components
src/game/       player/weapons/economy/inventory rules + state
src/react.{h,cpp}  the hook runtime (full API in the react.h header comment)
third_party/    stb_image
tests/          C++ unit-test binaries + python guards + CLI smoke tests
tools/          cppx_transpile.py, ui_cli.py (headless control/capture driver)
```

---

## 2. The retained runtime + React-style hook model

Components are authored as functions that *return element descriptions*, not as
objects that imperatively mutate a tree. The hook runtime (`src/react.h`) is
intentionally React-shaped:

- `REACT_COMPONENT_BEGIN` / `_KEY` open a component; slot callbacks supply children.
  `children` are valid only during the call — never store them.
- Hooks (`use_state`, `use_effect`, `use_memo`, providers/context, …) carry per-fiber
  state. Stable hook identity comes from the runtime's **parent-hash + sibling index**.
  Reorderable / keyed siblings must use keyed component/node identity so state follows
  the element across reorders.
- The reconciler in `ui/runtime/element.{h,cpp}` commits returned element descriptions
  into the retained `UiTree` (`ui/runtime/tree.{h,cpp}`): it diffs, reuses, and tags
  each committed `Node` with both its layout box and the **fiber id** that produced it.

There are **two distinct id systems**, kept separate on purpose:

- **Fiber ids** — hook state identity (parent-hash + sibling index), from the runtime.
- **Node ids** — retained-tree identity (`make_child_id` = FNV of parent + type + key +
  sibling), used for layout and focus.

The bridge between them is the per-node `fiber_id` tag (see §5).

### Per-frame flow (`GameLoop::tick`)

```text
poll SDL events  →  build UI-shaped input frame  →  run UiPipeline:
    UiPipeline::render_client_ui_frame(frame, render_cb):
        ClientUi::update_retained_runtime(...)         // declaration + reconcile + layout
            └─ publishes last frame's InteractionSnapshot as a provider (§5)
            └─ runs the component tree, reconciles into UiTree, lays out (Yoga/flex)
            └─ build_draw_command_list(tree, &cmd_list, focused)   // transcriber (§3)
        render_cb():
            surface.clear(...)
            execute_draw_commands(renderer, cmd_list, fonts, textures)   // executor (§3)
            control.capture_after_render(...)          // headless BMP capture
            surface.present()
→  drain deferred mutations (§6)
```

---

## 3. The styling / render system (post-rewrite)

This is the from-first-principles styling/render pipeline. Styling ownership lives in
the **components**; the render path is a transcriber + a linear executor over a single
tagged-union IR. The design is documented in
`docs/retained-ui/styling-render-system-design.md`.

```text
  component (use_theme + resolve)         ui/style/, ui/runtime/interaction_hooks
        │  produces a dense, resolved VisualStyle and commits it as node.visual
        ▼
  build_draw_command_list(tree, &list)    ui/runtime/draw_command_builder  (transcriber)
        │  walks the laid-out UiTree depth-first; for each node emits the IR:
        │  Shadow → (Gradient | Rect) fill → Image → Border(+fused focus Outline)
        │  → Text; group opacity wraps a subtree in LayerPush/LayerPop; clipping
        │  uses ClipPush/ClipPop. Colors are PREMULTIPLIED at this boundary.
        ▼
  DrawCommandList  (the IR)                ui/runtime/draw_command.{h,cpp}
        │  tagged-union DrawCommand{kind, node_id, DrawRect, DrawPayload};
        │  POD arms; variable data (text bytes, gradient stops) lives in arenas
        │  addressed by integer handles, never pointers. Bounds-checked: overflow
        │  fails the whole frame, never truncates.
        ▼
  execute_draw_commands(renderer, list,    src/renderer/draw_executor.{h,cpp}
                        fonts, textures)
           a linear executor: Rect / Gradient / Border / Image / Shadow via the
           SDL-free geometry (below); Text via the font registry; ClipPush/Pop via
           SDL clip rect; LayerPush/Pop via offscreen render targets for group
           opacity. Draws under SDL_BLENDMODE_BLEND_PREMULTIPLIED. fonts may be
           null (text skipped); textures may be null (images skipped).
```

### Style types (SDL-free, `src/ui/style/`)

- `visual_style.h` — `Color` (STRAIGHT alpha), `DrawRect`, `SideWidths/Colors`,
  `Border`, `Outline` (signed offset: `<0` inset, `>0` outset), `Gradient`,
  `BackgroundImage` (tint + optional nine-slice), `Shadow`, `TextVisual`, and the dense
  `VisualStyle` the renderer reads. `LineRun` is one measured, alignment-baked line.
- `style_patch.h` — the **one** sparse authoring overlay. Optionality is always
  `Opt<T>{set, value}`; presence is `Opt::set`, **never a value-space sentinel**
  (see §8). `apply()` overlays a patch onto a dense `VisualStyle`; `merge()` overlays a
  patch onto a patch; `patch()` is the fluent builder so the set-flag is impossible to
  forget. (In the *resolved* `VisualStyle`, "no fill / no gradient / no shadow / no
  image" is read via value cues — `background.a==0`, `gradient.stop_count==0`,
  `shadow.color.a==0`, `image.texture_id==0` — because those are resolved outputs, not
  authoring inputs.)
- `theme.h` / `default_theme.cpp` — `RoleStyle{base, hover, pressed, active, disabled,
  checked, focus_visible}`, a `Theme` of roles (button, input, checkbox, box, text,
  dialog, focus_ring, …), `use_theme()`, and a **neutral, unopinionated**
  `default_theme()` fallback. Theme *values* are a client responsibility: the product
  palette lives in `client/ui/app_theme.cpp` and is installed via `ThemeProvider`
  (pushes `ThemeContext`) at the app-shell root; `use_theme()` hits the neutral
  `default_theme()` only when no provider is installed. Every primitive also accepts a
  per-instance `StylePatch style_override` prop, merged over its theme role by `resolve()`.
- `resolve.{h,cpp}` — `resolve(role, variant, interaction) → VisualStyle` with a locked
  precedence: the interaction state selects the slot (disabled wins; active sits between
  checked and disabled), and `focus_visible` contributes the focus outline only when the
  focus source is keyboard-visible.

### Color-space contract

`VisualStyle` colors are **STRAIGHT** alpha; the IR is **PREMULTIPLIED**. The transcriber
premultiplies every color at emit: `out.rgb = c.rgb * c.a / 255`, `out.a = c.a`. The only
fully-transparent color is `(0,0,0,0)`.

### Two SDL-free seams

`ui/` never links SDL or SDL_ttf. The boundary is crossed through exactly two seams:

1. **Geometry** (`ui/runtime/geometry.{h,cpp}`) — pure tessellation math
   (`tessellate_rect_fill`, `tessellate_frame`, `gradient_fill_colors`,
   `tessellate_shadow`) emitting triangle meshes into a caller-owned, fixed-capacity
   `MeshSink`. Zero SDL types. The executor hands the meshes to `SDL_RenderGeometry`.
2. **Text measurement** (`ui/style/text_measure.{h,cpp}`) — a single
   `MeasureTextFn` function pointer, installed once at startup by the renderer (which
   owns SDL_ttf). It is used as **both** the Yoga per-node measure shim **and** the
   draw-time per-line emitter, so *measure == draw by construction*. This function
   pointer is the only way text metrics enter `ui/`.

---

## 4. Focus & interaction model

- Focus and input routing happen in **UI coordinates**, never SDL coordinates.
  `ui/runtime/focus.{h,cpp}` tracks the focused node plus the hovered / focus-hovered /
  focus-pressed ids and whether the focus source is keyboard-visible.
- Interaction state reaches components through an **every-frame-read** provider, with a
  deliberate one-frame lag: after `focus_update`, `ClientUi` builds an
  `InteractionSnapshot` mapping the focused / hovered / pressed `NodeId` to the
  `fiber_id` of the owning node and pushes it as `InteractionContext` around the next
  reconcile.
- Components read it via fiber-keyed hooks in `ui/runtime/interaction_hooks.{h,cpp}`:
  `use_focused()`, `use_hovered()`, `use_pressed()`, `use_focus_visible()`. Each compares
  `react_current_fiber_id()` against the snapshot. Components feed that `InteractionState`
  into `resolve()` to pick the right `RoleStyle` slot — so hover/press/focus visuals are
  pure data, not imperative paint.

Keying interaction by fiber (not a recomputed node-id hash) is intentional: render runs
before the host's `begin_node`, so recomputing the node id inside a hook would be fragile.
Tagging each node with its producing fiber is robust and honors the every-frame-read model.

---

## 5. The deferred-mutation rule

The UI declaration pass must be **pure** — it may read game state but must never mutate it
(mutating during layout would tear the frame and break determinism).

> Any write that mutates game state in response to UI must be queued during the
> declaration pass via `client::ui::ClientUi::queue_deferred_mutation(...)`, and is drained
> by the loop **after** the frame is rendered.

Never mutate game state inside a component body, hook, or layout callback. The mutation
queue is the one sanctioned write path.

---

## 6. The `ui/` boundaries (hard rules)

These are enforced by guards in `tests/` (see §8) and are not optional:

- **`ui/` is SDL-free.** No `<SDL3/...>` / `<SDL.h>` includes and no `SDL_` / `TTF_`
  symbols anywhere under `src/ui/`. SDL enters only through `platform/` (input adapters)
  and `renderer/` (the executor), and crosses into `ui/` only via the geometry mesh
  contract and the single `MeasureTextFn` pointer.
- **`ui/` has no game vocabulary.** No weapon / loadout / HUD / round / shooter concepts,
  and no replicated game state. Game-specific UI lives in `client/ui/`.
- **`game/` includes no SDL or UI headers.** Rules and state stay renderer-agnostic.
- **No value-space sentinels in the style authoring overlay.** Optionality is `Opt<T>`
  (§3, §8).
- C++20, **no exceptions** in UI/runtime code, **no RTTI** assumed. Warnings are errors of
  attention (`-Wall -Wextra`).

---

## 7. Where new code goes

- **Game rules / state** → `game/`.
- **A new screen** → `client/ui/screens/<screen>/<screen>_screen.{h,cpp}` (with a
  `components/` subdir if it has screen-local components).
- **Cross-screen UI hook / component / provider** → `client/ui/{hooks,components,providers}/`.
- **Generic widget** (no game vocabulary) → `ui/components/`.
- **Generic runtime primitive** → `ui/runtime/`.
- **OS/SDL glue** → `platform/sdl/`.
- **Rendering backend code** → `renderer/`.
- **Per-frame loop / app lifecycle stage** → `GameLoop::tick()` in `app/`.

---

## 8. Tests & guards

Each subsystem has a focused test binary registered in `CMakeLists.txt`. The whole gate is
`./build.sh --tests` (configure + build everything + `ctest`); it is fully green at every
commit. Categories:

- **C++ unit tests** (hermetic, no SDL): `react_runtime_tests`, `retained_ui_*_tests`,
  `ui_components_tests`, `ui_style_tests`, `ui_style_resolve_tests`, `ui_geometry_tests`,
  `ui_interaction_hooks_tests`, `ui_draw_command_tests`, `retained_ui_focus_tests`,
  `client_ui_tests`, `ui_pipeline_tests`, `shooter_ui_tests`.
- **Golden render test**: `renderer_golden_tests` renders a hand-authored IR scene through
  a headless SDL (dummy video + software renderer) and pins it to
  `tests/fixtures/golden/*.bmp` via `tests/golden_util.h`. Regenerate intentionally with
  `UI_GOLDEN_REGEN=1`, then re-run without it to confirm, and commit the fixtures.
- **Python guards** (structural invariants, run under ctest):
  - `runtime_dependency_guard.py` — blocks banned UI dependency terms across `src/`, and
    enforces that **nothing under `src/ui/` includes SDL/SDL_ttf or uses `SDL_` / `TTF_`
    symbols** (comments are stripped before scanning; the `MeasureTextFn` pointer and the
    geometry mesh contract are the only seams).
  - `ui_style_invariants_guard.py` — pins the authoring optionality model: `Opt<T>` is the
    `{bool set; T value;}` shape, every `StylePatch` member is an `Opt<...>`, and
    `apply()` / `merge()` gate every field on `.set` (no value-equality presence test) —
    i.e. **no value-space sentinels** in the style authoring layer.
  - `component_descriptor_guard.py`, `cppx_transpiler_tests.py`, `cppx_editor_package_tests.py`
    — guard the `.cppx`/`.hx` component dialect and its transpiler.
- **Headless CLI smoke tests**: `ui_cli_smoke.py`, `ui_cli_commands.py`, `ui_cli_dm.py`
  drive the `hello` binary through `tools/ui_cli.py` over a control directory and assert on
  captured BMPs.

When adding a module, add a matching test target with a minimal dependency list so it
compiles in isolation.

---

## 9. Build

```sh
./build.sh           # configure + build hello (macOS/Linux)
./build.sh --tests   # build everything + run ctest
./build.ps1          # Windows: configure + build hello
./build.ps1 -Tests   # Windows: build everything + run ctest
```

The wrappers cache the CMake configure, hold a build lock (one builder at a time), and place
artifacts in `cmake-build-debug/` by default. SDL3 and SDL3_ttf are fetched via CMake when
not found locally; libcurl is required from the system. Components authored in `.cppx`/`.hx`
are transpiled by `cmake/cppx_transpile.cmake` (driving `tools/cppx_transpile.py`) before the
C++ compile.
