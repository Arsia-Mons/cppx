# Game UI Navigation: The Screen/Scene Stack

A reference for building modern game UI navigation around an explicit stack of
screens (a.k.a. scenes, states, views, panels) with `push`/`pop` semantics.
Framework-agnostic. Opinionated where it helps.

---

## 1. The Core Model

A **screen stack** is a LIFO collection of UI "screens" where the top element
owns the user's attention. Navigation is expressed as mutations of that stack:
you `push` a new screen on top, you `pop` to go back, you `replace` the top
when a back-step makes no sense.

A "screen" here is a self-contained chunk of UI with its own state, lifecycle,
and input handling: a main menu, a settings panel, a pause menu, a confirm
dialog, a loading screen, the gameplay HUD container, etc. It's the unit of
navigation, not the unit of rendering.

### Why games converged on this

Early games shipped a flat state machine:

```
enum AppState { MainMenu, Game, Settings, Credits, ... };
AppState current;
```

This works until you hit any of:

- **Overlays.** A pause menu over running gameplay. A confirm-quit dialog over
  the settings screen. The thing underneath isn't "gone," it's *covered*.
- **Modal dialogs.** "Save before quitting?" — needs to suspend whatever was
  beneath it without that screen forgetting its state.
- **Nested drill-downs.** Main Menu → Settings → Audio → Output Device. Each
  step needs an unambiguous "back."
- **Reentry.** The same Settings screen reached from Main Menu vs. from the
  pause menu should return to wherever it was opened from, not a hardcoded
  destination.

A flat router has to encode all of that as transitions in a single global
variable plus side tables ("when I leave Settings, where do I go back to?").
That side table *is* a stack — you may as well make it explicit.

### Difference from a flat router

| | Flat `currentScreen = X` | Screen stack |
|---|---|---|
| Back behavior | Hardcoded per screen | Implicit: pop |
| Overlays | Impossible or hacked | First-class |
| Screen state on exit | Lost on switch | Preserved while covered |
| Reentry | Forgets context | Returns to caller |
| Modality | Manual flags | Natural via stack depth |

The stack is just an honest data structure for what the UI already is.

### Comparison points

- **Unity** has `SceneManager` (heavyweight, more like `replace`) plus the
  `UI Toolkit` / uGUI `Canvas` ordering (lighter, more like overlays). Most
  Unity games end up writing a `UIManager` that is exactly a screen stack on
  top of canvases.
- **Unreal** ships UMG widgets and `PushWidget`/`PopWidget` patterns via
  `CommonUI`'s `CommonActivatableWidgetStack` — explicit, typed, with input
  routing built in.
- **Godot** has a scene tree, not a stack, but `push_scene`-style helpers and
  the community pattern of a "screen manager" autoload reimplement the stack.
- **Web** browsers' history API is the same model with serialization.

---

## 2. Primitive Operations

A minimal but complete API:

```cpp
stack.push(screen);          // open a new screen on top
stack.pop();                 // close the top screen, reveal what was beneath
stack.replace(screen);       // pop top, push new — one logical step
stack.popTo(predicate);      // pop until predicate(top) is true
stack.popToRoot();           // pop until exactly one screen remains
stack.clear();               // remove everything (usually before pushing a root)
stack.peek();                // inspect top without mutating
```

### Semantics, one by one

**`push(screen)`**
Adds `screen` to the top. The previously-top screen is *paused* (not destroyed).
Use when "back" should return to where you were: Main Menu → Settings,
Game → Pause, anything modal.

**`pop()`**
Removes the top, runs its exit lifecycle, resumes the new top. The popped
screen is typically destroyed (or returned to a pool). Use for any "back."

**`replace(screen)`**
Atomically pops the top and pushes a new screen. The two operations should be
indistinguishable from a single transition to outside observers (no flicker of
the screen below). Use when leaving the previous screen on the stack would be
nonsensical:

- Main Menu → Game: you don't want Escape from gameplay to drop you back to
  the menu *under* the game.
