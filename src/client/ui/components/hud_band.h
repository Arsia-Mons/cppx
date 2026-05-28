#pragma once

#include "../../../ui/runtime/element.h"

namespace shooter {

struct HudBandProps {
  uint32_t unused = 0;
};

::ui::UiElement HudBand(const HudBandProps &props);

} // namespace shooter
