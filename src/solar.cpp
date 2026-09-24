#include "solar.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace geodark {
namespace {

constexpr double pi = 3.1415926535897932384626433832795;
constexpr double radians(double degrees) { return degrees * pi / 180.0; }
constexpr double degrees(double radians_value) { return radians_value * 180.0 / pi; }
constexpr std::int64_t day_seconds = 86400;
constexpr double horizon_degrees = -0.833;

double normalized_degrees(double value) {
    value = std::fmod(value, 360.0);
    return value < 0.0 ? value + 360.0 : value;
}

std::int64_t floor_day(std::int64_t seconds) {
    std::int64_t quotient = seconds / day_seconds;
    if (seconds < 0 && seconds % day_seconds != 0) --quotient;
    return quotient * day_seconds;
}

double horizon_delta(std::int64_t utc, double latitude, double longitude) {
    return solar_elevation_degrees(utc, latitude, longitude) - horizon_degrees;
}

} // namespace

double solar_elevation_degrees(std::int64_t utc_seconds, double latitude, double longitude) {
    if (!std::isfinite(latitude) || !std::isfinite(longitude) ||
        latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // NOAA/Meeus solar coordinates and equation of time, evaluated at UTC.
    const double jd = static_cast<double>(utc_seconds) / 86400.0 + 2440587.5;
    const double t = (jd - 2451545.0) / 36525.0;
    const double l0 = normalized_degrees(280.46646 + t * (36000.76983 + 0.0003032 * t));
    const double m = 357.52911 + t * (35999.05029 - 0.0001537 * t);
    const double e = 0.016708634 - t * (0.000042037 + 0.0000001267 * t);
    const double c = std::sin(radians(m)) * (1.914602 - t * (0.004817 + 0.000014 * t)) +
                     std::sin(radians(2.0 * m)) * (0.019993 - 0.000101 * t) +
                     std::sin(radians(3.0 * m)) * 0.000289;
    const double omega = 125.04 - 1934.136 * t;
    const double lambda = l0 + c - 0.00569 - 0.00478 * std::sin(radians(omega));
    const double mean_obliquity = 23.0 +
        (26.0 + (21.448 - t * (46.815 + t * (0.00059 - t * 0.001813))) / 60.0) / 60.0;
    const double obliquity = mean_obliquity + 0.00256 * std::cos(radians(omega));
    const double declination = std::asin(std::sin(radians(obliquity)) * std::sin(radians(lambda)));
    const double y = std::pow(std::tan(radians(obliquity) / 2.0), 2.0);
    const double equation_minutes = 4.0 * degrees(
        y * std::sin(2.0 * radians(l0)) - 2.0 * e * std::sin(radians(m)) +
        4.0 * e * y * std::sin(radians(m)) * std::cos(2.0 * radians(l0)) -
        0.5 * y * y * std::sin(4.0 * radians(l0)) -
        1.25 * e * e * std::sin(2.0 * radians(m)));

    double minutes = std::fmod(static_cast<double>(utc_seconds) / 60.0 +
                                   equation_minutes + 4.0 * longitude,
                               1440.0);
    if (minutes < 0.0) minutes += 1440.0;
    const double hour_angle = radians(minutes / 4.0 - 180.0);
    const double sine_elevation = std::sin(radians(latitude)) * std::sin(declination) +
        std::cos(radians(latitude)) * std::cos(declination) * std::cos(hour_angle);
    return degrees(std::asin(std::clamp(sine_elevation, -1.0, 1.0)));
}

SolarSchedule calculate_solar_schedule(std::int64_t now_utc, double latitude,
                                       double longitude, int sunrise_offset_minutes,
                                       int sunset_offset_minutes) {
    SolarSchedule result;
    if (!std::isfinite(latitude) || !std::isfinite(longitude) ||
        latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) {
        return result;
    }

    // Sampling the continuous solar elevation avoids local-calendar and date-line
    // assumptions. Five-minute samples are bisected to one second at each crossing.
    const std::int64_t start = floor_day(now_utc) - 3 * day_seconds;
    const std::int64_t end = floor_day(now_utc) + 5 * day_seconds;
    constexpr std::int64_t step = 300;
    double previous = horizon_delta(start, latitude, longitude);
    for (std::int64_t at = start + step; at <= end; at += step) {
        const double current = horizon_delta(at, latitude, longitude);
        if ((previous < 0.0 && current >= 0.0) || (previous >= 0.0 && current < 0.0)) {
            const bool rising = current >= 0.0;
            std::int64_t low = at - step;
            std::int64_t high = at;
            while (high - low > 1) {
                const std::int64_t mid = low + (high - low) / 2;
                const bool above = horizon_delta(mid, latitude, longitude) >= 0.0;
                if (above == rising) high = mid;
                else low = mid;
            }
            const int offset = rising ? sunrise_offset_minutes : sunset_offset_minutes;
            result.events.push_back({high + static_cast<std::int64_t>(offset) * 60,
                                     rising ? EventKind::Sunrise : EventKind::Sunset});
        }
        previous = current;
    }

    std::sort(result.events.begin(), result.events.end(), [](const SolarEvent& a, const SolarEvent& b) {
        if (a.utc_seconds != b.utc_seconds) return a.utc_seconds < b.utc_seconds;
        return static_cast<int>(a.kind) < static_cast<int>(b.kind);
    });
    result.polar = result.events.empty();
    result.light_now = horizon_delta(now_utc, latitude, longitude) >= 0.0;
    for (const auto& event : result.events) {
        if (event.utc_seconds <= now_utc)
            result.light_now = event.kind == EventKind::Sunrise;
    }
    bool future_light = result.light_now;
    for (const auto& event : result.events) {
        if (event.utc_seconds <= now_utc) continue;
        const bool after_event_light = event.kind == EventKind::Sunrise;
        if (result.next_switch_utc == 0 && future_light != after_event_light)
            result.next_switch_utc = event.utc_seconds;
        future_light = after_event_light;
        if (event.kind == EventKind::Sunrise && result.next_sunrise_utc == 0)
            result.next_sunrise_utc = event.utc_seconds;
        if (event.kind == EventKind::Sunset && result.next_sunset_utc == 0)
            result.next_sunset_utc = event.utc_seconds;
    }
    return result;
}

} // namespace geodark
