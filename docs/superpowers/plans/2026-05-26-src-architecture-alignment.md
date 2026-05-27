# src/ Architecture Alignment Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bring `src/` in line with `architecture.md`. Pull the SDL/Clay/renderer shell out of `main.cpp` into proper `app/`, `renderer/`, `platform/sdl/` modules; move the UI pipeline out of `game/`; decompose `shooter::ShooterGame` into HL1/CS1.6-style `game/{player_state, inventory, weapons, economy}`; move shooter UI screens into `client/ui/screens/`; tidy `client/ui/` into the spec layout.

> **Amendment 2026-05-26:** Drop the `/shooter/` subdirectory from `game/` and `client/ui/screens/`. This is a tightly-coupled engine+game project (shooter IS the game, no multi-game plans), so `game/X` and `client/ui/screens/Y` is enough — no need to nest further. Namespaces and class names (`namespace shooter`, `ShooterGame`) remain unchanged. Affects Tasks 5, 6, 8.

**Architecture:** Pure refactor — zero behavior change. Existing tests (`react_runtime_tests`, `ui_focus_tests`, `ui_primitives_tests`, `client_ui_tests`, `game_ui_pipeline_tests` → renamed, `shooter_ui_tests`, plus the three Python smoke tests) act as the safety net. Each task: move/extract → update CMake & includes → build → ctest → commit. Commit per task; keep diffs reviewable.

**Tech Stack:** C++20, CMake 3.28+, SDL3, SDL3_ttf, Clay, libcurl, Python 3 (smoke tests).

**Design decisions locked in (from prior conversation):**

- Game loop lives in `app/` (`architecture.md:50-68`). `game/` exposes step functions; `app/GameLoop` decides cadence.
- `game/ui/game_ui_pipeline.*` → `client/ui/ui_pipeline.*`. Rename namespace/types to `client::ui::UiPipeline`, `UiPipelineFrame`, `use_ui_pipeline_frame`. The pipeline is UI infrastructure, not game truth.
- `ShooterGame` decomposes into `game/{player_state, weapon_defs, weapon_state, inventory, economy, shooter_game}`. `shooter_game` becomes a thin façade so the UI API stays stable through task 6.
- `compare_enabled` is UI state, not game state. Extracted in task 6 when its only consumer (`LoadoutScreen`) moves.
- Shooter UI screens go to `client/ui/screens/screens.{h,cpp}` (one file for now). The `src/shooter/` directory disappears at the end of task 6.
- `client/ui/` empty placeholder folders the user created (`components/`, `hooks/`, `providers/`) are kept as scaffolding hints; `screens/screen-name/` template is deleted in favour of real screens at `screens/`. No `overlays/` folder until something actually overlays.
- No `client/commands/` for now; deferred writes remain the intent channel.

**Out of scope (deliberate):**

- Re-introducing a typed `client/commands/` bus.
- Adding `audio/`, `vfx/`, `server/`, `net/` directories.
- Adding new tests; only renaming/retargeting existing ones.

---

## File-touch map

**Created:**
```
src/renderer/font_registry.{h,cpp}
src/renderer/sdl_clay_renderer.{h,cpp}
src/platform/sdl/window.{h,cpp}
src/platform/sdl/input.{h,cpp}      (moved from src/platform/input_adapter.{h,cpp})
src/app/app.{h,cpp}
src/app/game_loop.{h,cpp}
src/client/ui/ui_pipeline.{h,cpp}   (moved from src/game/ui/game_ui_pipeline.{h,cpp})
src/client/ui/navigation/screen_stack.{h,cpp}   (moved from src/client/ui/)
src/client/ui/navigation/ui_screen.h            (moved from src/client/ui/)
src/client/ui/providers/shooter_provider.{h,cpp}            (extracted from shooter_ui.cpp)
src/client/ui/hooks/shooter_hud.{h,cpp}                     (extracted from shooter_ui.cpp)
src/client/ui/hooks/shooter_weapons.{h,cpp}                 (extracted from shooter_ui.cpp)
src/client/ui/components/hud_band.{h,cpp}                   (extracted from shooter_ui.cpp)
src/client/ui/screens/main_menu/main_menu_screen.{h,cpp}    (extracted from shooter_ui.cpp)
src/client/ui/screens/in_game/in_game_screen.{h,cpp}        (extracted from shooter_ui.cpp)
src/client/ui/screens/pause/pause_screen.{h,cpp}            (extracted from shooter_ui.cpp)
src/client/ui/screens/loadout/loadout_screen.{h,cpp}        (extracted from shooter_ui.cpp)
src/client/ui/screens/loadout/components/weapon_tile.{h,cpp}     (extracted from shooter_ui.cpp)
src/client/ui/screens/loadout/components/equipment_slot.{h,cpp}  (extracted from shooter_ui.cpp)
src/client/ui/screens/loadout/components/confirm_dialog.{h,cpp}  (extracted from shooter_ui.cpp)
src/client/ui/screens/options/options_screen.{h,cpp}        (extracted from shooter_ui.cpp)
src/game/player_state.h
src/game/weapon_defs.h
src/game/weapon_state.h
src/game/inventory.{h,cpp}
src/game/economy.{h,cpp}
src/game/shooter_game.{h,cpp}                               (moved from src/shooter/, slimmed)
```

**Modified:**
```
src/main.cpp                        (drops ~250 lines, becomes a tiny shim)
src/platform/control_mailbox.{h,cpp} (signatures: GameUiPipeline → ClientUi; include updates)
src/client/ui/client_ui.h           (include updates after navigation/ move)
CMakeLists.txt                      (every source list)
src/CLAUDE.md
src/client/ui/CLAUDE.md
CLAUDE.md                           (paths/layout references)
tests/game_ui_pipeline_tests.cpp → tests/ui_pipeline_tests.cpp (renamed + include/namespace updates)
tests/client_ui_tests.cpp           (include updates)
tests/shooter_ui_tests.cpp          (include updates)
```

**Deleted:**
```
src/platform/input_adapter.{h,cpp}                  (moved to platform/sdl/)
src/game/ui/                                         (whole dir)
src/shooter/                                         (whole dir, after task 7)
src/client/ui/screen_stack.{h,cpp}                  (moved to navigation/)
src/client/ui/ui_screen.h                           (moved to navigation/)
src/client/ui/screens/screen-name/                  (whole template dir)
```

---

## Proposed end-state `src/` hierarchy

Complete enumeration of every file `src/` should contain after Task 8 lands. **Legend:** `(unchanged)` = exists today and stays put; `(new)` = added by this refactor; `(moved)` = file relocated; `(was X)` = previous location. Headers/sources are listed as `.{h,cpp}` pairs unless only one exists.

```text
src/
  CLAUDE.md                              (unchanged — content updated in Task 8)
  AGENTS.md                              (unchanged — symlink to CLAUDE.md)
  main.cpp                               (unchanged path; gutted to ~15-line shim in Task 3)
  react.{h,cpp}                          (unchanged — React-style hook runtime)

  app/                                   (new directory)
    CLAUDE.md                            (new — lifecycle ownership, frame-loop conventions)
    AGENTS.md                            (new — symlink to CLAUDE.md)
    app.{h,cpp}                          (new — lifecycle: SDL/TTF/curl init, owns subsystems)
    game_loop.{h,cpp}                    (new — per-frame body: events → input → render → drain)

  renderer/                              (new directory)
    CLAUDE.md                            (new — what belongs here vs platform/ vs ui/)
    AGENTS.md                            (new — symlink to CLAUDE.md)
    font_registry.{h,cpp}                (new — SDL_ttf font discovery + Clay measure_text)
    sdl_clay_renderer.{h,cpp}            (new — Clay command array → SDL draw calls)

  platform/                              (existing directory)
    CLAUDE.md                            (new — platform/ is SDL-adapter + test IPC; no game/UI here)
    AGENTS.md                            (new — symlink to CLAUDE.md)
    control_mailbox.{h,cpp}              (unchanged path; signatures updated in Task 4)
    sdl/                                 (new directory — no CLAUDE.md; parent suffices)
      window.{h,cpp}                     (new — SDL_Window + SDL_Renderer lifetime/resize/vsync)
      input.{h,cpp}                      (moved — was src/platform/input_adapter.{h,cpp})

  ui/                                    (unchanged directory — generic Clay toolkit)
    CLAUDE.md                            (unchanged)
    AGENTS.md                            (unchanged — symlink)
    focus/
      ui_focus.{h,cpp}                   (unchanged)
    primitives/
      button.{h,cpp}                     (unchanged)
      clay_text.h                        (unchanged — header-only)
      focusable.{h,cpp}                  (unchanged)
      selectable.{h,cpp}                 (unchanged)
      toggle.{h,cpp}                     (unchanged)
      visual_state.h                     (unchanged — header-only)

  game/                                  (existing dir; src/game/ui/ deleted in Task 4)
    CLAUDE.md                            (new — game/ owns game truth; no Clay/SDL/UI imports)
    AGENTS.md                            (new — symlink to CLAUDE.md)
    shooter_game.{h,cpp}                 (moved + slimmed — façade; was src/shooter/shooter_game.*)
    player_state.h                       (new — health/armor/credits struct; header-only)
    weapon_defs.h                        (new — WeaponSpec)
    weapon_state.h                       (new — WeaponState: spec + owned + equipped)
    inventory.{h,cpp}                    (new — weapons array, select/equip/reset)
    economy.{h,cpp}                      (new — pure can_buy / try_buy functions)

  client/
    ui/                                  (existing directory)
      CLAUDE.md                          (unchanged path; rewritten in Task 8)
      AGENTS.md                          (unchanged — symlink)
      client_ui.{h,cpp}                  (unchanged path; include updated in Task 7)
      ui_pipeline.{h,cpp}                (moved + renamed — was src/game/ui/game_ui_pipeline.*)

      navigation/                        (new directory)
        screen_stack.{h,cpp}             (moved — was src/client/ui/screen_stack.*)
        ui_screen.h                      (moved — was src/client/ui/ui_screen.h)

      providers/                         (existing empty dir; populated in Task 6)
        shooter_provider.{h,cpp}         (new — ShooterContext + ShooterContextValue + ShooterProvider +
                                                use_shooter_game + use_request_quit)

      hooks/                             (existing empty dir; populated in Task 6)
        shooter_hud.{h,cpp}              (new — ShooterHudRead + use_shooter_hud)
        shooter_weapons.{h,cpp}          (new — ShooterWeaponRead +
                                                use_shooter_weapon_count + use_selected_weapon_index +
                                                use_weapon_read + use_select_weapon +
                                                use_buy_weapon + use_equip_weapon)

      components/                        (existing empty dir; populated in Task 6)
        hud_band.{h,cpp}                 (new — HudBand; used by in_game + pause + loadout)

      screens/                           (existing directory; screen-name/ template deleted)
        main_menu/                       (new dir)
          main_menu_screen.{h,cpp}       (new — MainMenuScreen class + view +
                                                use_exit_to_main_menu helper)
        in_game/                         (new dir; named in_game to avoid src/game/ readability clash)
          in_game_screen.{h,cpp}         (new — ShooterGameScreen class + view + use_start_match)
        pause/                           (new dir)
          pause_screen.{h,cpp}           (new — PauseScreen class + view + use_push_pause_screen)
        loadout/                         (new dir)
          CLAUDE.md                      (new — only screen with components subdir + screen-local state)
          AGENTS.md                      (new — symlink to CLAUDE.md)
          loadout_screen.{h,cpp}         (new — LoadoutScreen class + LoadoutScreenView +
                                                use_push_loadout_screen + compare_enabled state)
          components/
            weapon_tile.{h,cpp}          (new — WeaponTile + weapon_tile_id + weapon_in_tab +
                                                first_weapon_for_tab + LOADOUT_TAB_* constants)
            equipment_slot.{h,cpp}       (new — EquipmentSlot)
            confirm_dialog.{h,cpp}       (new — LoadoutConfirmDialog + LOADOUT_ACTION_* constants)
        options/                         (new dir)
          options_screen.{h,cpp}         (new — OptionsScreen class + view + use_push_options_screen)
```

**Files / directories deleted by this refactor:**

