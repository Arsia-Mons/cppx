#include "client/ui/client_ui.h"
#include "react.h"
#include "shooter/shooter_game.h"
#include "shooter/shooter_ui.h"

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
                             const ::ui::UiInputFrame &input = {}) {
    Clay_SetLayoutDimensions({ 800, 500 });
    Clay_SetPointerState({ -1000.0f, -1000.0f }, input.pointer_down);
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
    client_ui.drain_writes();
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

int main(void) {
    if (!init_clay_once()) return 1;

    if (!shooter_game_buy_and_equip_are_real_state_writes()) return 1;
    if (!shooter_screen_pushes_pause_after_confirm()) return 1;
    if (!pause_options_returns_to_pause_through_screen_stack()) return 1;

    react_shutdown();
    free(g_clay_memory);
    return 0;
}
