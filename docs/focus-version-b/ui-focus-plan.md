# Plan: UI Focus And Input State

This plan integrates the archived UI focus contract from
`docs/archive/ui.md` as a standalone UI-runtime effort. It intentionally stays
separate from `plan.md`.

In this document, **focus means UI element focus only**: which button, row,
field, toggle, tab, or list item receives keyboard/gamepad activation and draws
a focus indicator. It is not engine focus, gameplay focus, simulation focus,
screen lifecycle focus, pause semantics, or system tick gating.

## Boundary

UI focus is a host-runtime adapter for React-over-Clay, similar to the DOM
focus/event layer in browser React. It owns input targeting, traversal,
capture, and focus visuals. It does not own component state, domain actions,
screen lifecycle, or a second React-like reconciliation model.

It may read:

- The current per-frame input snapshot.
- The current rendered UI tree.
- The active UI focus scope, such as the top input-accepting screen entry.
- Each registered interactable element's stable ID, role, state, and navigation
  metadata.

It may write:

- UI-local focus state.
- UI-local pointer capture state.
- UI-local last-input-modality state.
- Short-lived host activation dispatch records.

It must not write:

- Gameplay state directly.
- Engine tick gates.
- Screen stack state directly.
- Screen lifecycle phase.
- World or actor data.

If an activated control needs to mutate application state, the component passes
a current-frame activation handler, normally produced by a domain action hook
such as `use_pause_actions()` or `use_upgrade_actions()`. The focus runtime may
delay dispatching that handler until after render-command consumption, but it
must not reinterpret the handler as a typed UI command bus. React-style
providers, hooks, keyed identity, effects, and event handlers remain the
canonical mechanisms for component state and actions.

This is the anti-reinvention rule for the whole plan: whenever the question is
"who owns identity, component-local state, effects, lifecycle, or domain
actions?", the answer is the existing React-shaped runtime and the domain hooks
from `plan.md`. The focus layer only supplies the host services that Clay lacks:
hit testing, traversal, capture, focus-visible state, and post-render dispatch
timing.

## Vocabulary

Carry forward the archived state model directly:

```cpp
struct UiInteractionState {
    bool focused;
    bool focus_visible;
    bool focus_within;
    bool hovered;
    bool pressed;
};

struct UiControlState {
    bool checked;
    bool selected;
    bool disabled;
};

struct UiVisualState {
    bool targeted;     // hovered || focus_visible
    bool active;       // pressed
    bool chosen;       // selected || checked
    bool unavailable;  // disabled
};
```

Rules:

- `focused` is the single keyboard/gamepad activation target inside a focus
  scope.
- `focus_visible` is true when focus should be drawn, usually after keyboard,
  gamepad, or programmatic focus movement. Pointer focus does not automatically
  force a visible ring.
- `focus_within` is derived for containers from descendant focus.
- `hovered` comes from pointer/touch hit testing in the same coordinate space
  Clay uses for layout and rendering.
- `pressed` means the element is currently being activated by pointer, touch,
  keyboard confirm, or gamepad confirm.
- `disabled` suppresses focus traversal and activation.
- `selected` and `checked` are semantic component state, not interaction state.

## Runtime Pieces

### `UiInputSnapshot`

Normalize raw platform input once per frame before the UI tree is declared.

It should include:

- Layout dimensions in UI coordinates.
- Pointer position, pointer down, pointer pressed this frame, pointer released
  this frame.
- Touch mapped into the same pointer channel for single-touch UI activation.
- Scroll delta and whether pointer-drag scrolling is enabled for this frame.
- Delta time for Clay scroll momentum and focus-repeat timing.
- Keyboard focus commands: next, previous, up, down, left, right, confirm,
  cancel.
- Gamepad focus commands mapped to the same command vocabulary.
- Text input events, kept separate from focus movement.
- Last input modality: pointer, keyboard, gamepad, touch.

The engine can consume its own gameplay input snapshot separately. UI focus
does not need to own movement, firing, AI, or pause simulation rules.

### `UiFocusManager`

Owns focus state for the UI runtime:

