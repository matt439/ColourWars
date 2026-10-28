// Standalone tooling fixture: no game/engine objects, devices or player data.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <algorithm>
#include <cstdlib>
#include <cwchar>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    class Handle
    {
    public:
        explicit Handle(HANDLE value) noexcept : value_(value) {}
        ~Handle() { if (value_ != nullptr && value_ != INVALID_HANDLE_VALUE) { CloseHandle(value_); } }
        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;
        HANDLE value() const noexcept { return value_; }
    private:
        HANDLE value_;
    };

    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    std::string utf8(std::wstring_view value)
    {
        if (value.empty()) { return {}; }
        require(value.size() <= static_cast<size_t>((std::numeric_limits<int>::max)()), "Argument is too long");
        const int length = static_cast<int>(value.size());
        const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), length,
            nullptr, 0, nullptr, nullptr);
        require(count != 0, "Cannot measure UTF-8 argument");
        std::string result(static_cast<size_t>(count), '\0');
        require(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), length,
            result.data(), count, nullptr, nullptr) == count, "Cannot encode UTF-8 argument");
        return result;
    }

    void write(HANDLE stream, std::string_view value)
    {
        size_t offset = 0;
        while (offset < value.size())
        {
            const auto remaining = value.size() - offset;
            const DWORD amount = static_cast<DWORD>((std::min)(remaining,
                static_cast<size_t>((std::numeric_limits<DWORD>::max)())));
            DWORD written = 0;
            require(WriteFile(stream, value.data() + offset, amount, &written, nullptr) != FALSE &&
                written != 0, "Cannot write fixture output");
            offset += written;
        }
    }

    void out(std::string_view value) { write(GetStdHandle(STD_OUTPUT_HANDLE), value); }
    void err(std::string_view value) { write(GetStdHandle(STD_ERROR_HANDLE), value); }

    void ready(const std::filesystem::path& path, DWORD child = 0)
    {
        const Handle file(CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        require(file.value() != INVALID_HANDLE_VALUE, "Cannot create private readiness marker");
        write(file.value(), "pid=" + std::to_string(GetCurrentProcessId()) + "\nchild=" +
            std::to_string(child) + "\nready=1\n");
        require(FlushFileBuffers(file.value()) != FALSE, "Cannot flush readiness marker");
    }

    std::string hex(std::string_view value)
    {
        constexpr char digits[] = "0123456789ABCDEF";
        std::string result;
        result.reserve(value.size() * 2);
        for (const unsigned char byte : value)
        {
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    }

    int number(const wchar_t* value)
    {
        wchar_t* end = nullptr;
        const long result = std::wcstol(value, &end, 10);
        require(end != value && *end == L'\0' && result >= 0 && result <= 4096, "Invalid numeric argument");
        return static_cast<int>(result);
    }

    std::wstring quote(std::wstring_view value)
    {
        std::wstring result = L"\"";
        size_t slashes = 0;
        for (const wchar_t character : value)
        {
            if (character == L'\\') { ++slashes; continue; }
            if (character == L'\"') { result.append(slashes * 2 + 1, L'\\'); }
            else { result.append(slashes, L'\\'); }
            slashes = 0;
            result.push_back(character);
        }
        result.append(slashes * 2, L'\\');
        result.push_back(L'\"');
        return result;
    }

    std::wstring executable()
    {
        std::vector<wchar_t> path(32768);
        const DWORD count = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        require(count != 0 && count < path.size(), "Cannot locate private fixture executable");
        return { path.data(), count };
    }

    [[noreturn]] void sleep(std::string_view token)
    {
        out("OUT ready " + std::string(token) + "\r\n");
        err("ERR ready " + std::string(token) + "\r\n");
        for (;;) { Sleep(1000); }
    }
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        require(argc >= 2, "Missing fixture mode");
        const std::wstring_view mode(argv[1]);
        if (mode == L"emit")
        {
            require(argc >= 5, "emit requires exit code, token and launch marker");
            const int code = number(argv[2]);
            const std::string token = utf8(argv[3]);
            if (std::wstring_view(argv[4]) != L"-") { ready(argv[4]); }
            out("OUT begin " + token + "\r\n");
            err("ERR begin " + token + "\r\n");
            out("UTF8 caf\xC3\xA9 \xCE\xA9\r\n");
            out("ARGC " + std::to_string(argc - 5) + "\r\n");
            for (int index = 5; index < argc; ++index)
            {
                const std::string value = utf8(argv[index]);
                out("ARG " + std::to_string(index - 5) + " " + std::to_string(value.size()) + " " + hex(value) + "\r\n");
            }
            out("OUT end " + token + "\r\n");
            err("ERR end " + token + "\r\n");
            return code;
        }
        if (mode == L"burst" || mode == L"burst-wait")
        {
            const bool wait = mode == L"burst-wait";
            require(argc == (wait ? 6 : 4), "burst requires token/count and optional readiness/release markers");
            const std::string token = utf8(argv[2]);
            const int rows = number(argv[3]);
            if (wait)
            {
                ready(argv[4]);
                while (GetFileAttributesW(argv[5]) == INVALID_FILE_ATTRIBUTES) { Sleep(10); }
            }
            const std::string padding(96, 'x');
            for (int index = 0; index < rows; ++index)
            {
                out("OUT " + token + " " + std::to_string(index) + " " + padding + "\r\n");
                err("ERR " + token + " " + std::to_string(index) + " " + padding + "\r\n");
            }
            out("DONE " + token + "\r\n");
            return EXIT_SUCCESS;
        }
        if (mode == L"legacy")
        {
            require(argc == 2, "legacy takes no extra arguments");
            out("OUT legacy caf\xE9\r\n");
            err("ERR legacy caf\xE9\r\n");
            return EXIT_SUCCESS;
        }
        if (mode == L"sleep")
        {
            require(argc == 4, "sleep requires token and readiness marker");
            ready(argv[3]);
            sleep(utf8(argv[2]));
        }
        if (mode == L"tree" || mode == L"tree-exit")
        {
            require(argc == 5, "tree requires token, parent marker and child marker");
            const std::wstring path = executable();
            const std::wstring child_token = std::wstring(argv[2]) + L"-child";
            std::wstring command = quote(path) + L" sleep " + quote(child_token) + L" " + quote(argv[4]);
            STARTUPINFOW startup{};
            startup.cb = static_cast<DWORD>(sizeof(startup));
            startup.dwFlags = STARTF_USESTDHANDLES;
            startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
            PROCESS_INFORMATION process{};
            require(CreateProcessW(path.c_str(), command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                nullptr, nullptr, &startup, &process) != FALSE, "Cannot start owned fixture child");
            const Handle child_process(process.hProcess);
            const Handle child_thread(process.hThread);
            ready(argv[3], process.dwProcessId);
            out("CHILD " + std::to_string(process.dwProcessId) + "\r\n");
            if (mode == L"tree-exit") { return EXIT_SUCCESS; }
            sleep(utf8(argv[2]));
        }
        throw std::runtime_error("Unknown fixture mode");
    }
    catch (const std::exception& error)
    {
        try { err("FIXTURE FAIL " + std::string(error.what()) + "\r\n"); } catch (...) {}
        return EXIT_FAILURE;
    }
}