- Loading Screen → Game: the loading screen has done its job; it shouldn't be
  recoverable.
- Splash → Main Menu.

A good heuristic: if pressing Back from the new screen should *not* return to
the screen you came from, use `replace`, not `push`.

**`popTo(predicate)`**
Pops repeatedly until `predicate(top)` is true. Common form: `popTo<MainMenu>()`
after the player clicks "Quit to Menu" from a nested pause/settings flow.
Implementations usually accept a screen type, a tag, or a predicate.

**`popToRoot()`**
Convenience for `popTo(it == bottom)`. Useful for "Home" buttons and panic
exits.

**`clear()`**
Wipes the stack. Almost always paired with an immediate `push` of a new root.
Be wary: every screen popped this way still needs proper lifecycle teardown.

**`peek()` / `top()`**
Non-mutating access to the active screen. Used by the main loop for input and
update routing. Should be safe to call on an empty stack (returns null /
optional).

### Why `replace` is not just `pop + push`

Two reasons:

1. **Lifecycle ordering.** A naive `pop` then `push` calls `onExit` on the
   outgoing screen, then briefly the screen *below* it becomes the top and
   gets `onResume`, then the new screen pushes and the below screen gets
   `onPause` again. That's a spurious resume/pause cycle visible to game code
   (audio unducks then re-ducks, music swaps twice, etc.).
2. **Transitions.** `replace` is one animation (cross-fade, slide-over). `pop`
   then `push` is two animations chained, which looks wrong.

`replace` should be a primitive, not a helper.

---

## 3. Screen Lifecycle

Each screen has a small set of hooks. The exact names vary; what matters is the
*set of distinguishable events*.

```cpp
class Screen {
public:
    virtual void onEnter() {}    // pushed onto stack, first time visible
    virtual void onExit() {}     // popped from stack, about to be destroyed
    virtual void onPause() {}    // something was pushed on top of me
    virtual void onResume() {}   // the thing above me was popped
    virtual void update(float dt) {}
    virtual void render() {}
    virtual bool handleInput(const InputEvent&) { return false; }
};
```

### The four states

```
              push                push (on top of me)
   (none) ----------> Active ----------------------> Paused
                       ^  |                            |
                  pop  |  | pop                        | pop (the one above me)
                       |  v                            |
                     (none)  <----------------------- Active
                                  onResume
```

Transitions trigger hooks:

- `(none) -> Active`:           `onEnter`
- `Active -> Paused`:           `onPause`
- `Paused -> Active`:           `onResume`
- `Active -> (none)`:           `onExit`
- `Paused -> (none)` (via `popTo`/`clear`): `onResume` then `onExit`, or just
  `onExit` — pick one and be consistent. Most engines skip the spurious
  `onResume` and document it.

### Why pause/resume is the killer feature

In a flat router, leaving the gameplay screen for the pause menu either:

(a) destroys gameplay state (unacceptable), or
(b) requires gameplay to keep running invisibly (input bugs, audio bugs), or
(c) requires a manual "pausedScreen" side channel (reinventing the stack).

With a stack, pushing the pause menu naturally fires `Game.onPause()`. The
game stops its simulation, ducks its audio, freezes its input — and it does
all of this from a hook that is *symmetric* with `onResume`. The pause screen
gets popped; gameplay's `onResume` runs; everything restored.

### What goes in which hook

| Hook | Typical work |
|---|---|
| `onEnter` | Allocate resources, subscribe to events, request focus, start enter animation. |
| `onExit` | Free resources, unsubscribe, cancel async work, run exit animation. |
| `onPause` | Pause timers, stop simulation, release input focus, duck audio, *not* free resources. |
| `onResume` | Resume timers, re-acquire input focus, undo audio ducking. |

`onEnter`/`onExit` are *resource* hooks. `onPause`/`onResume` are *activity*
hooks. Don't conflate them.

### Async enter

