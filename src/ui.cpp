#include "settings.h"
#include "solar.h"
#include "ui_html.h"

#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wrl/client.h>
#include <wrl/event.h>
#include <WebView2.h>

#include <cmath>
#include <cwchar>
#include <iomanip>
#include <sstream>
#include <string>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace geodark {
namespace {

std::wstring escape_json(const std::wstring& input) {
    std::wstring out;
    out.reserve(input.size() * 2);
    for (wchar_t c : input) {
        switch (c) {
        case L'\"': out += L"\\\""; break;
        case L'\\': out += L"\\\\"; break;
        case L'\b': out += L"\\b"; break;
        case L'\f': out += L"\\f"; break;
        case L'\n': out += L"\\n"; break;
        case L'\r': out += L"\\r"; break;
        case L'\t': out += L"\\t"; break;
        default:
            if (c < 32) {
                wchar_t buf[8];
                swprintf_s(buf, L"\\u%04x", static_cast<unsigned int>(c));
                out += buf;
            } else {
                out += c;
            }
            break;
        }
    }
    return out;
}

std::wstring json_extract_string(const std::wstring& json, const std::wstring& key) {
    const std::wstring pattern = L"\"" + key + L"\"";
    std::size_t pos = json.find(pattern);
    if (pos == std::wstring::npos) return L"";
    pos = json.find(L':', pos + pattern.size());
    if (pos == std::wstring::npos) return L"";
    pos = json.find_first_not_of(L" \t\r\n", pos + 1);
    if (pos == std::wstring::npos || json[pos] != L'\"') return L"";
    ++pos;
    std::wstring result;
    bool escaped = false;
    for (; pos < json.size(); ++pos) {
        const wchar_t c = json[pos];
        if (escaped) {
            result += c;
            escaped = false;
        } else if (c == L'\\') {
            escaped = true;
        } else if (c == L'\"') {
            break;
        } else {
            result += c;
        }
    }
    return result;
}

int json_extract_int(const std::wstring& json, const std::wstring& key, int default_val = 0) {
    const std::wstring pattern = L"\"" + key + L"\"";
    std::size_t pos = json.find(pattern);
    if (pos == std::wstring::npos) return default_val;
    pos = json.find(L':', pos + pattern.size());
    if (pos == std::wstring::npos) return default_val;
    pos = json.find_first_not_of(L" \t\r\n", pos + 1);
    if (pos == std::wstring::npos) return default_val;
    wchar_t* end = nullptr;
    const long val = wcstol(json.c_str() + pos, &end, 10);
    return end != (json.c_str() + pos) ? static_cast<int>(val) : default_val;
}

bool json_extract_bool(const std::wstring& json, const std::wstring& key, bool default_val = false) {
    const std::wstring pattern = L"\"" + key + L"\"";
    std::size_t pos = json.find(pattern);
    if (pos == std::wstring::npos) return default_val;
    pos = json.find(L':', pos + pattern.size());
    if (pos == std::wstring::npos) return default_val;
    pos = json.find_first_not_of(L" \t\r\n", pos + 1);
    if (pos == std::wstring::npos) return default_val;
    if (json.compare(pos, 4, L"true") == 0) return true;
    if (json.compare(pos, 5, L"false") == 0) return false;
    return default_val;
}

bool parse_double(const std::wstring& text, double& result) {
    if (text.empty()) return false;
    wchar_t* end = nullptr;
    result = wcstod(text.c_str(), &end);
    return end != text.c_str() && *end == L'\0' && std::isfinite(result);
}

bool parse_offset(const std::wstring& text, int& result) {
    if (text.empty()) return false;
    wchar_t* end = nullptr;
    const long value = wcstol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != L'\0' || value < -120 || value > 120)
        return false;
    result = static_cast<int>(value);
    return true;
}

const wchar_t* source_name(LocationSource source) {
    switch (source) {
    case LocationSource::Windows: return L"Windows 定位";
    case LocationSource::Cached: return L"上次有效位置";
    case LocationSource::Manual: return L"手动坐标";
    default: return L"未取得";
    }
}

