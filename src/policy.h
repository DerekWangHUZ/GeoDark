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

// Pure comparison helper: true when a readable theme value differs from the
// solar expectation. Unreadable values (-1) never count as divergence.
bool theme_diverges(int actual_light, bool expected_light);

// Readable theme values (-1 = unreadable).
struct ThemeObservation {
    int apps = -1;
    int system = -1;
};

// True when the live theme is a user change that must be protected rather
// than overwritten. In-session it compares against the phase GeoDark itself
// last applied; at the first decision of a run it also protects a choice made
// while GeoDark was not running, unless the solar phase moved on in the
// meantime (a missed transition must catch up) or the phase is polar.
bool external_theme_change(const ThemeObservation& actual, int session_applied_light,
                           bool has_persisted_state, bool persisted_expected_light,
                           bool expected_light, bool polar);

} // namespace geodark
