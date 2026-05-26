# Client UI focus and navigation architecture

The client UI architecture:

- `ui/` owns the generic focus/navigation engine.
- `client/ui/` marks interactive components as focusable and reads values /
  functions through hooks.
- Screen authors declare components, not directional edges between siblings.
- Directional navigation is derived from the focusable components' laid-out
  rectangles after Clay computes layout.
- Manual navigation exists only as a boundary or exception rule.

The example uses a buy menu because it has the shape that breaks flat-list
focus models: a category list, an item grid, a details panel, disabled rows,
and a confirm dialog.

The snippets are implementation-shaped C++. They define the ownership model and
the intended shape of client screen code.

`{Name}ScreenView` means the root component tree for a screen. It is not an MVC
view, and it does not get a companion screen-wide container.

---

## 1. Runtime contract

`ui/` exposes a focus frame object. Client UI registers focusable controls
during layout, and the focus runtime builds navigation from the harvested Clay
rectangles. Scopes are persistent managers keyed by the screen entry and
component identity.

```cpp
// ui/focus/UiFocus.h

#include <functional>

enum class UiNavDir {
    Up,
    Down,
    Left,
    Right,
};

enum class UiFocusSource {
    None,
    Keyboard,
    Gamepad,
    Mouse,
    Touch,
    Programmatic,
};

enum class UiNavRuleKind {
    Auto,       // default: spatial search inside the active scope
    Stop,       // stay on the current element at this boundary
    Wrap,       // spatial search from the opposite edge
    Explicit,   // jump to a specific element
    Custom,     // call a boundary resolver
};

struct UiNavRule {
    UiNavRuleKind kind = UiNavRuleKind::Auto;
    Clay_ElementId explicit_target = {};
};

struct UiNavRules {
    UiNavRule up;
    UiNavRule down;
    UiNavRule left;
    UiNavRule right;
};

struct UiFocusScopeDesc {
    Clay_ElementId id;
    bool modal = false;
    bool wrap = false;
};

struct UiFocusableDesc {
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

void ui_focus_begin_frame(const InputState *input);
void ui_focus_end_layout(void);

void ui_focus_push_scope(const UiFocusScopeDesc &desc);
void ui_focus_pop_scope(void);
void ui_focus_request_initial_focus(Clay_ElementId id);

UiFocusableState ui_focusable(const UiFocusableDesc &desc);
```

Runtime-owned UI lists use explicit bounded storage. The container policy is
part of the runtime contract: focus registration, harvested layout, UI intent
queues, and screen-stack storage do not grow through hidden allocator calls in
the middle of a UI frame.

```cpp
// ui/UiBuffer.h

template <typename T>
struct Span {
    T *items = nullptr;
    int count = 0;

    T *begin() const { return items; }
    T *end() const { return items + count; }
    T &operator[](int index) const { return items[index]; }
};

template <typename T>
struct UiBuffer {
    T *items = nullptr;
    int count = 0;
    int capacity = 0;

    void clear() { count = 0; }
    bool push(T value);

    Span<T> span() {
        return { items, count };
    }

    Span<const T> span() const {
        return { items, count };
    }
};

struct UiRuntimeLimits {
    int max_focus_scopes = 16;
    int max_focusables_per_scope = 256;
    int max_ui_intents = 128;
    int max_screens = 32;
};
```

The backing memory is allocated when `ClientUi` is initialized, from either a
UI arena or stable owned storage sized by `UiRuntimeLimits`. Overflow is a
runtime diagnostic and a dropped registration/intent, not a surprise
reallocation. This mirrors Clay's explicit `length`/`capacity` arrays and the
current hook runtime's fixed-capacity tables.

The important part is `ui_focus_end_layout()`: it runs after
`Clay_EndLayout()`, when Clay knows final positions. For every focusable
registered this frame, it calls `Clay_GetElementData(id)` and stores the
element's rectangle for next frame's navigation.

