# Plan: UI focus + interaction-state model

A sister plan to `plan.md`. Pure UI-layer. Lands `docs/archive/ui.md` —
input-agnostic navigation and the interaction / control / visual state
vocabulary — inside the React/Clay runtime, without touching the engine seam.

---

## Why separate from plan.md

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

## Scope

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

## Non-overlap with plan.md (binding)

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

## Layering

```
┌─ Visual states (targeted / active / chosen / unavailable)   ← pure fn
├─ Interaction states (focused / focusVisible / focusWithin /  ← hook
│   hovered / pressed)
├─ Control states (checked / selected / disabled)              ← props
├─ Focus model (per-screen owner + traversal)                  ← context + hook
└─ Input sources                                                ← existing
    - nav / confirm / cancel edges on `InputState` (F1)
    - pointer hover + state via Clay (`Clay_PointerOver`,
      `Clay_GetPointerData`)
```

Each layer reads only from the layer below it. Component bodies branch on
**visual** state only — never on raw interaction or control state — so
styling code is uniform regardless of input device or semantic role.

---

## Runtime integration notes (binding for every phase)

These constraints come from `src/react.cpp` and constrain every primitive in
this plan:

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

### Phase F1 — Extend `InputState` with UI-nav fields

**Goal:** per-frame nav / confirm / cancel edges available to the UI tree,
on the same `InputState` snapshot the engine and existing UI already share.