```cpp
struct UiElementKey {
    UiFocusScopeId scope;
    uint32_t clay_id_hash;
};

struct UiScopeFocusState {
    Optional<UiElementKey> focused;
    Optional<UiElementKey> restore_target;
    bool initial_focus_consumed;
};

struct UiScopeFocusRecord {
    UiFocusScopeId scope;
    UiScopeFocusState state;
};

struct UiFocusManager {
    UiFocusScopeId active_scope;
    UiScopeFocusRecord scope_states[UI_MAX_FOCUS_SCOPES];
    int scope_state_count;
    Optional<UiElementKey> pointer_capture;
    Optional<UiElementKey> confirm_pressed;
    UiInputModality last_modality;
};
```

Focus and capture state stores app-owned keys, not full `Clay_ElementId` values.
`Clay_ElementId` contains a string slice and other frame-local data; it is valid
for Clay calls and diagnostics during the frame that created it, but the
persistent identity is only `{scope, clay_id_hash}`.

`UiFocusScopeId` must be React/key aware. For a screen or modal, the scope is
derived from the keyed screen entry instance, such as `ScreenEntry::entry_id`,
through a focus-scope provider around that entry's body. It is not just the
screen type. Two `Pause` entries stacked at once must have independent focus,
capture, and restoration state, the same way keyed React siblings have
independent hook state.

The per-scope focus map is persistent UI state keyed by mounted
`UiFocusScopeId`. `pointer_capture` and `confirm_pressed` are active-scope
transients and are cleared on active-scope changes, unmount, disable, or release.
The focus runtime does not infer screen lifecycle to prune `scopes`; the
focus-scope provider reports the set of mounted keyed scopes for the completed
frame, and absent scopes are removed after the current frame is reconciled.

Responsibilities:

- Keep one focused/restorable element record per mounted focus scope.
- Move focus with keyboard/gamepad commands.
- Restore focus when returning to a scope if the old element still exists.
- Pick a deterministic default focus when a scope first becomes active.
- Clear focus when the focused element disappears or becomes disabled.
- Track pointer capture from press to release.
- Track keyboard/gamepad confirm press and release against the same focused key.

### `UiInteractableRegistry`

Every primitive that can receive input registers itself during declaration:

```cpp
struct UiNavMetadata {
    int order;
    UiNavGroupId group;
    Optional<UiRect> previous_bounds;
    Optional<UiElementKey> up;
    Optional<UiElementKey> down;
    Optional<UiElementKey> left;
    Optional<UiElementKey> right;
};

struct UiInteractable {
    UiElementKey key;
    Clay_ElementId clay_id;       // frame-local only
    UiFocusScopeId scope;
    UiRole role;
    UiControlState control;
    UiNavMetadata nav;
    bool accepts_focus;
    bool accepts_pointer;
    UiActivationHandler on_activate; // token into frame-owned handler storage
};
```

`UiActivationHandler` is a host event target, not a domain command. It is a
small token into frame-owned handler storage:

```cpp
struct UiActivationHandler {
    uint32_t token;
};

struct UiFrameActivation {
    void (*dispatch)(const void *payload);
    uint32_t payload_offset;
    uint32_t payload_size;
};
```

The handler is usually created by a primitive caller from the current React
render, often from a domain action hook result. Creating the handler copies the
small dispatch payload into the UI frame arena and stores only a token in the
interactable registry. The payload must not point at render-stack locals. Large
or long-lived data is addressed by stable IDs and resolved by the domain hook's
owning subsystem during dispatch. The frame arena lives until the activation
queue drains and is then cleared; persistent focus state stores only the
`UiElementKey`.

The first pass may use only deterministic linear order for next/previous
movement. Directional navigation uses this resolution order:

1. Explicit directional override (`up/down/left/right`) when provided.
2. Bounds-aware candidate search from the previous completed frame.
3. Deterministic linear order fallback inside the same group.

The registry is frame-local. Focus state is persistent; registered interactables
are not.

### `UiActivationQueue`

Activation is queued, not executed inside layout declaration.

Pointer/touch:

1. Press inside an enabled interactable captures that element.
2. While captured, the element reports `pressed`.
3. Release inside the same enabled element queues activation.
4. Release elsewhere clears capture without activation.

Keyboard/gamepad:

1. Confirm while an enabled focused element is focused reports `pressed`.
2. Confirm release queues activation.

The queue drains after `Clay_EndLayout()` and after the renderer has consumed
the command array for the frame. This keeps current-frame render resources and
custom payloads alive through rendering even when an activation closes or
replaces the UI that produced those commands.

