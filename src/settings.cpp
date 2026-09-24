#include "settings.h"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <string>

namespace geodark {
namespace {

constexpr std::int64_t unix_to_filetime_seconds = 11644473600LL;

HKEY open_key(const wchar_t* path, REGSAM access, bool create) {
    HKEY key = nullptr;
    const LONG status = create
        ? RegCreateKeyExW(HKEY_CURRENT_USER, path, 0, nullptr, 0, access, nullptr, &key, nullptr)
        : RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, access, &key);
    return status == ERROR_SUCCESS ? key : nullptr;
}

DWORD read_dword(HKEY key, const wchar_t* name, DWORD fallback) {
    DWORD type = 0;
    DWORD size = sizeof(DWORD);
    DWORD value = 0;
    return key && RegQueryValueExW(key, name, nullptr, &type,
                                  reinterpret_cast<BYTE*>(&value), &size) == ERROR_SUCCESS &&
                   type == REG_DWORD && size == sizeof(DWORD)
        ? value : fallback;
}

std::wstring read_string(HKEY key, const wchar_t* name) {
    if (!key) return {};
    DWORD type = 0;
    DWORD size = 0;
    if (RegQueryValueExW(key, name, nullptr, &type, nullptr, &size) != ERROR_SUCCESS ||
        type != REG_SZ || size < sizeof(wchar_t) || size > 4096) return {};
    std::wstring value(size / sizeof(wchar_t), L'\0');
    if (RegQueryValueExW(key, name, nullptr, &type,
                         reinterpret_cast<BYTE*>(value.data()), &size) != ERROR_SUCCESS) return {};
    value.resize(wcsnlen_s(value.c_str(), value.size()));
    return value;
}

double parse_double(const std::wstring& value, double fallback) {
    if (value.empty()) return fallback;
    wchar_t* end = nullptr;
    const double parsed = wcstod(value.c_str(), &end);
    return end != value.c_str() && *end == L'\0' && std::isfinite(parsed) ? parsed : fallback;
}

std::int64_t parse_i64(const std::wstring& value, std::int64_t fallback = 0) {
    if (value.empty()) return fallback;
    wchar_t* end = nullptr;
    const auto parsed = _wcstoi64(value.c_str(), &end, 10);
    return end != value.c_str() && *end == L'\0' ? parsed : fallback;
}

bool write_dword(HKEY key, const wchar_t* name, DWORD value) {
    return key && RegSetValueExW(key, name, 0, REG_DWORD,
                                reinterpret_cast<const BYTE*>(&value), sizeof(value)) == ERROR_SUCCESS;
}

bool write_string(HKEY key, const wchar_t* name, const std::wstring& value) {
    return key && RegSetValueExW(key, name, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(value.c_str()),
        static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
}

std::wstring decimal(double value) {
    wchar_t buffer[64];
    swprintf_s(buffer, L"%.8f", value);
    return buffer;
}

std::wstring number(std::int64_t value) { return std::to_wstring(value); }

} // namespace

Config load_config() {
    Config result;
    HKEY key = open_key(config_key, KEY_READ, false);
    if (!key) return result;
    result.auto_enabled = read_dword(key, L"AutoEnabled", 1) != 0;
    result.location_mode = read_dword(key, L"LocationMode", 0) == 1
        ? LocationMode::Manual : LocationMode::Windows;
    result.has_manual_coordinates = read_dword(key, L"HasManualCoordinates", 0) != 0;
    result.manual_latitude = parse_double(read_string(key, L"ManualLatitude"), 0.0);
    result.manual_longitude = parse_double(read_string(key, L"ManualLongitude"), 0.0);
    result.sunrise_offset_minutes = std::clamp(
        static_cast<int>(read_dword(key, L"SunriseOffsetMinutes", 0)), -120, 120);
    result.sunset_offset_minutes = std::clamp(
        static_cast<int>(read_dword(key, L"SunsetOffsetMinutes", 0)), -120, 120);
    RegCloseKey(key);
    if (result.manual_latitude < -90.0 || result.manual_latitude > 90.0 ||
        result.manual_longitude < -180.0 || result.manual_longitude > 180.0)
        result.has_manual_coordinates = false;
    return result;
}

bool save_config(const Config& config) {
    if (config.sunrise_offset_minutes < -120 || config.sunrise_offset_minutes > 120 ||
        config.sunset_offset_minutes < -120 || config.sunset_offset_minutes > 120 ||
        !std::isfinite(config.manual_latitude) || !std::isfinite(config.manual_longitude) ||
        config.manual_latitude < -90 || config.manual_latitude > 90 ||
        config.manual_longitude < -180 || config.manual_longitude > 180) return false;
    HKEY key = open_key(config_key, KEY_SET_VALUE, true);
    if (!key) return false;
    const bool success =
        write_dword(key, L"AutoEnabled", config.auto_enabled ? 1 : 0) &&
        write_dword(key, L"LocationMode", static_cast<DWORD>(config.location_mode)) &&
        write_dword(key, L"HasManualCoordinates", config.has_manual_coordinates ? 1 : 0) &&
        write_string(key, L"ManualLatitude", decimal(config.manual_latitude)) &&
        write_string(key, L"ManualLongitude", decimal(config.manual_longitude)) &&
        write_dword(key, L"SunriseOffsetMinutes", static_cast<DWORD>(config.sunrise_offset_minutes)) &&
        write_dword(key, L"SunsetOffsetMinutes", static_cast<DWORD>(config.sunset_offset_minutes));
    RegCloseKey(key);
    return success;
}

State load_state() {
    State result;
    HKEY key = open_key(state_key, KEY_READ, false);
    if (!key) return result;
    result.has_location = read_dword(key, L"HasLocation", 0) != 0;
    result.latitude = parse_double(read_string(key, L"Latitude"), 0.0);
    result.longitude = parse_double(read_string(key, L"Longitude"), 0.0);
    result.accuracy_meters = parse_double(read_string(key, L"AccuracyMeters"), 0.0);
    result.location_utc = parse_i64(read_string(key, L"LocationUtc"));
    result.last_attempt_utc = parse_i64(read_string(key, L"LastAttemptUtc"));
    result.active_source = static_cast<LocationSource>(read_dword(key, L"ActiveSource", 0));
    result.location_error = read_string(key, L"LocationError");
    result.theme_error = read_string(key, L"ThemeError");
    result.expected_light = read_dword(key, L"ExpectedLight", 0) != 0;
    result.next_switch_utc = parse_i64(read_string(key, L"NextSwitchUtc"));
    result.next_sunrise_utc = parse_i64(read_string(key, L"NextSunriseUtc"));
    result.next_sunset_utc = parse_i64(read_string(key, L"NextSunsetUtc"));
    result.polar = read_dword(key, L"Polar", 0) != 0;
    result.override_active = read_dword(key, L"OverrideActive", 0) != 0;
    result.override_phase_light = read_dword(key, L"OverridePhaseLight", 0) != 0;
    result.override_until_utc = parse_i64(read_string(key, L"OverrideUntilUtc"));
    RegCloseKey(key);
    if (!std::isfinite(result.latitude) || !std::isfinite(result.longitude) ||
        result.latitude < -90.0 || result.latitude > 90.0 ||
        result.longitude < -180.0 || result.longitude > 180.0) result.has_location = false;
    return result;
}

bool save_state(const State& state) {
    HKEY key = open_key(state_key, KEY_SET_VALUE, true);
    if (!key) return false;
    const bool success =
        write_dword(key, L"HasLocation", state.has_location ? 1 : 0) &&
        write_string(key, L"Latitude", decimal(state.latitude)) &&
        write_string(key, L"Longitude", decimal(state.longitude)) &&
        write_string(key, L"AccuracyMeters", decimal(state.accuracy_meters)) &&
        write_string(key, L"LocationUtc", number(state.location_utc)) &&
        write_string(key, L"LastAttemptUtc", number(state.last_attempt_utc)) &&
        write_dword(key, L"ActiveSource", static_cast<DWORD>(state.active_source)) &&
        write_string(key, L"LocationError", state.location_error) &&
        write_string(key, L"ThemeError", state.theme_error) &&
        write_dword(key, L"ExpectedLight", state.expected_light ? 1 : 0) &&
        write_string(key, L"NextSwitchUtc", number(state.next_switch_utc)) &&
        write_string(key, L"NextSunriseUtc", number(state.next_sunrise_utc)) &&
        write_string(key, L"NextSunsetUtc", number(state.next_sunset_utc)) &&
        write_dword(key, L"Polar", state.polar ? 1 : 0) &&
        write_dword(key, L"OverrideActive", state.override_active ? 1 : 0) &&
        write_dword(key, L"OverridePhaseLight", state.override_phase_light ? 1 : 0) &&
        write_string(key, L"OverrideUntilUtc", number(state.override_until_utc));
    RegCloseKey(key);
    return success;
}

ThemeValues read_theme() {
    ThemeValues result;
    HKEY key = open_key(theme_key, KEY_READ, false);
    if (!key) return result;
    result.apps = static_cast<int>(read_dword(key, L"AppsUseLightTheme", 0xFFFFFFFF));
    result.system = static_cast<int>(read_dword(key, L"SystemUsesLightTheme", 0xFFFFFFFF));
    RegCloseKey(key);
    return result;
}

bool apply_theme(bool light) {
    const ThemeValues current = read_theme();
    const int value = light ? 1 : 0;
    if (current.apps == value && current.system == value) return true;
    HKEY key = open_key(theme_key, KEY_SET_VALUE, true);
    if (!key) return false;
    bool success = true;
    if (current.apps != value) success = write_dword(key, L"AppsUseLightTheme", value) && success;
    if (current.system != value) success = write_dword(key, L"SystemUsesLightTheme", value) && success;
    RegCloseKey(key);
    if (!success) return false;
    DWORD_PTR ignored = 0;
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                        reinterpret_cast<LPARAM>(L"ImmersiveColorSet"),
                        SMTO_ABORTIFHUNG | SMTO_BLOCK, 1500, &ignored);
    return true;
}

