# UI focus stress test: Loadout screen

This document stress-tests the active focus and interaction contracts with a
large screen. The goal is to show the kind of component code a client screen
author should write when the primitives from `ui-focus-interaction-plan.md`
and the stack from `client-ui-focus-navigation-architecture.md` are in place.

The screen is intentionally busy:

- Three tabs: `Weapons`, `Armor`, `Consumables`.
- A scrollable inventory grid.
- An equipment panel with slot buttons.
- A stat readout that follows the focused or selected item.
- Item tooltips.
- A compare toggle.
- A quantity stepper for consumables.
- An equip-confirm dialog.
- A dirty-changes dialog on cancel.
- Disabled cells for level requirements.
- A newly-acquired item that should become initial focus.

The test is passed only if this screen uses the same primitives as a simple
button menu. It must not define a parallel navigation model.

---

## 1. Contracts Exercised

| Contract | Loadout pressure point |
|---|---|
| One focus scope per screen | The screen body opens the loadout scope. |
| Nested modal scope | Confirm and discard dialogs trap focus while open. |
| Spatial navigation | Tabs, grid cells, equipment slots, and dialog buttons are navigated from Clay rectangles. |
| `use_initial_focus` | A new item can request initial focus when it enters the grid. |
| `Focusable` escape hatch | Inventory cells are not plain buttons. |
| `Button` | Equipment slots and dialog controls use the primitive directly. |
| `Toggle` | Compare-to-equipped state is caller-owned. |
| Visual state derivation | Tabs, cells, slots, steppers, and dialog buttons style from `VisualState`. |
| Pointer press-target match | Dragging off an item cell cancels confirm. |
| Hooks for data and functions | Loadout components read values/functions through `use_loadout()`. |
| Local hook state | Dialog-open flags and screen-local scratch stay in the screen tree. |

---

## 2. Loadout Hook Boundary

Screen-specific data lives behind a hook. The hook returns values and functions
used by whichever component needs them.

```cpp
struct LoadoutItem {
    ItemId id;
    const char *name;
    int count;
    int level_req;
    bool is_new;
};

struct LoadoutTabOption {
    LoadoutTab id;
    const char *label;
};

struct LoadoutSlot {
    SlotKind kind;
    ItemId equipped_item;
    const char *label;
};

struct LoadoutResult {
    Span<const LoadoutTabOption> tabs;
    Span<const LoadoutItem> items;
    Span<const LoadoutSlot> slots;

    LoadoutTab active_tab;
    ItemId preview_item;
    ItemId pending_confirm_item;
    ItemId new_item_id;
    bool has_pending_confirm;
    bool has_new_item;
    bool compare_to_equipped;
    bool dirty;
    int player_level;
    int quantity;

    std::function<void(LoadoutTab)> select_tab;
    std::function<void(ItemId)> preview;
    std::function<void(ItemId)> request_equip;
    std::function<void()> cancel_pending_equip;
    std::function<void()> confirm_pending_equip;
    std::function<void(SlotKind)> inspect_slot;
    std::function<void(bool)> set_compare_to_equipped;
    std::function<void(int)> set_quantity;
};

LoadoutResult use_loadout();
```

The focus layer sees ids, disabled flags, and callbacks. It does not know what
an item, slot, tab, or stat preview means.

---

## 3. Inventory Cell

`InventoryCell` uses `Focusable` because it draws richer content than a
generic button. It still follows the normal state layering.

