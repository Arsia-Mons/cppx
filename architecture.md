# Architecture Notes

This project demonstrates sane multiplayer game UI architecture using a Counter-Strike 1.6-like scenario as a concrete reference point. The architecture should be specific enough to be useful, but not so specific that it only fits one shooter. The same boundaries should be reusable when pivoting to other multiplayer games.

The architecture should keep three ideas separate:

- Engine-ish infrastructure: how the program talks to the machine.
- Game/domain code: what the match, world, and rules are.
- Game client code: how the local player experiences and controls the game.

The goal is not to force everything into a generic `features` directory. Use direct game-client language: `client`, `game`, `server`, `net`, `ui`, `renderer`, `platform`, and `app`. The examples below use shooter concepts such as buy menus, teams, weapons, and HUD because they are familiar stress tests for UI architecture; they are examples, not a product constraint.

## Root Directories

```text
src/
  app/       # Process lifecycle, composition, and frame sequencing.
  platform/  # OS/library adapters such as SDL windowing, input, audio, and video.
  renderer/  # Drawing backends, render resources, world rendering, and Clay command rendering.
  audio/     # Generic audio playback, mixing, sound registries, and audio infrastructure.
  vfx/       # Generic visual effect systems such as particles, decals, and effect registries.
  ui/        # Generic Clay UI toolkit: runtime, layout, primitives, and design tokens.
  client/    # Local player-facing app: input, UI, presentation, prediction, and outbound intent.
  game/      # Shared game rules, state transitions, simulation, and domain types.
  server/    # Multiplayer authority: request validation, authoritative state, and replication.
  net/       # Protocol, messages, serialization, and transport abstractions.
```

## Mental Model

```text
app/platform/renderer/audio/vfx/ui
  reusable or engine-ish infrastructure

game/
  shared game rules, state transitions, simulation, and domain types

client/
  local player-facing application: UI, input, prediction, presentation

server/
  multiplayer authority: validates requests, owns authoritative match state

net/
  protocol, messages, serialization, transport abstractions
```

The client can request. The server authorizes. The game code defines the rules.

## `app/`

Owns process-level application lifecycle.

Typical responsibilities:

- Program startup and shutdown orchestration.
- Main loop ownership.
- Per-frame sequencing.
- Connecting platform, renderer, UI, client, game, server, and net systems.

Examples:

```text
app/App.cpp
app/GameLoop.cpp
app/AppComposition.cpp
app/FrameContext.h
```

`app/` should not become the home for menu behavior, match rules, UI widgets, SDL event details, or gameplay decisions. It wires systems together and drives the frame.

## `platform/`

Owns operating-system and library adapters.

Typical responsibilities:

- SDL window creation and lifetime.
- SDL event collection.
- Platform input normalization.
- Audio device setup.
- Fullscreen and vsync platform calls.

Examples:

```text
platform/sdl/SdlWindow.cpp
platform/sdl/SdlInput.cpp
platform/sdl/SdlAudio.cpp
platform/sdl/SdlVideo.cpp
```

Platform code can know about SDL. Game rules and UI screens should not.

## `renderer/`

Owns drawing backends and render resources.

Typical responsibilities:

- Rendering the game world.
- Rendering Clay command arrays.
- Font loading and text measurement.
- Texture/resource registries.
- Debug drawing.

Examples:

```text
renderer/Renderer.cpp
renderer/WorldRenderer.cpp
renderer/SdlClayRenderer.cpp
renderer/FontRegistry.cpp
renderer/TextureRegistry.cpp
```

The renderer can know about SDL and Clay render commands. Game simulation should not know about either.

## `audio/` And `vfx/`

Own reusable playback and effect systems, not game-specific behavior.

Examples:

```text
audio/AudioSystem.cpp
audio/SoundRegistry.cpp
audio/Mixer.cpp

vfx/ParticleSystem.cpp
vfx/DecalSystem.cpp
vfx/EffectRegistry.cpp
```

Actor-specific sound and visual effect mapping should live in client presentation code, not in `game/`.

Example:

```text
client/presentation/weapons/WeaponAudio.cpp
client/presentation/weapons/WeaponVfx.cpp
client/presentation/player/FootstepAudio.cpp
client/presentation/grenades/SmokeVfx.cpp
```

The game emits facts such as `WeaponFired` or `SmokeGrenadeDetonated`. Client presentation decides how those facts sound and look.

