#include "policy.h"
#include "solar.h"

#include <cmath>
#include <cstdint>
#include <cstdio>

#define CHECK(expression) do { \
    if (!(expression)) { \
        std::fprintf(stderr, "CHECK failed at line %d: %s\n", __LINE__, #expression); \
        return 1; \
    } \
} while (false)

using geodark::calculate_solar_schedule;
using geodark::EventKind;

namespace {

// Civil date to Unix days (Gregorian calendar, UTC).
std::int64_t days_from_civil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const unsigned yoe_days = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<std::int64_t>(era) * 146097 + yoe_days - 719468;
}

std::int64_t utc(int year, unsigned month, unsigned day, int hour = 0, int minute = 0) {
    return days_from_civil(year, month, day) * 86400 + hour * 3600 + minute * 60;
}

} // namespace

int main() {
    // Shanghai: startup after a missed sunset must select dark immediately.
    const auto shanghai_day = calculate_solar_schedule(utc(2026, 3, 20, 4), 31.23, 121.47, 0, 0);
    const auto shanghai_night = calculate_solar_schedule(utc(2026, 3, 20, 14), 31.23, 121.47, 0, 0);
    CHECK(shanghai_day.light_now);
    CHECK(!shanghai_night.light_now);
    CHECK(shanghai_day.next_switch_utc > utc(2026, 3, 20, 4));
    CHECK(shanghai_day.next_switch_utc < utc(2026, 3, 20, 14));

    // Waking after sunset and a manual clock jump give the same current-state result.
    const auto before_sleep = calculate_solar_schedule(utc(2026, 3, 20, 9), 31.23, 121.47, 0, 0);
    const auto after_wake = calculate_solar_schedule(utc(2026, 3, 20, 12), 31.23, 121.47, 0, 0);
    CHECK(before_sleep.light_now && !after_wake.light_now);

    // The same UTC instant can be daytime on one coast and nighttime on another.
    const auto los_angeles = calculate_solar_schedule(utc(2026, 6, 22, 1), 34.05, -118.24, 0, 0);
    const auto new_york = calculate_solar_schedule(utc(2026, 6, 22, 1), 40.71, -74.01, 0, 0);
    CHECK(los_angeles.light_now && !new_york.light_now);

    // DST is a display conversion only; UTC solar instants remain ordered.
    const auto dst = calculate_solar_schedule(utc(2026, 3, 8, 12), 40.71, -74.01, 0, 0);
    CHECK(dst.next_switch_utc > utc(2026, 3, 8, 12));
    CHECK(dst.next_sunrise_utc > utc(2026, 3, 8, 12));

    // Coordinate changes across the international date line do not break scheduling.
    for (double longitude : {179.9, -179.9}) {
        const auto date_line = calculate_solar_schedule(utc(2026, 9, 24, 0), 0.0, longitude, 0, 0);
        CHECK(date_line.next_switch_utc > utc(2026, 9, 24, 0));
        CHECK(date_line.next_sunrise_utc > utc(2026, 9, 24, 0));
        CHECK(date_line.next_sunset_utc > utc(2026, 9, 24, 0));
    }

    const auto polar_summer = calculate_solar_schedule(utc(2026, 6, 21, 12), 80.0, 0.0, 0, 0);
    const auto polar_winter = calculate_solar_schedule(utc(2026, 12, 21, 12), 80.0, 0.0, 0, 0);
    CHECK(polar_summer.polar && polar_summer.light_now);
    CHECK(polar_winter.polar && !polar_winter.light_now);

    // Positive offsets delay the event; negative offsets advance it.
    const auto base = calculate_solar_schedule(utc(2026, 3, 20, 0), 51.48, 0.0, 0, 0);
    const auto shifted = calculate_solar_schedule(utc(2026, 3, 20, 0), 51.48, 0.0, 30, -30);
    CHECK(base.next_sunrise_utc > 0 && base.next_sunset_utc > 0);
    CHECK(shifted.next_sunrise_utc - base.next_sunrise_utc == 1800);
    CHECK(shifted.next_sunset_utc - base.next_sunset_utc == -1800);

    const auto invalid = calculate_solar_schedule(utc(2026, 3, 20), 91.0, 0.0, 0, 0);
    CHECK(invalid.events.empty());

    // A user choice is kept until the next solar transition, including reboot.
    const auto next_sunset = shanghai_day.next_switch_utc;
    const geodark::ManualOverride manual{true, true, next_sunset};
    const auto still_day = geodark::decide_theme(true, true, utc(2026, 3, 20, 8),
                                                  next_sunset, manual, false);
    CHECK(!still_day.apply && still_day.manual_override.active);
    const auto missed_sunset = geodark::decide_theme(true, false, utc(2026, 3, 20, 14),
                                                      shanghai_night.next_switch_utc,
                                                      manual, true);
    CHECK(missed_sunset.apply && !missed_sunset.light);
    CHECK(!missed_sunset.manual_override.active);
    const auto user_changed = geodark::decide_theme(true, true, utc(2026, 3, 20, 8),
                                                     next_sunset, {}, true);
    CHECK(!user_changed.apply && user_changed.manual_override.active);
    CHECK(user_changed.manual_override.until_utc == next_sunset);
    const auto disabled = geodark::decide_theme(false, true, utc(2026, 3, 20, 8),
                                                 next_sunset, manual, false);
    CHECK(!disabled.apply && !disabled.manual_override.active);
    return 0;
}