```text
src/shooter/                             (whole dir gone — game logic split to src/game/,
                                          UI split across src/client/ui/{providers,hooks,components,screens}/)
src/shooter/shooter_game.{h,cpp}         (decomposed in Task 5)
src/shooter/shooter_ui.{h,cpp}           (decomposed in Task 6)

src/game/ui/                             (whole dir gone — pipeline moved in Task 4)
src/game/ui/game_ui_pipeline.{h,cpp}     (moved to src/client/ui/ui_pipeline.*)

src/platform/input_adapter.{h,cpp}       (moved to src/platform/sdl/input.*)

src/client/ui/screen_stack.{h,cpp}       (moved to src/client/ui/navigation/)
src/client/ui/ui_screen.h                (moved to src/client/ui/navigation/)

src/client/ui/screens/screen-name/       (template tree — Task 7)
src/client/ui/screens/screen-name/components/
src/client/ui/screens/screen-name/hooks/
src/client/ui/screens/screen-name/lib/
src/client/ui/screens/screen-name/providers/
```

**Mapping `shooter_ui.cpp` (930 lines) onto the new tree:**

| `shooter_ui.cpp` symbol(s) | Destination |
| --- | --- |
| `ShooterContext`, `ShooterContextValue`, `ShooterProvider`, `use_shooter_game`, `use_request_quit` | `client/ui/providers/shooter_provider.{h,cpp}` |
| `ShooterHudRead`, `use_shooter_hud` | `client/ui/hooks/shooter_hud.{h,cpp}` |
| `ShooterWeaponRead`, `use_shooter_weapon_count`, `use_selected_weapon_index`, `use_weapon_read`, `use_select_weapon`, `use_buy_weapon`, `use_equip_weapon` | `client/ui/hooks/shooter_weapons.{h,cpp}` |
| `HudBand` | `client/ui/components/hud_band.{h,cpp}` |
| `MainMenuScreen::build_ui`, `MainMenuScreenView`, `use_exit_to_main_menu` | `client/ui/screens/main_menu/main_menu_screen.{h,cpp}` |
| `ShooterGameScreen::build_ui`, `ShooterGameScreenView`, `use_start_match` | `client/ui/screens/in_game/in_game_screen.{h,cpp}` |
| `PauseScreen::build_ui`, `PauseScreenView`, `use_push_pause_screen` | `client/ui/screens/pause/pause_screen.{h,cpp}` |
| `LoadoutScreen::build_ui`, `LoadoutScreenView`, `use_push_loadout_screen`, `use_compare_enabled`, `use_set_compare_enabled`, `compare_enabled_` field | `client/ui/screens/loadout/loadout_screen.{h,cpp}` |
| `WeaponTile`, `weapon_tile_id`, `weapon_in_tab`, `first_weapon_for_tab`, `LOADOUT_TAB_WEAPONS`, `LOADOUT_TAB_GEAR` | `client/ui/screens/loadout/components/weapon_tile.{h,cpp}` |
| `EquipmentSlot` | `client/ui/screens/loadout/components/equipment_slot.{h,cpp}` |
| `LoadoutConfirmDialog`, `LOADOUT_ACTION_NONE`, `LOADOUT_ACTION_BUY`, `LOADOUT_ACTION_EQUIP` | `client/ui/screens/loadout/components/confirm_dialog.{h,cpp}` |
| `OptionsScreen::build_ui`, `OptionsScreenView`, `use_push_options_screen` | `client/ui/screens/options/options_screen.{h,cpp}` |

**Why each "push X screen" hook lives with X (not with the screens that call it):** the destination screen owns the knowledge of how to construct itself (its constructor signature, dependencies). Callers `#include` the destination header and read off the hook — no circular deps because callers never need to know about each other.

**Notable non-changes (deliberate):**

- `namespace shooter` and the `ShooterGame` / `ShooterGameScreen` / `*Screen` class names are kept. The shooter IS the game, so renaming churn isn't worth it.
- No `audio/`, `vfx/`, `server/`, `net/` directories until something actually needs them.
- No `client/commands/` bus; deferred writes via `ClientUi::queue_deferred_write` stay the intent channel.
- No `client/ui/overlays/` or `hud/` folder; current pause/loadout screens use the `is_overlay()` flag on `UiScreen` — promote out of `screens/` only if real overlay/hud surfaces need distinct co-location.
- No new CLAUDE.md files in `app/`, `renderer/`, `game/`, `platform/sdl/`, or per-screen dirs. Add them later if a directory's responsibilities aren't obvious from `architecture.md` + the parent `CLAUDE.md`.

**This expands Task 6's scope considerably.** The original Task 6 spec described moving `shooter_ui.{h,cpp}` to a single `screens.{h,cpp}`. The new target above implies ~14 new file pairs and ~25 file-level moves of code. Task 6 needs to be re-decomposed before execution — see the note at the top of Task 6.

---

## Task 0: Baseline — confirm green before touching anything

**Files:** none (read-only verification).

- [ ] **Step 1: Build the full project**

