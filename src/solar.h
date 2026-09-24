#pragma once

#include <cstdint>
#include <vector>

namespace geodark {

enum class EventKind { Sunrise, Sunset };

struct SolarEvent {
    std::int64_t utc_seconds;
    EventKind kind;
};

struct SolarSchedule {
    bool light_now = false;
    bool polar = false;
    std::int64_t next_switch_utc = 0;
    std::int64_t next_sunrise_utc = 0;
    std::int64_t next_sunset_utc = 0;
    std::vector<SolarEvent> events;
};

// Longitude is positive east. UTC seconds use the Unix epoch.
double solar_elevation_degrees(std::int64_t utc_seconds, double latitude, double longitude);
SolarSchedule calculate_solar_schedule(std::int64_t now_utc, double latitude,
                                       double longitude, int sunrise_offset_minutes,
                                       int sunset_offset_minutes);

} // namespace geodark
