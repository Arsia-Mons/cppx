#pragma once

#include "ui/components/components.h"

struct StatefulProbeProps {
    const char *key = nullptr;
    int slot = 0;
    int write = -1;
};

ui::retained::UiElement StatefulProbe(ui::retained::UiElementFrame &frame,
                                      const StatefulProbeProps &props);
ui::retained::UiElement
BuildGeneratedRetainedTree(ui::retained::UiElementFrame &frame, int write);
