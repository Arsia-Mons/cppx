# EVE-Style Window System Planning Process

## Purpose

Use this process before implementing the EVE-style window system.

The goal is not to write a plausible plan once. The goal is to produce a
functional implementation plan and a separate hypothetical client proof that
stress-tests the plan hard enough to expose design flaws before source work
starts.

This process should be handed to an agent as the work contract for creating:

- `docs/eve-window-system-implementation-plan.md`
- `docs/eve-window-system-client-proof.md`

The implementation plan describes how the system will be built. The client proof
shows, in document form, that a complex real client could use the plan without
inventing a second UI architecture.

## Inputs

The agent must read these first:

- `AGENTS.md`
- `docs/eve-window-system-functional-requirements.md`
- `docs/archive/ui-focus-interaction-plan.md`
- `docs/archive/ui-focus-stress-test.md`
- current `src/` and `tests/` relevant to React-style hooks, Clay frame
  lifecycle, focus, retained UI lifetime, and input routing

The archived docs are context and precedent. They are not active requirements
unless a current functional requirement depends on the same invariant.

## Non-Negotiable Architecture Rules

The generated plan must preserve these rules:

- Window content uses component roots and hooks.
- Hooks return values and functions where components need them.
- Local UI scratch lives in hook state inside the component tree.
- Shared app/game data is exposed through narrow hooks.
- The plan does not introduce broad state bundles, route-table rendering,
  sibling-authored navigation graphs, or a parallel UI architecture.
- Clay owns layout and render commands. It does not own app state, window
  persistence, focus policy, or game behavior.
- The plan must include viewport and resolution repair as a first-class runtime
  responsibility, not a later visual polish task.

## Required Output 1: Implementation Plan

`docs/eve-window-system-implementation-plan.md` must include:

1. Objective and scope.
2. Current-code readback: what source files and runtime mechanics the plan is
   building on.
3. Ownership model for the window manager, retained window entries, z-order,
   persistence, drag/resize state, docking, and viewport repair.
4. Hook/provider API sketch for window contents and window chrome.
5. Frame order, including where input is captured, where layout is declared,
   where hit testing occurs, where resize repair runs, and where queued writes
   drain.
6. Data model for window identity, bounds, saved geometry, repaired geometry,
   tab groups, and modal state.
7. Input routing for pointer, keyboard, text input, and gamepad-style focus.
8. Viewport change algorithm, including temporary repair versus saved layout.
9. Persistence format and migration/versioning strategy.
10. Implementation phases with clear exit evidence.
11. Test plan covering state inspection and rendered-frame proof.
12. Anti-reward-hack gates that make it hard to pass by stubbing behavior,
    bypassing the runtime input path, or only checking in-memory state.
13. Known risks and open decisions.

Every phase must name the functional requirements it satisfies by requirement
id.

## Required Output 2: Client Proof

`docs/eve-window-system-client-proof.md` must describe a hypothetical complex
client implementation that uses the planned window system. This proof should be
specific enough that contradictions in the plan become obvious.

The proof must not be a marketing scenario. It must include component and hook
sketches, data flow, window operations, and failure cases.

The scenario should pressure the system with a dense MMO-style workspace, for
example:

- inventory window with filters, search, scroll position, and item selection;
- character fitting window with equipment slots and stat recalculation;
- market window with sortable tables, order entry, and confirmation modal;
- chat/comms window with text input and tabs;
- fleet or party window with live membership updates;
- map/probe window with resize-sensitive canvas-like content;
- notifications or combat log window with pinned/compact mode;
- modal confirmation and dirty-close dialogs;
- at least one docked tab group;
- at least one multi-instance window type;
- one tiny-viewport repair sequence and one restore-to-large sequence.

The client proof must include:

1. Workspace overview and user goals.
2. Window list, identity rules, and persistence keys.
3. Hook sketches for each major window.
4. Component sketches for representative content.
5. Interaction walkthroughs for opening, focusing, dragging, resizing,
   docking, minimizing, closing, modal confirmation, text input, and shortcut
   precedence.
6. Viewport stress walkthroughs for shrink, grow, fullscreen toggle, HiDPI
   change, and tiny viewport.
7. Explicit notes showing where local hook state is preserved or reset.
8. Expected inspection state and screenshot proof points.
9. A section named `Plan Flaws Found` that lists every contradiction,
   underspecified behavior, or missing requirement uncovered while writing the
   proof.
10. A section named `Plan Fixes Applied` that points back to the implementation
    plan changes made because of those flaws.

If the proof needs a concept that the implementation plan does not define, that
is a planning failure. The agent must update the plan or remove the unsupported
proof behavior.

## Iteration Loop

The agent must use this loop:

1. Draft or update the implementation plan from the functional requirements and
   current source.
2. Draft or update the client proof against that plan.
3. While writing the client proof, record every plan flaw immediately.
4. Update the implementation plan to fix those flaws.
5. Update the client proof so it uses the corrected plan.
6. Run a read-only review pass over both docs.
7. Fix every high-confidence finding.
8. Repeat until the reviewer returns no findings or only explicitly deferred
   product choices.

The implementation plan and client proof must converge together. A clean plan
with a stale proof is not acceptable. A rich proof that depends on undocumented
mechanics is not acceptable.

## Review Prompt

Use a review prompt with this shape:

```text
Review docs/eve-window-system-implementation-plan.md and
docs/eve-window-system-client-proof.md as a strict architecture reviewer.

Prioritize high-confidence structural findings:
- places where the client proof depends on behavior the plan does not define;
- places where the plan violates the React-style hook/component architecture;
- places where viewport repair, persistence, focus, or input routing can be
  reward-hacked;
- missing negative tests or rendered proof gates;
- ambiguous ownership that would let implementation drift into a second UI
  system.

Return findings with exact document sections and concrete remedies. Ignore
style nits.
```

The review must inspect both docs together, not one at a time.

## Exit Criteria

The planning process is complete only when:

- the implementation plan maps every MVP requirement to a phase and proof gate;
- the client proof exercises every MVP phase through a complex workspace;
- every flaw found by the proof is fixed in the implementation plan;
- every implementation-plan fix is reflected back in the proof;
- the review loop has no unresolved high-confidence findings;
- the docs include enough evidence requirements that an implementation cannot
  pass by stubbing state, bypassing real input, or skipping rendered proof.

## Handoff Requirements

When the agent finishes the planning process, it must report:

- paths of both produced docs;
- the number of proof-driven flaws found and fixed;
- the final review result;
- any intentionally deferred requirements by id;
- the first implementation phase that should be handed to a coding agent.

Do not start source implementation as part of this planning process unless the
user explicitly asks for implementation.
