#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <netioapi.h>

#include "location.h"
#include "policy.h"
#include "settings.h"
#include "solar.h"

#include <winrt/Windows.Devices.Geolocation.h>

#include <algorithm>
#include <cstdint>
#include <cwchar>
#include <memory>
#include <string>

namespace geodark {
namespace {

constexpr UINT network_changed_message = WM_APP + 2;
constexpr UINT access_result_message = WM_APP + 3;
constexpr std::int64_t location_interval_seconds = 6 * 3600;
constexpr std::int64_t day_seconds = 24 * 3600;
constexpr std::int64_t unix_to_filetime_seconds = 11644473600LL;

class CoreApp {
public:
    explicit CoreApp(HINSTANCE instance) : instance_(instance) {}

    int run() {
        mutex_ = CreateMutexW(nullptr, TRUE, core_mutex_name);
        if (!mutex_) return 1;
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(mutex_);
            mutex_ = nullptr;
            return 0;
        }
        config_ = load_config();
        state_ = load_state();
        last_saved_state_ = state_;

        WNDCLASSW klass{};
        klass.lpfnWndProc = &CoreApp::window_proc;
        klass.hInstance = instance_;
        klass.lpszClassName = L"GeoDarkCoreWindow";
        if (!RegisterClassW(&klass)) return 1;
        // An invisible *top-level* window is required for system broadcasts.
        window_ = CreateWindowExW(0, klass.lpszClassName, L"GeoDark", WS_OVERLAPPED,
                                  0, 0, 0, 0, nullptr, nullptr, instance_, this);
        if (!window_) return 1;

        timer_ = CreateWaitableTimerExW(nullptr, nullptr, 0, TIMER_ALL_ACCESS);
        config_event_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        theme_event_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        refresh_event_ = CreateEventW(nullptr, FALSE, FALSE, refresh_event_name);
        manual_event_ = CreateEventW(nullptr, FALSE, FALSE, manual_event_name);
        if (!timer_ || !config_event_ || !theme_event_ || !refresh_event_ || !manual_event_) return 1;

        RegCreateKeyExW(HKEY_CURRENT_USER, config_key, 0, nullptr, 0,
                        KEY_NOTIFY, nullptr, &config_registry_, nullptr);
        RegCreateKeyExW(HKEY_CURRENT_USER, theme_key, 0, nullptr, 0,
                        KEY_NOTIFY, nullptr, &theme_registry_, nullptr);
        arm_registry(config_registry_, config_event_);
        arm_registry(theme_registry_, theme_event_);
        NotifyIpInterfaceChange(AF_UNSPEC, &CoreApp::network_callback,
                                this, FALSE, &network_notification_);

        reconcile();
        if (config_.auto_enabled && config_.location_mode == LocationMode::Windows)
            request_location();
        schedule_timer();

        const HANDLE handles[] = {timer_, config_event_, theme_event_, refresh_event_, manual_event_};
        while (running_) {
            const DWORD result = MsgWaitForMultipleObjectsEx(5, handles, INFINITE,
                                                              QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            if (result == WAIT_OBJECT_0) {
                on_timer();
            } else if (result == WAIT_OBJECT_0 + 1) {
                ResetEvent(config_event_);
                arm_registry(config_registry_, config_event_);
                const Config previous = config_;
                config_ = load_config();
                if (!config_.auto_enabled) state_.override_active = false;
                reconcile();
                if (config_.auto_enabled && config_.location_mode == LocationMode::Windows &&
                    (!previous.auto_enabled || previous.location_mode != LocationMode::Windows))
                    request_location();
                schedule_timer();
            } else if (result == WAIT_OBJECT_0 + 2) {
                ResetEvent(theme_event_);
                arm_registry(theme_registry_, theme_event_);
                on_theme_change();
            } else if (result == WAIT_OBJECT_0 + 3) {
                request_location();
                schedule_timer();
            } else if (result == WAIT_OBJECT_0 + 4) {
                const State persisted = load_state();
                state_.override_active = persisted.override_active;
                state_.override_phase_light = persisted.override_phase_light;
                state_.override_until_utc = persisted.override_until_utc;
                reconcile();
                schedule_timer();
            } else if (result == WAIT_OBJECT_0 + 5) {
                MSG message{};
                while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) { running_ = false; break; }
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
            } else {
                running_ = false;
            }
        }

