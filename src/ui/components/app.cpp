#include "app.h"

#include <clay.h>

#include "../../app_state.h"
#include "../../input.h"
#include "../../react.h"

#include "counter.h"
#include "image.h"

#include "../providers/theme_provider.h"

#include <stdio.h>

// App owns InputContext; ThemeProvider owns ThemeContext (see theme_provider.cpp).
ReactContext InputContext = {};

void App(const InputState *input) {
    REACT_COMPONENT_BEGIN("App") {
        PROVIDE(&InputContext, (void *)input) {
            THEME_PROVIDER {
                Theme *theme = (Theme *)use_context(&ThemeContext);

                if (input && input->toggle_counter) g_show_counter = !g_show_counter;

                static char hint_buf[128];
                snprintf(hint_buf, sizeof(hint_buf),
                         "T: theme (%s)   M: toggle Counter   I: fetch image   ESC: quit",
                         theme->name);

                CLAY({
                    .id = CLAY_ID_LOCAL("Root"),
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                        .padding = CLAY_PADDING_ALL(32),
                        .childGap = 16,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = theme->bg,
                }) {
                    CLAY_TEXT(cs("Hello, World!"),
                        CLAY_TEXT_CONFIG({ .textColor = theme->fg, .fontSize = 40 }));
                    CLAY_TEXT(cs(hint_buf),
                        CLAY_TEXT_CONFIG({ .textColor = theme->fg, .fontSize = 14 }));

                    CLAY({
                        .id = CLAY_ID_LOCAL("ContentRow"),
                        .layout = {
                            .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                            .childGap = 16,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    }) {
                        if (g_show_counter) {
                            Counter();
                        }
                        Image();
                    }
                }
            }
        }
    } REACT_COMPONENT_END();
}
