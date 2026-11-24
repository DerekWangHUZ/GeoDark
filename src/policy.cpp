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

bool theme_diverges(int actual_light, bool expected_light) {
    if (actual_light < 0) return false;
    return (actual_light != 0) != expected_light;
}

bool external_theme_change(const ThemeObservation& actual, int session_applied_light,
                           bool has_persisted_state, bool persisted_expected_light,
                           bool expected_light, bool polar) {
    if (session_applied_light >= 0) {
        // Steady state: divergence from what GeoDark itself last applied is a
        // user change. Never compare against the freshly computed expectation
        // here, because at a solar boundary the expectation flips before the
        // new theme is applied and that must not read as user intent.
        const bool applied_light = session_applied_light != 0;
        return theme_diverges(actual.apps, applied_light) ||
               theme_diverges(actual.system, applied_light);
    }
    if (!has_persisted_state || polar) return false;
    if (persisted_expected_light != expected_light) return false; // missed transition: catch up
    return theme_diverges(actual.apps, expected_light) ||
           theme_diverges(actual.system, expected_light);
}

} // namespace geodark