```cpp
struct InventoryCellProps {
    const LoadoutItem *item;
    bool disabled;
};

void InventoryCell(const InventoryCellProps &props) {
    REACT_COMPONENT_BEGIN_KEY("InventoryCell", (uint32_t)props.item->id) {
        LoadoutResult loadout = use_loadout();
        ItemId item_id = props.item->id;

        Focusable({
            .id = CLAY_IDI("InventoryItem", (uint32_t)item_id),
            .disabled = props.disabled,
            .on_focus = [loadout, item_id] {
                loadout.preview(item_id);
            },
            .on_confirm = [loadout, item_id] {
                loadout.request_equip(item_id);
            },
        }, [&](const UiFocusableState &focus) {
            ControlState control = {
                .disabled = props.disabled,
            };
            VisualState visual = derive_visual_state(focus, control);
            InventoryCellStyle style = inventory_cell_style(visual);

            CLAY({
                .id = focus.id,
                .layout = {
                    .sizing = {
                        CLAY_SIZING_FIXED(64),
                        CLAY_SIZING_FIXED(64),
                    },
                    .padding = CLAY_PADDING_ALL(6),
                    .childAlignment = {
                        CLAY_ALIGN_X_CENTER,
                        CLAY_ALIGN_Y_CENTER,
                    },
                },
                .backgroundColor = style.background,
                .border = {
                    .width = CLAY_BORDER_OUTSIDE(style.border_width),
                    .color = style.border,
                },
                .cornerRadius = CLAY_CORNER_RADIUS(6),
            }) {
                ItemIcon(item_id, style.icon);
                ItemCount(props.item->count, style.text);

                if (props.item->is_new) {
                    NewItemPip();
                }

                if (visual.unavailable) {
                    LockIcon(style.text);
                }

                if (visual.targeted) {
                    InventoryTooltip({
                        .item = props.item,
                        .anchor = focus.id,
                    });
                }
            }
        });
    } REACT_COMPONENT_END();
}
```

The tooltip is declared by the targeted cell itself. The screen does not need a
global helper for "the focused id" just to place item text next to the target.
The loadout preview value changes through the `on_focus` callback, so the
right-side stat readout is updated by the normal post-layout dispatch path
rather than by mutating hook-owned state from the render body.

---

## 4. Inventory Grid

The grid declares rows for layout only. Navigation comes from the final
rectangles harvested after Clay layout.

```cpp
void InventoryGrid(void) {
    REACT_COMPONENT_BEGIN("InventoryGrid") {
        LoadoutResult loadout = use_loadout();

        CLAY({
            .id = CLAY_ID_LOCAL("InventoryGrid"),
            .layout = {
                .sizing = { CLAY_SIZING_FIXED(420), CLAY_SIZING_GROW(0) },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childGap = 8,
            },
            .clip = { .vertical = true },
        }) {
            int columns = loadout_grid_columns_for_width();

            for (int row = 0; row * columns < loadout.items.count; ++row) {
                CLAY({
                    .id = CLAY_IDI_LOCAL("InventoryRow", row),
                    .layout = {
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        .childGap = 8,
                    },
                }) {
                    for (int col = 0; col < columns; ++col) {
                        int index = row * columns + col;
                        if (index >= loadout.items.count) break;

                        const LoadoutItem &item = loadout.items[index];
                        InventoryCell({
                            .item = &item,
                            .disabled = item.level_req > loadout.player_level,
                        });
                    }
                }
            }
        }
    } REACT_COMPONENT_END();
}
```

If the grid changes from six columns to four columns, navigation changes
because the rectangles changed. No screen-authored neighbor table changes.

---

## 5. Screen View

The screen view owns local hook state and composes the loadout pieces.

