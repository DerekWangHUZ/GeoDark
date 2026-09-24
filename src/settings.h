#pragma once

#include <windows.h>

#include <cstdint>
#include <string>

namespace geodark {

inline constexpr wchar_t config_key[] = L"Software\\GeoDark\\Config";
inline constexpr wchar_t state_key[] = L"Software\\GeoDark\\State";
inline constexpr wchar_t theme_key[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
inline constexpr wchar_t refresh_event_name[] = L"Local\\GeoDarkRefreshLocation";
inline constexpr wchar_t manual_event_name[] = L"Local\\GeoDarkManualOverride";
inline constexpr wchar_t core_mutex_name[] = L"Local\\GeoDarkCore";

enum class LocationMode : DWORD { Windows = 0, Manual = 1 };
enum class LocationSource : DWORD { None = 0, Windows = 1, Cached = 2, Manual = 3 };

struct Config {
    bool auto_enabled = true;
    LocationMode location_mode = LocationMode::Windows;
    double manual_latitude = 0.0;
    double manual_longitude = 0.0;
    bool has_manual_coordinates = false;
    int sunrise_offset_minutes = 0;
    int sunset_offset_minutes = 0;
};

struct State {
    bool has_location = false;
    double latitude = 0.0;
    double longitude = 0.0;
    double accuracy_meters = 0.0;
    std::int64_t location_utc = 0;
    std::int64_t last_attempt_utc = 0;
    LocationSource active_source = LocationSource::None;
    std::wstring location_error;
    std::wstring theme_error;
    bool expected_light = false;
    std::int64_t next_switch_utc = 0;
    std::int64_t next_sunrise_utc = 0;
    std::int64_t next_sunset_utc = 0;
    bool polar = false;
    bool override_active = false;
    bool override_phase_light = false;
    std::int64_t override_until_utc = 0;
};

struct ThemeValues {
    int apps = -1;
    int system = -1;
};

Config load_config();
bool save_config(const Config& config);
State load_state();
bool save_state(const State& state);
ThemeValues read_theme();
bool apply_theme(bool light);
std::int64_t utc_now();
std::wstring format_local_time(std::int64_t utc_seconds);
std::wstring module_directory();
std::wstring core_executable_path();
bool startup_enabled();
bool set_startup_enabled(bool enabled);
bool core_running();
bool launch_core();
bool signal_location_refresh();
bool signal_manual_override();

} // namespace geodark
