// Minimal React-style hook runtime on top of Clay.
//
// Fiber identity comes from Clay's parent-hashed element IDs plus a sibling
// position. Use REACT_COMPONENT_BEGIN_KEY for reorderable same-type siblings.
// Hook state lives in a side table keyed by that ID. Effects run after
// Clay_EndLayout. Unmount detection uses the runtime's frame generation.
//
// Public API:
//   react_init(clay_ctx)           - call once after Clay_Initialize.
//   react_begin_frame()            - call once per frame, before component tree.
//   react_end_frame()              - call once per frame, after Clay_EndLayout.
//   REACT_COMPONENT_BEGIN/END      - bracket a component's body.
//   REACT_COMPONENT_BEGIN_KEY/END  - bracket a repeated/keyed component body.
//   REACT_FRAGMENT_COMPONENT_*     - bracket a component that emits its own Clay root.
//   REACT_PROVIDER_ENTER/EXIT      - bracket a transparent provider body.
//   use_state_int(initial)         - returns int* that persists across frames.
//   use_effect(fn, cleanup, user, deps_hash) - runs after commit when deps change.
//   PROVIDE(ctx_ptr, value) { ... } - pushes a context value for the body.
//   use_context(ctx_ptr)           - reads current value of a context.
//
// Component shapes:
//   void Component(const ComponentProps &props);
//
// Components that expose a child slot add a direct C++ callback:
//   template <typename Children>
//   void SlotComponent(const SlotProps &props, Children children);
//
// Slot components decide where children render by calling children() inside
// their Clay body. That keeps composition JSX-like while preserving Clay's
// native parent/child layout model. Children are invoked synchronously; don't
// store them beyond the component call.

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <clay.h>

#ifdef __cplusplus
extern "C" {
#endif

void react_init(Clay_Context *clay_ctx);
void react_begin_frame(void);
void react_end_frame(void);
void react_shutdown(void);
int  react_error_count(void);

// Internal: push/pop the "currently rendering fiber" + reset/restore hook index.
void react_enter(uint32_t fiber_id);
void react_leave(void);
uint32_t react_next_child_index(void);
Clay_ElementId react_make_instance_id(Clay_String name, uint32_t index, bool keyed);

#define REACT_INSTANCE_ID(name_literal)                                           \
    react_make_instance_id(CLAY_STRING(name_literal), react_next_child_index(), false)

#define REACT_INSTANCE_ID_KEY(name_literal, key_index)                            \
    react_make_instance_id(CLAY_STRING(name_literal),                             \
        ((void)react_next_child_index(), (uint32_t)(key_index)), true)

#define REACT_COMPONENT_BEGIN(name_literal)                                       \
    {                                                                             \
        Clay_ElementId _react_cid = REACT_INSTANCE_ID(name_literal);              \
        react_enter(_react_cid.id);                                               \
        CLAY({ .id = _react_cid })

#define REACT_COMPONENT_BEGIN_KEY(name_literal, key_index)                        \
    {                                                                             \
        Clay_ElementId _react_cid = REACT_INSTANCE_ID_KEY(name_literal, key_index); \
        react_enter(_react_cid.id);                                               \
        CLAY({ .id = _react_cid })

#define REACT_COMPONENT_END()                                                     \
        react_leave();                                                            \
    }

#define REACT_FRAGMENT_COMPONENT_BEGIN(name_literal)                              \
    {                                                                             \
        react_enter(REACT_INSTANCE_ID(name_literal).id);

#define REACT_FRAGMENT_COMPONENT_BEGIN_KEY(name_literal, key_index)                \
    {                                                                             \
        react_enter(REACT_INSTANCE_ID_KEY(name_literal, key_index).id);

#define REACT_FRAGMENT_COMPONENT_END()                                             \
        react_leave();                                                            \
    }

#define REACT_PROVIDER_ENTER(name_literal)                                        \
    react_enter(REACT_INSTANCE_ID(name_literal).id)

#define REACT_PROVIDER_ENTER_KEY(name_literal, key_index)                         \
    react_enter(REACT_INSTANCE_ID_KEY(name_literal, key_index).id)

#define REACT_PROVIDER_EXIT()                                                     \
    react_leave()

typedef struct ReactNoProps {
    uint8_t unused;
} ReactNoProps;

#ifdef __cplusplus
#define REACT_NO_PROPS ReactNoProps{}
#else
#define REACT_NO_PROPS ((ReactNoProps){ 0 })
#endif

// --- Hooks ---

int *use_state_int(int initial);

typedef void (*ReactEffectFn)(void *user);
typedef void (*ReactCleanupFn)(void *user);

void use_effect(ReactEffectFn fn, ReactCleanupFn cleanup, void *user, uint64_t deps_hash);

// A slot holding a single pointer that persists across renders.
// Returns the address of the slot; the caller reads `*ref` and writes `*ref = ...`.
void **use_ref(void *initial);

// --- Context / providers ---

#define REACT_CONTEXT_MAX_DEPTH 16

typedef struct ReactContext {
    void *current;
    void *stack[REACT_CONTEXT_MAX_DEPTH];
    int   depth;
} ReactContext;

void  react_provider_push(ReactContext *ctx, void *value);
void  react_provider_pop(ReactContext *ctx);
void *use_context(ReactContext *ctx);

// Scoped provider via the same for-loop trick Clay uses for CLAY(...).
#define PROVIDE(ctx_ptr, value)                                                   \
    for (int _react_once = (react_provider_push((ctx_ptr), (value)), 0);          \
         !_react_once;                                                            \
         _react_once = 1, react_provider_pop((ctx_ptr)))

#ifdef __cplusplus
}
#endif
