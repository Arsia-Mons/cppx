# UI focus stress test: Loadout screen

This is a deliberately complex client screen written *as if* every primitive in
`ui-focus-interaction-plan.md` (`<FocusScope>`, `use_focusable`, `<Button>`, `<Toggle>`,
`<Selectable>`, `derive_visual_state`, the new `InputState` nav fields) were
already shipped. The goal is not to be runnable; it is to be the kind of code
an app developer would actually write at the end of F5 and then *use the
result to grade the API* — what answered cleanly, what required guessing.

---

## 1. What this screen is

A character **Loadout** screen with:

- A header row of **three tabs** (`Weapons` / `Armor` / `Consumables`) that
  swap the inventory panel's contents.
- A **scrollable 6×N inventory grid** on the left (every cell is focusable
  and tooltip-bearing).
- An **equipment panel** on the right with 4 slot buttons (head/chest/main/off);
  pressing confirm on a slot opens a sub-panel of compatible items.
- A **stat readout** below the equipment panel that updates live as focus
  moves over inventory cells (preview vs. equipped).
- A **focus-following tooltip** that paints next to the targeted cell only when
  `focusVisible` is true (no flicker on mouse drift).
- A **confirm-equip modal** that traps focus until Yes / No fires.
- A **dirty-changes guard** dialog on Cancel/Esc that asks
  "Discard changes?" before popping the screen.
- A **disabled state** on cells whose level requirement isn't met.
- A **dynamic focusable**: when the player has a brand-new item, an
  animated "NEW" badge cell mounts into the grid on frame N+1 and we want
  initial focus to land there.

Three input devices in parallel: keyboard arrows, gamepad d-pad/stick + A/B,
mouse hover/click. No per-device branches in any screen-level code.

---

## 2. Why I picked it (which parts of the design it exercises)

| Design surface (plan §) | Where this screen forces it |
|---|---|
| `<FocusScope>` per screen (§F2) | The whole Loadout is one scope; the modal pushes a second nested scope. |
| `use_initial_focus(predicate)` (§F2) | The "NEW" badge wants to be initial focus on its first frame alive. |
| `nav_*` traversal + `use_focus_traversal(strategy)` (§F2 scope bullet) | The 6-wide grid needs 2D left/right/up/down, not the default vertical list. |
| `confirm_pressed` vs `confirm_down` vs `confirm_released` (§F1) | Slot stepper buttons (`+`/`-`) need *repeat-on-held*. |
| `cancel_pressed` (§F1) | Esc / B on the modal and on the screen itself; different behavior at each scope. |
| `Clay_PointerOver` / `Clay_GetPointerData` (§F3) | Tooltip placement; pointer press with drag-off cancellation in the grid. |
| `last_focus_move_source` → `focusVisible` (§F2) | Tooltip only renders when `focusVisible`; cell outlines suppressed on mouse-click focus. |
| Live-set fallback (§F2) | The NEW cell mounts mid-frame — falls into scope cleanly. |
| `<Focusable>` escape hatch (§8) | Inventory cells are not `<Button>` (they show an icon + count + glow). |
| `derive_visual_state` (§F4) | All cells, slots, tabs, modal buttons, toggles run through it. |
| Disabled (§F4 behavioral test) | Level-locked cells must reject confirm, no `pressed`, but stay tooltip-able. |
| `<Toggle>` stateless (§F5) | A "Compare to equipped" toggle lives in the header. |
| Press-target match (§F3) | Mouse drag-off cancellation on inventory cells. |

---

## 3. The screen body code

Self-contained — includes the helper primitives the screen uses
(`InventoryCell`, `EquipmentSlot`, `Tabs`, `Stepper`, `Tooltip`, `Modal`)
so the whole thing is reviewable in one read.