std::wstring get_webview_user_data_path() {
    PWSTR local_app_data = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local_app_data)) && local_app_data) {
        std::wstring path = local_app_data;
        CoTaskMemFree(local_app_data);
        path += L"\\GeoDark";
        CreateDirectoryW(path.c_str(), nullptr);
        path += L"\\WebView2";
        CreateDirectoryW(path.c_str(), nullptr);
        return path;
    }
    return L"";
}

class UiApp {
public:
    explicit UiApp(HINSTANCE instance) : instance_(instance) {
        start_in_settings_ = (wcsstr(GetCommandLineW(), L"--settings") != nullptr);
    }

    int run(int show) {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        WNDCLASSEXW klass{};
        klass.cbSize = sizeof(WNDCLASSEXW);
        klass.lpfnWndProc = &UiApp::window_proc;
        klass.hInstance = instance_;
        klass.lpszClassName = L"GeoDarkSettingsWindow";
        klass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        klass.hIcon = LoadIconW(instance_, MAKEINTRESOURCEW(1));
        klass.hIconSm = LoadIconW(instance_, MAKEINTRESOURCEW(1));
        klass.hbrBackground = nullptr;

        if (!RegisterClassExW(&klass)) return 1;

        UINT dpi = 96;
        HMODULE user32_mod = GetModuleHandleW(L"user32.dll");
        if (user32_mod) {
            typedef UINT (WINAPI* GetDpiForSystemProc)();
            auto get_dpi = reinterpret_cast<GetDpiForSystemProc>(GetProcAddress(user32_mod, "GetDpiForSystem"));
            if (get_dpi) {
                dpi = get_dpi();
            }
        }
        if (dpi == 0) dpi = 96;

        const int width = MulDiv(940, dpi, 96);
        const int height = MulDiv(660, dpi, 96);
        const int screen_w = GetSystemMetrics(SM_CXSCREEN);
        const int screen_h = GetSystemMetrics(SM_CYSCREEN);
        const int x = (screen_w - width) / 2;
        const int y = (screen_h - height) / 2;

        window_ = CreateWindowExW(
            WS_EX_APPWINDOW,
            klass.lpszClassName,
            L"GeoDark 设置",
            WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            x > 0 ? x : CW_USEDEFAULT,
            y > 0 ? y : CW_USEDEFAULT,
            width, height,
            nullptr, nullptr, instance_, this);

        if (!window_) return 1;

        // Apply DWM frame extension for seamless shadow and border
        MARGINS margins = { 1, 1, 1, 1 };
        DwmExtendFrameIntoClientArea(window_, &margins);

        // Dark mode titlebar attribute if supported
        BOOL dark = TRUE;
        DwmSetWindowAttribute(window_, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));

        init_webview();

        ShowWindow(window_, show);
        UpdateWindow(window_);

        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        CoUninitialize();
        return 0;
    }

