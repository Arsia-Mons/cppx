#include "hud_band.h"

#include "../../../react.h"
#include "../../../ui/components/components.h"
#include "../hooks/shooter_hud.h"

namespace shooter {

struct HudBandProps {
  uint32_t unused = 0;
};

static ::ui::UiElement render_hud_band(const HudBandProps &props) {
  (void)props;
  ShooterHudRead hud = use_shooter_hud();
  const char *health = use_text_storage("HP %d", hud.health);
  const char *armor = use_text_storage("ARMOR %d", hud.armor);
  const char *ammo = use_text_storage("AMMO %d", hud.ammo);
  const char *credits = use_text_storage("CREDITS %d", hud.credits);
  namespace components = ::ui::components;

  auto hud_text = [](const char *key, const char *value) {
    return components::Text({
        .key = key,
        .value = value,
        .style =
            {
                .height = ::ui::Length::points(22.0f),
                .text = {224, 238, 236, 255},
                .font_size = 18,
            },
    });
  };

  return components::Box({
      .key = "band",
      .style =
          {
              .width = ::ui::Length::percent(100.0f),
              .height = ::ui::Length::points(44.0f),
              .direction = ::ui::FlexDirection::Row,
              .align_items = ::ui::AlignItems::Center,
              .padding = {16.0f, 16.0f, 10.0f, 10.0f},
              .gap = 18.0f,
              .background = {18, 24, 28, 245},
              .border = {82, 106, 118, 255},
              .border_width = 1.0f,
          },
      .children = ::ui::children({
          hud_text("health", health),
          hud_text("armor", armor),
          hud_text("ammo", ammo),
          hud_text("credits", credits),
          hud_text("weapon", hud.weapon),
      }),
  });
}

::ui::UiElement HudBand() {
  return ::ui::component("HudBand", HudBandProps{}, render_hud_band);
}

} // namespace shooter
