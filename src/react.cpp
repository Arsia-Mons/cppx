#include "react.h"

#include <stdio.h>
#include <string.h>

// Fixed capacities. Bump if needed; "incredibly simple" hello-world doesn't need much.
#define REACT_MAX_FIBERS        128
#define REACT_HOOKS_PER_FIBER     8
#define REACT_MAX_EFFECT_QUEUE   64

enum HookKind : uint8_t {
    HOOK_NONE   = 0,
    HOOK_STATE  = 1,
    HOOK_EFFECT = 2,
};

struct StateData {
    int value;
};

struct EffectData {
    ReactEffectFn  fn;
    ReactCleanupFn cleanup;
    void          *user;
    uint64_t       deps_hash;
    bool           has_run;
};

struct HookSlot {
    HookKind kind;
    union {
        StateData  state;
        EffectData effect;
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

static struct {
    Fiber             fibers[REACT_MAX_FIBERS];
    int32_t           fiber_count;
    int32_t           buckets[REACT_MAX_FIBERS]; // hash%cap -> fiber index, or -1
    EffectQueueEntry  effect_queue[REACT_MAX_EFFECT_QUEUE];
    int32_t           effect_queue_count;

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
    Fiber *f = fiber_lookup(fiber_id);
    if (!f) f = fiber_create(fiber_id);
    if (!f) { G.current = nullptr; G.hook_index = 0; return; }
    f->generation = G.frame;
    f->slot_count = 0;
    G.current = f;
    G.hook_index = 0;
}

void react_leave(void) {
    G.current = nullptr;
    G.hook_index = 0;
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
        s->u.effect.deps_hash = ~deps_hash; // force mismatch on first run
    }
    EffectData *e = &s->u.effect;

    if (first || e->deps_hash != deps_hash) {
        e->fn       = fn;
        e->cleanup  = cleanup;
        e->user     = user;
        e->deps_hash = deps_hash;
        if (G.effect_queue_count < REACT_MAX_EFFECT_QUEUE) {
            G.effect_queue[G.effect_queue_count++] = { G.current->id, idx };
        } else {
            fprintf(stderr, "react: effect queue full\n");
        }
    }
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
}

static void run_cleanup_if_active(HookSlot *s) {
    if (s->kind == HOOK_EFFECT && s->u.effect.has_run && s->u.effect.cleanup) {
        s->u.effect.cleanup(s->u.effect.user);
        s->u.effect.has_run = false;
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
                run_cleanup_if_active(&f->slots[j]);
            }
            f->id = 0; // tombstone; lookup still skips because id check fails
        }
    }

    // 2. Flush queued effects: cleanup-then-run.
    for (int i = 0; i < G.effect_queue_count; i++) {
        EffectQueueEntry &q = G.effect_queue[i];
        Fiber *f = fiber_lookup(q.fiber_id);
        if (!f) continue; // fiber was unmounted in same frame; skip
        HookSlot *s = &f->slots[q.slot_index];
        if (s->kind != HOOK_EFFECT) continue;
        EffectData *e = &s->u.effect;
        if (e->has_run && e->cleanup) e->cleanup(e->user);
        if (e->fn) e->fn(e->user);
        e->has_run = true;
    }
    G.effect_queue_count = 0;
}