Queued activations carry the `UiElementKey` that produced the activation and a
`UiActivationHandler` token into the completed frame's handler arena. The drain
step validates that the key still belongs to the active scope and was registered
as enabled in the completed frame before dispatching the copied payload. This
queue exists only to preserve the React/Clay host timing boundary; it is not a
universal command queue, it does not define domain verbs, and it must not grow
long-lived callback ownership separate from React/domain hooks.

## Frame Lifecycle

The UI host has one authoritative per-frame order. The focus runtime plugs into
the React/Clay frame; it does not create a parallel lifecycle coordinator:

1. Platform code builds `UiInputSnapshot` from raw SDL input. This includes
   layout dimensions, pointer/touch state, scroll delta, delta time,
   keyboard/gamepad focus commands, text input, and last input modality.
2. `ui_begin_frame(snapshot)` freezes the previous completed registry for
   traversal and bounds lookups, clears the current-frame registry and activation
   queue, sets the active focus scope supplied by the UI owner, and clears
   stale focus/capture keys that were absent or disabled in the previous
   registry.
3. Keyboard/gamepad focus movement is applied against the previous completed
   registry. Confirm press records the active scope's focused key in
   `confirm_pressed` for visual state; confirm release is not queued until the
   current frame has completed and the focused key is validated against the
   current registry.
4. Clay frame input is updated before layout:
   ```cpp
   Clay_SetLayoutDimensions(snapshot.layout_dimensions);
   Clay_SetPointerState(snapshot.pointer_position, snapshot.pointer_down);
   Clay_UpdateScrollContainers(
       snapshot.enable_drag_scroll,
       snapshot.scroll_delta,
       snapshot.delta_seconds);
   ```
5. `Clay_BeginLayout()` starts declaration. UI code declares the complete tree.
   Focus-aware primitives register current-frame interactables, derive visual
   state from `UiFocusManager`, and use Clay hover/pointer data only to update
   UI focus/capture state or enqueue host activation dispatch. They do not
   mutate application state.
6. `Clay_EndLayout()` produces render commands. The focus runtime reconciles
   focus and capture against the completed current-frame registry, chooses a
   default focus target if the active scope needs one, validates queued
   pointer/touch activations, validates keyboard/gamepad confirm release,
   snapshots current registry bounds for the next frame, and prunes per-scope
   focus state using the mounted focus scopes reported by providers. React
   cleanup and unmount sweeping have not run yet.
7. Rendering consumes the Clay command array while all current-frame UI render
   resources are still alive.
8. The React runtime runs post-render work: effect flush, active cleanup, and
   unmount sweeping. The current `react_end_frame()` API runs this immediately
   after `Clay_EndLayout()`; the React runtime boundary should be split or moved
   so cleanup that can release UI render resources lives after render-command
   consumption. This is a React host phase change, not focus-specific
   lifecycle machinery.
9. The `UiActivationQueue` drains by dispatching the validated current-frame
   activation handlers from the frame-owned handler arena. Those handlers may
   call domain action hooks' verbs.

No domain action mutation caused by UI activation happens before step 9. No
focus traversal uses a half-built current-frame registry. No React cleanup may
release resources that are still referenced by the current frame's Clay render
command array.

## Primitive Contract

Primitive controls should expose semantic state and receive derived visual
state from the focus runtime.

Examples:

```cpp
void ui_button(Clay_ElementId id, UiButtonProps props, UiActivationHandler on_activate);
void ui_toggle(Clay_ElementId id, UiToggleProps props, UiActivationHandler on_activate);
void ui_list_item(Clay_ElementId id, UiListItemProps props, UiActivationHandler on_activate);
```

Each primitive:

- Registers an interactable with a stable ID.
- Reads the derived `UiInteractionState`.
- Combines interaction and semantic control state into `UiVisualState`.
- Draws hover/focus/pressed/selected/checked/disabled consistently.
- Queues activation dispatch rather than mutating application state directly.

Primitives do not return "clicked" booleans. A return value consumed during
declaration invites `if (ui_button(...)) { mutate(); }`, which violates the
queued-dispatch boundary. If a caller needs read-only styling or diagnostics, it
may ask for derived `UiInteractionState`; activation still flows only through
`UiActivationQueue`.

