#include "client/ui/client_ui.h"
#include "react.h"
#include "game/shooter_game.h"
#include "client/ui/screens/in_game/in_game_screen.h"
#include "client/ui/screens/loadout/loadout_screen.h"
#include "client/ui/screens/main_menu/main_menu_screen.h"
#include "client/ui/screens/options/options_screen.h"
#include "client/ui/screens/pause/pause_screen.h"

#include <clay.h>

#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr)                                                              \
    do {                                                                         \
        if (!(expr)) {                                                           \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__,  \
                    #expr);                                                      \
            return false;                                                        \
        }                                                                        \
    } while (0)

static Clay_Context *g_clay = nullptr;
static void *g_clay_memory = nullptr;

static void on_clay_error(Clay_ErrorData error) {
    fprintf(stderr, "clay: %.*s\n", (int)error.errorText.length, error.errorText.chars);
}

static Clay_Dimensions measure_text(Clay_StringSlice text,
                                    Clay_TextElementConfig *,
                                    void *) {
    return Clay_Dimensions{ (float)text.length * 8.0f, 16.0f };
}

static Clay_ElementId test_id(const char *name) {
    return Clay_GetElementId(Clay_String{ false, (int32_t)strlen(name), name });
}

static bool same_id(Clay_ElementId a, Clay_ElementId b) {
    return a.id != 0 && a.id == b.id;
}

static bool init_clay_once(void) {
    if (g_clay) return true;

    uint32_t clay_memory_size = Clay_MinMemorySize();
    g_clay_memory = malloc(clay_memory_size);
    CHECK(g_clay_memory != nullptr);

    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(clay_memory_size, g_clay_memory);
    g_clay = Clay_Initialize(arena, Clay_Dimensions{ 800, 500 },
                             Clay_ErrorHandler{ on_clay_error, nullptr });
    CHECK(g_clay != nullptr);
    Clay_SetMeasureTextFunction(measure_text, nullptr);
    return true;
}

static void run_client_frame(client::ui::ClientUi &client_ui,
                             const ::ui::UiInputFrame &input = {},
                             Clay_Vector2 pointer = { -1000.0f, -1000.0f },
                             bool drain_writes = true) {
    Clay_SetLayoutDimensions({ 800, 500 });
    Clay_SetPointerState(pointer, input.pointer_down);
    client_ui.begin_frame(input);
    react_begin_frame();
    Clay_BeginLayout();
    CLAY({
        .id = Clay_GetElementId(CLAY_STRING("ShooterTestRoot")),
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
        },
    }) {
        client_ui.build_visible_screens();
    }
    (void)Clay_EndLayout();
    client_ui.end_layout(input);
    react_end_frame();
    if (drain_writes) {
        client_ui.drain_writes();
    }
}

static Clay_Vector2 center_of(Clay_ElementId id) {
    Clay_ElementData data = Clay_GetElementData(id);
    if (!data.found) return { -1000.0f, -1000.0f };
    return {
        data.boundingBox.x + data.boundingBox.width * 0.5f,
        data.boundingBox.y + data.boundingBox.height * 0.5f,
    };
}

static ::ui::UiInputFrame keyboard_confirm(void) {
    return {
        .confirm_pressed = true,
        .confirm_down = true,
        .source = ::ui::UiFocusSource::Keyboard,
    };
}

static ::ui::UiInputFrame keyboard_down(void) {
    return {
        .nav_down = true,
        .source = ::ui::UiFocusSource::Keyboard,
    };
}

static ::ui::UiInputFrame keyboard_right(void) {
    return {
        .nav_right = true,
        .source = ::ui::UiFocusSource::Keyboard,
    };
}

static ::ui::UiInputFrame keyboard_cancel(void) {
    return {
        .cancel_pressed = true,
        .cancel_down = true,
        .source = ::ui::UiFocusSource::Keyboard,
    };
}

static ::ui::UiInputFrame pointer_press(void) {
    return {
        .pointer_pressed = true,
        .pointer_down = true,
        .source = ::ui::UiFocusSource::Mouse,
    };
}

