# Implementing the Core of React

This document describes how to build the *core* of React from first principles, independent of any host renderer or platform. The goal is the algorithm and the data structures: a tree of component instances with positional hook state, a render walk producing a new description of the tree, a reconciliation step matching new descriptions to existing instances, a commit step applying changes, and an effects queue that fires after commit.

One note up front. The same core machinery is host-agnostic by design. It can sit on top of a retained-mode tree (the host owns persistent nodes that you mutate in place) or on top of an immediate-mode system that rebuilds its tree every frame from a fresh description. The hook dispatcher, the fiber/instance bookkeeping, the dep-array semantics, the reconciliation algorithm, the effects pipeline — identical in both cases. Only "commit" differs in what it does to the host. For the rest of the document we drop the distinction and talk about "the host tree" or "the commit target" in abstract terms.

---

## 1. What "core React" actually is

Strip away JSX (sugar for function calls), the DOM renderer (one specific host), the concurrent scheduler (an optimization), devtools, SyntheticEvent, Suspense, transitions, server components, and `act()`. What remains is small:

- A **tree of component instances** (historically "fibers"). Each instance is one invocation of a component function in the rendered tree.
- A **per-instance hook-state array**, indexed by the order hooks were called.
- A **render function** that walks the tree, calls components, lets them touch hook slots, and returns a new tree of *element descriptions* (plain data).
- A **reconciliation step** matching new descriptions to old instances by identity — preserving state where possible, mounting/unmounting where not.
- A **commit step** applying the structural and prop diffs to the host tree.
- An **effects queue** populated during render and drained after commit.
- A **scheduler** that decides when to start the next render — at its simplest, "next tick after any instance is marked dirty."

Everything else React ships is either a host-specific concrete, a developer-experience layer, or an optimization.

The mental model: **render is pure, commit mutates, effects run after.** Three phases, conceptually disjoint. The whole document hangs off this pipeline.

---

## 2. The fiber / component-instance model

An *element* is plain data: `{type, props, key}`. It describes what the user wants. An *instance* (fiber) is the runtime object that backs an element across renders: it owns hook state, child instances, last props, dirty flags, and a back-pointer to its parent.

Across renders, instances are matched to new elements by a positional identity:

```
identity(child) = (parent, key ?? positionIndex, componentType)
```

- If a new element at a position has the same `(key, type)` as the old child at that position, the old instance is reused: hook state preserved, props updated, recurse.
- If it differs, the old instance is **unmounted** (running all cleanups, depth-first) and a new instance is **mounted** fresh.

Position-based identity is the default. Without keys, the third child in render N is matched to the third child in render N+1, regardless of what was rendered there. This is fine for static layouts and breaks for reorderable lists: swap items A and B and position-only matching keeps each *slot's* state in place, so A's state ends up on B and vice versa. Focus, scroll, input contents, animations all stick to the wrong item.

Keys fix this. When an element carries a `key`, identity becomes `(parent, key, type)`, independent of position. Stateful list items can reorder, insert, and remove without state migrating.

Two invariants:

1. **Type changes always remount.** Same position, same key, but the type differs across renders? Unmount, mount. State is never transferred across types.
2. **Keys are scoped to siblings.** Cousins with the same key don't conflict; siblings with the same key are a bug.

The instance owns: `parent`, `type`, `props`, `key`, `children`, `hookSlots`, `dirty`, `mounted`, `pendingPassiveEffects`, `pendingLayoutEffects`, and optionally a handle into the host's tree (if the host is retained-mode).

---

## 3. Hooks as positional state on the instance

The trick: **hooks are stored positionally on the currently-rendering instance.** During render, the runtime sets a global "currently-rendering instance" pointer and a "hook index" counter. Each hook call:

1. Looks up `currentInstance.hookSlots[hookIndex]`.
2. Initializes the slot if absent (first render for this slot).
3. Reads or updates the slot.
4. Increments `hookIndex`.
5. Returns whatever the hook contract returns.

The Rules of Hooks fall out as direct consequences:

- **No conditional hook calls.** Skip a hook on render N+1 and every subsequent index shifts; slot N+1 (a `useState` last time) is suddenly read as `useEffect`. Garbage.
- **No early returns before hooks.** Same problem.
- **No calling hooks outside render.** No current instance, no hook index.
- **No calling hooks from regular functions** unless those functions are themselves only called during render (custom hooks).

Minimal dispatcher state:

```c
struct Runtime {
    Instance* currentInstance;   // set during renderInstance, null otherwise
    int       currentHookIndex;  // reset to 0 at start of each renderInstance
};
```

In debug builds, also store the *kind* of each hook slot (`STATE`, `EFFECT`, `REF`, ...) and assert on read that kind matches the call. This catches Rules-of-Hooks violations early instead of silent state corruption.

Custom hooks are just functions that happen to call hooks. Same rules — unconditional, same order every render. The positional indexing flows through transparently; no runtime support needed.

