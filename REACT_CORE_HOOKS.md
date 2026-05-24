# Implementing the Core of React on top of an Immediate-Mode C UI Library

This document walks through how to implement the *core* of React — hooks, context, and the render/reconcile/commit pipeline — in a framework-agnostic way, targeting an immediate-mode C/C++ UI library such as Clay UI. The audience is someone who has used React extensively but has never implemented it, and who is now trying to build a React-like authoring layer on top of a UI library that already rebuilds its entire layout tree every frame.

The end goal: components written as functions, hooks for state and effects, providers for context, and a runtime that makes all of that fit naturally into a `while (running) { poll_input(); build_tree(); compute_layout(); draw(); }` loop.

---

## 1. The retained-vs-immediate mental shift

React *looks* retained-mode. You write components, they have state, they "rerender" when state changes. It feels like Cocoa or WinForms: persistent objects with persistent state, mutated in place.

The implementation is the opposite. Every render, React invokes every component function in the tree (modulo bailouts) and produces a fresh tree of "element" descriptions — plain data. It then diffs that fresh tree against the previous one and applies the minimum set of mutations to the host (the DOM). The component instances ("fibers") are retained, but the tree-of-elements is rebuilt from scratch every render. The retained part is just a sidecar that holds hook slots and child pointers.

This is *exactly* what an immediate-mode UI library does, with one twist. Clay UI rebuilds its layout tree every frame: you call `CLAY({...})` declarations top-to-bottom, Clay computes layout, you submit render commands. There is no persistent UI node graph that you mutate. So the "diff against previous tree and apply mutations" step that React performs against the DOM is *not needed* — the host already rebuilds every frame. What you *do* need is the sidecar: a parallel tree of fiber instances that survives across frames and holds hook state.

The mental model becomes: every frame, walk the user's component tree top-down. As you walk, (a) emit `CLAY(...)` declarations into the immediate-mode library, and (b) thread state through a persistent fiber tree on the side. The persistent fiber tree is *the* thing that makes hooks work; it is the only retained data structure in the system.

---

## 2. The fiber / component-instance model

A **fiber** (call it a Node, an Instance, whatever) is a per-render-position object that survives across frames. One fiber per component instance in the tree. It owns:

```cpp
struct Fiber {
    ComponentFn        type;          // function pointer / type tag
    Props              props;         // current props
    Key                key;           // optional user-supplied key
    std::vector<Hook>  hooks;         // positional hook slots
    Fiber*             parent;
    std::vector<Fiber*> children;
    bool               dirty;         // needs rerender
    bool               alive;         // still present after last reconcile
    // host-specific scratch: measured rect, last clay element id, etc.
};
```

**Identity is by position + key + type, not by reference.** When the user writes:

```jsx
<Parent>
  <Foo />
  <Bar />
</Parent>
```

Parent's fiber has two child slots. Slot 0 is "the `Foo` at position 0," slot 1 is "the `Bar` at position 1." Next frame, if the user still emits a `Foo` at slot 0, the runtime *reuses* the old fiber — same hook state, same identity. If the user instead emits a `Baz` at slot 0, the old `Foo` fiber is destroyed (cleanups run, hook state freed) and a new `Baz` fiber is mounted in its place.

**Keys disambiguate lists.** Position-based identity falls apart for reordering:

```jsx
{items.map(item => <Row data={item} />)}
```

If `items` is reordered, position 0 now contains a different logical row. Without keys, the runtime would treat it as "same fiber, new props," and any per-row hook state (expanded? editing?) would teleport to the wrong row. With `key={item.id}`, the runtime matches by key instead of by position, so reordering moves the fibers correctly.

**Unmounting drops the fiber and its hook state.** When a fiber disappears from the tree (no match this frame), the runtime walks its subtree post-order, runs every effect's cleanup, frees hook state, and drops the fiber pointer. There is no resurrection; if the same component reappears next frame, it gets a fresh fiber with fresh hooks.

This is the whole reason "rules of hooks" exist: hook state is bound to fiber identity, and fiber identity is bound to tree position. Move the call out of stable position and you've broken the binding.

---

## 3. Hooks as positional state

The fundamental trick of hooks: during a component's render, hooks are called in a **fixed order**, and the Nth hook call maps to the Nth slot in the fiber's hook array.

```cpp
// Global render context
thread_local Fiber* current_fiber = nullptr;
thread_local int    current_hook_index = 0;

void render_component(Fiber* f) {
    current_fiber = f;
    current_hook_index = 0;
    f->type(f->props);  // user's component function runs here
    current_fiber = nullptr;
}

template <typename T>
Hook& next_hook_slot() {
    auto& hooks = current_fiber->hooks;
    if (current_hook_index >= hooks.size()) {
        hooks.emplace_back();  // first render: create new slot
        hooks.back().kind = HookKind::Uninit;
    }
    Hook& slot = hooks[current_hook_index++];
    return slot;
}
```

That's the entire dispatcher. Every hook (`useState`, `useEffect`, `useContext`, `useMemo`, `useRef`, `useReducer`) is "grab the next slot, do my thing with it." The component function reads/writes its own slots in order without ever naming them.

This is why:

- **No conditionals around hooks.** `if (x) useState(...)` shifts every later hook's index by one, corrupting all subsequent slots.
- **No hooks outside render.** Outside render, `current_fiber` is null and there's nowhere to anchor the slot.
- **No hooks in loops with variable counts.** Same reason as conditionals.

