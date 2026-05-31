# Hook runtime reference

Ground truth: `src/ui/runtime/react.h` (header comment is the API doc), `react.cpp`,
`element.h`/`element.cpp`, `interaction_hooks.h`. Executable spec: `tests/react_runtime_tests.cpp`.

## Two authoring layers — don't mix them

1. **Declarative / retained (what you write in screens and components).** A component is a free
   function `::ui::UiElement Foo(const FooProps&)` returned via `::ui::component("Foo", props,
   Foo, key)`. The **Reconciler** (`element.cpp`) owns `react_enter`/`react_leave` and provider
   `push`/`pop` around your `render`. **You never call `react_enter`/`react_leave` yourself.**
2. **Imperative / macro (runtime internals + `react_runtime_tests.cpp` only).**
   `REACT_COMPONENT_BEGIN("Name") { ... } REACT_COMPONENT_END();` (the `_BEGIN` opens a brace the
   `_END` closes), `REACT_PROVIDER_ENTER/EXIT`, `PROVIDE(ctx, value){...}`. You'll see these in
   tests; don't use them in authored UI — calling them inside a retained component double-enters
   the fiber.

## Fiber identity & keys

A **fiber** is the unit of hook-state identity. Its 64-bit id =
`hash(parent fiber id, component name, keyed-salt, sibling-index-or-key)`
(`react_make_instance_fiber_id`, `react.cpp`). Consequences:

- The same component under a different parent, or at a different sibling slot, gets **distinct**
  hook state. This is why state "just works" per-instance.
- **Fixed-order siblings are fine positionally.** Only **reorderable same-type siblings** need a
  key — without one, reordering swaps their hook state. Supply a key via the JSX `key=` /
  `props.key` (string key, hashed) or `REACT_COMPONENT_BEGIN_KEY`.
- List idiom: key each child by a stable item-derived key **and** pass the same index as a prop —
  `<WeaponTile key={weapon_tile_key(i)} index={i} />` (`loadout_weapon_grid.cppx`). Row
  containers also get fixed keys so focus survives tab switches.
- Screens key their component by `screen_entry_key(prefix, entry_id())` — stable across frames,
  unique per stack entry.

## The hooks

Call them **unconditionally, in a fixed order, before any early return.** Hook slots are
positional; the runtime diagnoses a changed hook count or kind. **Cap: 8 hooks per fiber**
(`REACT_HOOKS_PER_FIBER`) — consolidate many cells into one `use_state<struct>`.

| Hook | Shape | Notes |
| --- | --- | --- |
| `use_state_int(int)` | `int*` | Stable per call site; `initial` consumed on first mount only. |
| `use_state<T>(const T&)` | `T*` | Write through the pointer (not a `[value,setter]` tuple). Runs `T`'s destructor on unmount. Rejects `alignof(T) > max_align_t`. |
| `use_ref(void*)` | `void**` | Persistent pointer slot. Does **not** free the pointee on unmount — you do (via `use_effect` cleanup). |
| `use_callback<Sig>(fn, deps_hash)` | `std::function<Sig>&` | Stable across frames while `deps_hash` matches; rebuilt when it changes. Default `Sig` = `void()`. Copy the result into retained handler fields. |
| `use_effect(fn, cleanup, user, deps_hash)` | — | Runs **after** commit when `deps_hash` changes (first mount forced). `fn`/`cleanup` are **captureless** `void(*)(void*)` — pass instance data via `user` (often a `use_ref` slot); cleanup receives the **old** `user`. |
| `use_text_storage(fmt, ...)` | `const char*` | Per-fiber printf scratch (192-byte cap). Use for formatted text passed to nodes — fixes the file-static `char buf[]` clobber bug. |
| `use_context(ReactContext*)` | `void*` | Pure read of the provider value; **consumes no hook slot**, no subscription/selective re-render (the whole tree re-renders each frame). May be called conditionally. |
| `use_focused()`/`use_hovered()`/`use_pressed()`/`use_focus_visible()` | `bool` | Read the per-fiber `InteractionSnapshot` (one-frame lag, keyed by fiber). Feed `resolve()`. |

### Deps hashes

`use_callback`/`use_effect` re-derive only when `deps_hash` differs from the previous frame at
that call site. `deps_hash = 0` (or a constant) means **run once / never rebuild** → stale
captures. Build it from the actual dependencies:

```cpp
client::ui::callback_deps(a, b, c, d)                 // up to 4 u64s mixed
client::ui::callback_deps_ptr(some_pointer)           // hashes pointer identity
```

Navigation closures key on `callback_deps(navigation.current_entry_id)`; deferred setters key on
`callback_deps(callback_deps_ptr(state_ptr), callback_deps_ptr(mutations.owner()))`.

## Context

`ReactContext { void* current; void* stack[16]; int depth; }`. A provider pushes a value for its
children and pops after. Discipline (see `app_provider.cpp`):

1. Keep the `ReactContext` a **file-static** (or anonymous-namespace) global in the provider `.cpp`.
2. In the provider component, `const T* stored = ::ui::copy_value(value);` (arena-copies it so it
   outlives the build call), null-check, then `return ::ui::provider(name, &Ctx,
   const_cast<T*>(stored), children, key);` — else `return ::ui::empty();`.
3. The consumer hook (implemented in the same `.cpp`) does `static_cast<T*>(use_context(&Ctx))`,
   null-checks with `react_report_error("client/ui: missing XProvider\n")`, and **projects** the
   private value into a clean public struct (read fields + named `std::function` setters).

`copy_value`/`copy_string` and `UiChildren` pointers are **per-frame** — they dangle next frame.
Never store `children` or any frame-arena pointer past the synchronous build/commit call; persist
across frames with `use_ref`/`use_state`.

## Element factories

`::ui::component(name, props, render, key=nullptr)`, `::ui::provider(...)`, `::ui::fragment(children)`
(group with no host node), `::ui::empty()` (the null-render / early-return sentinel),
`::ui::copy_value<T>` / `::ui::copy_string`.

## The per-frame contract

`UiPipeline::render_client_ui_frame` drives the canonical order — prefer it over calling
`ClientUi` phases by hand:

```
begin_frame → react_begin_frame → tree.begin_frame
  → build_visible_screens(wrap)   // the PURE declaration pass. It injects Nav + Interaction
                                  // providers; the `wrap` callback (app-installed frame_provider)
                                  // adds Theme/App/Server around each screen root.
  → tree.end_frame → end_layout → update_retained_runtime   // reconcile + layout + build draw IR
  → react_end_frame              // unmount sweep + effect flush (effects observe a committed tree)
  → render_frame()
  → drain_deferred_mutations()   // queued screen ops + closures run HERE, after render
```

`begin_frame` discards stale queued mutations, so if you ever drive phases manually you **must**
`drain_deferred_mutations()` before the next `begin_frame` or scheduled writes vanish.
`CLIENT_UI_MAX_QUEUED_MUTATIONS = 128` is a hard cap — hitting it means a per-frame mutation
*leak* (you're queuing every frame instead of on an event), not a sizing problem.

## Capacities (fixed arrays — overflow degrades, doesn't allocate)

8 hooks/fiber · 128 fibers · 64 queued effects · 384 retained elements · 256 KiB element arena ·
128 queued mutations. Exceeding any reports an error and silently drops behavior.