```cpp
// ui/focus/UiFocus.cpp

struct UiFocusableLayout {
    Clay_ElementId id;
    Clay_BoundingBox rect;
    bool disabled;
    uint32_t order;
    UiNavRules nav;
};

struct UiFocusableRegistration {
    Clay_ElementId id;
    bool disabled;
    UiNavRules nav;
};

struct UiFocusScope {
    Clay_ElementId id;
    bool modal;
    bool wrap;
    Clay_ElementId focused_id;
    UiFocusSource source;
    Clay_ElementId requested_initial_focus;

    // Built during this frame's render.
    UiBuffer<UiFocusableRegistration> pending;

    // Last completed layout. Directional navigation uses this.
    UiBuffer<UiFocusableLayout> layout;
};

struct UiFocusRuntime {
    UiBuffer<UiFocusScope> scopes;
};

static UiFocusScope *active_scope(void);
static Clay_ElementId resolve_spatial(UiFocusScope *scope,
                                      Clay_ElementId from,
                                      UiNavDir dir);

void ui_focus_begin_frame(const InputState *input) {
    UiFocusScope *scope = active_scope();
    if (!scope || !input) return;

    UiNavDir dir;
    bool has_nav = read_nav_dir(input, &dir);
    if (!has_nav) return;

    Clay_ElementId next = resolve_spatial(scope, scope->focused_id, dir);
    if (next.id != 0 && next.id != scope->focused_id.id) {
        scope->focused_id = next;
        scope->source = input->nav_source_gamepad
            ? UiFocusSource::Gamepad
            : UiFocusSource::Keyboard;
    }
}

void ui_focus_end_layout(void) {
    for (UiFocusScope &scope : g_focus.scopes) {
        scope.layout.clear();

        uint32_t order = 0;
        for (const UiFocusableRegistration &entry : scope.pending) {
            Clay_ElementData data = Clay_GetElementData(entry.id);
            if (!data.found) continue;

            bool stored = scope.layout.push({
                .id = entry.id,
                .rect = data.boundingBox,
                .disabled = entry.disabled,
                .order = order++,
                .nav = entry.nav,
            });
            if (!stored) {
                ui_focus_report_overflow(scope.id, "layout");
                break;
            }
        }

        scope.pending.clear();

        if (!contains_focusable(scope.layout.span(), scope.focused_id)) {
            Clay_ElementId next = first_enabled(scope.layout.span());
            if (contains_enabled_focusable(scope.layout.span(),
                                           scope.requested_initial_focus)) {
                next = scope.requested_initial_focus;
            }

            scope.focused_id = next;
            scope.source = UiFocusSource::Programmatic;
        }

        scope.requested_initial_focus = {};
    }
}
```

`ui_focus_request_initial_focus(id)` is scoped to the currently active focus
scope and applies only when that scope has no valid focused element in the
current harvested layout. The requested id is matched against the layout just
harvested by `ui_focus_end_layout()`. If the requested id is missing or
disabled, focus falls back to the first enabled focusable in declaration order.
Nested modal scopes keep their own request and do not overwrite the parent
scope's stored focus.

Component code can expose this as a hook-shaped helper:

```cpp
inline void use_initial_focus(Clay_ElementId id) {
    ui_focus_request_initial_focus(id);
}
```

The main loop gets one extra call after `Clay_EndLayout()`:

```cpp
ui_focus_begin_frame(&input);

react_begin_frame();
Clay_BeginLayout();
ClientUi_Build(&client_ui);
Clay_RenderCommandArray cmds = Clay_EndLayout();

ui_focus_end_layout(); // harvest Clay_GetElementData() for next frame nav

SDL_Clay_RenderClayCommands(&renderer, &cmds);
react_end_frame();
```

This keeps navigation deterministic. Frame N input navigates using frame N-1
layout data. Frame N layout is harvested for frame N+1. The focus live set is
one frame old, and it contains real geometry rather than a flat list.

---

## 2. Spatial resolver

The default resolver searches by geometry:

1. Ignore disabled controls.
2. Ignore controls outside the active focus scope.
3. Keep candidates in the requested half-plane.
4. Prefer candidates that overlap the current rect on the perpendicular axis.
5. Then prefer shortest primary-axis distance.
6. Then shortest center-to-center distance.
7. Then declaration order for a stable tie-break.

```cpp
static float center_x(Clay_BoundingBox r) { return r.x + r.width * 0.5f; }
static float center_y(Clay_BoundingBox r) { return r.y + r.height * 0.5f; }

static bool interval_overlaps(float a0, float a1, float b0, float b1) {
    return a0 < b1 && b0 < a1;
}

static bool is_candidate_in_direction(Clay_BoundingBox from,
                                      Clay_BoundingBox to,
                                      UiNavDir dir) {
    switch (dir) {
        case UiNavDir::Left:  return center_x(to) < center_x(from);
        case UiNavDir::Right: return center_x(to) > center_x(from);
        case UiNavDir::Up:    return center_y(to) < center_y(from);
        case UiNavDir::Down:  return center_y(to) > center_y(from);
    }
    return false;
}

static float primary_distance(Clay_BoundingBox from,
                              Clay_BoundingBox to,
                              UiNavDir dir) {
    switch (dir) {
        case UiNavDir::Left:  return from.x - (to.x + to.width);
        case UiNavDir::Right: return to.x - (from.x + from.width);
        case UiNavDir::Up:    return from.y - (to.y + to.height);
        case UiNavDir::Down:  return to.y - (from.y + from.height);
    }
    return 0.0f;
}

static float perpendicular_miss(Clay_BoundingBox from,
                                Clay_BoundingBox to,
                                UiNavDir dir) {
    bool horizontal = dir == UiNavDir::Left || dir == UiNavDir::Right;
    if (horizontal) {
        if (interval_overlaps(from.y, from.y + from.height,
                              to.y, to.y + to.height)) {
            return 0.0f;
        }
        return fabsf(center_y(to) - center_y(from));
    }

    if (interval_overlaps(from.x, from.x + from.width,
                          to.x, to.x + to.width)) {
        return 0.0f;
    }
    return fabsf(center_x(to) - center_x(from));
}

static Clay_ElementId resolve_spatial(UiFocusScope *scope,
                                      Clay_ElementId from_id,
                                      UiNavDir dir) {
    Span<const UiFocusableLayout> layout = scope->layout.span();
    const UiFocusableLayout *from = find_layout(layout, from_id);
    if (!from) return first_enabled(layout);

    const UiFocusableLayout *best = nullptr;
    float best_perp = 0.0f;
    float best_primary = 0.0f;
    float best_center = 0.0f;

    for (const UiFocusableLayout &candidate : layout) {
        if (candidate.id.id == from_id.id || candidate.disabled) continue;
        if (!is_candidate_in_direction(from->rect, candidate.rect, dir)) continue;

        float primary = primary_distance(from->rect, candidate.rect, dir);
        if (primary < 0.0f) primary = 0.0f;

        float perp = perpendicular_miss(from->rect, candidate.rect, dir);
        float dx = center_x(candidate.rect) - center_x(from->rect);
        float dy = center_y(candidate.rect) - center_y(from->rect);
        float center = dx * dx + dy * dy;

        bool better =
            !best ||
            perp < best_perp ||
            (perp == best_perp && primary < best_primary) ||
            (perp == best_perp && primary == best_primary && center < best_center) ||
            (perp == best_perp && primary == best_primary &&
             center == best_center && candidate.order < best->order);

        if (better) {
            best = &candidate;
            best_perp = perp;
            best_primary = primary;
            best_center = center;
        }
    }

    if (best) return best->id;
    return scope->wrap ? resolve_wrap(scope, from->rect, dir) : from_id;
}
```