        if (network_notification_) CancelMibChangeNotify2(network_notification_);
        if (config_registry_) RegCloseKey(config_registry_);
        if (theme_registry_) RegCloseKey(theme_registry_);
        if (window_) DestroyWindow(window_);
        if (timer_) CloseHandle(timer_);
        if (config_event_) CloseHandle(config_event_);
        if (theme_event_) CloseHandle(theme_event_);
        if (refresh_event_) CloseHandle(refresh_event_);
        if (manual_event_) CloseHandle(manual_event_);
        if (mutex_) { ReleaseMutex(mutex_); CloseHandle(mutex_); }
        return 0;
    }

private:
    static void arm_registry(HKEY key, HANDLE event) {
        if (key && event)
            RegNotifyChangeKeyValue(key, FALSE, REG_NOTIFY_CHANGE_LAST_SET,
                                    event, TRUE);
    }

    static bool same_state(const State& a, const State& b) {
        return a.has_location == b.has_location && a.latitude == b.latitude &&
            a.longitude == b.longitude && a.accuracy_meters == b.accuracy_meters &&
            a.location_utc == b.location_utc && a.last_attempt_utc == b.last_attempt_utc &&
            a.active_source == b.active_source && a.location_error == b.location_error &&
            a.theme_error == b.theme_error && a.expected_light == b.expected_light &&
            a.next_switch_utc == b.next_switch_utc &&
            a.next_sunrise_utc == b.next_sunrise_utc &&
            a.next_sunset_utc == b.next_sunset_utc && a.polar == b.polar &&
            a.override_active == b.override_active &&
            a.override_phase_light == b.override_phase_light &&
            a.override_until_utc == b.override_until_utc;
    }

    void persist_state() {
        if (!same_state(state_, last_saved_state_) && save_state(state_))
            last_saved_state_ = state_;
    }

    static void WINAPI network_callback(void* context, PMIB_IPINTERFACE_ROW,
                                        MIB_NOTIFICATION_TYPE) {
        auto* app = static_cast<CoreApp*>(context);
        if (app->window_) PostMessageW(app->window_, network_changed_message, 0, 0);
    }

    static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<CREATESTRUCTW*>(lp);
            SetWindowLongPtrW(window, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        }
        auto* app = reinterpret_cast<CoreApp*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (!app) return DefWindowProcW(window, message, wp, lp);
        switch (message) {
        case WM_POWERBROADCAST:
            if (wp == PBT_APMRESUMEAUTOMATIC || wp == PBT_APMRESUMESUSPEND) {
                const auto now = utc_now();
                if (now - app->last_resume_utc_ < 60) return TRUE;
                app->last_resume_utc_ = now;
                app->reconcile();
                app->request_location();
                app->schedule_timer();
            }
            return TRUE;
        case WM_TIMECHANGE:
        case WM_SETTINGCHANGE:
            app->reconcile();
            app->schedule_timer();
            return 0;
        case network_changed_message:
            if (app->config_.auto_enabled && app->config_.location_mode == LocationMode::Windows &&
                app->network_due_ == 0) {
                const auto now = utc_now();
                const auto minimum_gap = app->state_.has_location ? 15 * 60 : 60;
                app->network_due_ = std::max(now + 30,
                                             app->state_.last_attempt_utc + minimum_gap);
                app->schedule_timer();
            }
            return 0;
        case location_result_message: {
            std::unique_ptr<LocationResult> result(reinterpret_cast<LocationResult*>(lp));
            app->requester_.finish();
            if (result->success) {
                app->state_.has_location = true;
                app->state_.latitude = result->latitude;
                app->state_.longitude = result->longitude;
                app->state_.accuracy_meters = result->accuracy_meters;
                app->state_.location_utc = utc_now();
                app->state_.active_source = LocationSource::Windows;
                app->state_.location_error.clear();
            } else {
                app->state_.location_error = result->error.empty()
                    ? L"Windows 定位不可用，请检查系统定位权限" : result->error;
                app->state_.active_source = app->state_.has_location
                    ? LocationSource::Cached : LocationSource::None;
            }
            app->reconcile();
            if (app->retry_after_inflight_) {
                app->retry_after_inflight_ = false;
                app->request_location();
            }
            app->schedule_timer();
            return 0;
        }
        case WM_DESTROY:
            app->running_ = false;
            return 0;
        default:
            return DefWindowProcW(window, message, wp, lp);
        }
    }