**Why one struct, not two.** A separate `UIInput` struct + provider +
context + hook would parallel `InputState`'s entire pipeline
(`input_provider.{h,cpp}`, `InputContext`, `use_context(&InputContext)`)
for the sake of a code-review rule that already enforces itself ("engine
reads game fields, UI hooks read nav fields"). The split would be style,
not structure. Pointer state is out entirely — Clay already produces it
per-element via `Clay_PointerOver` and `Clay_GetPointerData`; F3 reads
Clay directly.

**Fields to add to `InputState`** (in `src/input.h`):

```cpp
struct InputState {
    // existing game-action fields stay as-is
    // ...

    // UI navigation edges (one frame on press / key-repeat).
    bool nav_up, nav_down, nav_left, nav_right;

    // Confirm / cancel as edge + held + release. Edge enables one-shot
    // fire; held drives `pressed`/`active` UI; release pairs with edge
    // for press-target match (F3).
    bool confirm_pressed, confirm_down, confirm_released;
    bool cancel_pressed,  cancel_down,  cancel_released;
};
```

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

**Per-frame model.**

`FocusManager` is allocated via `use_ref` at the top of `<FocusScope>` and
persists across frames. It holds:

- `focused_id: Clay_ElementId` (or none) — persistent.
- `last_focus_move_source: Source ∈ {Keyboard, Gamepad, Mouse, Touch, None}`
  — persistent; updated atomically with every focus move. Single source of
  truth for `focusVisible`. Edges live on `InputState`; persistent state
  lives on the manager.
- `focusable_list: vector<Clay_ElementId>` — populated during render; nav
  reads it before this frame's repopulation.

Each frame, in render order:

1. **FocusScope open:** nav input is processed against the existing
   `focusable_list` (last frame's population). If `nav_up` etc. fires,
   move `focused_id` to the next entry. If `focused_id` is not in the
   list (focusable unmounted, screen re-keyed, first frame), fall back to
   the first entry — or `use_initial_focus(predicate)`'s result if the
   screen supplied one. Then clear the list.
2. **Children render:** each `use_focusable(...)` call appends its
   `Clay_ElementId` to `focusable_list`.
3. **FocusScope close:** list is now fully populated for next frame's
   nav processing.

One list, no swap. `use_focusable`'s registration is a call on the
manager pointer — not a hook, no slot consumed.

A new focusable that mounts this frame is reachable starting *next* frame
— a one-frame latency invisible at 60fps and the natural consequence of
the runtime model. Style code should not rely on `focused` being
eventually-consistent within frame 1.

**`focusVisible` derivation.**

```
focusVisible := (manager.last_focus_move_source ∈ { Keyboard, Gamepad })
```

Updated atomically with every focus move: keyboard / gamepad nav writes
`Keyboard|Gamepad`; pointer interaction that focuses an element (F3)
writes `Mouse|Touch`.

**Live-set fallback subsumes the "focus restoration" problem.**

The single rule "if `focused_id` is not in `focusable_list` at FocusScope
open, fall back to initial" covers every scenario the original plan had a
separate restoration phase for:

- **Conditional unmount:** focus lands on the next valid entry.
- **Subtree stays mounted across a stack push** (Pause under Settings per
  `plan.md` Phase 6): `focused_id` never changed, list never changed,
  focus stays where it was. *No restoration code needed.*
- **Screen fully unmounted then re-pushed:** new `entry_id` re-keys all
  descendants, the previous `focused_id` is invalid by construction, fall
  back fires correctly.

Cross-screen nav-input gating (Pause shouldn't move its focus while
Settings is on top) is a screen-stack concern (`plan.md`), not a
focus-model concern.

**Hook budget per primitive.**

| Primitive       | Hook slots                                                       |
|-----------------|------------------------------------------------------------------|
| `<FocusScope>`  | 1 `use_ref` (the `FocusManager`)                                 |
| `use_focusable` | 1 `use_ref` (press state — allocated here, populated in F3)      |
| `<Button>`      | 0 beyond `use_focusable`                                         |
| `<Toggle>`      | 0 beyond `use_focusable`                                         |
| `<Selectable>`  | 0 beyond `use_focusable`                                         |

Context reads (`InputContext`, `FocusContext`) are not slots
(`react.cpp:355-359`).

**`FocusableState`** (returned by `use_focusable({on_confirm, on_cancel?, disabled?})`):

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

**Per-element pointer state, read directly from Clay.**

Clay already provides everything needed:
- `Clay_PointerOver(id)` — is the pointer over this element this frame.
- `Clay_GetPointerData().state` — global pointer state
  (`PRESSED_THIS_FRAME` / `PRESSED` / `RELEASED_THIS_FRAME` / `RELEASED`).

There is no parallel state machine. Per-element press state collapses to
two persistent fields, stored in the `use_ref` slot the F2 `use_focusable`
already allocated:

```cpp
struct PressState {
    Clay_ElementId pointer_press_origin; // 0 = none; set on PRESSED_THIS_FRAME
                                         //  while Clay_PointerOver(id)
    bool           key_held;             // confirm_down on this focused element
};
```

Per-frame derivation in `use_focusable`:

```cpp
auto pd  = Clay_GetPointerData();
bool over = Clay_PointerOver(state.id);

// Pointer path
if (pd.state == PRESSED_THIS_FRAME && over && !disabled) {
    press.pointer_press_origin    = state.id;
    manager->focused_id           = state.id;
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

Clay hit-tests to a single topmost element per frame, so at most one
focusable sees `over == true`; the shared `manager.focused_id` /
`last_focus_move_source` writes have no contention.

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

**Derivation header** (`src/ui/state/visual_state.h`):

```cpp
struct ControlState {
    bool checked;
    bool selected;
    bool disabled;
};
struct VisualState {
    bool targeted;     // hovered || (focused && focusVisible)
    bool active;       // pressed
    bool chosen;       // selected || checked
    bool unavailable;  // disabled
};
VisualState derive_visual_state(const FocusableState &, const ControlState &);
```

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

### Phase F5 — Primitive components + first real consumers

**Goal:** ship the primitives and migrate the first `plan.md` screens
that exist.

**Scope:**
- [ ] `<Focusable>` — raw wrapper exposing `FocusableState` to a child
      slot lambda. Escape hatch for one-off needs.
- [ ] `<Button>` — composes `<Focusable>` + `on_confirm` + label + style.
- [ ] `<Toggle>` — `<Focusable>` + `checked` prop + `on_change(bool)`.
- [ ] `<Selectable>` — `<Focusable>` + `selected` prop. **Leaf primitive,
      no group wrapper.** Focus traversal is the screen's single
      `FocusManager`; routing "which one is now selected" is the parent's
      job. If a future screen ever has two independent selectable groups
      on the same surface, revisit then.
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