---

## 4. `useState` from first principles

Slot: `{value}`. The setter is created once on mount and captures the *handle* `(instanceId, slotIndex)` — not a pointer to the slot, because the slot's memory might move across renders, but the handle is stable.

```c
struct StateSlot { HookKind kind; Value value; };

(Value, Setter) useState(InitialOrFn initial) {
    Instance* inst = runtime.currentInstance;
    int idx = runtime.currentHookIndex++;

    StateSlot* slot;
    if (idx >= inst->hookSlots.len) {
        slot = alloc(StateSlot);
        slot->kind = STATE;
        slot->value = isFunction(initial) ? initial() : initial;  // lazy init
        inst->hookSlots.push(slot);
    } else {
        slot = (StateSlot*)inst->hookSlots[idx];
        assert(slot->kind == STATE);
    }
    return (slot->value, makeSetter(inst->id, idx));
}

Setter makeSetter(InstanceId id, int idx) {
    return closure(next) {
        Instance* inst = runtime.findInstance(id);
        if (!inst) return;                       // instance gone; drop update
        StateSlot* slot = (StateSlot*)inst->hookSlots[idx];
        Value resolved = isFunction(next) ? next(slot->value) : next;
        if (objectIs(resolved, slot->value)) return;  // bail-out
        slot->value = resolved;
        markDirty(inst);
        scheduleRender();
    };
}
```

Points worth highlighting:

- **Lazy initial state.** `useState(() => expensive())` defers to mount only.
- **Functional updates.** `setX(prev => prev + 1)` reads the *current* slot value at call time, not the value captured by the setter closure. This is why multiple `setX(p => p+1)` in a row compose correctly even when each call sees the same stale captured `x`.
- **Bail-out.** New value `Object.is`-equal to old? Do nothing. Don't mark dirty.
- **Batching.** Multiple sets in one synchronous handler should mark dirty repeatedly but schedule only one render. The scheduler coalesces (§12).
- **Stable setter identity.** The same setter function is returned every render. Downstream code can rely on referential stability.

---

## 5. `useReducer` as the general primitive

`useState` is a degenerate `useReducer`. Slot stores `{state, reducer}`; the reducer is read fresh each render (so it can close over fresh values); dispatch becomes:

```c
(State, Dispatch) useReducer(Reducer reducer, InitialArg init, InitFn? initFn) {
    Instance* inst = runtime.currentInstance;
    int idx = runtime.currentHookIndex++;
    ReducerSlot* slot = ensureSlot(inst, idx, REDUCER, () -> initFn ? initFn(init) : init);
    slot->reducer = reducer;
    return (slot->state, makeDispatch(inst->id, idx));
}

Dispatch makeDispatch(InstanceId id, int idx) {
    return closure(action) {
        Instance* inst = runtime.findInstance(id);
        if (!inst) return;
        ReducerSlot* slot = (ReducerSlot*)inst->hookSlots[idx];
        Value next = slot->reducer(slot->state, action);
        if (objectIs(next, slot->state)) return;
        slot->state = next;
        markDirty(inst);
        scheduleRender();
    };
}
```

And literally:

```c
(Value, Setter) useState(InitialOrFn initial) {
    return useReducer(
        (prev, next) -> isFunction(next) ? next(prev) : next,
        initial,
        (i) -> isFunction(i) ? i() : i);
}
```

`useReducer` shines when transitions are non-trivial, when several call sites share update logic, or when the transition is worth testing in isolation as pure `(state, action) -> state`.

---

## 6. `useEffect` from first principles

Effects are *registered* during render and *executed* after commit. Slot: `{deps, cleanup, pendingEffect}`.

```c
struct EffectSlot {
    HookKind kind;          // EFFECT
    Deps?    deps;
    Cleanup? cleanup;       // returned from previous run
    Effect?  pendingEffect; // queued by render, drained after commit
};

void useEffect(Effect effect, Deps? deps) {
    Instance* inst = runtime.currentInstance;
    int idx = runtime.currentHookIndex++;
    bool first = idx >= inst->hookSlots.len;
    EffectSlot* slot = ensureSlot(inst, idx, EFFECT);

    bool changed = first || deps == null || !depsEqual(slot->deps, deps);
    if (changed) {
        slot->pendingEffect = effect;
        slot->deps = deps;
        inst->pendingPassiveEffects.push(slot);
    }
}
```

After commit, the runtime walks the queue:

```c
void flushPassiveEffects() {
    for (slot in allPendingPassiveEffects) {
        if (slot->cleanup) slot->cleanup();          // cleanup PREVIOUS effect
        slot->cleanup = slot->pendingEffect();       // run new effect, capture cleanup
        slot->pendingEffect = null;
    }
}
```

On unmount, every effect slot's `cleanup` (if any) runs.

Dep-array semantics:

- `undefined` (omitted) → every commit. Almost always a bug or a deliberate "sync every frame" pattern.
- `[]` → once on mount, cleanup on unmount. The effect's closure captures values from the first render only.
- `[a,b,c]` → on mount, then whenever any dep is unequal to last render's, comparing slot-wise.

Ordering invariants:

1. **Cleanup before next effect.** Two runs of the same slot never overlap.
2. **Effects fire in instance + hook-index order.** Commit walks bottom-up, so children's effects fire before parents' on mount; reverse on unmount.
3. **Effects fire after commit.** Reading the host inside an effect always sees post-commit state.

Equality helper:

```c
bool depsEqual(Deps? prev, Deps? next) {
    if (prev == null || next == null) return false;
    if (prev.len != next.len) return false;
    for (i in 0..prev.len) if (!objectIs(prev[i], next[i])) return false;
    return true;
}
```

---

## 7. `useLayoutEffect` vs `useEffect`

Same hook shape, different scheduling. Layout effects fire **synchronously** after commit but **before** the host has a chance to present the new state. Passive effects fire **asynchronously** later, after the user has seen the frame.

Why split them?

- **Layout effects** exist for the "commit changes, measure something on the host, derive new state from the measurement, re-commit, all before any pixel hits the screen" case. The classic example: position a tooltip from a measured anchor. A passive effect there produces a one-frame flicker.
- **Passive effects** are everything else — data fetches, subscriptions, logging. They don't block presentation.

Two queues. After commit:

1. `flushLayoutEffects()` synchronously — blocking until done.
2. Yield to the host for "frame present."
3. `flushPassiveEffects()` asynchronously on the next tick.

A subtlety: a layout effect that calls a setter triggers another render that completes before the user sees the original frame. This is the mechanism that makes "measure and reposition" race-free. The cost is misuse can produce loops; the runtime should detect "layout effect rendered the same instance twice in one tick" and warn.

---

## 8. `useMemo` and `useCallback`

Same primitive. Slot: `{value, deps}`. Recompute when deps change.

```c
Value useMemo(Compute compute, Deps deps) {
    Instance* inst = runtime.currentInstance;
    int idx = runtime.currentHookIndex++;
    MemoSlot* slot = ensureSlot(inst, idx, MEMO);
    if (!inst->mounted || !depsEqual(slot->deps, deps)) {
        slot->value = compute();
        slot->deps = deps;
    }
    return slot->value;
}

Fn useCallback(Fn fn, Deps deps) { return useMemo(() -> fn, deps); }
```

The equivalence is exact; nothing else differs. Caveat: `useMemo` is a hint. React itself reserves the right to drop cached values and recompute. Simple implementations always cache.

---

## 9. `useRef`

A `{current}` cell that persists across renders and never triggers a rerender on mutation.

```c
Ref useRef(Value initial) {
    Instance* inst = runtime.currentInstance;
    int idx = runtime.currentHookIndex++;
    RefSlot* slot = ensureSlot(inst, idx, REF);
    if (!inst->mounted) slot->ref.current = initial;
    return &slot->ref;   // stable across renders
}
```

The ref object itself is stable. Mutating `ref.current` does **not** mark dirty and does **not** schedule a render. That's the whole point: a back channel for values that participate in imperative logic but not in dataflow.

Typical uses: holding an imperative handle (timer ID, file descriptor, subscription token); holding the "latest" of some prop or state so a long-lived callback can read the current value without being recreated; tracking whether the component has mounted at least once.

---

## 10. Context and providers

A context is an identity:

```c
Context createContext(Value defaultValue) { return { id: freshUniqueId(), defaultValue }; }
```

Two implementations.

### (a) Descent stack

A global stack per context. As the renderer descends into a `Provider`, push the provider's value; on the way back up, pop. `useContext(C)` reads the top of `C`'s stack (or `C.defaultValue` if empty).

```c
void renderInstance(Instance* inst) {
    if (isProvider(inst->type)) {
        ContextId cid = inst->props.context.id;
        runtime.ctxStacks[cid].push(inst->props.value);
        renderChildren(inst);
        runtime.ctxStacks[cid].pop();
    } else {
        // normal render
    }
}

Value useContext(Context c) {
    Instance* inst = runtime.currentInstance;
    int idx = runtime.currentHookIndex++;
    ensureSlot(inst, idx, CONTEXT);                // mostly for hook-ordering assert
    Stack<Value>& stk = runtime.ctxStacks[c.id];
    return stk.empty() ? c.defaultValue : stk.top();
}
```

Conceptually clean. The downside: when a provider's value changes, every descendant consumer must rerender — and the runtime has to walk down to find consumers (or rerender broadly, bailing out via memoization where possible).

### (b) Subscription

Each `useContext(C)` registers the calling instance as a subscriber to the nearest `<Provider C>` ancestor. When the provider's value changes (its `value` prop differs across renders), it iterates its subscriber set and marks each dirty.

