#include "control_mailbox.h"

#include "sdl/input.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <utility>

namespace platform {

static std::string json_escape(const std::string &value) {
  std::string out;
  out.reserve(value.size() + 8);
  for (char c : value) {
    switch (c) {
    case '\\':
      out += "\\\\";
      break;
    case '"':
      out += "\\\"";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      out += c;
      break;
    }
  }
  return out;
}

static bool write_text_atomic(const std::filesystem::path &path,
                              const std::string &text) {
  std::filesystem::create_directories(path.parent_path());
  std::filesystem::path tmp = path;
  tmp += ".tmp";
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out)
      return false;
    out << text;
  }
  std::error_code ec;
  std::filesystem::rename(tmp, path, ec);
  if (ec) {
    std::filesystem::remove(path, ec);
    ec.clear();
    std::filesystem::rename(tmp, path, ec);
  }
  return !ec;
}

static std::string read_text(const std::filesystem::path &path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

static size_t value_start(const std::string &json, const char *key) {
  std::string needle = "\"";
  needle += key;
  needle += "\"";
  size_t pos = json.find(needle);
  if (pos == std::string::npos)
    return std::string::npos;
  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos)
    return std::string::npos;
  ++pos;
  while (pos < json.size() &&
         (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) {
    ++pos;
  }
  return pos;
}

static std::string json_string_value(const std::string &json, const char *key,
                                     const char *fallback = "") {
  size_t pos = value_start(json, key);
  if (pos == std::string::npos || pos >= json.size() || json[pos] != '"') {
    return fallback;
  }
  ++pos;
  std::string out;
  while (pos < json.size()) {
    char c = json[pos++];
    if (c == '"')
      break;
    if (c == '\\' && pos < json.size()) {
      char escaped = json[pos++];
      switch (escaped) {
      case 'n':
        out += '\n';
        break;
      case 'r':
        out += '\r';
        break;
      case 't':
        out += '\t';
        break;
      default:
        out += escaped;
        break;
      }
    } else {
      out += c;
    }
  }
  return out;
}

static int json_int_value(const std::string &json, const char *key,
                          int fallback = 0) {
  size_t pos = value_start(json, key);
  if (pos == std::string::npos)
    return fallback;
  char *end = nullptr;
  long value = std::strtol(json.c_str() + pos, &end, 10);
  return end == json.c_str() + pos ? fallback : (int)value;
}

static float json_float_value(const std::string &json, const char *key,
                              float fallback = 0.0f) {
  size_t pos = value_start(json, key);
  if (pos == std::string::npos)
    return fallback;
  char *end = nullptr;
  float value = std::strtof(json.c_str() + pos, &end);
  return end == json.c_str() + pos ? fallback : value;
}

static const char *retained_focus_source_name(::ui::FocusSource source) {
  switch (source) {
  case ::ui::FocusSource::None:
    return "None";
  case ::ui::FocusSource::Keyboard:
    return "Keyboard";
  case ::ui::FocusSource::Gamepad:
    return "Gamepad";
  case ::ui::FocusSource::Mouse:
    return "Mouse";
  case ::ui::FocusSource::Touch:
    return "Touch";
  case ::ui::FocusSource::Programmatic:
    return "Programmatic";
  }
  return "None";
}

static const char *retained_node_role_name(::ui::NodeRole role) {
  switch (role) {
  case ::ui::NodeRole::Generic:
    return "generic";
  case ::ui::NodeRole::Box:
    return "box";
  case ::ui::NodeRole::Text:
    return "text";
  case ::ui::NodeRole::Button:
    return "button";
  case ::ui::NodeRole::Input:
    return "input";
  case ::ui::NodeRole::Checkbox:
    return "checkbox";
  case ::ui::NodeRole::Dialog:
    return "dialog";
  }
  return "generic";
}

static const char *retained_semantic_role_name(::ui::SemanticRole role) {
  switch (role) {
  case ::ui::SemanticRole::Auto:
    return "auto";
  case ::ui::SemanticRole::Button:
    return "button";
  case ::ui::SemanticRole::Checkbox:
    return "checkbox";
  case ::ui::SemanticRole::TextBox:
    return "textbox";
  case ::ui::SemanticRole::Tab:
    return "tab";
  case ::ui::SemanticRole::Dialog:
    return "dialog";
  }
  return "auto";
}

static bool is_text_input_node(const ::ui::NodeSnapshot &node) {
  return node.role == ::ui::NodeRole::Input ||
         node.semantic_role == ::ui::SemanticRole::TextBox;
}

bool ControlMailbox::init(const char *dir) {
  if (!dir || !*dir)
    return false;
  dir_ = dir;
  requests_dir_ = dir_ / "requests";
  replies_dir_ = dir_ / "replies";
  artifacts_dir_ = dir_ / "artifacts";
  std::error_code ec;
  std::filesystem::create_directories(requests_dir_, ec);
  std::filesystem::create_directories(replies_dir_, ec);
  std::filesystem::create_directories(artifacts_dir_, ec);
  if (ec) {
    SDL_Log("control: failed to create %s: %s", dir, ec.message().c_str());
    return false;
  }
  active_ = true;
  write_ready();
  return true;
}

void ControlMailbox::set_game_state_json_provider(
    std::function<std::string()> provider) {
  game_state_json_provider_ = std::move(provider);
}

void ControlMailbox::write_ready(void) {
  std::ostringstream body;
  body << "{"
       << "\"ok\":true,"
       << "\"protocol\":\"sdl3-retained-mailbox-v1\","
       << "\"requests\":\"" << json_escape(requests_dir_.string()) << "\","
       << "\"replies\":\"" << json_escape(replies_dir_.string()) << "\","
       << "\"artifacts\":\"" << json_escape(artifacts_dir_.string()) << "\""
       << "}\n";
  write_text_atomic(dir_ / "ready.json", body.str());
}

void ControlMailbox::write_reply(int id, bool ok, const std::string &body) {
  std::ostringstream reply;
  reply << "{\"id\":" << id << ",\"ok\":" << (ok ? "true" : "false");
  if (!body.empty()) {
    reply << "," << body;
  }
  reply << "}\n";
  write_text_atomic(replies_dir_ / (std::to_string(id) + ".json"), reply.str());
}

void ControlMailbox::write_error(int id, const char *code,
                                 const std::string &message) {
  std::ostringstream body;
  body << "\"code\":\"" << json_escape(code ? code : "ERROR") << "\","
       << "\"error\":\"" << json_escape(message) << "\"";
  write_reply(id, false, body.str());
}

std::string ControlMailbox::state_json(client::ui::UiPipeline &pipeline) {
  client::ui::ClientUi &client_ui = pipeline.client_ui();
  client::ui::UiScreen *top = client_ui.screens().top();
  ::ui::NodeId retained_focused =
      ::ui::focus_focused_id(client_ui.retained_focus());
  ::ui::FocusSource retained_source =
      ::ui::focus_source(client_ui.retained_focus());

  std::ostringstream body;
  body << "\"result\":{"
       << "\"frame\":" << frame_index_ << ","
       << "\"screen_count\":" << client_ui.screens().count() << ","
       << "\"top_screen\":\"" << json_escape(top ? top->debug_name() : "")
       << "\","
       << "\"pending_mutations\":" << client_ui.pending_mutation_count() << ","
       << "\"focused_id\":" << retained_focused << ","
       << "\"focus_source\":\"" << retained_focus_source_name(retained_source)
       << "\","
       << "\"retained_focused_id\":" << retained_focused << ","
       << "\"retained_focus_source\":\""
       << retained_focus_source_name(retained_source) << "\","
       << "\"screens\":[";
  for (int i = 0; i < client_ui.screens().count(); ++i) {
    client::ui::UiScreen *screen = client_ui.screens().at(i);
    if (i)
      body << ",";
    body << "{"
         << "\"entry_id\":" << (screen ? screen->entry_id() : 0) << ","
         << "\"name\":\"" << json_escape(screen ? screen->debug_name() : "")
         << "\","
         << "\"overlay\":"
         << (screen && screen->kind() == client::ui::ScreenKind::Overlay
                 ? "true"
                 : "false")
         << "}";
  }
  body << "],\"focusables\":[";
  bool first_focusable = true;
  const ::ui::FocusRuntime &retained_focus = client_ui.retained_focus();
  std::string retained_scope_name = "RetainedRoot";
  ::ui::NodeSnapshot retained_scope = {};
  if (client_ui.retained_tree().snapshot(retained_focus.active_scope_id,
                                         &retained_scope)) {
    retained_scope_name =
        retained_scope.control_id && retained_scope.control_id[0] != '\0'
            ? retained_scope.control_id
            : retained_scope.type;
  }
  for (int i = 0; i < retained_focus.focusable_count; ++i) {
    const ::ui::FocusableLayout &layout = retained_focus.focusables[i];
    ::ui::NodeSnapshot node = {};
    if (!client_ui.retained_tree().snapshot(layout.id, &node))
      continue;
    if (!first_focusable)
      body << ",";
    first_focusable = false;
    std::string name = node.control_id && node.control_id[0] != '\0'
                           ? node.control_id
                           : node.type;
    body << "{"
         << "\"scope_id\":" << retained_focus.active_scope_id << ","
         << "\"scope_name\":\"" << json_escape(retained_scope_name) << "\","
         << "\"id\":" << layout.id << ","
         << "\"name\":\"" << json_escape(name) << "\","
         << "\"offset\":" << node.control_offset << ","
         << "\"role\":\"" << retained_node_role_name(node.role) << "\","
         << "\"semantic_role\":\""
         << retained_semantic_role_name(node.semantic_role) << "\","
         << "\"value\":\"" << json_escape(node.value ? node.value : "") << "\","
         << "\"disabled\":" << (layout.disabled ? "true" : "false") << ","
         << "\"focused\":"
         << (layout.id == retained_focus.focused_id ? "true" : "false") << ","
         << "\"retained\":true,"
         << "\"rect\":{"
         << "\"x\":" << layout.rect.x << ","
         << "\"y\":" << layout.rect.y << ","
         << "\"w\":" << layout.rect.width << ","
         << "\"h\":" << layout.rect.height << "}}";
  }
  body << "],\"text_inputs\":[";
  bool first_text_input = true;
  for (int i = 0; i < retained_focus.focusable_count; ++i) {
    const ::ui::FocusableLayout &layout = retained_focus.focusables[i];
    ::ui::NodeSnapshot node = {};
    if (!client_ui.retained_tree().snapshot(layout.id, &node) ||
        !is_text_input_node(node))
      continue;
    if (!first_text_input)
      body << ",";
    first_text_input = false;
    std::string name = node.control_id && node.control_id[0] != '\0'
                           ? node.control_id
                           : node.type;
    body << "{"
         << "\"id\":" << layout.id << ","
         << "\"name\":\"" << json_escape(name) << "\","
         << "\"offset\":" << node.control_offset << ","
         << "\"value\":\"" << json_escape(node.value ? node.value : "") << "\","
         << "\"focused\":"
         << (layout.id == retained_focus.focused_id ? "true" : "false") << ","
         << "\"disabled\":" << (layout.disabled ? "true" : "false") << ","
         << "\"caret\":" << node.text_edit.caret << ","
         << "\"selection_start\":" << node.text_edit.selection_start << ","
         << "\"selection_end\":" << node.text_edit.selection_end << ","
         << "\"composition\":\""
         << json_escape(node.text_edit.composition ? node.text_edit.composition
                                                   : "")
         << "\"}";
  }
  body << "]";
  if (game_state_json_provider_) {
    std::string game_json = game_state_json_provider_();
    body << ",\"game\":" << (game_json.empty() ? "null" : game_json);
  }
  body << "}";
  return body.str();
}

void ControlMailbox::poll(::ui::UiInputFrame &ui_input, bool &running,
                          SDL_Window *window,
                          client::ui::UiPipeline &pipeline) {
  if (!active_)
    return;

  std::vector<std::filesystem::path> requests;
  std::error_code ec;
  for (const auto &entry :
       std::filesystem::directory_iterator(requests_dir_, ec)) {
    if (!entry.is_regular_file())
      continue;
    if (entry.path().extension() == ".json") {
      requests.push_back(entry.path());
    }
  }
  std::sort(requests.begin(), requests.end());

  for (const std::filesystem::path &path : requests) {
    std::string raw = read_text(path);
    std::filesystem::remove(path, ec);

    int id = json_int_value(raw, "id", 0);
    if (id == 0) {
      id = std::max(1, std::atoi(path.stem().string().c_str()));
    }
    std::string op = json_string_value(raw, "op");

    if (op == "state" || op == "inspect") {
      write_reply(id, true, state_json(pipeline));
    } else if (op == "quit") {
      running = false;
      write_reply(id, true, "\"result\":{\"quitting\":true}");
    } else if (op == "key") {
      std::string key = json_string_value(raw, "key");
      std::string action = json_string_value(raw, "action", "press");
      SDL_Keycode code = 0;
      if (!keycode_from_name(key.c_str(), &code)) {
        write_error(id, "BAD_KEY", "unknown key: " + key);
        continue;
      }
      if (action == "down" || action == "press") {
        apply_key_down(code, ui_input, &running);
      }
      if (action == "up" || action == "release") {
        apply_key_up(code, ui_input);
      }
      write_reply(id, true, "\"result\":{\"accepted\":true}");
    } else if (op == "text") {
      std::string text = json_string_value(raw, "text");
      apply_text_input(text.c_str(), ui_input);
      write_reply(id, true, "\"result\":{\"accepted\":true}");
    } else if (op == "gamepad") {
      std::string button_name = json_string_value(raw, "button");
      std::string action = json_string_value(raw, "action", "press");
      GamepadButton button = GamepadButton::Confirm;
      if (!gamepad_button_from_name(button_name.c_str(), &button)) {
        write_error(id, "BAD_GAMEPAD_BUTTON",
                    "unknown gamepad button: " + button_name);
        continue;
      }
      if (action == "down" || action == "press") {
        apply_gamepad_button_down(button, ui_input);
      }
      if (action == "up" || action == "release") {
        apply_gamepad_button_up(button, ui_input);
      }
      write_reply(id, true, "\"result\":{\"accepted\":true}");
    } else if (op == "pointer") {
      std::string action = json_string_value(raw, "action", "move");
      pointer_override_ = true;
      pointer_x_ = json_float_value(raw, "x", pointer_x_);
      pointer_y_ = json_float_value(raw, "y", pointer_y_);
      if (action == "press") {
        pointer_down_ = true;
        ui_input.pointer_pressed = true;
        ui_input.pointer_down = true;
        ui_input.source = ::ui::UiFocusSource::Mouse;
      } else if (action == "release") {
        pointer_down_ = false;
        ui_input.pointer_released = true;
        ui_input.pointer_down = false;
        ui_input.source = ::ui::UiFocusSource::Mouse;
      } else if (action == "move") {
        ui_input.pointer_down = pointer_down_;
      } else {
        write_error(id, "BAD_POINTER_ACTION",
                    "unknown pointer action: " + action);
        continue;
      }
      write_reply(id, true, "\"result\":{\"accepted\":true}");
    } else if (op == "resize") {
      int w = json_int_value(raw, "w", 800);
      int h = json_int_value(raw, "h", 500);
      if (!window || !SDL_SetWindowSize(window, w, h)) {
        write_error(id, "RESIZE_FAILED", SDL_GetError());
        continue;
      }
      write_reply(id, true,
                  "\"result\":{\"w\":" + std::to_string(w) +
                      ",\"h\":" + std::to_string(h) + "}");
    } else if (op == "wait_frames" || op == "wait" || op == "step") {
      int n = std::max(0, json_int_value(raw, "n", 1));
      pending_waits_.push_back(
          {.id = id, .target_frame = frame_index_ + (uint64_t)n});
    } else if (op == "screenshot") {
      std::string out = json_string_value(raw, "out");
      if (out.empty()) {
        out = (artifacts_dir_ / ("screenshot-" + std::to_string(id) + ".bmp"))
                  .string();
      }
      pending_screenshots_.push_back({.id = id, .out = out});
    } else if (op == "capture_frames") {
      int count = std::max(1, json_int_value(raw, "count", 3));
      std::string dir = json_string_value(raw, "out_dir");
      if (dir.empty()) {
        dir = (artifacts_dir_ / ("capture-" + std::to_string(id))).string();
      }
      std::filesystem::create_directories(dir, ec);
      pending_captures_.push_back({
          .id = id,
          .dir = dir,
          .remaining = count,
      });
    } else if (op == "render_mode") {
      // Forward the raw slug to the loop (app/ maps it to a RenderMode and
      // applies it after poll). Keeps platform/ free of renderer/ deps.
      std::string mode = json_string_value(raw, "mode");
      pending_render_mode_ = mode;
      write_reply(id, true,
                  "\"result\":{\"mode\":\"" + json_escape(mode) + "\"}");
    } else {
      write_error(id, "BAD_OP", "unknown op: " + op);
    }
  }
}

bool ControlMailbox::take_pending_render_mode(std::string *out) {
  if (pending_render_mode_.empty())
    return false;
  if (out)
    *out = pending_render_mode_;
  pending_render_mode_.clear();
  return true;
}

bool ControlMailbox::apply_pointer_override(float &x, float &y,
                                            bool &down) const {
  if (!active_ || !pointer_override_)
    return false;
  x = pointer_x_;
  y = pointer_y_;
  down = pointer_down_;
  return true;
}

bool ControlMailbox::save_screenshot(SDL_Renderer *renderer,
                                     const std::filesystem::path &out,
                                     std::string *error) {
  if (!out.parent_path().empty()) {
    std::filesystem::create_directories(out.parent_path());
  }
  SDL_Surface *surface = SDL_RenderReadPixels(renderer, nullptr);
  if (!surface) {
    if (error)
      *error = SDL_GetError();
    return false;
  }
  bool ok = SDL_SaveBMP(surface, out.string().c_str());
  if (!ok && error)
    *error = SDL_GetError();
  SDL_DestroySurface(surface);
  return ok;
}

void ControlMailbox::capture_after_render(SDL_Renderer *renderer,
                                          client::ui::UiPipeline &pipeline) {
  if (!active_)
    return;

  for (const PendingScreenshot &shot : pending_screenshots_) {
    std::string error;
    if (save_screenshot(renderer, shot.out, &error)) {
      write_reply(shot.id, true,
                  "\"result\":{\"out\":\"" + json_escape(shot.out.string()) +
                      "\"}");
    } else {
      write_error(shot.id, "SCREENSHOT_FAILED", error);
    }
  }
  pending_screenshots_.clear();

  for (auto it = pending_captures_.begin(); it != pending_captures_.end();) {
    char name[64];
    std::snprintf(name, sizeof(name), "frame-%04d.bmp", it->index++);
    std::filesystem::path out = it->dir / name;
    std::string error;
    if (!save_screenshot(renderer, out, &error)) {
      write_error(it->id, "CAPTURE_FAILED", error);
      it = pending_captures_.erase(it);
      continue;
    }
    it->frames.push_back(out.string());
    it->remaining -= 1;
    if (it->remaining <= 0) {
      std::ostringstream frames;
      frames << "\"result\":{\"frames\":[";
      for (size_t i = 0; i < it->frames.size(); ++i) {
        if (i)
          frames << ",";
        frames << "\"" << json_escape(it->frames[i]) << "\"";
      }
      frames << "]}";
      write_reply(it->id, true, frames.str());
      it = pending_captures_.erase(it);
    } else {
      ++it;
    }
  }

  (void)pipeline;
}

void ControlMailbox::finish_frame(client::ui::UiPipeline &pipeline) {
  if (!active_)
    return;
  frame_index_ += 1;
  for (auto it = pending_waits_.begin(); it != pending_waits_.end();) {
    if (frame_index_ >= it->target_frame) {
      write_reply(it->id, true, state_json(pipeline));
      it = pending_waits_.erase(it);
    } else {
      ++it;
    }
  }
}

void ControlMailbox::shutdown(void) {
  if (!active_)
    return;
  write_text_atomic(dir_ / "shutdown.json", "{\"ok\":true}\n");
}

} // namespace platform
