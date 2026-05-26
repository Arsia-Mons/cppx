# Engine/UI boundary plan

This document defines the game-to-UI boundary around the client UI subsystem.
The intra-UI stack is defined by
`client-ui-focus-navigation-architecture.md`; this document only describes how
the game tick, platform input, world reads, UI intents, and rendering order meet
that subsystem.

The boundary exists to keep three ownership facts true:

- Game/engine code owns world simulation, game rules, raw platform polling, and
  world rendering.
- `ClientUi` owns focus runtime, input routing, retained `ScreenStack`, and UI
  intents.
- Screen components read values and functions through hooks.

---

## 1. Frame Owner

The game tick owns the outer frame:

```cpp
void Game::tick_one_frame(float dt) {
    PlatformInput raw_input = platform.poll_input();

    world.tick(dt, raw_input.gameplay);
    renderer.draw_world(world);

    ui_pipeline.render_client_ui_frame({
        .surface = renderer.backbuffer(),
        .raw_input = raw_input,
        .world = world,
    });

    renderer.present();
}
```

The UI pipeline adapts the frame for `ClientUi`:

```cpp
void GameUiPipeline::render_client_ui_frame(const GameUiFrame &frame) {
    UiInputFrame input = client_ui_input.build_frame(frame.raw_input);
    ClientPresentation presentation =
        client_presenters.build(frame.world);

    client_ui.begin_frame({
        .input = input,
        .presentation = presentation,
    });

    react_begin_frame();
    Clay_BeginLayout();
    client_ui.build_visible_screens();
    Clay_RenderCommandArray commands = Clay_EndLayout();

    client_ui.end_layout();
    render_clay(frame.surface, commands);
    react_end_frame();

    Span<const UiIntent> intents = client_ui.drain_ui_intents();
    apply_ui_intents(intents);
}
```

The order is binding:

1. Poll platform input.
2. Tick gameplay.
3. Draw the world scene.
4. Build the UI frame.
5. Render Clay commands.
6. Drain UI intents after declaration.
7. Present.

UI code never runs a gameplay tick. Gameplay code never calls Clay lifecycle
functions.

---

## 2. Read Flow

World data is adapted into presentation providers before screen components read
it. Components do not navigate the world object directly.

```cpp
struct HudSnapshot {
    int hp;
    int max_hp;
    int credits;
};

struct LoadoutSnapshot {
    Span<const LoadoutItem> items;
    Span<const LoadoutSlot> slots;
    ItemId preview_item;
};

struct ClientPresentation {
    HudSnapshot hud;
    LoadoutSnapshot loadout;
};
```

Providers publish narrow slices:

```cpp
HudSnapshot use_hud();
LoadoutResult use_loadout();
DisplaySettingsResult use_display_settings();
```

Hooks return values and functions. A component asks for the hook it needs at
the point it needs it:

```cpp
void HudView(void) {
    REACT_COMPONENT_BEGIN("Hud") {
        HudSnapshot hud = use_hud();
        HpBar({ .hp = hud.hp, .max_hp = hud.max_hp });
        CreditsText({ .credits = hud.credits });
    } REACT_COMPONENT_END();
}
```

There is no generic world-shaped context for ordinary screen code. If a screen
needs a value, name the value and expose it through a hook owned by the
relevant client UI/domain adapter.

---

## 3. Intent Flow

UI intents start as functions returned by hooks:

```cpp
struct PauseControls {
    std::function<void()> resume;
    std::function<void()> quit_to_title;
};

PauseControls use_pause_controls();
```

The hook implementation captures the `ClientUi` intent sink from context and
queues intent records owned by the client UI boundary:

```cpp
PauseControls use_pause_controls() {
    UiIntentSink sink = use_ui_intent_sink();
    ScreenNavigator nav = use_screen_navigator();

    return {
        .resume = [sink, nav] {
            sink.enqueue_resume();
            nav.pop_current();
        },
        .quit_to_title = [sink] {
            sink.enqueue_quit_to_title();
        },
    };
}
```

Screen code calls the returned functions:

```cpp
void PauseScreenView(void) {
    REACT_COMPONENT_BEGIN("PauseScreen") {
        PauseControls pause = use_pause_controls();

        Button({
            .id = CLAY_ID("PauseResume"),
            .label = "Resume",
            .on_confirm = pause.resume,
        });

        Button({
            .id = CLAY_ID("PauseQuit"),
            .label = "Quit to title",
            .on_confirm = pause.quit_to_title,
        });
    } REACT_COMPONENT_END();
}
```

