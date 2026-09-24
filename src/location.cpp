#include "location.h"

#include <chrono>
#include <cmath>
#include <memory>

namespace geodark {

bool LocationRequester::start(HWND target, std::wstring& error) {
    if (active_) return true;
    try {
        using namespace winrt::Windows::Devices::Geolocation;
        using winrt::Windows::Foundation::TimeSpan;
        geolocator_ = Geolocator();
        geolocator_.DesiredAccuracy(PositionAccuracy::Default);
        const auto timeout = std::chrono::duration_cast<TimeSpan>(std::chrono::seconds(30));
        operation_ = geolocator_.GetGeopositionAsync(TimeSpan{0}, timeout);
        active_ = true;
        operation_.Completed([target](auto const& operation,
                                      winrt::Windows::Foundation::AsyncStatus status) {
            auto result = std::make_unique<LocationResult>();
            try {
                if (status != winrt::Windows::Foundation::AsyncStatus::Completed)
                    throw winrt::hresult_error(E_FAIL, L"定位已取消或超时");
                const auto coordinate = operation.GetResults().Coordinate();
                const auto point = coordinate.Point().Position();
                result->latitude = point.Latitude;
                result->longitude = point.Longitude;
                result->accuracy_meters = coordinate.Accuracy();
                if (!std::isfinite(result->accuracy_meters) || result->accuracy_meters < 0.0)
                    result->accuracy_meters = 0.0;
                result->success = std::isfinite(result->latitude) &&
                    std::isfinite(result->longitude) &&
                    result->latitude >= -90.0 && result->latitude <= 90.0 &&
                    result->longitude >= -180.0 && result->longitude <= 180.0;
                if (!result->success) result->error = L"定位结果无效";
            } catch (const winrt::hresult_error& exception) {
                result->error = exception.message().c_str();
            } catch (...) {
                result->error = L"定位失败";
            }
            if (PostMessageW(target, location_result_message, 0,
                             reinterpret_cast<LPARAM>(result.get()))) result.release();
        });
        return true;
    } catch (const winrt::hresult_error& exception) {
        error = exception.message().c_str();
    } catch (...) {
        error = L"无法启动 Windows 定位";
    }
    finish();
    return false;
}

void LocationRequester::finish() {
    operation_ = nullptr;
    geolocator_ = nullptr;
    active_ = false;
}

} // namespace geodark
