#pragma once

#include <windows.h>

#include <cwchar>
#include <stdexcept>
#include <string>

namespace colourwars_editor
{
    inline std::wstring wide(const std::string& value)
    {
        if (value.empty()) return {};
        const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            value.data(), static_cast<int>(value.size()), nullptr, 0);
        if (!count) throw std::runtime_error("Text is not valid UTF-8.");
        std::wstring result(static_cast<size_t>(count), L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), result.data(), count);
        return result;
    }

    inline std::string narrow(const std::wstring& value)
    {
        if (value.empty()) return {};
        const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
            value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (!count) throw std::runtime_error("Text contains an invalid Unicode character.");
        std::string result(static_cast<size_t>(count), '\0');
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), result.data(), count, nullptr, nullptr);
        return result;
    }

    inline std::wstring window_text(HWND window)
    {
        std::wstring result(static_cast<size_t>(GetWindowTextLengthW(window)) + 1, L'\0');
        GetWindowTextW(window, result.data(), static_cast<int>(result.size()));
        result.resize(wcslen(result.c_str()));
        return result;
    }
}