## `ui/`

Owns the generic Clay UI toolkit/runtime. This is not game-specific UI.

At 30,000 feet:

```text
ui/
  generic Clay-based mini framework

client/ui/
  actual game UI built with that framework
```

Typical `ui/` responsibilities:

- Clay lifecycle integration.
- Per-frame UI context.
- Input state in UI coordinates.
- Callback payload lifetime.
- Text storage for Clay strings.
- Generic layout primitives.
- Generic visual primitives.
- Design tokens.

Suggested shape:

```text
ui/
  runtime/
    ClayService.h
    ClayService.cpp
    UiFrameContext.h
    UiInputState.h
    CallbackStore.h
    TextStorage.h
    UiIds.h

  design/
    Theme.h
    Colors.h
    Typography.h
    Spacing.h

  layout/
    Box.h
    Box.cpp
    Row.h
    Column.h
    ScrollArea.h
    Spacer.h
    Divider.h

  primitives/
    Text.h
    Text.cpp
    Button.h
    Button.cpp
    Field.h
    Field.cpp
    Panel.h
    Panel.cpp
    Tabs.h
    Tabs.cpp
    ListItem.h
    ListItem.cpp
    Slider.h
    Slider.cpp
    ProgressBar.h
    ProgressBar.cpp
```

`ui/` should not know which game is being built. In the reference scenario, that means it should not know about weapons, money, teams, health, buy zones, servers, scoreboards, or multiplayer sessions.

## UI Building Blocks

Clay's raw `CLAY({ ... }) { ... }` block is the real node. The project should expose a small composition layer so screen code reads structurally instead of as raw Clay config.

Core layout building blocks:

```text
Box
  generic div-like container

Row
  horizontal flex composition

Column
  vertical flex composition

ScrollArea
  clipped container with scroll offset handling

Spacer
  flexible empty space

Divider
  simple separator
```

Core visual/control primitives:

```text
Text
Button
Field
Panel
Tabs
ListItem
Slider
ProgressBar
```

Split responsibilities:

```text
Box / Row / Column / ScrollArea
  layout only

Panel / Button / Field / Tabs / ListItem
  generic visual and interaction primitives

WeaponIcon / MoneyText / TeamBadge / BuyItemRow
  game-specific UI components under client/ui
```

## `client/`

Owns the local player-facing game client.

Typical responsibilities:

- Local client state.
- Mapping local input into client commands.
- Game-specific UI screens, overlays, HUD, and modals.
- Menu routing.
- Modal routing.
- Client-side presentation: SFX, VFX, feedback, transitions.
- Sending player intent to the server.
- Receiving replicated state/events from the server.

Examples:

```text
client/Client.cpp
client/ClientState.h
client/ClientInput.cpp

client/commands/ClientCommand.h
client/commands/ClientCommandQueue.h
client/commands/ClientCommandRouter.cpp
client/commands/handlers/BuyCommandHandler.cpp
client/commands/handlers/SettingsCommandHandler.cpp
client/commands/handlers/SessionCommandHandler.cpp

client/net/ClientConnection.cpp
client/replication/ClientReplicator.cpp
client/prediction/ClientPrediction.cpp

client/presentation/weapons/WeaponAudio.cpp
client/presentation/weapons/WeaponVfx.cpp
client/presentation/ui/UiAudio.cpp
```

`client/` is the player's application. It can know about game concepts. It should not pretend to be authoritative in multiplayer.

## `client/ui/`

Owns the game-specific interface. This is where the UI should have room to breathe.

Suggested shape:

