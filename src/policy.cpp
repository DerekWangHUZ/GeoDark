#include "policy.h"

namespace geodark {

ThemeDecision decide_theme(bool auto_enabled, bool expected_light,
                           std::int64_t now_utc, std::int64_t next_switch_utc,
                           ManualOverride previous_override, bool external_theme_change) {
    ThemeDecision decision;
    decision.light = expected_light;
    if (!auto_enabled) return decision;

    decision.manual_override = previous_override;
    if (previous_override.active) {
        if (previous_override.baseline_light != expected_light ||
            (previous_override.until_utc > 0 && now_utc >= previous_override.until_utc)) {
            decision.manual_override = {};
        } else {
            decision.manual_override.until_utc = next_switch_utc;
        }
    } else if (external_theme_change) {
        decision.manual_override = {true, expected_light, next_switch_utc};
    }
    decision.apply = !decision.manual_override.active;
    return decision;
}

} // namespace geodark
