# Plan: UI focus + interaction-state model

A sister plan to `plan.md`. Pure UI-layer. Lands `docs/archive/ui.md` —
input-agnostic navigation and the interaction / control / visual state
vocabulary — inside the React/Clay runtime, without touching the engine seam.

This doc is meant to be **falsifiable**, not persuasive — every claim about
current behavior cites a file/line, and every claim about proposed behavior
names the primitive that owns it. If something here doesn't match the code,
the doc is wrong; flag and fix it.

Anchored against: `src/react.{h,cpp}`, `src/input.h`, `src/main.cpp`,
`src/ui/providers/input_provider.{h,cpp}`,
`src/ui/providers/theme_provider.{h,cpp}`,
`src/ui/components/{app,counter,image}.cpp`, `docs/archive/ui.md`.

---

## 1. What problem this solves

The repo's UI spec in `docs/archive/ui.md` declares a vocabulary for any
interactive surface — button, toggle, selectable card — split into three
layers:

- **Interaction states**: `focused`, `focusVisible`, `focusWithin`,
  `hovered`, `pressed` — derived from raw input.
- **Control states**: `checked`, `selected`, `disabled` — semantic, passed
  by the caller.
- **Visual states**: `targeted`, `active`, `chosen`, `unavailable` — pure
  functions of the above; the only thing component styling branches on.

The goal is that the same component definition runs identically under
keyboard, mouse, gamepad, and touch — without per-device code in any
component body. The current codebase has none of this: there is no focus
concept anywhere, hover is exposed by Clay but not consumed, and
`InputState` holds semantic action flags (`increment_counter`,
`cycle_theme`, …) rather than navigation edges.

> **Why it can ship orthogonally.** The plan adds *no* engine state. Focus
> lives in React hook state under each screen's fiber subtree. The only
> thing it touches outside the React tree is `InputState` in `src/input.h`
> and the SDL event loop in `main.cpp`.

---

## 2. Why separate from plan.md

`plan.md` is about engine/UI seams: actor pools, screen stack, write lane,
lifecycle. **This plan is orthogonal.** It is about how a single UI
surface — a button, a card, a toggle — behaves uniformly across keyboard,
mouse, gamepad, and touch.

That orthogonality is structural. This plan does not extend `World`, call
`game.nav.*`, change `engine_tick`, or add anything to any `game.*`
subsystem. It *does* add platform-layer plumbing (SDL gamepad / touch in
`main.cpp`) and extends the existing per-frame `InputState` with UI-nav
fields — see F1.

Anything described here can ship before, during, or after any phase in
`plan.md` as long as it lands before the screen that needs it.

---

## 3. Scope

**In scope:**
- Normalizing **UI navigation** input (directional, confirm, cancel)
  across keyboard / mouse / gamepad / touch.
- A per-screen focus model: who owns focus, how it moves, how `focusVisible`
  is derived.
- Interaction state derivation: `focused`, `focusVisible`, `focusWithin`,
  `hovered`, `pressed`.
- Control state declaration: `checked`, `selected`, `disabled`.
- Visual state derivation: `targeted`, `active`, `chosen`, `unavailable` —
  pure functions of the above.
- A small set of primitive components (`<Focusable>`, `<Button>`,
  `<Toggle>`, `<Selectable>`) that wire it together.

**Out of scope:**
- Engine ownership of focus. Focus is UI state.
- Per-game-action input (movement, fire). `engine_tick` keeps reading
  `InputState`'s game-action fields unchanged.
- Pointer plumbing. Clay already produces per-frame hover + per-element
  pointer state (`Clay_PointerOver`, `Clay_GetPointerData`); F3 reads it
  directly rather than mirroring it into `InputState`.
- IME / text-input editing.
- Drag-and-drop, multi-pointer gestures beyond single touch.
- Accessibility (screen reader) — orthogonal pass.

---

## 4. Non-overlap with plan.md (binding)

- Focus state lives in React hook state **under each screen's fiber subtree**
  via `<FocusScope>` (F2). It does not appear on `World` or any `game.*`
  subsystem.
- Inter-screen navigation is unchanged: `use_*_actions()` hooks → subsystem
  methods → `game.nav` as `plan.md` specifies. Focus does not enter that path.
- `WorldContext` is not extended. The new `FocusContext` is an independent
  provider. The existing `InputContext` is reused; F1 only adds fields to
  the `InputState` struct it already carries.
- The platform-layer addition in F1 (gamepad/touch SDL handling in `main.cpp`)
  is the *one* non-React-tree change this plan makes; nothing beyond it
  crosses into engine-owned state.

---

## 5. The architecture today

### Per-frame data flow (current main loop)

```
SDL3 event poll ──► InputState (increment_counter, cycle_theme, …)
                ──► Clay_SetPointerState(pos, lmb_held)

react_begin_frame   (G.frame++  — generation sweep)
Clay_BeginLayout
App(&input)         INPUT_PROVIDER { THEME_PROVIDER { Counter() Image() } }
Clay_EndLayout      → render commands
react_end_frame     (unmount sweep + queued effects)

SDL_Clay_Render → SDL_RenderClear → SDL_RenderPresent
```

The render loop is small enough to read at a glance (`src/main.cpp:132-178`):

```cpp
while (running) {
    InputState input = {};
    // drain SDL events into input.* flags
    while (SDL_PollEvent(&ev)) { … }

    Clay_SetPointerState({mx, my}, lmb_held);

    react_begin_frame();
    Clay_BeginLayout();
    App(&input);
    Clay_RenderCommandArray cmds = Clay_EndLayout();
    react_end_frame();   // unmount sweep + queued effects

    SDL_Clay_RenderClayCommands(&rd, &cmds);
    SDL_RenderPresent(g_sdl);
}
```

> **Known hazard, fixed in F0.** `react_end_frame()` runs cleanup callbacks
> *before* `SDL_Clay_RenderClayCommands` consumes `cmds`. A same-frame
> unmount of a textured fiber (e.g. `Image()`'s `cancel_fetch` at
> `image.cpp:117-129`) can destroy a resource that `cmds` is about to
> draw. Today this is rare because `Image` is mounted permanently; focus
> work creates same-frame unmounts of textured elements (modal close,
> screen pop) and will start triggering it. Phase F0 reorders the loop —
> render commands first, cleanup after — and is a prerequisite for every
> later phase.

### The React runtime — what it actually guarantees

`src/react.cpp` is not React. It is a small hook-table layered on top of
Clay's element-ID hashing. Key facts a focus implementation has to live with:

| Property | Where | Implication for focus |
|---|---|---|
| Every fiber re-runs every frame; there is no diffing. | `react.cpp:363-389` | "first render only" doesn't exist. Cross-frame state must use `use_ref` or `use_state_int`. |
| Hook count per fiber must be stable across frames; cap of 8. | `react.cpp:9, 248-251` | Every focus primitive must declare a fixed hook budget. A focusable cannot conditionally call `use_ref`. |
| `use_context` does *not* consume a hook slot. | `react.cpp:355-359` | `FocusContext` reads are free. Adds zero hooks to consumers. |
| Fiber identity is parent-hashed. | `react.cpp:232-236` | Re-keying a screen (new parent id) re-keys every descendant — any cached `Clay_ElementId` from before becomes invalid by construction. |
| Effects fire *after* `Clay_EndLayout`. | `react.cpp:391-414` | Anything that needs post-layout data is one frame late. Focus intentionally avoids this by reading Clay pointer state inline during render. |
| Unmount sweep is generational. | `react.cpp:382-389` | An unmounted focusable's fiber is destroyed before the next frame; focus's "live-set" check piggybacks on this. |

### Providers today