Components should not manually ask "am I hovered and is Enter down?" or branch
on primitive activation. They should call primitives or small focus-aware
helpers with activation handlers returned from their domain action hooks.

## Focus Scopes

A focus scope is a UI boundary for traversal and activation. It is not engine
focus.

Expected scope owners:

- Root title/menu screen.
- Pause menu.
- Settings screen.
- Upgrade modal.
- Any future text-entry modal.

Scope identity comes from the active UI tree's React/keyed instance, not from
these labels. A focus-scope provider wraps each screen or modal body and supplies
the exact scope id for that entry. Initial focus requests and restore-on-uncover
state are keyed by that id.

Rules:

- Only the top input-accepting scope receives keyboard/gamepad focus movement.
- Lower screens may remain rendered and mounted, but their interactables are not
  focusable or activatable while covered by a modal scope.
- Exiting scopes draw their transition but do not accept new activation.
- A scope may request an initial focus target by stable element ID.
- A scope may restore its previous focused target when uncovered if the same
  keyed scope instance is active again.

This is UI input ownership only. Engine systems still use their own explicit
rules for whether gameplay ticks, pauses, or resumes.

## Phases

### Phase 1 - Types And Frame Plumbing

Goal: introduce the runtime vocabulary without changing behavior.

Scope:

- Add `UiInputSnapshot`, `UiFocusManager`, `UiInteractableRegistry`, and
  `UiActivationQueue` types.
- Add `UiElementKey` for persistent focus/capture identity; keep full
  `Clay_ElementId` values frame-local.
- Convert the existing platform input provider to produce a UI snapshot in
  addition to any demo command booleans still needed. The snapshot includes
  layout dimensions, pointer/touch state, scroll delta, delta time, text input,
  and keyboard/gamepad focus commands.
- Implement `ui_begin_frame` / `ui_end_frame` around the Clay lifecycle described
  above.
- Move or split the current `react_end_frame()` boundary as a React host phase
  change so effect cleanup and unmount sweeping run after render-command
  consumption and before queued UI activation handlers drain.
- Keep Clay layout, pointer, and scroll state fed every frame from the same
  normalized UI coordinates.
- Add tests for empty registry, disabled element filtering, and focus clearing
  when the focused key is absent.

Acceptance:

- Existing demo controls still work.
- No component needs direct SDL event access.
- Scroll remains inside the same UI input boundary as pointer and focus input.
- Focus state can survive from one frame to the next without referencing dead
  registered elements.
- No persistent focus or capture field stores a full `Clay_ElementId`.
- Two keyed instances of the same screen type do not share focus or capture
  state.
- Closing a modal restores the covered scope from that scope's stored
  `restore_target`, not from a single global `focused` slot.
- Unmount cleanup cannot destroy UI render resources before the Clay command
  array that references them has been rendered.

### Phase 2 - Focus-Aware Primitives

Goal: move interaction styling and activation into primitives.

Scope:

- Implement focus-aware button/list/toggle helpers.
- Derive `UiInteractionState`, `UiControlState`, and `UiVisualState`.
- Replace clicked-bool primitive APIs with declarative primitives that accept
  current-frame activation handlers.
- Render a visible focus indicator only when `focus_visible` is true.
- Skip disabled controls during focus traversal and activation.
- Keep selected/checked styling semantic and independent from focus.

Acceptance:

- Mouse hover and keyboard focus produce the same `targeted` visual state.
- Disabled controls can render unavailable but cannot be focused or activated.
- Selected list rows and checked toggles can be chosen without being focused.
- No primitive return value is used as an activation signal during declaration.
- No focus primitive introduces a domain `UiIntent` enum or generic command bus.

### Phase 3 - Keyboard And Gamepad Navigation

Goal: make non-pointer input activate the same controls.

Scope:

- Map Tab/Shift+Tab or equivalent keys to next/previous focus.
- Add `UiNavMetadata` for declaration order, navigation groups, optional
  previous-frame bounds, and optional explicit directional overrides.
- Map arrow keys and D-pad to directional focus commands using explicit
  overrides first, previous-frame bounds second, and linear order fallback last.
