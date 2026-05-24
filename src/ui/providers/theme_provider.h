#pragma once

#include "../../react.h"

// Provider for ThemeContext. Owns the theme catalog and the currently-selected
// index; listens to InputContext for the T key to cycle themes.
//
// Usage (mirrors PROVIDE's block-scope shape):
//
//     THEME_PROVIDER {
//         // children - use_context(&ThemeContext) here returns the current Theme*
//     }

// Internal - invoked by the macro. Not part of the public API.
extern ReactContext ThemeContext;

void theme_provider__enter(void);
void theme_provider__exit(void);

#define THEME_PROVIDER                                                              \
    for (int _theme_provider_once =                                                 \
             (REACT_PROVIDER_ENTER("ThemeProvider"), theme_provider__enter(), 0);   \
         !_theme_provider_once;                                                     \
         _theme_provider_once = 1, theme_provider__exit(), REACT_PROVIDER_EXIT())
