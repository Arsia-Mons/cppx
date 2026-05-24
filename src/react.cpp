#include "react.h"

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
    int32_t  slot_count;    // highest hook index touched this render
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
} G;

void react_init(Clay_Context *clay_ctx) {
    (void)clay_ctx; // reserved for future use; we don't need to read Clay internals
    memset(&G, 0, sizeof(G));
    for (int i = 0; i < REACT_MAX_FIBERS; i++) G.buckets[i] = -1;
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

static Fiber *fiber_create(uint32_t id) {
    if (G.fiber_count >= REACT_MAX_FIBERS) {
        fprintf(stderr, "react: out of fibers (max=%d)\n", REACT_MAX_FIBERS);
        return nullptr;
    }
    Fiber *f = &G.fibers[G.fiber_count];
    *f = {};
    f->id = id;
    f->next_index = -1;

    uint32_t bucket = id % REACT_MAX_FIBERS;
    int32_t head = G.buckets[bucket];
    if (head < 0) {
        G.buckets[bucket] = G.fiber_count;
    } else {
        Fiber *p = &G.fibers[head];
        while (p->next_index >= 0) p = &G.fibers[p->next_index];
        p->next_index = G.fiber_count;
    }
    G.fiber_count++;
    return f;
}

void react_enter(uint32_t fiber_id) {
    if (G.render_stack_count >= REACT_MAX_RENDER_DEPTH) {
        fprintf(stderr, "react: render stack overflow (max=%d)\n", REACT_MAX_RENDER_DEPTH);
        G.current = nullptr;
        G.hook_index = 0;
        return;
    }

    G.render_stack[G.render_stack_count++] = { G.current, G.hook_index };

    if (fiber_id == 0) {
        fprintf(stderr, "react: component entered with id=0; add a Clay .id\n");
        G.current = nullptr;
        G.hook_index = 0;
        return;
    }

    Fiber *f = fiber_lookup(fiber_id);
    if (!f) f = fiber_create(fiber_id);
    if (!f) { G.current = nullptr; G.hook_index = 0; return; }
    f->generation = G.frame;
    f->slot_count = 0;
    G.current = f;
    G.hook_index = 0;
}

void react_leave(void) {
    if (G.render_stack_count <= 0) {
        fprintf(stderr, "react: leave without matching enter\n");
        G.current = nullptr;
        G.hook_index = 0;
        return;
    }

    RenderFrame previous = G.render_stack[--G.render_stack_count];
    G.current = previous.current;
    G.hook_index = previous.hook_index;
}

static HookSlot *take_slot(int32_t *index_out) {
    if (!G.current) return nullptr;
    int i = G.hook_index++;
    if (i >= REACT_HOOKS_PER_FIBER) {
        fprintf(stderr, "react: hook overflow on fiber %u (max=%d)\n",
                G.current->id, REACT_HOOKS_PER_FIBER);
        return nullptr;
    }
    if (i + 1 > G.current->slot_count) G.current->slot_count = i + 1;
    if (index_out) *index_out = i;
    return &G.current->slots[i];
}

// --- Hooks ---

int *use_state_int(int initial) {
    HookSlot *s = take_slot(nullptr);
    if (!s) { static int sink = 0; sink = initial; return &sink; }
    if (s->kind == HOOK_NONE) {
        s->kind = HOOK_STATE;
        s->u.state.value = initial;
    }
    return &s->u.state.value;
}

void use_effect(ReactEffectFn fn, ReactCleanupFn cleanup, void *user, uint64_t deps_hash) {
    int32_t idx;
    HookSlot *s = take_slot(&idx);
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
            fprintf(stderr, "react: effect queue full\n");
        }
    }
}

void **use_ref(void *initial) {
    HookSlot *s = take_slot(nullptr);
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
    // useContext doesn't consume a hook slot in this simple impl — it's a pure
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
            for (int j = 0; j < REACT_HOOKS_PER_FIBER; j++) {
                run_active_cleanup(&f->slots[j]);
            }
            f->id = 0; // tombstone
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
