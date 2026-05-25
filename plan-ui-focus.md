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
`main.cpp`) and a second per-frame snapshot (`UIInput`) threaded into
`App` alongside the existing `InputState` — see F1.

Anything described here can ship before, during, or after any phase in
`plan.md` as long as it lands before the screen that needs it.

---

## Scope

**In scope:**
- Normalizing **UI navigation** input (directional, confirm, cancel, pointer)
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
- Per-game-action input (movement, fire). `engine_tick` keeps reading the
  existing `InputState` for those, unchanged. Gamepad axes/buttons that
  become candidates for in-world controls (left-stick movement, right-trigger
  fire) go on `InputState`, not `UIInput`. `UIInput` only carries
  UI-navigation intent.
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
- `WorldContext` is not extended. New contexts (`UIInputContext`,
  `FocusContext`) are independent providers.
- The platform-layer addition in F1 (gamepad/touch SDL handling in `main.cpp`,
  a second per-frame `UIInput` snapshot threaded into `App`) is the *one*
  non-React-tree change this plan makes; nothing beyond it crosses into
  engine-owned state.

---

## Layering

```
┌─ Visual states (targeted / active / chosen / unavailable)   ← pure fn
├─ Interaction states (focused / focusVisible / focusWithin /  ← hook
│   hovered / pressed)
├─ Control states (checked / selected / disabled)              ← props
├─ Focus model (per-screen owner + traversal)                  ← context + hook
└─ UI-input normalization (dirX/dirY, confirm, cancel,         ← context
    pointer)
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
  budget; the totals are well under the ceiling, but the ceiling is a
  one-line bump if a future primitive needs more.
- **`use_context` is not a hook slot** (`react.cpp:355-359`); it is a pure
  read of the context's descent-stack. Informal references to "a `use_context`
  hook" in this plan do not consume a slot.
- **Effects fire after `Clay_EndLayout`** (`react.cpp:391-414`); any logic
  that needs Clay layout results sees them one frame late.
- **Fiber identity is parent-hashed** (`react.cpp:232-236`). A screen
  re-pushed with a new `entry_id` (per `plan.md` Phase 6) re-keys all
  descendants — `Clay_ElementId`s captured across an unmount are invalid.
  Phase F6 below relies on this fact rather than fighting it.

---

## Phases

Each phase is shippable in isolation; later phases compose on earlier ones.

---

### Phase F1 — UI input normalization

**Goal:** a per-frame `UIInput` snapshot capturing navigation intent across
all four device classes, separate from the existing game `InputState`.

**Platform-layer note.** This phase adds SDL gamepad and touch handling to
`main.cpp` and threads a second snapshot into `App`. It does not change
`engine_tick`, does not modify `InputState`, and does not touch `World`. It
is the one platform-layer carve-out from the otherwise React-tree-only
nature of this plan.

**`UIInput` shape** (new file, e.g. `src/ui_input.h`; distinct from
`InputState` in `src/input.h:10-16`, which keeps game-action bools and is
unchanged):

```cpp
struct UIInput {
    // Nav direction edges (true for one frame on press / key-repeat).
    bool nav_up, nav_down, nav_left, nav_right;

    // Confirm/cancel as edge + held + release. Edge enables one-shot
    // fire; held drives `pressed`/`active` UI during the hold; release
    // pairs with edge to implement press-target match (F3).
    bool confirm_pressed, confirm_down, confirm_released;
    bool cancel_pressed,  cancel_down,  cancel_released;