Real screens often have async setup (load textures, fetch lobby list).
Convention: `onEnter` kicks off the work, the screen renders a local "loading"
state until ready, and only marks itself ready when finished. The stack
doesn't need to know.

Alternative: an async-aware stack that won't show the new screen until its
`onEnter` future resolves. This is cleaner but couples the stack to the async
model. Usually not worth it.

---

## 4. Update & Render Semantics

### Render order: bottom-up

```
for (Screen* s : stack) {           // bottom to top
    s->render();
}
```

This is what makes overlays work. The paused gameplay renders first; the pause
menu renders on top of it, dimmed background and all. The confirm dialog
renders on top of the pause menu. The player sees the world dimming through
the layers, which is exactly what they expect.

Some screens want to *opaquely* cover everything beneath (a loading screen
after a level transition). Two ways to handle this:

1. **`isOpaque` flag.** The renderer skips screens below the topmost opaque
   one. Cheap, common.
2. **Trust the screen.** It paints a full-bleed background and you pay the
   overdraw. Fine for menu-heavy games.

### Input: top-only, with bubbling option

```
if (auto top = stack.peek()) {
    bool handled = top->handleInput(event);
    if (!handled && top->propagatesInput) {
        // walk down the stack until handled, optional
    }
}
```

Default: input goes to the top screen and stops. This is what users expect —
clicking through a modal should be impossible. Bubbling is opt-in per screen
or per event type (e.g. global shortcuts like screenshot or push-to-talk that
should always work).

The Escape / B-button is *usually* handled by the stack itself, not by a
screen, as a default-pop (see section 7).

### Update: it depends

Three reasonable policies, none universally correct:

1. **Only the top updates.** Simplest. Wrong for gameplay-under-pause if the
   gameplay needs to keep animating ambient stuff. Right for menu-only stacks.
2. **All update, screens self-pause.** Every screen's `update` runs every
   frame; paused screens internally do nothing (or only ambient work). This
   is what most engines actually do.
3. **Top updates, others get a per-screen `updateWhenCovered` flag.** Explicit.
   The HUD/Game screen sets the flag if it wants ambient updates (idle
   animations, background music crossfades) while paused.

Pick (3) if you can stomach the extra flag. It makes the policy local to the
screen that cares.

### Modal vs. non-modal overlays

- **Modal overlay**: dims/blocks input to everything below. Confirm dialogs,
  pause menu. Implemented as a normal pushed screen with an opaque-ish
  backdrop and `propagatesInput = false`.
- **Non-modal overlay**: visually on top but lets input pass to a sibling.
  Toast notifications, tutorial highlights, picture-in-picture chat. These
  are *usually not* stack screens — they're better modeled as a separate
  "overlay layer" (HUD-like) that the renderer composites on top of the stack
  output. Putting them on the stack confuses Back-button semantics.

The rule of thumb: **if Back should close it, it's on the stack. If Back
should ignore it, it isn't.**

---

## 5. Transitions

Transitions are the most underestimated source of bugs in screen stacks.

### The lifetime problem

When you `pop` a screen, it can't disappear immediately if it has an exit
animation. It needs to keep rendering until the animation finishes. So:

```
logical stack:  [A, B]
user pops B
logical stack:  [A]
render stack:   [A, B-exiting]   for the next N frames
```

The stack needs a notion of "screens still rendering but no longer logically
present." Implementations vary:

- A separate `exiting` list rendered after the logical stack.
- A flag on the screen plus keeping it in the render-only list.
- A coroutine/task per screen that the stack awaits before destroying.

Whichever you pick, **input to an exiting screen must be disabled**, and
**the logical top is the new top immediately** — its `onResume`/`onEnter`
fires when the pop is requested, not when the animation finishes. Otherwise
you get input dead-zones during transitions.

### The transition lock

Players will mash buttons. Without a lock you'll see:

- Two `push(Settings)` in the same frame → two Settings screens stacked.
- `push` immediately followed by `pop` while the push animation is still
  playing → screens animating in and out simultaneously, inconsistent state.

