#include "input_provider.h"

ReactContext InputContext = {};

void input_provider__enter(const InputState *input) {
    react_provider_push(&InputContext, (void *)input);
}

void input_provider__exit(void) {
    react_provider_pop(&InputContext);
}