This enables true selective rerender — no descent walk, just notify the known subscribers. The cost: providers carry a subscriber set; consumers register/unregister on mount/unmount; unsubscription must be reliable.

React uses (a) augmented with bailout. A v1 should start with (a): smaller, correct, fast enough for normal-sized trees.

### Who rerenders on a provider change

In model (a): conceptually all descendants are re-walked. If a subtree's root is wrapped in `memo` and the only thing that changed is a context it doesn't consume, the bailout skips it. So in practice: consumers always rerender; non-consumers under memo skip; non-consumers without memo rerender wastefully.

In model (b): only consumers rerender. Memoization is irrelevant to propagation.

---

## 11. The render → commit → effects pipeline

Three disjoint phases per update.

### Render (pure)

Walk the tree from the root (or each dirty subtree root). For each instance: set `currentInstance` and `currentHookIndex = 0`, call the component, hooks fire in order touching slots, the return value is an element description. Reconcile children (§13) against existing child instances, recurse. Effects are *registered* into pending queues, not executed.

Render must not touch the host. It produces only an updated instance tree, a pending-effects list, and a pending diff against the host tree.

If render throws, an error boundary (§15) catches it; otherwise the whole render is discarded.

### Commit (mutating)

Apply the diff to the host tree: new instance → create + insert; removed → detach + destroy; moved (keyed reorder) → reorder; same instance, new props → update node properties. Commit is the only phase that talks to the host. It must be atomic from the user's perspective. (For an immediate-mode host that rebuilds its tree from a description every frame, "commit" degenerates to "emit the new description"; no diff to apply.)

### Effects

Two queues, drained in order:

1. **Layout effects**, synchronously, before yielding to host present.
2. **Passive effects**, asynchronously after present.

Both: cleanup for slots whose deps changed → then new effect.

Effects may call setters. If a layout effect does, the render-commit-layout loop runs again synchronously before present. If a passive effect does, the next render is scheduled normally.

The pipeline is the contract: components rely on render being pure, commit being atomic, effects firing after the host has settled.

---

## 12. Scheduling rerenders

When a setter or external trigger marks an instance dirty, the scheduler eventually rerenders it. The simplest version:

```c
void markDirty(Instance* inst) {
    runtime.dirtySet.insert(inst);
    if (!runtime.renderScheduled) {
        runtime.renderScheduled = true;
        host.scheduleNextTick(performRenderPass);
    }
}

void performRenderPass() {
    runtime.renderScheduled = false;
    Set<Instance*> dirty = move(runtime.dirtySet);
    for (root in highestAncestorsIn(dirty)) renderInstance(root);
    commitTree();
    flushLayoutEffects();
    host.requestPresent();
    enqueueMicrotask(flushPassiveEffects);
}
```

Coalescing falls out: any number of `markDirty` between the schedule and the actual pass merge into one. This is "automatic batching" — a typical event handler triggers many setters that all mark dirty but produce one render.

More elaborate schedulers add:

- **Priority lanes.** Different update urgencies. Render highest first.
- **Time-slicing.** Yield mid-walk when a deadline is exceeded; resume later. Requires an explicit work stack (no recursion).
- **Suspension.** A component throws a promise; the scheduler suspends the subtree, awaits, retries.

That's "concurrent mode" — out of scope for the core. A fully synchronous scheduler is correct, tractable, and where every implementation should start.

---

## 13. Reconciliation algorithm

Given a parent with existing children `old[]` and new element list `new[]` from the parent's render, the reconciler matches them. Identity: `(key ?? positionIndex, type)`.

### Unkeyed case

```c
void reconcileChildrenUnkeyed(Instance* parent, Element[] newElements) {
    int i = 0;
    int oldLen = parent->children.len, newLen = newElements.len;

    for (; i < min(oldLen, newLen); i++) {
        Instance* oldChild = parent->children[i];
        Element   newEl    = newElements[i];
        if (oldChild->type == newEl.type) {
            oldChild->props = newEl.props;
            renderInstance(oldChild);
        } else {
            unmountInstance(oldChild);
            parent->children[i] = mountInstance(parent, newEl);
        }
    }
    for (; i < oldLen; i++) unmountInstance(parent->children[i]);
    parent->children.truncate(newLen);
    for (; i < newLen; i++) parent->children.push(mountInstance(parent, newElements[i]));
}
```

Correct but loses state on any reordering.

### Keyed case

Two-pass, map-based, `O(n)`:

```c
void reconcileChildrenKeyed(Instance* parent, Element[] newElements) {
    Map<Key, Instance*> oldByKey;
    Vec<Instance*>      oldUnkeyed;
    for (oldChild in parent->children) {
        if (oldChild->key != null) oldByKey[oldChild->key] = oldChild;
        else                       oldUnkeyed.push(oldChild);
    }

    Vec<Instance*> newChildren;
    Set<Instance*> reused;
    int unkeyedCursor = 0;

    for (newEl in newElements) {
        Instance* match = null;
        if (newEl.key != null) { match = oldByKey.get(newEl.key); oldByKey.remove(newEl.key); }
        else if (unkeyedCursor < oldUnkeyed.len) match = oldUnkeyed[unkeyedCursor++];

        if (match && match->type == newEl.type) {
            match->props = newEl.props;
            renderInstance(match);
            newChildren.push(match);
            reused.insert(match);
        } else {
            if (match) unmountInstance(match);    // type changed, can't reuse
            newChildren.push(mountInstance(parent, newEl));
        }
    }

    for (oldChild in parent->children) if (!reused.contains(oldChild)) unmountInstance(oldChild);
    parent->children = newChildren;
}
```

The cost: a hash map per parent per reconciliation. The win: stateful list items retain identity across reordering. For very small lists, a linear scan beats the map; production implementations often special-case `n <= 4`.

To restate: **state attaches to the instance, the instance is matched by identity, and identity defaults to position.** If you want state to track a logical item across reorderings, give the runtime an identity stable across reorderings — a key.

---

## 14. Memoization and bailout

`memo(Component)` wraps a component so that if props are shallowly equal to last render's *and* the instance is not internally dirty, the runtime skips rerendering and reuses the previous element tree.

```c
Component memo(Component inner, EqualityFn? propsEqual) {
    return wrapped(props) {
        Instance* inst = runtime.currentInstance;
        Element lastResult = inst->memoCache?.result;
        Props   lastProps  = inst->memoCache?.props;

        bool canSkip = lastResult && !inst->dirty &&
            (propsEqual ? propsEqual(lastProps, props) : shallowEqual(lastProps, props));

        if (canSkip) return lastResult;

        Element next = inner(props);
        inst->memoCache = { props, result: next };
        return next;
    };
}
```

Subtleties:

- **Internal dirtiness wins.** A setter fired in this instance → must rerender, props equality irrelevant.
- **Context changes pierce memo.** A consumer rerenders when its context value changes, regardless.
- **Shallow equality.** Compare each prop slot with `Object.is`. New object literals every render defeat memo; this is what `useMemo`/`useCallback` are for upstream.

When to use memo: deep trees with a frequently-rerendering parent where most descendants don't depend on what changed. It costs a shallow equality check on every render — not free.

---

## 15. Error boundaries

Components that catch render-time errors thrown by descendants. Wrap the descendant render walk in try/catch within the boundary's own render frame.

```c
void renderInstance(Instance* inst) {
    if (inst->isErrorBoundary) {
        try { renderChildrenNormally(inst); }
        catch (err) {
            inst->errorState = err;
            renderChildrenNormally(inst);  // boundary's render returns the fallback
        }
    } else {
        renderChildrenNormally(inst);
    }
}
```

`getDerivedStateFromError(err)` is sugar for "before the rerender, apply this update." `componentDidCatch(err, info)` runs as a side effect after commit; logging goes there.

Boundaries catch: errors from descendant render and descendant lifecycle hooks. They do **not** catch: errors from event handlers (no render frame on the stack) and errors from async code (effects, promises) — those need their own try/catch.

Boundaries are subtree-scoped. An uncaught error propagates to the next boundary up; with no boundary, the whole tree unmounts.

---

## 16. Concurrent rendering, suspense, transitions

Out of scope for the core. Briefly so you know what you're *not* getting:

- **Concurrent rendering.** Render becomes interruptible — explicit work stack, yields mid-tree, resumes later. Each work unit checks a deadline.
- **Priority lanes.** Each update has a priority. The scheduler runs higher first, may throw away in-progress lower work when higher arrives.
- **Suspense.** A component throws a thenable; the runtime catches it, suspends the subtree, shows a fallback, retries on resolve. Requires the walk to handle "not ready" without aborting.
- **Transitions.** Sugar for marking updates as low-priority.

A v1 core is fully synchronous: render top-to-bottom in one pass, commit, flush effects, return. Everything concurrent lives on top.

---

## 17. A minimal implementation sketch

Language-agnostic, host-agnostic, C-/Rust-like. The host is an opaque interface that knows how to create, update, reorder, and destroy nodes.

