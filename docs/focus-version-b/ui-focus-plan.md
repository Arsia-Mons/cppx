# UI Focus Plan: How It Would Actually Work

*Falsifiable implementation explainer.*

This document explains the focus plan from first principles and ties each major
claim back to the repository as it exists now. It separates current compiled
behavior, archived reference code, and proposed implementation work.

In this document, **focus means UI element focus only**: which button, row,
field, toggle, tab, or list item receives keyboard/gamepad activation and draws
a focus indicator. It is not engine focus, gameplay focus, simulation focus,
screen lifecycle focus, pause semantics, or system tick gating.

## Top-Level Status

- **1** chosen source plan.
- **0** focus manager types in compiled `src/` today.
- **4** input families the plan must normalize (mouse, keyboard, gamepad, touch/text).
- **1** frame-order issue that must be proven fixed.

## Source Selection

**Chosen source:** `docs/ui-focus-plan.md`. It is the only top-level document
explicitly titled as a UI focus and input state plan.

| Tag | Document | Role |
| --- | --- | --- |
| chosen | `docs/ui-focus-plan.md` | Defines UI element focus, focus state, registry, queue, scopes, frame lifecycle, phases, and acceptance checks. |
| context | `docs/archive/ui.md` | Provides the archived vocabulary for interaction, control, and visual states. |
| context | `plan.md` | Describes the larger game UI direction: domain hooks, screen stack, and action boundaries. |
| not source | `README.md`, `architecture.md` | Broader architecture notes; do not define the focus implementation contract. |

---

## 1. What Problem Is Being Solved

Clay lays out and renders retained-looking UI from immediate declarations, but
it does not supply a browser-like focus layer. The plan adds the missing host
services: targetable element registration, keyboard/gamepad traversal, pointer
capture, focus-visible styling, and delayed activation dispatch.

> **Plan source:** `docs/ui-focus-plan.md:7-17` defines focus as UI element
> focus only, and says it owns targeting, traversal, capture, and focus visuals.

### Today: direct demo command booleans

1. **SDL key event** — UP, DOWN, M, T, I.
2. **`InputState`** — five booleans only.
3. **Component render** — Counter/theme/image mutate local state during
   declaration.
4. **Clay commands** — no registered focus targets.

### Proposed: focus host layer

1. **`UiInputSnapshot`** — pointer, scroll, keyboard, gamepad, text, modality.
2. **Focus manager** — persistent focus keys by scope.
3. **Interactable registry** — frame-local controls and handlers.
4. **Activation queue** — validated dispatch after render.

---

## 2. Current Architecture In Code Today

The compiled app is the `hello` target. It initializes SDL, SDL_ttf, Clay, and
a small React-style runtime, then re-declares the whole UI each frame. There is
no compiled focus manager, focus scope provider, interactable registry, or
activation queue yet.

> **Build source:** `CMakeLists.txt:46-57` compiles `src/main.cpp`,
> `src/react.cpp`, current components, providers, Clay, and stb only.

### Current Frame Architecture

1. **Platform loop** — `main()` polls SDL events and maps keys into demo
   booleans.
2. **React begin** — `react_begin_frame()` increments frame generation and
   resets render cursors.
3. **Clay declaration** — `App(&input)` calls providers, components, and raw
   `CLAY` blocks.
4. **React end (problematic)** — currently runs immediately after
   `Clay_EndLayout()`.
5. **Render** — SDL renderer consumes the command array after React cleanup
   has already run.

### Input Today

`src/input.h:10-16` contains only `increment_counter`, `decrement_counter`,
`toggle_counter`, `cycle_theme`, and `fetch_image`.

### Component State Today

Hook state is stored per fiber in `src/react.cpp:53-61`. `Counter()` uses
`use_state_int`, `use_ref`, and `use_effect`.

### Providers Today

`App()` wraps children with `INPUT_PROVIDER` and `THEME_PROVIDER`. Context is a
descent stack, not a subscription system.

### Current Code Evidence

- `src/main.cpp:136-159` polls SDL events and maps only demo key-down events.
- `src/main.cpp:161-164` feeds pointer state directly to Clay each frame.
- `src/main.cpp:166-177` runs React/Clay declaration, ends layout, runs React
  cleanup, then renders.
- `src/react.h:1-18` documents the React-style public API and current timing.
- `src/ui/components/app.cpp:17-63` is the current root UI surface.

### Archived Reference Evidence

- `src-archive/ui/runtime/ClayService.cpp:22-34` shows a former runtime wrapper
  for dimensions, pointer, scroll, and layout.
- `src-archive/ui/runtime/CallbackStore.cpp:7-25` stores callbacks and
  dispatches them immediately on Clay hover press.
- `src-archive/ui/primitives/Button.cpp:17-58` is a primitive API that
  currently returns no clicked boolean, but still dispatches through Clay hover
  callbacks.
- `src-archive/client/ui/navigation/ScreenStack.h:7-15` is a single-active-screen
  archive stack, not the keyed modal stack required by the focus plan.

---

## 3. Proposed Implementation, Step By Step

The implementation is a host runtime around the existing React-over-Clay frame.
It should add four reusable runtime pieces first, then move primitives and
screen bodies onto those pieces.

