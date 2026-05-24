#include "app.h"

#include <clay.h>

#include "../app_state.h"
#include "../react.h"

#include "counter.h"
#include "image.h"

#include <stdio.h>

void App(void) {
    REACT_COMPONENT_BEGIN("App") {
        // --- hooks ---
        int *theme_idx = use_state_int(0);

        // --- input → state ---
        if (g_edges.m) g_show_counter = !g_show_counter;
        if (g_edges.t) *theme_idx = (*theme_idx + 1) % g_theme_count;

        Theme *theme = &g_themes[*theme_idx];

        static char hint_buf[128];
        snprintf(hint_buf, sizeof(hint_buf),
                 "T: theme (%s)   M: toggle Counter   I: fetch image   ESC: quit",
                 theme->name);

        PROVIDE(&ThemeContext, theme) {
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
    } REACT_COMPONENT_END();
}