```c
// ---- Core types ----

typedef int InstanceId;

enum HookKind {
    HOOK_STATE, HOOK_REDUCER, HOOK_EFFECT, HOOK_LAYOUT_EFFECT,
    HOOK_MEMO, HOOK_REF, HOOK_CONTEXT
};

struct HookSlot {
    HookKind kind;
    union {
        struct { Value value; }                                       state;
        struct { Value state; Reducer reducer; }                      reducer;
        struct { Deps? deps; Cleanup? cleanup; Effect? pending; }     effect;
        struct { Value value; Deps deps; }                            memo;
        struct { Value current; }                                     ref;
        struct { ContextId cid; Value lastRead; }                     context;
    } u;
};

struct Element { Type type; Props props; Key? key; };

struct Instance {
    InstanceId      id;
    Instance*       parent;
    Type            type;
    Props           props;
    Key?            key;
    Vec<Instance*>  children;
    Vec<HookSlot>   hookSlots;
    bool            dirty;
    bool            mounted;
    Element?        lastRenderResult;
    Vec<HookSlot*>  pendingPassiveEffects;
    Vec<HookSlot*>  pendingLayoutEffects;
    HostHandle      hostHandle;          // null for non-host components
};

struct Runtime {
    Instance*                    root;
    Instance*                    currentInstance;
    int                          currentHookIndex;
    Set<Instance*>               dirtySet;
    bool                         renderScheduled;
    Map<ContextId, Stack<Value>> ctxStacks;
    InstanceId                   nextInstanceId;
};
Runtime runtime;

// ---- Hooks ----

HookSlot* ensureSlot(Instance* inst, int idx, HookKind kind) {
    if (idx >= inst->hookSlots.len) inst->hookSlots.push({ .kind = kind });
    HookSlot* slot = &inst->hookSlots[idx];
    debug_assert(slot->kind == kind);
    return slot;
}

(Value, Setter) useState(InitialOrFn initial) {
    Instance* inst = runtime.currentInstance;
    int idx = runtime.currentHookIndex++;
    bool first = idx >= inst->hookSlots.len;
    HookSlot* slot = ensureSlot(inst, idx, HOOK_STATE);
    if (first) slot->u.state.value = isFunction(initial) ? initial() : initial;
    return (slot->u.state.value, makeSetter(inst->id, idx));
}

Setter makeSetter(InstanceId id, int idx) {
    return closure(next) {
        Instance* inst = findInstance(id);
        if (!inst) return;
        HookSlot* slot = &inst->hookSlots[idx];
        Value r = isFunction(next) ? next(slot->u.state.value) : next;
        if (objectIs(r, slot->u.state.value)) return;
        slot->u.state.value = r;
        markDirty(inst);
    };
}

void useEffect(Effect effect, Deps? deps) {
    Instance* inst = runtime.currentInstance;
    int idx = runtime.currentHookIndex++;
    bool first = idx >= inst->hookSlots.len;
    HookSlot* slot = ensureSlot(inst, idx, HOOK_EFFECT);
    if (first || deps == null || !depsEqual(slot->u.effect.deps, deps)) {
        slot->u.effect.pending = effect;
        slot->u.effect.deps    = deps;
        inst->pendingPassiveEffects.push(slot);
    }
}

void useLayoutEffect(Effect effect, Deps? deps) {
    // Identical to useEffect but queued into pendingLayoutEffects.
    Instance* inst = runtime.currentInstance;
    int idx = runtime.currentHookIndex++;
    bool first = idx >= inst->hookSlots.len;
    HookSlot* slot = ensureSlot(inst, idx, HOOK_LAYOUT_EFFECT);
    if (first || deps == null || !depsEqual(slot->u.effect.deps, deps)) {
        slot->u.effect.pending = effect;
        slot->u.effect.deps    = deps;
        inst->pendingLayoutEffects.push(slot);
    }
}

Value useContext(Context c) {
    Instance* inst = runtime.currentInstance;
    int idx = runtime.currentHookIndex++;
    HookSlot* slot = ensureSlot(inst, idx, HOOK_CONTEXT);
    Stack<Value>& stk = runtime.ctxStacks[c.id];
    Value v = stk.empty() ? c.defaultValue : stk.top();
    slot->u.context.cid = c.id;
    slot->u.context.lastRead = v;
    return v;
}

Ref useRef(Value initial) {
    Instance* inst = runtime.currentInstance;
    int idx = runtime.currentHookIndex++;
    bool first = idx >= inst->hookSlots.len;
    HookSlot* slot = ensureSlot(inst, idx, HOOK_REF);
    if (first) slot->u.ref.current = initial;
    return &slot->u.ref.current;
}

Value useMemo(Compute compute, Deps deps) {
    Instance* inst = runtime.currentInstance;
    int idx = runtime.currentHookIndex++;
    bool first = idx >= inst->hookSlots.len;
    HookSlot* slot = ensureSlot(inst, idx, HOOK_MEMO);
    if (first || !depsEqual(slot->u.memo.deps, deps)) {
        slot->u.memo.value = compute();
        slot->u.memo.deps  = deps;
    }
    return slot->u.memo.value;
}

// ---- Render walk ----

void renderInstance(Instance* inst) {
    runtime.currentInstance  = inst;
    runtime.currentHookIndex = 0;

    bool isProv = isProviderType(inst->type);
    ContextId pushedCid = -1;
    if (isProv) {
        pushedCid = inst->props.context.id;
        runtime.ctxStacks[pushedCid].push(inst->props.value);
    }

    Element result;
    if (isHostIntrinsic(inst->type) || isProv) result = inst->props.children;
    else                                       result = inst->type(inst->props);
    inst->lastRenderResult = result;

    reconcileChildren(inst, asElementList(result));

    if (isProv) runtime.ctxStacks[pushedCid].pop();
    inst->dirty = false;
    runtime.currentInstance = null;
}

void reconcileChildren(Instance* parent, Element[] els) {
    bool keyed = false;
    for (e in els) if (e.key != null) { keyed = true; break; }
    if (keyed) reconcileChildrenKeyed(parent, els);
    else       reconcileChildrenUnkeyed(parent, els);
}

Instance* mountInstance(Instance* parent, Element el) {
    Instance* inst = alloc(Instance);
    inst->id      = runtime.nextInstanceId++;
    inst->parent  = parent;
    inst->type    = el.type;
    inst->props   = el.props;
    inst->key     = el.key;
    inst->mounted = false;
    renderInstance(inst);
    inst->mounted = true;
    return inst;
}

void unmountInstance(Instance* inst) {
    for (c in inst->children) unmountInstance(c);            // depth-first
    for (slot in inst->hookSlots) {
        if (slot.kind == HOOK_EFFECT || slot.kind == HOOK_LAYOUT_EFFECT) {
            if (slot.u.effect.cleanup) slot.u.effect.cleanup();
        }
    }
    if (inst->hostHandle) host.destroyNode(inst->hostHandle);
    free(inst);
}

// ---- Commit ----

void commit(Instance* root) {
    host.beginCommit();
    commitInstance(root);
    host.endCommit();
}

void commitInstance(Instance* inst) {
    if (!inst->hostHandle) inst->hostHandle = host.createNode(inst->type, inst->props);
    else                   host.updateNode(inst->hostHandle, inst->props);
    for (c in inst->children) {
        commitInstance(c);
        host.appendChild(inst->hostHandle, c->hostHandle);
    }
}

// ---- Effects ----

void flushLayoutEffects() {
    for (slot in collectAllPendingLayoutEffects(runtime.root)) {
        if (slot->u.effect.cleanup) slot->u.effect.cleanup();
        slot->u.effect.cleanup = slot->u.effect.pending();
        slot->u.effect.pending = null;
    }
}

void flushPassiveEffects() {
    for (slot in collectAllPendingPassiveEffects(runtime.root)) {
        if (slot->u.effect.cleanup) slot->u.effect.cleanup();
        slot->u.effect.cleanup = slot->u.effect.pending();
        slot->u.effect.pending = null;
    }
}

// ---- Scheduler ----

void markDirty(Instance* inst) {
    runtime.dirtySet.insert(inst);
    if (!runtime.renderScheduled) {
        runtime.renderScheduled = true;
        host.scheduleTick(performRenderPass);
    }
}

void performRenderPass() {
    runtime.renderScheduled = false;
    Set<Instance*> dirty = move(runtime.dirtySet);
    for (root in highestAncestorsIn(dirty)) renderInstance(root);
    commit(runtime.root);
    flushLayoutEffects();
    host.requestPresent();
    enqueueTask(flushPassiveEffects);
}

void mountRoot(Element rootEl) {
    runtime.root = mountInstance(null, rootEl);
    commit(runtime.root);
    flushLayoutEffects();
    host.requestPresent();
    enqueueTask(flushPassiveEffects);
}
```