```text
client/ui/
  ClientUi.h
  ClientUi.cpp
  ClientUiState.h
  ClientUiView.h

  navigation/
    ScreenId.h
    ScreenStack.h
    ScreenStack.cpp
    ModalStack.h
    ModalStack.cpp
    UiRoutes.h

  shell/
    MainMenuShell.h
    MainMenuShell.cpp
    InGameShell.h
    InGameShell.cpp
    MenuBackground.h
    MenuBackground.cpp

  common/
    MoneyText.h
    MoneyText.cpp
    WeaponIcon.h
    WeaponIcon.cpp
    TeamBadge.h
    TeamBadge.cpp
    PlayerName.h
    PlayerName.cpp
    ConfirmDialog.h
    ConfirmDialog.cpp
    ErrorBanner.h
    ErrorBanner.cpp

  screens/
    main_menu/
      MainMenuScreen.h
      MainMenuScreen.cpp
      components/
        MenuPanel.h
        MenuPanel.cpp

    options/
      OptionsScreen.h
      OptionsScreen.cpp
      OptionsState.h
      OptionsController.h
      OptionsController.cpp
      components/
        SettingsTabs.h
        SettingsTabs.cpp
        AudioPanel.h
        AudioPanel.cpp
        VideoPanel.h
        VideoPanel.cpp
        ControlsPanel.h
        ControlsPanel.cpp

    server_browser/
      ServerBrowserScreen.h
      ServerBrowserScreen.cpp
      components/
        ServerList.h
        ServerList.cpp
        ServerDetails.h
        ServerDetails.cpp

    loading/
      LoadingScreen.h
      LoadingScreen.cpp

  overlays/
    buy_menu/
      BuyMenu.h
      BuyMenu.cpp
      BuyMenuView.h
      BuyMenuState.h
      BuyMenuCommands.h
      BuyMenuController.h
      BuyMenuController.cpp
      components/
        BuyCategoryList.h
        BuyCategoryList.cpp
        BuyItemRow.h
        BuyItemRow.cpp
        BuyDetailsPanel.h
        BuyDetailsPanel.cpp
        BuyMoneyPanel.h
        BuyMoneyPanel.cpp

    team_select/
      TeamSelectMenu.h
      TeamSelectMenu.cpp
      TeamSelectState.h
      TeamSelectController.cpp
      components/
        TeamSelectGrid.h
        TeamSelectGrid.cpp

    pause_menu/
      PauseMenu.h
      PauseMenu.cpp

    scoreboard/
      ScoreboardOverlay.h
      ScoreboardOverlay.cpp

  hud/
    Hud.h
    Hud.cpp
    HudView.h
    Crosshair.h
    Crosshair.cpp
    HealthArmor.h
    HealthArmor.cpp
    AmmoCounter.h
    AmmoCounter.cpp
    RoundTimer.h
    RoundTimer.cpp
    KillFeed.h
    KillFeed.cpp
```

Use these meanings:

```text
screens/
  primary full-screen client routes: main menu, options, server browser, loading

overlays/
  UI layered over an active session or game context: buy menu, team select, pause menu, scoreboard in the reference scenario

hud/
  in-game display surfaces driven by game state

common/
  reusable game-specific UI components
```

## Co-Location Rule

Promote components only to the nearest useful level.

This rule is not optional.

```text
Only buy menu uses it:
  client/ui/overlays/buy_menu/components/BuyItemRow.cpp

Only options screen uses it:
  client/ui/screens/options/components/SettingsTabs.cpp

Used by multiple game UI surfaces:
  client/ui/common/WeaponIcon.cpp

Generic enough for any Clay app:
  ui/primitives/Button.cpp
```

Avoid catch-all component folders. A component should move upward only when real reuse proves it belongs there.

## UI State And View Models

Do not pass one mutable app state through every screen.

UI should read view models and emit intent.

UI-only state belongs near the surface that owns it:

```text
client/ui/overlays/buy_menu/BuyMenuState.h
  selected category
  selected item row
  local filter text
  pending confirmation

client/ui/screens/options/OptionsState.h
  active tab
  focused setting row

client/ui/navigation/ScreenStack.h
  active screen route

client/ui/navigation/ModalStack.h
  active modal route
```

Game truth belongs in `game/`:

```text
game/player/PlayerState.h
  health
  armor
  money
  inventory

game/weapons/WeaponState.h
  current weapon
  ammo
  reload state

game/match/RoundState.h
  round phase
  round timer
  team scores
```

Screens should consume read-only view data:

```cpp
struct BuyMenuView {
    Money money;
    bool isInBuyZone;
    BuyCategoryId selectedCategory;
    std::span<const BuyItemView> items;
    std::optional<BuyItemView> selectedItem;
};
```

## UI Controllers

UI-specific logic that interacts with the game should live next to the UI surface that needs it.

Example:

```text
client/ui/overlays/buy_menu/
  BuyMenu.cpp
  BuyMenuState.h
  BuyMenuView.h
  BuyMenuCommands.h
  BuyMenuController.cpp
  components/
```

Responsibilities:

```text
BuyMenu.cpp
  declares the Clay UI tree

BuyMenuState.h
  UI-only state: selected category, selected item, filter, confirmation state

BuyMenuView.h
  read-only data needed to draw: money, buy-zone status, item list, disabled reasons

BuyMenuController.cpp
  handles UI-local commands and forwards meaningful player intent

BuyMenuCommands.h
  typed UI-surface commands: SelectCategory, SelectItem, RequestBuyWeapon, CloseBuyMenu
```

The controller can know this is the buy menu. It can know selected rows, confirmation prompts, disabled messaging, previews, and which client command to emit. It should not own the economy, inventory, buy-zone, or weapon rules.

## Reference Example: Buy Menu

Rough structure:

```text
BuyMenuOverlay
  ModalPanel
    HeaderRow
      Title
      MoneyText
      CloseButton

    BodyRow
      CategorySidebar
        ScrollArea
          CategoryButton
          CategoryButton

      ItemContent
        ScrollArea
          BuyItemRow
          BuyItemRow

      DetailPanel
        WeaponIcon
        WeaponName
        Price
        StatBars
        BuyButton

    FooterRow
      HintText
      InventorySummary
```

Screen code should read structurally:

```cpp
void BuyMenuScreen(UiFrameContext& ui, const BuyMenuView& view, BuyMenuController& controller) {
    ui::Panel(ui, CLAY_ID("BuyMenu"), panel::Modal(), [&] {
        ui::Row(ui, CLAY_ID("Header"), layout::Header(), [&] {
            ui::Text(ui, "Buy Menu", text::Title());
            client_ui::MoneyText(ui, view.money);
        });

        ui::Row(ui, CLAY_ID("Body"), layout::Grow().gap(16), [&] {
            BuyCategoryList(ui, view, controller);
            BuyItemList(ui, view, controller);
            BuyDetailsPanel(ui, view, controller);
        });
    });
}
```

Button presses become typed UI-surface intent:

```cpp
if (ui::Button(ui, CLAY_IDI("BuyItem", item.id.raw), item.name)) {
    controller.handle(buy_menu::RequestBuyWeapon{.weaponId = item.weaponId});
}
```

## Commands And Authority

Commands are handled at the nearest owner that has the authority to handle them.

Use these layers:

```text
1. Surface controller
   Handles UI-local commands.

2. Client command router
   Handles local player/client intent and delegates to net, platform, session, audio, or prediction.

3. Server command handler
   Validates multiplayer requests and applies authoritative game operations.

4. Game/domain systems
   Define rules and state transitions.
```

Do not route every tiny UI interaction through the global client command layer.

Examples:

```text
Buy menu selected row changed:
  client/ui/overlays/buy_menu/BuyMenuController.cpp

Modal opened or closed:
  client/ui/navigation/ModalStack.cpp

Fullscreen toggled:
  client/commands/handlers/SettingsCommandHandler.cpp -> platform/sdl/SdlVideo.cpp

Buy weapon requested:
  buy menu controller -> client command -> net request -> server command -> game rules
```

The client command router should be thin. It drains and dispatches. It should not become a thousand-line handler.

Suggested shape:

```text
client/commands/
  ClientCommand.h
  ClientCommandQueue.h
  ClientCommandRouter.h
  ClientCommandRouter.cpp

  handlers/
    NavigationCommandHandler.cpp
    SettingsCommandHandler.cpp
    SessionCommandHandler.cpp
    BuyCommandHandler.cpp
    TeamCommandHandler.cpp
    PauseCommandHandler.cpp
```

## Multiplayer Command Flow

For multiplayer, the client is not authoritative.

Example buy flow:

```text
BuyItemRow button pressed
  -> buy_menu::RequestBuyWeapon
  -> BuyMenuController
  -> client::BuyWeaponRequested
  -> net::BuyWeaponRequest sent to server
  -> server receives request
  -> server validates player, round phase, buy zone, team, money, inventory
  -> game::tryBuyWeapon mutates authoritative GameState if legal
  -> server emits replicated state and/or game event
  -> client receives update
  -> UI/audio/vfx react next frame
```

The same player intent can have different representations at different trust boundaries:

```text
client UI command:
  "This UI surface wants to buy AK-47."

client command:
  "The local player requested a buy."

network request:
  "Client 12 requests BuyWeapon(AK47)."

server command:
  "Validate and process client 12's buy request."

game operation:
  "Apply valid purchase to authoritative game state."

replicated event:
  "Player 7 bought AK-47" or "Purchase denied."
```

These are not duplicates. They are different authority boundaries.