In dev builds, enforce this with a per-fiber `last_hook_count` field and assert at end-of-render that it matches `current_hook_index`. React does exactly this. In C/C++ without runtime "where am I being called from" introspection, this assert is the *only* way to catch hook ordering violations early — make it loud.

---

## 4. `useState` from scratch

The slot stores the current value and a stable setter handle. The setter, when invoked, marks the fiber dirty and stores the pending update.

```cpp
struct StateSlot {
    void* value;
    UpdateQueue pending;  // queued updates, applied at render time
};

template <typename T>
pair<T, std::function<void(T)>> useState(T initial) {
    Hook& slot = next_hook_slot();
    if (slot.kind == HookKind::Uninit) {
        slot.kind = HookKind::State;
        slot.state.value = new T(initial);  // lazy alternative below
    }

    // Apply queued updates (from setters called since last render)
    T current = *static_cast<T*>(slot.state.value);
    while (!slot.state.pending.empty()) {
        auto update = slot.state.pending.pop_front();
        T next = update.is_functional
            ? update.fn(current)
            : update.value;
        if (!std::equal_to{}(next, current)) {
            current = next;
        }
    }
    *static_cast<T*>(slot.state.value) = current;

    // Setter captures fiber + slot index by value — NOT by closure-over-this
    Fiber* owner = current_fiber;
    int   index  = current_hook_index - 1;
    auto setter = [owner, index](T next) {
        owner->hooks[index].state.pending.push({false, next, nullptr});
        schedule_rerender(owner);
    };

    return {current, setter};
}
```

Things to notice:

- **Lazy initial state.** If `initial` is expensive, accept a thunk: `useState([] { return expensive(); })`. Only invoke it when the slot is uninitialized.
- **Functional updates.** `setX(prev => prev + 1)` is essential for correctness when batching. Multiple `setX(x+1)` calls all read the same stale `x`; multiple `setX(p => p+1)` compose correctly. Store the update as `{is_functional, value_or_fn}` in the queue and apply in order.
- **Batching.** During an event handler (input dispatch), multiple `setState` calls should produce one rerender, not N. Implement this as: while inside `dispatch_input_event()`, `schedule_rerender` just marks the fiber dirty; only after the event handler returns does the runtime collect dirty fibers and do a single rebuild.
- **Bailout.** If, after applying all pending updates, the new value `Object.is`-equals (in C++: `==` or a user-supplied comparator) the old value, *don't* mark dirty — there's nothing to do. React does this bailout per-state-slot; if every slot bails, the component skips render entirely (modulo context changes).

The setter must capture `owner` and `index` by value, not by lambda-capturing some `this` on the component. The fiber outlives any particular render frame; the setter must remain valid across rebuilds.

---

## 5. `useReducer` as the general case

`useState` is just `useReducer` with a fixed reducer:

```cpp
template <typename T>
pair<T, std::function<void(T)>> useState(T initial) {
    return useReducer<T, T>(
        [](T, T next) { return next; },  // reducer: ignore prev, take next
        initial
    );
}
```

`useReducer` is sometimes a better primitive for an immediate-mode bridge because it gives you one place to funnel all state mutations — useful when you want a single audit log of every dispatched action per frame, or when state shape grows past a single value.

```cpp
template <typename S, typename A>
pair<S, std::function<void(A)>> useReducer(Reducer<S,A> reducer, S initial) {
    Hook& slot = next_hook_slot();
    if (slot.kind == HookKind::Uninit) {
        slot.kind = HookKind::Reducer;
        slot.reducer.state = new S(initial);
        slot.reducer.fn = reducer;
    }

    // Drain queued actions
    S state = *static_cast<S*>(slot.reducer.state);
    while (!slot.reducer.queue.empty()) {
        A action = slot.reducer.queue.pop_front();
        state = reducer(state, action);
    }
    *static_cast<S*>(slot.reducer.state) = state;

    Fiber* owner = current_fiber;
    int idx = current_hook_index - 1;
    auto dispatch = [owner, idx](A action) {
        owner->hooks[idx].reducer.queue.push(action);
        schedule_rerender(owner);
    };
    return {state, dispatch};
}
```

---

## 6. `useEffect` from scratch

Effects are *registered* during render, *run* after commit.

```cpp
struct EffectSlot {
    std::function<Cleanup()> effect;
    std::optional<DepsArray> deps;
    std::optional<DepsArray> last_deps;
    Cleanup last_cleanup;     // may be null
    bool    pending_run;
};

void useEffect(std::function<Cleanup()> fn,
               std::optional<DepsArray> deps) {
    Hook& slot = next_hook_slot();
    if (slot.kind == HookKind::Uninit) {
        slot.kind = HookKind::Effect;
        slot.effect.pending_run = true;  // first render always runs
        slot.effect.effect = fn;
        slot.effect.deps = deps;
    } else {
        bool changed =
            !deps.has_value() ||                    // undefined deps: every render
            !slot.effect.last_deps.has_value() ||
            deps_differ(*deps, *slot.effect.last_deps);
        slot.effect.pending_run = changed;
        slot.effect.effect = fn;
        slot.effect.deps = deps;
    }
    current_fiber->pending_effects.push_back(&slot.effect);
}
```

