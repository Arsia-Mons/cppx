#pragma once

#include <functional>

namespace client::ui {

struct AppValue {
  bool can_quit = false;
  std::function<void()> quit = {};
};

AppValue use_app();

} // namespace client::ui
