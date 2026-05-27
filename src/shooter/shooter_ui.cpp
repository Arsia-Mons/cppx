#include "shooter_ui.h"

#include <functional>
#include <memory>
#include <stdio.h>
#include <stdint.h>

#include <clay.h>

#include "../client/ui/client_ui.h"
#include "../react.h"
#include "../ui/focus/ui_focus.h"
#include "../ui/primitives/button.h"
#include "../ui/primitives/clay_text.h"
#include "../ui/primitives/focusable.h"
#include "../ui/primitives/selectable.h"
#include "../ui/primitives/toggle.h"
#include "../ui/primitives/visual_state.h"

namespace shooter {

struct ShooterContext {
    ShooterGame *game = nullptr;
};

struct ShooterHudRead {
    int health = 0;
    int armor = 0;
    int ammo = 0;
    int credits = 0;
    const char *weapon = "";
};

struct ShooterWeaponRead {
    bool valid = false;
    const char *name = "";
    const char *role = "";
    int cost = 0;
    int damage = 0;
    int ammo = 0;
    bool owned = false;
    bool equipped = false;
    bool can_buy = false;
    bool can_equip = false;
    bool disabled = true;
};

constexpr int LOADOUT_ACTION_NONE = 0;
constexpr int LOADOUT_ACTION_BUY = 1;
constexpr int LOADOUT_ACTION_EQUIP = 2;
constexpr int LOADOUT_TAB_WEAPONS = 0;
constexpr int LOADOUT_TAB_GEAR = 1;

static ReactContext ShooterContextValue = {};

static ShooterGame *use_shooter_game(void) {
    ShooterContext *context =
        static_cast<ShooterContext *>(use_context(&ShooterContextValue));
    return context ? context->game : nullptr;
}

static ShooterHudRead use_shooter_hud(void) {
    ShooterGame *game = use_shooter_game();
    if (!game) return {};
    return {
        .health = game->health(),
        .armor = game->armor(),
        .ammo = game->ammo(),
        .credits = game->credits(),
        .weapon = game->weapon(game->selected_weapon()).spec.name,
    };
}

static int use_shooter_weapon_count(void) {
    ShooterGame *game = use_shooter_game();
    return game ? game->weapon_count() : 0;
}

static int use_selected_weapon_index(void) {
    ShooterGame *game = use_shooter_game();
    return game ? game->selected_weapon() : 0;
}

static ShooterWeaponRead use_weapon_read(int index) {
    ShooterGame *game = use_shooter_game();
    if (!game || index < 0 || index >= game->weapon_count()) return {};
    const WeaponState &weapon = game->weapon(index);
    bool can_buy = game->can_buy_weapon(index);
    bool can_equip = game->can_equip_weapon(index);
    return {
        .valid = true,
        .name = weapon.spec.name,
        .role = weapon.spec.role,
        .cost = weapon.spec.cost,
        .damage = weapon.spec.damage,
        .ammo = weapon.spec.ammo,
        .owned = weapon.owned,
        .equipped = weapon.equipped,
        .can_buy = can_buy,
        .can_equip = can_equip,
        .disabled = !weapon.owned && !can_buy,
    };
}

static std::function<void()> use_select_weapon(int index) {
    ShooterGame *game = use_shooter_game();
    client::ui::QueueUiWrite queue_write = client::ui::use_ui_write_queue();
    return [game, index, queue_write] {
        if (game && queue_write) {
            queue_write([game, index] {
                game->select_weapon(index);
            });
        }
    };
}

static std::function<void()> use_buy_weapon(int index) {
    ShooterGame *game = use_shooter_game();
    client::ui::QueueUiWrite queue_write = client::ui::use_ui_write_queue();
    return [game, index, queue_write] {
        if (game && queue_write) {
            queue_write([game, index] {
                game->buy_weapon(index);
            });
        }
    };
}

static std::function<void()> use_equip_weapon(int index) {
    ShooterGame *game = use_shooter_game();
    client::ui::QueueUiWrite queue_write = client::ui::use_ui_write_queue();
    return [game, index, queue_write] {
        if (game && queue_write) {
            queue_write([game, index] {
                game->equip_weapon(index);
            });
        }
    };
}

static std::function<void()> use_push_pause_screen(void) {
    ShooterGame *game = use_shooter_game();
    client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
    return [game, nav] {
        if (nav.push && game) nav.push(std::make_unique<PauseScreen>(game));
    };
}

static std::function<void()> use_push_loadout_screen(void) {
    ShooterGame *game = use_shooter_game();
    client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
    return [game, nav] {
        if (nav.push && game) nav.push(std::make_unique<LoadoutScreen>(game));
    };
}

static std::function<void()> use_push_options_screen(void) {
    ShooterGame *game = use_shooter_game();
    client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
    return [game, nav] {
        if (nav.push && game) nav.push(std::make_unique<OptionsScreen>(game));
    };
}

static bool use_compare_enabled(void) {
    ShooterGame *game = use_shooter_game();
    return game ? game->compare_enabled() : false;
}

static std::function<void(bool)> use_set_compare_enabled(void) {
    ShooterGame *game = use_shooter_game();
    client::ui::QueueUiWrite queue_write = client::ui::use_ui_write_queue();
    return [game, queue_write](bool enabled) {
        if (game && queue_write) {
            queue_write([game, enabled] {
                game->set_compare_enabled(enabled);
            });
        }
    };
}

static void ShooterProvider(ShooterGame *game, const std::function<void()> &children) {
    ShooterContext context = { .game = game };
    REACT_PROVIDER_ENTER_KEY("ShooterProvider", reinterpret_cast<uintptr_t>(game));
    PROVIDE(&ShooterContextValue, &context) {
        children();
    }
    REACT_PROVIDER_EXIT();
}

static void HudBand(void) {
    REACT_FRAGMENT_COMPONENT_BEGIN("HudBand") {
        ShooterHudRead hud = use_shooter_hud();
        static char health[32];
        static char armor[32];
        static char ammo[32];
        static char credits[40];
        snprintf(health, sizeof(health), "HP %d", hud.health);
        snprintf(armor, sizeof(armor), "ARMOR %d", hud.armor);
        snprintf(ammo, sizeof(ammo), "AMMO %d", hud.ammo);
        snprintf(credits, sizeof(credits), "CREDITS %d", hud.credits);
        CLAY({
            .id = CLAY_ID("ShooterHudBand"),
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(44) },
                .padding = { 16, 16, 10, 10 },
                .childGap = 18,
                .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
            .backgroundColor = { 18, 24, 28, 245 },
            .border = {
                .color = { 82, 106, 118, 255 },
                .width = { .bottom = 1 },
            },
        }) {
            CLAY_TEXT(::ui::clay_text(health),
                CLAY_TEXT_CONFIG({ .textColor = { 224, 238, 236, 255 }, .fontSize = 18 }));
            CLAY_TEXT(::ui::clay_text(armor),
                CLAY_TEXT_CONFIG({ .textColor = { 224, 238, 236, 255 }, .fontSize = 18 }));
            CLAY_TEXT(::ui::clay_text(ammo),
                CLAY_TEXT_CONFIG({ .textColor = { 224, 238, 236, 255 }, .fontSize = 18 }));
            CLAY_TEXT(::ui::clay_text(credits),
                CLAY_TEXT_CONFIG({ .textColor = { 224, 238, 236, 255 }, .fontSize = 18 }));
            CLAY_TEXT(::ui::clay_text(hud.weapon),
                CLAY_TEXT_CONFIG({ .textColor = { 224, 238, 236, 255 }, .fontSize = 18 }));
        }
    } REACT_FRAGMENT_COMPONENT_END();
}

