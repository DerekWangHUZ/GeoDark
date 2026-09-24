#include "settings.h"
#include "solar.h"

#include <shellapi.h>

#include <cmath>
#include <cwchar>
#include <sstream>
#include <string>

namespace geodark {
namespace {

enum ControlId {
    id_auto = 100, id_mode, id_latitude, id_longitude, id_sunrise_offset,
    id_sunset_offset, id_startup, id_save, id_refresh_location,
    id_authorize, id_light, id_dark, id_status, id_refresh_status
};

std::wstring control_text(HWND control) {
    const int length = GetWindowTextLengthW(control);
    std::wstring value(static_cast<std::size_t>(length) + 1, L'\0');
    GetWindowTextW(control, value.data(), length + 1);
    value.resize(static_cast<std::size_t>(length));
    return value;
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

std::wstring coordinate_text(double value) {
    wchar_t buffer[48];
    swprintf_s(buffer, L"%.6f", value);
    return buffer;
}

const wchar_t* source_name(LocationSource source) {
    switch (source) {
    case LocationSource::Windows: return L"Windows 定位";
    case LocationSource::Cached: return L"上次有效位置";
    case LocationSource::Manual: return L"手动坐标";
    default: return L"未取得";
    }
}

class UiApp {
public:
    explicit UiApp(HINSTANCE instance) : instance_(instance) {}

    int run(int show) {
        WNDCLASSW klass{};
        klass.lpfnWndProc = &UiApp::window_proc;
        klass.hInstance = instance_;
        klass.lpszClassName = L"GeoDarkSettingsWindow";
        klass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        klass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        if (!RegisterClassW(&klass)) return 1;
        window_ = CreateWindowExW(WS_EX_APPWINDOW, klass.lpszClassName, L"GeoDark 设置",
                                  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 640, 625,
                                  nullptr, nullptr, instance_, this);
        if (!window_) return 1;
        ShowWindow(window_, show);
        UpdateWindow(window_);
        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        return 0;
    }

private:
    HWND create(const wchar_t* klass, const wchar_t* text, DWORD style,
                int x, int y, int width, int height, int id = 0, DWORD ex = 0) {
        HWND control = CreateWindowExW(ex, klass, text, WS_CHILD | WS_VISIBLE | style,
                                       x, y, width, height, window_,
                                       reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                       instance_, nullptr);
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
        return control;
    }

    void create_controls() {
        font_ = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        auto_ = create(L"BUTTON", L"自动模式", BS_AUTOCHECKBOX, 20, 18, 160, 28, id_auto);
        create(L"STATIC", L"定位方式", 0, 20, 59, 100, 25);
        mode_ = create(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL,
                       130, 55, 240, 200, id_mode);
        SendMessageW(mode_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Windows 自动定位"));
        SendMessageW(mode_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"手动坐标"));

        create(L"STATIC", L"纬度 (-90 ~ 90)", 0, 20, 99, 140, 25);
        latitude_ = create(L"EDIT", L"", ES_AUTOHSCROLL, 175, 95, 160, 27,
                           id_latitude, WS_EX_CLIENTEDGE);
        create(L"STATIC", L"经度 (-180 ~ 180)", 0, 345, 99, 145, 25);
        longitude_ = create(L"EDIT", L"", ES_AUTOHSCROLL, 490, 95, 120, 27,
                            id_longitude, WS_EX_CLIENTEDGE);

        create(L"STATIC", L"日出偏移（分钟）", 0, 20, 140, 150, 25);
        sunrise_ = create(L"EDIT", L"0", ES_AUTOHSCROLL, 175, 136, 100, 27,
                          id_sunrise_offset, WS_EX_CLIENTEDGE);
        create(L"STATIC", L"日落偏移（分钟）", 0, 305, 140, 150, 25);
        sunset_ = create(L"EDIT", L"0", ES_AUTOHSCROLL, 460, 136, 100, 27,
                         id_sunset_offset, WS_EX_CLIENTEDGE);
        create(L"STATIC", L"正数推迟，负数提前；范围 -120 ~ +120。", 0,
               20, 172, 450, 22);

        startup_ = create(L"BUTTON", L"登录 Windows 后自动运行", BS_AUTOCHECKBOX,
                          20, 204, 260, 28, id_startup);
        create(L"BUTTON", L"保存设置", BS_PUSHBUTTON, 20, 245, 105, 34, id_save);
        refresh_button_ = create(L"BUTTON", L"刷新位置", BS_PUSHBUTTON,
                                 135, 245, 105, 34, id_refresh_location);
        create(L"BUTTON", L"授权定位", BS_PUSHBUTTON, 250, 245, 105, 34, id_authorize);
        create(L"BUTTON", L"切换浅色", BS_PUSHBUTTON, 365, 245, 105, 34, id_light);
        create(L"BUTTON", L"切换深色", BS_PUSHBUTTON, 480, 245, 105, 34, id_dark);

        create(L"STATIC", L"当前状态", 0, 20, 302, 120, 25);
        create(L"BUTTON", L"刷新状态", BS_PUSHBUTTON, 505, 294, 105, 28, id_refresh_status);
        status_ = create(L"EDIT", L"", ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL |
                         WS_VSCROLL, 20, 332, 590, 240, id_status, WS_EX_CLIENTEDGE);

        const Config config = load_config();
        SendMessageW(auto_, BM_SETCHECK, config.auto_enabled ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(mode_, CB_SETCURSEL,
                     config.location_mode == LocationMode::Manual ? 1 : 0, 0);
        if (config.has_manual_coordinates) {
            SetWindowTextW(latitude_, coordinate_text(config.manual_latitude).c_str());
            SetWindowTextW(longitude_, coordinate_text(config.manual_longitude).c_str());
        }
        SetWindowTextW(sunrise_, std::to_wstring(config.sunrise_offset_minutes).c_str());
        SetWindowTextW(sunset_, std::to_wstring(config.sunset_offset_minutes).c_str());
        SendMessageW(startup_, BM_SETCHECK, startup_enabled() ? BST_CHECKED : BST_UNCHECKED, 0);
        update_coordinate_controls();
        update_status();
        SetTimer(window_, 1, 3000, nullptr); // UI-only refresh; stopped when UI exits.
    }

    void update_coordinate_controls() {
        const bool manual = SendMessageW(mode_, CB_GETCURSEL, 0, 0) == 1;
        const bool automatic = SendMessageW(auto_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        EnableWindow(latitude_, manual);
        EnableWindow(longitude_, manual);
        EnableWindow(refresh_button_, !manual && automatic);
    }

    void update_status() {
        const State state = load_state();
        const Config config = load_config();
        const ThemeValues theme = read_theme();
        std::wstringstream stream;
        stream << L"后台：" << (core_running() ? L"运行中" : L"未运行") << L"\r\n";
        stream << L"自动模式：" << (config.auto_enabled ? L"开启" : L"关闭") << L"\r\n";
        stream << L"位置来源：" << source_name(state.active_source) << L"\r\n";
        if (state.active_source != LocationSource::None) {
            const double lat = config.location_mode == LocationMode::Manual
                ? config.manual_latitude : state.latitude;
            const double lon = config.location_mode == LocationMode::Manual
                ? config.manual_longitude : state.longitude;
            stream << L"经纬度：" << coordinate_text(lat) << L", " << coordinate_text(lon) << L"\r\n";
        }
        if (config.location_mode == LocationMode::Windows) {
            stream << L"上次定位：" << format_local_time(state.location_utc);
            if (state.has_location && state.accuracy_meters > 0)
                stream << L"（精度约 " << static_cast<int>(state.accuracy_meters) << L" 米）";
            stream << L"\r\n";
        }
        stream << L"下次日出：" << format_local_time(state.next_sunrise_utc) << L"\r\n";
        stream << L"下次日落：" << format_local_time(state.next_sunset_utc) << L"\r\n";
        stream << (config.auto_enabled ? L"下次切换：" : L"下次太阳变化：")
               << format_local_time(state.next_switch_utc) << L"\r\n";
        stream << L"太阳状态：" << (state.active_source == LocationSource::None
            ? L"未知" : state.expected_light ? L"白天" : L"夜晚");
        if (state.active_source != LocationSource::None && state.polar)
            stream << L"（当前无日出或日落）";
        stream << L"\r\n";
        stream << L"Windows 主题：应用 " << (theme.apps == 1 ? L"浅色" : theme.apps == 0 ? L"深色" : L"未知")
               << L" / 系统 " << (theme.system == 1 ? L"浅色" : theme.system == 0 ? L"深色" : L"未知") << L"\r\n";
        if (state.override_active)
            stream << L"手动覆盖至：" << (state.override_until_utc > 0
                ? format_local_time(state.override_until_utc) : L"下一次太阳事件") << L"\r\n";
        if (!state.location_error.empty()) stream << L"定位信息：" << state.location_error << L"\r\n";
        if (!state.theme_error.empty()) stream << L"主题错误：" << state.theme_error << L"\r\n";
        SetWindowTextW(status_, stream.str().c_str());
    }

    void save() {
        Config config = load_config();
        config.auto_enabled = SendMessageW(auto_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        config.location_mode = SendMessageW(mode_, CB_GETCURSEL, 0, 0) == 1
            ? LocationMode::Manual : LocationMode::Windows;
        const std::wstring latitude = control_text(latitude_);
        const std::wstring longitude = control_text(longitude_);
        config.has_manual_coordinates = !latitude.empty() && !longitude.empty();
        if (config.location_mode == LocationMode::Manual && !config.has_manual_coordinates) {
            MessageBoxW(window_, L"手动定位需要输入经纬度。", L"GeoDark", MB_OK | MB_ICONWARNING);
            return;
        }
        if (config.has_manual_coordinates &&
            (!parse_double(latitude, config.manual_latitude) ||
             !parse_double(longitude, config.manual_longitude))) {
            MessageBoxW(window_, L"经纬度格式无效。", L"GeoDark", MB_OK | MB_ICONWARNING);
            return;
        }
        if (!parse_offset(control_text(sunrise_), config.sunrise_offset_minutes) ||
            !parse_offset(control_text(sunset_), config.sunset_offset_minutes)) {
            MessageBoxW(window_, L"偏移必须是 -120 到 +120 之间的整数。", L"GeoDark", MB_OK | MB_ICONWARNING);
            return;
        }
        if (!save_config(config) || !set_startup_enabled(
            SendMessageW(startup_, BM_GETCHECK, 0, 0) == BST_CHECKED)) {
            MessageBoxW(window_, L"保存设置失败，请检查当前用户注册表权限。",
                        L"GeoDark", MB_OK | MB_ICONERROR);
            return;
        }
        if (!launch_core()) {
            MessageBoxW(window_, L"设置已保存，但后台程序启动失败。请确认 GeoDark.exe 位于同一目录。",
                        L"GeoDark", MB_OK | MB_ICONWARNING);
        } else {
            signal_location_refresh();
        }
        if (config.auto_enabled && config.location_mode == LocationMode::Windows &&
            !load_state().has_location) launch_authorizer();
        update_status();
    }

    void launch_authorizer() {
        const auto path = core_executable_path();
        if (reinterpret_cast<INT_PTR>(ShellExecuteW(window_, L"open", path.c_str(),
                                                     L"--authorize-location", nullptr,
                                                     SW_SHOWNORMAL)) <= 32)
            MessageBoxW(window_, L"无法启动定位授权窗口。", L"GeoDark", MB_OK | MB_ICONERROR);
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
        if (!apply_theme(light))
            MessageBoxW(window_, L"切换 Windows 主题失败。", L"GeoDark", MB_OK | MB_ICONERROR);
        update_status();
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
        case WM_CREATE:
            app->create_controls();
            return 0;
        case WM_COMMAND:
            if (LOWORD(wp) == id_mode && HIWORD(wp) == CBN_SELCHANGE)
                app->update_coordinate_controls();
            else if (LOWORD(wp) == id_auto && HIWORD(wp) == BN_CLICKED)
                app->update_coordinate_controls();
            else if (HIWORD(wp) == BN_CLICKED) {
                switch (LOWORD(wp)) {
                case id_save: app->save(); break;
                case id_refresh_location:
                    {
                    const bool was_running = core_running();
                    if (!geodark::launch_core() ||
                        (was_running && !signal_location_refresh()))
                        MessageBoxW(window, L"后台未能接受定位请求。", L"GeoDark", MB_OK | MB_ICONWARNING);
                    break;
                    }
                case id_authorize: app->launch_authorizer(); break;
                case id_light: app->switch_theme(true); break;
                case id_dark: app->switch_theme(false); break;
                case id_refresh_status: app->update_status(); break;
                }
            }
            return 0;
        case WM_TIMER:
            app->update_status();
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
    HFONT font_ = nullptr;
    HWND auto_ = nullptr;
    HWND mode_ = nullptr;
    HWND latitude_ = nullptr;
    HWND longitude_ = nullptr;
    HWND sunrise_ = nullptr;
    HWND sunset_ = nullptr;
    HWND startup_ = nullptr;
    HWND refresh_button_ = nullptr;
    HWND status_ = nullptr;
};

} // namespace
} // namespace geodark

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    geodark::UiApp app(instance);
    return app.run(show);
}