After commit, the runtime walks the collected pending effects:

```cpp
void run_passive_effects() {
    for (EffectSlot* e : frame_passive_effects) {
        if (!e->pending_run) continue;
        if (e->last_cleanup) e->last_cleanup();   // cleanup before next effect
        e->last_cleanup = e->effect();            // capture new cleanup
        e->last_deps = e->deps;
        e->pending_run = false;
    }
    frame_passive_effects.clear();
}
```

Dep array semantics:

| Form              | Behavior                                |
|-------------------|-----------------------------------------|
| `useEffect(fn)`   | Run after every commit (deps undefined) |
| `useEffect(fn, [])` | Run once on mount; cleanup on unmount |
| `useEffect(fn, [a,b])` | Run when `a` or `b` changes (and on mount) |

On unmount, run the cleanup one final time. This is part of the fiber-disposal walk: post-order, for each effect slot in the dying fiber, call `last_cleanup` if non-null.

**Timing in an immediate-mode loop.** "After paint" in DOM-React maps to "after the frame's render commands are submitted" in the immediate-mode world. The natural place is after `Clay_EndLayout()` and after you've dispatched draw commands to your renderer (SDL, OpenGL, whatever). Effects can safely call into the OS, fire network requests, set timers — all the stuff that mustn't block the frame.

---

## 7. `useLayoutEffect` vs `useEffect`

`useLayoutEffect` is identical to `useEffect` except in timing: it runs *synchronously* after the tree is committed, but *before* the user sees the frame. This matters when you need to measure layout and write back into state to fix it up before the user notices.

```
Render phase         → run all component functions, collect effects
Commit phase         → host applies the tree (in our case: Clay_EndLayout)
[layout known here]
Layout effects       → useLayoutEffect callbacks fire synchronously
[possible state updates from layout effects trigger a synchronous re-render]
Draw                 → dispatch render commands
[frame visible]
Passive effects      → useEffect callbacks fire
```

In Clay terms: between `Clay_EndLayout()` (which produces the computed layout) and the moment you walk the render-command array to draw, run layout effects. They can read measured rectangles via `Clay_GetElementData(id)` and call `setState` to, e.g., adjust a tooltip's position. If a layout effect calls `setState`, you have a choice: either re-render synchronously within the same frame (the React behavior) or let it land next frame (simpler, one-frame-late tooltip). For most immediate-mode bridges, "land next frame" is fine and avoids a re-entrant render path.

Use `useEffect` for almost everything; reserve `useLayoutEffect` for "I just measured something and need to fix the frame before it's visible."

---

## 8. `useMemo` and `useCallback` as the same primitive

```cpp
template <typename T>
T useMemo(std::function<T()> compute, DepsArray deps) {
    Hook& slot = next_hook_slot();
    if (slot.kind == HookKind::Uninit
        || deps_differ(deps, slot.memo.deps)) {
        slot.kind = HookKind::Memo;
        slot.memo.value = compute();
        slot.memo.deps  = deps;
    }
    return slot.memo.value;
}

template <typename Fn>
Fn useCallback(Fn fn, DepsArray deps) {
    return useMemo<Fn>([&] { return fn; }, deps);
}
```

That's it. `useCallback(fn, deps)` is literally `useMemo(() => fn, deps)`. The point is to give you a *stable reference* to a function so memoized children don't see it as a new prop every render. In immediate-mode without prop-equality memoization (see §14), `useCallback` mostly doesn't matter; keep the API for source-compatibility with React but don't sweat its absence.

---

## 9. `useRef`

```cpp
template <typename T>
struct Ref { T current; };

template <typename T>
Ref<T>* useRef(T initial) {
    Hook& slot = next_hook_slot();
    if (slot.kind == HookKind::Uninit) {
        slot.kind = HookKind::Ref;
        slot.ref.box = new Ref<T>{initial};
    }
    return static_cast<Ref<T>*>(slot.ref.box);
}
```

A `Ref` is a one-slot box that persists across renders. Mutating `ref->current` does *not* trigger a rerender. Used for:

- Holding a Clay element ID across frames so layout effects can look up `Clay_GetElementData(id)`.
- Storing a measured rect from last frame.
- Holding a mutable counter (animation phase, debounce timer) that the render doesn't need to react to.
- Caching a native handle (SDL_Texture*, font atlas pointer) that you populated in an effect.

Trivial to implement; surprisingly load-bearing in practice.

---

## 10. Context and providers

A `Context` is just an identity — a pointer or a unique integer. It has no state of its own; it's a key into the provider stack.

```cpp
struct Context {
    int    id;
    void*  default_value;
};

Context* createContext(void* default_value) {
    static int next_id = 0;
    return new Context{next_id++, default_value};
}
```

A `<Provider value={x}>` pushes `(context_id, x)` onto a runtime stack as the renderer descends into its children, and pops on the way back up.

```cpp
thread_local std::vector<ContextFrame> context_stack;

void render_provider(Context* ctx, void* value, ChildrenFn children) {
    context_stack.push_back({ctx->id, value});
    children();
    context_stack.pop_back();
}

void* useContext(Context* ctx) {
    // Walk back-to-front for the nearest provider
    for (auto it = context_stack.rbegin(); it != context_stack.rend(); ++it) {
        if (it->id == ctx->id) {
            // Subscribe current fiber so it rerenders when the value changes
            current_fiber->subscribed_contexts.insert(ctx->id);
            return it->value;
        }
    }
    return ctx->default_value;
}
```