static ::ui::UiInputFrame pointer_release(void) {
    return {
        .pointer_released = true,
        .source = ::ui::UiFocusSource::Mouse,
    };
}

static bool shooter_game_buy_and_equip_are_real_state_writes(void) {
    shooter::ShooterGame game;

    CHECK(game.credits() == 450);
    CHECK(!game.weapon(1).owned);
    CHECK(game.buy_weapon(1));
    CHECK(game.credits() == 150);
    CHECK(game.weapon(1).owned);
    CHECK(game.equip_weapon(1));
    CHECK(game.selected_weapon() == 1);
    CHECK(game.weapon(1).equipped);
    CHECK(!game.can_buy_weapon(2));
    return true;
}

static bool main_menu_start_match_resets_game_and_stack(void) {
    react_init(g_clay);
    shooter::ShooterGame game;
    CHECK(game.buy_weapon(1));
    CHECK(game.equip_weapon(1));
    CHECK(game.credits() == 150);
    CHECK(game.selected_weapon() == 1);

    bool quit_requested = false;
    client::ui::ClientUi client_ui;
    CHECK(client_ui.push_screen(std::make_unique<shooter::MainMenuScreen>(&game, [&quit_requested] {
        quit_requested = true;
    })));

    run_client_frame(client_ui);
    CHECK(client_ui.screens().count() == 1);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "MainMenu") == 0);

    run_client_frame(client_ui, keyboard_confirm());

    CHECK(client_ui.screens().count() == 1);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "ShooterGame") == 0);
    CHECK(game.credits() == 450);
    CHECK(game.selected_weapon() == 0);
    CHECK(game.weapon(0).equipped);
    CHECK(!game.weapon(1).owned);
    CHECK(!quit_requested);
    return true;
}

static bool main_menu_options_returns_to_menu_with_cancel(void) {
    react_init(g_clay);
    shooter::ShooterGame game;
    client::ui::ClientUi client_ui;
    CHECK(client_ui.push_screen(std::make_unique<shooter::MainMenuScreen>(&game)));

    run_client_frame(client_ui);
    run_client_frame(client_ui, keyboard_down());
    run_client_frame(client_ui, keyboard_confirm());

    CHECK(client_ui.screens().count() == 2);
    CHECK(strcmp(client_ui.screens().at(0)->debug_name(), "MainMenu") == 0);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "Options") == 0);

    run_client_frame(client_ui, keyboard_cancel());

    CHECK(client_ui.screens().count() == 1);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "MainMenu") == 0);
    return true;
}

static bool main_menu_quit_callback_runs(void) {
    react_init(g_clay);
    shooter::ShooterGame game;
    bool quit_requested = false;
    client::ui::ClientUi client_ui;
    CHECK(client_ui.push_screen(std::make_unique<shooter::MainMenuScreen>(&game, [&quit_requested] {
        quit_requested = true;
    })));

    run_client_frame(client_ui);
    run_client_frame(client_ui, keyboard_down());
    run_client_frame(client_ui, keyboard_down());
    run_client_frame(client_ui, keyboard_confirm());

    CHECK(quit_requested);
    CHECK(client_ui.screens().count() == 1);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "MainMenu") == 0);
    return true;
}

static bool shooter_screen_pushes_pause_after_confirm(void) {
    react_init(g_clay);
    shooter::ShooterGame game;
    client::ui::ClientUi client_ui;
    CHECK(client_ui.push_screen(std::make_unique<shooter::ShooterGameScreen>(&game)));

    run_client_frame(client_ui);
    CHECK(client_ui.screens().count() == 1);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "ShooterGame") == 0);

    run_client_frame(client_ui, keyboard_confirm());

    CHECK(client_ui.screens().count() == 2);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "Pause") == 0);
    return true;
}