> **Plan source:** runtime pieces are named in `docs/ui-focus-plan.md:99-271`,
> and phases are listed in `docs/ui-focus-plan.md:391-567`.

### The Four Runtime Pieces

1. **Snapshot** — normalize raw SDL input once per frame before UI
   declaration. Fields: `layout`, `pointer`, `scroll`, `keys`, `gamepad`,
   `text`.
2. **Focus Manager** — persist one focused/restorable element per keyed scope,
   plus transient capture/confirm state. Fields: `active_scope`, `focused`,
   `restore_target`, `last_modality`.
3. **Registry** — each focus-aware primitive registers its current-frame role,
   stable key, state, navigation, and handler token. Fields: `UiElementKey`,
   `role`, `disabled`, `bounds`.
4. **Queue** — activation is validated and dispatched after Clay commands are
   rendered. Fields: `key`, `handler token`, `copied payload`.

### Code-Shaped Plan

```text
Platform SDL events
  -> UiInputSnapshot snapshot
  -> ui_begin_frame(snapshot, active_scope)
  -> react_begin_frame()
  -> Clay_BeginLayout()
  -> Screen/App declares focus-aware primitives
       ui_button(id, props, make_activation_handler(payload))
       registry.add({ key: {scope, id_hash}, role, disabled, nav, handler })
       visual = focus_manager.visual_state(key, control_state)
  -> Clay_EndLayout()
  -> ui_end_frame() reconciles registry, focus, capture, default focus, mounted scopes
  -> renderer consumes Clay_RenderCommandArray
  -> react host post-render cleanup/effects
  -> activation_queue.drain()
```

---

## 4. How Focus State Is Represented

The plan draws a hard line between persistent identity and frame-local Clay
data. Persistent focus stores app-owned keys. The registry can carry full
`Clay_ElementId` values only for the frame that declared them.

> **Plan source:** `docs/ui-focus-plan.md:122-180` defines `UiFocusManager` and
> the app-owned `UiElementKey`.

### Persistent Across Frames

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
```

This is the durable record. It must survive remounts only when the keyed scope
and element hash still refer to a mounted, enabled interactable.

### Frame-Local Only

```cpp
struct UiInteractable {
    UiElementKey key;
    Clay_ElementId clay_id;   // diagnostic/current frame only
    UiRole role;
    UiControlState control;
    UiNavMetadata nav;
    UiActivationHandler on_activate;
};
```

Full Clay IDs and handler tokens belong to the completed frame registry. They
must not be stored as persistent focus state.

### Focus State Transitions

- **Assigned** — initial focus chooses a deterministic enabled target when a
  scope first becomes active, or uses a screen-provided initial target if still
  valid.
- **Moved** — keyboard/gamepad navigation uses explicit directional overrides,
  then previous-frame bounds, then linear order fallback.
- **Cleared** — focused, captured, or confirm-pressed keys are cleared when
  missing, disabled, unmounted, hidden from the active scope, or released
  outside.
- **Restored** — a covered scope stores `restore_target`. When uncovered, the
  target is restored only if the same keyed scope instance still owns a
  matching enabled element.
- **Visible** — `focus_visible` is derived from modality. Keyboard, gamepad,
  and programmatic moves draw rings; pointer hover does not force a ring.
- **Pressed** — pointer capture and keyboard/gamepad confirm hold set pressed
  visuals. Release validates before queueing activation.

---

## 5. How An Individual Component Uses These States

A normal screen component should not manually juggle every focus flag. The
reusable primitive consumes the raw interaction state, combines it with
semantic control state, draws the right visuals, and registers the activation
handler. Screen-specific code mostly passes domain meaning: disabled, selected,
checked, value, and what action to run.

> **Plan source:** `docs/ui-focus-plan.md:329-358` says primitives register
> interactables, read derived interaction state, combine it with control state,
> draw visuals, and queue activation instead of returning clicked booleans.

### Three Layers of Responsibility

- **Screen Component** — knows domain state and user verbs. It decides whether
  a control is disabled or selected, and passes the current-frame action
  handler.
- **Primitive** — knows UI roles and visuals. It registers the element, asks
  the runtime for interaction state, derives visual flags, and emits Clay nodes.
- **Focus Runtime** — knows input targeting. It owns focused, focus-visible,
  hover, pressed, capture, modality, traversal, and queued dispatch validation.

### Button: Host Scope Plus Screen Body

```cpp
void RenderScreenEntry(const ScreenEntry &entry) {
    REACT_COMPONENT_BEGIN_KEY("ScreenEntry", entry.entry_id) {
        PROVIDE(&EntryContext, &entry) {
            UI_FOCUS_SCOPE(entry.entry_id, entry.input_active) {
                render_screen_body(entry);
            }
        }
    } REACT_COMPONENT_END();
}

