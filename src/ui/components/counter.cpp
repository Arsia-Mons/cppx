#include "counter.h"

#include <clay.h>

#include "../../app_state.h"
#include "../../input.h"
#include "../../react.h"

#include "../providers/input_provider.h"
#include "../providers/theme_provider.h"

#include <SDL3/SDL.h>

#include <stdio.h>

struct CounterScratch {
    char count_buf[64];
};

static void on_counter_mount(void *user) {
    (void)user;
    SDL_Log("[effect] Counter mounted");
}

static void on_counter_unmount(void *user) {
    SDL_Log("[effect] Counter unmounted (cleanup ran)");
    delete static_cast<CounterScratch *>(user);
}

void Counter(void) {
    REACT_COMPONENT_BEGIN("Counter") {
        // --- hooks (always in this order; never conditional) ---
        int *count = use_state_int(0);
        void **scratch_ref = use_ref(nullptr);
        if (!*scratch_ref) {
            *scratch_ref = new CounterScratch();
        }
        CounterScratch *scratch = static_cast<CounterScratch *>(*scratch_ref);
        use_effect(on_counter_mount, on_counter_unmount, scratch, /*deps*/ 0);
        Theme            *theme = (Theme *)use_context(&ThemeContext);
        const InputState *input = (const InputState *)use_context(&InputContext);

        // --- input → state (would be a setter in real React) ---
        if (input && input->increment_counter) *count += 1;
        if (input && input->decrement_counter) *count -= 1;

        // --- format the dynamic text into a buffer that outlives Clay_EndLayout ---
        snprintf(scratch->count_buf, sizeof(scratch->count_buf), "Count: %d", *count);

        CLAY({
            .id = CLAY_ID_LOCAL("CounterPanel"),
            .layout = {
                .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(20),
                .childGap = 8,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = theme->panel,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
        }) {
            CLAY_TEXT(cs(scratch->count_buf),
                CLAY_TEXT_CONFIG({ .textColor = theme->fg, .fontSize = 32 }));
            CLAY_TEXT(cs("(UP/DOWN to change)"),
                CLAY_TEXT_CONFIG({ .textColor = theme->fg, .fontSize = 14 }));
        }
    } REACT_COMPONENT_END();
}