static void ShooterGameScreenView(void) {
    REACT_COMPONENT_BEGIN("ShooterGameScreenView") {
        std::function<void()> open_pause = use_push_pause_screen();
        std::function<void()> open_loadout = use_push_loadout_screen();
        ::ui::ui_focus_push_scope({ .id = CLAY_ID("ShooterGameScope") });
        ::ui::ui_focus_request_initial_focus(CLAY_ID("OpenPauseButton"));

        CLAY({
            .id = CLAY_ID("ShooterGameRoot"),
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(24),
                .childGap = 18,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = { 10, 16, 18, 255 },
        }) {
            HudBand();
            CLAY({
                .id = CLAY_ID("ShooterGameActions"),
                .layout = {
                    .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                    .childGap = 12,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
            }) {
                ::ui::Button({
                    .id = CLAY_ID("OpenPauseButton"),
                    .label = "Pause",
                    .on_confirm = open_pause,
                });
                ::ui::Button({
                    .id = CLAY_ID("OpenLoadoutButton"),
                    .label = "Loadout",
                    .on_confirm = open_loadout,
                });
            }
        }

        ::ui::ui_focus_pop_scope();
    } REACT_COMPONENT_END();
}

static void PauseScreenView(void) {
    REACT_COMPONENT_BEGIN("PauseScreenView") {
        client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
        std::function<void()> open_options = use_push_options_screen();
        std::function<void()> open_loadout = use_push_loadout_screen();
        ::ui::ui_focus_push_scope({ .id = CLAY_ID("PauseScope"), .modal = true });
        ::ui::ui_focus_request_initial_focus(CLAY_ID("ResumeButton"));

        CLAY({
            .id = CLAY_ID("PauseRoot"),
            .layout = {
                .sizing = { CLAY_SIZING_FIXED(220), CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(18),
                .childGap = 12,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = { 17, 24, 30, 255 },
            .cornerRadius = CLAY_CORNER_RADIUS(4),
            .border = {
                .color = { 78, 96, 108, 255 },
                .width = CLAY_BORDER_OUTSIDE(1),
            },
        }) {
            CLAY_TEXT(::ui::clay_text("Paused"),
                CLAY_TEXT_CONFIG({ .textColor = { 236, 246, 242, 255 }, .fontSize = 28 }));
            ::ui::Button({
                .id = CLAY_ID("ResumeButton"),
                .label = "Resume",
                .on_confirm = nav.pop_current,
            });
            ::ui::Button({
                .id = CLAY_ID("OpenOptionsFromPauseButton"),
                .label = "Options",
                .on_confirm = open_options,
            });
            ::ui::Button({
                .id = CLAY_ID("OpenLoadoutFromPauseButton"),
                .label = "Loadout",
                .on_confirm = open_loadout,
            });
        }

        ::ui::ui_focus_pop_scope();
    } REACT_COMPONENT_END();
}

static Clay_ElementId weapon_tile_id(int index) {
    return CLAY_IDI("WeaponTile", index);
}

static bool weapon_in_tab(int index, int tab) {
    if (tab == LOADOUT_TAB_GEAR) return index == 3;
    return index >= 0 && index < 3;
}

static int first_weapon_for_tab(int tab) {
    return tab == LOADOUT_TAB_GEAR ? 3 : 0;
}

static void WeaponTile(int index, int *selected_index) {
    REACT_FRAGMENT_COMPONENT_BEGIN_KEY("WeaponTile", (uint32_t)index) {
        ShooterWeaponRead weapon = use_weapon_read(index);
        if (weapon.valid) {
            bool selected = selected_index && *selected_index == index;
            bool disabled = weapon.disabled;
            std::function<void()> select = use_select_weapon(index);

            static char detail[SHOOTER_WEAPON_COUNT][96];
            snprintf(detail[index], sizeof(detail[index]), "%s  DMG %d  %s",
                     weapon.role,
                     weapon.damage,
                     weapon.owned ? (weapon.equipped ? "equipped" : "owned") :
                         (disabled ? "locked" : "available"));

            ::ui::Focusable({
                .id = weapon_tile_id(index),
                .disabled = disabled,
                .on_confirm = [selected_index, index, select] {
                    if (selected_index) *selected_index = index;
                    if (select) select();
                },
                .on_focus = [selected_index, index, select] {
                    if (selected_index) *selected_index = index;
                    if (select) select();
                },
            }, [&](const ::ui::UiFocusableState &focus) {
                ::ui::VisualState visual = ::ui::derive_visual_state(focus, {
                    .selected = selected,
                    .disabled = disabled,
                });
                uint16_t border_width = visual.targeted ? 2 : 1;
                CLAY({
                    .id = focus.id,
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(190), CLAY_SIZING_FIXED(78) },
                        .padding = CLAY_PADDING_ALL(10),
                        .childGap = 5,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = visual.chosen
                        ? Clay_Color{ 35, 72, 62, 255 }
                        : Clay_Color{ 24, 31, 36, 255 },
                    .cornerRadius = CLAY_CORNER_RADIUS(4),
                    .border = {
                        .color = disabled
                            ? Clay_Color{ 58, 62, 66, 255 }
                            : Clay_Color{ 102, 142, 150, 255 },
                        .width = CLAY_BORDER_OUTSIDE(border_width),
                    },
                }) {
                    CLAY_TEXT(::ui::clay_text(weapon.name),
                        CLAY_TEXT_CONFIG({ .textColor = { 238, 246, 244, 255 }, .fontSize = 16 }));
                    CLAY_TEXT(::ui::clay_text(detail[index]),
                        CLAY_TEXT_CONFIG({
                            .textColor = disabled
                                ? Clay_Color{ 142, 148, 150, 255 }
                                : Clay_Color{ 184, 204, 204, 255 },
                            .fontSize = 12,
                        }));
                }
            });
        }
    } REACT_FRAGMENT_COMPONENT_END();
}

static void EquipmentSlot(Clay_ElementId id,
                          const char *label,
                          int weapon_index,
                          int *selected_index) {
    REACT_FRAGMENT_COMPONENT_BEGIN_KEY("EquipmentSlot", id.id) {
        ShooterWeaponRead weapon = use_weapon_read(weapon_index);
        if (weapon.valid) {
            bool selected = selected_index && *selected_index == weapon_index;
            std::function<void()> select = use_select_weapon(weapon_index);

            static char slot_text[2][96];
            int slot = weapon_index == 3 ? 1 : 0;
            snprintf(slot_text[slot], sizeof(slot_text[slot]), "%s: %s",
                     label,
                     weapon.owned ? weapon.name : "empty");

            ::ui::Focusable({
                .id = id,
                .on_confirm = [selected_index, weapon_index, select] {
                    if (selected_index) *selected_index = weapon_index;
                    if (select) select();
                },
                .on_focus = [selected_index, weapon_index, select] {
                    if (selected_index) *selected_index = weapon_index;
                    if (select) select();
                },
            }, [&](const ::ui::UiFocusableState &focus) {
                ::ui::VisualState visual = ::ui::derive_visual_state(focus, {
                    .selected = selected,
                });
                uint16_t border_width = visual.targeted ? 2 : 1;
                CLAY({
                    .id = focus.id,
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(232), CLAY_SIZING_FIXED(38) },
                        .padding = { 10, 10, 8, 8 },
                        .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                    },
                    .backgroundColor = visual.chosen
                        ? Clay_Color{ 35, 72, 62, 255 }
                        : Clay_Color{ 24, 28, 36, 255 },
                    .cornerRadius = CLAY_CORNER_RADIUS(4),
                    .border = {
                        .color = visual.targeted
                            ? Clay_Color{ 122, 176, 238, 255 }
                            : Clay_Color{ 78, 88, 104, 255 },
                        .width = CLAY_BORDER_OUTSIDE(border_width),
                    },
                }) {
                    CLAY_TEXT(::ui::clay_text(slot_text[slot]),
                        CLAY_TEXT_CONFIG({ .textColor = { 226, 238, 236, 255 }, .fontSize = 14 }));
                }
            });
        }
    } REACT_FRAGMENT_COMPONENT_END();
}