That's the whole core, modulo error boundaries and memo. Roughly 200 lines once you fill in details. Production complexity is almost entirely in: the scheduler, the host-specific commit phase, devtools instrumentation, and edge cases (Strict Mode double-invocation, hydration, server rendering).

---

## 18. Pitfalls when implementing in a systems language

Implementing in C, C++, Rust, Zig surfaces issues that GC'd languages paper over.

**Memory ownership.** Instances are owned by the runtime (transitively from the root). Lifetime is mount → unmount; on unmount, instance and hook slots are freed *after* cleanups run. Hook slot storage lives on the instance — be careful with vectors-of-slots that reallocate on push (pointers invalidate). Either pre-size, use a stable-address container (linked list, indexable arena), or always look up slots via `(instanceId, slotIndex)` rather than cached pointers. Props are produced by the parent's render and consumed by the child's; two viable strategies are (a) copy into the child instance per render (simple, one copy per render) or (b) bump-allocate into a per-render arena that resets each pass (cheap allocation, but props don't survive past commit unless snapshotted).

**Closures.** Setters, dispatchers, effect bodies, cleanups, memo computes — each is a closure that must be heap- or arena-allocated with a lifetime covering its possible invocation window. The setter's lifetime is the instance's; an effect cleanup's lifetime is until the next effect run or unmount.

**Stable handles, not pointers.** A naive `setX` captures a pointer to the slot — broken the moment the slot vector reallocates. Capture `(instanceId, slotIndex)` and look up on call. This handle is stable for the entire instance lifetime. Same rule applies for any setter that survives across renders.

**Cleanup determinism.** No GC. On unmount: run all effect cleanups in a defined order (layout then passive, in reverse-mount order or hook-index order — pick one and document); free all slot allocations (closures, captured values); free child instances depth-first; free the instance. A missed cleanup is a leak that grows per mount/unmount. Worse, a leaked subscription firing on a freed instance is a use-after-free. Make sure subscription-like effects always return a cleanup, and that cleanups actually execute.

**Equality without `Object.is`.** JS gets away with one rule because everything is primitive or reference. In a systems language: trivial copyable types can use bitwise equality (watch `NaN` and `+0/-0` if you care); non-trivial types need a user-provided comparator or a deps wrapper that carries one; closures can only be compared by identity (heap pointer) — which is why `useCallback` exists. A pragmatic implementation parameterizes equality per dep slot: store a `cmp` alongside the deps, or require deps to be a fixed-shape tuple whose members have known comparators.

**Hook ordering enforcement.** In debug builds, record each hook's *kind* in order on first render; on subsequent renders assert the Nth call matches the Nth recorded kind. Also assert the total hook count matches at end of render. Ship debug-mode during development — silent corruption from a misplaced conditional hook is brutal otherwise.

**Render walk: recursion vs work stack.** Recursion is natural but imposes the host stack as a tree-depth limit. For deep trees, switch to an explicit work stack — push children, loop, pop until empty. This also positions you for interruptible rendering later.

**Re-entrant updates during render.** If a setter fires *during* render (typically via a synchronously-invoked subscription callback), detect that render is active, mark the target instance dirty, pick it up on the next pass. Don't try to interrupt the current render to handle the update. A setter for the *currently rendering* instance is also legal and is the standard "derived state during render" pattern: render the instance, check if a setter fired for it, re-render — bounded by a max iteration count to catch infinite loops.

---

## 19. What's worth implementing first

A prioritized v1 checklist. Each tier is useful before the next is added.

**Tier 1 — indispensable core.**

1. **Stable instance identity** by (parent, position-or-key, type). No keys yet — just position. Get a basic tree that re-renders with state preserved.
2. **Hook dispatcher.** `runtime.currentInstance` + `runtime.currentHookIndex`. Slot vector per instance.
3. **`useState`.** The minimum hook that justifies the architecture. Lazy init, functional updates, bail-out.
4. **`useEffect`** with dep arrays. Register during render, run after commit, cleanup before next or on unmount. Passive only at first; skip the layout/passive split.
5. **`useRef`.** Trivial, immediately useful.
6. **Render → commit → effects pipeline.** Fully synchronous. One pass per scheduled render.
7. **Dirty-marking + simple scheduler.** `markDirty`, next-tick schedule, coalesce. Render from highest common ancestor of the dirty set.

Stop here, ship, see what hurts.

**Tier 2 — once you feel the pain.** Keys + keyed reconciliation (as soon as a list reorders); `useContext` via descent stack; `useReducer`; `useLayoutEffect` as a separate queue (when you hit a measurement-driven flicker).

**Tier 3 — optimizations.** `useMemo` / `useCallback` (pure optimization — wait for profiling); `memo()` and prop-equality bailout; error boundaries.

**Tier 4 — probably never in v1.** Concurrent mode (interruptible rendering, work stack); priority lanes; Suspense and transitions; hydration / server rendering; Strict Mode double-invocation; refs-as-callbacks, `forwardRef`, imperative handles; fragments and portals (host-feature-dependent anyway).

The discipline of building in this order is the difference between a working framework in a weekend and getting stuck for months. Tier 1 contains almost all the value.

---

## Appendix: invariants

A flat list of the rules the whole system rests on. Violate one, stop and fix it — don't paper over.

- **Render is pure.** No host mutations, no effect bodies, no subscriptions firing. Only: read props, read hook slots, write hook slots (same instance), produce element descriptions.
- **Commit is atomic.** No partial host state visible between commits.
- **Effects run after commit.** Reads inside an effect always see post-commit state.
- **Cleanup before next effect.** Two runs of the same slot never overlap.
- **Hook order is fixed per instance.** The Nth call on render K is the same hook as on render K+1.
- **State lives on the instance.** Hook state survives renders iff the instance survives.
- **Instance identity = (parent, key ?? position, type).** Type change always remounts.
- **Setters are stable.** Same setter object every render.
- **Refs don't trigger renders.** Mutating `ref.current` is invisible to the dataflow graph.
- **Context value is positional.** A consumer reads the nearest provider ancestor's value at render time.
- **All cleanups run during unmount, in a defined order, even if one throws.**

Those eleven invariants are the contract. The rest is performance, ergonomics, and integration with whatever host lives downstream.