std::int64_t utc_now() {
    FILETIME ft{};
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER value{};
    value.LowPart = ft.dwLowDateTime;
    value.HighPart = ft.dwHighDateTime;
    return static_cast<std::int64_t>(value.QuadPart / 10000000ULL) - unix_to_filetime_seconds;
}

std::wstring format_local_time(std::int64_t utc_seconds) {
    if (utc_seconds <= 0) return L"—";
    const ULONGLONG ticks = static_cast<ULONGLONG>(utc_seconds + unix_to_filetime_seconds) * 10000000ULL;
    FILETIME ft{static_cast<DWORD>(ticks), static_cast<DWORD>(ticks >> 32)};
    SYSTEMTIME utc{}, local{};
    if (!FileTimeToSystemTime(&ft, &utc) || !SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local))
        return L"—";
    wchar_t text[48];
    swprintf_s(text, L"%04u-%02u-%02u %02u:%02u", local.wYear, local.wMonth,
               local.wDay, local.wHour, local.wMinute);
    return text;
}

std::wstring module_directory() {
    wchar_t path[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return {};
    std::wstring result(path, length);
    const auto slash = result.find_last_of(L"\\/");
    return slash == std::wstring::npos ? std::wstring{} : result.substr(0, slash);
}

std::wstring core_executable_path() { return module_directory() + L"\\GeoDark.exe"; }

bool startup_enabled() {
    HKEY key = open_key(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", KEY_READ, false);
    if (!key) return false;
    const bool enabled = !read_string(key, L"GeoDark").empty();
    RegCloseKey(key);
    return enabled;
}

bool set_startup_enabled(bool enabled) {
    HKEY key = open_key(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", KEY_SET_VALUE, enabled);
    if (!key) return !enabled;
    const std::wstring command = L"\"" + core_executable_path() + L"\"";
    const LONG deletion = enabled ? ERROR_SUCCESS : RegDeleteValueW(key, L"GeoDark");
    const bool success = enabled ? write_string(key, L"GeoDark", command)
                                 : deletion == ERROR_SUCCESS || deletion == ERROR_FILE_NOT_FOUND;
    RegCloseKey(key);
    return success;
}

bool core_running() {
    HANDLE mutex = OpenMutexW(SYNCHRONIZE, FALSE, core_mutex_name);
    if (!mutex) return false;
    CloseHandle(mutex);
    return true;
}

bool launch_core() {
    if (core_running()) return true;
    std::wstring command = L"\"" + core_executable_path() + L"\"";
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};
    const BOOL success = CreateProcessW(core_executable_path().c_str(), command.data(),
                                        nullptr, nullptr, FALSE, 0, nullptr, nullptr,
                                        &startup, &process);
    if (success) {
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }
    return success != FALSE;
}

bool signal_location_refresh() {
    HANDLE event = OpenEventW(EVENT_MODIFY_STATE, FALSE, refresh_event_name);
    if (!event) return false;
    const BOOL signaled = SetEvent(event);
    CloseHandle(event);
    return signaled != FALSE;
}

bool signal_manual_override() {
    HANDLE event = OpenEventW(EVENT_MODIFY_STATE, FALSE, manual_event_name);
    if (!event) return false;
    const BOOL signaled = SetEvent(event);
    CloseHandle(event);
    return signaled != FALSE;
}

} // namespace geodark
