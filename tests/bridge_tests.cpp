#include "bridge_json.h"

#include <cwchar>

#define CHECK(expression) do { \
    if (!(expression)) { \
        std::fwprintf(stderr, L"CHECK failed at line %d: %hs\n", __LINE__, #expression); \
        return 1; \
    } \
} while (false)

using geodark::json_extract_bool;
using geodark::json_extract_int;
using geodark::json_extract_string;

int main() {
    // ---- json_extract_string ----
    const std::wstring simple = L"{\"action\":\"save\",\"latitude\":\"31.23\"}";
    CHECK(json_extract_string(simple, L"action") == L"save");
    CHECK(json_extract_string(simple, L"latitude") == L"31.23");
    CHECK(json_extract_string(simple, L"missing").empty());
    CHECK(json_extract_string(L"{}", L"action").empty());

    // Escapes decode per JSON rules.
    CHECK(json_extract_string(L"{\"v\":\"a\\\"b\"}", L"v") == L"a\"b");
    CHECK(json_extract_string(L"{\"v\":\"a\\\\b\"}", L"v") == L"a\\b");
    CHECK(json_extract_string(L"{\"v\":\"a\\/b\"}", L"v") == L"a/b");
    CHECK(json_extract_string(L"{\"v\":\"a\\nb\"}", L"v") == L"a\nb");
    CHECK(json_extract_string(L"{\"v\":\"a\\tb\"}", L"v") == L"a\tb");

    // \uXXXX decoding, including CJK text.
    CHECK(json_extract_string(L"{\"v\":\"\\u4e2d\\u6587\"}", L"v") == L"\x4e2d\x6587");
    CHECK(json_extract_string(L"{\"v\":\"\\u0041\\u0042\"}", L"v") == L"AB");

    // Surrogate pairs stay as UTF-16 pairs on 16-bit wchar_t.
    const std::wstring pair_json = L"{\"v\":\"\\ud83d\\ude00\"}";
    const std::wstring pair = json_extract_string(pair_json, L"v");
    CHECK(pair.size() == 2);
    CHECK(static_cast<unsigned short>(pair[0]) == 0xD83D);
    CHECK(static_cast<unsigned short>(pair[1]) == 0xDE00);

    // Invalid \u sequences keep the raw text instead of corrupting output.
    CHECK(json_extract_string(L"{\"v\":\"\\u12zz\"}", L"v") == L"\\u12zz");

    // ---- json_extract_int / json_extract_bool ----
    const std::wstring mixed = L"{\"n\":-42,\"b\":true,\"f\":false,\"s\":\"7\"}";
    CHECK(json_extract_int(mixed, L"n") == -42);
    CHECK(json_extract_int(mixed, L"missing", 5) == 5);
    // String values are NOT parsed as numbers: int fields arrive as JSON numbers.
    CHECK(json_extract_int(mixed, L"s", 0) == 0);
    CHECK(json_extract_bool(mixed, L"b"));
    CHECK(!json_extract_bool(mixed, L"f"));
    CHECK(!json_extract_bool(mixed, L"missing"));

    // A key that is a suffix of another key must not collide.
    const std::wstring suffix = L"{\"sunrise_offset\":\"30\",\"offset\":\"99\"}";
    CHECK(json_extract_string(suffix, L"offset") == L"99");
    CHECK(json_extract_string(suffix, L"sunrise_offset") == L"30");
    return 0;
}