private:
    void init_webview() {
        const std::wstring user_data = get_webview_user_data_path();

        CreateCoreWebView2EnvironmentWithOptions(
            nullptr,
            user_data.empty() ? nullptr : user_data.c_str(),
            nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [this](HRESULT hr, ICoreWebView2Environment* env) -> HRESULT {
                    if (FAILED(hr) || !env) return hr;
                    env->CreateCoreWebView2Controller(
                        window_,
                        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                            [this](HRESULT res, ICoreWebView2Controller* controller) -> HRESULT {
                                if (FAILED(res) || !controller) return res;
                                controller_ = controller;
                                controller_->get_CoreWebView2(&webview_);
                                if (!webview_) return E_FAIL;

                                RECT bounds;
                                GetClientRect(window_, &bounds);
                                controller_->put_Bounds(bounds);

                                ComPtr<ICoreWebView2Settings> settings;
                                webview_->get_Settings(&settings);
                                if (settings) {
                                    settings->put_AreDefaultContextMenusEnabled(FALSE);
                                    settings->put_IsStatusBarEnabled(FALSE);
                                }

                                webview_->add_WebMessageReceived(
                                    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                        [this](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                            PWSTR raw_json = nullptr;
                                            if (SUCCEEDED(args->get_WebMessageAsJson(&raw_json)) && raw_json) {
                                                handle_web_message(raw_json);
                                                CoTaskMemFree(raw_json);
                                            }
                                            return S_OK;
                                        }).Get(),
                                    &message_token_);

                                const std::string html_utf8 = get_ui_html();
                                std::wstring html_utf16;
                                const int len = MultiByteToWideChar(CP_UTF8, 0, html_utf8.c_str(), -1, nullptr, 0);
                                if (len > 0) {
                                    html_utf16.resize(static_cast<std::size_t>(len - 1));
                                    MultiByteToWideChar(CP_UTF8, 0, html_utf8.c_str(), -1, html_utf16.data(), len);
                                }
                                webview_->NavigateToString(html_utf16.c_str());

                                send_state_to_ui();
                                SetTimer(window_, 1, 3000, nullptr);
                                return S_OK;
                            }).Get());
                    return S_OK;
                }).Get());
    }

    void handle_web_message(const std::wstring& json) {
        const std::wstring action = json_extract_string(json, L"action");

        if (action == L"window_close") {
            PostMessageW(window_, WM_CLOSE, 0, 0);
        } else if (action == L"window_min") {
            ShowWindow(window_, SW_MINIMIZE);
        } else if (action == L"window_max") {
            ShowWindow(window_, IsZoomed(window_) ? SW_RESTORE : SW_MAXIMIZE);
        } else if (action == L"window_drag") {
            ReleaseCapture();
            SendMessageW(window_, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        } else if (action == L"init" || action == L"refresh_status") {
            send_state_to_ui();
            if (action == L"init" && start_in_settings_) {
                webview_->PostWebMessageAsJson(L"{\"switch_view\":\"settings\"}");
            }
        } else if (action == L"refresh_location") {
            refresh_location();
        } else if (action == L"authorize") {
            launch_authorizer();
        } else if (action == L"switch_theme") {
            const bool light = json_extract_bool(json, L"light", true);
            switch_theme(light);
        } else if (action == L"save") {
            save(json);
        }
    }

    void send_state_to_ui() {
        if (!webview_) return;

        const Config config = load_config();
        const State state = load_state();
        const ThemeValues theme = read_theme();
        const bool startup = startup_enabled();
        const bool running = core_running();

        std::wstringstream stream;
        stream << std::fixed << std::setprecision(6);
        stream << L"{"
               << L"\"state\":{"
               << L"\"core_running\":" << (running ? L"true" : L"false") << L","
               << L"\"has_location\":" << (state.has_location ? L"true" : L"false") << L","
               << L"\"active_source\":" << static_cast<int>(state.active_source) << L","
               << L"\"source_name\":\"" << escape_json(source_name(state.active_source)) << L"\","
               << L"\"latitude\":" << state.latitude << L","
               << L"\"longitude\":" << state.longitude << L","
               << L"\"accuracy_meters\":" << state.accuracy_meters << L","
               << L"\"location_time\":\"" << escape_json(format_local_time(state.location_utc)) << L"\","
               << L"\"next_sunrise\":\"" << escape_json(format_local_time(state.next_sunrise_utc)) << L"\","
               << L"\"next_sunset\":\"" << escape_json(format_local_time(state.next_sunset_utc)) << L"\","
               << L"\"next_switch\":\"" << escape_json(format_local_time(state.next_switch_utc)) << L"\","
               << L"\"expected_light\":" << (state.expected_light ? L"true" : L"false") << L","
               << L"\"polar\":" << (state.polar ? L"true" : L"false") << L","
               << L"\"override_active\":" << (state.override_active ? L"true" : L"false") << L","
               << L"\"override_until\":\"" << escape_json(state.override_until_utc > 0 ? format_local_time(state.override_until_utc) : L"") << L"\","
               << L"\"location_error\":\"" << escape_json(state.location_error) << L"\","
               << L"\"theme_error\":\"" << escape_json(state.theme_error) << L"\""
               << L"},"
               << L"\"config\":{"
               << L"\"auto_enabled\":" << (config.auto_enabled ? L"true" : L"false") << L","
               << L"\"location_mode\":" << static_cast<int>(config.location_mode) << L","
               << L"\"manual_latitude\":" << config.manual_latitude << L","
               << L"\"manual_longitude\":" << config.manual_longitude << L","
               << L"\"has_manual_coordinates\":" << (config.has_manual_coordinates ? L"true" : L"false") << L","
               << L"\"sunrise_offset\":" << config.sunrise_offset_minutes << L","
               << L"\"sunset_offset\":" << config.sunset_offset_minutes
               << L"},"
               << L"\"theme\":{"
               << L"\"apps\":" << theme.apps << L","
               << L"\"system\":" << theme.system
               << L"},"
               << L"\"startup\":" << (startup ? L"true" : L"false")
               << L"}";

        webview_->PostWebMessageAsJson(stream.str().c_str());
    }

    void send_toast(const std::wstring& text, const std::wstring& type = L"info") {
        if (!webview_) return;
        const std::wstring json = L"{\"toast\":{\"text\":\"" + escape_json(text) + L"\",\"type\":\"" + type + L"\"}}";
        webview_->PostWebMessageAsJson(json.c_str());
    }

    void save(const std::wstring& json) {
        Config config = load_config();
        config.auto_enabled = json_extract_bool(json, L"auto_enabled", true);
        config.location_mode = (json_extract_int(json, L"location_mode", 0) == 1)
            ? LocationMode::Manual : LocationMode::Windows;

        const std::wstring latitude = json_extract_string(json, L"latitude");
        const std::wstring longitude = json_extract_string(json, L"longitude");
        config.has_manual_coordinates = !latitude.empty() && !longitude.empty();

        if (config.location_mode == LocationMode::Manual && !config.has_manual_coordinates) {
            send_toast(L"手动定位需要输入经纬度。", L"error");
            MessageBoxW(window_, L"手动定位需要输入经纬度。", L"GeoDark", MB_OK | MB_ICONWARNING);
            return;
        }

        if (config.has_manual_coordinates &&
            (!parse_double(latitude, config.manual_latitude) ||
             !parse_double(longitude, config.manual_longitude))) {
            send_toast(L"经纬度格式无效。", L"error");
            MessageBoxW(window_, L"经纬度格式无效。", L"GeoDark", MB_OK | MB_ICONWARNING);
            return;
        }

        const std::wstring sunrise_str = json_extract_string(json, L"sunrise_offset");
        const std::wstring sunset_str = json_extract_string(json, L"sunset_offset");
        if (!parse_offset(sunrise_str, config.sunrise_offset_minutes) ||
            !parse_offset(sunset_str, config.sunset_offset_minutes)) {
            send_toast(L"偏移必须是 -120 到 +120 之间的整数。", L"error");
            MessageBoxW(window_, L"偏移必须是 -120 到 +120 之间的整数。", L"GeoDark", MB_OK | MB_ICONWARNING);
            return;
        }

        const bool startup = json_extract_bool(json, L"startup_enabled", false);
        if (!save_config(config) || !set_startup_enabled(startup)) {
            send_toast(L"保存设置失败，请检查当前用户注册表权限。", L"error");
            MessageBoxW(window_, L"保存设置失败，请检查当前用户注册表权限。", L"GeoDark", MB_OK | MB_ICONERROR);
            return;
        }

        if (!launch_core()) {
            send_toast(L"设置已保存，但后台程序启动失败。", L"warning");
            MessageBoxW(window_, L"设置已保存，但后台程序启动失败。请确认 GeoDark.exe 位于同一目录。",
                        L"GeoDark", MB_OK | MB_ICONWARNING);
        } else {
            signal_location_refresh();
        }

        if (config.auto_enabled && config.location_mode == LocationMode::Windows &&
            !load_state().has_location) {
            launch_authorizer();
        }

        send_toast(L"设置已成功保存", L"success");
        send_state_to_ui();
    }

    void refresh_location() {
        const bool was_running = core_running();
        if (!geodark::launch_core() || (was_running && !signal_location_refresh())) {
            send_toast(L"后台未能接受定位请求。", L"warning");
            MessageBoxW(window_, L"后台未能接受定位请求。", L"GeoDark", MB_OK | MB_ICONWARNING);
        } else {
            send_toast(L"已请求后台刷新定位", L"success");
        }
        send_state_to_ui();
    }

    void launch_authorizer() {
        const auto path = core_executable_path();
        if (reinterpret_cast<INT_PTR>(ShellExecuteW(window_, L"open", path.c_str(),
                                                     L"--authorize-location", nullptr,
                                                     SW_SHOWNORMAL)) <= 32) {
            send_toast(L"无法启动定位授权窗口。", L"error");
            MessageBoxW(window_, L"无法启动定位授权窗口。", L"GeoDark", MB_OK | MB_ICONERROR);
        }
    }

    void switch_theme(bool light) {
        const Config config = load_config();
        State state = load_state();
        if (config.auto_enabled) {
            const bool available = config.location_mode == LocationMode::Manual
                ? config.has_manual_coordinates : state.has_location;
            if (available) {
                const double lat = config.location_mode == LocationMode::Manual
                    ? config.manual_latitude : state.latitude;
                const double lon = config.location_mode == LocationMode::Manual
                    ? config.manual_longitude : state.longitude;
                const SolarSchedule schedule = calculate_solar_schedule(
                    utc_now(), lat, lon, config.sunrise_offset_minutes,
                    config.sunset_offset_minutes);
                state.override_active = true;
                state.override_phase_light = schedule.light_now;
                state.override_until_utc = schedule.next_switch_utc;
                save_state(state);
                signal_manual_override();
            }
        }
        if (!apply_theme(light)) {
            send_toast(L"切换 Windows 主题失败。", L"error");
            MessageBoxW(window_, L"切换 Windows 主题失败。", L"GeoDark", MB_OK | MB_ICONERROR);
        } else {
            send_toast(light ? L"已切换为浅色主题" : L"已切换为深色主题", L"success");
        }
        send_state_to_ui();
    }

    static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
        if (message == WM_NCCREATE) {
            auto* app = reinterpret_cast<UiApp*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
            app->window_ = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }

        auto* app = reinterpret_cast<UiApp*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (!app) return DefWindowProcW(window, message, wp, lp);

        switch (message) {
        case WM_NCCALCSIZE:
            // Remove standard window frame when wp is TRUE, keeping DWM shadow
            if (wp == TRUE) return 0;
            return DefWindowProcW(window, message, wp, lp);

        case WM_NCHITTEST: {
            // Resize borders for frameless window
            POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            ScreenToClient(window, &pt);
            RECT rc;
            GetClientRect(window, &rc);
            const int border = 8;

            if (pt.y < border) {
                if (pt.x < border) return HTTOPLEFT;
                if (pt.x >= rc.right - border) return HTTOPRIGHT;
                return HTTOP;
            }
            if (pt.y >= rc.bottom - border) {
                if (pt.x < border) return HTBOTTOMLEFT;
                if (pt.x >= rc.right - border) return HTBOTTOMRIGHT;
                return HTBOTTOM;
            }
            if (pt.x < border) return HTLEFT;
            if (pt.x >= rc.right - border) return HTRIGHT;
            return HTCLIENT;
        }

        case WM_DPICHANGED: {
            const RECT* rect = reinterpret_cast<const RECT*>(lp);
            SetWindowPos(window, nullptr,
                         rect->left, rect->top,
                         rect->right - rect->left, rect->bottom - rect->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            if (app->controller_) {
                RECT bounds;
                GetClientRect(window, &bounds);
                app->controller_->put_Bounds(bounds);
            }
            return 0;
        }

        case WM_SIZE:
            if (app->controller_) {
                RECT bounds;
                GetClientRect(window, &bounds);
                app->controller_->put_Bounds(bounds);
            }
            return 0;

        case WM_TIMER:
            app->send_state_to_ui();
            return 0;

        case WM_DESTROY:
            KillTimer(window, 1);
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(window, message, wp, lp);
        }
    }

    HINSTANCE instance_;
    HWND window_ = nullptr;
    ComPtr<ICoreWebView2Controller> controller_;
    ComPtr<ICoreWebView2> webview_;
    EventRegistrationToken message_token_{};
    bool start_in_settings_ = false;
};

} // namespace
} // namespace geodark

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    geodark::UiApp app(instance);
    return app.run(show);
}
