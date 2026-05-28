#include "hud_band.h"

#include "../../../react.h"
#include "../../../ui/retained/element_components.h"
#include "../hooks/shooter_hud.h"

namespace shooter {

struct HudBandProps {
  uint32_t unused = 0;
};

static ::ui::retained::UiElement
render_hud_band(const HudBandProps &props,
                ::ui::retained::UiElementFrame &frame) {
  (void)props;
  ShooterHudRead hud = use_shooter_hud();
  const char *health = use_text_storage("HP %d", hud.health);
  const char *armor = use_text_storage("ARMOR %d", hud.armor);
  const char *ammo = use_text_storage("AMMO %d", hud.ammo);
  const char *credits = use_text_storage("CREDITS %d", hud.credits);
  namespace retained = ::ui::retained;

  auto hud_text = [&frame](const char *key, const char *value) {
    return retained::TextElement(frame,
                                 {
                                     .key = key,
                                     .value = value,
                                     .height = retained::Length::points(22.0f),
                                     .text_color = {224, 238, 236, 255},
                                     .font_size = 18,
                                 });
  };

  return retained::BoxElement(frame,
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
                                  .children = frame.children({
                                      hud_text("health", health),
                                      hud_text("armor", armor),
                                      hud_text("ammo", ammo),
                                      hud_text("credits", credits),
                                      hud_text("weapon", hud.weapon),
                                  }),
                              });
}

::ui::retained::UiElement HudBand(::ui::retained::UiElementFrame &frame) {
  return frame.component("HudBand", HudBandProps{}, render_hud_band);
}

} // namespace shooter