static void LoadoutConfirmDialog(int action,
                                 int weapon_index,
                                 int serial,
                                 int *pending_action) {
    REACT_COMPONENT_BEGIN_KEY("LoadoutConfirmDialog", (uint32_t)((serial << 8) | action)) {
        ShooterWeaponRead weapon = use_weapon_read(weapon_index);
        std::function<void()> buy = use_buy_weapon(weapon_index);
        std::function<void()> equip = use_equip_weapon(weapon_index);
        bool valid = weapon.valid && action != LOADOUT_ACTION_NONE;
        if (valid) {
            static char title[64];
            static char message[128];
            if (action == LOADOUT_ACTION_BUY) {
                snprintf(title, sizeof(title), "Confirm Buy");
                snprintf(message, sizeof(message), "Buy %s for %d credits?",
                         weapon.name, weapon.cost);
            } else {
                snprintf(title, sizeof(title), "Confirm Equip");
                snprintf(message, sizeof(message), "Equip %s as active weapon?",
                         weapon.name);
            }

            auto close = [pending_action] {
                if (pending_action) *pending_action = LOADOUT_ACTION_NONE;
            };

            ::ui::ui_focus_push_scope({
                .id = CLAY_IDI("LoadoutConfirmScope", serial),
                .modal = true,
                .wrap = true,
            });
            ::ui::ui_focus_request_initial_focus(CLAY_ID("ConfirmLoadoutActionButton"));

            CLAY({
                .id = CLAY_ID("LoadoutConfirmScrim"),
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = { 0, 0, 0, 160 },
                .floating = {
                    .zIndex = 300,
                    .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_CAPTURE,
                    .attachTo = CLAY_ATTACH_TO_ROOT,
                },
            }) {
                CLAY({
                    .id = CLAY_ID("LoadoutConfirmPanel"),
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(360), CLAY_SIZING_FIT(0) },
                        .padding = CLAY_PADDING_ALL(18),
                        .childGap = 12,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = { 18, 26, 32, 255 },
                    .cornerRadius = CLAY_CORNER_RADIUS(4),
                    .border = {
                        .color = { 92, 116, 126, 255 },
                        .width = CLAY_BORDER_OUTSIDE(1),
                    },
                }) {
                    CLAY_TEXT(::ui::clay_text(title),
                        CLAY_TEXT_CONFIG({ .textColor = { 240, 248, 244, 255 }, .fontSize = 22 }));
                    CLAY_TEXT(::ui::clay_text(message),
                        CLAY_TEXT_CONFIG({ .textColor = { 202, 218, 216, 255 }, .fontSize = 15 }));
                    CLAY({
                        .id = CLAY_ID("LoadoutConfirmActions"),
                        .layout = {
                            .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                            .childGap = 10,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    }) {
                        ::ui::Button({
                            .id = CLAY_ID("ConfirmLoadoutActionButton"),
                            .label = "Confirm",
                            .on_confirm = [action, buy, equip, close] {
                                if (action == LOADOUT_ACTION_BUY) {
                                    if (buy) buy();
                                } else if (action == LOADOUT_ACTION_EQUIP) {
                                    if (equip) equip();
                                }
                                close();
                            },
                        });
                        ::ui::Button({
                            .id = CLAY_ID("CancelLoadoutActionButton"),
                            .label = "Cancel",
                            .on_confirm = close,
                        });
                    }
                }
            }

            ::ui::ui_focus_pop_scope();
        }
    } REACT_COMPONENT_END();
}

