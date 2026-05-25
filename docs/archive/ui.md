# Input Agnostic Navigation
The UI should normalize keyboard, mouse, gamepad, and touch input so each device can target and activate the same components.

The UI should respond to the following input types:
- keyboard
- mouse
- gamepad
- touch

# Interaction States
Interaction states are raw per-element conditions produced by input and focus.

- focused: The element currently owns input focus.
- focusVisible: The element is focused and should render an explicit focus indicator.
- focusWithin: The element or one of its descendants currently owns input focus.
- hovered: A pointer is currently over the element.
- pressed: The element is currently being activated, such as during pointer down or key confirm.

# Control States
Control states are semantic per-component conditions that affect behavior and rendering.

- checked: A checkable control is currently on.
- selected: The item is currently chosen within a selectable set.
- disabled: The element is unavailable for interaction. 

# Visual States
Visual states are normalized styling flags derived from interaction and control states so different input devices can share the same presentation.

- targeted: hovered or focusVisible
- active: pressed
- chosen: selected or checked
- unavailable: disabled