Providers are real fibers wrapped in a `for`-loop macro that brackets
`react_provider_push`/`pop`. Critically,
**`theme_provider__enter` calls `use_state_int(0)`** — meaning the
`ThemeProvider` fiber consumes a hook slot, even though it looks
transparent at the call site (`theme_provider.cpp:18-27`). The new
`<FocusScope>` follows the same pattern: it is a fiber that owns one
`use_ref` for the `FocusManager`.

```cpp
// src/ui/providers/input_provider.h
#define INPUT_PROVIDER(input_ptr)                                  \
    for (int _once =                                               \
             (REACT_PROVIDER_ENTER("InputProvider"),               \
              input_provider__enter((input_ptr)), 0);              \
         !_once;                                                   \
         _once = 1, input_provider__exit(),                        \
                    REACT_PROVIDER_EXIT())
```

---

## 6. The five-layer stack

This is the plan's structural claim: each layer reads only from the one
below it, and component bodies branch on the **top** layer only.
Read it bottom-up.

- **L5 — Visual states** *(pure fn)*
  `targeted`, `active`, `chosen`, `unavailable`. Derived in
  `derive_visual_state(FocusableState, ControlState)`. Styling code reads
  only this.
- **L4 — Interaction states** *(hook return value)*
  `focused`, `focusVisible`, `hovered`, `pressed` (`focusWithin`
  deferred). Returned by `use_focusable(...)`. Reads L1+L2.
- **L3 — Control states** *(caller props)*
  `checked`, `selected`, `disabled` — passed in by the component caller.
  Not derived.
- **L2 — Focus model** *(context + manager via `use_ref`)*
  One `FocusManager` per `<FocusScope>`. Owns `focused_id`,
  `last_focus_move_source`, and `focusable_list` (rebuilt each frame).
- **L1 — Input sources** *(existing primitives)*
  **InputState** (nav/confirm/cancel edges, added in F1) +
  **Clay** (`Clay_PointerOver(id)`, `Clay_GetPointerData()`). Two
  sources, no mirror.

```
┌─ L5 Visual states (targeted / active / chosen / unavailable)   ← pure fn
├─ L4 Interaction states (focused / focusVisible / focusWithin /  ← hook
│       hovered / pressed)
├─ L3 Control states (checked / selected / disabled)              ← props
├─ L2 Focus model (per-screen owner + traversal)                  ← context + hook
└─ L1 Input sources                                                ← existing
       - nav / confirm / cancel edges on InputState (F1)
       - pointer hover + state via Clay (Clay_PointerOver,
         Clay_GetPointerData)
```

Each layer reads only from the layer below it. Component bodies branch on
**visual** state only — never on raw interaction or control state — so
styling code is uniform regardless of input device or semantic role.

> **Non-obvious binding.** Pointer state is **not mirrored into `InputState`**.
> Clay already produces per-element pointer hits; the focus layer reads
> Clay directly. That keeps `InputState` a flat snapshot of raw device
> edges and avoids two sources of truth for "is the cursor over X".

---

## 7. Runtime integration notes (binding for every phase)

These constraints come from `src/react.cpp` and constrain every primitive
in this plan:

- **Every fiber re-runs every frame** (`react.cpp:363-389`). There is no
  "first render only" hook; `use_ref` and `use_effect` are the only
  cross-frame persistence primitives.
- **Hook count per fiber is fixed across frames and capped at 8**
  (`REACT_HOOKS_PER_FIBER` at `react.cpp:9`, invariant enforced at
  `react.cpp:248-251`). Every primitive in this plan declares its hook
  budget; totals are well under the ceiling.
- **`use_context` is not a hook slot** (`react.cpp:355-359`); it is a pure
  read of the context's descent-stack.
- **Effects fire after `Clay_EndLayout`** (`react.cpp:391-414`); any logic
  that needs Clay layout results sees them one frame late.
- **Fiber identity is parent-hashed** (`react.cpp:232-236`). A screen
  re-pushed with a new `entry_id` re-keys all descendants.
- **The runtime already tracks fiber liveness per frame** via the
  generation sweep (`react.cpp:382-389`). Focus management piggybacks on
  this rather than maintaining a parallel liveness list.

---

## Phases

Each phase is shippable in isolation; later phases compose on earlier ones.

---

### Phase F0 — Reorder the main loop so cleanup runs after render

**Goal:** eliminate a latent same-frame use-after-free between `react_end_frame()`'s cleanup callbacks and Clay's pending render commands. This is a small prerequisite that lands before any focus code.

#### The hazard

Today the main loop runs (`src/main.cpp:132-178`):

```
Clay_EndLayout()         → cmds (may reference SDL textures owned by fibers)
react_end_frame()        → unmount sweep + queued effects (may destroy those textures)
SDL_Clay_RenderClayCommands(cmds)   ← reads textures that no longer exist
```

A fiber that unmounts this frame can run a cleanup that destroys a resource still referenced by `cmds`. The concrete case in the tree today is `Image()` (`src/ui/components/image.cpp:117-129`): its `cancel_fetch` cleanup calls `SDL_DestroyTexture` on a handle that `cmds` is about to draw (`image.cpp:185-190`). Today this is rare — Image is mounted permanently — but every focus-driven screen swap will start creating same-frame unmounts of textured elements (modal close, screen pop), so the focus work *causes* this to fire if not addressed.

#### The fix

Move `react_end_frame()` past `SDL_Clay_RenderClayCommands`. Render commands consume `cmds` first; cleanup runs after.

```cpp
// src/main.cpp — patched loop
Clay_RenderCommandArray cmds = Clay_EndLayout();
SDL_Clay_RenderClayCommands(&rd, &cmds);   // render BEFORE cleanup
react_end_frame();                          // unmount sweep + queued effects
SDL_RenderPresent(g_sdl);
```

This is the *only* main-loop change F0 makes. The React API itself is untouched — no split of `react_end_frame` into `react_commit_layout` / `react_flush_post_render`, no parallel host runtime. The runtime stays whole; the loop just consumes the render commands before the cleanup callbacks run.

Update the per-frame data-flow diagram in §5 to match this order when shipping F0.

#### Scope

- [ ] Reorder `src/main.cpp` so `SDL_Clay_RenderClayCommands` runs before `react_end_frame`.
- [ ] Update `src/main.cpp:132-178` and §5's diagram together.
- [ ] No changes to `src/react.cpp` or `src/react.h`.

#### Acceptance

- [ ] Existing tests (`tests/react_runtime_tests.cpp:46-57` `run_frame` and successors) still pass — the harness doesn't observe the loop order; the public React contract is unchanged.
- [ ] A new smoke test: mount an `Image()` with a successful `fetch_image`, then in the next frame conditionally unmount it. Before F0, this can intermittently access a destroyed `SDL_Texture` mid-render; after F0, it must not.
- [ ] Effects that schedule subsequent-frame side effects (`use_effect` queue) still observe one-frame latency — F0 does not change *when* effects fire relative to the next frame's render, only relative to *this* frame's render.

---

### Phase F1 — Extend `InputState` with UI-nav edges

**Goal:** per-frame nav / confirm / cancel edges available to the UI tree,
on the same `InputState` snapshot the engine and existing UI already share.

Adds fields to the existing struct in `src/input.h`. The producer is
`main.cpp`, exactly like the existing `increment_counter` edges.

```cpp
// src/input.h (after F1)
struct InputState {
    // existing game-action edges stay as-is
    bool increment_counter, decrement_counter,
         toggle_counter, cycle_theme, fetch_image;

    // NEW: UI navigation edges (one frame on press / key-repeat)
    bool nav_up, nav_down, nav_left, nav_right;

    // NEW: confirm / cancel — edge + held + release
    bool confirm_pressed, confirm_down, confirm_released;
    bool cancel_pressed,  cancel_down,  cancel_released;
};
```

