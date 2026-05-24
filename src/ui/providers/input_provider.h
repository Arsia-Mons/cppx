#pragma once

#include "../../input.h"
#include "../../react.h"

extern ReactContext InputContext;

void input_provider__enter(const InputState *input);
void input_provider__exit(void);

#define INPUT_PROVIDER(input_ptr)                                                \
    for (int _input_provider_once = (input_provider__enter((input_ptr)), 0);     \
         !_input_provider_once;                                                  \
         _input_provider_once = 1, input_provider__exit())