This is not a screen-authored traversal policy. It works for:

- one column of categories,
- a 4-column item grid,
- tabs above a panel,
- a row of modal buttons,
- layouts that reflow at different window sizes.

If the layout changes, the nav graph changes because the rectangles changed.

---

## 3. Generic `Focusable`

The primitive registers itself with the focus engine and then renders a Clay
element using the same id. `render` is just the small block of UI that should
be drawn after focus state is known.

```cpp
// ui/primitives/Focusable.h

#include <functional>

using FocusableRender = std::function<void(const UiFocusableState &focus)>;

struct FocusableProps {
    Clay_ElementId id;
    bool disabled = false;
    UiNavRules nav = {};
    std::function<void()> on_confirm = {};
    std::function<void()> on_focus = {};
};

void Focusable(const FocusableProps &props,
               FocusableRender render);
```

```cpp
// ui/primitives/Focusable.cpp

void Focusable(const FocusableProps &props,
               FocusableRender render) {
    UiFocusableState state = ui_focusable({
        .id = props.id,
        .disabled = props.disabled,
        .nav = props.nav,
        .on_confirm = props.on_confirm,
        .on_focus = props.on_focus,
    });

    render(state);
}
```

This shape intentionally uses a capturing lambda-friendly API. If a later
profiling pass needs to remove `std::function`, the implementation can switch
to a template or `FunctionRef` without changing how `Button` is written.
The focus system should store only stable registration data between layout and
navigation frames; callbacks are for the current frame's input handling.

`Button`, `ListItem`, `Toggle`, and client-specific tiles compose this. They
do not own navigation.

---

## 4. First consumer: `Button`

`Button` is the first real consumer of `Focusable`. It is still generic UI,
so it lives under `ui/primitives/`, not `client/ui/`.

The button owns:

- label text,
- padding and visual styling,
- disabled styling,
- confirm callback forwarding.

The button does not own:

- how directional navigation finds the next control,
- which focus scope is active,
- what "Buy", "Cancel", or "Play" means to the game.

```cpp
// ui/primitives/Button.h

#include <functional>

struct ButtonProps {
    Clay_ElementId id;
    const char *label;
    bool disabled = false;

    // Optional escape hatch for boundary rules. Most buttons leave this Auto.
    UiNavRules nav = {};

    std::function<void()> on_confirm = {};
};

void Button(const ButtonProps &props);
```

`Button.cpp` consumes `Focusable`, derives visual state, and attaches the
focusable id to the actual Clay hit target.

```cpp
// ui/primitives/Button.cpp

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

The callback lambdas are captured during the current UI declaration pass and
invoked by current-frame input dispatch. A production write lane should enqueue
a typed UI intent from `on_confirm` and drain it after layout, rather than
mutating the screen stack or game state from inside `Button`.

### First place `Button` is consumed

Now step up one level into a generic dialog component. `ConfirmDialog` is
game-agnostic but composed: it uses two `Button`s. It still contains no
directional navigation graph.

```cpp
// ui/components/ConfirmDialog.h

#include <functional>

struct ConfirmDialogProps {
    const char *title;
    const char *body;
    const char *cancel_label;
    const char *confirm_label;

    std::function<void()> on_cancel = {};
    std::function<void()> on_confirm = {};
};