#### Why a single struct, not `UIInput`

A parallel `UIInput` struct + provider + context + hook would duplicate
every line of `input_provider.{h,cpp}` for the sake of a style rule that
already enforces itself by code review ("engine reads game fields, UI
hooks read nav fields"). The split would be style, not structure. The
fields go on the same struct that already flows through `InputContext`;
engine consumers ignore the new ones, UI hooks read them.

Pointer state is out entirely — Clay already produces it per-element via
`Clay_PointerOver` and `Clay_GetPointerData`; F3 reads Clay directly.

#### Three timings of "confirm" — why all three matter

| Field | True when | Used by |
|---|---|---|
| `confirm_pressed` | The frame the key/button goes down (single frame). | F3 keyboard path: marks the start of a press. |
| `confirm_down` | Every frame the key/button is held. | F3 keyboard path: drives the `pressed` / `active` visual flag. |
| `confirm_released` | The frame the key/button comes up (single frame). | F3 keyboard path: pairs with `confirm_pressed` for press-target match, fires `on_confirm`. |

> **Edge vs held semantics.** `main.cpp` is the single producer.
> `*_pressed` and `*_released` are true for exactly one frame each;
> `*_down` mirrors SDL's "key is held now" state. The
> `SDL_EVENT_KEY_DOWN` handler at `src/main.cpp:141` currently skips
> repeats; for `nav_*` the plan reverses that — keyboard repeat is the
> canonical way to hold a direction.

**Scope:**

- [ ] Extend `InputState` as above.
- [ ] `main.cpp` collects gamepad input via `SDL_OpenGamepad` /
  `SDL_GAMEPAD_BUTTON_*` and touch via `SDL_EVENT_FINGER_*`, alongside
  existing keyboard / mouse handling, and populates the new fields.
- [ ] Keyboard repeat (held arrow keys) is the canonical mapping for
  `nav_*` edges; gamepad d-pad mirrors that with SDL's repeat behavior.
- [ ] Pointer state stays where it is — `main.cpp` continues to call
  `Clay_SetPointerState`; the new fields do *not* mirror pointer position
  or button.
- [ ] Edge vs held semantics: `*_pressed` and `*_released` are true for
  exactly one frame; `*_down` is true every frame held. `main.cpp` is the
  single producer.

**Acceptance:**
- Debug overlay shows live nav / confirm / cancel values; pressing `A` on
  a gamepad, Enter on a keyboard, left-clicking, and tapping touch all
  produce `confirm_pressed = true` for exactly one frame and
  `confirm_down = true` for every frame held.
- `confirm_released` fires exactly once on release; same for cancel.
- `<Counter>` and other existing consumers of `use_context(&InputContext)`
  still compile and behave the same.

---

### Phase F2 — Focus model

**Goal:** every screen has exactly one focus owner; movement is
deterministic; `focusVisible` derivation is correct.

**This phase ships keyboard + gamepad confirm only.** Pointer-driven
confirm lands in F3 (it needs press-origin tracking and the
"slip-out / slip-back-in" rule).

#### What it adds, structurally

- A new context `FocusContext` (peer of `InputContext`).
- A new provider `<FocusScope>` — one fiber, one `use_ref` slot, holds a
  `FocusManager*`.
- A new hook `use_focusable({on_confirm, on_cancel?, disabled?})`.
- Optional `use_initial_focus(predicate)` for screen-specific starting focus.

#### Scope identity rule — key by entry, never by screen type

> **Rule.** A `<FocusScope>` is keyed by the **fiber identity of the screen
> stack entry that mounted it** — i.e. the per-push entry id — not by a
> stable typed identifier like `ScreenId::Pause`.

Why this matters: `react.cpp:232-236` derives every Clay element id by
hashing the parent chain. A scope keyed by `entry_id` produces fresh
descendant ids on every re-push, which is exactly what the live-set
fallback (below) needs to fire correctly when a screen is popped and later
re-pushed. A scope keyed by `ScreenId::Pause` would collide across pushes:
two simultaneously-mounted Pause screens (or a Pause that re-pushes
during the same generation) would resolve to the same `focused_id` slot
and silently leak focus from the old instance into the new one.

This is the same pattern `tests/react_runtime_tests.cpp:104-125` already
validates for keyed React siblings — sibling fibers with distinct keys
get distinct identities even when the component type is identical. Focus
scopes inherit that contract by construction; this rule just makes the
binding explicit so the screen-stack layer (`plan.md`) cannot accidentally
violate it by passing a typed key as the React key.

Practically: the screen stack must supply each push with a unique
`entry_id` (monotonically allocated per push, *not* `ScreenId`) and use
it as the `REACT_COMPONENT_KEY` for the screen's root component. The
`<FocusScope>` inside that screen then inherits the right identity for
free.

#### State that survives across frames

`FocusManager` is allocated via `use_ref` at the top of `<FocusScope>` and
persists across frames. It holds:

```cpp
// Lives in the use_ref slot of <FocusScope>
struct FocusManager {
    Clay_ElementId          focused_id;             // 0 = none
    Source                  last_focus_move_source; // Kbd|Pad|Mouse|Touch|None
    std::vector<Clay_ElementId> focusable_list;     // cleared+rebuilt every frame
};
```

- `focused_id` — persistent.
- `last_focus_move_source` — persistent; updated atomically with every
  focus move. Single source of truth for `focusVisible`. Edges live on
  `InputState`; persistent state lives on the manager.
- `focusable_list` — populated during render; nav reads it before this
  frame's repopulation.

#### The per-frame cycle inside one `<FocusScope>`

Each frame, in render order:

1. **FocusScope open.** Read `InputState.nav_* / confirm_*`. Resolve
   `focused_id` against the PREVIOUS frame's `focusable_list`. If
   `nav_up` etc. fires, move `focused_id` to the next entry. If
   `focused_id` is not in the list (focusable unmounted, screen re-keyed,
   first frame), fall back to the first entry — or
   `use_initial_focus(predicate)`'s result if the screen supplied one.
   Then clear the list.
2. **Children render.** Each `use_focusable(...)` call appends its
   `Clay_ElementId` to `focusable_list`, reads `Clay_PointerOver` for
   `hovered`, and fires `on_confirm` if `focused && confirm_pressed`
   (keyboard/gamepad path).
3. **FocusScope close.** `focusable_list` is now fully populated for the
   NEXT frame's nav processing.

One list, no swap. `use_focusable`'s registration is a call on the
manager pointer — not a hook, no slot consumed.

A new focusable that mounts this frame is reachable starting *next* frame —
a one-frame latency invisible at 60fps and the natural consequence of the
runtime model. Style code should not rely on `focused` being
eventually-consistent within frame 1.

#### Hook budget — non-negotiable under the 8-slot cap

| Primitive | Slots used | What for |
|---|---|---|
| `<FocusScope>` | 1 | `use_ref` — the `FocusManager`. |
| `use_focusable` | 1 | `use_ref` — F2 allocates it for press state; F3 populates it. |
| `<Button>` / `<Toggle>` / `<Selectable>` | 0 beyond `use_focusable` | Composition only. |

Total per focusable: **1 hook slot**. Of the 8-slot cap that
`react.cpp:9` hard-codes, that leaves 7 for any state a component wants
to keep on its own. Context reads (`InputContext`, `FocusContext`) are
not slots (`react.cpp:355-359`).

#### Why "live-set fallback" is the whole restoration story

The plan's strongest claim — and the one most worth vetting — is that
one rule covers every focus-restoration case:

> **Rule.** At `FocusScope` open, if `manager.focused_id` is *not* in
> last frame's `focusable_list`, replace it with `use_initial_focus()`'s
> result (or the first entry if not supplied).

| Scenario | Outcome under the rule |
|---|---|
| A focusable conditionally unmounts (the focused one). | Its id is missing from the list → fallback. Other items keep registering by parent-hashed id, so the fallback "first entry" is stable. |
| Pause stays mounted while Settings is on top. | Pause's `FocusManager` never ran nav this frame (gate is a screen-stack concern, not focus's). Its `focused_id` is unchanged, its list is unchanged on re-entry → no fallback needed. |
| A screen is fully unmounted and later re-pushed. | New `entry_id` means every descendant gets a new parent-hashed id (`react.cpp:232-236`). The old `focused_id` is by construction not in the new list → fallback fires. |
| First frame ever; no list yet. | Empty list → fallback (initial). Same code path. |

