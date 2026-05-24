#include "counter.h"

#include <clay.h>

#include "../app_state.h"
#include "../react.h"

#include "panel.h"

#include <SDL3/SDL.h>

#include <stdio.h>

static void on_counter_mount(void *user) {
    (void)user;
    SDL_Log("[effect] Counter mounted");
}

static void on_counter_unmount(void *user) {
    (void)user;
    SDL_Log("[effect] Counter unmounted (cleanup ran)");
}

void Counter(const ReactNoProps &props) {
    (void)props;
    REACT_COMPONENT_BEGIN("Counter") {
        // --- hooks (always in this order; never conditional) ---
        int *count = use_state_int(0);
        use_effect(on_counter_mount, on_counter_unmount, /*user*/ nullptr, /*deps*/ 0);
        Theme *theme = (Theme *)use_context(&ThemeContext);

        // --- input → state (would be a setter in real React) ---
        if (g_edges.up)   *count += 1;
        if (g_edges.down) *count -= 1;

        // --- format the dynamic text into a buffer that outlives Clay_EndLayout ---
        static char count_buf[64];
        snprintf(count_buf, sizeof(count_buf), "Count: %d", *count);

        PanelProps panel = {
            .id = CLAY_ID_LOCAL("CounterPanel"),
            .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
            .padding = CLAY_PADDING_ALL(20),
            .child_gap = 8,
            .child_alignment = {},
            .direction = CLAY_TOP_TO_BOTTOM,
            .background = theme->panel,
            .radius = CLAY_CORNER_RADIUS(8),
        };
        Panel(panel, [&] {
            CLAY_TEXT(cs(count_buf),
                CLAY_TEXT_CONFIG({ .textColor = theme->fg, .fontSize = 32 }));
            CLAY_TEXT(cs("(UP/DOWN to change)"),
                CLAY_TEXT_CONFIG({ .textColor = theme->fg, .fontSize = 14 }));
        });
    } REACT_COMPONENT_END();
}
