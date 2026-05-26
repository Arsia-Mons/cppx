#pragma once

#include <stdint.h>

namespace client::ui {

using UiScreenEntryId = uint32_t;

class UiScreen {
public:
    virtual ~UiScreen() = default;

    UiScreenEntryId entry_id() const { return entry_id_; }
    void set_entry_id(UiScreenEntryId id) { entry_id_ = id; }

    virtual const char *debug_name() const = 0;
    virtual bool is_overlay() const { return false; }
    virtual void build_ui() = 0;

private:
    UiScreenEntryId entry_id_ = 0;
};

} // namespace client::ui
