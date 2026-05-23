#pragma once

#include "client/ui/navigation/ScreenId.h"

namespace client::ui::navigation {

class ScreenStack {
public:
    ScreenId active() const { return active_; }
    void navigate(ScreenId screen) { active_ = screen; }
    void backToMenu() { active_ = ScreenId::MainMenu; }

private:
    ScreenId active_ = ScreenId::MainMenu;
};

}