The descent-stack model is the simplest implementation: context is whatever is on the stack when `useContext` is called. Because the renderer walks the tree depth-first and pushes/pops as it enters/exits providers, the stack always reflects "providers in scope at this point in the tree."

**Who rerenders when a provider's value changes?** Conceptually: every consumer beneath it. In the simplest implementation, when a provider sees its value change relative to last frame, it marks its entire subtree dirty. That's coarse but correct. A more selective implementation tracks which fibers called `useContext(C)` and only marks *those* dirty; this matters once you start memoizing subtrees (see §14). For an immediate-mode bridge that rebuilds everything every frame anyway, the distinction is mostly cosmetic.

**Alternative model: subscription-based context.** Instead of reading off a stack at render time, the Provider keeps a list of subscribed consumer fibers. When `value` changes, it notifies subscribers directly. This is what libraries like Zustand or `use-context-selector` do for fine-grained reactivity. For a v1 React-on-Clay, the descent-stack model is simpler and good enough.

---

## 11. The render → commit → effects pipeline

Three phases, in order, every time the tree needs to be (re)built:

1. **Render.** Pure. Walk the fiber tree top-down. For each fiber: set `current_fiber`, reset `current_hook_index`, call the component function. Hooks pull/push slots. Component returns its children (in a React-y model) or emits `CLAY(...)` declarations directly as a side effect (in the immediate-mode-bridge model). Effects are *registered* into a per-frame queue but not run. New child fibers are created or matched against old ones (reconciliation, §15).

2. **Commit.** The host is mutated. In DOM-React, this is `appendChild`/`removeChild`/`setAttribute` calls. In an immediate-mode bridge against Clay, *render itself emitted the Clay declarations*, so commit collapses to: call `Clay_EndLayout()`, which freezes the tree and runs layout computation. The "host" was already built during render — commit is just the closing parenthesis.

3. **Effects.** Layout effects fire first (synchronous, between commit and draw). Then draw commands are dispatched to the renderer. Then passive effects fire (asynchronous w.r.t. the frame).

The crucial observation for the immediate-mode case: **render and commit are essentially fused.** You can't separate "describe the tree" from "build the tree" because Clay's API *is* the tree-building API. There is no intermediate VDOM data structure unless you build one yourself (and you mostly shouldn't — it doubles the per-frame work for no benefit, since you can't skip the Clay calls anyway).

This has implications for what hooks are *allowed* to do during render: nothing observable. Don't fire network requests. Don't mutate refs. Don't call `setState` on other fibers. Hooks during render must be referentially transparent w.r.t. the host — they read from slots, compute, return, and that's it. Anything with side effects goes in `useEffect`.

---

## 12. Scheduling rerenders

When `setState` is called from outside render (an input handler, an effect, a timer):

1. Apply the update to the slot's pending queue.
2. Mark the owning fiber dirty.
3. Request a rerender of its subtree.

In a stable-tick game loop, "request a rerender" is essentially free: you're going to rebuild next frame anyway. The implementation collapses to "set a dirty flag somewhere."

```cpp
void schedule_rerender(Fiber* f) {
    f->dirty = true;
    runtime.needs_render = true;
}
```

Then at the top of the next frame:

```cpp
void frame() {
    poll_input();          // may call setState, marking fibers dirty
    if (runtime.needs_render || always_render) {
        render_tree(root); // rebuild everything dirty (or everything, for simplicity)
        runtime.needs_render = false;
    }
    Clay_EndLayout();
    run_layout_effects();
    dispatch_draw_commands();
    run_passive_effects();
}
```

**Coalescing.** Multiple `setState` calls in the same input-handler frame should produce one rerender. Easiest implementation: input handlers run before render; whatever setState calls they make all land in pending queues; the single render pass at the top of the frame applies all of them. No special batching code needed — the frame structure does it for you.

**Lazy vs eager.** In a 60Hz game loop, "lazy" (wait for next frame) is almost always fine — 16ms latency is below human perception for most UI. "Eager" (re-render mid-frame) matters only when an effect or input handler needs to see updated UI state before the frame ends. The `useLayoutEffect`-triggers-setState case is the canonical example. You can punt on eager rerender for v1 and just accept a one-frame lag.

**Whose subtree gets rerendered?** Technically, you only need to rerender the dirty fiber and its descendants. In an immediate-mode bridge, *you have to rebuild the entire tree anyway* to emit Clay declarations — Clay doesn't let you splice in a subtree at an arbitrary point. So the practical answer is: rebuild the whole tree, but during the rebuild, *skip the component-function call* for clean fibers if you've cached their last output (memoization). For v1, don't bother — just always re-run every component function. The cost is low and the complexity savings are large.

---

## 13. The immediate-mode bridge specifically

This is the heart of the writeup. Here's what one frame looks like end-to-end:

