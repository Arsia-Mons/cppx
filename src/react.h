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
#include <stdarg.h>

#include <clay.h>

#ifdef __cplusplus
#include <functional>
#include <new>
#include <type_traits>
#include <utility>
#endif

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

// --- Generic state (raw byte slot + destructor) ---
//
// Backs the `use_state<T>` template below. Returns a stable pointer to the
// slot's storage (heap-allocated to honor `align`). `destructor` is invoked
// once when the owning fiber unmounts. `is_new_slot` is set to true on the
// frame the slot is first created so the caller can placement-new the value.

typedef void (*ReactSlotDestructor)(void *storage);

void *react_use_generic_state_slot(uint32_t size,
                                   uint32_t align,
                                   ReactSlotDestructor destructor,
                                   bool *is_new_slot);

// --- Callback memo slot ---
//
// Backs the `use_callback` template below. Returns a stable storage pointer
// large enough for a std::function<void()>. When `deps_hash` differs from the
// previous frame (or on first frame), the existing function is destroyed and
// `is_stale` is set so the caller can reconstruct it; otherwise `is_stale` is
// false and the caller reuses the existing function in place.

void *react_use_callback_slot(uint32_t size,
                              uint32_t align,
                              ReactSlotDestructor destructor,
                              uint64_t deps_hash,
                              bool *is_stale);

// --- Per-fiber printf scratch ---
//
// Returns a stable per-call-site buffer (192 bytes cap). Two instances of the
// same component at different fiber identities receive distinct buffers, so
// they don't clobber each other within a single frame. If the formatted text
// exceeds the cap, the result is truncated (snprintf semantics).

#define REACT_TEXT_STORAGE_CAP 192

#ifdef __GNUC__
const char *use_text_storage(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#else
const char *use_text_storage(const char *fmt, ...);
#endif
const char *use_text_storage_v(const char *fmt, va_list args);

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

#ifdef __cplusplus

// --- use_state<T> ---
//
// Typed analogue of use_state_int. Returns a stable T* keyed by fiber identity
// + call-site index. `initial` is consumed only when the slot is first
// constructed; subsequent frames reuse the existing value. The destructor runs
// once when the owning fiber unmounts.

namespace react_detail {

template <typename T>
void destroy_state_slot(void *storage) {
    static_cast<T *>(storage)->~T();
}

} // namespace react_detail

template <typename T>
T *use_state(T initial) {
    bool is_new_slot = false;
    void *storage = react_use_generic_state_slot(
        (uint32_t)sizeof(T),
        (uint32_t)alignof(T),
        &react_detail::destroy_state_slot<T>,
        &is_new_slot);
    if (!storage) {
        // Slot allocation failed; return a per-call sink so callers never
        // crash. Sink is reconstructed every call — caller writes are lost,
        // which matches the existing use_state_int sink behavior.
        static thread_local typename std::aligned_storage<sizeof(T), alignof(T)>::type sink_storage;
        T *sink = reinterpret_cast<T *>(&sink_storage);
        // Reconstruct in place each call to keep behavior defined.
        sink->~T();
        new (sink) T(std::move(initial));
        return sink;
    }
    if (is_new_slot) {
        new (storage) T(std::move(initial));
    }
    return static_cast<T *>(storage);
}

// --- use_callback ---
//
// Returns a std::function<void()> that is stable across frames as long as
// `deps_hash` matches the previous call at this site. When the hash changes,
// the stored function is destroyed and rebuilt from `fn`.
//
// The returned reference is to per-fiber slot storage; callers typically copy
// it into a handler field (Clay configs take std::function by value).

namespace react_detail {

inline void destroy_callback_slot(void *storage) {
    static_cast<std::function<void()> *>(storage)->~function();
}

} // namespace react_detail

template <typename F>
std::function<void()> &use_callback(F &&fn, uint64_t deps_hash) {
    using Fn = std::function<void()>;
    bool is_stale = false;
    void *storage = react_use_callback_slot(
        (uint32_t)sizeof(Fn),
        (uint32_t)alignof(Fn),
        &react_detail::destroy_callback_slot,
        deps_hash,
        &is_stale);
    if (!storage) {
        static thread_local Fn sink;
        sink = Fn(std::forward<F>(fn));
        return sink;
    }
    if (is_stale) {
        new (storage) Fn(std::forward<F>(fn));
    }
    return *static_cast<Fn *>(storage);
}

#endif // __cplusplus