static void LoadoutScreenView(void) {
    REACT_COMPONENT_BEGIN("LoadoutScreenView") {
        client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
        int weapon_count = use_shooter_weapon_count();
        int *selected_index = use_state_int(use_selected_weapon_index());
        int *pending_action = use_state_int(LOADOUT_ACTION_NONE);
        int *pending_weapon_index = use_state_int(*selected_index);
        int *pending_serial = use_state_int(0);
        int *active_tab = use_state_int(LOADOUT_TAB_WEAPONS);
        if (*selected_index < 0 || *selected_index >= weapon_count) {
            *selected_index = 0;
        }
        if (!weapon_in_tab(*selected_index, *active_tab)) {
            *selected_index = first_weapon_for_tab(*active_tab);
        }
        ShooterWeaponRead selected = use_weapon_read(*selected_index);
        if (!selected.valid) return;
        bool can_buy = selected.can_buy;
        bool can_equip = selected.can_equip && !selected.equipped;
        std::function<void()> select_weapons_tab_weapon =
            use_select_weapon(first_weapon_for_tab(LOADOUT_TAB_WEAPONS));
        std::function<void()> select_gear_tab_weapon =
            use_select_weapon(first_weapon_for_tab(LOADOUT_TAB_GEAR));

        static char details[160];
        snprintf(details, sizeof(details), "%s: %s, cost %d, ammo %d",
                 selected.name, selected.role, selected.cost, selected.ammo);

        ::ui::ui_focus_push_scope({ .id = CLAY_ID("LoadoutScope"), .modal = true });
        ::ui::ui_focus_request_initial_focus(weapon_tile_id(*selected_index));

        CLAY({
            .id = CLAY_ID("LoadoutRoot"),
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(24),
                .childGap = 18,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = { 12, 20, 24, 245 },
        }) {
            CLAY_TEXT(::ui::clay_text("Loadout"),
                CLAY_TEXT_CONFIG({ .textColor = { 236, 246, 242, 255 }, .fontSize = 26 }));
            CLAY({
                .id = CLAY_ID("LoadoutTabs"),
                .layout = {
                    .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                    .childGap = 10,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
            }) {
                ::ui::Selectable({
                    .id = CLAY_ID("WeaponsTab"),
                    .label = "Weapons",
                    .selected = *active_tab == LOADOUT_TAB_WEAPONS,
                    .on_select = [active_tab, selected_index, select_weapons_tab_weapon] {
                        if (active_tab) *active_tab = LOADOUT_TAB_WEAPONS;
                        if (selected_index) *selected_index = first_weapon_for_tab(LOADOUT_TAB_WEAPONS);
                        if (select_weapons_tab_weapon) select_weapons_tab_weapon();
                    },
                });
                ::ui::Selectable({
                    .id = CLAY_ID("GearTab"),
                    .label = "Gear",
                    .selected = *active_tab == LOADOUT_TAB_GEAR,
                    .on_select = [active_tab, selected_index, select_gear_tab_weapon] {
                        if (active_tab) *active_tab = LOADOUT_TAB_GEAR;
                        if (selected_index) *selected_index = first_weapon_for_tab(LOADOUT_TAB_GEAR);
                        if (select_gear_tab_weapon) select_gear_tab_weapon();
                    },
                });
            }
            CLAY({
                .id = CLAY_ID("LoadoutBody"),
                .layout = {
                    .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                    .childGap = 18,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
            }) {
                CLAY({
                    .id = CLAY_ID("WeaponGrid"),
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(400), CLAY_SIZING_FIT(0) },
                        .childGap = 10,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                }) {
                    if (*active_tab == LOADOUT_TAB_WEAPONS) {
                        for (int row = 0; row < 2; ++row) {
                            CLAY({
                                .id = CLAY_IDI("WeaponGridRow", row),
                                .layout = {
                                    .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                                    .childGap = 10,
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                },
                            }) {
                                WeaponTile(row * 2, selected_index);
                                if (row == 0) {
                                    WeaponTile(row * 2 + 1, selected_index);
                                }
                            }
                        }
                    } else {
                        CLAY({
                            .id = CLAY_ID("GearGridRow"),
                            .layout = {
                                .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                                .childGap = 10,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            },
                        }) {
                            WeaponTile(3, selected_index);
                        }
                    }
                }
                CLAY({
                    .id = CLAY_ID("LoadoutDetails"),
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(260), CLAY_SIZING_FIT(0) },
                        .padding = CLAY_PADDING_ALL(14),
                        .childGap = 10,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = { 22, 30, 36, 255 },
                    .cornerRadius = CLAY_CORNER_RADIUS(4),
                }) {
                    CLAY_TEXT(::ui::clay_text(details),
                        CLAY_TEXT_CONFIG({ .textColor = { 226, 238, 236, 255 }, .fontSize = 14 }));
                    ::ui::Toggle({
                        .id = CLAY_ID("CompareToggle"),
                        .label = "Compare",
                        .checked = use_compare_enabled(),
                        .on_change = use_set_compare_enabled(),
                    });
                    ::ui::Button({
                        .id = CLAY_ID("BuyWeaponButton"),
                        .label = "Buy",
                        .disabled = !can_buy,
                        .on_confirm = [pending_action,
                                       pending_weapon_index,
                                       pending_serial,
                                       selected_index] {
                            if (pending_weapon_index && selected_index) {
                                *pending_weapon_index = *selected_index;
                            }
                            if (pending_serial) *pending_serial += 1;
                            if (pending_action) *pending_action = LOADOUT_ACTION_BUY;
                        },
                    });
                    ::ui::Button({
                        .id = CLAY_ID("EquipWeaponButton"),
                        .label = "Equip",
                        .disabled = !can_equip,
                        .on_confirm = [pending_action,
                                       pending_weapon_index,
                                       pending_serial,
                                       selected_index] {
                            if (pending_weapon_index && selected_index) {
                                *pending_weapon_index = *selected_index;
                            }
                            if (pending_serial) *pending_serial += 1;
                            if (pending_action) *pending_action = LOADOUT_ACTION_EQUIP;
                        },
                    });
                    ::ui::Button({
                        .id = CLAY_ID("BackFromLoadoutButton"),
                        .label = "Back",
                        .on_confirm = nav.pop_current,
                    });
                    CLAY_TEXT(::ui::clay_text("Equipment Slots"),
                        CLAY_TEXT_CONFIG({ .textColor = { 202, 218, 216, 255 }, .fontSize = 14 }));
                    EquipmentSlot(CLAY_ID("PrimarySlot"), "Primary", 0, selected_index);
                    EquipmentSlot(CLAY_ID("GearSlot"), "Gear", 3, selected_index);
                }
            }
        }

        if (*pending_action != LOADOUT_ACTION_NONE) {
            LoadoutConfirmDialog(
                *pending_action, *pending_weapon_index, *pending_serial, pending_action);
        }

        ::ui::ui_focus_pop_scope();
    } REACT_COMPONENT_END();
}