`ClientUi` drains the intents after Clay declaration and dispatches them to the
owning game subsystem. This keeps screen code declarative while still allowing
buttons, toggles, and dialogs to request real changes.

---

## 4. Screen Ownership

`ClientUi` owns the retained screen stack:

```cpp
class ClientUi {
public:
    void push_screen(std::unique_ptr<UiScreen> screen);
    void build_visible_screens();
    Span<const UiIntent> drain_ui_intents();

private:
    ScreenStack screens;
    UiFocusRuntime focus;
    UiIntentQueue intents;
};
```

`ScreenStack` stores `UiScreen` objects, assigns entry ids, and decides which
screens are visible. A retained screen object delegates to its component tree:

```cpp
class PauseScreen final : public UiScreen {
public:
    const char *debug_name() const override { return "Pause"; }
    bool is_overlay() const override { return true; }

    void build_ui() override {
        PauseScreenView();
    }
};
```

Game code may request a top-level UI transition through the pipeline or
through a domain function. It does not own the stack data structure, render
screens, or pass screen state into component trees.

---

## 5. Input Boundary

The platform layer owns raw input collection. It produces two views:

- gameplay input, consumed by the game tick;
- UI input, consumed by `ClientUi`.

```cpp
struct PlatformInput {
    GameplayInput gameplay;
    UiInputFrame ui;
};
```

Pointer position is still delivered to Clay before layout:

```cpp
Clay_SetPointerState(input.ui.pointer_position,
                     input.ui.primary_pointer_down);
```

Keyboard/gamepad/touch confirm, cancel, and navigation edges are normalized
into `UiInputFrame`. Components do not inspect SDL events.

---

## 6. Rendering Boundary

The world renderer draws game-space content before the UI frame. React/Clay
draws client UI surfaces: HUD, menus, overlays, modals, and screen stack
content.

Screen/modal/HUD component code may declare Clay elements. It must not call:

- `Clay_BeginLayout`
- `Clay_EndLayout`
- `Clay_SetPointerState`
- the renderer's present function
- the game tick

Those calls belong to the frame owner and UI pipeline.

---

## 7. Local UI State

Local UI state belongs in hooks inside the component tree:

```cpp
void SettingsScreenView(void) {
    REACT_COMPONENT_BEGIN("SettingsScreen") {
        int *discard_dialog_open = use_state_int(0);
        DisplaySettingsResult settings = use_display_settings();

        Toggle({
            .id = CLAY_ID("Fullscreen"),
            .label = "Fullscreen",
            .checked = settings.fullscreen,
            .on_change = settings.set_fullscreen,
        });

        if (*discard_dialog_open) {
            SettingsDiscardDialog();
        }
    } REACT_COMPONENT_END();
}
```

Promote state out of the screen tree only when another real owner needs it.
Shared data goes behind a named hook; it is not passed as a screen-wide bundle.

---

## 8. Boundary Checklist

The boundary is correct when:

- `Game` or the platform layer owns raw input polling.
- `Game` ticks gameplay before the UI frame.
- `GameUiPipeline` adapts platform/game state into `UiInputFrame` and
  presentation providers.
- `ClientUi` owns `ScreenStack`, focus, and UI intent draining.
- `UiScreen` objects are retained stack entries.
- `{Name}ScreenView` functions are component roots with hook-local state.
- Components read values/functions through hooks.
- Components do not receive a broad screen data bundle.
- Stack and game writes are applied after Clay declaration.
- The engine-boundary doc does not duplicate focus/navigation component
  internals from the UI architecture docs.

---

## 9. Verification

Use these checks when implementation changes this boundary:

```sh
rg -n "Clay_BeginLayout|Clay_EndLayout|Clay_SetPointerState" src/ui
rg -n "use_context\\(&.*World|World[A-Za-z]*Context" src/ui src/client/ui
rg -n "ScreenStack" src/game src/engine
```

Expected result:

- Clay lifecycle calls are only in the UI pipeline/frame owner.
- Ordinary UI components do not read a raw world context.
- Game/engine code does not own the client screen stack.

Runtime scenarios:

- Pause opens over gameplay, freezes gameplay systems, and leaves parent focus
  intact.
- Settings opens from Pause and closes back to Pause without prop threading.
- An input-triggered local hook state change is visible on the next UI
  declaration pass.
- A UI intent requested by a button is dispatched after layout and never mutates
  game state during Clay declaration.
