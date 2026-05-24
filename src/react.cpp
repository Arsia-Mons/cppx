#include "react.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Fixed capacities. Bump if needed; "incredibly simple" hello-world doesn't need much.
#define REACT_MAX_FIBERS        128
#define REACT_HOOKS_PER_FIBER     8
#define REACT_MAX_EFFECT_QUEUE   64
#define REACT_MAX_RENDER_DEPTH  128

enum HookKind : uint8_t {
    HOOK_NONE   = 0,
    HOOK_STATE  = 1,
    HOOK_EFFECT = 2,
    HOOK_REF    = 3,
};

struct StateData {
    int value;
};

struct EffectData {
    // What use_effect just scheduled, awaiting next flush.
    ReactEffectFn  pending_fn;
    ReactCleanupFn pending_cleanup;
    void          *pending_user;
    bool           has_pending;

    // The cleanup paired with the most recently executed effect, waiting
    // to be invoked on the next deps-change or on unmount.
    ReactCleanupFn active_cleanup;
    void          *active_user;
    bool           has_active;

    uint64_t       deps_hash;
};

struct RefData {
    void *current;
};

struct HookSlot {
    HookKind kind;
    union {
        StateData  state;
        EffectData effect;
        RefData    ref;
    } u;
};

struct Fiber {
    uint32_t id;            // Clay element ID; 0 = dead/free slot
    int32_t  next_index;    // hash collision chain
    uint32_t generation;    // last frame seen (Clay's generation)
    HookSlot slots[REACT_HOOKS_PER_FIBER];
    int32_t  slot_count;    // hook count from the previous render; -1 on mount
    int32_t  render_slot_count;
    uint32_t next_child_index;
};

struct EffectQueueEntry {
    uint32_t fiber_id;
    int32_t  slot_index;
};

struct RenderFrame {
    Fiber   *current;
    int32_t  hook_index;
};

static struct {
    Fiber             fibers[REACT_MAX_FIBERS];
    int32_t           fiber_count;
    int32_t           buckets[REACT_MAX_FIBERS]; // hash%cap -> fiber index, or -1
    EffectQueueEntry  effect_queue[REACT_MAX_EFFECT_QUEUE];
    int32_t           effect_queue_count;
    RenderFrame       render_stack[REACT_MAX_RENDER_DEPTH];
    int32_t           render_stack_count;

    Fiber            *current;
    int32_t           hook_index;
    uint32_t          frame;   // our own per-frame generation counter
    uint32_t          root_child_index;
    int32_t           error_count;
} G;

static void run_active_cleanup(HookSlot *s);

