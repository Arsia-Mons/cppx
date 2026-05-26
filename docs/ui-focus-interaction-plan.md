# UI focus and interaction-state plan

This document is the active UI-layer companion to
`client-ui-focus-navigation-architecture.md`. The canonical architecture doc
defines the stack from `Focusable` up through `ClientUi` and `GameUiPipeline`.
This plan pins down the focus, interaction, control, and visual-state contracts
used by those layers.

This is not an engine-boundary document. It stays inside the React-style
Clay UI runtime:

- `ui/` owns generic focus, input, and primitive component mechanics.
- `client/ui/` owns screen components, retained `UiScreen` entries,
  `ScreenStack`, and `ClientUi`.
- Screen components read values and functions through hooks.
- Child components receive narrow props only for their own behavior.
- Clay owns layout and render commands, not app state or navigation ownership.

---

## 1. Runtime Stack

The active UI stack is:

```text
Clay
  layout and render commands

ui/focus
  focus scopes, focusable registration, spatial navigation from Clay rectangles

ui/primitives
  Focusable, Button, Toggle, Selectable, TextInput

client/ui components
  dialogs, panels, rows, cards, menus

{Name}ScreenView
  root component tree for one screen

UiScreen
  retained stack entry for one top-level surface

ScreenStack
  retained screen lifetime and visible-screen ordering

ClientUi
  UI frame owner: focus runtime, input routing, UI intent queue, ScreenStack

GameUiPipeline
  adapts platform/game state into one UI frame and renders Clay commands

Game tick
  polls input, ticks world, draws world, invokes GameUiPipeline, presents
```

`{Name}ScreenView` is a component entry point. It is not a retained stack
object and it does not get a companion screen-wide data container. Local UI
state is ordinary hook state inside the component tree.

---

## 2. Input Frame

The platform layer normalizes device input into a `UiInputFrame` for the UI
runtime. It carries directional navigation, confirm/cancel edges, held state,
and release state.

```cpp
struct UiInputFrame {
    bool nav_up;
    bool nav_down;
    bool nav_left;
    bool nav_right;

    bool confirm_pressed;
    bool confirm_down;
    bool confirm_released;

    bool cancel_pressed;
    bool cancel_down;
    bool cancel_released;

    UiInputSource source;
};
```

Pointer position and button state still flow through Clay:

```cpp
Clay_SetPointerState(pointer_position, primary_pointer_down);
```

Components do not branch on keyboard, mouse, gamepad, or touch. They consume
focus and visual-state hooks; the runtime absorbs device differences.

---

## 3. Focus Scopes

A focus scope is the owner for focus inside one interactive region. Screens
usually open one scope. Dialogs and modal overlays open a nested modal scope.
The active modal scope consumes navigation while it is open; parent scopes keep
their stored focus and resume when the modal scope closes.

```cpp
struct UiFocusScopeDesc {
    Clay_ElementId id;
    bool modal = false;
    bool wrap = false;
};
```

The focus runtime keeps two bounded lists per scope:

```cpp
struct UiFocusScope {
    Clay_ElementId focused_id;
    UiFocusSource source;

    UiBuffer<UiFocusableRegistration> pending;
    UiBuffer<UiFocusableLayout> layout;
};
```

`pending` is rebuilt during declaration. `layout` is harvested after
`Clay_EndLayout()` by reading `Clay_GetElementData()` for each registered
focusable. Directional navigation in frame N uses frame N-1 layout. That makes
navigation depend on real rectangles instead of screen-authored sibling edges.

The default resolver:

1. Ignores disabled controls.
2. Ignores controls outside the active focus scope.
3. Keeps candidates in the requested half-plane.
4. Prefers perpendicular-axis overlap.
5. Prefers shortest primary-axis distance.
6. Prefers shortest center-to-center distance.
7. Uses declaration order as the stable tie-break.

Manual rules exist only at boundaries:

```cpp
Button({
    .id = CLAY_ID("Cancel"),
    .label = "Cancel",
    .nav = {
        .left = { .kind = UiNavRuleKind::Stop },
    },
});
```

The ordinary screen path is to render focusable components in the same tree
that Clay lays out.

---

## 4. Focusable

`Focusable` is the generic primitive that crosses from component code into the
focus runtime. It registers a stable Clay id, receives interaction state, and
renders the caller-provided body.

```cpp
struct FocusableProps {
    Clay_ElementId id;
    bool disabled = false;
    UiNavRules nav = {};
    std::function<void()> on_confirm = {};
    std::function<void()> on_focus = {};
};

struct UiFocusableState {
    Clay_ElementId id;
    bool focused = false;
    bool focus_visible = false;
    bool hovered = false;
    bool pressed = false;
    bool disabled = false;
};

void Focusable(const FocusableProps &props,
               std::function<void(const UiFocusableState &)> render);
```

`on_confirm` is a frame-local function. It may update local hook state or
request a write through the `ClientUi` UI intent queue. Stack and game
mutation is applied after the layout pass, not while Clay declarations are
still being built.

---

## 5. Control and Visual State

Interaction state comes from `Focusable`. Control state comes from the caller.
Visual state is the only thing component styling reads.