## `game/`

Owns game truth: match state, simulation, and rules.

Typical responsibilities:

- Round and match rules.
- Team state.
- Player state.
- Movement.
- Weapons, ammo, reload, recoil.
- Damage and armor.
- Economy rules.
- Buy zones.
- Map/world state.

Examples:

```text
game/GameState.h
game/GameSimulation.cpp

game/match/MatchState.h
game/match/RoundRules.cpp
game/match/Economy.cpp

game/player/PlayerState.h
game/player/PlayerMovement.cpp
game/player/PlayerInventory.cpp

game/weapons/WeaponDefs.h
game/weapons/WeaponSystem.cpp
game/weapons/Recoil.h

game/combat/Damage.cpp
game/combat/HitScan.cpp

game/world/MapState.h
game/world/CollisionWorld.cpp
game/world/BuyZones.cpp
```

`game/` should be able to exist without Clay, SDL, UI screens, or renderer code.

## `server/`

Owns multiplayer authority.

Typical responsibilities:

- Client connection ownership.
- Session ownership.
- Request validation.
- Authoritative game state.
- Server-side command handling.
- Replication output.

Examples:

```text
server/Server.cpp
server/ServerSession.cpp
server/ClientConnection.cpp

server/commands/ServerCommand.h
server/commands/ServerCommandRouter.cpp
server/commands/handlers/BuyWeaponHandler.cpp
server/commands/handlers/TeamSelectionHandler.cpp

server/replication/ReplicationSystem.cpp
```

The server can call `game/` operations. It should not know about Clay widgets or client UI layout.

## `net/`

Owns the protocol boundary.

Typical responsibilities:

- Message definitions.
- Serialization.
- Transport abstraction.
- Connection state shared between client and server.

Examples:

```text
net/Protocol.h
net/Messages.h
net/Serialization.cpp
net/Transport.h
```

`net/` should carry requests, responses, snapshots, and events. It should not contain UI layout or game rule implementation.

## Ownership Examples

### Buy Menu

The buy menu is client UI. The rules are game/server code.

```text
client/ui/overlays/buy_menu/BuyMenu.cpp
client/ui/overlays/buy_menu/BuyMenuController.cpp
client/ui/overlays/buy_menu/components/BuyCategoryList.cpp
client/ui/overlays/buy_menu/components/BuyItemRow.cpp
client/ui/overlays/buy_menu/components/BuyDetailsPanel.cpp
```

These files draw the menu, manage UI-local state, and emit typed requests.

```text
server/commands/handlers/BuyWeaponHandler.cpp
game/match/Economy.cpp
game/world/BuyZones.cpp
game/weapons/WeaponDefs.h
game/player/PlayerInventory.cpp
```

These files validate whether the player can buy the weapon and how authoritative state changes.

### HUD

The HUD is client UI over game-owned replicated state.

```text
client/ui/hud/Hud.cpp
client/ui/hud/HealthArmor.cpp
client/ui/hud/AmmoCounter.cpp
client/ui/hud/RoundTimer.cpp
```

These files display values.

```text
game/player/PlayerState.h
game/weapons/WeaponState.h
game/match/RoundState.h
server/replication/ReplicationSystem.cpp
client/replication/ClientReplicator.cpp
```

These files own, replicate, or receive the values.

### Settings

Settings are mixed.

```text
client/ui/screens/options/OptionsScreen.cpp
client/ui/screens/options/OptionsController.cpp
client/ui/screens/options/components/SettingsTabs.cpp
```

These files own the screen presentation, tab state, and interaction.

```text
client/commands/handlers/SettingsCommandHandler.cpp
platform/sdl/SdlVideo.cpp
platform/sdl/SdlAudio.cpp
```

These files apply platform-level changes such as fullscreen, vsync, and audio device settings.

## Hard Rules

- Promote components only to the nearest useful level.
- Keep `ui/` generic. Put game-specific UI in `client/ui/`.
- Keep UI-local state near the UI surface that owns it.
- Use view models for screen rendering.
- Use typed commands for meaningful intent.
- Do not send hover, selected row, or temporary widget details through global command routers.
- Do not mutate authoritative multiplayer game state from client UI.
- Do not put Clay, SDL, or renderer dependencies in `game/`.
- Do not put game rules in buttons, panels, menus, or HUD components.
- Let server/game code validate intent. Let UI redraw from resulting state.
