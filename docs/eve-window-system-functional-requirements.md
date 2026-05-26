# EVE-Style Window System Functional Requirements

## Status

This is the active functional requirements document for adding an EVE-style
window system to the current React-style Clay UI.

Historical UI planning documents have been moved to `docs/archive/`.

## Objective

Add a retained in-game window system that supports dense, overlapping,
movable, resizable, persistent windows while remaining usable across viewport
and resolution changes.

The result should feel closer to a serious MMO cockpit than a single-screen
menu stack: many information and control surfaces may be open at once, each with
stable placement, stable local state, predictable focus, and compact controls.

## Architectural Constraints

FR-001. Window contents are declared as component roots.

FR-002. Window contents cross the game/UI boundary through hooks. Hooks return
values and functions at the point components need them.

FR-003. Local window scratch state, such as selected tab, filter text, sort
mode, scroll position, and temporary input, lives in hook state owned by the
window content component.

FR-004. Shared app or game data is exposed through narrow hooks. Components do
not receive broad state bundles just because they live inside a window.

FR-005. Window chrome behavior belongs to the window system. Content-specific
behavior belongs to the component rendered inside the window.

FR-006. The window system must coexist with the current retained UI screen
stack. It should not require replacing every existing screen with a floating
window before the first useful slice can ship.

## Window Identity And Lifecycle

FR-010. Each window has a stable identity, title, type, bounds, z-order,
visibility state, focus state, and optional persistence key.

FR-011. The system supports multiple simultaneous windows.

FR-012. A window type may opt into single-instance behavior. Opening an existing
single-instance window brings it to front instead of creating a duplicate.

FR-013. A window type may opt into multi-instance behavior. Multi-instance
windows receive distinct identities and preserve independent hook state.

FR-014. Windows can be opened, closed, minimized, restored, collapsed, pinned,
and brought to front.

FR-015. Closing a window unmounts its content and runs the normal hook cleanup
path.

FR-016. Minimizing a window hides its content without losing retained content
state unless that window type explicitly chooses to unload while minimized.

FR-017. Collapsing a window keeps the title/chrome visible and hides the content
area without changing z-order.

FR-018. Pinned windows remain visible but may opt out of ordinary auto-raise
behavior.

## Geometry And Layout

FR-020. Floating windows can overlap freely.

FR-021. Every window has minimum size constraints for chrome and required
content.

FR-022. A window may define preferred, minimum, and maximum content sizes.

FR-023. A window can be dragged by its title/header area.

FR-024. A window can be resized from edges and corners when resizing is enabled
for that window type.

FR-025. Drag and resize operations use pointer capture until release or
cancellation.

FR-026. The system supports snapping to viewport edges.

FR-027. The system supports snapping to nearby window edges.

FR-028. Window geometry is evaluated in UI coordinates after platform and
drawable-pixel scaling have been resolved.

FR-029. Hit testing distinguishes title drag zones, resize handles, window
controls, scroll regions, and content controls with deterministic priority.

## Z-Order And Focus

FR-040. Clicking or focusing a window brings it to the front unless the window
type or pinned state says otherwise.

FR-041. The active window has a visually distinct focused state.

FR-042. Inactive windows remain readable and usable, but their chrome clearly
indicates that they are not focused.

FR-043. Pointer hit testing selects the topmost eligible window at the pointer
position.

FR-044. Keyboard input is delivered to the focused window first.

FR-045. Text input inside a focused text field does not leak into global
shortcuts.

FR-046. Directional navigation moves inside the focused window before crossing
to another window.

FR-047. Modal windows trap focus until dismissed.

FR-048. A modal window appears above all non-modal windows that belong to the
same UI layer.

## Docking And Tab Groups

FR-060. Windows can dock into tab groups when both window types allow docking.

FR-061. A tab group is moved, minimized, restored, collapsed, and clamped as one
unit.

FR-062. Each tab preserves its own content hook state while docked.

FR-063. Tabs can be reordered within a tab group.

FR-064. A tab can detach from a tab group into its own floating window.

FR-065. Closing the active tab selects a predictable neighboring tab.

FR-066. Empty tab groups are removed automatically.

## Viewport And Resolution Changes

FR-080. Window placement must gracefully handle viewport size changes, drawable
pixel size changes, HiDPI scale changes, fullscreen toggles, and orientation-like
aspect ratio changes.

FR-081. Every visible window remains reachable after a viewport change.

FR-082. At least the window header and one resize affordance remain inside the
usable area after repair.

