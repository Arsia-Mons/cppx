#pragma once

// Shared app-level globals and small helpers used by components.
//
// Definitions live in main.cpp; everything here is either a type, a constant,
// an inline helper, or an `extern` declaration.

#include <SDL3/SDL.h>
#include <clay.h>

#include <stdint.h>
#include <string.h>

// ----------------------------------------------------------------------------
// Theme

struct Theme {
    const char *name;
    Clay_Color  fg;
    Clay_Color  bg;
    Clay_Color  panel;
};

extern SDL_Renderer *g_sdl;

// ----------------------------------------------------------------------------
// Helpers

inline Clay_String cs(const char *s) {
    return Clay_String{ false, (int32_t)strlen(s), s };
}