```cpp
void LoadoutScreenView(void) {
    REACT_COMPONENT_BEGIN("LoadoutScreen") {
        ScreenNavigator nav = use_screen_navigator();
        LoadoutResult loadout = use_loadout();

        int *discard_open = use_state_int(0);

        ui_focus_push_scope({
            .id = CLAY_ID("LoadoutFocusScope"),
            .modal = false,
            .wrap = false,
        });

        Clay_ElementId initial_focus = {};
        if (loadout.has_new_item) {
            initial_focus =
                CLAY_IDI("InventoryItem", (uint32_t)loadout.new_item_id);
        }
        use_initial_focus(initial_focus);

        bool loadout_dirty = loadout.dirty;

        CLAY({
            .id = CLAY_ID_LOCAL("LoadoutRoot"),
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(20),
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childGap = 16,
            },
        }) {
            LoadoutHeader();

            CLAY({
                .id = CLAY_ID_LOCAL("LoadoutBody"),
                .layout = {
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .childGap = 16,
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                },
            }) {
                InventoryGrid();
                EquipmentPanel();
            }

            LoadoutFooter({
                .on_cancel = [discard_open, nav, loadout_dirty] {
                    if (loadout_dirty) {
                        *discard_open = 1;
                    } else {
                        nav.pop_current();
                    }
                },
            });
        }

        if (loadout.has_pending_confirm) {
            EquipConfirmDialog({
                .item_id = loadout.pending_confirm_item,
                .on_cancel = loadout.cancel_pending_equip,
                .on_confirm = loadout.confirm_pending_equip,
            });
        }

        if (*discard_open) {
            ConfirmDialog({
                .title = "Discard changes?",
                .body = "Unsaved loadout changes will be lost.",
                .cancel_label = "Keep editing",
                .confirm_label = "Discard",
                .on_cancel = [discard_open] {
                    *discard_open = 0;
                },
                .on_confirm = [discard_open, nav] {
                    *discard_open = 0;
                    nav.pop_current();
                },
            });
        }

        ui_focus_pop_scope();
    } REACT_COMPONENT_END();
}
```

`LoadoutScreenView` does not receive a screen-shaped data parameter. It reads
`use_loadout()`, `use_screen_navigator()`, and local hook state where needed.

---

## 6. Retained Screen Entry

The retained stack object stays thin:

```cpp
class LoadoutScreen final : public UiScreen {
public:
    const char *debug_name() const override { return "Loadout"; }

    void build_ui() override {
        LoadoutScreenView();
    }
};
```

`ScreenStack` owns the object. The component tree owns the declaration and
local hooks.

---

## 7. Interaction Walkthroughs

### Keyboard navigation in the grid

1. The platform layer sets `nav_down = true`.
2. `ClientUi` begins the frame and focus uses the last completed layout.
3. The spatial resolver chooses the nearest enabled rectangle below the
   focused item.
4. The new item renders with `focused = true` and `focus_visible = true`.
5. `InventoryCell` previews the item through `use_loadout().preview`.
6. The tooltip renders from the targeted cell body.

### Gamepad confirm on a slot

1. The platform layer sets `confirm_pressed = true`.
2. The focused `Button` inside `EquipmentPanel` confirms.
3. The slot-specific function from `use_loadout()` opens the compatible item
   panel or requests an equip flow.
4. Any stack/game mutation is applied after layout through `ClientUi`.

### Mouse drag-off cancellation

1. Pointer press starts over item A, so A records the press origin and becomes
   focused with `focus_visible = false`.
2. The pointer drags away; A clears `pressed` but keeps the origin.
3. Release outside A clears the origin without confirming.
4. Release back on A confirms exactly once.

### Modal focus

1. A confirm dialog opens a modal focus scope after the parent content.
2. The modal scope becomes the active navigation consumer.
3. Parent loadout focus remains stored but frozen.
4. Closing the dialog unmounts the modal scope.
5. Parent focus resumes on the item or slot that opened the dialog.

---

## 8. Pass Criteria

The Loadout screen passes the stress test when:

- Grid navigation follows layout after resizing or tab changes.
- Disabled items remain visible, can show explanatory tooltip content, and do
  not confirm.
- The compare toggle stores truth in the screen/loadout hook, not in the
  primitive.
- Dialog buttons use the same `Button` primitive as equipment slots.
- No component contains per-device branches.
- No component declares sibling navigation edges for ordinary grid/list
  movement.
- Data access and intent requests flow through hooks returning values and
  functions.
- The retained `LoadoutScreen` only delegates to `LoadoutScreenView`.
