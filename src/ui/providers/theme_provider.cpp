#include "theme_provider.h"

#include "../../app_state.h"
#include "../../input.h"
#include "../../react.h"

#include "input_provider.h"

ReactContext ThemeContext = {};

static Theme s_themes[] = {
    { "Dark",    {220, 230, 255, 255}, { 12,  14,  22, 255}, { 28,  32,  48, 255} },
    { "Light",   { 30,  30,  40, 255}, {245, 245, 250, 255}, {220, 225, 235, 255} },
    { "Sunset",  {255, 240, 210, 255}, { 30,  15,  40, 255}, { 80,  30,  60, 255} },
};
static const int s_theme_count = sizeof(s_themes) / sizeof(s_themes[0]);

void theme_provider__enter(void) {
    int                *theme_idx = use_state_int(0);
    const InputState   *input     = (const InputState *)use_context(&InputContext);

    if (input && input->cycle_theme) {
        *theme_idx = (*theme_idx + 1) % s_theme_count;
    }

    react_provider_push(&ThemeContext, &s_themes[*theme_idx]);
}

void theme_provider__exit(void) {
    react_provider_pop(&ThemeContext);
}
