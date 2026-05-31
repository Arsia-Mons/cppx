# UI Reference

A C++20 / SDL3 reference project showing how to build a retained game UI with a React-style hook runtime. The shooter sample doubles as a stress test: focus navigation, retained screens, post-layout writes, and a deterministic CLI for headless tests.

## Layout

```text
src/app/...        # process lifecycle and the per-frame loop
src/platform/...   # OS adapters: SDL window/input, control mailbox
src/renderer/...   # SDL retained render glue + font registry
src/ui/...         # generic retained UI toolkit: hook runtime (runtime/react.{h,cpp}), focus, primitives
src/client/ui/...  # client UI shell + the game's screens (providers, hooks, components, screens/*)
src/game/...       # game rules and state (player, weapons, economy, inventory)
third_party/       # stb_image
tests/             # gtest-free unit tests + python CLI smoke tests
tools/ui_cli.py    # headless control/capture driver used by CLI smoke tests
```

`architecture.md` is the canonical mental model — read it before proposing structural changes.

## Build & Test

```sh
./build.sh           # configure + build hello (macOS/Linux)
./build.sh --tests   # build everything + run ctest
./build.ps1          # configure + build hello (Windows)
./build.ps1 -Tests   # build everything + run ctest
```

The wrappers cache CMake configure, hold a build lock to prevent concurrent builds, and place artifacts in `cmake-build-debug/` by default. SDL3 and SDL3_ttf are fetched via CMake when not found locally; libcurl is required from the system.

Headless tests live in `tests/`. The Python smoke tests drive the binary through `tools/ui_cli.py` over a control directory and assert on captured BMPs — see `tests/ui_cli_smoke.py`.

## Conventions

- C++20, no exceptions in UI/runtime code, no RTTI assumed. Warnings are errors-of-attention (`-Wall -Wextra`, see `CMakeLists.txt:76`).
- React-style hooks are the composition primitive — read `src/ui/runtime/react.h` before writing a component. Use `REACT_COMPONENT_BEGIN` / `_KEY` and slot callbacks; never store `children` past the call.
- Stable hook IDs come from the hook runtime's parent-hash + sibling index. Use keyed component/node identity when reordering keyed siblings.
- Writes that mutate game state during layout must go through `client::ui::ClientUi::queue_deferred_mutation` — never mutate during the UI declaration pass.
- Game-specific UI lives in `src/client/ui` (shell + screens) and game rules in `src/game/`. Keep `src/ui/` free of game vocabulary.

## More

- `architecture.md` — boundaries between `ui/`, `client/`, `game/`, `server/`, `net/`. Source of truth for where new code goes.
- `docs/eve-window-system-functional-requirements.md` — window-system spec being designed against.
- `docs/eve-window-system-planning-process.md` — planning notes for the same.
- `src/ui/runtime/react.h` — full hook-runtime API reference (header comment is the docs).
- `.claude/skills/composition-patterns/` — Vercel's React composition-patterns skill (compound components, lifted state, explicit variants, no boolean-prop proliferation). This UI is C++ on SDL/retained nodes, not real React, but the hook runtime in `src/ui/runtime/react.h` is intentionally React-shaped — apply these patterns when designing components, hooks, and provider/context layouts to the extent the language allows. Mirrored into `.codex/skills/composition-patterns/` for Codex.