The reason this works is that **fiber identity is the only handle**;
there is no "name" or "key" the focus model could use to remember things
across remounts even if it wanted to.

Cross-screen nav-input gating (Pause shouldn't move its focus while
Settings is on top) is a screen-stack concern (`plan.md`), not a
focus-model concern.

#### Deriving `focusVisible`

```
focusVisible := (manager.last_focus_move_source ∈ { Keyboard, Gamepad })
```

Updated atomically with every focus move. Keyboard/gamepad nav writes
`Keyboard`/`Gamepad`; pointer interaction (F3) that focuses an element
writes `Mouse`/`Touch`. That single field is the only thing that decides
whether a focused element draws its outline.

#### `FocusableState` (returned by `use_focusable`)

```cpp
struct FocusableState {
    Clay_ElementId id;
    bool focused;
    bool focusVisible;
    bool hovered;
    bool pressed;       // populated in F3; false in F2
};
```

In F2 the hook:

- Allocates its `use_ref` press-state slot (unused until F3).
- Appends this frame's `Clay_ElementId` to `focusable_list`.
- Reads `Clay_PointerOver(id)` for `hovered`.
- Fires `on_confirm` when `focused && InputState.confirm_pressed && !disabled`.
  Keyboard / gamepad only; pointer confirm lands in F3.
- Returns `pressed = false`.

**Scope:**

- [ ] `FocusManager` struct + `<FocusScope>` provider via `use_ref` /
      `FocusContext`.
- [ ] `use_focusable` with the behavior above.
- [ ] Traversal: linear in `focusable_list` order. `nav_down` / `nav_up`
      cycle. `nav_left` / `nav_right` unbound by default; 2D screens
      override via `use_focus_traversal(strategy)`.
- [ ] `use_initial_focus(predicate)` lets a screen pick initial focus
      differently; otherwise the first entry of `focusable_list` becomes
      focused on the second frame the screen is alive.

**Acceptance:**
- Three stacked `<Button>`s: arrow keys, d-pad, and left-stick all move
  focus through them in registration order.
- `focusVisible` is `true` immediately after any keyboard / gamepad nav.
  Mouse interaction in F2 only updates `hovered`.
- A focusable that conditionally renders (toggled with a debug hotkey)
  causes no hook-count error; if the focused one disappears, focus falls
  back to the first entry.
- **Pause under Settings test:** focus the third button (Quit) on Pause.
  Push Settings (its own `<FocusScope>`). Pop Settings. Focus on Pause is
  still Quit — no restoration code required; Pause's `focused_id` was
  never touched while it stayed mounted under Settings.
- **Unmount-and-re-push test:** Quit-to-Title from Pause → start a new
  run → Pause again. Focus is on Resume (initial), confirming that a
  fully re-keyed subtree falls back via the live-set rule.

---

### Phase F3 — Pointer press + full interaction states

**Goal:** complete the five raw interaction states from `ui.md`, including
pointer-driven confirm with drag-off cancellation.

The hard problem F3 solves is "press began on me AND released on me" with
drag-off cancellation. It does this entirely with two persistent fields
per focusable, stored in the same `use_ref` slot F2 already allocated.

#### Per-element pointer state, read directly from Clay

Clay already provides everything needed:
- `Clay_PointerOver(id)` — is the pointer over this element this frame.
- `Clay_GetPointerData().state` — global pointer state
  (`PRESSED_THIS_FRAME` / `PRESSED` / `RELEASED_THIS_FRAME` / `RELEASED`).

There is no parallel state machine. Per-element press state collapses to
two persistent fields:

```cpp
struct PressState {
    Clay_ElementId pointer_press_origin; // 0 = none; set on PRESSED_THIS_FRAME
                                         //  while Clay_PointerOver(id)
    bool           key_held;             // confirm_down on this focused element
};
```

#### Decision table at `use_focusable` time

| Clay state | `over` | Action |
|---|---|---|
| `PRESSED_THIS_FRAME` | true, !disabled | set `press_origin = this.id`; set `manager.focused_id = this.id`; set source = Mouse/Touch. |
| `RELEASED_THIS_FRAME` | `press_origin == this.id && over && !disabled` | fire `on_confirm`; clear `press_origin`. |
| `RELEASED_THIS_FRAME` | otherwise | clear `press_origin` without firing. |
| `PRESSED` (held) | over && press_origin == this.id | aggregate `pressed` = true. |
| `PRESSED` (held) | !over (slip-out) | `pressed` = false, but `press_origin` stays — slip-back-in re-engages. |

#### Per-frame derivation in `use_focusable`

```cpp
auto pd  = Clay_GetPointerData();
bool over = Clay_PointerOver(state.id);

// Pointer path
if (pd.state == PRESSED_THIS_FRAME && over && !disabled) {
    press.pointer_press_origin      = state.id;
    manager->focused_id             = state.id;
    manager->last_focus_move_source = pointer_is_touch ? Touch : Mouse;
}
if (pd.state == RELEASED_THIS_FRAME) {
    if (press.pointer_press_origin == state.id && over && !disabled) {
        on_confirm();
    }
    press.pointer_press_origin = 0;
}

// Keyboard / gamepad path
bool was_held  = press.key_held;
press.key_held = focused && input->confirm_down && !disabled;
if (was_held && !press.key_held && focused
    && input->confirm_released && !disabled) {
    on_confirm();
}

// Aggregate
bool pointer_pressed = (press.pointer_press_origin == state.id) && over
                       && (pd.state == PRESSED_THIS_FRAME
                           || pd.state == PRESSED);
state.pressed = pointer_pressed || press.key_held;
state.hovered = over;
```