Solution: a global `isTransitioning` flag on the stack. While true, navigation
requests are either queued or dropped. Drop-policy is usually fine and
simpler; queue-policy feels more responsive but multiplies edge cases.

A subtler variant: per-screen "navigation-arming" — a newly entered screen
ignores input for the first N ms to prevent stale taps from triggering its
buttons. UX research backs this; players have already moved on.

### Coordinated push transitions

Push animations often need *both* the outgoing and incoming screen to animate
simultaneously (the old slides left, the new slides in from the right). The
stack should drive both:

```
beginPush(newScreen):
    oldTop.startPauseTransition()
    newScreen.startEnterTransition()
    awaitBoth()
    oldTop.onPause()
    newScreen.onEnter()   # or fire onEnter at start; pick a convention
```

There's a real choice about *when* `onEnter` fires: at the start of the
transition (resources allocated early, but you might cancel) or at the end
(simpler, but no chance to use the resource during the animation). Most
codebases fire `onEnter` at the start and accept the cancel-cleanup work.

### Common transition types

- **Fade**: cheap, universal, slightly boring. Good default.
- **Slide**: directional, communicates hierarchy ("forward" vs "back"). Pair
  with push direction.
- **Scale/zoom**: useful for "drilling into" something (inventory item → item
  details).
- **Cross-dissolve with shared elements**: a tile in a menu morphs into the
  header of the next screen. Looks great; expensive to implement; needs the
  stack to expose a "shared element" channel between outgoing and incoming
  screens.
- **None**: instant. Always offer this for accessibility and for fast
  development.

---

## 6. Data Flow Between Screens

The stack is also a useful place to make screen-to-screen data flow *typed
and local*, instead of routed through globals.

### Params on push

```cpp
stack.push<LobbyScreen>({ .serverId = "abc123", .asSpectator = false });
```

The screen receives a typed config in its constructor or `onEnter`. This is
the right place for:

- IDs of things to display (item to inspect, server to join).
- Mode flags (read-only vs editable).
- Caller-supplied callbacks (rare; prefer results, see below).

Don't use params for global state — they're for *this instance of this
screen*.

### Results on pop

A file picker should *return* the picked path. A confirm dialog should return
yes/no. A character creator should return the created character.

Two patterns:

**(a) Callback on push:**

```cpp
stack.push<FilePicker>({ .startDir = "/home" },
    [](std::optional<Path> result) {
        if (result) loadFile(*result);
    });
```

Pros: locality (the consumer of the result is right there). Cons: lambda
captures, lifetime concerns if the pusher itself gets popped.

**(b) Typed result on a promise/future:**

```cpp
auto future = stack.pushForResult<FilePicker>({ .startDir = "/home" });
// later, in the pusher's update:
if (future.ready()) handle(future.get());
```

Pros: composes with async code, no captures. Cons: more machinery.

**(c) The screen below polls the popped screen:**

Discouraged. Couples screens directly, defeats the point.

### The typed-result pattern

A polymorphic `Screen` base can't really expose a typed result without
templates or `std::any`. A common shape:

```cpp
template <typename Result>
class ScreenT : public Screen {
public:
    using ResultType = Result;
protected:
    void setResult(Result r) { result_ = std::move(r); }
private:
    std::optional<Result> result_;
    friend class ScreenStack;
};

class ConfirmDialog : public ScreenT<bool> { /* ... */ };

bool ok = co_await stack.pushAsync<ConfirmDialog>({.text = "Quit?"});
```

The stack's `pushAsync` returns an awaitable / future of `Result`. The screen
calls `setResult(...)` somewhere before `pop`-ing itself.

If you don't want templates, use a `Result` variant or a typed channel
per-screen. The principle is: **results are explicit, typed, and flow back
through the stack — not through a global "lastFilePickerPath" string.**

### Contrast with global state

