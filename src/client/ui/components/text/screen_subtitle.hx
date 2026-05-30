#pragma once

#include "ui/components/common.h"

namespace shooter {

struct ScreenSubtitleProps {
  const char *key = nullptr;
  const char *value = "";
};

::ui::UiElement ScreenSubtitle(const ScreenSubtitleProps &props);

} // namespace shooter
