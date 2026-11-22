#pragma once

// Minimal JSON field extractors for the WebView2 bridge protocol.
//
// The embedded UI always sends flat JSON objects produced by
// JSON.stringify / window.chrome.webview.postMessage, and C++ -> JS messages
// are serialized with escape_json (ui.cpp). Parsing is therefore limited to
// flat top-level string/int/bool fields, but string values are decoded per
// JSON rules (" \\ \/ \b \f \n \r \t and \uXXXX) so downstream consumers
// (wcstod, registry writes) never see raw escapes.
//
// Windows wchar_t is 16-bit, so astral code points stay as UTF-16 surrogate
// pairs (exactly how JS strings represent them); a lone surrogate passes
// through unchanged instead of being merged or dropped.
//
// These are pure functions with no Win32 dependencies so they can be unit
// tested (tests/bridge_tests.cpp).

#include <string>

namespace geodark {

// Decode a JSON string body starting at `start` (first char after the opening
// quote) until the closing quote. Invalid \uXXXX sequences keep the raw text.
inline std::wstring decode_json_string(const std::wstring& json, std::size_t start) {
    std::wstring result;
    bool escaped = false;
    for (std::size_t pos = start; pos < json.size(); ++pos) {
        const wchar_t c = json[pos];
        if (escaped) {
            escaped = false;
            switch (c) {
            case L'"': result += L'"'; break;
            case L'\\': result += L'\\'; break;
            case L'/': result += L'/'; break;
            case L'b': result += L'\b'; break;
            case L'f': result += L'\f'; break;
            case L'n': result += L'\n'; break;
            case L'r': result += L'\r'; break;
            case L't': result += L'\t'; break;
            case L'u': {
                auto hex_at = [&json](std::size_t at, int& out) {
                    if (at + 4 > json.size()) return false;
                    int value = 0;
                    for (std::size_t i = 0; i < 4; ++i) {
                        const wchar_t h = json[at + i];
                        int digit;
                        if (h >= L'0' && h <= L'9') digit = h - L'0';
                        else if (h >= L'a' && h <= L'f') digit = h - L'a' + 10;
                        else if (h >= L'A' && h <= L'F') digit = h - L'A' + 10;
                        else return false;
                        value = value * 16 + digit;
                    }
                    out = value;
                    return true;
                };
                int code = 0;
                if (!hex_at(pos + 1, code)) {
                    // Invalid escape: keep the raw "\u" text verbatim.
                    result += L'\\';
                    result += L'u';
                    break;
                }
                pos += 4; // consume the 4 hex digits ("u" itself is pos);
                          // the loop's ++pos then lands on the next char
                if (code >= 0xD800 && code <= 0xDBFF && pos + 2 < json.size() &&
                    json[pos + 1] == L'\\' && json[pos + 2] == L'u') {
                    int low = 0;
                    if (hex_at(pos + 3, low) && low >= 0xDC00 && low <= 0xDFFF) {
                        pos += 6; // consume the low surrogate escape
                        result += static_cast<wchar_t>(code);
                        result += static_cast<wchar_t>(low);
                        break;
                    }
                }
                result += static_cast<wchar_t>(code);
                break;
            }
            default:
                // Unknown escape: JSON is strict, but we keep the literal char.
                result += c;
                break;
            }
        } else if (c == L'\\') {
            escaped = true;
        } else if (c == L'"') {
            break;
        } else {
            result += c;
        }
    }
    return result;
}

// Locate a top-level "key" and return the decoded string value, or {}.
inline std::wstring json_extract_string(const std::wstring& json, const std::wstring& key) {
    const std::wstring pattern = L"\"" + key + L"\"";
    std::size_t pos = json.find(pattern);
    if (pos == std::wstring::npos) return L"";
    pos = json.find(L':', pos + pattern.size());
    if (pos == std::wstring::npos) return L"";
    pos = json.find_first_not_of(L" \t\r\n", pos + 1);
    if (pos == std::wstring::npos || json[pos] != L'\"') return L"";
    return decode_json_string(json, pos + 1);
}

inline int json_extract_int(const std::wstring& json, const std::wstring& key,
                            int default_val = 0) {
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

inline bool json_extract_bool(const std::wstring& json, const std::wstring& key,
                              bool default_val = false) {
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

} // namespace geodark