void PauseMenu() {
    REACT_COMPONENT_BEGIN("PauseMenu") {
        PauseMenuStatus pause_status = use_pause_menu_status();
        auto pause = use_pause_actions();

        ui_button(CLAY_ID_LOCAL("Resume"), {
            .label = "Resume",
            .disabled = !pause_status.can_resume,
            .nav_order = 10,
        }, pause.resume_handler());

        ui_button(CLAY_ID_LOCAL("Settings"), {
            .label = "Settings",
            .disabled = false,
            .nav_order = 20,
        }, pause.open_settings_handler());
    } REACT_COMPONENT_END();
}
```

The screen does not receive a retained screen-state object, and it does not
ask whether Resume is hovered, pressed, or focused. It uses hooks: one hook
reads current game/engine status, another exposes current-frame actions, and
screen-local UI state would live in `use_state`, `use_ref`, or `use_effect`.
The screen stack or modal host, not the screen body, calls `UI_FOCUS_SCOPE`
around the active entry body.

### Button Primitive: No React Macro Because It Has No Hook State

```cpp
void ui_button(Clay_ElementId id,
               UiButtonProps props,
               UiActivationHandler on_activate) {
    UiElementKey key = ui_make_key(ui_current_scope(), id.id);

    UiControlState control = {
        .checked = false,
        .selected = false,
        .disabled = props.disabled,
    };

    ui_register_interactable({
        .key = key,
        .clay_id = id,
        .role = UiRole_Button,
        .control = control,
        .nav = {.order = props.nav_order},
        .accepts_focus = !props.disabled,
        .accepts_pointer = !props.disabled,
        .on_activate = on_activate,
    });

    UiInteractionState interaction = ui_interaction_state(key);
    UiVisualState visual = ui_visual_state(interaction, control);

    CLAY({
        .id = id,
        .backgroundColor = button_bg(visual),
        .border = button_border(visual),
    }) {
        if (visual.targeted) draw_target_treatment();
        if (interaction.focus_visible) draw_focus_ring();
        draw_label(props.label, visual);
    }
}
```

This primitive can be a plain function because it does not own hook state. It
receives a stable Clay ID from the caller, registers with the focus runtime,
and emits Clay nodes. If a primitive needs hooks, it must add its own React
component boundary.

### State Flow For One Button

1. **Caller** — `disabled=false`, label, handler.
2. **Register** — button enters current-frame registry with role and key.
3. **Runtime** — returns focused, focus_visible, hovered, pressed for that key.
4. **Primitive** — derives targeted, active, chosen, unavailable and draws
   Clay.
5. **Queue** — activation dispatches later if key is still valid.

### Toggle Or List Item Primitive: No Macro If It Has No Hook State

```cpp
void ui_toggle(Clay_ElementId id,
               UiToggleProps props,
               UiActivationHandler on_toggle) {
    UiControlState control = {
        .checked = props.checked,
        .selected = false,
        .disabled = props.disabled,
    };
    UiInteractionState interaction = ui_interaction_state_for_registered(id, control, on_toggle);
    UiVisualState visual = ui_visual_state(interaction, control);

    // checked is semantic state from the caller.
    // focused is only the current activation target.
    draw_toggle_box(visual.chosen, visual.targeted, visual.unavailable);
}
```

Checked or selected does not mean focused. A selected server row can remain
selected while keyboard focus is on the Play button.

### Text Field Primitive: React Macro Because It Owns Hook State

```cpp
void ui_text_field(Clay_ElementId id, UiTextFieldProps props) {
    REACT_COMPONENT_BEGIN_KEY("TextField", id.id) {
        TextEditState *edit = use_text_edit_state(props.value);
        UiElementKey key = ui_register_text_field(id, props.disabled);

        UiInteractionState interaction = ui_interaction_state(key);
        UiTextInputEvents text = ui_text_input_for_focused_key(key);

        if (interaction.focused) {
            text_edit_apply(edit, text);
        }

        draw_field({
            .text = edit->display_text,
            .cursor_visible = interaction.focused,
            .focus_ring = interaction.focus_visible,
            .hover = interaction.hovered,
            .disabled = props.disabled,
        });
    } REACT_COMPONENT_END();
}
```

The focus runtime routes text events. Cursor, selection, composition, and
draft text are component hook/ref state, not focus-manager state. Because this
primitive owns hook state, it must establish a React identity boundary instead
of consuming the caller's hook slots.

### Wrong Shape vs Intended Shape

Wrong shape:

```cpp
// Screen code manually interprets raw interaction.
if (ui_is_focused(resume_id) && input.confirm_pressed) {
    game.nav.pop_screen();       // mutates during declaration
}

if (Clay_Hovered() && mouse_down) {
    *selected_server = row;     // local state write in primitive path
}
```

Intended shape:

```cpp
// Host wraps the current screen entry once.
UI_FOCUS_SCOPE(entry.entry_id, entry.input_active) {
    PauseMenu();
}

// Screen describes controls. Runtime handles targeting.
REACT_COMPONENT_BEGIN("PauseMenu") {
    PauseMenuStatus pause_status = use_pause_menu_status();
    auto pause = use_pause_actions();

    ui_button(resume_id, {
        .label = "Resume",
        .disabled = !pause_status.can_resume,
    }, pause.resume_handler());
} REACT_COMPONENT_END();

