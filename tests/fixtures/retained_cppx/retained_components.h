#pragma once

#include "ui/components/components.h"

struct StatefulProbeProps {
  const char *key = nullptr;
  int slot = 0;
  int write = -1;
};

ui::UiElement StatefulProbe(const StatefulProbeProps &props);
ui::UiElement BuildGeneratedRetainedTree(int write);