static void react_report_error(const char *fmt, ...) {
    G.error_count++;
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void react_init(Clay_Context *clay_ctx) {
    (void)clay_ctx; // reserved for future use; we don't need to read Clay internals
    memset(&G, 0, sizeof(G));
    for (int i = 0; i < REACT_MAX_FIBERS; i++) G.buckets[i] = -1;
}

int react_error_count(void) {
    return G.error_count;
}

static Fiber *fiber_lookup(uint32_t id) {
    if (id == 0) return nullptr;
    uint32_t bucket = id % REACT_MAX_FIBERS;
    int32_t idx = G.buckets[bucket];
    while (idx >= 0) {
        Fiber *f = &G.fibers[idx];
        if (f->id == id) return f;
        idx = f->next_index;
    }
    return nullptr;
}

static void fiber_link_to_bucket(int32_t idx) {
    Fiber *f = &G.fibers[idx];
    uint32_t bucket = f->id % REACT_MAX_FIBERS;
    f->next_index = G.buckets[bucket];
    G.buckets[bucket] = idx;
}

static void fiber_unlink_from_bucket(int32_t idx) {
    Fiber *f = &G.fibers[idx];
    if (f->id == 0) return;

    uint32_t bucket = f->id % REACT_MAX_FIBERS;
    int32_t prev = -1;
    int32_t cur = G.buckets[bucket];
    while (cur >= 0) {
        Fiber *entry = &G.fibers[cur];
        if (cur == idx) {
            if (prev >= 0) {
                G.fibers[prev].next_index = entry->next_index;
            } else {
                G.buckets[bucket] = entry->next_index;
            }
            entry->next_index = -1;
            return;
        }
        prev = cur;
        cur = entry->next_index;
    }
}

static Fiber *fiber_create(uint32_t id) {
    int32_t idx = -1;
    for (int32_t i = 0; i < G.fiber_count; i++) {
        if (G.fibers[i].id == 0) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        if (G.fiber_count >= REACT_MAX_FIBERS) {
            react_report_error("react: out of fibers (max=%d)\n", REACT_MAX_FIBERS);
            return nullptr;
        }
        idx = G.fiber_count++;
    }

    Fiber *f = &G.fibers[idx];
    *f = {};
    f->id = id;
    f->slot_count = -1;
    fiber_link_to_bucket(idx);
    return f;
}

static void fiber_destroy(int32_t idx) {
    Fiber *f = &G.fibers[idx];
    if (f->id == 0) return;

    for (int j = 0; j < REACT_HOOKS_PER_FIBER; j++) {
        run_active_cleanup(&f->slots[j]);
    }
    fiber_unlink_from_bucket(idx);
    *f = {};
}

void react_enter(uint32_t fiber_id) {
    if (G.render_stack_count >= REACT_MAX_RENDER_DEPTH) {
        react_report_error("react: render stack overflow (max=%d)\n", REACT_MAX_RENDER_DEPTH);
        G.current = nullptr;
        G.hook_index = 0;
        return;
    }

    G.render_stack[G.render_stack_count++] = { G.current, G.hook_index };

    if (fiber_id == 0) {
        react_report_error("react: component entered with id=0; add a Clay .id\n");
        G.current = nullptr;
        G.hook_index = 0;
        return;
    }

    Fiber *f = fiber_lookup(fiber_id);
    if (!f) f = fiber_create(fiber_id);
    if (!f) { G.current = nullptr; G.hook_index = 0; return; }
    f->generation = G.frame;
    f->render_slot_count = 0;
    f->next_child_index = 0;
    G.current = f;
    G.hook_index = 0;
}

uint32_t react_next_child_index(void) {
    if (!G.current) return G.root_child_index++;
    return G.current->next_child_index++;
}

void react_leave(void) {
    if (G.render_stack_count <= 0) {
        react_report_error("react: leave without matching enter\n");
        G.current = nullptr;
        G.hook_index = 0;
        return;
    }

    Fiber *leaving = G.current;
    if (leaving) {
        if (leaving->slot_count >= 0 && leaving->slot_count != leaving->render_slot_count) {
            react_report_error("react: hook count changed on fiber %u (was=%d now=%d)\n",
                               leaving->id, leaving->slot_count, leaving->render_slot_count);
        }
        leaving->slot_count = leaving->render_slot_count;
    }

    RenderFrame previous = G.render_stack[--G.render_stack_count];
    G.current = previous.current;
    G.hook_index = previous.hook_index;
}

static const char *hook_kind_name(HookKind kind) {
    switch (kind) {
        case HOOK_NONE:   return "none";
        case HOOK_STATE:  return "state";
        case HOOK_EFFECT: return "effect";
        case HOOK_REF:    return "ref";
    }
    return "unknown";
}

static HookSlot *take_slot(HookKind expected, int32_t *index_out) {
    if (!G.current) return nullptr;
    int i = G.hook_index++;
    if (i + 1 > G.current->render_slot_count) G.current->render_slot_count = i + 1;
    if (i >= REACT_HOOKS_PER_FIBER) {
        react_report_error("react: hook overflow on fiber %u (max=%d)\n",
                           G.current->id, REACT_HOOKS_PER_FIBER);
        return nullptr;
    }
    if (index_out) *index_out = i;
    HookSlot *slot = &G.current->slots[i];
    if (slot->kind != HOOK_NONE && slot->kind != expected) {
        react_report_error("react: hook kind changed on fiber %u slot %d (was=%s now=%s)\n",
                           G.current->id, i, hook_kind_name(slot->kind), hook_kind_name(expected));
        return nullptr;
    }
    return slot;
}

// --- Hooks ---

int *use_state_int(int initial) {
    HookSlot *s = take_slot(HOOK_STATE, nullptr);
    if (!s) { static int sink = 0; sink = initial; return &sink; }
    if (s->kind == HOOK_NONE) {
        s->kind = HOOK_STATE;
        s->u.state.value = initial;
    }
    return &s->u.state.value;
}

void use_effect(ReactEffectFn fn, ReactCleanupFn cleanup, void *user, uint64_t deps_hash) {
    int32_t idx;
    HookSlot *s = take_slot(HOOK_EFFECT, &idx);
    if (!s) return;

    bool first = (s->kind == HOOK_NONE);
    if (first) {
        s->kind = HOOK_EFFECT;
        s->u.effect = {};
        s->u.effect.deps_hash = ~deps_hash; // force first-run mismatch
    }
    EffectData *e = &s->u.effect;

    if (first || e->deps_hash != deps_hash) {
        e->pending_fn      = fn;
        e->pending_cleanup = cleanup;
        e->pending_user    = user;
        e->has_pending     = true;
        e->deps_hash       = deps_hash;
        if (G.effect_queue_count < REACT_MAX_EFFECT_QUEUE) {
            G.effect_queue[G.effect_queue_count++] = { G.current->id, idx };
        } else {
            react_report_error("react: effect queue full\n");
        }
    }
}

void **use_ref(void *initial) {
    HookSlot *s = take_slot(HOOK_REF, nullptr);
    if (!s) { static void *sink = nullptr; sink = initial; return &sink; }
    if (s->kind == HOOK_NONE) {
        s->kind = HOOK_REF;
        s->u.ref.current = initial;
    }
    return &s->u.ref.current;
}

// --- Context ---

void react_provider_push(ReactContext *ctx, void *value) {
    if (ctx->depth < REACT_CONTEXT_MAX_DEPTH) {
        ctx->stack[ctx->depth++] = ctx->current;
    }
    ctx->current = value;
}

void react_provider_pop(ReactContext *ctx) {
    if (ctx->depth > 0) {
        ctx->current = ctx->stack[--ctx->depth];
    } else {
        ctx->current = nullptr;
    }
}

void *use_context(ReactContext *ctx) {
    // useContext doesn't consume a hook slot in this simple impl; it's a pure
    // read of the descent-stack. (No selective rerender, so no subscription.)
    return ctx->current;
}

// --- Frame boundaries ---

void react_begin_frame(void) {
    G.frame++;
    G.effect_queue_count = 0;
    G.render_stack_count = 0;
    G.current = nullptr;
    G.hook_index = 0;
    G.root_child_index = 0;
}

static void run_active_cleanup(HookSlot *s) {
    if (s->kind == HOOK_EFFECT && s->u.effect.has_active && s->u.effect.active_cleanup) {
        s->u.effect.active_cleanup(s->u.effect.active_user);
        s->u.effect.has_active = false;
    }
}

void react_end_frame(void) {
    uint32_t current_gen = G.frame;

    // 1. Unmount sweep: any fiber not seen this frame is dead.
    for (int i = 0; i < G.fiber_count; i++) {
        Fiber *f = &G.fibers[i];
        if (f->id == 0) continue;
        if (f->generation != current_gen) {
            fiber_destroy(i);
        }
    }

    // 2. Flush queued effects: prior active cleanup, then new effect.
    //    Uses pending_* for what just got scheduled, active_* for what was
    //    previously running. Crucially: active cleanup uses the *old* user,
    //    not whatever use_effect was just called with.
    for (int i = 0; i < G.effect_queue_count; i++) {
        EffectQueueEntry &q = G.effect_queue[i];
        Fiber *f = fiber_lookup(q.fiber_id);
        if (!f) continue; // fiber unmounted in same frame
        HookSlot *s = &f->slots[q.slot_index];
        if (s->kind != HOOK_EFFECT) continue;
        EffectData *e = &s->u.effect;
        if (!e->has_pending) continue;

        if (e->has_active && e->active_cleanup) {
            e->active_cleanup(e->active_user);
        }
        if (e->pending_fn) {
            e->pending_fn(e->pending_user);
        }
        e->active_cleanup = e->pending_cleanup;
        e->active_user    = e->pending_user;
        e->has_active     = true;
        e->has_pending    = false;
    }
    G.effect_queue_count = 0;
}
