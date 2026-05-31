#pragma once

// The single SDL_ttf-backed MeasureTextFn (design §10.6). This is the ONLY
// implementation of the ui/ text-measure seam; it lives in renderer/ because it
// owns SDL_ttf. App installs it once at startup via ui::set_text_measurer, after
// FontRegistry::initialize. ui/ never touches SDL_ttf.

namespace renderer {

class FontRegistry;

// Wire the renderer-owned measurer into ui::set_text_measurer. The FontRegistry
// must outlive every measure call (App owns it for the process lifetime). Pass
// nullptr to clear (uninstall) the measurer.
void install_text_measurer(FontRegistry *fonts);

} // namespace renderer