static bool pause_exit_to_main_menu_resets_stack(void) {
    react_init(g_clay);
    shooter::ShooterGame game;
    client::ui::ClientUi client_ui;
    CHECK(client_ui.push_screen(std::make_unique<shooter::ShooterGameScreen>(&game)));

    run_client_frame(client_ui);
    run_client_frame(client_ui, keyboard_confirm());
    CHECK(client_ui.screens().count() == 2);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "Pause") == 0);

    run_client_frame(client_ui);
    run_client_frame(client_ui, keyboard_down());
    run_client_frame(client_ui, keyboard_down());
    run_client_frame(client_ui, keyboard_down());
    run_client_frame(client_ui, keyboard_confirm());

    CHECK(client_ui.screens().count() == 1);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "MainMenu") == 0);
    return true;
}

static bool pause_options_returns_to_pause_through_screen_stack(void) {
    react_init(g_clay);
    shooter::ShooterGame game;
    client::ui::ClientUi client_ui;
    CHECK(client_ui.push_screen(std::make_unique<shooter::ShooterGameScreen>(&game)));

    run_client_frame(client_ui);
    run_client_frame(client_ui, keyboard_confirm());
    CHECK(client_ui.screens().count() == 2);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "Pause") == 0);

    run_client_frame(client_ui);
    run_client_frame(client_ui, keyboard_down());
    run_client_frame(client_ui, keyboard_confirm());
    CHECK(client_ui.screens().count() == 3);
    CHECK(strcmp(client_ui.screens().at(1)->debug_name(), "Pause") == 0);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "Options") == 0);

    run_client_frame(client_ui);
    run_client_frame(client_ui, keyboard_down());
    run_client_frame(client_ui, keyboard_down());
    run_client_frame(client_ui, keyboard_confirm());
    CHECK(client_ui.screens().count() == 2);
    CHECK(strcmp(client_ui.screens().top()->debug_name(), "Pause") == 0);
    return true;
}

static bool loadout_buy_uses_confirm_dialog_and_restores_parent_focus(void) {
    react_init(g_clay);
    shooter::ShooterGame game;
    client::ui::ClientUi client_ui;
    CHECK(client_ui.push_screen(std::make_unique<shooter::LoadoutScreen>(&game)));

    run_client_frame(client_ui);
    CHECK(same_id(
        ::ui::ui_focus_focused_id_for_scope(test_id("LoadoutScope")),
        CLAY_IDI("WeaponTile", 0)));
    CHECK(game.selected_weapon() == 0);

    run_client_frame(client_ui, keyboard_right());
    CHECK(same_id(
        ::ui::ui_focus_focused_id_for_scope(test_id("LoadoutScope")),
        CLAY_IDI("WeaponTile", 1)));
    CHECK(game.selected_weapon() == 1);
    CHECK(!game.weapon(1).owned);
    CHECK(game.credits() == 450);

    run_client_frame(client_ui, keyboard_right());
    run_client_frame(client_ui, keyboard_down());
    CHECK(same_id(
        ::ui::ui_focus_focused_id_for_scope(test_id("LoadoutScope")),
        test_id("BuyWeaponButton")));

    run_client_frame(client_ui, keyboard_confirm());
    CHECK(!game.weapon(1).owned);
    CHECK(game.credits() == 450);

    run_client_frame(client_ui);
    CHECK(same_id(
        ::ui::ui_focus_focused_id_for_scope(CLAY_IDI("LoadoutConfirmScope", 1)),
        test_id("ConfirmLoadoutActionButton")));
    CHECK(same_id(
        ::ui::ui_focus_focused_id_for_scope(test_id("LoadoutScope")),
        test_id("BuyWeaponButton")));

    run_client_frame(client_ui, keyboard_right());
    CHECK(same_id(
        ::ui::ui_focus_focused_id_for_scope(CLAY_IDI("LoadoutConfirmScope", 1)),
        test_id("CancelLoadoutActionButton")));
    run_client_frame(client_ui, keyboard_confirm());
    CHECK(!game.weapon(1).owned);
    CHECK(game.credits() == 450);

    run_client_frame(client_ui);
    CHECK(same_id(
        ::ui::ui_focus_focused_id_for_scope(test_id("LoadoutScope")),
        test_id("BuyWeaponButton")));

    run_client_frame(client_ui, keyboard_confirm());
    CHECK(!game.weapon(1).owned);
    run_client_frame(client_ui);
    CHECK(same_id(
        ::ui::ui_focus_focused_id_for_scope(CLAY_IDI("LoadoutConfirmScope", 2)),
        test_id("ConfirmLoadoutActionButton")));
    run_client_frame(client_ui, keyboard_confirm());
    CHECK(game.weapon(1).owned);
    CHECK(game.credits() == 150);
    return true;
}

