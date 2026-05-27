#include "hud_band.h"

#include <clay.h>

#include "../hooks/shooter_hud.h"
#include "../../../react.h"
#include "../../../ui/primitives/clay_text.h"

namespace shooter {

void HudBand() {
    REACT_FRAGMENT_COMPONENT_BEGIN("HudBand") {
        ShooterHudRead hud = use_shooter_hud();
        const char *health  = use_text_storage("HP %d",      hud.health);
        const char *armor   = use_text_storage("ARMOR %d",   hud.armor);
        const char *ammo    = use_text_storage("AMMO %d",    hud.ammo);
        const char *credits = use_text_storage("CREDITS %d", hud.credits);
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

} // namespace shooter
