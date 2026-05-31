#pragma once

// LoadoutWeaponGrid: the weapon-tile grid column. Adapts ui::components::Box
// (width 400, gap 10). Its children reproduce the per-tab row partition exactly:
// weapons tab -> tiles {0,1} in weapon-row-0, tile {2} in weapon-row-1; gear tab
// -> tile {3} in gear-row. Pure factory (no hooks of its own; the tiles read).

#include "ui/components/common.h"

namespace shooter {

struct LoadoutWeaponGridProps {
  const char *key = nullptr;
  int active_tab = 0;
};

::ui::UiElement LoadoutWeaponGrid(const LoadoutWeaponGridProps &props);

} // namespace shooter