// Later, after render:
activation_queue.drain(); // validates key, then calls handler
```

### Custom Interactive Box: Hover Styling Plus Click Handler

```cpp
void InventoryCard(const InventoryItemSummary &item) {
    REACT_COMPONENT_BEGIN_KEY("InventoryCard", item.item_id) {
        auto actions = use_inventory_actions();

        ui_interactive(CLAY_ID_LOCAL("InventoryCard"), {
            .role = UiRole_Button,
            .accepts_focus = true,
            .accepts_pointer = true,
            .nav_order = 40,
            .disabled = false,
            .on_activate = actions.open_item_handler(item.item_id),
        }, [&item](UiInteractionState interaction, UiVisualState visual) {
            ui_box({
                .padding = 16,
                .background = interaction.hovered ? color::SurfaceRaised : color::Surface,
                .border = interaction.focus_visible ? color::Accent : color::Border,
                .offset_y = interaction.pressed ? 1 : 0,
            }) {
                ui_text(item.name);
                ui_text(item.description);
            }
        });
    } REACT_COMPONENT_END();
}
```

This is the intended equivalent of an HTML `div` with `tabindex`, `role`,
hover styling, and an `onclick` handler. A parent would get
`InventoryItemSummary` values from a hook such as `use_inventory_items()`; the
card receives one item, reads computed interaction state for styling, and
provides an activation handler. The caller still does not set `hovered`,
`pressed`, or `focused`; the runtime computes those facts.

### Where React Macros Belong

| Function kind | Use React macro? | Reason |
| --- | --- | --- |
| Screen or composed widget using hooks | Yes: `REACT_COMPONENT_BEGIN[_KEY]` | It owns hook state, effects, refs, and keyed identity. |
| Provider or focus scope | Yes: provider-style enter/exit macro | It must create a scoped context value and often needs keyed identity. |
| Leaf primitive with no hooks | No | It can be a plain helper that registers an interactable and emits Clay nodes. |
| Leaf primitive with local state | Yes | Otherwise its hooks attach to the caller's fiber and can corrupt hook order. |

### Default Rule

`ui_box`, `ui_row`, `ui_column`, `ui_text`, and spacing primitives are not
focusable and do not register with the interaction runtime by default.

### Opt-In Rule

Any visible region can become interactive by using `ui_interactive` or a
convenience primitive built on it, such as `ui_clickable_box`. That is how
complex custom cards, rows, toolbar items, and canvas overlays participate in
hover, focus, pressed, and activation behavior.

---

## 6. Canonical Primitive Set

The UI should be built from a small number of reusable primitives, not from
screen code reimplementing input checks. The set below is enough to build dense
menus, settings panels, modal flows, inventory grids, chat surfaces, tabbed
screens, and custom interactive cards.

> **Repo context:** the archive already sketches layout and primitive names in
> `src-archive/ui/layout` and `src-archive/ui/primitives`. The current compiled
> app has not moved these into `src/` yet.

### Layout Primitives

- `ui_box` — generic Clay container; no interaction by default.
- `ui_row` and `ui_column` — directional layout wrappers.
- `ui_stack` — layered children for overlays or badge positioning.
- `ui_spacer` and `ui_divider` — rhythm and separation.
- `ui_scroll_area` — scroll clipping, wheel/drag-scroll integration, and
  optional focus auto-scroll.

### Content Primitives

- `ui_text` — immutable display text.
- `ui_icon` — theme-aware symbolic image or glyph.
- `ui_image` — texture/image region with explicit lifetime rules.
- `ui_badge` — compact status label, count, or severity mark.
- `ui_progress_bar` — read-only progress or meter visualization.

### Interaction Adapter

- `ui_interactive` — opt-in wrapper that registers any region as an
  interactable.
- `ui_clickable_box` — convenience wrapper for custom hover/click cards.
- `UiInteractionState` — runtime facts exposed to the render callback.
- `UiControlState` — semantic caller state such as disabled, selected, checked.
- `UiVisualState` — normalized styling flags derived from both.

### Action Primitives

- `ui_button` — primary action with keyboard/gamepad/pointer activation.
- `ui_icon_button` — compact toolbar action with tooltip/accessibility label.
- `ui_menu_item` — menu row with optional shortcut and selected/disabled state.
- `ui_list_item` — selectable row/card in a list or grid.

### Value Primitives

- `ui_toggle`, `ui_checkbox`, `ui_switch` — boolean controls.
- `ui_radio` and `ui_radio_group` — exclusive choice.
- `ui_slider` and `ui_stepper` — numeric adjustment.
- `ui_select` — compact option picker.
- `ui_text_field` and `ui_text_area` — editable text with local
  cursor/selection state.

### Composition Primitives

- `ui_tabs` — tab list plus active panel selection.
- `ui_toolbar` — grouped icon/action controls.
- `ui_panel` — visual grouping with title/action slots.
- `ui_dialog` and `ui_modal` — focus-scoped overlay surfaces.
- `ui_tooltip` and `ui_popover` — transient explanatory or option surfaces.

### How Complex UI Gets Built

```cpp
void SettingsDialog() {
    REACT_COMPONENT_BEGIN("SettingsDialog") {
        TabState tabs = use_tab_state(SettingsTab_Controls);
        auto settings = use_settings_actions();
        ControlsSettings controls = use_controls_settings();
        AudioSettings audio = use_audio_settings();
        VideoSettings video = use_video_settings();

        ui_dialog("Settings") {
            ui_tabs(CLAY_ID_LOCAL("SettingsTabs"), {
                .active = tabs.active,
                .items = {
                    tab("Controls", SettingsTab_Controls,
                        tabs.set_handler(SettingsTab_Controls)),
                    tab("Audio", SettingsTab_Audio,
                        tabs.set_handler(SettingsTab_Audio)),
                    tab("Video", SettingsTab_Video,
                        tabs.set_handler(SettingsTab_Video)),
                },
            });

            if (tabs.active == SettingsTab_Controls) {
                ui_panel("Controls") {
                    ui_select(CLAY_ID_LOCAL("AimMode"), {
                        .label = "Aim mode",
                        .value = controls.aim_mode,
                        .options = controls.aim_mode_options,
                    }, settings.set_aim_mode_handler());

                    ui_toggle(CLAY_ID_LOCAL("InvertLook"), {
                        .label = "Invert look",
                        .checked = controls.invert_look,
                    }, settings.toggle_invert_look_handler());
                }
            }

            if (tabs.active == SettingsTab_Audio) {
                ui_panel("Audio") {
                    ui_slider(CLAY_ID_LOCAL("MasterVolume"), {
                        .label = "Master",
                        .value = audio.master_volume,
                        .disabled = false,
                    }, settings.set_master_volume_handler());

                    ui_toggle(CLAY_ID_LOCAL("MuteWhenUnfocused"), {
                        .label = "Mute when unfocused",
                        .checked = audio.mute_when_unfocused,
                    }, settings.toggle_mute_unfocused_handler());

                    ui_clickable_box(CLAY_ID_LOCAL("ResetAudioCard"), {
                        .role = UiRole_Button,
                        .on_activate = settings.reset_audio_handler(),
                    }, [](UiInteractionState s, UiVisualState v) {
                        ui_box(card_style(s, v)) {
                            ui_text("Reset audio defaults");
                            ui_text("Restores all volume and device settings");
                        }
                    });
                }
            }

            if (tabs.active == SettingsTab_Video) {
                ui_panel("Video") {
                    ui_select(CLAY_ID_LOCAL("Resolution"), {
                        .label = "Resolution",
                        .value = video.resolution,
                        .options = video.resolution_options,
                    }, settings.set_resolution_handler());

                    ui_toggle(CLAY_ID_LOCAL("Vsync"), {
                        .label = "Vsync",
                        .checked = video.vsync,
                    }, settings.toggle_vsync_handler());
                }
            }
        }
    } REACT_COMPONENT_END();
}
```

The complex screen is still composed from primitives. It does not poll mouse
state, check Enter/Space, mutate domain state during layout, or manually
maintain hover/focus booleans. The active tab is local UI state returned by
`use_tab_state`, not a field on a retained screen-state object. Settings
values come from domain read hooks because they live outside UI, and settings
mutations go through domain action hooks.

---

## 7. Mouse, Keyboard, Gamepad, And Text Input Flow

The same primitive should be targetable and activatable from every supported
input family. Text input is routed by focus but editing state stays in the text
component's hook/ref state.

> **Plan source:** snapshot fields are listed in
> `docs/ui-focus-plan.md:101-120`. Text entry is scoped in
> `docs/ui-focus-plan.md:525-539`.

| Input family | Snapshot | Focus manager | Primitive state | Dispatch |
| --- | --- | --- | --- | --- |
| **Mouse** | Position, down, pressed, released, scroll. | Hit test/capture key. Last modality becomes pointer. | Hover and pressed. Focus ring only if policy assigns visible focus. | Release inside captured enabled element queues activation. |
| **Keyboard** | Next, previous, arrows, confirm, cancel. | Moves active-scope focus against previous registry. | Focused, focus visible, pressed during confirm hold. | Confirm release queues the focused element handler after current registry validation. |
| **Gamepad** | D-pad/stick mapped to same commands as keyboard. | Uses same traversal and repeat logic as keyboard. | Same focus ring and pressed semantics. | Same activation queue. No separate gamepad-only path. |
| **Text** | Text input events stay separate from movement commands. | Routes text only to focused text-entry control. | Cursor, selection, placeholder, composition live in component state. | Domain action only on commit, such as Apply or Submit. |

---

## 8. Clay Frame Lifecycle Fit

The plan is mostly a frame-order contract. The focus layer must not traverse
against a half-built registry, and UI activation must not mutate state while
Clay commands still reference current-frame resources.

> **Falsifiable issue:** current code calls `react_end_frame()` at
> `src/main.cpp:170-171`, before `SDL_Clay_RenderClayCommands` at
> `src/main.cpp:173-177`.

### Current `src` Frame Order (Broken)

1. Poll SDL into `InputState`.
2. Set Clay pointer.
3. `react_begin_frame`.
4. `Clay_BeginLayout` and declare UI.
5. `Clay_EndLayout`.
6. **`react_end_frame` cleanup/effects** *(too early)*.
7. Render commands.

### Required Plan Frame Order

1. Build `UiInputSnapshot`.
2. `ui_begin_frame`.
3. Apply Clay dimensions, pointer, scroll.
4. Declare UI and register interactables.
5. `ui_end_frame` validates current registry.
6. **Render commands while resources are alive**.
7. **React post-render cleanup, then activation drain**.

### Why This Matters

`Image()` can hold an `SDL_Texture*` in hook/ref state and cleanup destroys it
in `cancel_fetch()`. If an activation closes a screen or unmounts an image
before the current frame's command array is rendered, the renderer could hold
a command referencing a destroyed resource. The plan blocks that by rendering
before cleanup and draining activation last.

Source callouts: `src/ui/components/image.cpp:117-129` destroys textures
during cleanup; `docs/ui-focus-plan.md:260-327` requires queue drain after
render-command consumption and prohibits cleanup from invalidating
current-frame render resources.

---

## 9. React-Style Runtime Fit

The focus layer should use the existing runtime's identity and lifecycle rules.
It should not introduce a second reconciliation model or own component-local
state.

> **Runtime source:** `src/react.h:1-18` describes identity, hooks, effects,
> providers, and context. The focus doc delegates identity and component state
> to this runtime at `docs/ui-focus-plan.md:42-55`.

### Existing Runtime Mechanics

- Fiber identity is a Clay hash derived from parent ID and sibling index or
  explicit key.
- `use_state_int`, `use_ref`, and `use_effect` use ordered hook slots.
- Providers enter transparent fibers and push context values onto a scoped
  stack.
- Unmount detection is generation-based: missing fibers are destroyed in
  `react_end_frame()`.

### Focus Runtime Fit

- A focus-scope provider can mirror existing provider macros and should be
  called by the screen stack or modal host around an entry body.
- Scope IDs must be keyed screen/modal entry instances, not screen type names.
- Focus state belongs to `UiFocusManager`, not to arbitrary screen components.
- Text cursor/selection state remains hook/ref state inside the text component.
- Screen-local state stays in hooks; game/engine reads and actions come from
  named domain hooks.
- Domain actions are current-frame handlers produced by domain hooks, copied
  into frame-owned storage.

### Identity Check

```cpp
REACT_COMPONENT_BEGIN_KEY("ScreenEntry", entry.entry_id) {
    PROVIDE(&EntryContext, &entry) {
        UI_FOCUS_SCOPE(entry.entry_id, entry.input_active) {
            render_screen_body(entry);
        }
    }
} REACT_COMPONENT_END();
```

### Why Keyed Scope Matters

If scope identity is just `ScreenId::Pause`, two simultaneously mounted
pause-like entries share focus and capture state. The plan requires
`entry_id`-style keyed scope identity so local UI state behaves like keyed
React siblings.

Current tests already verify keyed sibling state across reorder in
`tests/react_runtime_tests.cpp:104-125`.

---

## 10. Reads, State Ownership, Commands, And Reuse Boundaries

The plan is falsifiable by ownership. If focus code starts mutating domain
state, defining user verbs, or owning text editing state, it has crossed its
boundary.

> **Boundary source:** `docs/ui-focus-plan.md:19-55` lists what the focus
> runtime may read, may write, and must not write.

| Area | Reads | Owns State | Sends Commands Or Actions |
| --- | --- | --- | --- |
| **Platform** | SDL events, window size, pointer, text, gamepad. | Pending platform input only. | Produces `UiInputSnapshot`; does not mutate UI components directly. |
| **Focus runtime** | Snapshot, active focus scope, previous/current registries, mounted scopes. | Interaction state only: focus keys, restore targets, pointer capture, confirm press, last modality, frame-local queue. | Dispatches copied current-frame activation handlers after validation. |
| **Primitives** | Props, semantic control state, focus visual state. | No domain state. Optional text editing state only in field component hooks. | Register interactables; provide handler tokens. They do not return clicked booleans. |
| **Screen-specific code** | Domain read/action hooks, current screen entry context, local hook state. | Screen-local UI state through hooks, such as tabs, filters, drafts, and initial focus request. | Supplies domain action handlers such as resume, apply, choose upgrade, or cancel. |
| **Domain subsystem** | Validated app/game state. | Game state, screen stack, settings, inventory, run state. | Executes named user verbs. Focus runtime must not define a generic domain command bus. |

### Reusable Primitives

`UiInputSnapshot`, `UiFocusManager`, registry, activation queue, focus-scope
provider, focus-aware Button/Field/ListItem, navigation helpers, duplicate-ID
diagnostics, debug overlay.

### Screen-Specific Code

Which element gets initial focus, which cancel handler is active, field
validation, domain action payloads, modal stacking policy, selected row
semantics, and domain state mutation.

---

## 11. Concrete Walkthroughs

These are step-by-step traces that should be testable. They do not assume
existing compiled focus support; they describe what the implementation must do.

> **Current gap:** compiled `src/` has no modal stack or focus scopes. The
> walkthrough uses the plan's target mechanics and labels archive screen code
> as reference only.

### Single Frame Walkthrough: Keyboard Confirm On Resume

Screen state: Pause scope `#42`, Resume focused, Settings unfocused, Quit
disabled. `focused={scope:42, hash:Resume}`.