    // Pointer (mouse + single-touch unified).
    bool  pointer_present;
    float pointer_x, pointer_y;
    bool  pointer_down;
    bool  pointer_pressed;
    bool  pointer_released;
};
```

`UIInput` is a **pure per-frame snapshot of platform events** — no
cross-frame state lives on it. The "which device is in charge" concept
(sometimes implemented as a sticky `last_source` field) lives on the
`FocusManager` in F2, where it has exactly one consumer (`focusVisible`
derivation).

**Scope:**

- [ ] Add the `UIInput` struct above.
- [ ] `main.cpp` collects gamepad input via `SDL_OpenGamepad` /
  `SDL_GAMEPAD_BUTTON_*` and touch via `SDL_EVENT_FINGER_*`, alongside
  existing keyboard/mouse handling. It produces a `UIInput` next to the
  existing `InputState` and threads both into `App`.
- [ ] `<UIInputProvider>` mirrors the shape of `<InputProvider>` at
  `src/ui/providers/input_provider.{h,cpp}`. `use_ui_input()` returns
  `const UIInput *` via `use_context(&UIInputContext)` (not a hook slot;
  see runtime notes).
- [ ] Edge vs held semantics: `*_pressed` and `*_released` are true for
  exactly one frame each; `*_down` is true every frame the input is held.
  `main.cpp` is the single producer.
- [ ] Gamepad axes/buttons that are candidates for in-world controls stay
  off `UIInput`. If/when they're needed for gameplay, they go on
  `InputState`.

**Acceptance:**
- Debug overlay shows live `UIInput` values; pressing `A` on a gamepad,
  Enter on a keyboard, left-clicking, and tapping touch all produce
  `confirm_pressed = true` for exactly one frame and `confirm_down = true`
  for every frame the input is held.
- `confirm_released` fires exactly once on release; same for cancel and
  pointer.

---

### Phase F2 — Focus model (keyboard/gamepad confirm only)

**Goal:** every screen has exactly one focus owner; movement is
deterministic; `focusVisible` derivation is correct.

**This phase ships keyboard + gamepad confirm only.** Pointer-driven
confirm — which requires the press-track state machine to handle drag-off
cancellation — lands in F3.

**Per-frame registration model (binding).**

The runtime re-runs every fiber every frame. The focus manager is
specified explicitly against that:

- `FocusManager` is allocated via `use_ref` at the top of `<FocusScope>`
  and persists across frames. It holds:
  - `focused_id: Clay_ElementId` (or none) — persistent.
  - `last_focus_move_source: Source ∈ {Keyboard, Gamepad, Mouse, Touch, None}`
    — persistent; updated atomically with every focus move.
  - `committed_list: vector<Clay_ElementId>` — last frame's registration,
    finalized at end of the previous frame. Persistent across frames.
  - `frame_list: vector<Clay_ElementId>` — this frame's registration,
    cleared at `<FocusScope>` open, populated during render.
  - `captured_id: Clay_ElementId` — used by F6 for restoration.
- `use_focusable(...)` pushes its `Clay_ElementId` into `frame_list` each
  render. This is a call on the manager pointer obtained from
  `FocusContext`, **not a hook** — it consumes no slot.
- At `<FocusScope>` close (the bottom of the screen body, before the
  provider exit), the manager swaps `committed_list <- frame_list` and
  validates `focused_id`: if absent from the just-committed list, fall
  back to the first entry, or to `use_initial_focus(predicate)`'s result
  if the screen supplied one.
- **Nav events apply against `committed_list`** (the *previous* frame's
  registration). A focusable that mounts this frame is reachable starting
  next frame — a one-frame latency that is invisible at 60fps and acceptable
  for keyboard nav.
- Focusables that conditionally render keep stable identity because
  `Clay_ElementId` is parent-hashed from the call site
  (`react.cpp:232-236`). Their index in `committed_list` shifts when
  siblings come and go; that is fine because nav operates on the ID
  list, not on indices.

**`focusVisible` derivation (single source of truth).**

```
focusVisible := (manager.last_focus_move_source ∈ { Keyboard, Gamepad })
```

Updated atomically with every focus move:
- Keyboard or gamepad nav moves focus → `last_focus_move_source = Keyboard|Gamepad`.
- A pointer interaction that focuses an element (lands in F3 with the
  press-track state machine) → `last_focus_move_source = Mouse|Touch`.

No clear-on-press rule. No second store. `UIInput` carries no sticky
source field.

**Hook budget per primitive (binding against `REACT_HOOKS_PER_FIBER = 8`).**

| Primitive       | Hook slots used                                            |
|-----------------|------------------------------------------------------------|
| `<FocusScope>`  | 1 `use_ref` (the `FocusManager`); F6 adds 1 `use_effect` via `use_screen_lifecycle` → 2 total post-F6 |
| `use_focusable` | 1 `use_ref` (press-track state — allocated here, populated in F3) |
| `<Button>`      | 0 beyond `use_focusable`                                   |
| `<Toggle>`      | 0 beyond `use_focusable`                                   |
| `<Selectable>`  | 0 beyond `use_focusable`                                   |

The F6 addition is the same slot in the same order on every frame (the
hook is unconditionally called from `<FocusScope>` once F6 lands), so the
`react.cpp:248-251` invariant holds.

Context reads (`UIInputContext`, `FocusContext`) are not slots
(`react.cpp:355-359`). If a future primitive ever needs more than the
ceiling allows, raise `REACT_HOOKS_PER_FIBER` — a one-line constant bump —
rather than working around it.

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

- Allocates its `use_ref` press-track slot (unused until F3).
- Registers this frame's `Clay_ElementId` into `frame_list`.
- Reads Clay pointer hover for `hovered`.
- Fires `on_confirm` when `focused && UIInput.confirm_pressed && !disabled`.
  Keyboard / gamepad path only; pointer confirm lands in F3.
- Returns `pressed = false`.

**Scope:**

- [ ] `FocusManager` struct as above; `<FocusScope>` provider that
  allocates it via `use_ref` and exposes it through `FocusContext`.
- [ ] `use_focusable` with the shape and behavior above.
- [ ] Traversal: linear in `committed_list` order. `nav_down` / `nav_up`
  cycle. `nav_left` / `nav_right` unbound by default; a screen with a 2D
  layout overrides via `use_focus_traversal(strategy)`.
- [ ] Initial focus: first entry in `committed_list` is focused once that
  list is non-empty (starting on the second frame the screen is alive).
  `use_initial_focus(predicate)` lets a screen pick differently.
  Consequence: frame 1 of any screen renders with no focused element;
  `focused = false` on every focusable that first frame. This is a
  one-frame paint, not a bug, and is invisible at 60fps. Style code
  should not rely on `focused` being eventually-consistent within frame 1.

**Acceptance:**
- Three stacked `<Button>`s on a test screen: arrow keys, d-pad, and
  left-stick all move focus through them in registration order.
- `focusVisible` is `true` immediately after any keyboard or gamepad
  nav. Mouse interaction in F2 only updates `hovered` (pointer-driven
  focus changes are F3).
- A focusable that conditionally renders (toggled with a debug hotkey)
  does not cause a hook-count error in `react.cpp:248-251`, because its
  registration is a manager call, not a hook.

---

### Phase F3 — Pointer press-track + full interaction states

**Goal:** complete the five raw interaction states from `ui.md`, including
the press-track state machine needed for pointer-driven confirm with
drag-off cancellation.

**Press-track state machine (binding).**

`use_focusable` already allocated a `use_ref` press-track slot in F2.
F3 populates it:

```cpp
struct PressTrack {
    enum { Idle, KeyHeld, PointerHeld } mode;
    Clay_ElementId press_target;  // element where pointer_pressed landed
    bool           pressed;
};
```

Transitions, evaluated each render:

| Event                                            | Pre-condition                                    | Result                                                                                       |
|--------------------------------------------------|--------------------------------------------------|----------------------------------------------------------------------------------------------|
| `UIInput.confirm_pressed && focused`             | `mode == Idle`                                   | `mode = KeyHeld`, `pressed = true`                                                           |
| `UIInput.confirm_released && focused`            | `mode == KeyHeld`                                | fire `on_confirm` (if `!disabled`), `mode = Idle`, `pressed = false`                         |
| focus lost                                       | `mode == KeyHeld`                                | `mode = Idle`, `pressed = false`, **no fire**                                                |
| `UIInput.pointer_pressed && hovered`             | `mode == Idle`                                   | move focus to this element, `last_focus_move_source = Mouse\|Touch`, `mode = PointerHeld`, `press_target = this.id`, `pressed = true` |
| `UIInput.pointer_released && hovered`            | `mode == PointerHeld && press_target == this.id` | fire `on_confirm` (if `!disabled`), `mode = Idle`, `pressed = false`                         |
| pointer leaves                                   | `mode == PointerHeld`                            | `pressed = false`, `press_target` retained                                                   |
| pointer re-enters                                | `mode == PointerHeld && press_target == this.id && pointer_down` | `pressed = true`                                                            |
| `UIInput.pointer_released` not on this           | `mode == PointerHeld`                            | `mode = Idle`, `press_target = 0`, **no fire**                                               |

"Press-target match" semantics (DOM `click`-style): a pointer press that
began on this element and ends on this element fires confirm. Slip-out
then slip-back-in is allowed. Slip-out and release elsewhere cancels.

The transition table is evaluated for **every focusable** each render.
"Focus lost" is the condition `mode == KeyHeld && !focused` observed at
the start of a render — no separate event needed. This naturally handles
"hold Enter on A, click B" — A's row sees `!focused` and clears its
KeyHeld; B's row sees `Idle` and enters PointerHeld.

Clay's pointer hit-testing returns a single topmost element per frame, so
at most one focusable observes `hovered == true` in a given frame; the
`pointer_pressed && hovered` row therefore fires on at most one focusable
per frame and the shared `manager.focused_id` / `last_focus_move_source`
writes have no contention.

The `pointer_pressed` row is also where the F2 single-store
`last_focus_move_source` update happens for pointer-driven focus moves —
keeping the rule "every focus move sets the source" honest across all
input classes.

**Scope:**
- [ ] Implement the transition table inside `use_focusable`. The
      `pressed` field of `FocusableState` now reflects `track.pressed`.
- [ ] `focusWithin` is **omitted from `FocusableState`** until a real
      consumer lands. Aliasing it to `focused` would be structurally
      false (the whole point of `focusWithin` is "true when a descendant
      is focused and the container is not"); a no-op alias invites
      consumers to read a guarantee that isn't there. The field comes
      back when a nested-focusable consumer exists, alongside the
      descendant-aggregation pass that makes it correct.
- [ ] Debug overlay (toggle hotkey) paints `focused / focusVisible /
      hovered / pressed` on the currently focused / hovered element.

**Acceptance:**
- Test button visibly transitions `targeted → active → confirm` under
  each input device: keyboard hold, gamepad A hold, mouse press-and-hold,
  touch press-and-hold.
- Mouse: press on button, drag off, release elsewhere — no `on_confirm`,
  `pressed` returned to false on drag-off.
- Mouse: press on button, drag off, drag back, release on button —
  `on_confirm` fires.
- Keyboard: hold Enter on focused button, tab focus away mid-hold,
  release — no `on_confirm` (focus-lost-while-held cancels).
- Pointer click on element B while element A is focused: focus moves to
  B, `last_focus_move_source = Mouse`, `focusVisible` correctly clears
  for the outline derivation, no double-confirm fires.

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
- [ ] Style functions take `VisualState`. Component bodies do not branch on
  raw interaction or control flags. Convention enforced by code review,
  not the type system.

**Acceptance:**
- **Falsifiable layering test:** adding an arbitrary new visual flag
  (e.g. `pulse`) to `VisualState` and consuming it in
  `style_for_visual_state` requires changes to *exactly two files*:
  `visual_state.h` and the style fn. If any of `<Button>`, `<Toggle>`,
  `<Selectable>` need editing, the visual layer is leaking into the
  primitive layer and the layering is violated. Verified by diff
  inspection on a throwaway branch.
- **Behavioral disabled test:** flipping `disabled = true` on a focused
  button: `targeted` clears, `unavailable` sets, `on_confirm` does not
  fire on Enter, and `confirm_down`-driven `pressed` does not engage
  (the press-track state machine refuses to leave `Idle`).

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
      job (the parent knows its option set; a `<SelectableGroup>`
      wrapper would either duplicate traversal — violating F2's "one
      focus owner per screen" — or add nothing). If a future screen has
      two independent selectable groups on the same surface, revisit then.
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

### Phase F6 — Focus restoration across screen lifecycle

**Goal:** focus survives push / pop of the engine-driven screen stack
correctly, *when the screen subtree stays mounted across the transition*.

**Depends on:** `plan.md` Phase 7 (lifecycle events).

**Restoration model (binding).**

- Restoration applies **only while the screen's fiber subtree stays
  mounted**. `plan.md` Phase 6 (lower entries stay mounted under upper
  ones) makes this the common case: Pause is pushed over Gameplay,
  Settings over Pause; Pause's subtree remains mounted throughout the
  Settings detour.
- A screen that is fully popped and later re-pushed gets a new
  `entry_id`, which re-keys all descendant `Clay_ElementId`s via
  `react.cpp:232-236`. Any previously-captured ID is meaningless. The
  re-mounted screen starts at its `use_initial_focus` (or the first
  entry of `committed_list` if no predicate was supplied). Restoration
  does not attempt to bridge the unmount gap.
- Restoration is by `Clay_ElementId`. Valid because, while the subtree
  stays mounted, the parent-hashed Clay IDs of its descendants are
  stable. Per `plan.md` Phase 7, `on_focus` fires when the entry reaches
  `entered`, and `use_screen_lifecycle` is itself a wrapper over
  `use_effect`, which the runtime flushes *after* `react_end_frame`
  (`react.cpp:391-414`). By the time the `on_focus` callback runs, this
  frame's `committed_list` is already finalized. The check is therefore
  against the **currently-committed** `committed_list` (i.e. the one
  just produced by this frame's render). If `manager.captured_id` is
  present in it, the effect writes `manager.focused_id = captured_id`,
  which the next render observes. Otherwise fall back.
- Restoration does **not** override `last_focus_move_source`. Whatever
  device popped the upper screen (e.g. a mouse click on a Back button)
  remains the source of truth; `focusVisible` reflects that. This avoids
  the "force `focusVisible = true` on restore" rule that would have
  contradicted F2's single-store derivation.

**Scope:**
- [ ] Inside `<FocusScope>`, use `use_screen_lifecycle({on_blur, on_focus})`
      to:
      - `on_blur`: snapshot `manager.focused_id` into `manager.captured_id`.
      - `on_focus`: if `captured_id` is in the currently-committed
        `committed_list` (i.e. the list this frame's render just
        produced — see "Restoration model" above for why this is the
        right snapshot at effect-flush time), write
        `manager.focused_id = captured_id`. The next render observes the
        restored focus. Else fall back to `use_initial_focus` (or first
        entry).
- [ ] `use_initial_focus(predicate)` is honored on first mount *and* as
      the restoration fallback when `captured_id` is no longer in
      `committed_list`.

**Acceptance:**
- **Falsifiable restoration test:** on the Pause menu, focus the third
  button (Quit). Push Settings (which has its own focusables). Pop
  Settings. Focus on Pause is **Quit**, not Resume. Removing the
  restoration code from `<FocusScope>` causes this test to fail (focus
  reverts to the initial — Resume), which isolates restoration from the
  passive "still mounted" path that would otherwise mask its absence.
- Quit-to-Title from Pause → start a new run → Pause again: focus is on
  Resume (initial), not Quit — confirms the "unmount drops captured ID"
  rule.

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
  `game.*` subsystem) is touched. The only cross-React-tree change is the
  platform-layer SDL gamepad/touch handling added to `main.cpp` in F1.
- `docs/archive/ui.md` can be retired or moved out of `archive/` once
  this plan lands.