Run: `.\build.ps1`
Expected: build succeeds. If not, fix or abandon plan until tree is green. (The wrapper handles vcvars + vcpkg toolchain for CURL; plain `cmake -S . -B build` fails on Windows because CURL isn't on the default search path. Build artifacts go to `cmake-build-debug/`.)

- [ ] **Step 2: Run all tests**

Run: `.\build.ps1 -Tests`
Expected: all of `react_runtime_tests`, `ui_focus_tests`, `ui_primitives_tests`, `client_ui_tests`, `game_ui_pipeline_tests`, `shooter_ui_tests`, `ui_cli_smoke`, `ui_cli_dm`, `ui_cli_commands` pass (9 total).

- [ ] **Step 3: Snapshot baseline status into the plan**

If anything fails: stop, fix the existing failure on `main`, then resume here. The whole plan relies on these tests as the safety net.

---

## Task 1: Extract `renderer/` (FontRegistry + SdlClayRenderer)

Pull font discovery, Clay text measurement, and Clay→SDL render glue out of `src/main.cpp` statics into a dedicated module.

**Files:**
- Create: `src/renderer/font_registry.h`
- Create: `src/renderer/font_registry.cpp`
- Create: `src/renderer/sdl_clay_renderer.h`
- Create: `src/renderer/sdl_clay_renderer.cpp`
- Modify: `src/main.cpp` (replace statics `open_some_font`, `measure_text`, `g_font`, `g_text_eng`, `g_clay_rd`)
- Modify: `CMakeLists.txt` (add to `hello` target sources)

- [ ] **Step 1: Write `src/renderer/font_registry.h`**

```cpp
#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <clay.h>

namespace renderer {

class FontRegistry {
public:
    FontRegistry() = default;
    ~FontRegistry();

    FontRegistry(const FontRegistry &) = delete;
    FontRegistry &operator=(const FontRegistry &) = delete;

    bool initialize(SDL_Renderer *renderer, float default_size = 16.0f);
    void shutdown();

    TTF_TextEngine *text_engine() const { return text_engine_; }
    TTF_Font       *default_font() const { return default_font_; }

    Clay_Dimensions measure(Clay_StringSlice text, Clay_TextElementConfig *config) const;

    static Clay_Dimensions measure_thunk(Clay_StringSlice text,
                                         Clay_TextElementConfig *config,
                                         void *user_data);

private:
    static TTF_Font *open_default_font(float pt_size);

    TTF_TextEngine *text_engine_  = nullptr;
    TTF_Font       *default_font_ = nullptr;
};

} // namespace renderer
```

- [ ] **Step 2: Write `src/renderer/font_registry.cpp`**

```cpp
#include "font_registry.h"

namespace renderer {

FontRegistry::~FontRegistry() {
    shutdown();
}

bool FontRegistry::initialize(SDL_Renderer *renderer, float default_size) {
    if (!renderer) return false;
    text_engine_ = TTF_CreateRendererTextEngine(renderer);
    if (!text_engine_) {
        SDL_Log("TTF_CreateRendererTextEngine: %s", SDL_GetError());
        return false;
    }
    default_font_ = open_default_font(default_size);
    if (!default_font_) {
        SDL_Log("no font found; tried system defaults");
        return false;
    }
    return true;
}

void FontRegistry::shutdown() {
    if (default_font_) {
        TTF_CloseFont(default_font_);
        default_font_ = nullptr;
    }
    if (text_engine_) {
        TTF_DestroyRendererTextEngine(text_engine_);
        text_engine_ = nullptr;
    }
}

TTF_Font *FontRegistry::open_default_font(float pt_size) {
    const char *candidates[] = {
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
    };
    for (const char *path : candidates) {
        TTF_Font *font = TTF_OpenFont(path, pt_size);
        if (font) {
            SDL_Log("loaded font: %s", path);
            return font;
        }
    }
    return nullptr;
}

Clay_Dimensions FontRegistry::measure(Clay_StringSlice text,
                                      Clay_TextElementConfig *config) const {
    if (!default_font_) return { 0, 0 };
    TTF_SetFontSize(default_font_, config->fontSize);
    int w = 0, h = 0;
    TTF_GetStringSize(default_font_, text.chars, (size_t)text.length, &w, &h);
    return { (float)w, (float)h };
}

Clay_Dimensions FontRegistry::measure_thunk(Clay_StringSlice text,
                                            Clay_TextElementConfig *config,
                                            void *user_data) {
    return static_cast<FontRegistry *>(user_data)->measure(text, config);
}

} // namespace renderer
```

- [ ] **Step 3: Write `src/renderer/sdl_clay_renderer.h`**

```cpp
#pragma once

#include <SDL3/SDL.h>

#include <clay.h>
#include <clay_renderer_SDL3.h>

#include "font_registry.h"

namespace renderer {

class SdlClayRenderer {
public:
    bool initialize(SDL_Renderer *renderer, FontRegistry &fonts);

    void clear(Clay_Color background);
    void render(Clay_RenderCommandArray &commands);
    void present();

    SDL_Renderer *sdl_renderer() const { return renderer_; }

private:
    SDL_Renderer            *renderer_ = nullptr;
    TTF_Font                *fonts_[1] = { nullptr };
    Clay_SDL3RendererData    data_     = {};
};

} // namespace renderer
```

- [ ] **Step 4: Write `src/renderer/sdl_clay_renderer.cpp`**

```cpp
#include "sdl_clay_renderer.h"

namespace renderer {

bool SdlClayRenderer::initialize(SDL_Renderer *renderer, FontRegistry &fonts) {
    if (!renderer || !fonts.text_engine() || !fonts.default_font()) return false;
    renderer_   = renderer;
    fonts_[0]   = fonts.default_font();
    data_.renderer   = renderer_;
    data_.textEngine = fonts.text_engine();
    data_.fonts      = fonts_;
    return true;
}

void SdlClayRenderer::clear(Clay_Color background) {
    SDL_SetRenderDrawColor(renderer_,
                           (Uint8)background.r,
                           (Uint8)background.g,
                           (Uint8)background.b,
                           (Uint8)background.a);
    SDL_RenderClear(renderer_);
}

void SdlClayRenderer::render(Clay_RenderCommandArray &commands) {
    SDL_Clay_RenderClayCommands(&data_, &commands);
}

void SdlClayRenderer::present() {
    SDL_RenderPresent(renderer_);
}

} // namespace renderer
```

- [ ] **Step 5: Update `CMakeLists.txt` — add the two new sources to the `hello` target**

Edit the `add_executable(hello ...)` list (currently `CMakeLists.txt:47-65`). After the existing `src/platform/input_adapter.cpp` line, add:
```
  src/renderer/font_registry.cpp
  src/renderer/sdl_clay_renderer.cpp
```

- [ ] **Step 6: Rewrite the renderer-touching parts of `src/main.cpp`**

In `src/main.cpp`:

Replace the `g_text_eng`, `g_font`, `g_clay_rd`, `open_some_font`, `measure_text` statics (`main.cpp:40-77`) with local instances of `renderer::FontRegistry` and `renderer::SdlClayRenderer`. Add includes near the top:

```cpp
#include "renderer/font_registry.h"
#include "renderer/sdl_clay_renderer.h"
```

In `main()`, replace the font/text-engine init block (~lines 157-166) with:
```cpp
    renderer::FontRegistry fonts;
    if (!fonts.initialize(g_renderer)) {
        return 1;
    }
    renderer::SdlClayRenderer clay_renderer;
    if (!clay_renderer.initialize(g_renderer, fonts)) {
        return 1;
    }
```

Replace `Clay_SetMeasureTextFunction(measure_text, nullptr);` with:
```cpp
    Clay_SetMeasureTextFunction(renderer::FontRegistry::measure_thunk, &fonts);
```

Replace the inline `SDL_SetRenderDrawColor` + `SDL_RenderClear` + `SDL_Clay_RenderClayCommands` + `SDL_RenderPresent` block (~lines 249-253) with:
```cpp
            clay_renderer.clear({ 12, 14, 22, 255 });
            clay_renderer.render(cmds);
            control.capture_after_render(g_renderer, ui_pipeline);
            clay_renderer.present();
```

Delete the shutdown lines `TTF_CloseFont(g_font); TTF_DestroyRendererTextEngine(g_text_eng);` (~lines 261-262). The `FontRegistry` destructor handles them.

Delete the `g_text_eng`, `g_font`, `g_clay_rd`, `open_some_font`, `measure_text` definitions in their entirety.

- [ ] **Step 7: Build**

Run: `.\build.ps1`
Expected: success. (Build artifacts go to `cmake-build-debug/`.)

- [ ] **Step 8: Run all tests**

Run: `.\build.ps1 -Tests`
Expected: all 9 tests green (including `ui_cli_smoke`, `ui_cli_commands`, which exercise the actual binary).

- [ ] **Step 9: Commit**

```
git add src/renderer src/main.cpp CMakeLists.txt
git commit -m "Extract renderer/ from main.cpp (FontRegistry + SdlClayRenderer)"
```

---

## Task 2: Extract `platform/sdl/` (window/renderer + input)

Move SDL window/renderer ownership out of `main.cpp` globals into `platform::sdl::Window`. Move `platform/input_adapter.{h,cpp}` → `platform/sdl/input.{h,cpp}` (same code, different home).

**Files:**
- Create: `src/platform/sdl/window.h`, `src/platform/sdl/window.cpp`
- Create: `src/platform/sdl/input.h` (content of old `input_adapter.h`)
- Create: `src/platform/sdl/input.cpp` (content of old `input_adapter.cpp`)
- Delete: `src/platform/input_adapter.h`, `src/platform/input_adapter.cpp`
- Modify: `src/main.cpp`, `src/platform/control_mailbox.cpp` (include path updates)
- Modify: `CMakeLists.txt` (every target that referenced `input_adapter.cpp`)

- [ ] **Step 1: Write `src/platform/sdl/window.h`**

```cpp
#pragma once

#include <SDL3/SDL.h>

namespace platform::sdl {

class Window {
public:
    Window() = default;
    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    bool initialize(const char *title, int width, int height, bool vsync);
    void shutdown();

    SDL_Window   *handle()   const { return window_; }
    SDL_Renderer *renderer() const { return renderer_; }

    void size(int *w, int *h) const;
    bool set_size(int w, int h);
    void set_vsync(bool enabled);

private:
    SDL_Window   *window_   = nullptr;
    SDL_Renderer *renderer_ = nullptr;
};

} // namespace platform::sdl
```

- [ ] **Step 2: Write `src/platform/sdl/window.cpp`**

```cpp
#include "window.h"

#include <stdio.h>

namespace platform::sdl {

Window::~Window() {
    shutdown();
}

bool Window::initialize(const char *title, int width, int height, bool vsync) {
    window_ = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    if (!window_) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return false;
    }
    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (!renderer_) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetRenderVSync(renderer_, vsync ? 1 : 0);
    return true;
}

void Window::shutdown() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
}

void Window::size(int *w, int *h) const {
    if (window_) SDL_GetWindowSize(window_, w, h);
}

bool Window::set_size(int w, int h) {
    return window_ && SDL_SetWindowSize(window_, w, h);
}

void Window::set_vsync(bool enabled) {
    if (renderer_) SDL_SetRenderVSync(renderer_, enabled ? 1 : 0);
}

} // namespace platform::sdl
```

- [ ] **Step 3: Move input adapter into `platform/sdl/`**

Run:
```
git mv src/platform/input_adapter.h  src/platform/sdl/input.h
git mv src/platform/input_adapter.cpp src/platform/sdl/input.cpp
```

- [ ] **Step 4: Update the include in `src/platform/sdl/input.cpp`**

Edit the first line:
```cpp
#include "input.h"
```
(was `#include "input_adapter.h"`).

The `#include "../../ui/focus/ui_focus.h"` line in `input.h` was `#include "../ui/focus/ui_focus.h"`. Update it to one more `..`:
```cpp
#include "../../ui/focus/ui_focus.h"
```

- [ ] **Step 5: Update `src/platform/control_mailbox.cpp` include**

In `src/platform/control_mailbox.cpp` line 3:
```cpp
#include "sdl/input.h"
```
(was `#include "input_adapter.h"`).

- [ ] **Step 6: Update `src/main.cpp` includes and window/renderer use**

Replace `#include "platform/input_adapter.h"` with:
```cpp
#include "platform/sdl/input.h"
#include "platform/sdl/window.h"
```

Replace the `g_window` / `g_renderer` static declarations and their `SDL_CreateWindow` / `SDL_CreateRenderer` / `SDL_SetRenderVSync` setup in `main()` with:
```cpp
    platform::sdl::Window window;
    if (!window.initialize("clay + react hello-world", 800, 500,
                           /*vsync=*/control_dir == nullptr)) {
        return 1;
    }
    SDL_Window   *g_window   = window.handle();
    SDL_Renderer *g_renderer = window.renderer();
```
(Keeping the `g_window` / `g_renderer` local aliases minimises further edits in this task; they go away in task 3.)

Replace the trailing `SDL_DestroyRenderer(g_renderer); SDL_DestroyWindow(g_window);` (~lines 263-264) with nothing (Window destructor handles them).

- [ ] **Step 7: Update `CMakeLists.txt` source lists**

Search-and-replace `src/platform/input_adapter.cpp` → `src/platform/sdl/input.cpp` in every target source list (currently appears once in the `hello` target at `CMakeLists.txt:54`).

In the `hello` target sources, add:
```
  src/platform/sdl/window.cpp
```

- [ ] **Step 8: Build**

Run: `.\build.ps1`
Expected: success. (Build artifacts go to `cmake-build-debug/`.)

- [ ] **Step 9: Run all tests**

Run: `.\build.ps1 -Tests`
Expected: all 9 tests green.

- [ ] **Step 10: Commit**

```
git add -A
git commit -m "Extract platform/sdl/ (Window + input)"
```

---

## Task 3: Extract `app/` (App + GameLoop), shrink `main.cpp`

Pull the rest of the SDL bootstrap, event polling, frame timing, and frame body out of `main.cpp` into `app::App` (owns subsystems, lifecycle) and `app::GameLoop` (per-frame body). Reduce `main.cpp` to ~15 lines.

**Files:**
- Create: `src/app/app.h`, `src/app/app.cpp`
- Create: `src/app/game_loop.h`, `src/app/game_loop.cpp`
- Modify: `src/main.cpp` (gut to a shim)
- Modify: `CMakeLists.txt` (add new sources to `hello` target)

- [ ] **Step 1: Write `src/app/app.h`**

```cpp
#pragma once

#include "../game/ui/game_ui_pipeline.h"
#include "../platform/control_mailbox.h"
#include "../platform/sdl/window.h"
#include "../renderer/font_registry.h"
#include "../renderer/sdl_clay_renderer.h"
#include "../shooter/shooter_game.h"

#include <clay.h>

namespace app {

struct AppOptions {
    const char *title       = "clay + react hello-world";
    int         width       = 800;
    int         height      = 500;
    const char *control_dir = nullptr;
};

class App {
public:
    App();
    ~App();

    App(const App &) = delete;
    App &operator=(const App &) = delete;

    bool initialize(const AppOptions &options);
    int  run();
    void shutdown();

private:
    static void on_clay_error(Clay_ErrorData err);

    AppOptions                     options_   = {};
    platform::sdl::Window          window_    = {};
    renderer::FontRegistry         fonts_     = {};
    renderer::SdlClayRenderer      clay_render_ = {};
    void                          *clay_arena_mem_ = nullptr;
    Clay_Context                  *clay_ctx_  = nullptr;
    game::ui::GameUiPipeline       ui_pipeline_ = {};
    shooter::ShooterGame           shooter_game_ = {};
    platform::ControlMailbox       control_   = {};
    bool                           initialized_ = false;
    bool                           running_   = true;
};

} // namespace app
```

- [ ] **Step 2: Write `src/app/app.cpp`**

```cpp
#include "app.h"

#include "game_loop.h"
#include "../react.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <curl/curl.h>

#include <sstream>
#include <stdio.h>

namespace app {

static std::string json_escape(const char *value) {
    std::string out;
    if (!value) return out;
    for (const char *p = value; *p; ++p) {
        switch (*p) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += *p;     break;
        }
    }
    return out;
}

static std::string shooter_state_json(const shooter::ShooterGame &game) {
    std::ostringstream out;
    out << "{"
        << "\"health\":" << game.health() << ","
        << "\"armor\":" << game.armor() << ","
        << "\"ammo\":" << game.ammo() << ","
        << "\"credits\":" << game.credits() << ","
        << "\"selected_weapon\":" << game.selected_weapon() << ","
        << "\"compare_enabled\":" << (game.compare_enabled() ? "true" : "false")
        << ",\"weapons\":[";
    for (int i = 0; i < game.weapon_count(); ++i) {
        const shooter::WeaponState &weapon = game.weapon(i);
        if (i) out << ",";
        out << "{"
            << "\"index\":" << i << ","
            << "\"name\":\"" << json_escape(weapon.spec.name) << "\","
            << "\"role\":\"" << json_escape(weapon.spec.role) << "\","
            << "\"cost\":" << weapon.spec.cost << ","
            << "\"damage\":" << weapon.spec.damage << ","
            << "\"ammo\":" << weapon.spec.ammo << ","
            << "\"owned\":" << (weapon.owned ? "true" : "false") << ","
            << "\"equipped\":" << (weapon.equipped ? "true" : "false") << ","
            << "\"can_buy\":" << (game.can_buy_weapon(i) ? "true" : "false") << ","
            << "\"can_equip\":" << (game.can_equip_weapon(i) ? "true" : "false")
            << "}";
    }
    out << "]}";
    return out.str();
}

void App::on_clay_error(Clay_ErrorData err) {
    fprintf(stderr, "clay: %.*s\n", (int)err.errorText.length, err.errorText.chars);
}

App::App()  = default;
App::~App() { shutdown(); }

bool App::initialize(const AppOptions &options) {
    options_ = options;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return false;
    }
    if (!TTF_Init()) {
        fprintf(stderr, "TTF_Init: %s\n", SDL_GetError());
        return false;
    }
    curl_global_init(CURL_GLOBAL_DEFAULT);

    if (!window_.initialize(options.title, options.width, options.height,
                            /*vsync=*/options.control_dir == nullptr)) {
        return false;
    }
    if (!fonts_.initialize(window_.renderer())) {
        return false;
    }
    if (!clay_render_.initialize(window_.renderer(), fonts_)) {
        return false;
    }

    uint32_t clay_mem_size = Clay_MinMemorySize();
    clay_arena_mem_ = SDL_malloc(clay_mem_size);
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(clay_mem_size, clay_arena_mem_);
    int win_w = options.width, win_h = options.height;
    window_.size(&win_w, &win_h);
    clay_ctx_ = Clay_Initialize(
        arena,
        Clay_Dimensions{ (float)win_w, (float)win_h },
        Clay_ErrorHandler{ on_clay_error, 0 });
    Clay_SetMeasureTextFunction(renderer::FontRegistry::measure_thunk, &fonts_);

    react_init(clay_ctx_);

    ui_pipeline_.client_ui().push_screen(
        std::make_unique<shooter::MainMenuScreen>(&shooter_game_, [this] {
            running_ = false;
        }));

    control_.set_game_state_json_provider([this] {
        return shooter_state_json(shooter_game_);
    });
    if (options.control_dir && !control_.init(options.control_dir)) {
        return false;
    }

    initialized_ = true;
    return true;
}

int App::run() {
    if (!initialized_) return 1;
    GameLoop loop(window_, clay_render_, ui_pipeline_, control_, running_);
    while (running_) {
        loop.tick();
    }
    return 0;
}

void App::shutdown() {
    if (!initialized_) return;
    control_.shutdown();
    react_shutdown();
    if (clay_arena_mem_) {
        SDL_free(clay_arena_mem_);
        clay_arena_mem_ = nullptr;
    }
    clay_render_ = {};
    fonts_.shutdown();
    window_.shutdown();
    TTF_Quit();
    SDL_Quit();
    curl_global_cleanup();
    initialized_ = false;
}

} // namespace app
```

- [ ] **Step 3: Write `src/app/game_loop.h`**

```cpp
#pragma once

#include "../game/ui/game_ui_pipeline.h"
#include "../platform/control_mailbox.h"
#include "../platform/sdl/window.h"
#include "../renderer/sdl_clay_renderer.h"

namespace app {

class GameLoop {
public:
    GameLoop(platform::sdl::Window      &window,
             renderer::SdlClayRenderer  &clay_render,
             game::ui::GameUiPipeline   &ui_pipeline,
             platform::ControlMailbox   &control,
             bool                       &running);

    void tick();

private:
    platform::sdl::Window      &window_;
    renderer::SdlClayRenderer  &clay_render_;
    game::ui::GameUiPipeline   &ui_pipeline_;
    platform::ControlMailbox   &control_;
    bool                       &running_;
    bool                        previous_pointer_down_ = false;
};

} // namespace app
```

- [ ] **Step 4: Write `src/app/game_loop.cpp`**

```cpp
#include "game_loop.h"

#include "../platform/sdl/input.h"
#include "../ui/focus/ui_focus.h"

#include <SDL3/SDL.h>

namespace app {

GameLoop::GameLoop(platform::sdl::Window      &window,
                   renderer::SdlClayRenderer  &clay_render,
                   game::ui::GameUiPipeline   &ui_pipeline,
                   platform::ControlMailbox   &control,
                   bool                       &running)
    : window_(window),
      clay_render_(clay_render),
      ui_pipeline_(ui_pipeline),
      control_(control),
      running_(running) {}

void GameLoop::tick() {
    ::ui::UiInputFrame ui_input = {};

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            case SDL_EVENT_QUIT:
                running_ = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (ev.key.repeat) break;
                platform::apply_key_down(ev.key.key, ui_input, &running_);
                break;
            case SDL_EVENT_KEY_UP:
                platform::apply_key_up(ev.key.key, ui_input);
                break;
            default:
                break;
        }
    }

    const bool *keys = SDL_GetKeyboardState(nullptr);
    ui_input.confirm_down = ui_input.confirm_down ||
        keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_SPACE];
    ui_input.cancel_down = ui_input.cancel_down || keys[SDL_SCANCODE_ESCAPE];

    float mx, my;
    Uint32 mb = SDL_GetMouseState(&mx, &my);
    bool pointer_down = (mb & SDL_BUTTON_LMASK) != 0;
    ui_input.pointer_down     = pointer_down;
    ui_input.pointer_pressed  = pointer_down && !previous_pointer_down_;
    ui_input.pointer_released = !pointer_down && previous_pointer_down_;
    if (ui_input.pointer_pressed || ui_input.pointer_released) {
        ui_input.source = ::ui::UiFocusSource::Mouse;
    }
    previous_pointer_down_ = pointer_down;

    control_.poll(ui_input, running_, window_.handle(), ui_pipeline_);
    if (control_.apply_pointer_override(mx, my, pointer_down)) {
        ui_input.pointer_down = pointer_down;
    }

    int frame_w = 800, frame_h = 500;
    window_.size(&frame_w, &frame_h);
    game::ui::GameUiFrame frame = {
        .input   = ui_input,
        .layout  = { (float)frame_w, (float)frame_h },
        .pointer = { mx, my },
    };

    ui_pipeline_.render_client_ui_frame(frame, [&](Clay_RenderCommandArray &cmds) {
        clay_render_.clear({ 12, 14, 22, 255 });
        clay_render_.render(cmds);
        control_.capture_after_render(clay_render_.sdl_renderer(), ui_pipeline_);
        clay_render_.present();
    });
    control_.finish_frame(ui_pipeline_);
}

} // namespace app
```

- [ ] **Step 5: Rewrite `src/main.cpp` to a shim**

Replace the entire contents of `src/main.cpp` with:

```cpp
#include "app/app.h"

#include <string.h>

int main(int argc, char **argv) {
    app::AppOptions options = {};
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--control-dir") == 0 && i + 1 < argc) {
            options.control_dir = argv[++i];
        }
    }

    app::App app;
    if (!app.initialize(options)) {
        return 1;
    }
    return app.run();
}
```

- [ ] **Step 6: Update `CMakeLists.txt`**

In the `hello` target sources, add:
```
  src/app/app.cpp
  src/app/game_loop.cpp
```

- [ ] **Step 7: Build**

Run: `.\build.ps1`
Expected: success. (Build artifacts go to `cmake-build-debug/`.)

- [ ] **Step 8: Run all tests**

Run: `.\build.ps1 -Tests`
Expected: all 9 tests green.

- [ ] **Step 9: Commit**

```
git add -A
git commit -m "Extract app/ (App + GameLoop) from main.cpp"
```

---

## Task 4: Move pipeline `game/ui/` → `client/ui/ui_pipeline`

The pipeline is UI infrastructure that knows ClientUi; `game/` should not host it. Rename:
- file: `src/game/ui/game_ui_pipeline.{h,cpp}` → `src/client/ui/ui_pipeline.{h,cpp}`
- namespace: `game::ui` → `client::ui`
- class: `GameUiPipeline` → `UiPipeline`
- struct: `GameUiFrame` → `UiPipelineFrame`
- hook: `use_game_ui_frame` → `use_ui_pipeline_frame`
- test: `tests/game_ui_pipeline_tests.cpp` → `tests/ui_pipeline_tests.cpp`, CMake target `ui_pipeline_tests`

**Files:**
- Create: `src/client/ui/ui_pipeline.h`, `src/client/ui/ui_pipeline.cpp` (moved + renamed from `src/game/ui/`)
- Delete: `src/game/ui/game_ui_pipeline.{h,cpp}` and the now-empty `src/game/ui/` directory
- Modify: `src/app/app.h`, `src/app/game_loop.{h,cpp}`, `src/platform/control_mailbox.{h,cpp}` (include + type updates)
- Move: `tests/game_ui_pipeline_tests.cpp` → `tests/ui_pipeline_tests.cpp`; update its includes/types
- Modify: `CMakeLists.txt` (rename target, rename source path)

- [ ] **Step 1: Move and rename files**

```
git mv src/game/ui/game_ui_pipeline.h   src/client/ui/ui_pipeline.h
git mv src/game/ui/game_ui_pipeline.cpp src/client/ui/ui_pipeline.cpp
git mv tests/game_ui_pipeline_tests.cpp tests/ui_pipeline_tests.cpp
```

Then delete the now-empty `src/game/ui` directory (some Windows shells leave it behind):
```
rmdir src\game\ui
rmdir src\game
```
(`src/game` itself becomes empty here; task 5 will re-populate it with the decomposed shooter game files.)

- [ ] **Step 2: Rewrite `src/client/ui/ui_pipeline.h`**

```cpp
#pragma once

#include <functional>

#include <clay.h>

#include "client_ui.h"
#include "../../ui/focus/ui_focus.h"

namespace client::ui {

struct UiPipelineFrame {
    ::ui::UiInputFrame input   = {};
    Clay_Dimensions    layout  = {};
    Clay_Vector2       pointer = {};
};

using RenderClayCommands = std::function<void(Clay_RenderCommandArray &)>;

class UiPipeline {
public:
    ClientUi       &client_ui()       { return client_ui_; }
    const ClientUi &client_ui() const { return client_ui_; }

    void render_client_ui_frame(const UiPipelineFrame    &frame,
                                const RenderClayCommands &render_commands);

private:
    ClientUi client_ui_;
};

const UiPipelineFrame *use_ui_pipeline_frame();

} // namespace client::ui
```

- [ ] **Step 3: Rewrite `src/client/ui/ui_pipeline.cpp`**

```cpp
#include "ui_pipeline.h"

#include "../../react.h"

namespace client::ui {

static ReactContext UiPipelineFrameContext = {};

const UiPipelineFrame *use_ui_pipeline_frame() {
    return static_cast<const UiPipelineFrame *>(use_context(&UiPipelineFrameContext));
}

void UiPipeline::render_client_ui_frame(const UiPipelineFrame    &frame,
                                        const RenderClayCommands &render_commands) {
    Clay_SetLayoutDimensions(frame.layout);
    Clay_SetPointerState(frame.pointer, frame.input.pointer_down);

    client_ui_.begin_frame(frame.input);
    react_begin_frame();
    Clay_BeginLayout();

    REACT_PROVIDER_ENTER("UiPipelineFrameProvider");
    PROVIDE(&UiPipelineFrameContext, const_cast<UiPipelineFrame *>(&frame)) {
        CLAY({
            .id = Clay_GetElementId(CLAY_STRING("ClientUiRoot")),
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            },
        }) {
            client_ui_.build_visible_screens();
        }
    }
    REACT_PROVIDER_EXIT();

    Clay_RenderCommandArray commands = Clay_EndLayout();
    client_ui_.end_layout(frame.input);
    react_end_frame();

    if (render_commands) {
        render_commands(commands);
    }

    client_ui_.drain_writes();
}

} // namespace client::ui
```

- [ ] **Step 4: Update `src/app/app.h`**

Replace `#include "../game/ui/game_ui_pipeline.h"` with:
```cpp
#include "../client/ui/ui_pipeline.h"
```
Replace `game::ui::GameUiPipeline ui_pipeline_ = {};` with:
```cpp
client::ui::UiPipeline ui_pipeline_ = {};
```

- [ ] **Step 5: Update `src/app/game_loop.h` and `src/app/game_loop.cpp`**

In `game_loop.h`:
- Replace `#include "../game/ui/game_ui_pipeline.h"` with `#include "../client/ui/ui_pipeline.h"`.
- Replace `game::ui::GameUiPipeline &ui_pipeline_` (parameter and member) with `client::ui::UiPipeline &ui_pipeline_`.

In `game_loop.cpp`:
- Replace `game::ui::GameUiFrame frame = { ... };` with `client::ui::UiPipelineFrame frame = { ... };`.

- [ ] **Step 6: Update `src/platform/control_mailbox.h`**

Replace `#include "../game/ui/game_ui_pipeline.h"` with:
```cpp
#include "../client/ui/ui_pipeline.h"
```
Replace every `game::ui::GameUiPipeline` with `client::ui::UiPipeline` (in `poll`, `capture_after_render`, `finish_frame`, `state_json` signatures).

- [ ] **Step 7: Update `src/platform/control_mailbox.cpp`**

Replace every `game::ui::GameUiPipeline` reference with `client::ui::UiPipeline` (parameter types). The body otherwise is unchanged — it already accesses `.client_ui()`, which exists on both.

- [ ] **Step 8: Update `tests/ui_pipeline_tests.cpp`**

Edit the file:
- Replace `#include "game/ui/game_ui_pipeline.h"` with `#include "client/ui/ui_pipeline.h"`.
- Replace `using game::ui::GameUiFrame;` and `using game::ui::GameUiPipeline;` with:
  ```cpp
  using client::ui::UiPipeline;
  using client::ui::UiPipelineFrame;
  ```
- Replace every other `GameUiFrame` with `UiPipelineFrame`, every `GameUiPipeline` with `UiPipeline`, and `game::ui::use_game_ui_frame()` with `client::ui::use_ui_pipeline_frame()`.
- Rename the test function `game_ui_frame_provider_exposes_current_frame` to `ui_pipeline_frame_provider_exposes_current_frame` (and update its call site in `main`).

- [ ] **Step 9: Update `CMakeLists.txt`**

In the `hello` target sources: replace `src/game/ui/game_ui_pipeline.cpp` with `src/client/ui/ui_pipeline.cpp`.

Rename the test target block. Currently `CMakeLists.txt:175-197`:
```
add_executable(game_ui_pipeline_tests
  tests/game_ui_pipeline_tests.cpp
  ...
  src/game/ui/game_ui_pipeline.cpp
  ...
)
...
add_test(NAME game_ui_pipeline_tests COMMAND game_ui_pipeline_tests)
```
Change to:
```
add_executable(ui_pipeline_tests
  tests/ui_pipeline_tests.cpp
  ...
  src/client/ui/ui_pipeline.cpp
  ...
)
...
add_test(NAME ui_pipeline_tests COMMAND ui_pipeline_tests)
```
(Update all five occurrences of the name.)

- [ ] **Step 10: Build**

Run: `.\build.ps1`
Expected: success. (Build artifacts go to `cmake-build-debug/`.)

- [ ] **Step 11: Run all tests**

Run: `.\build.ps1 -Tests`
Expected: all 9 tests green; previously named `game_ui_pipeline_tests` is now `ui_pipeline_tests`.

- [ ] **Step 12: Commit**

```
git add -A
git commit -m "Move pipeline from game/ to client/ui/ as UiPipeline"
```

---

## Task 5: Decompose `ShooterGame` into `game/`

Split the monolithic `ShooterGame` into HL1/CS1.6-style modules:
- `player_state` — health, armor, credits.
- `weapon_defs` — `WeaponSpec`.
- `weapon_state` — `WeaponState` (per-instance: owned, equipped, ammo).
- `inventory` — owns the weapon array, equip/select.
- `economy` — pure functions for purchase validation/application.
- `shooter_game` — slim aggregate that owns `PlayerState` + `Inventory` and exposes the same public API the UI consumed before.

Keep the public surface of `shooter::ShooterGame` stable so no UI code changes in this task. UI hook for `compare_enabled` is **not** removed here (task 7 deals with it).

**Files:**
- Create:
  - `src/game/player_state.h`, `src/game/player_state.cpp`
  - `src/game/weapon_defs.h`
  - `src/game/weapon_state.h`
  - `src/game/inventory.h`, `src/game/inventory.cpp`
  - `src/game/economy.h`, `src/game/economy.cpp`
  - `src/game/shooter_game.h`, `src/game/shooter_game.cpp`
- Delete: `src/shooter/shooter_game.{h,cpp}`
- Modify: `src/shooter/shooter_ui.{h,cpp}` (include path only — `shooter_game.h` lives elsewhere now)
- Modify: `src/app/app.h`, `src/app/app.cpp` (include path)
- Modify: `tests/shooter_ui_tests.cpp` (include path)
- Modify: `CMakeLists.txt` (every reference to `src/shooter/shooter_game.cpp`)

- [ ] **Step 1: Write `src/game/weapon_defs.h`**

```cpp
#pragma once

namespace shooter {

struct WeaponSpec {
    const char *name   = "";
    const char *role   = "";
    int         cost   = 0;
    int         damage = 0;
    int         ammo   = 0;
};

} // namespace shooter
```

- [ ] **Step 2: Write `src/game/weapon_state.h`**

```cpp
#pragma once

#include "weapon_defs.h"

namespace shooter {

struct WeaponState {
    WeaponSpec spec     = {};
    bool       owned    = false;
    bool       equipped = false;
};

} // namespace shooter
```

- [ ] **Step 3: Write `src/game/player_state.h`**

```cpp
#pragma once

namespace shooter {

struct PlayerState {
    int health  = 86;
    int armor   = 42;
    int credits = 450;

    bool can_afford(int cost) const { return credits >= cost; }
    void spend(int cost)            { credits -= cost; }
};

} // namespace shooter
```

(`player_state.cpp` is not needed — all logic is inline. Drop it from the file list.)

- [ ] **Step 4: Adjust plan — remove `player_state.cpp` from file list**

Update the create list above accordingly. The CMake additions in step 12 only include the `.cpp` files; `player_state.h` is header-only.

- [ ] **Step 5: Write `src/game/inventory.h`**

```cpp
#pragma once

#include <array>

#include "weapon_state.h"

namespace shooter {

constexpr int SHOOTER_WEAPON_COUNT = 4;

class Inventory {
public:
    Inventory();

    int weapon_count() const { return SHOOTER_WEAPON_COUNT; }
    const WeaponState &weapon(int index) const { return weapons_[index]; }
          WeaponState &weapon_mut(int index)   { return weapons_[index]; }

    int selected() const { return selected_; }

    bool can_equip(int index) const;

    void select(int index);
    bool equip(int index);
    void mark_owned(int index);
    void reset();

private:
    std::array<WeaponState, SHOOTER_WEAPON_COUNT> weapons_ = {};
    int                                           selected_ = 0;
};

} // namespace shooter
```

- [ ] **Step 6: Write `src/game/inventory.cpp`**

```cpp
#include "inventory.h"

namespace shooter {

Inventory::Inventory() {
    weapons_[0] = {
        .spec = { .name = "Vandal",   .role = "Rifle",        .cost = 0,   .damage = 38,  .ammo = 24 },
        .owned    = true,
        .equipped = true,
    };
    weapons_[1] = {
        .spec = { .name = "Bulldog",  .role = "Burst rifle",  .cost = 300, .damage = 32,  .ammo = 30 },
    };
    weapons_[2] = {
        .spec = { .name = "Operator", .role = "Sniper",       .cost = 700, .damage = 120, .ammo = 5  },
    };
    weapons_[3] = {
        .spec = { .name = "Aegis",    .role = "Armor kit",    .cost = 250, .damage = 0,   .ammo = 1  },
    };
}

bool Inventory::can_equip(int index) const {
    if (index < 0 || index >= weapon_count()) return false;
    return weapons_[index].owned;
}

void Inventory::select(int index) {
    if (index < 0 || index >= weapon_count()) return;
    selected_ = index;
}

bool Inventory::equip(int index) {
    if (!can_equip(index)) return false;
    for (WeaponState &item : weapons_) item.equipped = false;
    weapons_[index].equipped = true;
    selected_ = index;
    return true;
}

void Inventory::mark_owned(int index) {
    if (index < 0 || index >= weapon_count()) return;
    weapons_[index].owned = true;
}

void Inventory::reset() {
    *this = Inventory();
}

} // namespace shooter
```

- [ ] **Step 7: Write `src/game/economy.h`**

```cpp
#pragma once

namespace shooter {

struct PlayerState;
class  Inventory;

namespace economy {

bool can_buy_weapon(const PlayerState &player, const Inventory &inventory, int index);
bool try_buy_weapon(PlayerState &player, Inventory &inventory, int index);

} // namespace economy
} // namespace shooter
```

- [ ] **Step 8: Write `src/game/economy.cpp`**

```cpp
#include "economy.h"

#include "inventory.h"
#include "player_state.h"

namespace shooter::economy {

bool can_buy_weapon(const PlayerState &player, const Inventory &inventory, int index) {
    if (index < 0 || index >= inventory.weapon_count()) return false;
    const WeaponState &item = inventory.weapon(index);
    return !item.owned && player.can_afford(item.spec.cost);
}

bool try_buy_weapon(PlayerState &player, Inventory &inventory, int index) {
    if (!can_buy_weapon(player, inventory, index)) return false;
    player.spend(inventory.weapon(index).spec.cost);
    inventory.mark_owned(index);
    return true;
}

} // namespace shooter::economy
```

- [ ] **Step 9: Write `src/game/shooter_game.h`**

This file is the façade — public surface must match the old `ShooterGame` so the UI compiles unchanged.

```cpp
#pragma once

#include "inventory.h"
#include "player_state.h"
#include "weapon_state.h"

namespace shooter {

class ShooterGame {
public:
    int health()          const { return player_.health; }
    int armor()           const { return player_.armor; }
    int credits()         const { return player_.credits; }
    int selected_weapon() const { return inventory_.selected(); }
    int ammo()            const;
    bool compare_enabled() const { return compare_enabled_; }

    const WeaponState &weapon(int index) const { return inventory_.weapon(index); }
    int  weapon_count()      const { return inventory_.weapon_count(); }
    bool can_buy_weapon(int index)   const;
    bool can_equip_weapon(int index) const { return inventory_.can_equip(index); }

    void select_weapon(int index) { inventory_.select(index); }
    bool buy_weapon(int index);
    bool equip_weapon(int index)  { return inventory_.equip(index); }
    void reset();
    void set_compare_enabled(bool enabled) { compare_enabled_ = enabled; }

    // Direct access for future migration of UI hooks. For now only the façade is used.
    PlayerState &player() { return player_; }
    Inventory   &inventory() { return inventory_; }

private:
    PlayerState player_     = {};
    Inventory   inventory_  = {};
    bool        compare_enabled_ = false;  // UI state — extracted in task 7
};

} // namespace shooter
```

- [ ] **Step 10: Write `src/game/shooter_game.cpp`**

```cpp
#include "shooter_game.h"

#include "economy.h"

namespace shooter {

int ShooterGame::ammo() const {
    return inventory_.weapon(inventory_.selected()).spec.ammo;
}

bool ShooterGame::can_buy_weapon(int index) const {
    return economy::can_buy_weapon(player_, inventory_, index);
}

bool ShooterGame::buy_weapon(int index) {
    return economy::try_buy_weapon(player_, inventory_, index);
}

void ShooterGame::reset() {
    *this = ShooterGame();
}

} // namespace shooter
```

- [ ] **Step 11: Update includes in old shooter UI**

`src/shooter/shooter_ui.h` line 4 (`#include "shooter_game.h"`) → `#include "../game/shooter_game.h"`.

`src/shooter/shooter_ui.cpp`: if it has any direct include of `shooter_game.h`, update it the same way. (It pulls types via `shooter_ui.h`; usually no change needed. Verify.)

`src/app/app.h` line `#include "../shooter/shooter_game.h"` → `#include "../game/shooter_game.h"`.

`tests/shooter_ui_tests.cpp`: any `#include "shooter/shooter_game.h"` → `#include "game/shooter_game.h"`. (Keep `#include "shooter/shooter_ui.h"` — the UI screens haven't moved yet.)

- [ ] **Step 12: Delete the old `src/shooter/shooter_game.{h,cpp}`**

```
git rm src/shooter/shooter_game.h src/shooter/shooter_game.cpp
```

- [ ] **Step 13: Update `CMakeLists.txt`**

Find every line referencing `src/shooter/shooter_game.cpp` and replace with these (all four `.cpp` files in this module):
```
  src/game/inventory.cpp
  src/game/economy.cpp
  src/game/shooter_game.cpp
```
Appears in: `hello` target (~line 55) and `shooter_ui_tests` target (~line 204).

- [ ] **Step 14: Build**

Run: `.\build.ps1`
Expected: success. (Build artifacts go to `cmake-build-debug/`.) UI code is unchanged because the façade preserves the same API.

- [ ] **Step 15: Run all tests**

Run: `.\build.ps1 -Tests`
Expected: all 9 tests green. Behavior is unchanged.

- [ ] **Step 16: Commit**

```
git add -A
git commit -m "Decompose ShooterGame into game/{player_state,inventory,economy,...}"
```

---

## Task 6: Decompose `shooter_ui.cpp` into the new `client/ui/` tree

`src/shooter/shooter_ui.cpp` is 930 lines holding ~16 hooks, 1 provider, 4 components, and 5 screens. Split it across `client/ui/providers/`, `client/ui/hooks/`, `client/ui/components/`, and per-screen subdirs of `client/ui/screens/` per the **mapping table in the end-state hierarchy above** — that table is authoritative for what code goes where. Execute in four sub-tasks; each ends with `.\build.ps1 -Tests` green and one commit.

### Conventions for all sub-tasks

- Use `.\build.ps1 -Tests` to build + test (not plain `cmake`). Build dir is `cmake-build-debug`.
- Symbol moves: cut from `shooter_ui.cpp`, paste into the new file. Don't rewrite logic. The destination file gets its own `#include` block — copy whatever the source needed, then trim what isn't actually used.
- Include path math from `src/client/ui/<sub>/<file>` to `src/X`: `../../../X` (three levels up). From `src/client/ui/screens/<screen>/<file>`: `../../../../X` (four levels up).
- Keep `namespace shooter { ... }` around moved symbols. Anonymous namespaces stay anonymous.
- `static` free functions used only in their new TU stay `static`. Hooks needed by other TUs become `extern` (non-static) in the new header.
- After each sub-task, `src/shooter/shooter_ui.cpp` either still owns the rest or is deleted; `shooter_ui_tests` must still build and pass.

---

### Task 6a: Extract shared pieces (provider + hud/weapons hooks + HudBand)

**Files:**
- Create: `src/client/ui/providers/shooter_provider.{h,cpp}`
- Create: `src/client/ui/hooks/shooter_hud.{h,cpp}`
- Create: `src/client/ui/hooks/shooter_weapons.{h,cpp}`
- Create: `src/client/ui/components/hud_band.{h,cpp}`
- Modify: `src/shooter/shooter_ui.cpp` (delete moved symbols; add `#include`s to the new headers)
- Modify: `CMakeLists.txt` (add the 4 new `.cpp` files to `hello` and `shooter_ui_tests`)

- [ ] **Step 1: Create `src/client/ui/providers/shooter_provider.h`**

Move into a `namespace shooter` block in this header:
  - `struct ShooterContext` (definition)
  - Forward decl of `ShooterGame` (or `#include "../../../game/shooter_game.h"`)
  - Function decls: `ShooterGame *use_shooter_game();` and `std::function<void()> use_request_quit();`
  - Function decl: `void ShooterProvider(ShooterGame *game, const std::function<void()> &request_quit, const std::function<void()> &children);`
  - **Do not** put `ShooterContextValue` here — that stays a `static ReactContext` inside the `.cpp` (translation-unit-local), with the helpers reaching it.

Includes the header needs: `<functional>`, `"../../../react.h"`, `"../../../game/shooter_game.h"`.

- [ ] **Step 2: Create `src/client/ui/providers/shooter_provider.cpp`**

Move into this TU:
  - `static ReactContext ShooterContextValue = {};` (line 55 of shooter_ui.cpp)
  - `use_shooter_game` body (lines 57-61)
  - `use_request_quit` body (lines 63-67)
  - `ShooterProvider` body (lines 217-226)

Includes: `"shooter_provider.h"`, `"../../../react.h"`.

- [ ] **Step 3: Create `src/client/ui/hooks/shooter_hud.h`**

Move into a `namespace shooter` block:
  - `struct ShooterHudRead` (definition, lines 27-33)
  - Function decl: `ShooterHudRead use_shooter_hud();`

Includes: nothing beyond `<>` for free.

- [ ] **Step 4: Create `src/client/ui/hooks/shooter_hud.cpp`**

Move the `use_shooter_hud` body (lines 69-79). It depends on `use_shooter_game` — `#include "../providers/shooter_provider.h"`.

- [ ] **Step 5: Create `src/client/ui/hooks/shooter_weapons.h`**

Move into a `namespace shooter` block:
  - `struct ShooterWeaponRead` (definition, lines 35-47)
  - Function decls for: `use_shooter_weapon_count`, `use_selected_weapon_index`, `use_weapon_read(int)`, `use_select_weapon(int)`, `use_buy_weapon(int)`, `use_equip_weapon(int)`.

Includes: `<functional>` for the `std::function` return types.

- [ ] **Step 6: Create `src/client/ui/hooks/shooter_weapons.cpp`**

Move bodies of the 6 weapon hooks (lines 81-146). Depends on `use_shooter_game` and the ClientUi write queue (`client::ui::use_ui_write_queue`).

Includes: `"shooter_weapons.h"`, `"../providers/shooter_provider.h"`, `"../client_ui.h"`, `"../../../game/shooter_game.h"`, `"../../../react.h"`.

- [ ] **Step 7: Create `src/client/ui/components/hud_band.h`**

Move into `namespace shooter`:
  - Function decl: `void HudBand();`

Includes: nothing.

- [ ] **Step 8: Create `src/client/ui/components/hud_band.cpp`**

Move the `HudBand` body (lines 228-266 — the `REACT_FRAGMENT_COMPONENT_BEGIN("HudBand")` block).

Includes: `"hud_band.h"`, `"../hooks/shooter_hud.h"`, `"../../../react.h"`, `"../../../ui/primitives/clay_text.h"`, plus any other primitives the body uses (scan for `CLAY_TEXT(`, `::ui::Button`, etc.). Adopt whatever subset of `shooter_ui.cpp`'s primitive includes the body actually references.

- [ ] **Step 9: Edit `src/shooter/shooter_ui.cpp`**

- Add includes at the top (after the existing project includes):
  ```cpp
  #include "../client/ui/providers/shooter_provider.h"
  #include "../client/ui/hooks/shooter_hud.h"
  #include "../client/ui/hooks/shooter_weapons.h"
  #include "../client/ui/components/hud_band.h"
  ```
- Delete every symbol that moved: `ShooterContext` struct, `ShooterHudRead`, `ShooterWeaponRead`, `ShooterContextValue`, `use_shooter_game`, `use_request_quit`, `use_shooter_hud`, all 6 weapon hooks, `ShooterProvider`, `HudBand`.
- Everything else (screens, push hooks, loadout components, helpers, constants) stays put.

- [ ] **Step 10: Update `CMakeLists.txt`**

In both `hello` and `shooter_ui_tests` target source lists, add (sorted alphabetically alongside the other `src/client/ui/...` sources):
```
  src/client/ui/components/hud_band.cpp
  src/client/ui/hooks/shooter_hud.cpp
  src/client/ui/hooks/shooter_weapons.cpp
  src/client/ui/providers/shooter_provider.cpp
```

- [ ] **Step 11: Build + test**

Run: `.\build.ps1 -Tests`
Expected: all 9 tests green.

- [ ] **Step 12: Commit**

```
git add -A
git commit -m "Extract shooter provider, hud/weapons hooks, and HudBand to client/ui/{providers,hooks,components}"
```

---

### Task 6b: Move 4 simple screens into per-screen dirs

`MainMenu`, `ShooterGame` (in_game), `Pause`, `Options` each become a dir with one `<name>_screen.{h,cpp}` pair. `LoadoutScreen` stays in `src/shooter/shooter_ui.*` for now.

**Files:**
- Create: `src/client/ui/screens/main_menu/main_menu_screen.{h,cpp}`
- Create: `src/client/ui/screens/in_game/in_game_screen.{h,cpp}`
- Create: `src/client/ui/screens/pause/pause_screen.{h,cpp}`
- Create: `src/client/ui/screens/options/options_screen.{h,cpp}`
- Modify: `src/shooter/shooter_ui.{h,cpp}` (remove the 4 moved screens; keep `LoadoutScreen`)
- Modify: `src/app/app.h` and/or `src/app/app.cpp` (include path for `MainMenuScreen`)
- Modify: `tests/shooter_ui_tests.cpp` (include paths)
- Modify: `CMakeLists.txt` (add the 4 new `.cpp` files)

For each of the four screens, the destination header declares:
- The screen class (definition moves from `shooter_ui.h`).
- The "push X" hook decl (e.g., `std::function<void()> use_push_pause_screen();`). `MainMenuScreen` exports `use_exit_to_main_menu` instead (calls `reset_to(MainMenuScreen)`); `ShooterGameScreen` exports `use_start_match`.

For each `.cpp`, move from `shooter_ui.cpp`:
- The `<X>ScreenView` function.
- The push/start/exit hook body.
- The screen class's `build_ui()` definition.

Header `#include` map:
- `main_menu_screen.h` / `in_game_screen.h` / `pause_screen.h` / `options_screen.h`: `<functional>`, `<memory>`, `"../../navigation/ui_screen.h"` (will become `../../navigation/ui_screen.h` after Task 7 — for now `"../../ui_screen.h"` since nav move hasn't happened), `"../../../../game/shooter_game.h"`.
- `.cpp` files: their own `.h` plus `"../../../react.h"`, `"../../providers/shooter_provider.h"`, the relevant hooks/components headers, and any `ui/primitives/` headers their views use.

- [ ] **Step 1: For each of the 4 screens** (do MainMenu first, then in_game, then pause, then options; build between to catch errors early):

  a. Create the destination `.h` and `.cpp` per the recipes above.
  b. Move the screen class from `shooter_ui.h` (delete it there).
  c. Move the `View` function, the push/start/exit hook, and the `build_ui()` def from `shooter_ui.cpp` (delete them there).
  d. Anywhere else in `shooter_ui.cpp` that references the moved push/start/exit hook (e.g., `MainMenuScreenView` calls `use_push_options_screen`, `use_push_loadout_screen`), add `#include` of the new header.
  e. Run `.\build.ps1` (no `-Tests`) — fix include errors before moving on.

- [ ] **Step 2: Update `src/app/app.cpp` (or wherever `MainMenuScreen` is constructed)**

The initial `push_screen(std::make_unique<shooter::MainMenuScreen>(...))` call needs `#include "../client/ui/screens/main_menu/main_menu_screen.h"` instead of (or in addition to) the old `shooter_ui.h`. Add it.

- [ ] **Step 3: Update `tests/shooter_ui_tests.cpp`**

Add `#include "client/ui/screens/main_menu/main_menu_screen.h"` etc. for whichever screens the tests construct. Keep `#include "shooter/shooter_ui.h"` for `LoadoutScreen` (still lives there until 6c).

- [ ] **Step 4: Update `CMakeLists.txt`**

In `hello` and `shooter_ui_tests` source lists, add:
```
  src/client/ui/screens/in_game/in_game_screen.cpp
  src/client/ui/screens/main_menu/main_menu_screen.cpp
  src/client/ui/screens/options/options_screen.cpp
  src/client/ui/screens/pause/pause_screen.cpp
```

- [ ] **Step 5: Build + test**

Run: `.\build.ps1 -Tests`
Expected: all 9 tests green.

- [ ] **Step 6: Commit**

```
git add -A
git commit -m "Split main_menu/in_game/pause/options screens into per-screen dirs under client/ui/screens/"
```

---

### Task 6c: Split LoadoutScreen and its 3 components

Move `LoadoutScreen` + its view + its push hook + its compare-enabled hooks into `client/ui/screens/loadout/loadout_screen.{h,cpp}`. Split `WeaponTile`, `EquipmentSlot`, `LoadoutConfirmDialog` (and their helpers/constants) into `client/ui/screens/loadout/components/`. After this, `src/shooter/` is empty and gets deleted.

**Files:**
- Create:
  - `src/client/ui/screens/loadout/loadout_screen.{h,cpp}`
  - `src/client/ui/screens/loadout/components/weapon_tile.{h,cpp}`
  - `src/client/ui/screens/loadout/components/equipment_slot.{h,cpp}`
  - `src/client/ui/screens/loadout/components/confirm_dialog.{h,cpp}`
- Delete: `src/shooter/shooter_ui.{h,cpp}` and the now-empty `src/shooter/` directory.
- Modify: `src/app/app.*` includes; `tests/shooter_ui_tests.cpp` includes; `CMakeLists.txt`.

Symbol → file mapping:

| File | Symbols moved from `shooter_ui.cpp` |
| --- | --- |
| `loadout/loadout_screen.h` | `LoadoutScreen` class (from `shooter_ui.h`) + decls for `use_push_loadout_screen`, `use_compare_enabled`, `use_set_compare_enabled` |
| `loadout/loadout_screen.cpp` | `use_push_loadout_screen` body, `use_compare_enabled` + `use_set_compare_enabled` bodies, `LoadoutScreenView` (lines 659-845), `LoadoutScreen::build_ui()` def (lines 914-920) |
| `loadout/components/weapon_tile.{h,cpp}` | `LOADOUT_TAB_WEAPONS`, `LOADOUT_TAB_GEAR` constants (lines 52-53), `weapon_tile_id`, `weapon_in_tab`, `first_weapon_for_tab` helpers (lines 424-435), `WeaponTile` (lines 437-501) |
| `loadout/components/equipment_slot.{h,cpp}` | `EquipmentSlot` (lines 503-558) |
| `loadout/components/confirm_dialog.{h,cpp}` | `LOADOUT_ACTION_NONE`, `LOADOUT_ACTION_BUY`, `LOADOUT_ACTION_EQUIP` constants (lines 49-51), `LoadoutConfirmDialog` (lines 560-657) |

- [ ] **Step 1: Create the 4 destination header/cpp pairs**

Each header lives in `namespace shooter`. The component headers expose only the public function (e.g., `void WeaponTile(int index, int *selected_index);`); constants and helpers stay in the `.cpp` as `static` unless other components need them. `LOADOUT_TAB_*` constants are used by both `WeaponTile` and `LoadoutScreenView` — expose them via `weapon_tile.h`. `LOADOUT_ACTION_*` constants are used by both `LoadoutScreenView` and `LoadoutConfirmDialog` — expose via `confirm_dialog.h`.

- [ ] **Step 2: Move the symbols**

Cut from `shooter_ui.cpp` and paste into the destinations. Headers needed by each `.cpp`:
- `loadout_screen.cpp`: own header + `"../../navigation/ui_screen.h"` (or `../../ui_screen.h` until Task 7), `"components/weapon_tile.h"`, `"components/equipment_slot.h"`, `"components/confirm_dialog.h"`, `"../../components/hud_band.h"`, `"../../providers/shooter_provider.h"`, `"../../hooks/shooter_hud.h"`, `"../../hooks/shooter_weapons.h"`, `"../../client_ui.h"`, `"../../../react.h"`, `"../../../../game/shooter_game.h"`, plus the `ui/primitives/...` headers the view uses.
- Component `.cpp`s: their own header + `"../../../providers/shooter_provider.h"`, `"../../../hooks/shooter_weapons.h"`, `"../../../../react.h"`, plus relevant primitives.

- [ ] **Step 3: Delete `src/shooter/shooter_ui.{h,cpp}` and `src/shooter/`**

```
git rm src/shooter/shooter_ui.h src/shooter/shooter_ui.cpp
```
PowerShell will remove the now-empty dir on commit; if not, `rmdir src\shooter`.

- [ ] **Step 4: Update `src/app/app.h`**

Replace `#include "../shooter/shooter_ui.h"` (or whatever brings in `MainMenuScreen` after 6b) with the per-screen include set if not already done. Also add `#include "../client/ui/screens/loadout/loadout_screen.h"` if `app.cpp` references `LoadoutScreen` (it shouldn't — only constructs `MainMenuScreen` — but verify).

- [ ] **Step 5: Update `tests/shooter_ui_tests.cpp`**

Replace `#include "shooter/shooter_ui.h"` with the screen-specific includes the tests need (`client/ui/screens/main_menu/main_menu_screen.h`, `.../loadout/loadout_screen.h`, etc.).

- [ ] **Step 6: Update `CMakeLists.txt`**

In `hello` and `shooter_ui_tests` source lists:
- Remove `src/shooter/shooter_ui.cpp`.
- Add (alphabetical):
  ```
  src/client/ui/screens/loadout/components/confirm_dialog.cpp
  src/client/ui/screens/loadout/components/equipment_slot.cpp
  src/client/ui/screens/loadout/components/weapon_tile.cpp
  src/client/ui/screens/loadout/loadout_screen.cpp
  ```

- [ ] **Step 7: Build + test**

Run: `.\build.ps1 -Tests`
Expected: all 9 tests green. `src/shooter/` no longer exists.

- [ ] **Step 8: Commit**

```
git add -A
git commit -m "Split LoadoutScreen into client/ui/screens/loadout/{loadout_screen, components/*}; delete src/shooter/"
```

---

### Task 6d: Lift `compare_enabled` from `ShooterGame` into `LoadoutScreen`

`compare_enabled` is UI state — purely "should the loadout screen show the compare panel?". It has no game meaning. Move it off `ShooterGame` onto `LoadoutScreen`, and let descendant hooks reach it via a React context.

**Files:**
- Modify: `src/game/shooter_game.h` (delete the field + accessors)
- Modify: `src/client/ui/screens/loadout/loadout_screen.h` (add private field + getter/setter)
- Modify: `src/client/ui/screens/loadout/loadout_screen.cpp` (add screen-local React context; rewrite `use_compare_enabled` / `use_set_compare_enabled` to read screen, not game; wrap `build_ui` body in `PROVIDE(&LoadoutScreenContext, this)`)
- Modify: `src/app/app.cpp` (drop `compare_enabled` field from `shooter_state_json`)
- Possibly: `tests/ui_cli_commands.py` (if it asserts the JSON field)

- [ ] **Step 1: Remove from `ShooterGame`**

Edit `src/game/shooter_game.h`:
- Delete `bool compare_enabled() const { return compare_enabled_; }`
- Delete `void set_compare_enabled(bool enabled) { compare_enabled_ = enabled; }`
- Delete the private field `bool compare_enabled_ = false;`

- [ ] **Step 2: Add to `LoadoutScreen`**

In `src/client/ui/screens/loadout/loadout_screen.h`, add to the class body:
```cpp
public:
    bool compare_enabled() const { return compare_enabled_; }
    void set_compare_enabled(bool value) { compare_enabled_ = value; }

private:
    bool compare_enabled_ = false;
```

- [ ] **Step 3: Wire the context provider in `loadout_screen.cpp`**

At file scope (anonymous namespace or `static`):
```cpp
static ReactContext LoadoutScreenContext = {};

static LoadoutScreen *use_current_loadout_screen() {
    return static_cast<LoadoutScreen *>(use_context(&LoadoutScreenContext));
}
```

Rewrite the existing `use_compare_enabled` and `use_set_compare_enabled` bodies to call `use_current_loadout_screen()` instead of `use_shooter_game()`:
```cpp
static bool use_compare_enabled() {
    LoadoutScreen *screen = use_current_loadout_screen();
    return screen ? screen->compare_enabled() : false;
}

static std::function<void(bool)> use_set_compare_enabled() {
    LoadoutScreen *screen = use_current_loadout_screen();
    client::ui::QueueUiWrite queue_write = client::ui::use_ui_write_queue();
    return [screen, queue_write](bool enabled) {
        if (screen && queue_write) {
            queue_write([screen, enabled] { screen->set_compare_enabled(enabled); });
        }
    };
}
```

Wrap `LoadoutScreen::build_ui()`'s body in the provider — if the existing body is `REACT_COMPONENT_BEGIN_KEY("LoadoutScreen", entry_id()) { ... } REACT_COMPONENT_END();`, nest the `PROVIDE` inside the COMPONENT block:
```cpp
void LoadoutScreen::build_ui() {
    REACT_COMPONENT_BEGIN_KEY("LoadoutScreen", entry_id()) {
        PROVIDE(&LoadoutScreenContext, this) {
            LoadoutScreenView();
        }
    } REACT_COMPONENT_END();
}
```

- [ ] **Step 4: Drop `compare_enabled` from `app::shooter_state_json`**

In `src/app/app.cpp`, remove the line:
```cpp
        << "\"compare_enabled\":" << (game.compare_enabled() ? "true" : "false")
```
and adjust the trailing comma on the previous JSON field so the output stays valid.

- [ ] **Step 5: Audit Python tests for the field**

Run from the repo root:
```
grep -RIn compare_enabled tests tools
```
If any hit asserts the JSON field exists, drop that assertion (the field is no longer reported — it's screen-local).

- [ ] **Step 6: Build + test**

Run: `.\build.ps1 -Tests`
Expected: all 9 tests green. The loadout-screen compare toggle should still work in the UI (the `shooter_ui_tests` `LoadoutScreen` cases exercise it).

- [ ] **Step 7: Commit**

```
git add -A
git commit -m "Lift compare_enabled from ShooterGame to LoadoutScreen (UI state belongs in the UI)"
```

---

## Task 7: Restructure `client/ui/` shell (navigation/, drop template screen)

Move the screen stack and `UiScreen` interface into `client/ui/navigation/`, matching the spec. Delete the empty `screens/screen-name/` template tree.

**Files:**
- Move: `src/client/ui/screen_stack.{h,cpp}` → `src/client/ui/navigation/screen_stack.{h,cpp}`
- Move: `src/client/ui/ui_screen.h` → `src/client/ui/navigation/ui_screen.h`
- Modify: `src/client/ui/client_ui.h` (include path), `src/client/ui/ui_pipeline.h` (transitively fine — pulls via `client_ui.h`)
- Modify: `src/client/ui/screens/screens.h` (`#include "../../ui_screen.h"` → `#include "../../navigation/ui_screen.h"`)
- Modify: `tests/client_ui_tests.cpp`, `tests/ui_pipeline_tests.cpp`, `tests/shooter_ui_tests.cpp` (include paths)
- Delete: `src/client/ui/screens/screen-name/` (whole template tree)
- Modify: `CMakeLists.txt` (every reference to `src/client/ui/screen_stack.cpp`)

(The user-created empty folders `src/client/ui/{components,hooks,providers}/` are intentional steering — leave them.)

- [ ] **Step 1: Move files into `navigation/`**

```
git mv src/client/ui/screen_stack.h   src/client/ui/navigation/screen_stack.h
git mv src/client/ui/screen_stack.cpp src/client/ui/navigation/screen_stack.cpp
git mv src/client/ui/ui_screen.h      src/client/ui/navigation/ui_screen.h
```

- [ ] **Step 2: Fix relative include inside `navigation/screen_stack.h`**

In `src/client/ui/navigation/screen_stack.h`:
- `#include "../../ui/focus/ui_focus.h"` → `#include "../../../ui/focus/ui_focus.h"`
- `#include "ui_screen.h"` (sibling) — unchanged, both moved together.

- [ ] **Step 3: Fix relative include inside `navigation/screen_stack.cpp`**

In `src/client/ui/navigation/screen_stack.cpp`, any `#include "../..."` paths get one more `..`. (Inspect file; only the `screen_stack.h` include is path-local — sibling, unchanged.)

- [ ] **Step 4: Update `src/client/ui/client_ui.h`**

Replace `#include "screen_stack.h"` with:
```cpp
#include "navigation/screen_stack.h"
```

- [ ] **Step 5: Update screens consumer**

In `src/client/ui/screens/screens.h`:
- Change `#include "../../ui_screen.h"` → `#include "../../navigation/ui_screen.h"`.

- [ ] **Step 6: Update test include paths**

`tests/client_ui_tests.cpp`, `tests/ui_pipeline_tests.cpp`: any `#include "client/ui/ui_screen.h"` or `#include "client/ui/screen_stack.h"` → `#include "client/ui/navigation/ui_screen.h"` / `#include "client/ui/navigation/screen_stack.h"`.

`tests/shooter_ui_tests.cpp`: same fix.

- [ ] **Step 7: Delete `src/client/ui/screens/screen-name/`**

```
git rm -r src/client/ui/screens/screen-name
```

- [ ] **Step 8: Update `CMakeLists.txt`**

Search-and-replace `src/client/ui/screen_stack.cpp` → `src/client/ui/navigation/screen_stack.cpp` everywhere (`hello`, `client_ui_tests`, `ui_pipeline_tests`, `shooter_ui_tests`).

- [ ] **Step 9: Build**

Run: `.\build.ps1`
Expected: success. (Build artifacts go to `cmake-build-debug/`.)

- [ ] **Step 10: Run all tests**

Run: `.\build.ps1 -Tests`
Expected: all 9 tests green.

- [ ] **Step 11: Commit**

```
git add -A
git commit -m "Move screen stack + UiScreen into client/ui/navigation/; drop screen-name template"
```

---

## Task 8: Reconcile CLAUDE.md docs and add per-directory CLAUDE.md files

Update existing CLAUDE.md docs so they describe the post-refactor layout, and add 5 new per-directory CLAUDE.md files where real architectural boundaries live (`app/`, `renderer/`, `game/`, `platform/`, `client/ui/screens/loadout/`). Each new CLAUDE.md gets an `AGENTS.md` symlink alongside it, matching the existing convention (see `src/AGENTS.md`, `src/ui/AGENTS.md`, `src/client/ui/AGENTS.md`). `architecture.md` itself is canonical and unchanged.

**Files modified:**
- `CLAUDE.md` (root)
- `src/CLAUDE.md`
- `src/client/ui/CLAUDE.md`
- `src/ui/CLAUDE.md` (only if it references moved files)

**Files created:**
- `src/app/CLAUDE.md` + `src/app/AGENTS.md` (symlink)
- `src/renderer/CLAUDE.md` + `src/renderer/AGENTS.md` (symlink)
- `src/game/CLAUDE.md` + `src/game/AGENTS.md` (symlink)
- `src/platform/CLAUDE.md` + `src/platform/AGENTS.md` (symlink)
- `src/client/ui/screens/loadout/CLAUDE.md` + `src/client/ui/screens/loadout/AGENTS.md` (symlink)

- [ ] **Step 1: Update root `CLAUDE.md`**

Replace the `## Layout` block with:

```text
src/app/...        # process lifecycle, frame loop, composition
src/platform/...   # OS adapters: SDL window/input, control mailbox
src/renderer/...   # SDL/Clay render glue + font registry
src/ui/...         # generic Clay toolkit: hooks runtime, focus, primitives
src/client/ui/...  # client UI shell + game screens (providers, hooks, components, screens/*)
src/game/...       # game rules and state (player, weapons, economy, inventory)
src/react.{h,cpp}  # React-style hook runtime over Clay (see src/react.h header)
third_party/       # vendored Clay + SDL3 renderer, stb_image
tests/             # gtest-free unit tests + python CLI smoke tests
tools/ui_cli.py    # headless control/capture driver used by CLI smoke tests
```

Remove the sentence "the repo today is mid-migration toward that layout; some pieces (e.g. a real `app/` split, `client/commands/`) still live in `src/main.cpp`" — the migration is done.

Also fix the build command examples: this repo builds via `.\build.ps1` on Windows (the wrapper that sets up vcvars + vcpkg for CURL); the documented `cmake -S . -B build` snippet fails because CURL isn't on the default search path. Replace the existing build snippet with:

```sh
.\build.ps1          # configure + build hello (Windows)
.\build.ps1 -Tests   # build everything + run ctest
```

- [ ] **Step 2: Update `src/CLAUDE.md`**

Replace the `## Layering` block with:

```text
app/      process lifecycle and the per-frame loop
ui/       generic Clay toolkit (no game vocabulary)
client/   client UI shell (screen stack, write queue, focus glue) + the game's screens
game/     game rules and state (player, weapons, economy, inventory)
platform/ OS/library adapters (SDL window/input, control mailbox)
renderer/ font + Clay→SDL render glue
react.{h,cpp}  React-style hook runtime layered on Clay
main.cpp  ~15-line entrypoint: builds AppOptions, hands off to app::App
```

Replace the "Allowed dependency direction" line with:
> `client/ui/screens → client/ui → game → ui → react → clay`. `app/` composes everything; `platform/` and `renderer/` are siblings consumed by `app/`. `game/` must not depend on Clay/SDL/UI.

Replace "Where new code goes" with:
- **Game rules / state** → `game/`.
- **A new screen** → `client/ui/screens/<screen_name>/<screen_name>_screen.{h,cpp}` (with `components/` subdir if the screen has its own components).
- **Cross-screen UI hook** → `client/ui/hooks/`.
- **Cross-screen UI component** → `client/ui/components/`.
- **Cross-screen UI provider** → `client/ui/providers/`.
- **Generic widget** (no game vocabulary) → `ui/primitives/`.
- **OS/SDL-specific glue** → `platform/sdl/`.
- **Rendering backend code** → `renderer/`.
- **Per-frame loop / app lifecycle** → `app/`.

Update "Hard rules":
- `ui/` must not know about game concepts.
- `game/` must not include Clay or SDL headers.
- UI must queue mutations during a Clay pass via `client::ui::ClientUi::queue_deferred_write`.
- Game-specific UI screens live under `client/ui/screens/<screen>/`, not in `game/`.

Update the "Tests" section: replace `game_ui_pipeline_tests` with `ui_pipeline_tests`.

- [ ] **Step 3: Update `src/client/ui/CLAUDE.md`**

Rewrite the `## Files` block:

```text
client_ui.{h,cpp}                ClientUi shell: ScreenStack + UiFocusRuntime + deferred-write queue.
ui_pipeline.{h,cpp}              UiPipeline: wraps ClientUi in a Clay frame pass.
navigation/screen_stack.{h,cpp}  Retained screens, overlay flag, build order, entry IDs.
navigation/ui_screen.h           UiScreen interface.
providers/                       Cross-screen React context providers (e.g., shooter_provider).
hooks/                           Cross-screen React hooks (e.g., shooter_hud, shooter_weapons).
components/                      Cross-screen visual components (e.g., hud_band).
screens/<screen>/                Per-screen dir: <screen>_screen.{h,cpp} + optional components/.
```

Replace the old "must stay game-agnostic" line with:
> The shell files (`client_ui`, `ui_pipeline`, `navigation/`) are framework-shaped — they don't know about specific games. Game vocabulary (the `ShooterGame`, `WeaponSpec`, etc.) shows up in `providers/`, `hooks/`, `components/`, and `screens/` because this project has exactly one game (the shooter). If a second game ever appears, promote shared pieces back into framework headers and namespace per-game code accordingly.

Update the per-frame contract section: replace `GameUiPipeline (../../game/ui/game_ui_pipeline.h)` with `UiPipeline (ui_pipeline.h)`.

- [ ] **Step 4: Scan `src/ui/CLAUDE.md` for stale references**

Run:
```
grep -n "game_ui_pipeline\|shooter\|GameUiPipeline\|src/shooter" src/ui/CLAUDE.md
```
If any hit, update it to match the new layout. (Likely none — `src/ui/` is pure generic toolkit.)

- [ ] **Step 5: Create `src/app/CLAUDE.md`**

```markdown
# src/app/

Owns process lifecycle and the per-frame loop. `app::App` constructs SDL, fonts, Clay, the `UiPipeline`, the `ShooterGame`, and the `ControlMailbox`; `app::GameLoop::tick()` is the per-frame body (poll events → build input frame → run UI pipeline → render → present → drain deferred writes).

## Files

- `app.{h,cpp}` — `App::initialize/run/shutdown`. Owns every subsystem; nothing else creates one.
- `game_loop.{h,cpp}` — `GameLoop::tick()`. Stateless except for per-frame transient flags (`previous_pointer_down_`).

## Hard rules

- `app/` is the only directory allowed to know about every subsystem at once. Don't push subsystem-wiring into `client/`, `game/`, or `platform/`.
- No game rules here — only orchestration. Game rules live in `game/`.
- No Clay layout calls here — `UiPipeline` owns the frame body.
- New per-frame stages go in `GameLoop::tick()`, not bolted on elsewhere.
```

- [ ] **Step 6: Create `src/renderer/CLAUDE.md`**

```markdown
# src/renderer/

Drawing backend: SDL3-based Clay command rendering + font measurement. This is *not* the place for game-world rendering yet (we don't have one); when it appears, `world_renderer.{h,cpp}` joins this directory.

## Files

- `font_registry.{h,cpp}` — opens a default font, exposes `measure(...)` for Clay's text-measurement callback. Owns `TTF_TextEngine` lifetime.
- `sdl_clay_renderer.{h,cpp}` — translates `Clay_RenderCommandArray` into SDL draw calls.

## Hard rules

- `renderer/` can depend on SDL3, SDL3_ttf, and Clay. It must not depend on `client/`, `game/`, or `app/`.
- Fonts and render state are owned here. Other modules ask for measurements / rendered output; they don't reach into renderer internals.
```

- [ ] **Step 7: Create `src/game/CLAUDE.md`**

```markdown
# src/game/

Game truth: rules, state, simulation. This is the shooter — health, armor, credits, weapons, buy economy, inventory. The split mirrors HL1/CS-era patterns (player state, inventory, rules) without inventing entity hierarchies we don't need yet.

## Files

- `shooter_game.{h,cpp}` — façade aggregate. Owns `PlayerState` + `Inventory`; delegates queries/mutations.
- `player_state.h` — health, armor, credits. Header-only.
- `weapon_defs.h` — `WeaponSpec` (immutable per-weapon data).
- `weapon_state.h` — `WeaponState` (per-instance: spec + owned + equipped).
- `inventory.{h,cpp}` — `Inventory` (array of weapons, selected index, equip rules).
- `economy.{h,cpp}` — free functions over `PlayerState` + `Inventory`: `can_buy_weapon`, `try_buy_weapon`.

## Hard rules

- **No Clay, SDL, or UI headers.** If you have to include one, the file is in the wrong directory.
- **No per-frame timing or rendering concerns.** Game functions accept the inputs they need and return state changes; `app/GameLoop` decides when to call them.
- UI state (selected tab, compare panel toggle, hover, modal visibility) does NOT live here. That's `client/ui/screens/<screen>/`.
- The façade pattern on `ShooterGame` is for stability of UI call sites. Direct callers (eventually `app/`, server-side simulation) can use the decomposed types directly — `player_state`, `inventory`, `economy`.
```

- [ ] **Step 8: Create `src/platform/CLAUDE.md`**

```markdown
# src/platform/

OS and library adapters. SDL window/renderer/input are isolated here so the rest of the tree doesn't include `<SDL3/SDL.h>`. The control mailbox is the headless test IPC (file-based request/reply) used by `tools/ui_cli.py`.

## Files

- `sdl/window.{h,cpp}` — `SDL_Window` + `SDL_Renderer` lifetime, resize, vsync.
- `sdl/input.{h,cpp}` — SDL keycodes + gamepad buttons → `::ui::UiInputFrame`.
- `control_mailbox.{h,cpp}` — JSON-over-files IPC for the headless CLI tests.

## Hard rules

- SDL types may appear in this directory's headers (they're SDL adapters). Other directories include these adapters but never include SDL headers directly outside `renderer/` and `app/`.
- New OS adapters (audio device, clipboard, etc.) join `platform/sdl/` (or a sibling `platform/<lib>/` if non-SDL).
- `control_mailbox` is test infrastructure, not gameplay infrastructure. Keep it at the parent level so it's discoverable as a sibling of `sdl/`.
```

- [ ] **Step 9: Create `src/client/ui/screens/loadout/CLAUDE.md`**

```markdown
# src/client/ui/screens/loadout/

The loadout screen — the only screen complex enough to need its own components dir + screen-local UI state. Treat this as the reference template for screens that grow beyond a single `<screen>_screen.{h,cpp}` file.

## Files

- `loadout_screen.{h,cpp}` — `LoadoutScreen` class (UI state lives here: `compare_enabled_`), `LoadoutScreenView`, `use_push_loadout_screen`, `use_compare_enabled` / `use_set_compare_enabled` hooks.
- `components/weapon_tile.{h,cpp}` — `WeaponTile` + tab constants (`LOADOUT_TAB_WEAPONS`, `LOADOUT_TAB_GEAR`) + helpers.
- `components/equipment_slot.{h,cpp}` — `EquipmentSlot`.
- `components/confirm_dialog.{h,cpp}` — `LoadoutConfirmDialog` + action constants (`LOADOUT_ACTION_*`).

## Conventions for screen-local state

- Screen-local state (e.g., `compare_enabled_`) is a private member on the screen class, with public getter/setter.
- Expose the screen to descendant components via a `ReactContext` provided in `build_ui()`. Descendant hooks call `use_current_loadout_screen()` to reach the state.
- Mutations from inside Clay layout MUST go through `client::ui::use_ui_write_queue()` — never write directly.

## When to graduate a component to the parent dir

- A component used by another screen → move to `client/ui/components/`.
- A hook used by another screen → move to `client/ui/hooks/`.
- A provider/context used by another screen → move to `client/ui/providers/`.

Co-location wins by default; promote only when there's a real second consumer.
```

- [ ] **Step 10: Create AGENTS.md symlinks**

The repo convention is one `AGENTS.md` symlink per `CLAUDE.md` (verify by running `ls -la src/AGENTS.md src/ui/AGENTS.md src/client/ui/AGENTS.md`). Create the matching symlinks for the 5 new CLAUDE.md files.

On PowerShell:
```powershell
New-Item -ItemType SymbolicLink -Path src\app\AGENTS.md                          -Target CLAUDE.md
New-Item -ItemType SymbolicLink -Path src\renderer\AGENTS.md                     -Target CLAUDE.md
New-Item -ItemType SymbolicLink -Path src\game\AGENTS.md                         -Target CLAUDE.md
New-Item -ItemType SymbolicLink -Path src\platform\AGENTS.md                     -Target CLAUDE.md
New-Item -ItemType SymbolicLink -Path src\client\ui\screens\loadout\AGENTS.md    -Target CLAUDE.md
```

Verify each is a symlink, not a regular file copy:
```
ls -la src/app/AGENTS.md src/renderer/AGENTS.md src/game/AGENTS.md src/platform/AGENTS.md src/client/ui/screens/loadout/AGENTS.md
```

- [ ] **Step 11: Build + test (sanity check; no code changed)**

Run: `.\build.ps1 -Tests`
Expected: all 9 tests green.

- [ ] **Step 12: Commit**

```
git add CLAUDE.md src/CLAUDE.md src/client/ui/CLAUDE.md src/ui/CLAUDE.md src/app src/renderer src/game src/platform src/client/ui/screens/loadout
git commit -m "Reconcile CLAUDE.md docs + add per-directory CLAUDE.md/AGENTS.md for app, renderer, game, platform, loadout"
```

---

## Self-review notes

- **Spec coverage:** every bullet in the agreed plan (`app/`, `renderer/`, `platform/sdl/`, pipeline relocation, shooter decomposition, UI screen decomposition, `client/ui/` restructure, docs + per-dir CLAUDE.md) maps to a task above.
- **Type consistency:** `UiPipeline`, `UiPipelineFrame`, `use_ui_pipeline_frame` used uniformly from Task 4 onward. `client::ui::ClientUi` (existing) accessed via `.client_ui()` on the pipeline.
- **Migration order:** renderer → platform → app → pipeline → game decomp → UI decomp (6a/6b/6c/6d) → client/ui shell → docs. Each task ends with `.\build.ps1 -Tests` green and one commit.