1. SDL key-up for Enter becomes `snapshot.confirm_released=true`, last
   modality keyboard.
2. `ui_begin_frame` freezes previous registry. Focus already points at
   `{scope:42, Resume}`.
3. Clay dimensions, pointer, and scroll are set. `Clay_BeginLayout()` starts a
   new declaration.
4. `PauseMenu` declares `ui_button("Resume")`. The button registers an enabled
   interactable and supplies a copied handler token.
5. Primitive asks the focus manager for visual state. It draws `focused=true`,
   `focus_visible=true`, and `pressed=false` after release.
6. `Clay_EndLayout()` completes the current registry. `ui_end_frame`
   validates that the focused key is still enabled in scope 42.
7. Renderer consumes the command array while resources from the just-declared
   UI still exist.
8. React post-render cleanup/effects run. Then activation queue drains and
   invokes the current-frame resume handler.

### Screen Transition Walkthrough: Modal Opens

1. Main scope `Run#9` has focus on `UpgradeButton`.
2. Domain state pushes modal entry `Upgrade#23`. This is domain/nav
   ownership, not focus runtime ownership.
3. Renderer declares both scopes, but only `Upgrade#23` is input-active.
4. Focus manager records `Run#9.restore_target = UpgradeButton` and picks
   modal initial focus.