```cpp
void frame(double dt) {
    // 1. Input + time
    poll_input();           // may call setState; marks fibers dirty
    advance_animations(dt); // may call setState too

    // 2. Render phase: walk the fiber tree, emitting Clay declarations
    Clay_BeginLayout();
    current_fiber = root;
    render_tree(root);      // calls component fns; each emits CLAY(...) blocks
    current_fiber = nullptr;

    // 3. Commit phase: Clay finalizes layout
    Clay_RenderCommandArray cmds = Clay_EndLayout();

    // 4. Layout effects (between layout and draw)
    run_layout_effects();   // may read Clay_GetElementData(id), may setState

    // 5. Draw
    dispatch(cmds);         // walk Clay_RenderCommandArray, call SDL_Render*

    // 6. Passive effects (after the frame is visible)
    run_passive_effects();

    // 7. Dispose unmounted fibers (run their cleanups)
    reap_dead_fibers();

    present();              // SDL_RenderPresent or equivalent
}
```

`render_tree` recursively descends the fiber tree:

```cpp
void render_tree(Fiber* f) {
    if (!f->dirty && f->cached_output) {
        // memoization: replay last frame's Clay declarations
        replay_cached(f);    // requires capturing them; see §14
        return;
    }
    current_fiber = f;
    current_hook_index = 0;
    push_context_for(f);           // push any Provider frames
    f->type(f->props);             // user code runs here, emits CLAY(...)
    pop_context_for(f);
    f->dirty = false;
}
```

The user's component function:

```cpp
void Counter(CounterProps p) {
    auto [count, setCount] = useState(0);
    useEffect([=] {
        printf("count is now %d\n", count);
        return []{};
    }, {count});

    CLAY({ .id = CLAY_ID("Counter"),
           .layout = { .padding = CLAY_PADDING_ALL(8) } }) {
        CLAY_TEXT(CLAY_STRING("Count:"), TextConfig());
        // child components nested here
        ChildButton({ .onClick = [=]{ setCount(count + 1); } });
    }
}
```

`CLAY(...)` is a side effect of `render_tree` — the Clay tree is built *as* the fiber tree is walked. There is no intermediate "elements" data structure. The fiber tree exists purely to hold hook slots; the Clay tree exists purely to drive layout and drawing; the two are built in lockstep but stored separately.

**The asymmetry with DOM-React.** In DOM-React, render is "build a JS object tree" (cheap) and commit is "apply diffs to the DOM" (expensive). In immediate-mode-React, render *is* the host build — every component function call directly emits `CLAY(...)`. There is no separate cheap "describe" phase. This means:

- You can't skip a subtree by skipping its Clay declarations and "carrying over" last frame's, *unless* you cache the declarations themselves. Clay needs every element declared every frame to produce the layout.
- The benefit of memoization is therefore lower: skipping `Component()` saves the function-call cost but you still need *something* to emit the Clay declarations for that subtree. Either you cache and replay, or you re-run.
- The cost of always re-rendering is also lower than people fear: component functions are pure (by convention), and a few hundred function calls per frame is nothing.

---

## 14. Memoization and bailout in an immediate-mode world

`React.memo(C)` wraps a component so that, when its props are shallow-equal to last render's, the component function isn't called and last render's output is reused.

In DOM-React this is straightforward: "last render's output" is a JS tree, and "reused" means "don't run the diff against children; assume identical." In immediate-mode-React, "last render's output" is a sequence of `CLAY(...)` calls already made against last frame's Clay context — those calls don't persist into this frame. To "reuse" them, you'd have to either:

1. **Record and replay.** During render, capture every `CLAY(...)` call (config + nesting structure) into a per-fiber buffer. On bailout, replay the buffer into the new Clay context. This is doable but requires hooking Clay's macro layer and is a non-trivial amount of plumbing.

2. **Re-run the function, accept the cost.** Treat memoization as a non-feature. Component functions are pure; running them is cheap; Clay's layout cost dominates anyway.

For a v1 bridge, **option 2 is the right call.** Skip `React.memo`, skip `useMemo`-driven prop stability, skip `useCallback`-as-rerender-prevention. Always re-render. The architectural simplification is enormous and the per-frame cost is small.

This becomes an *explicit non-goal* for v1. Write it down so future-you doesn't try to introduce it half-heartedly.

If you later need memoization for perf reasons (e.g., a huge list with expensive child components), the path is option 1 — but treat it as a deliberate optimization for hot subtrees, not a global feature.

---

## 15. Reconciliation details

Reconciliation is the process of matching old children to new children at each fiber.

**Match criteria, in order:**

1. **Position** — the Nth new child matches the Nth old child by default.
2. **Key** — if children carry keys, match by key instead of position. Keys override position.
3. **Type** — even with matching position+key, if the component type differs, treat it as a different fiber: unmount old, mount new.

When unkeyed children's positions stay stable, position matching works fine. When a list is reordered or items insert/remove in the middle, you *must* use keys or fiber state will teleport.

Pseudocode for keyed children reconciliation:

