#include "hud_band.h"

#include "../hooks/shooter_hud.h"
#include "../../../react.h"
#include "../../../ui/retained/components.h"

namespace shooter {

void HudBand() {
    REACT_RETAINED_COMPONENT_BEGIN("HudBand") {
        ShooterHudRead hud = use_shooter_hud();
        const char *health  = use_text_storage("HP %d",      hud.health);
        const char *armor   = use_text_storage("ARMOR %d",   hud.armor);
        const char *ammo    = use_text_storage("AMMO %d",    hud.ammo);
        const char *credits = use_text_storage("CREDITS %d", hud.credits);
        namespace retained = ::ui::retained;
        auto hud_text = [](const char *key, const char *value) {
            retained::Text({
                .key = key,
                .value = value,
                .height = retained::Length::points(22.0f),
                .text_color = {224, 238, 236, 255},
                .font_size = 18,
            });
        };

        retained::Panel(
            {
                .key = "band",
                .width = retained::Length::percent(100.0f),
                .height = retained::Length::points(44.0f),
                .direction = retained::FlexDirection::Row,
                .align_items = retained::AlignItems::Center,
                .padding = {16.0f, 16.0f, 10.0f, 10.0f},
                .gap = 18.0f,
                .background = {18, 24, 28, 245},
                .border = {82, 106, 118, 255},
                .border_width = 1.0f,
            },
            [&] {
                hud_text("health", health);
                hud_text("armor", armor);
                hud_text("ammo", ammo);
                hud_text("credits", credits);
                hud_text("weapon", hud.weapon);
            });
    } REACT_RETAINED_COMPONENT_END();
}

} // namespace shooter