```cpp
// docs/ui-focus-stress-test.md — IMAGINARY code, do not compile.
//
// Proposed file: src/ui/screens/loadout.cpp
//
// Assumes: InputState has the F1 nav/confirm/cancel fields; <FocusScope>,
// use_focusable, use_initial_focus, use_focus_traversal exist (F2);
// derive_visual_state + style fns exist (F4); <Focusable>, <Button>, <Toggle>
// primitives exist (F5).

#include <clay.h>

#include "../../app_state.h"
#include "../../input.h"
#include "../../react.h"

#include "../providers/input_provider.h"
#include "../providers/theme_provider.h"
#include "../providers/focus_provider.h"          // <FocusScope>, FocusContext  (proposed)

#include "../state/visual_state.h"                // FocusableState, ControlState, VisualState
#include "../state/focus_traversal.h"             // use_focus_traversal, GridStrategy
#include "../components/button.h"
#include "../components/toggle.h"
#include "../components/focusable.h"              // slot wrapper

#include "../game/inventory.h"                    // Item, slot kinds, requirements

// ---------------------------------------------------------------------------
// Screen-local state
// ---------------------------------------------------------------------------

enum LoadoutTab { TAB_WEAPONS, TAB_ARMOR, TAB_CONSUMABLES, TAB_COUNT };

struct LoadoutScratch {
    LoadoutTab    tab;
    int           equipped[SLOT_COUNT];      // item ids, -1 = empty
    int           pending[SLOT_COUNT];       // tentative; commit on Save
    int           hovered_item;              // -1 = none; drives stat preview
    bool          compare_mode;              // <Toggle> state
    bool          confirm_modal_open;
    bool          discard_guard_open;
    int           pending_equip_item;        // item being confirmed
    int           pending_equip_slot;
    int           quantity_to_use;           // for Consumables stepper
    bool          had_new_badge_last_frame;  // for live-set fallback test
};

static bool loadout_is_dirty(const LoadoutScratch *s) {
    for (int i = 0; i < SLOT_COUNT; ++i)
        if (s->pending[i] != s->equipped[i]) return true;
    return false;
}

// ---------------------------------------------------------------------------
// Tooltip — pure renderer; no focus participation. Reads pointer for placement
// only when focus source is mouse; otherwise sticks to the focused cell.
// ---------------------------------------------------------------------------

struct TooltipProps {
    const Item *item;            // nullptr → no tooltip
    Clay_ElementId anchor_id;    // focused/hovered cell to align beside
    bool          show;          // gated by focusVisible || hovered
};

void Tooltip(TooltipProps p) {
    REACT_COMPONENT_BEGIN("Tooltip") {
        if (!p.show || !p.item) {
            // Still render an empty fiber so id stays stable across frames.
            REACT_COMPONENT_END(); return;
        }
        CLAY({
            .id = CLAY_ID_LOCAL("TooltipPanel"),
            .floating = {
                .attachTo = CLAY_ATTACH_TO_ELEMENT_WITH_ID,
                .parentId = p.anchor_id.id,
                .attachPoints = { CLAY_ATTACH_POINT_LEFT_TOP,
                                  CLAY_ATTACH_POINT_RIGHT_TOP },
                .offset = { 8, 0 },
            },
            .layout = { .padding = CLAY_PADDING_ALL(10),
                        .childGap = 4,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM },
            .backgroundColor = tooltip_bg,
            .cornerRadius = CLAY_CORNER_RADIUS(6),
        }) {
            CLAY_TEXT(cs(p.item->name),
                CLAY_TEXT_CONFIG({ .textColor = fg_bright, .fontSize = 14 }));
            CLAY_TEXT(cs(p.item->flavor),
                CLAY_TEXT_CONFIG({ .textColor = fg_dim,    .fontSize = 12 }));
            if (p.item->level_req > 0) {
                static char buf[32];
                snprintf(buf, sizeof(buf), "Lv %d required", p.item->level_req);
                CLAY_TEXT(cs(buf),
                    CLAY_TEXT_CONFIG({ .textColor = warn, .fontSize = 12 }));
            }
        }
    } REACT_COMPONENT_END();
}

// ---------------------------------------------------------------------------
// InventoryCell — uses the <Focusable> escape hatch because it's not a plain
// button: it draws an icon, a count, a level-lock badge, and a NEW pip.
// ---------------------------------------------------------------------------

struct InventoryCellProps {
    const Item *item;
    bool        is_new;            // shows pip; first frame alive wants initial focus
    bool        disabled;          // level-locked
    void      (*on_pick)(void *u, int item_id);
    void       *user;
    int         row, col;          // for grid id + traversal
};

static void cell_pick_trampoline(void *user) {
    auto *p = static_cast<InventoryCellProps *>(user);
    p->on_pick(p->user, p->item->id);
}

void InventoryCell(InventoryCellProps p) {
    REACT_COMPONENT_BEGIN_KEY("InventoryCell", p.row * 64 + p.col) {
        // Cell hover should preview stats even on mouse — we want hovered, not
        // just focused, to drive the preview signal back up to the screen.
        Focusable({ .on_confirm = cell_pick_trampoline,
                    .user       = &p,
                    .disabled   = p.disabled },
            [&](FocusableState f) {
                ControlState c = { .checked  = false,
                                   .selected = false,
                                   .disabled = p.disabled };
                VisualState  v = derive_visual_state(f, c);
                CellStyle    s = cell_style_for(v);

                // Side-effect the screen reads next frame for the stat preview.
                // (Hovered OR focusVisible — we want the preview to follow the
                // gamepad cursor as well as the mouse.)
                if (f.hovered || (f.focused && f.focusVisible)) {
                    loadout_set_preview_item(p.item->id);
                }

                CLAY({
                    .id              = f.id,
                    .layout          = { .sizing = { CLAY_SIZING_FIXED(64),
                                                     CLAY_SIZING_FIXED(64) },
                                         .padding = CLAY_PADDING_ALL(6),
                                         .childAlignment = { CLAY_ALIGN_X_CENTER,
                                                             CLAY_ALIGN_Y_CENTER } },
                    .backgroundColor = s.bg,
                    .border          = { .width = CLAY_BORDER_OUTSIDE(s.border_w),
                                         .color = s.border },
                    .cornerRadius    = CLAY_CORNER_RADIUS(6),
                }) {
                    Icon(p.item->icon, s.fg);
                    if (p.item->count > 1) {
                        static char buf[8];
                        snprintf(buf, sizeof(buf), "x%d", p.item->count);
                        CLAY_TEXT(cs(buf),
                            CLAY_TEXT_CONFIG({ .textColor = s.fg, .fontSize = 11 }));
                    }
                    if (p.is_new) {
                        CLAY({ .id = CLAY_ID_LOCAL("NewPip"),
                               .floating = { .attachPoints = { CLAY_ATTACH_POINT_RIGHT_TOP,
                                                               CLAY_ATTACH_POINT_RIGHT_TOP },
                                             .offset = { -4, 4 } },
                               .backgroundColor = accent,
                               .cornerRadius = CLAY_CORNER_RADIUS(4),
                               .layout = { .sizing = { CLAY_SIZING_FIXED(10),
                                                       CLAY_SIZING_FIXED(10) } } }) {}
                    }
                    if (v.unavailable) {
                        Icon(ICON_LOCK, dim_fg);
                    }
                }
            });
    } REACT_COMPONENT_END();
}

// ---------------------------------------------------------------------------
// EquipmentSlot — plain <Button> wrapping a slot card.
// ---------------------------------------------------------------------------

struct EquipmentSlotProps {
    SlotKind     kind;
    int          equipped_item_id;
    void       (*on_pick_slot)(void *u, SlotKind kind);
    void        *user;
};

static void slot_pick_trampoline(void *user) {
    auto *p = static_cast<EquipmentSlotProps *>(user);
    p->on_pick_slot(p->user, p->kind);
}

void EquipmentSlot(EquipmentSlotProps p) {
    REACT_COMPONENT_BEGIN_KEY("EquipmentSlot", (int)p.kind) {
        Button({
            .label      = slot_label_for(p.kind, p.equipped_item_id),
            .on_confirm = slot_pick_trampoline,
            .user       = &p,
            .disabled   = false,
        });
    } REACT_COMPONENT_END();
}

// ---------------------------------------------------------------------------
// Tabs — three <Selectable>s in a row; the screen owns which is selected.
// ---------------------------------------------------------------------------

struct TabsProps {
    LoadoutTab current;
    void     (*on_change)(void *u, LoadoutTab t);
    void      *user;
};

void Tabs(TabsProps p) {
    REACT_COMPONENT_BEGIN("Tabs") {
        static const char *labels[TAB_COUNT] = { "Weapons", "Armor", "Consumables" };
        CLAY({ .id = CLAY_ID_LOCAL("TabRow"),
               .layout = { .childGap = 4,
                           .layoutDirection = CLAY_LEFT_TO_RIGHT } }) {
            for (int i = 0; i < TAB_COUNT; ++i) {
                REACT_COMPONENT_BEGIN_KEY("Tab", i) {
                    // Selectable's on_confirm fires when user picks a tab.
                    Selectable({
                        .label      = labels[i],
                        .selected   = (p.current == (LoadoutTab)i),
                        .on_confirm = [](void *u, int idx) {
                            auto *pp = static_cast<TabsProps *>(u);
                            pp->on_change(pp->user, (LoadoutTab)idx);
                        },
                        .user       = &p,
                        .index      = i,         // passed through to on_confirm
                    });
                } REACT_COMPONENT_END();
            }
        }
    } REACT_COMPONENT_END();
}

// ---------------------------------------------------------------------------
// Stepper — +/- on a quantity. Wants HELD repeat: confirm_down ticks every
// frame at a throttled cadence.  This is the spot where confirm_down vs
// confirm_pressed matters.
// ---------------------------------------------------------------------------

struct StepperProps {
    int *value;
    int  min_v, max_v;
};

void Stepper(StepperProps p) {
    REACT_COMPONENT_BEGIN("Stepper") {
        CLAY({ .id = CLAY_ID_LOCAL("StepperRow"),
               .layout = { .childGap = 6,
                           .layoutDirection = CLAY_LEFT_TO_RIGHT } }) {

            // Custom focusable buttons because we want repeat-while-held.
            Focusable({ .on_confirm = nullptr,         // we fire from raw fields
                        .user       = nullptr,
                        .disabled   = (*p.value <= p.min_v) },
                [&](FocusableState f) {
                    ControlState c = { .disabled = (*p.value <= p.min_v) };
                    VisualState  v = derive_visual_state(f, c);
                    const InputState *in = (const InputState *)use_context(&InputContext);
                    // Held repeat: every Nth frame while down and focused/hovered+pressed.
                    if (!c.disabled && f.pressed && in && in->confirm_down) {
                        if (frame_repeat_tick(/*hz=*/10)) *p.value -= 1;
                    }
                    CLAY({ .id = f.id,
                           .backgroundColor = stepper_btn_bg_for(v),
                           .layout = { .padding = CLAY_PADDING_ALL(6) } }) {
                        CLAY_TEXT(cs("-"), CLAY_TEXT_CONFIG({ .textColor = fg, .fontSize = 18 }));
                    }
                });

            static char buf[8];
            snprintf(buf, sizeof(buf), "%d", *p.value);
            CLAY_TEXT(cs(buf), CLAY_TEXT_CONFIG({ .textColor = fg, .fontSize = 18 }));

            Focusable({ .on_confirm = nullptr,
                        .user       = nullptr,
                        .disabled   = (*p.value >= p.max_v) },
                [&](FocusableState f) {
                    ControlState c = { .disabled = (*p.value >= p.max_v) };
                    VisualState  v = derive_visual_state(f, c);
                    const InputState *in = (const InputState *)use_context(&InputContext);
                    if (!c.disabled && f.pressed && in && in->confirm_down) {
                        if (frame_repeat_tick(/*hz=*/10)) *p.value += 1;
                    }
                    CLAY({ .id = f.id,
                           .backgroundColor = stepper_btn_bg_for(v),
                           .layout = { .padding = CLAY_PADDING_ALL(6) } }) {
                        CLAY_TEXT(cs("+"), CLAY_TEXT_CONFIG({ .textColor = fg, .fontSize = 18 }));
                    }
                });
        }
    } REACT_COMPONENT_END();
}

// ---------------------------------------------------------------------------
// Modal — opens a NESTED <FocusScope> that traps nav. Cancel pops itself.
// ---------------------------------------------------------------------------

struct ModalProps {
    const char *title;
    const char *body;
    const char *yes_label;
    const char *no_label;
    void      (*on_yes)(void *u);
    void      (*on_no)(void *u);
    void       *user;
};

void ConfirmModal(ModalProps p) {
    REACT_COMPONENT_BEGIN("ConfirmModal") {
        // Floating Clay element so it overlays the screen.
        CLAY({
            .id = CLAY_ID_LOCAL("ModalScrim"),
            .floating = { .attachPoints = { CLAY_ATTACH_POINT_LEFT_TOP,
                                            CLAY_ATTACH_POINT_LEFT_TOP } },
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                        .childAlignment = { CLAY_ALIGN_X_CENTER,
                                            CLAY_ALIGN_Y_CENTER } },
            .backgroundColor = scrim,
        }) {
            // Nested FocusScope: keyboard/gamepad nav is constrained to its
            // children by the screen-stack rule (open Q 2 in the plan; we
            // assume the rule says "deepest open scope wins").
            FOCUS_SCOPE("ModalScope") {
                // Default initial focus: the destructive button is NOT the first.
                use_initial_focus([](Clay_ElementId id) {
                    return id == CLAY_ID("ModalNo");          // No is safer.
                });

                CLAY({ .id = CLAY_ID_LOCAL("ModalPanel"),
                       .layout = { .padding = CLAY_PADDING_ALL(20),
                                   .childGap = 12,
                                   .layoutDirection = CLAY_TOP_TO_BOTTOM },
                       .backgroundColor = panel,
                       .cornerRadius = CLAY_CORNER_RADIUS(8) }) {
                    CLAY_TEXT(cs(p.title),
                        CLAY_TEXT_CONFIG({ .textColor = fg_bright, .fontSize = 20 }));
                    CLAY_TEXT(cs(p.body),
                        CLAY_TEXT_CONFIG({ .textColor = fg, .fontSize = 14 }));
                    CLAY({ .id = CLAY_ID_LOCAL("ModalRow"),
                           .layout = { .childGap = 8,
                                       .layoutDirection = CLAY_LEFT_TO_RIGHT } }) {
                        CLAY_WITH_ID("ModalNo")  Button({ .label = p.no_label,
                                                          .on_confirm = p.on_no,
                                                          .user = p.user });
                        CLAY_WITH_ID("ModalYes") Button({ .label = p.yes_label,
                                                          .on_confirm = p.on_yes,
                                                          .user = p.user });
                    }
                }

                // Modal-level cancel: B / Esc closes via on_no.
                const InputState *in = (const InputState *)use_context(&InputContext);
                if (in && in->cancel_pressed && p.on_no) p.on_no(p.user);
            }
        }
    } REACT_COMPONENT_END();
}

// ---------------------------------------------------------------------------
// The Loadout screen body
// ---------------------------------------------------------------------------

void Loadout(void) {
    REACT_COMPONENT_BEGIN("Loadout") {
        void **scratch_ref = use_ref(nullptr);
        if (!*scratch_ref) {
            auto *fresh = new LoadoutScratch{};
            fresh->tab            = TAB_WEAPONS;
            fresh->hovered_item   = -1;
            fresh->compare_mode   = false;
            fresh->pending_equip_item = -1;
            fresh->pending_equip_slot = -1;
            fresh->quantity_to_use= 1;
            for (int i = 0; i < SLOT_COUNT; ++i) {
                fresh->equipped[i] = game_inventory_equipped(i);
                fresh->pending [i] = fresh->equipped[i];
            }
            *scratch_ref = fresh;
        }
        auto *s = static_cast<LoadoutScratch *>(*scratch_ref);

        const InputState *in = (const InputState *)use_context(&InputContext);

        // ----- Screen-level cancel: Esc / B -----
        // Only consumed at this scope when no modal is open (deepest-scope wins).
        if (in && in->cancel_pressed && !s->confirm_modal_open
                                     && !s->discard_guard_open) {
            if (loadout_is_dirty(s)) s->discard_guard_open = true;
            else                     game_nav_pop();
        }

        FOCUS_SCOPE("LoadoutScope") {
            // 2D grid traversal for the inventory; lists fall back to vertical.
            use_focus_traversal(GridStrategy{ .cols = 6 });

            // If a NEW item is present, prefer it as initial focus *the first
            // frame it appears*; otherwise default to first tab.
            use_initial_focus([s](Clay_ElementId id) {
                if (item_pool_has_new() && !s->had_new_badge_last_frame) {
                    return id == CLAY_ID_LOCAL_KEY("InventoryCell",
                                                   item_pool_new_index());
                }
                return id == CLAY_ID("Tab_0");
            });
            s->had_new_badge_last_frame = item_pool_has_new();

            CLAY({ .id = CLAY_ID_LOCAL("LoadoutRoot"),
                   .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                               .padding = CLAY_PADDING_ALL(20),
                               .childGap = 16,
                               .layoutDirection = CLAY_TOP_TO_BOTTOM },
                   .backgroundColor = bg }) {

                // ---- Header: tabs + compare toggle -------------------------
                CLAY({ .id = CLAY_ID_LOCAL("Header"),
                       .layout = { .childGap = 16,
                                   .layoutDirection = CLAY_LEFT_TO_RIGHT } }) {
                    Tabs({ .current = s->tab,
                           .on_change = [](void *u, LoadoutTab t) {
                               static_cast<LoadoutScratch *>(u)->tab = t;
                           },
                           .user = s });
                    Toggle({ .label   = "Compare equipped",
                             .checked = s->compare_mode,
                             .on_change = [](void *u, bool v) {
                                 static_cast<LoadoutScratch *>(u)->compare_mode = v;
                             },
                             .user = s });
                }

                // ---- Body: grid + equipment column -------------------------
                CLAY({ .id = CLAY_ID_LOCAL("Body"),
                       .layout = { .childGap = 16,
                                   .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                   .sizing = { CLAY_SIZING_GROW(0),
                                               CLAY_SIZING_GROW(0) } } }) {

                    // ----- Scrollable inventory grid ------------------------
                    CLAY({ .id = CLAY_ID_LOCAL("InventoryScroll"),
                           .clip = { .vertical = true,
                                     .childOffset = { 0, s->scroll_y } },
                           .layout = { .sizing = { CLAY_SIZING_FIXED(64*6 + 5*4),
                                                   CLAY_SIZING_GROW(0) } } }) {
                        const ItemList *items = item_pool_for_tab(s->tab);
                        for (int i = 0; i < items->count; ++i) {
                            const Item *it = &items->data[i];
                            // Grid-laid out: 6 per row.
                            InventoryCell({
                                .item     = it,
                                .is_new   = it->is_new,
                                .disabled = it->level_req > player_level(),
                                .on_pick  = [](void *u, int item_id) {
                                    auto *ss = static_cast<LoadoutScratch *>(u);
                                    ss->pending_equip_item = item_id;
                                    ss->pending_equip_slot =
                                        default_slot_for(item_id);
                                    ss->confirm_modal_open = true;
                                },
                                .user     = s,
                                .row      = i / 6,
                                .col      = i % 6,
                            });
                        }
                    }

                    // ----- Right column: slots + stats + tooltip ------------
                    CLAY({ .id = CLAY_ID_LOCAL("RightCol"),
                           .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                       .childGap = 12,
                                       .sizing = { CLAY_SIZING_GROW(0),
                                                   CLAY_SIZING_GROW(0) } } }) {

                        // Equipment slots
                        CLAY({ .id = CLAY_ID_LOCAL("Slots"),
                               .layout = { .childGap = 6,
                                           .layoutDirection = CLAY_TOP_TO_BOTTOM } }) {
                            for (int k = 0; k < SLOT_COUNT; ++k) {
                                EquipmentSlot({
                                    .kind = (SlotKind)k,
                                    .equipped_item_id = s->pending[k],
                                    .on_pick_slot = [](void *u, SlotKind kind) {
                                        // jump focus into the inventory grid,
                                        // filtered to items compatible with `kind`.
                                        loadout_filter_to_slot(kind);
                                    },
                                    .user = s,
                                });
                            }
                        }

                        // Live stat preview (purely a renderer; reads loadout_preview())
                        StatPreview({ .current = s->equipped,
                                      .preview = loadout_preview_item(),
                                      .compare = s->compare_mode });

                        // Quantity stepper, only relevant on consumables tab.
                        if (s->tab == TAB_CONSUMABLES) {
                            Stepper({ .value = &s->quantity_to_use,
                                      .min_v = 1, .max_v = 99 });
                        }
                    }
                }
            }

            // ---- Focus-following tooltip ----------------------------------
            // Only show when the targeted cell is "visibly" targeted — i.e.
            // focusVisible (gamepad/kbd) OR mouse hover. This is the §F4
            // visual collapse: tooltip reads the same `targeted` notion.
            {
                Clay_ElementId anchor = focus_current_id();          // helper
                const Item    *item   = loadout_preview_item();
                bool show = item && (focus_current_is_visible()      // focusVisible
                                     || Clay_PointerOver(anchor));
                Tooltip({ .item = item, .anchor_id = anchor, .show = show });
            }
        } // FOCUS_SCOPE("LoadoutScope")

        // ---- Modals: open OUTSIDE the screen scope so they own their own. -
        if (s->confirm_modal_open) {
            ConfirmModal({
                .title     = "Equip item?",
                .body      = "This will replace what's currently in that slot.",
                .yes_label = "Equip",
                .no_label  = "Cancel",
                .on_yes    = [](void *u) {
                    auto *ss = static_cast<LoadoutScratch *>(u);
                    ss->pending[ss->pending_equip_slot] = ss->pending_equip_item;
                    ss->confirm_modal_open = false;
                },
                .on_no     = [](void *u) {
                    static_cast<LoadoutScratch *>(u)->confirm_modal_open = false;
                },
                .user = s,
            });
        }

        if (s->discard_guard_open) {
            ConfirmModal({
                .title     = "Discard changes?",
                .body      = "You have unsaved loadout changes.",
                .yes_label = "Discard",
                .no_label  = "Keep editing",
                .on_yes    = [](void *u) {
                    auto *ss = static_cast<LoadoutScratch *>(u);
                    ss->discard_guard_open = false;
                    game_nav_pop();
                },
                .on_no     = [](void *u) {
                    static_cast<LoadoutScratch *>(u)->discard_guard_open = false;
                },
                .user = s,
            });
        }
    } REACT_COMPONENT_END();
}
```