void ConfirmDialog(const ConfirmDialogProps &props);
```

```cpp
// ui/components/ConfirmDialog.cpp

void ConfirmDialog(const ConfirmDialogProps &props) {
    REACT_COMPONENT_BEGIN("ConfirmDialog") {
        ui_focus_push_scope({
            .id = CLAY_ID("ConfirmDialogFocusScope"),
            .modal = true,
            .wrap = true,
        });

        CLAY({
            .id = CLAY_ID_LOCAL("DialogPanel"),
            .layout = {
                .sizing = {
                    CLAY_SIZING_FIXED(360),
                    CLAY_SIZING_FIT(0),
                },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 12,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = panel_bg,
            .cornerRadius = CLAY_CORNER_RADIUS(6),
        }) {
            Text(props.title, text_style_title());
            Text(props.body, text_style_body());

            CLAY({
                .id = CLAY_ID_LOCAL("DialogButtons"),
                .layout = {
                    .childGap = 8,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .childAlignment = {
                        CLAY_ALIGN_X_RIGHT,
                        CLAY_ALIGN_Y_CENTER,
                    },
                },
            }) {
                Button({
                    .id = CLAY_ID("ConfirmDialogCancel"),
                    .label = props.cancel_label,
                    .on_confirm = props.on_cancel,
                });

                Button({
                    .id = CLAY_ID("ConfirmDialogConfirm"),
                    .label = props.confirm_label,
                    .on_confirm = props.on_confirm,
                });
            }
        }

        ui_focus_pop_scope();
    } REACT_COMPONENT_END();
}
```

`ConfirmDialog` composes the focusable buttons without owning the low-level
focus registration:

- The buttons are ordinary `Button` components.
- The buttons are siblings with laid-out Clay rectangles.
- Spatial navigation derives the left/right edge from those rectangles.
- The dialog owns focus trapping by opening a modal scope.

### Client code consuming the composed dialog

The climb from dialog to game loop is:

```text
OptionsDiscardDialog
  used by OptionsScreenView

OptionsScreenView
  owned by retained OptionsScreen

OptionsScreen
  stored by ScreenStack

ScreenStack
  owned by ClientUi

ClientUi
  invoked by GameUiPipeline

GameUiPipeline
  invoked by the main game tick
```

#### 1. Options discard dialog

The dialog receives only the callbacks it needs.

```cpp
// client/ui/screens/options/OptionsDiscardDialog.cpp

struct OptionsDiscardDialogProps {
    std::function<void()> on_keep_editing = {};
    std::function<void()> on_discard = {};
};

void OptionsDiscardDialog(const OptionsDiscardDialogProps &props) {
    ConfirmDialog({
        .title = "Discard changes?",
        .body = "Unsaved settings will be lost.",
        .cancel_label = "Keep editing",
        .confirm_label = "Discard",
        .on_cancel = props.on_keep_editing,
        .on_confirm = props.on_discard,
    });
}
```

#### 2. Options screen view

The screen view owns screen-local hook state and composes the dialog.

```cpp
// client/ui/screens/options/OptionsScreen.cpp

void OptionsScreenView(void) {
    REACT_COMPONENT_BEGIN("OptionsScreen") {
        ScreenNavigator nav = use_screen_navigator();
        int *discard_dialog_open = use_state_int(0);

        Button({
            .id = CLAY_ID("OptionsBack"),
            .label = "Back",
            .on_confirm = [discard_dialog_open] {
                *discard_dialog_open = 1;
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

Additional screen-local state is named directly:

```cpp
int *active_tab = use_state_int(0);
int *master_volume = use_state_int(70);
int *discard_dialog_open = use_state_int(0);
```

Screen-local state stays in the screen component unless a real shared owner
exists. The default shape is a small set of direct hook values.

#### 3. Retained screen object

The retained screen object is the stack entry. It owns top-level lifetime and
delegates visual composition to the component-style view function.

```cpp
// client/ui/navigation/UiScreen.h

class UiScreen {
public:
    virtual ~UiScreen() = default;

    virtual const char *debug_name() const = 0;
    virtual bool is_overlay() const { return false; }

    virtual void on_push() {}
    virtual void on_pop() {}
    virtual void tick() {}

    virtual void build_ui() = 0;
};
```

```cpp
// client/ui/screens/options/OptionsScreen.h

class OptionsScreen final : public UiScreen {
public:
    const char *debug_name() const override { return "Options"; }
    void build_ui() override;
};
```

```cpp
// client/ui/screens/options/OptionsScreen.cpp

void OptionsScreen::build_ui() {
    OptionsScreenView();
}
```

#### 4. Screen navigator hook

Screen navigation is a generic UI service exposed through context. `ClientUi`
installs the current screen context while it builds a screen, and screen code
reads that context with `use_screen_navigator()`.

```cpp
// client/ui/navigation/ScreenNavigator.h

class UiScreen;
class ScreenStack;

struct ScreenNavigator {
    ScreenStack *stack = nullptr;
    uint32_t entry_id = 0;

    void push(std::unique_ptr<UiScreen> screen) const;
    void pop_current() const;
};

struct UiScreenContext {
    ScreenNavigator navigator;
};

const UiScreenContext *ui_current_screen_context();
void ui_screen_context_push(const UiScreenContext &context);
void ui_screen_context_pop();

inline ScreenNavigator use_screen_navigator() {
    return ui_current_screen_context()->navigator;
}
```

Screen code opens another screen by constructing the next retained screen
object and pushing it through `ScreenNavigator`.

```cpp
// client/ui/screens/main_menu/MainMenuScreen.cpp

void MainMenuScreenView(void) {
    REACT_COMPONENT_BEGIN("MainMenuScreen") {
        ScreenNavigator nav = use_screen_navigator();

        Button({
            .id = CLAY_ID("MainMenuOptions"),
            .label = "Options",
            .on_confirm = [nav] {
                nav.push(std::make_unique<OptionsScreen>());
            },
        });
    } REACT_COMPONENT_END();
}
```

`Button::on_confirm` runs from input dispatch after layout. `nav.push()` and
`nav.pop_current()` mutate the stack after Clay has finished declaring the
current frame.

#### 5. Screen stack

`ScreenStack` owns screen lifetime and visible-screen ordering. It stores
retained `UiScreen` instances and exposes the visible entries for the current
frame.

```cpp
// client/ui/navigation/ScreenStack.h

struct ScreenStackEntry {
    uint32_t entry_id = 0;
    std::unique_ptr<UiScreen> screen;
};

class ScreenStack {
public:
    uint32_t push(std::unique_ptr<UiScreen> screen);
    void pop(uint32_t entry_id);
    void pop_top();

    UiScreen *top() const;
    Span<ScreenStackEntry *> visible_entries();

    void build_visible();

private:
    uint32_t next_entry_id = 1;
    UiBuffer<ScreenStackEntry> entries;
};

inline void ScreenNavigator::push(std::unique_ptr<UiScreen> screen) const {
    stack->push(std::move(screen));
}

inline void ScreenNavigator::pop_current() const {
    stack->pop(entry_id);
}
```

`ScreenStack::visible_entries()` returns the first opaque screen to draw plus
any overlays above it. A normal screen hides the entries below it. An overlay
draws on top of the screen below it.

`ScreenStack` can change screens only through `push`/`pop`, so it is allowed to
own retained storage internally. The important production rule is that this
storage stays behind the stack API and has explicit capacity diagnostics; the
component tree never receives or mutates the entries buffer.

`ScreenStack::build_visible()` installs the current screen context and then
calls each screen's `build_ui()`:

```cpp
void ScreenStack::build_visible(void) {
    for (ScreenStackEntry *entry : visible_entries()) {
        ui_screen_context_push({
            .navigator = {
                .stack = this,
                .entry_id = entry->entry_id,
            },
        });

        entry->screen->build_ui();

        ui_screen_context_pop();
    }
}
```

#### 6. Client UI owner

`ClientUi` owns the frame-level UI runtime: focus, input routing, intent
draining, and the screen stack.

```cpp
// client/ui/ClientUi.h

class ClientUi {
public:
    void begin_frame(const UiInputFrame &input);
    Clay_RenderCommandArray end_frame();
    void end_layout(const UiInputFrame &input);

    void push_screen(std::unique_ptr<UiScreen> screen) {
        screens.push(std::move(screen));
    }

    void build_visible_screens() {
        screens.build_visible();
    }

    Span<const UiIntent> drain_ui_intents();
    void clear_dispatched_intents();

private:
    ScreenStack screens;
    UiFocusRuntime focus;
    UiIntentQueue intents;
};
```

#### 7. Game UI pipeline

The UI pipeline adapts game/platform state into one client UI frame.

```cpp
// game/ui/GameUiPipeline.cpp

void GameUiPipeline::render_client_ui_frame(const GameUiFrame &frame) {
    UiInputFrame input = client_ui_input.build_frame({
        .surface = frame.surface,
        .raw_input = frame.raw_input,
    });

    client_ui.begin_frame(input);
    client_ui.build_visible_screens();

    Clay_RenderCommandArray commands = client_ui.end_frame();
    client_ui.end_layout(input);
    render_clay(frame.surface, commands);

    Span<const UiIntent> intents = client_ui.drain_ui_intents();
    dispatch_unhandled_game_ui_intents(intents);
    client_ui.clear_dispatched_intents();
}
```

#### 8. Main game tick

The main game tick invokes the UI frame through the game UI pipeline.

```cpp
// game/Game.cpp

void Game::tick_one_frame(float dt) {
    PlatformInput raw_input = platform.poll_input();

    world.tick(dt);
    renderer.draw_world(world);

    ui_pipeline.render_client_ui_frame({
        .surface = renderer.backbuffer(),
        .dt = dt,
        .raw_input = raw_input,
    });

    renderer.present();
}
```

The full call path is:

```text
Game::tick_one_frame
  polls input, ticks world, draws world, asks GameUiPipeline to render UI

GameUiPipeline::render_client_ui_frame
  builds UiInputFrame, runs ClientUi, renders Clay commands, drains UI intents

ClientUi
  owns focus runtime, UI intent queue, and ScreenStack

ScreenStack
  owns top-level screen lifetime and visible-screen ordering
  installs screen context
  asks each visible UiScreen to build itself

UiScreen object
  is the retained lifetime handle for one top-level surface

OptionsScreenView
  owns local hook state like discard_dialog_open
  composes OptionsDiscardDialog

OptionsDiscardDialog
  composes ConfirmDialog

ConfirmDialog
  composes Button

Button
  composes Focusable

Focusable
  registers with UiFocus
```

Lower components receive narrow props, and screen components read generic UI
services through hooks. Screen-wide bundles, the stack, and the game stay out of
the primitive/component layer.

---

## 5. Client UI: buy menu overlay

This is game-specific, so it lives under `client/ui/overlays/buy_menu/`.

```text
client/ui/overlays/buy_menu/
  BuyMenuOverlay.cpp
  buy_menu_hooks.h
  components/
    BuyCategoryList.cpp
    BuyItemGrid.cpp
    BuyItemTile.cpp
    BuyDetailsPanel.cpp
```

The overlay and its descendants read game data and write functions through
hooks. The focus runtime only sees ids, disabled flags, callbacks, and Clay
rectangles. There is no screen-wide bundle to pass through the component tree.

```cpp
// client/ui/overlays/buy_menu/buy_menu_hooks.h

#include <functional>

struct BuyCategoryOption {
    BuyCategoryId id;
    const char *label;
};

struct BuyItemOption {
    ItemId id;
    const char *name;
    int price;
    bool affordable;
    bool already_owned;
};

struct BuyMenuResult {
    Span<const BuyCategoryOption> categories;
    Span<const BuyItemOption> items;

    BuyCategoryId selected_category;
    ItemId preview_item;

    std::function<void(BuyCategoryId)> select_category;
    std::function<void(ItemId)> set_preview_item;
    std::function<void(ItemId)> request_buy;
};

BuyMenuResult use_buy_menu();
```

`BuyMenuResult` is the return value of a hook. Components call `use_buy_menu()`
where they need it; the result is not passed down as a prop.

### Overlay root

The overlay creates one modal focus scope. The modal flag means this scope is
the active navigation consumer while the buy menu is open. Parent HUD or game
screen focus remains stored but frozen.

```cpp
// client/ui/overlays/buy_menu/BuyMenuOverlay.cpp

void BuyMenuOverlay(void) {
    REACT_COMPONENT_BEGIN("BuyMenuOverlay") {
        BuyMenuResult buy_menu = use_buy_menu();
        int *confirm_open = use_state_int(0);

        auto open_confirmation = [confirm_open] {
            *confirm_open = 1;
        };

        auto close_confirmation = [confirm_open] {
            *confirm_open = 0;
        };

        ui_focus_push_scope({
            .id = CLAY_ID("BuyMenuFocusScope"),
            .modal = true,
            .wrap = false,
        });

        CLAY({
            .id = CLAY_ID_LOCAL("BuyMenuRoot"),
            .floating = {
                .attachTo = CLAY_ATTACH_TO_ROOT,
                .attachPoints = {
                    CLAY_ATTACH_POINT_CENTER_CENTER,
                    CLAY_ATTACH_POINT_CENTER_CENTER,
                },
            },
            .layout = {
                .sizing = {
                    CLAY_SIZING_FIXED(860),
                    CLAY_SIZING_FIXED(560),
                },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 16,
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
            .backgroundColor = panel_bg,
            .cornerRadius = CLAY_CORNER_RADIUS(6),
        }) {
            BuyCategoryList();
            BuyItemGrid(open_confirmation);
            BuyDetailsPanel();
        }

        if (*confirm_open) {
            BuyConfirmDialog(buy_menu.preview_item, close_confirmation);
        }

        ui_focus_pop_scope();
    } REACT_COMPONENT_END();
}
```

The overlay does not declare a separate traversal policy. It renders focusable
components with stable ids and lets the focus runtime derive neighbors from the
final Clay rectangles.

### Category list

The list is a vertical column of focusable rows. Spatial navigation will move
up/down through the row rectangles. Right naturally moves into the nearest item
tile because the grid sits to the right.

```cpp
// client/ui/overlays/buy_menu/components/BuyCategoryList.cpp

void BuyCategoryList(void) {
    REACT_COMPONENT_BEGIN("BuyCategoryList") {
        BuyMenuResult buy_menu = use_buy_menu();

        CLAY({
            .id = CLAY_ID_LOCAL("CategoryColumn"),
            .layout = {
                .sizing = { CLAY_SIZING_FIXED(180), CLAY_SIZING_GROW(0) },
                .childGap = 6,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        }) {
            for (int i = 0; i < buy_menu.categories.count; ++i) {
                const BuyCategoryOption &category = buy_menu.categories[i];
                BuyCategoryId category_id = category.id;

                ListItem({
                    .id = CLAY_IDI("BuyCategory", (uint32_t)category_id),
                    .label = category.label,
                    .selected = category_id == buy_menu.selected_category,
                    .on_confirm = [buy_menu, category_id] {
                        buy_menu.select_category(category_id);
                    },
                });
            }
        }
    } REACT_COMPONENT_END();
}
```

The only explicit id is a stable component identity. It is not a nav edge.

### Item grid

The item grid is laid out as rows for Clay. Navigation still comes from final
rectangles, not from `row` or `col` fields.

```cpp
// client/ui/overlays/buy_menu/components/BuyItemGrid.cpp

void BuyItemGrid(std::function<void()> open_confirmation) {
    REACT_COMPONENT_BEGIN("BuyItemGrid") {
        BuyMenuResult buy_menu = use_buy_menu();

        CLAY({
            .id = CLAY_ID_LOCAL("ItemGrid"),
            .layout = {
                .sizing = { CLAY_SIZING_FIXED(420), CLAY_SIZING_GROW(0) },
                .childGap = 8,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        }) {
            const int columns = 4; // layout detail, not navigation metadata

            for (int row = 0; row * columns < buy_menu.items.count; ++row) {
                CLAY({
                    .id = CLAY_IDI_LOCAL("ItemGridRow", row),
                    .layout = {
                        .childGap = 8,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    },
                }) {
                    for (int col = 0; col < columns; ++col) {
                        int index = row * columns + col;
                        if (index >= buy_menu.items.count) break;

                        BuyItemTile(buy_menu.items[index], open_confirmation);
                    }
                }
            }
        }
    } REACT_COMPONENT_END();
}
```

The value `columns = 4` is used only to make rows. If a responsive layout
changes this to 3 columns at a smaller width, navigation still works because it
will see different rectangles after layout.

### Item tile

The tile uses a generic `Focusable`, but the semantics are buy-menu specific:
focus previews the item, confirm requests a purchase, and unaffordable items
remain visible but disabled.

```cpp
// client/ui/overlays/buy_menu/components/BuyItemTile.cpp

void BuyItemTile(const BuyItemOption &item,
                 std::function<void()> open_confirmation) {
    REACT_COMPONENT_BEGIN_KEY("BuyItemTile", (uint32_t)item.id) {
        BuyMenuResult buy_menu = use_buy_menu();
        ItemId item_id = item.id;
        bool disabled = !item.affordable || item.already_owned;

        Focusable({
            .id = CLAY_IDI("BuyItem", (uint32_t)item_id),
            .disabled = disabled,
            .on_confirm = [buy_menu, open_confirmation, item_id] {
                buy_menu.set_preview_item(item_id);
                open_confirmation();
            },
            .on_focus = [buy_menu, item_id] {
                buy_menu.set_preview_item(item_id);
            },
        }, [&](const UiFocusableState &focus) {
            VisualState visual = derive_visual_state(focus, {
                .disabled = focus.disabled,
            });
            TileStyle style = buy_item_tile_style(visual);

            CLAY({
                .id = focus.id,
                .layout = {
                    .sizing = {
                        CLAY_SIZING_FIXED(96),
                        CLAY_SIZING_FIXED(88),
                    },
                    .padding = CLAY_PADDING_ALL(8),
                    .childGap = 4,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = style.bg,
                .border = {
                    .width = CLAY_BORDER_OUTSIDE(style.border_width),
                    .color = style.border,
                },
                .cornerRadius = CLAY_CORNER_RADIUS(4),
            }) {
                WeaponIcon(item.id, style.icon);
                Text(item.name, text_style_small(style.text));
                MoneyText(item.price, style.price);
            }
        });
    } REACT_COMPONENT_END();
}
```

Again, there is no navigation code. The tile only says:

- this Clay element is focusable,
- it has a stable id,
- it is disabled or enabled,
- this is what focus and confirm mean.

### Details panel

The details panel is not focusable. It reads the preview item through the
buy-menu hook. The focused tile updates that hook-owned value.

```cpp
void BuyDetailsPanel(void) {
    REACT_COMPONENT_BEGIN("BuyDetailsPanel") {
        BuyMenuResult buy_menu = use_buy_menu();

        CLAY({
            .id = CLAY_ID_LOCAL("DetailsPanel"),
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(12),
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childGap = 8,
            },
            .backgroundColor = details_bg,
            .cornerRadius = CLAY_CORNER_RADIUS(4),
        }) {
            const ItemDetails *details = lookup_item_details(buy_menu.preview_item);
            if (!details) {
                Text("Select an item", text_style_body());
                return;
            }

            Text(details->name, text_style_title());
            Text(details->description, text_style_body());
            StatRows(details->stats);
        }
    } REACT_COMPONENT_END();
}
```

This keeps preview state behind the buy-menu hooks, not in the generic focus
engine. The focus engine reports focus changes; the buy menu decides what those
changes mean.

---

## 6. Modal confirmation

The confirmation dialog pushes a nested modal scope. Because it is modal and
declared after the parent content, it becomes the active focus consumer. The
parent buy-menu scope keeps its focused item but does not move while the dialog
is open.

```cpp
void BuyConfirmDialog(ItemId item_to_buy,
                      std::function<void()> close_confirmation) {
    REACT_COMPONENT_BEGIN("BuyConfirmDialog") {
        BuyMenuResult buy_menu = use_buy_menu();

        ui_focus_push_scope({
            .id = CLAY_ID("BuyConfirmFocusScope"),
            .modal = true,
            .wrap = true,
        });

        CLAY({
            .id = CLAY_ID_LOCAL("Scrim"),
            .floating = { .attachTo = CLAY_ATTACH_TO_ROOT },
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .childAlignment = {
                    CLAY_ALIGN_X_CENTER,
                    CLAY_ALIGN_Y_CENTER,
                },
            },
            .backgroundColor = scrim_bg,
        }) {
            CLAY({
                .id = CLAY_ID_LOCAL("Dialog"),
                .layout = {
                    .sizing = {
                        CLAY_SIZING_FIXED(320),
                        CLAY_SIZING_FIT(0),
                    },
                    .padding = CLAY_PADDING_ALL(16),
                    .childGap = 12,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = panel_bg,
                .cornerRadius = CLAY_CORNER_RADIUS(6),
            }) {
                Text("Buy this item?", text_style_title());

                CLAY({
                    .id = CLAY_ID_LOCAL("DialogButtons"),
                    .layout = {
                        .childGap = 8,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    },
                }) {
                    Button({
                        .id = CLAY_ID("BuyConfirmNo"),
                        .label = "Cancel",
                        .on_confirm = [close_confirmation] {
                            close_confirmation();
                        },
                    });

                    Button({
                        .id = CLAY_ID("BuyConfirmYes"),
                        .label = "Buy",
                        .on_confirm = [buy_menu, close_confirmation, item_to_buy] {
                            close_confirmation();
                            buy_menu.request_buy(item_to_buy);
                        },
                    });
                }
            }
        }

        ui_focus_pop_scope();
    } REACT_COMPONENT_END();
}
```

The dialog does not need explicit left/right edges. Its two buttons are
siblings with adjacent rectangles, so left/right resolves naturally.

---

## 7. Where explicit navigation belongs

Manual navigation is still necessary, but it should be rare and local.

Example: pressing left from the category list should not move to a hidden
debug button behind the menu. The category list can stop at its left boundary.

```cpp
BuyCategoryId category_id = category.id;
BuyMenuResult buy_menu = use_buy_menu();

ListItem({
    .id = CLAY_IDI("BuyCategory", (uint32_t)category_id),
    .label = category.label,
    .selected = category_id == buy_menu.selected_category,
    .nav = {
        .left = { .kind = UiNavRuleKind::Stop },
    },
    .on_confirm = [buy_menu, category_id] {
        buy_menu.select_category(category_id);
    },
});
```

Example: pressing down on the last item in a wrapped dialog should wrap to the
first button.

```cpp
Button({
    .id = CLAY_ID("BuyConfirmYes"),
    .label = "Buy",
    .nav = {
        .down = { .kind = UiNavRuleKind::Wrap },
    },
    .on_confirm = [buy_menu, close_confirmation, item_to_buy] {
        close_confirmation();
        buy_menu.request_buy(item_to_buy);
    },
});
```

Example: a carousel might need an explicit target because the best visual
target is not the nearest rectangle.

```cpp
Button({
    .id = CLAY_ID("FeaturedWeapon"),
    .label = "Featured",
    .nav = {
        .right = {
            .kind = UiNavRuleKind::Explicit,
            .explicit_target = CLAY_ID("BuyCategoryRifles"),
        },
    },
    .on_confirm = [] {
        open_featured_weapon();
    },
});
```

These rules are exceptions. The ordinary buy-menu code above does not contain
manual jumps between siblings.

---

## 8. Focus contract summary

The focus manager keeps both the registrations from the in-progress frame and
the harvested rectangles from the completed layout:

```cpp
struct FocusManager {
    Clay_ElementId focused_id;
    UiFocusSource source;

    // Rebuilt during render.
    UiBuffer<UiFocusableRegistration> pending;

    // Harvested after Clay layout.
    UiBuffer<UiFocusableLayout> layout;
};
```

Screen code renders the component tree:

```cpp
BuyCategoryList();
BuyItemGrid(open_confirmation);
BuyDetailsPanel();
```

The runtime infers directional neighbors from the laid-out focusable rectangles.
The client UI controls semantics; the focus layer controls navigation mechanics.
Explicit rules remain local boundary exceptions such as `Stop`, `Wrap`, or a
single explicit target.