```cpp
void reconcile_children(Fiber* parent,
                        std::vector<Element> new_children) {
    std::unordered_map<Key, Fiber*> old_by_key;
    std::vector<Fiber*> old_unkeyed;
    for (Fiber* old : parent->children) {
        if (old->key.has_value()) old_by_key[*old->key] = old;
        else                       old_unkeyed.push_back(old);
    }

    std::vector<Fiber*> next_children;
    size_t unkeyed_cursor = 0;

    for (Element& e : new_children) {
        Fiber* matched = nullptr;
        if (e.key.has_value()) {
            auto it = old_by_key.find(*e.key);
            if (it != old_by_key.end() && it->second->type == e.type) {
                matched = it->second;
                old_by_key.erase(it);
            }
        } else if (unkeyed_cursor < old_unkeyed.size()
                   && old_unkeyed[unkeyed_cursor]->type == e.type) {
            matched = old_unkeyed[unkeyed_cursor++];
        }

        if (matched) {
            matched->props = e.props;
            matched->alive = true;
            next_children.push_back(matched);
        } else {
            Fiber* fresh = new Fiber{e.type, e.props, e.key, ...};
            fresh->parent = parent;
            next_children.push_back(fresh);
        }
    }

    // Unmount everything left over
    for (auto& [k, old] : old_by_key)    unmount(old);
    for (size_t i = unkeyed_cursor; i < old_unkeyed.size(); ++i)
        unmount(old_unkeyed[i]);

    parent->children = next_children;
}
```

Unmounting is post-order:

```cpp
void unmount(Fiber* f) {
    for (Fiber* child : f->children) unmount(child);
    for (Hook& h : f->hooks) {
        if (h.kind == HookKind::Effect && h.effect.last_cleanup)
            h.effect.last_cleanup();
    }
    delete f;
}
```

Note that in the immediate-mode model, you don't actually have a "list of Element" sitting around — children are emitted by side effect during the parent's render. Reconciliation in that world looks slightly different: as the parent's render emits each child component invocation, the runtime intercepts the invocation, finds-or-creates the matching child fiber, and recurses into it. The "list of new children" is built up incrementally as the parent's render proceeds. The matching logic above still applies, just streamed rather than batched.

---

## 16. Error handling

Error boundaries are components that catch render-time errors thrown by their descendants. Implementation: wrap each child render in a try/catch from the error boundary's render frame.

```cpp
void render_with_boundary(Fiber* boundary, Fiber* child) {
    try {
        render_tree(child);
    } catch (const std::exception& e) {
        boundary->error_state = e.what();
        // re-render boundary itself, which will render a fallback UI
        schedule_rerender(boundary);
    }
}
```

`getDerivedStateFromError` and `componentDidCatch` are just two phases of this: the former lets the boundary update its state synchronously to render a fallback, the latter is a side-effect hook (typically used for logging) that runs after.

In C++, you'll want to be careful about what counts as a "render-time error." Exceptions out of arbitrary user code, sure. But if your component functions are `noexcept` and use error codes, you'll need a sentinel return value instead. Either is fine; just pick a convention.

Effect errors are *not* caught by error boundaries (they happen outside render). Decide what you want there: log + ignore is the usual choice.

---

## 17. Concurrent rendering, suspense, transitions

Out of scope for v1. React's concurrent mode introduces interruptible rendering — the runtime can start rendering a tree, pause partway, work on something more urgent, and resume. Suspense lets a component "throw a promise" to signal it needs to wait for data; the runtime catches it, falls back to a placeholder, and retries when the promise resolves. Transitions let you mark some state updates as low-priority so they don't block urgent ones.

All of this requires (a) a scheduler with priority queues, (b) the ability to discard a partial render, (c) deeply rethinking the render pipeline as resumable. In an immediate-mode game loop running at 60Hz, you can usually skip all of this: just render synchronously each frame. If you need to gate on async data, do it with `useEffect` + state, not Suspense. Note that these exist, mark them as "v2 if ever," and move on.

---

## 18. A minimal implementation sketch

Roughly 150 lines of C++-ish pseudocode covering the core. Plug Clay calls in where noted.