---

## 4. Walkthrough — four representative interactions

Each step cites the design section that justifies it.

### A. Keyboard — user presses `Down` on the third grid cell, then `Enter`

Setup: focus is on `InventoryCell(row=0,col=2)`. Tab = Weapons.

**Frame N (Down)**

1. `main.cpp` produces `nav_down = true` for one frame, populates `InputState`
   (§F1 "edge vs held semantics", `nav_*` true on press *and* key-repeat).
2. `react_begin_frame`; `<FocusScope>` runs first; opens the per-frame cycle.
   `focused_id` (the col=2 cell) is in last frame's `focusable_list` → no
   fallback. `use_focus_traversal(GridStrategy{cols=6})` consumes `nav_down`
   and moves `focused_id` to the index +6 (col=2 of row=1).
   `last_focus_move_source = Keyboard`. (§F2 "The per-frame cycle inside one
   `<FocusScope>`" step 1.)
3. Children render. The new focused cell appends to `focusable_list`, sees
   `focused=true`, `focusVisible=true` (from the source field, §F2 "Deriving
   `focusVisible`"), and writes itself into the preview signal.
4. `Tooltip` sees `focus_current_is_visible() == true`, renders next to the
   new anchor.

**Frame N+k (Enter)**

1. `confirm_pressed = true` for one frame (§F1 timings table).
2. Cell's `use_focusable` body sees `focused && confirm_pressed && !disabled`
   → fires `on_pick` (§F2 hook bullet "Fires `on_confirm` when …").
3. `on_pick` sets `confirm_modal_open = true`. (Standard React semantics —
   §9 "`on_confirm` runs *during the same render pass* as the input edge".)
4. Same frame: the screen body's `if (s->confirm_modal_open)` mounts
   `<ConfirmModal>`. The modal's `FOCUS_SCOPE("ModalScope")` is a fresh
   scope; its initial-focus predicate (`ModalNo`) is run on the *next* frame
   because the live-set is empty this frame (§F2 "A new focusable that mounts
   this frame is reachable starting *next* frame").

### B. Gamepad — user holds `A` on the `+` stepper for half a second

1. Frame N: `A` press → `confirm_pressed = true`, `confirm_down = true`.
   `Stepper`'s `Focusable` body: `f.pressed` becomes true via the keyboard
   path of §F3 (`key_held = focused && confirm_down && !disabled`).
   `frame_repeat_tick(10)` returns true on the first frame → value += 1.
2. Frames N+1…N+k: `confirm_pressed = false`, `confirm_down = true`,
   `f.pressed` stays true. `frame_repeat_tick` returns true every 6 frames
   (at 60fps, ~10Hz). Value increments steadily.
3. Frame N+k+1: user releases. `confirm_down = false`,
   `confirm_released = true`. The §F3 release path
   (`was_held && !key_held && focused && confirm_released`) would normally
   fire `on_confirm` — but we passed `nullptr`, so nothing happens. Repeat
   stops because `f.pressed` is false.

### C. Mouse — user presses on cell A, drags to cell B, releases on B

1. Frame N: pointer over A, `PRESSED_THIS_FRAME`. A's
   `use_focusable` sets `press.pointer_press_origin = A.id`,
   `manager.focused_id = A.id`, `source = Mouse` (§F3 decision table row 1).
   `focusVisible` becomes false (source is Mouse) — no keyboard outline on A.
2. Frame N+1…N+k: pointer drags. A sees `over=false`,
   `press_origin == A.id` still set, `pressed=false` (slip-out, §F3
   row "PRESSED (held), !over"). B is hovered but its `press_origin` is 0
   (the global pointer press didn't originate on it).
3. Frame N+k+1: `RELEASED_THIS_FRAME` over B. B's row:
   `press_origin (0) != B.id` → release without firing (§F3 row 3). A's row:
   `press_origin == A.id && over == false` → release without firing, clear
   origin. Net: nothing fires. Correct.

### D. Edge case — modal opens while the mouse is mid-hover over a cell

1. Frame N: user clicks an inventory cell. `on_pick` fires → 
   `confirm_modal_open = true`. Same frame, modal renders (its scope is
   *open* this frame for layout — but its focusable list is empty this
   frame, so nav input is consumed by the modal scope but resolves to
   "fallback on next frame").
2. The mouse hasn't moved. `Clay_PointerOver(SomeInventoryCell)` is still
   true — but the modal scrim is `floating` and covers the screen. Clay's
   hit-tester returns the *topmost* element under the cursor → the scrim,
   not the cell. So `over` on the cell is false. (§F3 "Why this is
   contention-free" — Clay only marks one topmost element.)
3. `Tooltip`'s `show` predicate becomes `false` (no focusVisible, no
   hover on the anchor). Tooltip disappears as the modal opens. Good.
4. Frame N+1: modal's `use_initial_focus` runs and focuses `ModalNo`.
   `last_focus_move_source = Keyboard` (initial focus is treated as keyboard
   per §F2 "`focusVisible` is `true` immediately after any keyboard /
   gamepad nav" — by extension, programmatic initial focus). **Wait — the
   doc never actually pins this down.** See §6 below.

---

## 5. Where the design held up

These were answered cleanly and the code wrote itself:

1. **One `<FocusScope>` per screen.** The whole loadout sat inside one
   scope. The modal nested a second scope and the "deepest scope wins"
   intuition fell out of the screen-stack architecture (§F2 cross-scope
   gating note).
2. **`use_focusable` from `<Focusable>` escape hatch.** The inventory cell
   needed icon + count + lock badge + NEW pip — `<Button>` would have been
   wrong. The §8 escape hatch existed exactly for this case and worked
   verbatim.
3. **`derive_visual_state` collapsing input devices.** The cell, slot,
   tab, stepper, and modal buttons all branched on `VisualState` only. The
   ergonomic payoff is real — *every* style table is a 4-branch lookup, no
   `if (input == gamepad)` anywhere.
4. **`is_dirty` + cancel routing.** The screen's `cancel_pressed` handler
   is two lines. Because the deepest-open scope owns the input, the modal's
   own cancel doesn't fight with the screen's — clean separation.
5. **Press-target match for the mouse.** Walkthrough C is a five-line
   reasoning chain and it Just Works because §F3 already did the thinking.
6. **`focusVisible` driving tooltip visibility.** The plan's §F4 collapse
   of `hovered || (focused && focusVisible)` into `targeted` mapped 1:1 onto
   "show tooltip when targeted". One predicate, zero per-device branches.
7. **Disabled cells.** `c.disabled = item->level_req > player_level()`
   passed into `derive_visual_state` produced `unavailable`, which wins
   over `targeted` in the style table; `on_pick` is gated by `!disabled`
   inside `use_focusable`. Three lines.

---

## 6. Where the design creaked or had gaps

Honest list. For each I quote the plan or note "not covered".

### 6.1 No primitive for "2D grid traversal"

The walkthrough uses `use_focus_traversal(GridStrategy{ .cols = 6 })`. The
plan mentions this exactly once, in a bullet under §F2 scope:

> `nav_left` / `nav_right` unbound by default; 2D screens override via
> `use_focus_traversal(strategy)`.

What it doesn't specify:

- What does `strategy` look like as a type? Is it a function pointer? A
  vtable? An enum? My code invented `GridStrategy{ .cols = 6 }` — totally
  unspecified.
- How does a strategy know each focusable's *position* in the grid? The
  manager only has `focusable_list` (a flat sequence of ids); there's no
  notion of (row, col). The strategy would need either to receive the
  visual layout from Clay (it can't — effects fire after layout) or to be
  told by the cells (no hook for that). I papered over this.
- What if some children are tabs (linear) and others are grid cells (2D)?
  The strategy is scope-wide, not per-region. **No story.**

This is the single biggest gap. Almost every "real" UI has a 2D region.

### 6.2 Scope nesting — modal "trap" is hand-waved

Plan §F2:

> Cross-screen nav-input gating (Pause shouldn't move its focus while
> Settings is on top) is a screen-stack concern (`engine-ui-boundary-plan.md`), not a
> focus-model concern.

…and §15 open question 2:

> until then, `<FocusScope>` exists but only one scope at a time is
> meaningful.

But a modal is **not** a screen — it's a sub-region of the same screen.
There is *no* screen-stack push when the equip-confirm modal opens. So:
which scope consumes `nav_*` / `cancel_pressed` when two scopes are live
on one screen? My code assumes "deepest open scope wins" but the doc
doesn't say that. It also doesn't say where in the tree the rule is
enforced, or what hook a screen calls to indicate "I have a trap open".

### 6.3 `use_initial_focus` predicate shape is underspecified

My code:

```cpp
use_initial_focus([s](Clay_ElementId id) {
    if (item_pool_has_new() && !s->had_new_badge_last_frame)
        return id == CLAY_ID_LOCAL_KEY("InventoryCell", item_pool_new_index());
    return id == CLAY_ID("Tab_0");
});
```

The plan says:

> `use_initial_focus(predicate)` lets a screen pick initial focus
> differently; otherwise the first entry of `focusable_list` becomes
> focused on the second frame the screen is alive.

But:

- When is `predicate` evaluated? Every frame? Only on fallback?
- Does it run against last frame's list or this frame's?
- What if zero entries match the predicate? Fall through to first?
- Is the predicate called once per id (O(n)) every frame the fallback
  rule trips? Cheap, but unspecified.
- For my "first frame the NEW cell exists" case, I needed to track
  `had_new_badge_last_frame` *myself*. The plan provides no way to say
  "fire this predicate exactly when this id first enters the list."

### 6.4 No public way to read "the current focused id" from outside a focusable

My `Tooltip` block uses `focus_current_id()` and
`focus_current_is_visible()` — two helpers the plan does not propose. The
screen-level code needs them to render a tooltip *next to* whatever's
focused, but it isn't itself a focusable. Possible workarounds in the
spec'd API:

- Make every cell write its id+visible flag to scratch state when focused
  (gross — N writes per frame, only one is the truth).
- `use_context(&FocusContext)` and read `manager->focused_id` directly —
  but `FocusManager` is described as private to `<FocusScope>`'s `use_ref`
  and the doc doesn't say its layout is part of the public API.

The plan needs a `use_focus_state()` or similar that returns
`{ focused_id, focusVisible, source }` for the current scope.

### 6.5 Stepper repeat — `frame_repeat_tick(hz)` is invented

For held-button repeat, my Stepper code invokes
`frame_repeat_tick(/*hz=*/10)`. The plan covers `confirm_pressed`,
`confirm_down`, and `confirm_released` (§F1) but provides no repeat-rate
helper. A consumer must either:

- Track their own frame-counter ref to throttle (ugly, hook slot for every
  repeating button).
- Lean on `nav_*`'s "keyboard repeat is the canonical mapping" (§F1) —
  but repeat is only documented for nav, not for confirm.

Stepper-style UIs (sliders, scrollbars, +/- counters) are ubiquitous. The
plan says master-volume slider is "a separate primitive, separate plan"
(§F5) but doesn't even leave the hook for it.

### 6.6 No clean way for a focusable to publish "preview" data

`InventoryCell` calls `loadout_set_preview_item(...)` as a side effect
during render so the right-column `StatPreview` can read it. This is a
hack — it depends on render order (cells render before `StatPreview`).
The plan offers no "focus-following data" pattern; a real-world consumer
will reinvent this badly. Worth a §11-ish ownership note.

### 6.7 `<Selectable>` shape is genuinely vague

§F5 says:

> `<Selectable>` — Leaf only — *no* group wrapper. The screen owns the
> "which-of-N is selected" prop.

Fine, but my `Tabs` code wants `on_confirm(int idx)` — a callback that
knows *which* selectable fired. The plan's `<Button>` example takes
`on_confirm(void *user)` with no index argument. Either every selectable
needs its own closure, or the API needs an index slot. I invented an
`.index` field; the doc has no story.

### 6.8 Per-frame scroll offset has no integration with focus

My `InventoryScroll` uses `clip.childOffset = { 0, s->scroll_y }`. The
plan's §13 explicitly waves this off:

> Scrolling: focused element moves outside the viewport. Still focused;
> pointer hit-test now misses. Out of scope. Plan does not promise
> auto-scroll-to-focus; that is the screen's job.

"That is the screen's job" but the screen has no API to ask "where is
my focused cell in clay coordinates" without an effect (which fires
post-layout — too late to change scroll for this frame). Realistically
the screen needs to scroll *one frame late*. That's not a deal-breaker,
but the doc should acknowledge the one-frame delay rather than punt.

### 6.9 Closing a modal — focus restoration in the parent scope

When `confirm_modal_open` flips to false, the modal unmounts. The plan's
live-set fallback restores focus inside the parent scope to whatever was
focused before the modal opened — **iff** the parent scope's
`focused_id` was untouched while the modal was up. My code relies on this
working. But §F2's restoration story is about *screens*:

> A screen is fully unmounted and later re-pushed.

A modal is not a screen. The §10A "Pause under Settings" scenario is the
closest analogue and relies on a screen-stack rule. For modal-on-screen,
who guarantees the parent's `focused_id` wasn't poked? The "deepest scope
wins" handwave from §6.2 above is what makes this work — and it isn't in
the plan.

### 6.10 `CLAY_ID_LOCAL_KEY` in a predicate evaluated by the scope

My initial-focus predicate uses `CLAY_ID_LOCAL_KEY("InventoryCell",
item_pool_new_index())`. But `CLAY_ID_LOCAL_KEY` resolves relative to the
*current* Clay parent at call site — and the predicate is called by the
scope at scope-open time, before the cell's parent is on the stack. So
this lookup is wrong as written. The plan provides no recipe for
"compute the id of a focusable from outside its render position."

I worked around this in my head by assuming a `CLAY_ID(...)`-style
absolute lookup, but the codebase today doesn't have that primitive
either.

---

## 7. Verdict

The design holds up **for the central case it was built for**: a flat list
of buttons / toggles on a screen, navigable by every input device, with
clean visual states. That core (§F1–§F4, plus `<Button>`/`<Toggle>`)
collapses a real complexity dragon into about ten lines of consumer code
and the falsifiable layering test is genuine progress.

It begins to **creak the moment a consumer steps off the flat-list path**:
2D traversal is a one-line aside that hides a multi-week design problem
(§6.1); modal/sub-region trap semantics are punted to a screen-stack
concept that doesn't apply (§6.2, §6.9); reading focus state from outside
a focusable (§6.4) and held-button repeat (§6.5) are everyday needs the
plan has no primitive for. Several of the §15 open questions are not
"validate later" — they are "won't compile in any real screen" gaps.

Recommendation: before shipping F5, the plan needs concrete answers to
the 2D traversal API shape, the scope-nesting precedence rule, a
`use_focus_state()` reader, and a repeat-rate helper for confirm-held.
Everything else in §6 is fixable in the consumer with mild ugliness, but
those four leak straight into the consumption layer with no workaround.

Follow-up: `client-ui-focus-navigation-architecture.md` sketches the stronger
answer for the biggest flaw above. Instead of asking the screen author for a
`GridStrategy`, the focus layer harvests each focusable's Clay rectangle after
layout and derives directional neighbors spatially, with explicit navigation
rules reserved for boundaries and exceptions.
