#pragma once

#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Devices.Geolocation.h>

#include <string>

namespace geodark {

inline constexpr UINT location_result_message = WM_APP + 1;

struct LocationResult {
    bool success = false;
    double latitude = 0.0;
    double longitude = 0.0;
    double accuracy_meters = 0.0;
    std::wstring error;
};

class LocationRequester {
public:
    bool start(HWND target, std::wstring& error);
    void finish();
    bool active() const { return active_; }

private:
    bool active_ = false;
    winrt::Windows::Devices::Geolocation::Geolocator geolocator_{nullptr};
    winrt::Windows::Foundation::IAsyncOperation<
        winrt::Windows::Devices::Geolocation::Geoposition> operation_{nullptr};
};

} // namespace geodark
