#include "sdl_retained_renderer.h"

#include <string.h>

namespace renderer {

namespace {

SDL_Color to_sdl_color(::ui::retained::Color color) {
    return {color.r, color.g, color.b, color.a};
}

SDL_FRect to_sdl_rect(::ui::retained::Rect rect) {
    return {rect.x, rect.y, rect.width, rect.height};
}

void set_draw_color(SDL_Renderer *renderer, ::ui::retained::Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

} // namespace

bool SdlRetainedRenderer::initialize(SDL_Renderer *renderer,
                                     FontRegistry &fonts) {
    if (!renderer || !fonts.default_font())
        return false;
    renderer_ = renderer;
    fonts_ = &fonts;
    return true;
}

void SdlRetainedRenderer::clear(::ui::retained::Color background) {
    if (!renderer_)
        return;
    set_draw_color(renderer_, background);
    SDL_RenderClear(renderer_);
}

void SdlRetainedRenderer::render(const ::ui::retained::DrawList &draw_list) {
    if (!renderer_)
        return;
    for (int i = 0; i < draw_list.count; ++i) {
        const ::ui::retained::DrawCommand &command = draw_list.commands[i];
        switch (command.kind) {
        case ::ui::retained::DrawCommandKind::Rect:
            render_rect(command);
            break;
        case ::ui::retained::DrawCommandKind::Text:
            render_text(command);
            break;
        }
    }
}

void SdlRetainedRenderer::present() {
    if (renderer_) {
        SDL_RenderPresent(renderer_);
    }
}

void SdlRetainedRenderer::render_rect(
    const ::ui::retained::DrawCommand &command) {
    SDL_FRect rect = to_sdl_rect(command.rect);
    if (command.fill.a > 0) {
        set_draw_color(renderer_, command.fill);
        SDL_RenderFillRect(renderer_, &rect);
    }

    if (command.border.a == 0 || command.border_width <= 0.0f)
        return;

    set_draw_color(renderer_, command.border);
    int border_width = static_cast<int>(command.border_width);
    if (border_width < 1)
        border_width = 1;
    for (int inset = 0; inset < border_width; ++inset) {
        SDL_FRect border = {
            rect.x + static_cast<float>(inset),
            rect.y + static_cast<float>(inset),
            rect.w - static_cast<float>(inset * 2),
            rect.h - static_cast<float>(inset * 2),
        };
        if (border.w > 0.0f && border.h > 0.0f) {
            SDL_RenderRect(renderer_, &border);
        }
    }
}

void SdlRetainedRenderer::render_text(
    const ::ui::retained::DrawCommand &command) {
    if (!fonts_ || !fonts_->default_font() || command.text[0] == '\0')
        return;

    TTF_Font *font = fonts_->default_font();
    if (command.font_size > 0) {
        TTF_SetFontSize(font, static_cast<float>(command.font_size));
    }

    SDL_Surface *surface = TTF_RenderText_Blended(
        font, command.text, strlen(command.text), to_sdl_color(command.fill));
    if (!surface)
        return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (texture) {
        SDL_FRect dst = {
            command.rect.x,
            command.rect.y,
            static_cast<float>(surface->w),
            static_cast<float>(surface->h),
        };
        SDL_RenderTexture(renderer_, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }
    SDL_DestroySurface(surface);
}

} // namespace renderer