FR-083. The usable area may exclude reserved UI regions such as HUD bands,
safe-area insets, or platform overlays.

FR-084. When the viewport shrinks, windows are clamped into the usable area
instead of disappearing off-screen.

FR-085. If a window no longer fits, the system applies fallbacks in this order:
clamp position, reduce size within allowed constraints, enable content
scrolling, then use compact/collapsed presentation if the window supports it.

FR-086. Emergency repair may violate preferred size, but must not make the
window impossible to drag, close, or restore.

FR-087. Temporary resize repair does not permanently overwrite the user's saved
layout unless the user explicitly saves or changes the repaired layout.

FR-088. Restoring a larger viewport should restore the user's intended layout
when enough space is available.

FR-089. Docked tab groups are repaired as a single unit.

FR-090. Layout repair runs before the repaired frame is presented, so users do
not see windows briefly render off-screen after a resize event.

FR-091. Pointer capture during drag or resize survives viewport changes where
possible. If capture cannot continue safely, the operation is canceled cleanly
and the window remains reachable.

## Persistence

FR-110. Window layouts can be persisted per user profile, character, workspace,
or app mode.

FR-111. Persisted layout includes bounds, minimized/collapsed state, pinned
state, z-order, docked tab groups, and active tab.

FR-112. Persisted layout is versioned.

FR-113. Unknown window types in saved layout data are ignored safely.

FR-114. New window types receive sensible default placement when no saved layout
exists.

FR-115. A reset-layout command restores a known usable default.

FR-116. Persistence distinguishes canonical saved geometry from transient
viewport repair geometry.

## Window Content

FR-130. Window content receives a stable content area and can lay out normally
inside it.

FR-131. Content scrolling is independent from window dragging and resizing.

FR-132. Content can request close, minimize, restore, focus, resize-to-fit, or
bring-to-front behavior through hook-returned functions.

FR-133. Content can expose dirty/blocked state so the window system can confirm
or prevent destructive close requests.

FR-134. Content can render empty, loading, blocked, and error states without
breaking the outer window frame.

FR-135. Content can provide a compact presentation for small repaired windows.

## Visual Requirements

FR-150. Window chrome is compact and dense.

FR-151. Active, inactive, modal, pinned, minimized, collapsed, and blocked
states are visually distinct.

FR-152. Drag and resize affordances are discoverable without consuming excessive
content space.

FR-153. Window movement and resize feedback is immediate and stable.

FR-154. Window text and controls never overlap incoherently during normal or
repaired layouts.

FR-155. The system avoids jitter during drag, resize, docking, undocking,
viewport repair, and content reflow.

## Input Coverage

FR-170. Primary pointer support includes click, drag, resize, scroll, close,
minimize, restore, collapse, tab select, tab reorder, dock, and detach.

FR-171. Keyboard support includes focus traversal, close focused window,
minimize focused window, restore minimized window, and reset layout.

FR-172. Gamepad support includes focus traversal, confirm, cancel, tab change,
and basic window focus switching.

FR-173. Shortcut precedence is explicit: text input first, focused window next,
then global UI.

FR-174. Repeated key/button input does not duplicate a single confirm,
close, minimize, dock, or detach request within one frame.

## Automation And Verification

FR-190. Tests can inspect open windows, focused window, z-order, geometry,
visibility state, dock groups, active tabs, and repaired bounds.

FR-191. Tests can drive pointer, keyboard, gamepad-style focus, viewport resize,
and drawable-pixel resize through the same runtime input path used by the app.

FR-192. Tests cover open, close, minimize, restore, collapse, pin, focus,
z-order, drag, resize, snap, dock, detach, persistence, and reset layout.

FR-193. Resize tests cover large-to-small, small-to-large, ultrawide, narrow,
HiDPI, fullscreen toggle, and tiny viewport cases.

FR-194. Tests prove no visible window becomes unreachable after viewport repair.

FR-195. Visual proof includes real rendered screenshots for overlapping
windows, focused/inactive states, tab groups, tiny viewport repair, and restored
layouts.

FR-196. Any acceptance claim for viewport behavior includes both state
inspection and rendered-frame proof.

## MVP Scope

The first implementation slice should deliver:

- retained floating windows;
- open, close, minimize, restore, and bring-to-front;
- drag and resize;
- z-order and focused-window input routing;
- layout persistence with reset;
- viewport repair that keeps every visible window reachable;
- focused tests and rendered screenshots for the resize cases.

Docked tab groups, pinned low-chrome mode, gamepad window switching, and
advanced compact presentations can follow once the core floating window model is
stable.