```cpp
// ---- Types ----
using Key = std::optional<int>;
using DepsArray = std::vector<uint64_t>; // hashes of dep values

enum class HookKind { Uninit, State, Reducer, Effect, LayoutEffect,
                      Memo, Ref, Context };

struct EffectData {
    std::function<void()> effect;
    std::function<void()> cleanup;
    DepsArray deps;
    bool has_deps;
    bool pending_run;
};

struct StateData {
    void* value;
    std::deque<std::function<void*(void*)>> updates; // functional updates
};

struct Hook {
    HookKind kind = HookKind::Uninit;
    union {
        StateData    state;
        EffectData   effect;
        // ... memo, ref, etc.
    };
};

struct Fiber {
    void (*type)(void*);
    void* props;
    Key   key;
    std::vector<Hook> hooks;
    Fiber* parent = nullptr;
    std::vector<Fiber*> children;
    bool dirty = true;
    bool alive = true;
    std::unordered_set<int> subscribed_contexts;
};

// ---- Globals ----
thread_local Fiber* current_fiber = nullptr;
thread_local int    current_hook_index = 0;
thread_local std::vector<std::pair<int,void*>> context_stack;

std::vector<EffectData*> frame_passive_effects;
std::vector<EffectData*> frame_layout_effects;
std::vector<Fiber*>      pending_unmounts;

// ---- Hook dispatcher ----
Hook& next_hook() {
    auto& hs = current_fiber->hooks;
    if (current_hook_index >= (int)hs.size()) hs.emplace_back();
    return hs[current_hook_index++];
}

// ---- useState ----
template<typename T>
std::pair<T, std::function<void(T)>> useState(T init) {
    Hook& h = next_hook();
    if (h.kind == HookKind::Uninit) {
        h.kind = HookKind::State;
        h.state.value = new T(init);
    }
    T cur = *(T*)h.state.value;
    while (!h.state.updates.empty()) {
        auto u = h.state.updates.front(); h.state.updates.pop_front();
        cur = *(T*)u(&cur);
    }
    *(T*)h.state.value = cur;
    Fiber* f = current_fiber; int i = current_hook_index - 1;
    auto setter = [f, i](T next) {
        f->hooks[i].state.updates.push_back(
            [next](void*) { return (void*) new T(next); });
        f->dirty = true;
    };
    return {cur, setter};
}

// ---- useEffect ----
void useEffect(std::function<std::function<void()>()> fn,
               std::optional<DepsArray> deps) {
    Hook& h = next_hook();
    bool first = (h.kind == HookKind::Uninit);
    if (first) { h.kind = HookKind::Effect; h.effect.has_deps = false; }
    bool changed = first || !deps.has_value()
                 || h.effect.deps != *deps;
    if (changed) {
        // wrap to capture cleanup
        auto wrapper = [fn, &h]() {
            if (h.effect.cleanup) h.effect.cleanup();
            h.effect.cleanup = fn();
        };
        h.effect.effect = wrapper;
        h.effect.pending_run = true;
        h.effect.deps = deps.value_or(DepsArray{});
        h.effect.has_deps = deps.has_value();
        frame_passive_effects.push_back(&h.effect);
    }
}

// ---- useContext ----
void* useContext(int ctx_id, void* default_value) {
    current_fiber->subscribed_contexts.insert(ctx_id);
    for (auto it = context_stack.rbegin(); it != context_stack.rend(); ++it)
        if (it->first == ctx_id) return it->second;
    return default_value;
}

// ---- Render ----
void render_fiber(Fiber* f) {
    current_fiber = f;
    current_hook_index = 0;
    // PLUG IN: providers in f's element push onto context_stack here
    // PLUG IN: f->type emits CLAY(...) declarations as side effects
    f->type(f->props);
    // PLUG IN: pop context_stack frames pushed above
    f->dirty = false;
}

void render_tree(Fiber* root) {
    render_fiber(root);
    for (Fiber* c : root->children) render_tree(c);
}

// ---- Commit ----
void commit() {
    // PLUG IN: Clay_RenderCommandArray cmds = Clay_EndLayout();
    // run layout effects synchronously
    for (auto* e : frame_layout_effects) if (e->pending_run) {
        e->effect(); e->pending_run = false;
    }
    frame_layout_effects.clear();
    // PLUG IN: dispatch cmds to your renderer (SDL, GL, ...)
    // run passive effects
    for (auto* e : frame_passive_effects) if (e->pending_run) {
        e->effect(); e->pending_run = false;
    }
    frame_passive_effects.clear();
    // reap unmounted fibers
    for (Fiber* f : pending_unmounts) destroy(f);
    pending_unmounts.clear();
}

// ---- Per-frame loop ----
void frame() {
    poll_input();           // handlers may setState → mark dirty
    // PLUG IN: Clay_BeginLayout();
    render_tree(root);      // emits CLAY(...) as side effects
    commit();               // Clay_EndLayout + layout effects + draw + effects
}
```

That's the whole core. Everything else (`useReducer`, `useMemo`, `useRef`, `useLayoutEffect`, error boundaries) follows the same pattern and can be added incrementally.

---

## 19. Pitfalls when bridging to immediate-mode C/C++

A grab bag of gotchas that bite when you port React semantics to a non-GC, non-closure-friendly language.

**No GC; cleanup must be deterministic.** In JS, when a fiber is dropped, its hook slots become garbage. In C++, you own those slots. Every hook kind needs a destructor path:
- `useState`: `delete` the value pointer.
- `useEffect`: call `last_cleanup` if non-null, *then* destroy the function objects.
- `useMemo`: same.
- `useRef`: `delete` the box (carefully — refs may be shared if you return raw pointers).

Build a single `destroy_hook(Hook&)` dispatch and route the unmount walk through it.

**Setters can't capture `this`.** A naive C++ implementation might write:

```cpp
auto setter = [this](T next) { this->value = next; };
```

This is wrong. The component function doesn't have a stable `this` — it's a function, not a method. The setter has to capture a *fiber handle* (`Fiber*` or, better, a `FiberId` integer if you intern fibers) and a *slot index* (int), and look up the slot at call time. Both must be by value:

```cpp
Fiber* owner = current_fiber;
int    idx   = current_hook_index - 1;
auto setter = [owner, idx](T next) { /* lookup at call time */ };
```

If you store `FiberId` instead of raw `Fiber*`, you get a graceful failure when the fiber is unmounted before the setter is called (e.g., a debounced input handler firing after the user navigated away): just check the table, no-op if absent. With raw pointers, you risk use-after-free.

**Hook ordering needs runtime assertions.** Languages with stack-trace-based dev tools (JS) can detect "you called a hook outside render." C++ can't, easily. Compensate by:
- In debug builds, on every hook call: `assert(current_fiber != nullptr)`.
- At end of render: `assert(current_hook_index == fiber.hooks.size())` *if* the fiber rendered before. (First render establishes the count; later renders must match.)
- Tag each hook slot with its `HookKind` at creation; on subsequent renders, assert the requested hook kind matches the slot's kind. A drifted hook order will show up as a mismatch immediately.