static bool loadout_tabs_and_equipment_slots_are_real_focus_targets(void) {
    react_init(g_clay);
    shooter::ShooterGame game;
    client::ui::ClientUi client_ui;
    CHECK(client_ui.push_screen(std::make_unique<shooter::LoadoutScreen>(&game)));

    run_client_frame(client_ui);
    CHECK(Clay_GetElementData(test_id("WeaponsTab")).found);
    CHECK(Clay_GetElementData(test_id("GearTab")).found);
    CHECK(Clay_GetElementData(test_id("PrimarySlot")).found);
    CHECK(Clay_GetElementData(test_id("GearSlot")).found);

    Clay_Vector2 gear_tab = center_of(test_id("GearTab"));
    run_client_frame(client_ui, pointer_press(), gear_tab);
    run_client_frame(client_ui, pointer_release(), gear_tab);
    CHECK(game.selected_weapon() == 3);

    run_client_frame(client_ui);
    CHECK(Clay_GetElementData(CLAY_IDI("WeaponTile", 3)).found);

    Clay_Vector2 primary_slot = center_of(test_id("PrimarySlot"));
    run_client_frame(client_ui, pointer_press(), primary_slot);
    run_client_frame(client_ui, pointer_release(), primary_slot);
    CHECK(game.selected_weapon() == 0);

    Clay_Vector2 gear_slot = center_of(test_id("GearSlot"));
    run_client_frame(client_ui, pointer_press(), gear_slot);
    run_client_frame(client_ui, pointer_release(), gear_slot);
    CHECK(game.selected_weapon() == 3);
    return true;
}

static bool shooter_game_writes_wait_for_client_ui_drain(void) {
    react_init(g_clay);
    shooter::ShooterGame game;
    client::ui::ClientUi client_ui;
    CHECK(client_ui.push_screen(std::make_unique<shooter::LoadoutScreen>(&game)));

    run_client_frame(client_ui);
    Clay_Vector2 gear_tab = center_of(test_id("GearTab"));
    run_client_frame(client_ui, pointer_press(), gear_tab);
    run_client_frame(client_ui, pointer_release(), gear_tab, false);

    CHECK(game.selected_weapon() == 0);
    CHECK(client_ui.pending_write_count() == 1);
    client_ui.drain_writes();
    CHECK(game.selected_weapon() == 3);
    return true;
}

int main(void) {
    if (!init_clay_once()) return 1;

    if (!shooter_game_buy_and_equip_are_real_state_writes()) return 1;
    if (!main_menu_start_match_resets_game_and_stack()) return 1;
    if (!main_menu_options_returns_to_menu_with_cancel()) return 1;
    if (!main_menu_quit_callback_runs()) return 1;
    if (!shooter_screen_pushes_pause_after_confirm()) return 1;
    if (!pause_exit_to_main_menu_resets_stack()) return 1;
    if (!pause_options_returns_to_pause_through_screen_stack()) return 1;
    if (!loadout_buy_uses_confirm_dialog_and_restores_parent_focus()) return 1;
    if (!loadout_tabs_and_equipment_slots_are_real_focus_targets()) return 1;
    if (!shooter_game_writes_wait_for_client_ui_drain()) return 1;

    react_shutdown();
    free(g_clay_memory);
    return 0;
}
