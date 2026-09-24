#pragma once

#include <cstdint>

namespace geodark {

struct ManualOverride {
    bool active = false;
    bool baseline_light = false;
    std::int64_t until_utc = 0;
};

struct ThemeDecision {
    bool apply = false;
    bool light = false;
    ManualOverride manual_override;
};

ThemeDecision decide_theme(bool auto_enabled, bool expected_light,
                           std::int64_t now_utc, std::int64_t next_switch_utc,
                           ManualOverride previous_override, bool external_theme_change);

} // namespace geodark