**Prop comparison without `Object.is`.** React uses `Object.is` for state-change detection and shallow prop comparison for `React.memo`. In C++, you have to pick a comparator per prop type. Easiest: require components to take a single POD struct as `Props` and use `memcmp` for the comparison. For complex props (strings, vectors), provide a `props_equal(A, B)` overload per type, or skip the comparison entirely (you're already deferring memoization to v2).

**String identity.** Clay's `CLAY_STRING("...")` typically wraps a `{ length, chars }` view. Two string-equal Clay strings might or might not have the same pointer. If you're hashing prop deps for `useMemo`/`useEffect`, hash by content not by pointer. Same goes for any user-defined value type.

**Memory ownership of props.** Props are produced by a parent's render. In JS they're just object references and live as long as anyone holds them. In C++, decide:
- **Copy props into the fiber.** Simple, safe, costs a copy per render. Probably the right default.
- **Arena-allocate per-frame.** A per-frame bump allocator zeroed at frame start. Props live for one frame; fibers don't hold pointers across frames (instead, the parent re-emits them next frame). Fast, but you must be disciplined about not stashing prop pointers in refs/effects.

Whichever you pick, document it loudly. Mixing the two will produce nasal-demon bugs.

**Closure capture in effects.** When `useEffect(fn, deps)` captures locals from the render scope, those locals are pointers to either prop fields or hook-slot values. By the time the effect runs (after commit), the next frame might already be in progress. If your captures point into the *current* fiber's hook slots, they're stable; if they point into prop memory that the parent recycled, they're dangling. Default to capture-by-value (which in C++ means a real copy, not a reference). If a prop is heavyweight, capture only the fields you need.

**Threading.** Don't. Run everything on the UI thread. Any background work (HTTP, file I/O) lives in `useEffect` and communicates back via a thread-safe message queue that the UI thread drains at the top of each frame, calling `setState` on the appropriate fibers. Trying to run hooks on multiple threads will go badly.

---

## 20. What's worth implementing first

A prioritized v1 checklist. Don't skip steps; don't add steps from v2.

**v1: Build this.**

1. **Fiber tree with stable identity.** Position-based matching is enough; keys are nice-to-have but defer if you're impatient.
2. **Hook dispatcher.** Global `current_fiber` + `current_hook_index`. Make the debug-build hook-kind assertions loud.
3. **`useState`.** With functional updates and bailout-on-equal. Skip lazy initializer if you must, but it's twenty lines so just do it.
4. **`useEffect`.** With dep arrays. Wire the post-commit effect queue.
5. **`useRef`.** Trivial and you'll want it for Clay element IDs.
6. **Context with descent-stack `useContext`.** Coarse "rerender whole provider subtree on value change" is fine.
7. **The render → commit → effects loop.** Synchronous, single-threaded. One frame per game tick.
8. **Dirty-marking + per-frame rerender.** Just set a flag, rebuild next frame. No mid-frame re-entry.
9. **Reconciliation: position-based, with key override.** Get the keyed children algorithm right; lists are where bugs hide.
10. **Unmount cleanup walk.** Every effect cleanup must run. Every hook slot must be destroyed. No leaks.

**v1.5: Once v1 works and you've shipped a real screen.**

11. **`useReducer`.** Free, since `useState` is a special case. Sometimes nicer for complex state.
12. **`useMemo` / `useCallback`.** Provide the API; they're trivial. Don't expect them to gate rendering yet.
13. **`useLayoutEffect`.** For tooltip-positioning-style use cases.
14. **Keys on lists.** If you punted in v1.
15. **Error boundaries.** A few try/catches and a fallback-render path.

**v2 (only if you measure a need).**

16. **Subtree memoization** via Clay-call recording and replay. Only for hot subtrees identified by profiling.
17. **Selective context subscription** so providers don't blast the whole subtree dirty.
18. **Eager mid-frame rerender** for layout-effect-driven setState chains.

**Out of scope, probably forever.**

19. **Concurrent mode, suspense, transitions.** A 60Hz synchronous loop doesn't need them.
20. **`forwardRef`, refs-as-callbacks, imperative handles.** Cute API but adds machinery; refs as plain `{current}` boxes solve 95% of cases.
21. **Server components, hydration, portals.** Nothing to hydrate; nothing to portal to.

---

## Closing notes

The point of this writeup is to demystify the bridge. React on top of an immediate-mode UI library is not a contradiction — it's a *more honest* implementation of what React already does. The user's mental model ("components have state, things rerender") is preserved exactly; the implementation just happens to fit immediate-mode's "rebuild every frame" model better than it fits the DOM's "mutate in place" model.

Three things to internalize:

1. **The fiber tree is the only retained data structure.** Everything else (Clay tree, render commands, prop values) is rebuilt every frame. Fibers hold hook slots; hook slots survive rebuilds; that's the whole magic.

2. **Hooks are positional, full stop.** Every "rule of hooks" follows from this. Position determines slot determines value. Break position, break state.

3. **Render is the host build.** Don't try to separate "describe" from "emit Clay calls." They're the same pass. Effects are how you do anything that *isn't* host-building.

Build v1 as described, ship a screen with it, then revisit. Most of the "I wish I had X" items dissolve on contact with real usage.