static void OptionsScreenView(void) {
    REACT_COMPONENT_BEGIN("OptionsScreenView") {
        client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
        int *large_hud = use_state_int(0);
        int *reduced_motion = use_state_int(1);
        ::ui::ui_focus_push_scope({ .id = CLAY_ID("OptionsScope"), .modal = true });
        ::ui::ui_focus_request_initial_focus(CLAY_ID("LargeHudToggle"));

        CLAY({
            .id = CLAY_ID("OptionsRoot"),
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(24),
                .childGap = 12,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = { 14, 22, 28, 245 },
        }) {
            CLAY_TEXT(::ui::clay_text("Options"),
                CLAY_TEXT_CONFIG({ .textColor = { 236, 246, 242, 255 }, .fontSize = 26 }));
            ::ui::Toggle({
                .id = CLAY_ID("LargeHudToggle"),
                .label = "Large HUD",
                .checked = *large_hud != 0,
                .on_change = [large_hud](bool enabled) { *large_hud = enabled ? 1 : 0; },
            });
            ::ui::Toggle({
                .id = CLAY_ID("ReducedMotionToggle"),
                .label = "Reduce Motion",
                .checked = *reduced_motion != 0,
                .on_change = [reduced_motion](bool enabled) { *reduced_motion = enabled ? 1 : 0; },
            });
            ::ui::Button({
                .id = CLAY_ID("BackFromOptionsButton"),
                .label = "Back",
                .on_confirm = nav.pop_current,
            });
        }

        ::ui::ui_focus_pop_scope();
    } REACT_COMPONENT_END();
}

void ShooterGameScreen::build_ui() {
    ShooterProvider(game_, [this] {
        REACT_COMPONENT_BEGIN_KEY("ShooterGameScreen", entry_id()) {
            ShooterGameScreenView();
        } REACT_COMPONENT_END();
    });
}

void PauseScreen::build_ui() {
    ShooterProvider(game_, [this] {
        REACT_COMPONENT_BEGIN_KEY("PauseScreen", entry_id()) {
            PauseScreenView();
        } REACT_COMPONENT_END();
    });
}

void LoadoutScreen::build_ui() {
    ShooterProvider(game_, [this] {
        REACT_COMPONENT_BEGIN_KEY("LoadoutScreen", entry_id()) {
            LoadoutScreenView();
        } REACT_COMPONENT_END();
    });
}

void OptionsScreen::build_ui() {
    ShooterProvider(game_, [this] {
        REACT_COMPONENT_BEGIN_KEY("OptionsScreen", entry_id()) {
            OptionsScreenView();
        } REACT_COMPONENT_END();
    });
}

} // namespace shooter
