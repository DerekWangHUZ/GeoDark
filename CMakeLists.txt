cmake_minimum_required(VERSION 3.21)
project(GeoDark VERSION 1.0 LANGUAGES CXX RC)

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "GeoDark requires 64-bit Windows")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(MSVC)
  # /W4 for every target in the project, including static libs and tests.
  add_compile_options(/W4 /utf-8)
endif()

# WebView2 setup for GeoDarkUI
set(WEBVIEW2_PKG_DIR "$ENV{USERPROFILE}/.nuget/packages/microsoft.web.webview2/1.0.3719.77/build/native")
if(EXISTS "${WEBVIEW2_PKG_DIR}/include/WebView2.h" AND EXISTS "${WEBVIEW2_PKG_DIR}/x64/WebView2LoaderStatic.lib")
  set(WEBVIEW2_INCLUDE_DIR "${WEBVIEW2_PKG_DIR}/include")
  set(WEBVIEW2_LOADER_LIB "${WEBVIEW2_PKG_DIR}/x64/WebView2LoaderStatic.lib")
else()
  include(FetchContent)
  FetchContent_Declare(
    webview2_nuget
    URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.3719.77
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(webview2_nuget)
  set(WEBVIEW2_INCLUDE_DIR "${webview2_nuget_SOURCE_DIR}/build/native/include")
  set(WEBVIEW2_LOADER_LIB "${webview2_nuget_SOURCE_DIR}/build/native/x64/WebView2LoaderStatic.lib")
endif()

# Single source of truth for the binary version metadata: rc and manifest are
# generated from the project() VERSION (see CHANGELOG 1.0).
# RC 的 FILEVERSION 必须是四段数字。project(VERSION 1.0) 不提供 PATCH/TWEAK
# 段（本环境的 CMake 也不展开 PROJECT_VERSION_NUM），这里缺省补零后拼接，
# 再转成 RC 要求的逗号形式。
if(NOT PROJECT_VERSION_PATCH)
  set(PROJECT_VERSION_PATCH 0)
endif()
if(NOT PROJECT_VERSION_TWEAK)
  set(PROJECT_VERSION_TWEAK 0)
endif()
string(REGEX REPLACE "\\." "," GEODARK_VERSION_COMMA "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}")
configure_file(resources/geodark.rc.in "${CMAKE_CURRENT_BINARY_DIR}/geodark.rc" @ONLY)
configure_file(resources/geodark.manifest.in "${CMAKE_CURRENT_BINARY_DIR}/geodark.manifest" @ONLY)
set(GEODARK_RESOURCES
    "${CMAKE_CURRENT_BINARY_DIR}/geodark.rc"
    "${CMAKE_CURRENT_BINARY_DIR}/geodark.manifest")

add_library(geodark_solar STATIC src/solar.cpp src/policy.cpp)
target_include_directories(geodark_solar PUBLIC src)

# Common static code shared by both executables (not a DLL despite the old
# name; renamed from geodark_shared to geodark_common to avoid confusion).
add_library(geodark_common STATIC src/settings.cpp)
target_include_directories(geodark_common PUBLIC src)
target_link_libraries(geodark_common PUBLIC geodark_solar advapi32 user32)
target_compile_definitions(geodark_common PUBLIC UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

add_executable(GeoDark WIN32 src/core.cpp src/location.cpp ${GEODARK_RESOURCES})
target_link_libraries(GeoDark PRIVATE geodark_common windowsapp iphlpapi)

add_executable(GeoDarkUI WIN32 src/ui.cpp ${GEODARK_RESOURCES})
target_include_directories(GeoDarkUI PRIVATE src ${WEBVIEW2_INCLUDE_DIR})
target_link_libraries(GeoDarkUI PRIVATE geodark_common ${WEBVIEW2_LOADER_LIB} shell32 comctl32 ole32 shlwapi version dwmapi)

if(MSVC)
  foreach(target GeoDark GeoDarkUI)
    target_compile_options(${target} PRIVATE /Gy $<$<CONFIG:Release>:/O1>)
    target_link_options(${target} PRIVATE $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
  endforeach()
endif()

enable_testing()
add_executable(geodark_solar_tests tests/solar_tests.cpp)
target_link_libraries(geodark_solar_tests PRIVATE geodark_solar)
add_test(NAME solar COMMAND geodark_solar_tests)

# Bridge protocol extractors are pure functions; test them separately.
add_executable(geodark_bridge_tests tests/bridge_tests.cpp)
target_include_directories(geodark_bridge_tests PRIVATE src)
add_test(NAME bridge COMMAND geodark_bridge_tests)