    void request_location() {
        if (!config_.auto_enabled || config_.location_mode != LocationMode::Windows) return;
        if (requester_.active()) {
            retry_after_inflight_ = true;
            return;
        }
        state_.last_attempt_utc = utc_now();
        next_location_due_ = state_.last_attempt_utc + location_interval_seconds;
        network_due_ = 0;
        std::wstring error;
        if (!requester_.start(window_, error)) {
            state_.location_error = error;
            state_.active_source = state_.has_location
                ? LocationSource::Cached : LocationSource::None;
            reconcile();
        } else {
            persist_state();
        }
    }

    void on_timer() {
        const auto now = utc_now();
        if (config_.auto_enabled && config_.location_mode == LocationMode::Windows &&
            (now >= next_location_due_ || (network_due_ > 0 && now >= network_due_)))
            request_location();
        reconcile();
        schedule_timer();
    }

    void on_theme_change() {
        reconcile();
        schedule_timer();
    }

    double active_latitude() const {
        return config_.location_mode == LocationMode::Manual
            ? config_.manual_latitude : state_.latitude;
    }

    double active_longitude() const {
        return config_.location_mode == LocationMode::Manual
            ? config_.manual_longitude : state_.longitude;
    }

    void reconcile() {
        const auto now = utc_now();
        const bool manual = config_.location_mode == LocationMode::Manual;
        const bool location_available = manual ? config_.has_manual_coordinates : state_.has_location;
        if (!config_.auto_enabled) state_.override_active = false;
        if (manual && location_available) state_.location_error.clear();
        state_.active_source = !location_available ? LocationSource::None
            : manual ? LocationSource::Manual
            : state_.active_source == LocationSource::Windows && state_.location_utc >= now - 60
                ? LocationSource::Windows : LocationSource::Cached;
        if (!location_available) {
            state_.next_switch_utc = 0;
            state_.next_sunrise_utc = 0;
            state_.next_sunset_utc = 0;
            if (state_.location_error.empty())
                state_.location_error = L"尚无可用位置，请授权定位或输入经纬度";
            persist_state();
            return;
        }

        const SolarSchedule schedule = calculate_solar_schedule(
            now, active_latitude(), active_longitude(),
            config_.sunrise_offset_minutes, config_.sunset_offset_minutes);
        state_.expected_light = schedule.light_now;
        state_.next_switch_utc = schedule.next_switch_utc;
        state_.next_sunrise_utc = schedule.next_sunrise_utc;
        state_.next_sunset_utc = schedule.next_sunset_utc;
        state_.polar = schedule.polar;
        bool external_change = false;
        if (config_.auto_enabled && !state_.override_active && last_applied_light_ >= 0) {
            const ThemeValues actual = read_theme();
            external_change = (actual.apps >= 0 && actual.apps != last_applied_light_) ||
                              (actual.system >= 0 && actual.system != last_applied_light_);
        }
        const ThemeDecision decision = decide_theme(
            config_.auto_enabled, schedule.light_now, now, schedule.next_switch_utc,
            {state_.override_active, state_.override_phase_light, state_.override_until_utc},
            external_change);
        state_.override_active = decision.manual_override.active;
        state_.override_phase_light = decision.manual_override.baseline_light;
        state_.override_until_utc = decision.manual_override.until_utc;
        if (decision.apply) {
            last_applied_light_ = schedule.light_now ? 1 : 0;
            if (!apply_theme(schedule.light_now))
                state_.theme_error = L"无法写入或刷新 Windows 主题";
            else state_.theme_error.clear();
        }
        persist_state();
    }

    void schedule_timer() {
        if (!timer_) return;
        const auto now = utc_now();
        std::int64_t next = now + day_seconds;
        if (config_.auto_enabled && state_.next_switch_utc > now)
            next = std::min(next, state_.next_switch_utc);
        if (config_.auto_enabled && config_.location_mode == LocationMode::Windows) {
            if (next_location_due_ > now) next = std::min(next, next_location_due_);
            if (network_due_ > now) next = std::min(next, network_due_);
        }
        if (next <= now) next = now + 1;
        const ULONGLONG ticks = static_cast<ULONGLONG>(next + unix_to_filetime_seconds) * 10000000ULL;
        LARGE_INTEGER due{};
        due.QuadPart = static_cast<LONGLONG>(ticks);
        SetWaitableTimerEx(timer_, &due, 0, nullptr, nullptr, nullptr, 1000);
    }

