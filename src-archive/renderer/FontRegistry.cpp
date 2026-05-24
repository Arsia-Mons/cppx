#include "renderer/FontRegistry.h"

#include "ui/design/Typography.h"

#include <algorithm>
#include <array>

namespace {

std::array<const char *, 7> fontCandidates() {
    return {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Helvetica.ttf",
        "/Library/Fonts/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf"
    };
}

}

namespace renderer {

FontRegistry::~FontRegistry() {
    for (TTF_Font *font : fonts_) {
        if (font) {
            TTF_CloseFont(font);
        }
    }
    if (textEngine_) {
        TTF_DestroyRendererTextEngine(textEngine_);
    }
    if (TTF_WasInit()) {
        TTF_Quit();
    }
}

bool FontRegistry::initialize(SDL_Renderer *renderer) {
    if (!TTF_Init()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_Init failed: %s", SDL_GetError());
        return false;
    }

    textEngine_ = TTF_CreateRendererTextEngine(renderer);
    if (!textEngine_) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create TTF renderer text engine: %s", SDL_GetError());
        return false;
    }

    const char *path = nullptr;
    for (const char *candidate : fontCandidates()) {
        TTF_Font *probe = TTF_OpenFont(candidate, 18);
        if (probe) {
            TTF_CloseFont(probe);
            path = candidate;
            break;
        }
    }

    if (!path) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find a usable system font: %s", SDL_GetError());
        return false;
    }

    fonts_[ui::font::Body] = TTF_OpenFont(path, 18);
    fonts_[ui::font::Title] = TTF_OpenFont(path, 32);
    fonts_[ui::font::Mono] = TTF_OpenFont(path, 18);
    if (!fonts_[ui::font::Body] || !fonts_[ui::font::Title] || !fonts_[ui::font::Mono]) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open font set: %s", SDL_GetError());
        return false;
    }

    for (size_t i = 0; i < fonts_.size(); ++i) {
        fontPointers_[i] = fonts_[i];
    }
    return true;
}

Clay_Dimensions FontRegistry::measureText(Clay_StringSlice text, Clay_TextElementConfig *config) {
    TTF_Font *font = fonts_[std::min<size_t>(config->fontId, fonts_.size() - 1)];
    TTF_SetFontSize(font, config->fontSize);

    int width = 0;
    int height = 0;
    if (!TTF_GetStringSize(font, text.chars, text.length, &width, &height)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Text measurement failed: %s", SDL_GetError());
    }
    return {.width = static_cast<float>(width), .height = static_cast<float>(height)};
}

}