- Map Enter/Space and gamepad confirm to activation.
- Map Escape and gamepad cancel to the active scope's current-frame cancel
  handler, supplied by that screen/modal owner from its domain action hook. The
  focus runtime only dispatches the handler after validation; it does not own a
  cancel verb, mutate the screen stack, or define a scope-command vocabulary.
- Track last modality so keyboard/gamepad navigation enables focus-visible.

Acceptance:

- Every reachable button can be operated without a mouse.
- Gamepad and keyboard share the same focus and activation path.
- Directional navigation has a deterministic fallback when bounds or overrides
  are missing.
- Pressed visuals appear during confirm hold and clear on release.

### Phase 4 - Pointer And Touch Capture

Goal: normalize pointer and touch activation to the same button semantics.

Scope:

- Add pointer capture on press.
- Fire activation on release inside the captured element.
- Clear capture on release outside, scope change, unmount, or disabled state.
- Treat single-touch UI input as pointer input.

Acceptance:

- Press-drag-out-release does not activate.
- Press-drag-out-drag-in-release activates only if the captured element remains
  enabled and in the active scope.
- Touch and mouse produce the same hover/pressed/activation behavior where the
  platform can report it.

### Phase 5 - Focus Scopes

Goal: make stack and modal UI input ownership explicit without engine semantics.

Scope:

- Add a focus-scope provider around each screen/modal body.
- Derive each screen/modal scope id from the keyed screen entry instance, not
  just from `ScreenId`.
- Mark exactly one scope as input-active.
- Let covered or exiting scopes render but reject focus and activation.
- Support initial focus and restore-on-uncover.
- Add a small debug overlay or log mode that prints active scope and focused ID.

Acceptance:

- Opening a modal moves focus into the modal.
- Closing the modal restores focus to the previous screen if the element still
  exists.
- Two simultaneously mounted entries with the same `ScreenId` keep independent
  focus state.
- Covered screens cannot receive keyboard/gamepad activation.
- Exiting screens cannot receive fresh activation during their transition.

### Phase 6 - Text Entry

Goal: keep text editing inside UI focus without leaking into engine state.

Scope:

- Route text input only to focused text-entry controls.
- Keep text cursor, selection, composition, and placeholder state in the text
  entry component's React hook/ref state. The focus runtime routes text input; it
  does not store field-editing state.
- Use domain actions only when committing meaningful values, such as Apply or
  Submit.

Acceptance:

- Text input does not affect unfocused fields.
- Focus traversal can enter and leave fields deterministically.
- Confirm/cancel behavior is explicit per field or containing scope.

### Phase 7 - Verification

Goal: lock the focus model down with tests and runtime checks.

Scope:

- Unit-test focus traversal, disabled filtering, capture release, scope restore,
  and unmount clearing.
- Add a headless smoke path that exercises keyboard activation.
- Add a manual verification checklist for mouse, keyboard, gamepad, and touch.
- Add duplicate-ID diagnostics for focusable elements in the same scope.

Acceptance:

- All focus tests pass.
- Duplicate focus IDs are loud.
- Frame lifecycle tests prove traversal uses the previous completed registry and
  activation drains after render-command consumption.
- React cleanup/effect tests prove unmount cleanup runs after render-command
  consumption and before queued UI activation handlers mutate domain state.
- Regression tests prove activation dispatch uses the current-frame handler
  without introducing a typed focus-owned command bus.
- Regression tests prove queued activation payloads are copied into frame-owned
  storage and never point at render-stack locals.
- Input modality changes do not leave stale focus-visible or pressed state.
- No UI activation mutates state during layout declaration.

## Done

This plan is complete when:

- Keyboard, mouse, gamepad, and touch all target and activate the same primitive
  controls through one normalized path.
- Focus is scoped, restorable, and independent from engine state.
- Interaction, control, and visual state names match `docs/archive/ui.md`.
- The frame lifecycle preserves Clay's required ordering for layout dimensions,
  pointer state, scrolling, declaration, command production, and rendering.
- React cleanup and unmount sweeping cannot invalidate current-frame Clay render
  resources.
- Domain action hooks remain the only bridge from UI activation handlers to
  application mutation.
- Activation dispatch owns only per-frame copied handler payloads, not
  long-lived callbacks or domain command objects.
- Covered, disabled, unmounted, and exiting UI elements cannot accidentally
  receive activation.