That is the entire interaction model. Press-target match ("press began on
me AND released on me") is preserved by checking
`press_origin == this.id && over` at release time. Slip-out clears
`pressed` (because `over` becomes false) but keeps `press_origin`, so
slip-back-in re-engages. Release-off-target clears `press_origin` without
firing.

The transition `was_held && !press.key_held && focused` is exactly "was
holding, is no longer holding, still focused" — which cleanly handles
"hold Enter, tab focus away mid-hold, release" by *not* firing (because
`focused` is false on the release frame).

#### Why this is contention-free

Clay hit-tests to a single topmost element per frame, so at most one
focusable in the tree sees `over == true`. The shared `manager.focused_id`
and `last_focus_move_source` writes have no race; every focusable's
`use_focusable` body runs synchronously in render order.

**`focusWithin` is omitted from `FocusableState`** until a real
nested-focusable consumer exists. Aliasing it to `focused` would be a
false guarantee.

**Scope:**
- [ ] Add the per-frame derivation above to `use_focusable`. `state.pressed`
      and `state.hovered` come from Clay reads; `press_origin` and
      `key_held` live in the F2-allocated `use_ref`.
- [ ] Debug overlay paints `focused / focusVisible / hovered / pressed`
      on the currently focused / hovered element.

**Acceptance:**
- Test button visibly transitions `targeted → active → confirm` under
  each input device: keyboard hold, gamepad A hold, mouse press-and-hold,
  touch press-and-hold.
- Mouse: press on button, drag off, release elsewhere — no `on_confirm`,
  `pressed` clears on drag-off.
- Mouse: press on button, drag off, drag back, release on button —
  `on_confirm` fires.
- Keyboard: hold Enter on focused button, tab focus away mid-hold,
  release — no `on_confirm` (focus loss clears `key_held`).
- Pointer click on element B while element A is focused: focus moves to
  B, `last_focus_move_source = Mouse`, no double-confirm.

---

### Phase F4 — Control states + visual derivation

**Goal:** semantic component conditions and the unified visual flags the
rest of the UI styles against.

```cpp
// src/ui/state/visual_state.h (proposed)
struct ControlState { bool checked, selected, disabled; };

struct VisualState {
    bool targeted;    // hovered || (focused && focusVisible)
    bool active;      // pressed
    bool chosen;      // selected || checked
    bool unavailable; // disabled
};

VisualState derive_visual_state(const FocusableState &f,
                                const ControlState   &c);
```

#### The falsifiable layering check

The plan claims (correctly, if components are written honestly) that
introducing a new visual flag — say, `pulse` — should require edits to
**exactly two files**: the header above, and whichever style function
consumes `VisualState`. If a primitive (`Button`, `Toggle`, `Selectable`)
has to change, then it's branching on raw interaction or control state — a
layering violation. This is the cheapest review check this plan has going
for it.

**Scope:**

- [ ] Control state is **per-component-kind**:
  - `<Button>`: none beyond focusable.
  - `<Toggle>`: adds `checked: bool` (caller prop) + `on_change(bool)`.
  - `<Selectable>`: adds `selected: bool` (caller prop).
  - All three accept `disabled: bool`.
- [ ] Land the derivation header above.
- [ ] Style functions take `VisualState`. Component bodies do not branch
      on raw interaction or control flags. Convention enforced by code
      review, not the type system.

**Acceptance:**
- **Falsifiable layering test:** adding an arbitrary new visual flag
  (e.g. `pulse`) to `VisualState` and consuming it in
  `style_for_visual_state` requires changes to *exactly two files*:
  `visual_state.h` and the style fn. If any of `<Button>`, `<Toggle>`,
  `<Selectable>` need editing, the visual layer is leaking into the
  primitive layer. Verified by diff inspection on a throwaway branch.
- **Behavioral disabled test:** flipping `disabled = true` on a focused
  button: `targeted` clears, `unavailable` sets, `on_confirm` does not
  fire on Enter, and held confirm does not engage `pressed`.

---

## 8. How a component actually uses these states

The previous four phases defined `FocusableState`, `ControlState`, and
`VisualState` as types — but no component body has actually consumed them
yet. Here is the whole loop in one place, with nothing left implicit.

### The whole of `<Button>`, end to end

```cpp
// Proposed: src/ui/components/button.cpp
struct ButtonProps {
    const char *label;
    void (*on_confirm)(void *user);
    void  *user;
    bool   disabled;
};

void Button(ButtonProps p) {
    REACT_COMPONENT_BEGIN("Button") {
        // 1. Interaction state — reads InputState + Clay pointer + focus model.
        FocusableState f = use_focusable({
            .on_confirm = p.on_confirm,
            .user       = p.user,
            .disabled   = p.disabled,
        });

        // 2. Control state — purely caller-supplied. Plain button: empty.
        ControlState c = { .checked = false, .selected = false,
                           .disabled = p.disabled };

        // 3. Visual state — pure derivation, defined once for the whole UI.
        VisualState v = derive_visual_state(f, c);

        // 4. Style → Clay element. The body branches on `v` only.
        ButtonStyle s = button_style_for(v);

        CLAY({
            .id              = f.id,
            .layout          = { .padding = CLAY_PADDING_ALL(12),
                                 .childAlignment = { CLAY_ALIGN_X_CENTER,
                                                     CLAY_ALIGN_Y_CENTER } },
            .backgroundColor = s.bg,
            .border          = { .width = CLAY_BORDER_OUTSIDE(s.border_w),
                                 .color = s.border },
            .cornerRadius    = CLAY_CORNER_RADIUS(6),
        }) {
            CLAY_TEXT(cs(p.label),
                CLAY_TEXT_CONFIG({ .textColor = s.fg, .fontSize = 16 }));
        }
    } REACT_COMPONENT_END();
}
```

That is the whole component. Notice what's *not* there: no
`if (focused)`, no `if (hovered)`, no per-device branches, no
hit-testing. Each of the four layers appears exactly once. Behavior comes
from `use_focusable` and the table inside `button_style_for` — both pure
data.

### Where all the conditional logic lives

```cpp
// src/ui/state/visual_state.cpp (proposed)
VisualState derive_visual_state(const FocusableState &f,
                                const ControlState   &c) {
    return {
        .targeted    = f.hovered || (f.focused && f.focusVisible),
        .active      = f.pressed,
        .chosen      = c.selected || c.checked,
        .unavailable = c.disabled,
    };
}

// src/ui/components/button_style.cpp (proposed)
struct ButtonStyle { Clay_Color bg, fg, border; uint16_t border_w; };

ButtonStyle button_style_for(const VisualState &v) {
    if (v.unavailable) return { dim_bg,     dim_fg, transparent, 0 };
    if (v.active)      return { pressed_bg, fg,     accent,      2 };
    if (v.targeted)    return { hover_bg,   fg,     accent,      2 };
    return                    { base_bg,    fg,     transparent, 0 };
}
```

Four return statements, in priority order. If a future spec says "chosen
buttons look different", you add a branch here — and nothing else changes.
The primitive, the hook, the caller all stay put. This is what the F4
falsifiable layering test is checking.

### State matrix — same component, eight worlds

Every cell below is *the same `<Button>` source*. What changes is what
the runtime tells it. Read each row as: raw flags → derived visual flags
→ which branch of `button_style_for` wins.

| # | Scenario | Raw | Visual | Branch |
|---|---|---|---|---|
| 1 | idle | — | — | default |
| 2 | mouse hover | hovered | targeted | `if (targeted)` |
| 3 | clicked, mouse left | focused, !focusVisible | — | default — no outline by design |
| 4 | keyboard / gamepad focus | focused, focusVisible | targeted | `if (targeted)` |
| 5 | holding Enter | focused, focusVisible, pressed | targeted, active | `if (active)` |
| 6 | mouse held over | focused, hovered, pressed | targeted, active | `if (active)` |
| 7 | mouse-press slipped off | focused, !hovered, !pressed; press_origin still set; release without re-enter → no fire | — | default |
| 8 | disabled + hovered | hovered, disabled | targeted, unavailable | `if (unavailable)` — wins |

> **Why cells 5 and 6 look identical.** They are — by design. The visual
> layer collapses keyboard-hold and mouse-press into `active`, so styling
> code doesn't see the difference. Whether the user got there via
> gamepad-A or mouse-click, the button looks and "feels" the same.

> **Why cell 3 has no outline.** When a click moves focus to an element,
> `last_focus_move_source = Mouse`, so `focusVisible` is `false`.
> `targeted = hovered || (focused && focusVisible)`; with the mouse moved
> off and `focusVisible` false, both terms are false. This is the entire
> reason `focusVisible` exists — to suppress the keyboard-style outline
> on mouse-focus.

### Toggle reuses everything except one branch

```cpp
struct ToggleProps {
    const char *label;
    bool   checked;                    // caller owns the truth
    void (*on_change)(void *user, bool v);
    void  *user;
    bool   disabled;
};

void Toggle(ToggleProps p) {
    REACT_COMPONENT_BEGIN("Toggle") {
        FocusableState f = use_focusable({
            .on_confirm = toggle_flip,         // trampoline: calls on_change(!checked)
            .user       = &p,
            .disabled   = p.disabled,
        });

        ControlState c = { .checked  = p.checked,
                           .selected = false,
                           .disabled = p.disabled };
        VisualState  v = derive_visual_state(f, c);   // chosen = checked
        ToggleStyle  s = toggle_style_for(v);

        CLAY({ .id = f.id, ..., .backgroundColor = s.bg, ... }) {
            CLAY_TEXT(cs(p.label),
                CLAY_TEXT_CONFIG({ .textColor = s.fg, .fontSize = 16 }));
            if (v.chosen) CLAY_TEXT(cs("✓"), ...);
        }
    } REACT_COMPONENT_END();
}
```

Differences from `Button`: `c.checked` comes from a prop, the body draws
a checkmark when `v.chosen`, and `toggle_style_for` has additional
branches for chosen+targeted vs chosen+idle. The hook, the derivation,
the layering — all the same.

### What the screen body actually writes

```cpp
// Inside a screen, nested under one <FocusScope>
Button({ .label = "Resume",  .on_confirm = resume_run  });
Button({ .label = "Restart", .on_confirm = restart_run });
Button({ .label = "Quit",    .on_confirm = quit_to_title });

int *fullscreen = use_state_int(0);
Toggle({ .label     = "Fullscreen",
         .checked   = *fullscreen,
         .on_change = [](void *u, bool v) { *(int *)u = v; },
         .user      = fullscreen });
```

No focus state, no nav fields, no Clay ids in the screen body. The same
code runs under keyboard, mouse, gamepad, and touch — every per-device
difference is absorbed in `use_focusable` + `derive_visual_state` below it.

### What still lives at the screen level

- The `<FocusScope>` that wraps the menu.
- Optional `use_initial_focus(predicate)` if "first entry" isn't the
  right starting focus.
- Any `checked` / `selected` state, because the `<Toggle>` and
  `<Selectable>` primitives are stateless — the screen owns the truth.

### Escape hatches — making arbitrary content interactive

The shipped primitives cover the common cases. For everything else — a
card, a tile, a non-rectangular hit region, an existing Clay box you want
to retrofit — there are three escape hatches, increasing in power.

#### 1. Hover only — read Clay directly

```cpp
// Zero hooks consumed; no focus participation. Works in any context.
CLAY({ .id = CLAY_ID_LOCAL("Card"), ...,
       .backgroundColor = Clay_PointerOver(CLAY_ID_LOCAL("Card"))
                          ? hi : base }) {
    ...
}
```

Good for "this box brightens on mouseover". Doesn't enter focus
traversal; no keyboard/gamepad path. If you only need hover and have no
notion of "activate this with Enter", this is the right tool — reaching
for `use_focusable` would be overhead.

#### 2. `<Focusable>` — arbitrary Clay body, full interaction state

```cpp
Focusable({ .on_confirm = expand_panel, .disabled = false },
    [&](FocusableState f) {
        ControlState c = { .disabled = false };
        VisualState  v = derive_visual_state(f, c);

        CLAY({ .id = f.id,
               .layout = { .padding = CLAY_PADDING_ALL(16) },
               .backgroundColor = v.targeted ? hi_panel : base_panel,
               .border = { .width = CLAY_BORDER_OUTSIDE(v.targeted ? 2 : 0),
                           .color = accent },
               .cornerRadius = CLAY_CORNER_RADIUS(8) }) {
            CLAY_TEXT(cs("anything you want here"), ...);
            CLAY({ .id = CLAY_ID_LOCAL("Inner"), ... }) { ... }
            Image();
        }
    });
```

`<Focusable>` is a slot component — it follows the `SlotComponent`
pattern documented in `react.h`. It calls `use_focusable` for you, then
invokes the lambda with the resulting `FocusableState`. The lambda owns
everything inside Clay — layout, children, even whether to skip
`derive_visual_state` and branch on raw fields (`f.focused`,
`f.pressed`, `f.hovered`) directly. Hook budget is still 1 slot.

#### 3. Author your own primitive against `use_focusable`

```cpp
// A "Card" primitive — different layout, different style table.
void Card(CardProps p) {
    REACT_COMPONENT_BEGIN("Card") {
        FocusableState f = use_focusable({ .on_confirm = p.on_pick,
                                           .user = p.user });
        ControlState c = { .selected = p.is_current,
                           .disabled = false };
        VisualState  v = derive_visual_state(f, c);
        CardStyle    s = card_style_for(v);   // your own style table

        CLAY({ .id = f.id, ..., .backgroundColor = s.bg, ... }) {
            // portrait, name, stats, whatever the card is
        }
    } REACT_COMPONENT_END();
}
```

Nothing in the plan privileges `<Button>` — it is itself a `use_focusable`
caller, the same way your new primitive would be. Authoring a new one is
a new component file plus a new style table; the focus/interaction
machinery is shared identically.

### What the plan does *not* let you do

| Not supported | Why / what to do instead |
|---|---|
| Override `<Button>`'s internals from outside. | Button is a closed primitive — `button_style_for` is its style table, hard-coded. To get different visuals, write a new primitive against `use_focusable` (or use `<Focusable>`). No render-prop "give Button a custom body" mechanism is proposed. |
| Retrofit raw Clay (outside any React fiber) into focus. | Hooks need fiber storage (`react.cpp:53-62`). A bare `CLAY({...})` outside any component can read `Clay_PointerOver` for hover, but cannot focus, press, or fire confirm. Wrap it in a `REACT_COMPONENT_BEGIN`/`END` first. |
| Two focusables sharing one Clay element. | One id = one entry in `focusable_list`. Nested interactive behavior (a card with a focusable inside) means two ids on two Clay elements. This is also the reason the plan defers `focusWithin` until a real nested-focusable consumer exists. |
| Promote a fiber's outer Clay element (the one `REACT_COMPONENT_BEGIN` creates) into the focusable hit-target without thinking about ids. | `f.id` returned by `use_focusable` is the id the caller is expected to attach to a Clay element inside the component body. Two ids on the outer fiber-CLAY and the inner focusable-CLAY is fine; the same id on both would collide. Worth flagging in §15 (Open questions) as an implementation detail to pin down. |

---

### Phase F5 — Primitive components + first real consumers

**Goal:** ship the primitives and migrate the first `plan.md` screens
that exist.

- **`<Focusable>`** — slot wrapper. Renders a Clay element with an id;
  passes its `FocusableState` to a child lambda. Escape hatch for one-off
  needs.
- **`<Button>`** — `<Focusable>` + label + `on_confirm` + style. Zero
  hooks beyond `use_focusable`.
- **`<Toggle>`** — `<Focusable>` + caller-supplied `checked: bool` +
  `on_change(bool)`. State lives in the caller; primitive is stateless.
- **`<Selectable>`** — Leaf only — *no* group wrapper. The screen owns
  the "which-of-N is selected" prop. Focus traversal is the same single
  `FocusManager` walking the full list. If a future screen ever has two
  independent selectable groups on the same surface, revisit then.

**Migration order.** Title → Play (one button) before anything else,
because it's the smallest end-to-end test. Pause/ChooseUpgrade/Settings
migrate as their `plan.md` phases land. Settings' master-volume slider is
explicitly out of scope — it needs a separate continuous-input primitive.

**Scope:**
- [ ] `<Focusable>` — raw wrapper exposing `FocusableState` to a child
      slot lambda. Escape hatch for one-off needs.
- [ ] `<Button>` — composes `<Focusable>` + `on_confirm` + label + style.
- [ ] `<Toggle>` — `<Focusable>` + `checked` prop + `on_change(bool)`.
- [ ] `<Selectable>` — `<Focusable>` + `selected` prop. **Leaf primitive,
      no group wrapper.**
- [ ] Migrate the Title screen Play button (`plan.md` Phase 3) to
      `<Button>`.
- [ ] When `plan.md` Phase 6 lands: migrate Pause Resume / Quit-to-Title
      to `<Button>`; verify gamepad / keyboard navigation through the
      menu.
- [ ] When `plan.md` Phase 8 lands: migrate ChooseUpgrade cards. Prefer
      `<Button>` (one-shot fire) unless a persistent selection-then-confirm
      flow is actually required by design.
- [ ] When `plan.md` Phase 9 lands: migrate Settings fullscreen toggle to
      `<Toggle>`. Master-volume slider is out of scope (a separate
      primitive, separate plan).

**Acceptance:**
- Title → Play works on mouse, keyboard, gamepad, and touch with no
  per-device code in the Title screen body.
- (Post `plan.md` Phase 6) Pause menu fully navigable on every input
  class.
- (Post `plan.md` Phase 8) ChooseUpgrade picks an upgrade end-to-end via
  gamepad without touching the mouse.

---

## 9. Single-frame walkthrough

Setup: three buttons on a Pause screen. Focus is on the second
(`Restart`). User presses gamepad <kbd>A</kbd> (confirm). This is the
frame that fires.

```
Frame N — gamepad A pressed while Restart is focused

main.cpp           : SDL: A_pressed_this_frame → confirm_pressed=true,
                                                 confirm_down=true
react              : G.frame++ (gen counter)
FocusScope         : scope open: focused_id ∈ list (Restart) ✓
                     — no fallback; clear list
use_focusable(R)   : append Restart.id; focused = true;
                     confirm_pressed → on_confirm()
use_focusable(Q)   : append Quit.id; focused = false; no-op
react_end_frame    : unmount sweep: all 3 buttons alive; queued effects: none
render             : SDL_RenderPresent — frame visible
```

`on_confirm()` body: runs synchronously, in render phase — may call
setters (`use_state_int` writes), schedule a navigation, etc. No
post-commit delay.

> **Key observation.** `on_confirm` runs *during the same render pass* as
> the input edge. There is no "fired after commit" half-frame. If
> `on_confirm` calls a `game.nav.push` or sets state, that state is
> visible to siblings that render after it within the same frame — but
> anything *before* it in the tree sees the old state. Standard React
> semantics, but worth saying out loud given this runtime re-runs every
> fiber every frame.

---

## 10. Screen-transition walkthrough

Two scenarios that exercise the live-set rule. Same Pause screen as
above. Two outcomes depending on whether Pause stays mounted.

### A. Pause under Settings (stack push)

1. Pause is rendering; `FocusManager.focused_id = Quit.id`.
2. User opens Settings — Settings is pushed on top of Pause in the screen
   stack, but Pause's subtree *stays mounted*.
3. The screen stack gates nav input: Settings owns the focus consumer
   this frame. Pause's `FocusScope` still runs but reads `input` with nav
   fields treated as muted (mechanism is screen-stack-side, not
   focus-side).
4. Pause's `focusable_list` rebuilds with the same ids as before.
   `focused_id` is in the list. Nothing changes.
5. User closes Settings. Pause is on top again. First frame back:
   `focused_id = Quit.id` is still in the freshly rebuilt list.
   **Focus is on Quit. No restoration code ran.**

### B. Quit-to-Title → restart → Pause again

1. User picks Quit. Pause unmounts; its `FocusManager` fiber is destroyed
   by the gen sweep.
2. Title screen renders. Run begins; later Pause is opened again.
3. The new Pause is a new fiber: same component, but `entry_id` is
   different so its descendants have new parent-hashed ids
   (`react.cpp:232-236`).
4. First frame of new Pause: `focused_id = 0` on the fresh
   `FocusManager`. Not in list → fallback fires →
   `use_initial_focus(...)` picks `Resume.id`.
5. Focus is on Resume — the documented "initial focus" — and the plan
   paid zero bytes for explicit restoration logic.

---

## 11. Ownership boundaries — who reads, owns, sends

| Concern | Owner | Reader(s) | Lifetime |
|---|---|---|---|
| Raw SDL keyboard/mouse/gamepad/touch | `main.cpp` | — | per event |
| `InputState` (nav + game-action edges) | `main.cpp` populates; `InputProvider` publishes | any fiber via `use_context(&InputContext)` | 1 frame |
| Pointer hit-test | Clay (set by `main.cpp:163`) | any fiber via `Clay_PointerOver(id)` / `Clay_GetPointerData()` | 1 frame |
| `focused_id`, source, `focusable_list` | `FocusManager` in the parent `<FocusScope>`'s `use_ref` | siblings inside that scope only | across frames; destroyed on scope unmount |
| Per-focusable press state | That focusable's own `use_ref` | nobody else | across frames; destroyed on unmount |
| Control state (`checked`, `selected`) | The component caller (parent or screen) | the primitive via props | caller-defined |
| Action dispatch (`on_confirm`, `on_change`) | Caller-supplied lambda | fires during render, inside the primitive's `use_focusable` call | — |

### Reusable primitives vs screen-specific code

| Reusable (in `src/ui/...`) | Screen-specific |
|---|---|
| `<FocusScope>`, `use_focusable`, `use_initial_focus`, `<Focusable>`, `<Button>`, `<Toggle>`, `<Selectable>`, `derive_visual_state`, style functions. | Which screens get a `FocusScope`, what the initial-focus predicate is, which buttons/toggles a screen renders, what `on_confirm` does, the screen-stack rule that decides which scope actually consumes nav input each frame. |

---

## 12. Invariants the implementation has to hold

- **I1.** Every focusable's `Clay_ElementId` is stable across frames as
  long as its parent chain is unchanged. Violated iff a parent
  `REACT_COMPONENT_BEGIN`/`_KEY` uses a non-stable key.
- **I2.** `use_focusable` declares the same hook count every frame
  (1 slot). Required by `react.cpp:248-251`.
- **I3.** No focusable's `id` appears twice in `focusable_list` within a
  frame. (If it does, that's a duplicate-id bug at the call site; Clay
  would already collide on it.)
- **I4.** Exactly one focusable has `focused == true` per `FocusScope`
  per frame, with the sole exception of the first frame after a fresh
  mount where it can be zero before fallback resolves.
- **I5.** `last_focus_move_source` changes only when `focused_id` changes
  (or pointer interaction re-asserts focus on the same id).
- **I6.** `on_confirm` fires at most once per press-release pair, and
  never if `disabled` was true at either edge.
- **I7.** Engine state (`World`, pools, `game.*`) is not read or written
  from the focus layer — verified by `grep`, not by type.

---

## 13. What can go wrong

| Failure mode | Why it happens | Mitigation in the plan |
|---|---|---|
| Stale `focused_id` after a focusable unmounts. | Manager keeps the id; list rebuilds without it. | Live-set rule at scope open replaces it with initial focus. |
| Two focusables share the same Clay id. | Caller used the wrong `CLAY_ID_LOCAL` or omitted a keyed sibling. | Not specifically handled here — Clay already errors on collision; a duplicate would cause two `focusable_list` entries with the same id and erratic `nav_*`. Worth a runtime assert. |
| Focusable hidden by another Clay element (z-order). | `Clay_PointerOver` returns false for the lower one. | Keyboard/gamepad focus is unaffected (it ignores hit-testing); pointer never reaches the lower element, which is the correct behavior. |
| Modal transition while a press is in flight. | Element with `press_origin = self` unmounts before release. | Its fiber is destroyed by the gen sweep, taking `press_origin` with it. `on_confirm` cannot fire post-unmount. Correct by construction. |
| Screen change mid-press from keyboard. | `key_held = true`, but on the new screen the element is gone. | New screen's `FocusScope` has its own manager; old focusable's fiber and press state are destroyed. Confirm cannot cross screens. |
| Scrolling: focused element moves outside the viewport. | Still focused; pointer hit-test now misses. | Out of scope. Plan does not promise auto-scroll-to-focus; that is the screen's job. |
| Text input (typing) collides with confirm/cancel. | Enter is also the confirm key. | Out of scope (the plan explicitly defers IME / text input). Until F5 ships a text-input primitive, screens must avoid embedding both. |
| Disabled control while focused. | Caller flips `disabled = true` mid-press. | Hook checks `!disabled` at every edge. Held confirm clears `key_held`; pointer release will not fire. F4's behavioral disabled test exercises this exact case. |
| Layout change re-orders focusables. | Same ids, different list order. | `nav_down` walks the list in registration order — so the user sees the new order. If preserving the visual ordering matters, screens must register in visual order. |
| A new focusable mounts and we want immediate focus. | It isn't in last frame's list; nav this frame sees the old set. | One-frame latency. `use_initial_focus(predicate)` can target it on the next frame. At 60Hz this is invisible. |

---

## 14. How to verify

### Runtime checks the implementation should add

- **Hook-count assert** already exists at `react.cpp:248-251`. Adding
  `use_focusable` must not perturb sibling hook counts.
- Optional: assert `focusable_list` contains no duplicates at scope close.
- Debug overlay: paint `focused / focusVisible / hovered / pressed` over
  the targeted element (specified in F3's scope). Same overlay shows the
  live `InputState` nav fields (specified in F1's acceptance).

### Concrete acceptance scenarios (one per phase)

1. **F1.** Press <kbd>A</kbd>/<kbd>Enter</kbd>/click/tap on each device —
   the debug overlay shows `confirm_pressed = true` for exactly one
   frame, `confirm_down = true` while held, `confirm_released = true`
   for exactly one frame on release.
2. **F2.** Three stacked buttons; `nav_down` cycles
   keyboard/gamepad/left-stick through them in registration order;
   `focusVisible` turns on on the first keyboard nav and off the next
   time the mouse moves and clicks.
3. **F3.** Mouse press on button → drag off → release elsewhere: no
   `on_confirm`, `pressed` goes false on drag-off. Drag back, release on:
   `on_confirm` fires.
4. **F4 layering test.** On a throwaway branch, add a `pulse` flag to
   `VisualState` and consume it in one style function. `git diff` must
   touch exactly two files. If any of `Button`/`Toggle`/`Selectable`
   appears in the diff, the layering is broken.
5. **F4 disabled test.** Focus a button, set `disabled = true`:
   `targeted` clears, `unavailable` sets, holding confirm does not engage
   `pressed`.
6. **F5.** Title → Play works on mouse, keyboard, gamepad, touch with
   zero per-device branches in the Title screen body.

---

## 15. Open questions / assumptions to validate

1. **SDL3 key-repeat producing nav edges every frame is the right
   model.** The current loop at `main.cpp:142` explicitly drops repeats.
   F1 needs to keep that behavior for the existing action keys
   (`increment_counter` etc.) while accepting repeats for arrow keys.
   The exact split — "repeats on for arrows, off for everything else" —
   needs deciding before F1 lands.
2. **Cross-scope nav gating is "a screen-stack concern".** True for the
   current single-screen demo, but there is no screen-stack primitive in
   `src/` yet. Whoever wires F2 into the first multi-screen build needs
   to define which scope is the "active" consumer. Until then,
   `<FocusScope>` exists but only one scope at a time is meaningful.
3. **Provider fibers consuming hook slots.** `theme_provider__enter`
   already does this. If `<FocusScope>` ever nests inside a component
   that is itself at the hook cap, the budget needs revisiting. With the
   current 8-slot cap and a 1-slot scope, we're fine for now.
4. **`std::vector` in `FocusManager`.** The plan writes it that way;
   check whether the codebase has a house style against STL containers
   in UI hot paths (this list is touched every frame).
5. **Touch single-finger semantics.** F1 says touch maps to confirm via
   finger events but doesn't pin down the rule for multi-finger / pinch —
   the plan defers to "single touch only". This is fine as a constraint,
   but it should be enforced (e.g. ignore second finger) rather than
   left implicit.
6. **Press-target match for keyboard.** The pointer path tracks
   `press_origin`. The keyboard path uses
   `was_held && !key_held && focused` as a proxy. This is correct when
   focus is stable across the press, but if some external code moves
   focus while `confirm_down` is still true, `key_held` goes false on
   the new element (it isn't focused yet) and on the old element next
   frame. Worth a targeted test in F3 acceptance.
7. **`focusWithin` deferred — is that fine for any planned screen?**
   ChooseUpgrade cards in particular: if a card contains an inner
   focusable later, callers can't ask "is the card focus-within" without
   it. Defer is the right call *only* if no near-term screen needs it.

---

## 16. Glossary

| Term | Meaning here |
|---|---|
| **Fiber** | One node in the React-style hook table; identified by a Clay element id; holds up to 8 hook slots (`react.cpp:53-62`). |
| **Hook slot** | One persistent storage cell on a fiber. Allocated by the first call to `use_state_int` / `use_effect` / `use_ref` in a given position; the slot's kind is fixed for the life of the fiber. |
| **Context** | A descent-stack of pointers (`react.cpp:340-359`). Push on `PROVIDE`; pop on scope exit. `use_context` is a free read. |
| **Clay element id** | Parent-hashed string + index. Identical id every frame as long as parent chain and child position are stable. |
| **Generation sweep** | At `react_end_frame`, any fiber whose `generation` didn't match this frame's counter is destroyed (`react.cpp:382-389`) — including any attached `use_ref` heap. This is how unmount is detected. |
| **`FocusScope`** | A provider fiber that holds one `FocusManager`. One per screen. |
| **`FocusManager`** | The persistent struct in a `FocusScope`'s `use_ref`. Holds `focused_id`, `last_focus_move_source`, and the per-frame `focusable_list`. |
| **Live-set fallback** | The rule that, if `focused_id` isn't in last frame's `focusable_list` at scope open, focus falls back to `use_initial_focus()` (or the first entry). |
| **Source** | Enum tracked by `FocusManager`: Keyboard, Gamepad, Mouse, Touch, None. Drives `focusVisible`. |
| **Edge / held / release** | Three "timings" of a button. `*_pressed` and `*_released` are single-frame edges; `*_down` is the held state. All three live on `InputState` per F1. |
| **Press-target match** | "Press began on me AND released on me." Pointer path tracks it via `press_origin`; keyboard path tracks it via `key_held` + focus stability. |

---

## Definition of done

- All five `ui.md` interaction states observable in the debug overlay
  (`focusWithin` reserved until a nested-focusable consumer exists).
- All three control states wired through their component primitives.
- All four visual states drive every styled component; the F4 falsifiable
  layering test passes (new flag → two-file diff).
- Title, Pause, ChooseUpgrade, and Settings are fully operable on
  keyboard / mouse / gamepad / touch with no per-device branches in
  component code.
- No engine file (`engine_tick`, `World`, any pool, `game.nav`, any
  `game.*` subsystem) is touched. The only cross-React-tree change is
  the platform-layer SDL gamepad / touch handling in `main.cpp`
  populating the new `InputState` fields (F1).
- `docs/archive/ui.md` can be retired or moved out of `archive/` once
  this plan lands.
