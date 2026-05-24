#pragma once

// Normalized per-frame UI actions.
//
// The platform layer (main.cpp) collects raw SDL events and produces an
// InputState. That InputState is then handed to the UI root (App) as props.
// The UI side decides how to expose it to descendants (today: a React
// context provided by App).

struct InputState {
    bool increment_counter;
    bool decrement_counter;
    bool toggle_counter;
    bool cycle_theme;
    bool fetch_image;
};