5. Covered run controls may remain visible but are not focusable or
   activatable.

### Screen Transition Walkthrough: Modal Closes

1. Activation in `Upgrade#23` dispatches a domain handler after render.
2. Domain/nav pops `Upgrade#23`. Next frame reports mounted scopes without
   that entry.
3. Focus manager prunes `Upgrade#23` focus/capture state.
4. `Run#9` becomes input-active again. Its restore target is validated
   against the current registry.
5. If `UpgradeButton` still exists and is enabled, focus returns. Otherwise a
   deterministic default is chosen or focus clears.

---

## 12. Invariants And Failure Modes

The plan is correct only if these invariants hold under normal rendering,
remounts, modal transitions, disabled controls, scroll, text input, and layout
changes.

> **Verification source:** focus acceptance criteria and risk coverage are
> called out across `docs/ui-focus-plan.md:417-567`.

### Must-Hold Invariants

- Only one focused element exists in the active scope.
- Persistent focus/capture stores `UiElementKey`, never full `Clay_ElementId`.
- Traversal uses the previous completed registry, not the half-built current
  frame.
- Activation dispatch happens after Clay commands are rendered.
- Disabled, hidden, unmounted, covered, or exiting elements cannot receive
  activation.
- Duplicate focusable IDs in the same scope are loud diagnostics.
- Text input is routed only to focused text-entry controls.
- Focus scope identity is keyed by screen/modal entry instance.

