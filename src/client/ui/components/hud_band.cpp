#include "hud_band.h"

#include "../../../react.h"
#include "../../../ui/components/components.h"
#include "../hooks/shooter_hud.h"

namespace shooter {

::ui::UiElement HudBand(const HudBandProps &props) {
  (void)props;
  ShooterHudRead hud = use_shooter_hud();
  const char *health = use_text_storage("HP %d", hud.health);
  const char *armor = use_text_storage("ARMOR %d", hud.armor);
  const char *ammo = use_text_storage("AMMO %d", hud.ammo);
  const char *credits = use_text_storage("CREDITS %d", hud.credits);
  namespace components = ::ui::components;

  auto hud_text = [](const char *key, const char *value) {
    return ::ui::component(
        "Text",
        components::TextProps{
            .key = key,
            .value = value,
            .style =
                {
                    .height = ::ui::Length::points(22.0f),
                    .text = {224, 238, 236, 255},
                    .font_size = 18,
                },
        },
        components::Text);
  };

  return ::ui::component(
      "Box",
      components::BoxProps{
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
      },
      components::Box);
}

} // namespace shooter