Global state (a `GameSession` singleton, a settings store) is fine for things
that genuinely outlive screens: the current save game, the audio volume, the
logged-in user. It is wrong for transient screen-to-screen data ("the item
the player clicked on the previous screen"). The smell: if changing the
screen flow would require renaming or partitioning a global, it should have
been a param.

---

## 7. Back-Stack and Input

The "back" gesture is so ubiquitous it deserves first-class treatment.

### Mapping

- **Keyboard**: Escape.
- **Gamepad**: B (Xbox), Circle (PlayStation), East button.
- **Mouse**: middle-click-back, browser-back if relevant.
- **Touch**: edge-swipe or hardware back (Android).
- **Web**: browser back button — wire it to `pop` and push history entries on
  `push`.

The stack itself should handle Back by default, *after* giving the top screen
a chance to intercept:

```cpp
void onBack() {
    if (auto top = stack.peek(); top && top->handleBack())
        return;                       // screen consumed it
    if (stack.size() > 1)
        stack.pop();
    else
        requestQuitConfirm();         // or ignore
}
```

### Guarding the root

`pop` on the last screen is a bug waiting to happen. Either:

- The stack refuses (returns false / asserts).
- The stack delegates to an `onLastPop` hook that triggers app quit confirm.

Pick the second; it makes the "quit the game" flow a screen like any other.

### Confirmation dialogs

The idiom: **a confirm dialog is itself a screen pushed on top.** On Yes, it
pops itself *and* the screen below (the one that asked for confirmation).
On No, it pops only itself.

```cpp
stack.push<ConfirmDialog>({ .text = "Quit to menu?" },
    [&](bool yes) {
        if (yes) {
            stack.pop();              // close dialog
            stack.popTo<MainMenu>();  // unwind gameplay
        }
        // on No, the dialog popped itself before the callback ran
    });
```

This means dialogs need a small API beyond `pop` — typically `popWithResult`
that both removes the dialog and surfaces its answer.

### Don't trap the user

Some screens (legal acceptance, mandatory tutorials, "you must name your
character") legitimately need to disable Back. Implement this by having the
screen return `true` from `handleBack()` to consume the event. **Make this
visible** — if Back does nothing, the player must understand why (greyed-out
back chevron, an explanatory tooltip, etc.). Silent Back-eating is a top-tier
UX bug.

---

## 8. Common Patterns

### Pause menu

```
stack: [Game]
player presses Escape
stack: [Game, PauseMenu]
Game.onPause()  -> sim stops, audio ducks
PauseMenu.onEnter() -> render dimmed backdrop + buttons
```

Game keeps its state. Pop the pause menu and you're exactly where you were.
Don't reinvent this by storing `GameState` in a side variable.

### Nested settings

```
stack: [MainMenu]
push Settings        -> [MainMenu, Settings]
push AudioSettings   -> [MainMenu, Settings, AudioSettings]
pop                  -> [MainMenu, Settings]
pop                  -> [MainMenu]
```

Each Back step is one pop. No bookkeeping. If Settings is reached from
*either* MainMenu or PauseMenu, the same screen works — back goes wherever
you came from, because that's literally what the stack remembers.

### Confirm dialogs as modals

See section 7. Note that the dialog screen pattern composes: a confirm
dialog can itself push a sub-dialog ("are you really sure?") with no special
plumbing.

### Loading screens as `replace` targets

```
stack: [MainMenu]
player clicks "Play"
replace with LoadingScreen   -> [LoadingScreen]
loading completes
replace with Game            -> [Game]
```

Two `replace` calls, no `pop`s. Back from the Game does not return to the
LoadingScreen (which would be nonsense) and does not return to the MainMenu
mid-game (which would also be nonsense — it goes through a confirm-quit
dialog and then `popToRoot` to MainMenu).

### HUD that "is" the game screen

The in-game HUD usually isn't its own screen. It's part of the `Game`
screen's render. Treating HUD as a stacked overlay is a common over-engineer:
the HUD doesn't have its own back semantics, doesn't pause anything, and
doesn't receive exclusive input. Put it in the Game screen and stop.

(Sub-HUD pieces with their own modality — the inventory panel, the map
overlay — *can* be stack screens if they take input focus. If they're
purely informational, they're part of the HUD.)

### Tutorial highlight overlay

A non-modal overlay (see section 4). Not on the stack. Lives in an overlay
layer that the renderer composites after the stack. Has its own controller
that listens to the stack to know what screen is active.

### Tabbed UI

Don't use the stack for tabs. Tabs are sibling views within one screen.
Pushing each tab onto the stack means Back goes "tab → tab" which is wrong.
Build a tab container as one screen.

---

## 9. Anti-patterns and Pitfalls

### Leaking screens

You `push` from inside a button handler, the screen renders, the player
clicks a button you forgot to wire up, and now there's no path to `pop`.
The stack grows on every reopening. Eventually input feels weird because
some old screen is intercepting it.

Mitigations:

- Every screen must have at least one exit path that doesn't rely on user
  cleverness (a visible Back button, even if Escape also works).
- A debug HUD listing the stack depth and screens; alert on depth > N.
- Unit-test navigation flows where possible (it's surprisingly tractable for
  a deterministic stack).

### Popping during iteration

```cpp
for (Screen* s : stack) {
    s->update(dt);     // s might call stack.pop() — boom
}
```

Two fixes:

- Defer all stack mutations: navigation calls enqueue commands; the stack
  applies them at a safe point (end of frame, top of next frame). This is
  the right answer.
- Snapshot the stack before iterating and ignore mutations until the next
  frame. Acceptable; can cause one-frame visual glitches.

Deferred mutation also fixes "a screen pops itself during its own `update`"
and "two screens pop in the same frame and the second pop sees the wrong
top."

### Stale references

```cpp
auto* settings = stack.push<Settings>();
// ... later ...
stack.pop();
settings->doThing();   // use-after-free
```

Don't hand out raw pointers from `push`. Either return an opaque handle
(integer ID + generation counter), a weak reference, or — best — just don't
expose the screen instance at all. Communication goes through params and
results.

### Deep stacks that should have been `replace`

If `popToRoot` is your most-used operation, your stack is too deep. Each
"flow" (login → character select → server list → lobby → game) is a chain of
`replace`s, not `push`es. The stack should hold roughly one screen per
"context the user might want to back out to," not one per UI screen visited.

### Coupling screens to each other

```cpp
class MainMenu {
    void onPlay() {
        auto game = new GameScreen();
        game->onFinished = [this]() { this->showResults(); };
        stack.push(game);
    }
};
```

`MainMenu` now knows the lifecycle of `GameScreen`. Worse, if the player
navigates `MainMenu → Game → quit → MainMenu (a new instance) → ...` the
callback may point at a dead `MainMenu`.

Route through the stack: `GameScreen` pops with a result, the stack hands the
result to whoever pushed it (which may itself be gone — that's fine, the
result is dropped). Screens don't reference each other; they reference the
stack.

### Overusing modality

If every dialog dims the world behind it and locks input, players feel
trapped. Reserve modality for actions with consequence (overwrite save,
quit unsaved). Use non-modal toasts/inline messages for everything else.

### Treating transitions as decoration

Transitions are *part of the stack's correctness model*. A push that visually
animates over 300 ms but logically lands instantly creates a window where the
new screen is interactive but not fully visible — players tap blind. Either
the transition gates interactivity (preferred) or it's instant.

---

## 10. Sketch of an Implementation

Generic, no framework dependencies. Pseudocode-leaning C++.

```cpp
// ---------------- Screen base ----------------

class Screen {
public:
    virtual ~Screen() = default;

    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void onPause() {}
    virtual void onResume() {}

    virtual void update(float dt) {}
    virtual void render() {}
    virtual bool handleInput(const InputEvent&) { return false; }
    virtual bool handleBack() { return false; } // true = consumed

    // Policy flags
    bool isOpaque = false;            // skip rendering screens below
    bool updateWhenCovered = false;   // tick even when not top
    bool propagatesInput = false;     // let input fall to screen below
};

// ---------------- Navigation commands ----------------

struct NavCommand {
    enum Kind { Push, Pop, Replace, PopTo, PopToRoot, Clear };
    Kind kind;
    std::unique_ptr<Screen> screen;             // for Push/Replace
    std::function<bool(Screen*)> predicate;     // for PopTo
};

// ---------------- The stack ----------------

class ScreenStack {
public:
    void push(std::unique_ptr<Screen> s)    { enqueue({NavCommand::Push,     std::move(s), {}}); }
    void pop()                              { enqueue({NavCommand::Pop,      nullptr, {}}); }
    void replace(std::unique_ptr<Screen> s) { enqueue({NavCommand::Replace,  std::move(s), {}}); }
    void popTo(std::function<bool(Screen*)> p) { enqueue({NavCommand::PopTo, nullptr, std::move(p)}); }
    void popToRoot()                        { enqueue({NavCommand::PopToRoot, nullptr, {}}); }
    void clear()                            { enqueue({NavCommand::Clear,    nullptr, {}}); }

    Screen* peek() const { return screens_.empty() ? nullptr : screens_.back().get(); }
    size_t  size() const { return screens_.size(); }

    void tick(float dt, std::span<const InputEvent> input) {
        // 1. Input: top only (with optional bubbling)
        if (!transitioning_) {
            for (auto& ev : input) {
                Screen* top = peek();
                if (!top) break;
                bool handled = top->handleInput(ev);
                if (!handled && top->propagatesInput) {
                    for (auto it = screens_.rbegin() + 1; it != screens_.rend(); ++it) {
                        if ((*it)->handleInput(ev)) { handled = true; break; }
                    }
                }
                if (!handled && ev.isBack()) {
                    if (top->handleBack()) { /* consumed */ }
                    else if (screens_.size() > 1) pop();
                    else onLastPop();
                }
            }
        }

        // 2. Update: top, plus any that opted in
        for (size_t i = 0; i < screens_.size(); ++i) {
            bool isTop = (i + 1 == screens_.size());
            if (isTop || screens_[i]->updateWhenCovered)
                screens_[i]->update(dt);
        }

        // 3. Apply queued nav commands (safe point)
        applyPendingCommands();
    }

    void renderAll() {
        // Find the topmost opaque screen; render from there up.
        size_t start = 0;
        for (size_t i = screens_.size(); i-- > 0; ) {
            if (screens_[i]->isOpaque) { start = i; break; }
        }
        for (size_t i = start; i < screens_.size(); ++i)
            screens_[i]->render();

        // Render exiting screens last (or interleaved, depending on transition).
        for (auto& e : exiting_) e->render();
    }

private:
    std::vector<std::unique_ptr<Screen>> screens_;
    std::vector<std::unique_ptr<Screen>> exiting_;
    std::deque<NavCommand> pending_;
    bool transitioning_ = false;

    void enqueue(NavCommand c) { pending_.push_back(std::move(c)); }

    void applyPendingCommands() {
        while (!pending_.empty()) {
            auto c = std::move(pending_.front());
            pending_.pop_front();
            switch (c.kind) {
                case NavCommand::Push:      doPush(std::move(c.screen)); break;
                case NavCommand::Pop:       doPop(); break;
                case NavCommand::Replace:   doReplace(std::move(c.screen)); break;
                case NavCommand::PopTo:     doPopTo(c.predicate); break;
                case NavCommand::PopToRoot: while (screens_.size() > 1) doPop(); break;
                case NavCommand::Clear:     while (!screens_.empty()) doPop(); break;
            }
        }
    }

    void doPush(std::unique_ptr<Screen> s) {
        if (!screens_.empty()) screens_.back()->onPause();
        screens_.push_back(std::move(s));
        screens_.back()->onEnter();
    }

    void doPop() {
        if (screens_.empty()) return;
        auto leaving = std::move(screens_.back());
        screens_.pop_back();
        leaving->onExit();
        // (Move to exiting_ if there's an exit animation; destroy when done.)
        if (!screens_.empty()) screens_.back()->onResume();
    }

    void doReplace(std::unique_ptr<Screen> s) {
        // Don't fire onResume on the screen below; that's the whole point.
        if (!screens_.empty()) {
            auto leaving = std::move(screens_.back());
            screens_.pop_back();
            leaving->onExit();
        }
        // The screen below stays paused.
        screens_.push_back(std::move(s));
        screens_.back()->onEnter();
    }

    void doPopTo(const std::function<bool(Screen*)>& p) {
        while (screens_.size() > 1 && !p(screens_.back().get()))
            doPop();
    }

    void onLastPop() { /* trigger quit-confirm flow, app-specific */ }
};
```

### Main-loop integration

```cpp
ScreenStack stack;
stack.push(std::make_unique<MainMenu>());

while (running) {
    auto input = pollInput();
    float dt = clock.tick();
    stack.tick(dt, input);
    beginFrame();
    stack.renderAll();
    endFrame();
}
```

Things deliberately omitted from the sketch (you'll want them eventually):

- Per-screen enter/exit animations with proper await semantics.
- A typed-result `pushAsync<T>` returning a future/coroutine.
- A `transitioning_` flag flipped during animations.
- Pooling for frequently pushed/popped screens (e.g. tooltips treated as
  micro-screens).
- Telemetry hooks: log every navigation event for replaying bug reports.

But the bones are the bones. A working stack is ~200 lines.

---

## 11. When the Stack Model Is the Wrong Fit

The stack is great for menu-driven UIs and games where the player drills in
and out of contexts. It's the wrong shape for several common cases:

### Pure HUDs

A heads-up display has no concept of "back." It's a set of widgets layered on
the world. Build it as a flat composition (or an ECS — see below), not a
stack. The stack might own *one* screen called `GameplayHUD` and that screen
internally manages its widgets.

### ECS-driven UI

If your game already runs on an entity-component system, UI panels can be
entities with `UIComponent`, `VisibilityComponent`, `InputFocusComponent`,
etc. Visibility and focus are queries, not stack positions. This scales
better for HUD-heavy games (RTS, MMOs) where dozens of panels coexist with
independent visibility.

You can still combine: the stack manages *full-screen modal flows*, the ECS
manages *in-world overlays*. They don't conflict.

### Single-screen tools

Map editor, single-purpose utility, jam-game one-screener. The complexity of
a stack buys you nothing. A flat `currentMode` enum is fine. Don't
pre-engineer.

### Strongly graph-shaped navigation

Open-world games where you can warp between hub screens in arbitrary order
(map → quest log → inventory → map) sometimes feel more graph than stack.
Usually you can still model it as a stack with `replace`s between hubs and
`push`es into drill-downs — but if your Back behavior genuinely needs to
remember a graph traversal (not just a path), you want a navigation history
list, not a LIFO stack.

### Web-shaped UIs

If the UI is fundamentally URL-routable (a companion app, an in-game web
view), use a router, not a stack. The router is a stack in disguise but with
serialization and deep-linking baked in, which you'll need.

---

## TL;DR

- The stack is an honest data structure for what UI navigation already is.
- `push`, `pop`, `replace` are the three you'll use 95% of the time.
- `replace` is not `pop + push`. Make it a primitive.
- Lifecycle has *four* states: entering, active, paused, exiting. Hooks for
  each. Don't conflate enter/exit with pause/resume.
- Render bottom-up. Route input to the top. Decide your update policy
  explicitly.
- Transitions are part of correctness; gate input on them.
- Pass typed params on `push`, return typed results on `pop`. Avoid globals.
- Defer all stack mutations to a safe point in the frame.
- Back maps to `pop` by default; let the top screen intercept first.
- If `popToRoot` is your busiest op, your stack is too deep — use `replace`.
- HUDs aren't screens. Tabs aren't screens. Toasts aren't screens.
- Don't reach for a stack when a flat enum or an ECS would do.