    HINSTANCE instance_;
    HWND window_ = nullptr;
    HANDLE mutex_ = nullptr;
    HANDLE timer_ = nullptr;
    HANDLE config_event_ = nullptr;
    HANDLE theme_event_ = nullptr;
    HANDLE refresh_event_ = nullptr;
    HANDLE manual_event_ = nullptr;
    HKEY config_registry_ = nullptr;
    HKEY theme_registry_ = nullptr;
    HANDLE network_notification_ = nullptr;
    LocationRequester requester_;
    Config config_;
    State state_;
    State last_saved_state_;
    bool running_ = true;
    bool retry_after_inflight_ = false;
    int last_applied_light_ = -1;
    std::int64_t next_location_due_ = 0;
    std::int64_t network_due_ = 0;
    std::int64_t last_resume_utc_ = 0;
};

struct AuthorizeWindow {
    HWND window = nullptr;
    HWND button = nullptr;
    winrt::Windows::Foundation::IAsyncOperation<
        winrt::Windows::Devices::Geolocation::GeolocationAccessStatus> operation{nullptr};

    static LRESULT CALLBACK procedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
        if (message == WM_NCCREATE) {
            auto* self = reinterpret_cast<AuthorizeWindow*>(
                reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
            self->window = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        auto* self = reinterpret_cast<AuthorizeWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (!self) return DefWindowProcW(window, message, wp, lp);
        switch (message) {
        case WM_CREATE:
            CreateWindowW(L"STATIC", L"GeoDark 需要 Windows 定位权限。", WS_CHILD | WS_VISIBLE,
                          20, 20, 300, 26, window, nullptr, nullptr, nullptr);
            self->button = CreateWindowW(L"BUTTON", L"请求定位权限", WS_CHILD | WS_VISIBLE,
                                         20, 65, 160, 32, window, reinterpret_cast<HMENU>(1),
                                         nullptr, nullptr);
            return 0;
        case WM_COMMAND:
            if (LOWORD(wp) == 1) {
                EnableWindow(self->button, FALSE);
                try {
                    self->operation = winrt::Windows::Devices::Geolocation::
                        Geolocator::RequestAccessAsync();
                    self->operation.Completed([window](auto const& operation,
                        winrt::Windows::Foundation::AsyncStatus status) {
                        int allowed = 0;
                        try {
                            allowed = status == winrt::Windows::Foundation::AsyncStatus::Completed &&
                                operation.GetResults() == winrt::Windows::Devices::Geolocation::
                                    GeolocationAccessStatus::Allowed;
                        } catch (...) {}
                        PostMessageW(window, access_result_message, allowed, 0);
                    });
                } catch (...) {
                    PostMessageW(window, access_result_message, 0, 0);
                }
            }
            return 0;
        case access_result_message:
            if (wp) {
                signal_location_refresh();
                MessageBoxW(window, L"定位授权已完成。GeoDark 将重新获取位置。",
                            L"GeoDark", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(window, L"无法取得定位权限。请在 Windows 设置中允许桌面应用使用位置，或在 GeoDarkUI 中输入经纬度。",
                            L"GeoDark", MB_OK | MB_ICONWARNING);
            }
            DestroyWindow(window);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(window, message, wp, lp);
        }
    }
};

int run_authorizer(HINSTANCE instance) {
    winrt::init_apartment(winrt::apartment_type::single_threaded);
    WNDCLASSW klass{};
    klass.lpfnWndProc = &AuthorizeWindow::procedure;
    klass.hInstance = instance;
    klass.lpszClassName = L"GeoDarkAuthorizeWindow";
    klass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&klass);
    AuthorizeWindow self;
    HWND window = CreateWindowExW(WS_EX_APPWINDOW, klass.lpszClassName, L"GeoDark 定位授权",
                                   WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                                   CW_USEDEFAULT, CW_USEDEFAULT, 360, 155,
                                   nullptr, nullptr, instance, &self);
    if (!window) return 1;
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);
    SetForegroundWindow(window);
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return 0;
}

} // namespace
} // namespace geodark

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR command, int) {
    if (command && wcscmp(command, L"--authorize-location") == 0)
        return geodark::run_authorizer(instance);
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
    geodark::CoreApp app(instance);
    return app.run();
}