### Minimum Runtime Checks

- Assert one active focus scope per frame.
- Log duplicate `{scope, clay_id_hash}` registrations.
- Drop queued activation if current registry lacks the key or marks it
  disabled.
- Clear capture and confirm press on scope change.
- Track whether React cleanup ran before command rendering in tests.
- Expose debug overlay: active scope, focused key, capture key, modality.

### Risk Catalog

- **Stale IDs** — focus points at an element hash that no longer exists.
  Validation must clear or choose default every completed frame.
- **Remounts** — unkeyed reorder reuses state for the wrong control. Use keyed
  component/scope identity where order can change.
- **Duplicate Focusables** — two enabled controls in one scope share a key.
  Registration must fail loudly before traversal becomes nondeterministic.
- **Hidden Elements** — element remains registered while visually covered or
  clipped. Scope/visibility rules must mark it not focusable or not
  activatable.
- **Modal Transitions** — covered screens still receive confirm. Only the top
  input-accepting scope may traverse or activate.
- **Screen Changes** — a global focused slot restores focus to the wrong
  screen. Restore target must live per keyed scope.
- **Scrolling** — directional nav uses stale bounds after scroll.
  Previous-frame bounds are acceptable only if captured after scroll/layout
  for the completed frame.
- **Text Fields** — text composition leaks to gameplay or unfocused fields.
  Text events route by focused text-control role only.
- **Disabled Controls** — disabled controls render but still activate.
  Disabled must suppress focus traversal, capture, and queued dispatch.
- **Layout Changes** — bounds-aware movement points to a control that moved or
  disappeared. Current-frame validation and fallback order must cover it.

---

## 13. How To Verify It

Verification should prove behavior, ordering, and ownership boundaries. A
passing screenshot is not enough; tests need to cover stale state and frame
timing.

> **Existing test baseline:** `tests/react_runtime_tests.cpp:397-407` already
> checks runtime identity, providers, cleanup, and hook drift.

### Unit Tests

- Empty registry leaves focus empty.
- Initial focus chooses first enabled element.
- Disabled element is skipped in traversal and activation.
- Focused key missing in current registry clears focus.
- Duplicate focus IDs in one scope increment error count.
- Two keyed scopes preserve independent restore targets.

