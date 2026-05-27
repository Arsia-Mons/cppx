#pragma once

#include "ui/retained/components.h"

struct StatefulProbeProps {
    const char *key = nullptr;
    int slot = 0;
    int write = -1;
};

void StatefulProbe(const StatefulProbeProps &props);
void BuildGeneratedRetainedTree(int write);