```cpp
struct ControlState {
    bool checked = false;
    bool selected = false;
    bool disabled = false;
};

struct VisualState {
    bool targeted = false;
    bool active = false;
    bool chosen = false;
    bool unavailable = false;
};

inline VisualState derive_visual_state(const UiFocusableState &focus,
                                       const ControlState &control) {
    return {
        .targeted = focus.hovered || (focus.focused && focus.focus_visible),
        .active = focus.pressed,
        .chosen = control.checked || control.selected,
        .unavailable = control.disabled,
    };
}
```

`Button`, `Toggle`, `Selectable`, and client-specific tiles all follow the
same pattern:

1. Call `Focusable`.
2. Build caller-owned `ControlState`.
3. Derive `VisualState`.
4. Select style from `VisualState`.
5. Declare Clay elements.

Raw input fields never appear in component styling.

---

## 6. Primitive Consumers

`Button` is the first concrete consumer:

```cpp
struct ButtonProps {
    Clay_ElementId id;
    const char *label;
    bool disabled = false;
    UiNavRules nav = {};
    std::function<void()> on_confirm = {};
};

void Button(const ButtonProps &props) {
    REACT_COMPONENT_BEGIN_KEY("Button", props.id.id) {
        Focusable({
            .id = props.id,
            .disabled = props.disabled,
            .nav = props.nav,
            .on_confirm = props.on_confirm,
        }, [&](const UiFocusableState &focus) {
            VisualState visual = derive_visual_state(focus, {
                .disabled = props.disabled,
            });
            ButtonStyle style = button_style(visual);

            CLAY({
                .id = focus.id,
                .layout = {
                    .sizing = { CLAY_SIZING_FIT(96), CLAY_SIZING_FIXED(38) },
                    .padding = { 14, 14, 8, 8 },
                    .childAlignment = {
                        CLAY_ALIGN_X_CENTER,
                        CLAY_ALIGN_Y_CENTER,
                    },
                },
                .backgroundColor = style.background,
                .border = {
                    .width = CLAY_BORDER_OUTSIDE(style.border_width),
                    .color = style.border,
                },
                .cornerRadius = CLAY_CORNER_RADIUS(4),
            }) {
                CLAY_TEXT(cs(props.label),
                    CLAY_TEXT_CONFIG({
                        .textColor = style.text,
                        .fontSize = 15,
                    }));
            }
        });
    } REACT_COMPONENT_END();
}
```

`Toggle` adds caller-owned `checked` state and an `on_change(bool)` function.
`Selectable` adds caller-owned `selected` state. Neither primitive owns group
selection or screen state.

---

## 7. Screen Components

Screen components use hooks for UI services and local state:

```cpp
void OptionsScreenView(void) {
    REACT_COMPONENT_BEGIN("OptionsScreen") {
        ScreenNavigator nav = use_screen_navigator();
        DisplaySettingsResult settings = use_display_settings();
        int *discard_dialog_open = use_state_int(0);
        int *master_volume = use_state_int(70);

        Button({
            .id = CLAY_ID("OptionsBack"),
            .label = "Back",
            .on_confirm = [discard_dialog_open] {
                *discard_dialog_open = 1;
            },
        });

        Toggle({
            .id = CLAY_ID("OptionsFullscreen"),
            .label = "Fullscreen",
            .checked = settings.fullscreen,
            .on_change = settings.set_fullscreen,
        });

        VolumeSlider({
            .id = CLAY_ID("OptionsVolume"),
            .value = *master_volume,
            .on_change = [master_volume](int value) {
                *master_volume = value;
            },
        });

        if (*discard_dialog_open) {
            OptionsDiscardDialog({
                .on_keep_editing = [discard_dialog_open] {
                    *discard_dialog_open = 0;
                },
                .on_discard = [discard_dialog_open, nav] {
                    *discard_dialog_open = 0;
                    nav.pop_current();
                },
            });
        }
    } REACT_COMPONENT_END();
}
```

Data hooks return values and functions. Components call the hook where they
need the value; parents do not pass a screen-shaped bundle through every child.

---

## 8. Frame Order

The UI frame order is:

```cpp
client_ui.begin_frame(input);
// Directional focus navigation uses the last harvested layout.

react_begin_frame();
Clay_BeginLayout();
client_ui.build_visible_screens();
Clay_RenderCommandArray commands = Clay_EndLayout();

client_ui.end_layout(input); // harvest rectangles and dispatch confirm/cancel
render_clay(commands);
react_end_frame();

client_ui.drain_ui_intents();
```

The exact function names can vary by integration. The ordering is binding:

- Resolve directional focus before declaration using the last harvested
  rectangles.
- Build Clay declarations before harvesting current rectangles.
- Harvest current rectangles for the next directional-navigation frame.
- Dispatch confirm/cancel against the focused id and the current frame's
  focusable registrations.
- Render commands before destroying resources referenced by those commands.
- Apply stack/game intents after declaration.

---

## 9. Verification

The implementation is correct when these checks hold:

- Three stacked buttons navigate with keyboard, gamepad, mouse, and touch
  without per-device branches in the button body.
- A row of modal buttons uses spatial left/right navigation from Clay
  rectangles without explicit sibling edges.
- A grid of item tiles reflows from four columns to three columns and
  directional navigation follows the new rectangles.
- A disabled focused control clears `targeted`, sets `unavailable`, and does
  not confirm.
- A modal scope traps navigation, then parent focus resumes when the modal
  closes.
- Screen-local state is hook state in the screen component tree.
- Shared reads and writes flow through hooks that return values and functions.