### Frame Timing Tests

- Traversal reads previous completed bounds.
- Pointer press registers capture but release validates current registry.
- Activation handler payload is copied into frame arena, not stack-referenced.
- React cleanup runs after render-command consumption and before activation
  dispatch.
- Activation cannot mutate state during layout declaration.

### Runtime Smoke Checks

- Keyboard-only path activates every reachable button.
- Gamepad confirm uses the same dispatch path as Enter/Space.
- Press-drag-out-release does not activate.
- Press-drag-out-drag-in-release activates only if capture remains valid.
- Text input changes only the focused field.

### Concrete Test Harness Shape

```cpp
// Suggested additions beside tests/react_runtime_tests.cpp
focus_empty_registry_clears()
focus_linear_next_previous_skips_disabled()
focus_directional_uses_previous_bounds_then_order()
focus_duplicate_key_reports_error()
focus_scope_restore_is_keyed_by_entry_id()
activation_dispatches_after_render_and_react_cleanup()
activation_payload_is_frame_owned_copy()
text_input_routes_only_to_focused_field()
covered_scope_rejects_activation()
scroll_updates_bounds_for_next_frame()
```

---

## 14. Open Questions And Assumptions To Validate

These are not hidden objections. They are the places where implementation
choices determine whether the plan stays technically correct.

> **Most important assumption:** the React host phase can be split or moved
> without breaking existing runtime tests and component cleanup expectations.

### How exactly should `react_end_frame()` be split?

The current API combines unmount sweep and effect flush. The plan needs
command rendering before cleanup, but may still need hook diagnostics and
generation bookkeeping after layout. Candidate split: `react_commit_layout()`
for hook count checks, then render commands, then
`react_flush_post_render()`.

### What is the exact payload storage model for activation handlers?

The plan says copied payloads in a frame arena. The implementation must define
max payload size, alignment, token invalidation, and how lambdas/domain hooks
are represented without storing long-lived callbacks.

### How will focus scopes report mounted keyed scopes?

The plan requires providers to report mounted scopes after the completed
frame. The implementation needs an explicit provider API and a pruning order
that does not delete state for scopes still transitioning visually but not
input-active.

### Are Clay ID hashes collision-safe enough for focus identity?

The plan stores `clay_id_hash`. The implementation should add duplicate
diagnostics and decide whether debug builds should also retain source labels
for collision reporting.

### What counts as hidden or clipped for focus traversal?

Clay can clip and scroll. The runtime must decide whether offscreen-but-scrollable
items can be focused, and whether focus movement should auto-scroll the
containing scroll area.

### What is the first real screen-stack owner?

Current compiled `src/` has no stack. Archive `ScreenStack` is
single-active-screen only. The focus-scope design needs the future keyed entry
model from `plan.md`, not the archive stack as-is.

---

## 15. Glossary

Local terms used by the plan and this explainer.

> **Vocabulary source:** interaction/control/visual state names are carried
> forward from `docs/archive/ui.md` and expanded below.

- **UI focus** — the UI element that receives keyboard/gamepad activation. Not
  engine focus or simulation pause state.
- **Focus scope** — a keyed screen/modal boundary where focus traversal and
  activation are contained.
- **UiElementKey** — persistent identity: focus scope plus Clay ID hash. This
  is what focus stores across frames.
- **Interactable registry** — frame-local list of enabled/disabled targetable
  primitives declared in the current UI tree.
- **Activation handler** — a frame-owned dispatch token/payload supplied by
  the current render. It is not a domain command enum.
- **Focus visible** — whether the focus ring should render. Keyboard/gamepad
  movement generally makes focus visible; pointer hover alone should not.
- **Pointer capture** — the element pressed by pointer/touch remains the
  candidate activation target until release or invalidation.
- **Restore target** — the last focused element for a scope, used when a
  covered scope becomes input-active again.
- **Domain action hook** — a screen/domain hook that exposes named verbs such
  as resume or choose upgrade. Focus dispatch invokes these handlers after
  validation.
- **Frame arena** — short-lived storage that keeps activation payloads alive
  until the activation queue drains.

---

## Appendix: Original Plan Boundary, Vocabulary, And Phases

This appendix preserves the source plan's own normative content: boundary
rules, vocabulary definitions, runtime piece specifications, frame lifecycle
steps, primitive contract, focus scopes, phased rollout, and acceptance
criteria. The numbered sections above are the explainer view of the same
material; this appendix is the canonical contract.

### Boundary

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

### Vocabulary

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

### Runtime Pieces

#### `UiInputSnapshot`

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

#### `UiFocusManager`

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

#### `UiInteractableRegistry`

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

#### `UiActivationQueue`

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

### Frame Lifecycle

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

### Primitive Contract

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

### Focus Scopes

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

### Phases

#### Phase 1 - Types And Frame Plumbing

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

#### Phase 2 - Focus-Aware Primitives

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

#### Phase 3 - Keyboard And Gamepad Navigation

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

#### Phase 4 - Pointer And Touch Capture

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

#### Phase 5 - Focus Scopes

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

#### Phase 6 - Text Entry

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

#### Phase 7 - Verification

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

### Done

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
