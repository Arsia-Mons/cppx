#include "react.h"

#include <clay.h>

#include <stdio.h>
#include <stdlib.h>

#define CHECK(expr)                                                              \
    do {                                                                         \
        if (!(expr)) {                                                           \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__,  \
                    #expr);                                                      \
            return false;                                                        \
        }                                                                        \
    } while (0)

static Clay_Context *g_clay = nullptr;
static void *g_clay_memory = nullptr;

static void on_clay_error(Clay_ErrorData error) {
    fprintf(stderr, "clay: %.*s\n", (int)error.errorText.length, error.errorText.chars);
}

static bool init_clay_once(void) {
    if (g_clay) return true;

    uint32_t clay_memory_size = Clay_MinMemorySize();
    g_clay_memory = malloc(clay_memory_size);
    CHECK(g_clay_memory != nullptr);

    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(clay_memory_size, g_clay_memory);
    g_clay = Clay_Initialize(arena, Clay_Dimensions{ 640, 480 },
                             Clay_ErrorHandler{ on_clay_error, nullptr });
    CHECK(g_clay != nullptr);
    return true;
}

template <typename Build>
static void run_frame(Build build) {
    react_begin_frame();
    Clay_SetLayoutDimensions(Clay_Dimensions{ 640, 480 });
    Clay_BeginLayout();
    CLAY({ .id = CLAY_ID("TestRoot") }) {
        build();
    }
    (void)Clay_EndLayout();
    react_end_frame();
}

static int g_probe_values[2] = {};

static void PositionalStateProbe(int slot, int initial, int write_value) {
    REACT_COMPONENT_BEGIN("StateProbe") {
        int *value = use_state_int(initial);
        if (write_value >= 0) {
            *value = write_value;
        }
        g_probe_values[slot] = *value;
    } REACT_COMPONENT_END();
}

static void KeyedStateProbe(int key, int initial, int write_value) {
    REACT_COMPONENT_BEGIN_KEY("StateProbe", key) {
        int *value = use_state_int(initial);
        if (write_value >= 0) {
            *value = write_value;
        }
        g_probe_values[key] = *value;
    } REACT_COMPONENT_END();
}

static bool positional_siblings_keep_distinct_state(void) {
    react_init(g_clay);
    g_probe_values[0] = 0;
    g_probe_values[1] = 0;

    run_frame([] {
        PositionalStateProbe(0, 1, 10);
        PositionalStateProbe(1, 2, 20);
    });
    CHECK(react_error_count() == 0);
    CHECK(g_probe_values[0] == 10);
    CHECK(g_probe_values[1] == 20);

    run_frame([] {
        PositionalStateProbe(0, 99, -1);
        PositionalStateProbe(1, 99, -1);
    });
    CHECK(react_error_count() == 0);
    CHECK(g_probe_values[0] == 10);
    CHECK(g_probe_values[1] == 20);
    return true;
}

static bool keyed_siblings_keep_state_across_reorder(void) {
    react_init(g_clay);
    g_probe_values[0] = 0;
    g_probe_values[1] = 0;

    run_frame([] {
        KeyedStateProbe(0, 1, 10);
        KeyedStateProbe(1, 2, 20);
    });
    CHECK(react_error_count() == 0);
    CHECK(g_probe_values[0] == 10);
    CHECK(g_probe_values[1] == 20);

    run_frame([] {
        KeyedStateProbe(1, 99, -1);
        KeyedStateProbe(0, 99, -1);
    });
    CHECK(react_error_count() == 0);
    CHECK(g_probe_values[0] == 10);
    CHECK(g_probe_values[1] == 20);
    return true;
}

static int g_effect_mounts = 0;
static int g_effect_cleanups = 0;

static void on_mount(void *) {
    g_effect_mounts++;
}

static void on_unmount(void *) {
    g_effect_cleanups++;
}

static void EffectProbe(void) {
    REACT_COMPONENT_BEGIN("EffectProbe") {
        use_effect(on_mount, on_unmount, nullptr, 0);
    } REACT_COMPONENT_END();
}

static bool remounts_reuse_unmounted_fibers(void) {
    react_init(g_clay);
    g_effect_mounts = 0;
    g_effect_cleanups = 0;

    for (int i = 0; i < 200; i++) {
        run_frame([] {
            EffectProbe();
        });
        run_frame([] {});
    }

    CHECK(react_error_count() == 0);
    CHECK(g_effect_mounts == 200);
    CHECK(g_effect_cleanups == 200);
    return true;
}

static bool g_use_ref_for_kind_probe = false;
static bool g_extra_hook_for_count_probe = false;

static void KindDriftProbe(void) {
    REACT_COMPONENT_BEGIN("KindDriftProbe") {
        if (g_use_ref_for_kind_probe) {
            (void)use_ref(nullptr);
        } else {
            (void)use_state_int(1);
        }
    } REACT_COMPONENT_END();
}

static void CountDriftProbe(void) {
    REACT_COMPONENT_BEGIN("CountDriftProbe") {
        (void)use_state_int(1);
        if (g_extra_hook_for_count_probe) {
            (void)use_state_int(2);
        }
    } REACT_COMPONENT_END();
}

static bool hook_drift_is_diagnosed(void) {
    react_init(g_clay);
    g_use_ref_for_kind_probe = false;
    run_frame([] {
        KindDriftProbe();
    });
    CHECK(react_error_count() == 0);

    g_use_ref_for_kind_probe = true;
    run_frame([] {
        KindDriftProbe();
    });
    CHECK(react_error_count() == 1);

    react_init(g_clay);
    g_extra_hook_for_count_probe = false;
    run_frame([] {
        CountDriftProbe();
    });
    CHECK(react_error_count() == 0);

    g_extra_hook_for_count_probe = true;
    run_frame([] {
        CountDriftProbe();
    });
    CHECK(react_error_count() == 1);
    return true;
}

int main(void) {
    if (!init_clay_once()) return 1;

    if (!positional_siblings_keep_distinct_state()) return 1;
    if (!keyed_siblings_keep_state_across_reorder()) return 1;
    if (!remounts_reuse_unmounted_fibers()) return 1;
    if (!hook_drift_is_diagnosed()) return 1;

    free(g_clay_memory);
    return 0;
}
