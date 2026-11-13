#include "game/pch.h"
#include "game/crash_record.h"
#include "game/fatal_report.h"
#include "game/build_info.h"
#include "game/directory_consts.h"
#include "engine/core/file_path.h"
#include "scripts/acceptance/support/probe.h"
#include "scripts/acceptance/support/scratch.h"

#include <intrin.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

// REV#63: the crash record. The parent checks the pure pieces in process, then
// starts copies of itself that install the handlers and crash in each way the
// record covers, and reads what they leave in a private LOCALAPPDATA.

#pragma intrinsic(_ReturnAddress)

namespace
{
    using acceptance::require;

    constexpr wchar_t child_argument[] = L"--crash-child";
    constexpr wchar_t argv_marker[] = L"SECRET_ARGV_MARKER";
    constexpr char env_marker[] = "SECRET_ENV_MARKER";
    constexpr DWORD raised_code = 0xE0C0FFEE;
    constexpr DWORD abort_exit = 3;

    // ---- Child side: each mode crashes after leaving one marker. ----------

    // The RVA this marker returns to, in the main image, written where the
    // parent can read it: every pin below is a window after a marker's return,
    // because Debug's incremental-link thunks make a function's own address useless.
    __declspec(noinline) void note_return(const wchar_t* marker_file)
    {
        const auto rva = reinterpret_cast<std::uintptr_t>(_ReturnAddress()) -
            reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        char text[32]{};
        const int length = std::snprintf(text, sizeof(text), "%llX", static_cast<unsigned long long>(rva));
        const HANDLE file = CreateFileW(marker_file, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        DWORD written = 0;
        if (file == INVALID_HANDLE_VALUE || WriteFile(file, text, static_cast<DWORD>(length), &written, nullptr) == FALSE)
        {
            std::fputs("crash child could not write its marker\n", stderr);
            std::_Exit(2);
        }
        CloseHandle(file);
    }

    __declspec(noinline) void fault_through_null(const wchar_t* marker_file)
    {
        volatile int* volatile target = nullptr;
        note_return(marker_file);
        *target = 1;
    }

    __declspec(noinline) void throw_plainly(const wchar_t* marker_file)
    {
        note_return(marker_file);
        throw std::runtime_error("SECRET_NOEXCEPT_DIAGNOSTIC");
    }

    // The throw leaves a noexcept function, which MSVC turns into terminate.
    __declspec(noinline) void throw_through_noexcept(const wchar_t* marker_file) noexcept
    {
        throw_plainly(marker_file);
    }

    // The same, from a noexcept function with a /GS buffer, whose frame handler
    // is __GSHandlerCheck_EH4 in the main image rather than the runtime's own.
    __declspec(noinline) void throw_through_guarded_noexcept(const wchar_t* marker_file) noexcept
    {
        volatile char buffer[64]{};
        buffer[marker_file[0] % 64] = 1;
        throw_plainly(marker_file);
        buffer[1] = buffer[0];
    }

    __declspec(noinline) void terminate_in_catch(const wchar_t* marker_file)
    {
        try { throw std::invalid_argument("SECRET_CAUGHT_DIAGNOSTIC"); }
        catch (const std::invalid_argument&)
        {
            note_return(marker_file);
            std::terminate();
        }
    }

    struct Mode
    {
        const wchar_t* name;
        FailurePhase phase;
    };

    constexpr Mode modes[] = {
        { L"raise", FailurePhase::running },
        { L"terminate_in_catch", FailurePhase::validation },
        { L"noexcept_violation", FailurePhase::launch },
        { L"guarded_noexcept_violation", FailurePhase::running },
        { L"access_violation", FailurePhase::manifest },
    };

    int run_child(const std::wstring& mode, const wchar_t* marker_file)
    {
        // The parent judges the exit code; no dialog or abort message may wait.
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        install_crash_handlers();
        for (const Mode& known : modes)
        {
            if (mode == known.name) { set_crash_phase(known.phase); }
        }
        if (mode == L"raise")
        {
            note_return(marker_file);
            RaiseException(raised_code, EXCEPTION_NONCONTINUABLE, 0, nullptr);
        }
        else if (mode == L"terminate_in_catch") { terminate_in_catch(marker_file); }
        else if (mode == L"noexcept_violation") { throw_through_noexcept(marker_file); }
        else if (mode == L"guarded_noexcept_violation") { throw_through_guarded_noexcept(marker_file); }
        else if (mode == L"access_violation") { fault_through_null(marker_file); }
        std::fputs("crash child did not crash\n", stderr);
        return 2;
    }

    // ---- Parent side. ------------------------------------------------------

    using Record = std::map<std::string, std::string>;

    const std::vector<std::string> header_keys{
        "report_format", "utc", "phase", "category", "version", "game_revision", "engine_revision",
        "configuration", "render_backend", "audio_backend"
    };

    // The fatal_report case's allowlist plus the crash fields, in order.
    std::vector<std::string> record_keys(const std::string& text)
    {
        require(!text.empty() && text.back() == '\n', "Crash record is incomplete");
        std::vector<std::string> keys;
        size_t position = 0;
        while (position < text.size())
        {
            const size_t end = text.find('\n', position);
            const size_t separator = text.find('=', position);
            require(separator != std::string::npos && separator < end, "Crash record line has no field separator");
            keys.push_back(text.substr(position, separator - position));
            position = end + 1;
        }
        return keys;
    }

    Record parse_record(const std::string& text)
    {
        std::set<std::string> fixed(header_keys.begin(), header_keys.end());
        fixed.insert({ "causes_truncated", "message_truncated", "crash_kind", "exception_code",
            "crash_module", "crash_module_name", "crash_rva" });
        Record fields;
        size_t position = 0;
        for (const std::string& key : record_keys(text))
        {
            const size_t end = text.find('\n', position);
            const std::string value = text.substr(position + key.size() + 1, end - position - key.size() - 1);
            bool allowed = fixed.contains(key);
            for (size_t index = 0; index < FatalReport::max_causes && !allowed; ++index)
            {
                const std::string cause = "cause_" + std::to_string(index);
                allowed = key == cause || key == cause + "_code_category" ||
                    key == cause + "_code" || key == cause + "_reason";
            }
            require(allowed, "Crash record contains a field outside its allowlist: " + key);
            require(fields.emplace(key, value).second, "Crash record repeats " + key);
            position = end + 1;
        }
        for (const auto& key : header_keys) { require(fields.contains(key), "Crash record omitted " + key); }
        require(fields.contains("causes_truncated") && fields.contains("message_truncated"),
            "Crash record omitted its truncation flags");
        require(fields.at("report_format") == "1", "Crash record format is not 1");
        const std::string& utc = fields.at("utc");
        require(utc.size() == 20 && utc[4] == '-' && utc[7] == '-' && utc[10] == 'T' &&
            utc[13] == ':' && utc[16] == ':' && utc[19] == 'Z', "Crash record timestamp is not UTC ISO 8601");
        require(fields.at("version") == build_info::version &&
            fields.at("game_revision") == build_info::game_revision &&
            fields.at("engine_revision") == build_info::engine_revision &&
            fields.at("configuration") == build_info::configuration &&
            fields.at("render_backend") == build_info::render_backend &&
            fields.at("audio_backend") == build_info::audio_backend,
            "Crash record header does not describe the build");
        return fields;
    }

    unsigned long long hex_value(const std::string& text, bool prefixed)
    {
        const std::string digits = prefixed ? text.substr(2) : text;
        require(!prefixed || (text.size() > 2 && text.compare(0, 2, "0x") == 0), "Hex field has no 0x prefix");
        require(!digits.empty() && digits.find_first_not_of("0123456789ABCDEF") == std::string::npos,
            "Hex field is not upper-case hex: " + text);
        return std::stoull(digits, nullptr, 16);
    }

    void check_classification()
    {
        const wchar_t* exe = L"D:\\Games\\Colour Wars\\ColourWars.exe";
        const wchar_t* windows = L"C:\\Windows";
        struct Case { const wchar_t* path; CrashModule expected; const char* what; };
        const Case cases[] = {
            { exe, CrashModule::game, "the executable" },
            { L"d:\\games\\colour wars\\colourwars.EXE", CrashModule::game, "the executable in another case" },
            { L"D:\\Games\\Colour Wars\\DirectXTK.dll", CrashModule::engine, "DirectXTK beside the exe" },
            { L"D:\\Games\\Colour Wars\\xaudio2_9redist.DLL", CrashModule::engine, "XAudio2 redist in another case" },
            { L"D:\\Games\\Colour Wars\\sub\\DirectXTK.dll", CrashModule::other, "DirectXTK below the exe folder" },
            { L"E:\\Elsewhere\\DirectXTK.dll", CrashModule::other, "DirectXTK elsewhere" },
            { L"D:\\Games\\Colour Wars\\injected.dll", CrashModule::other, "an unlisted DLL beside the exe" },
            { L"D:\\Games\\Colour Wars\\msvcp140.dll", CrashModule::system, "the app-local C++ runtime" },
            { L"D:\\Games\\Colour Wars\\MSVCP140_ATOMIC_WAIT.dll", CrashModule::system, "a msvcp140 sibling" },
            { L"D:\\Games\\Colour Wars\\vcruntime140_1.dll", CrashModule::system, "vcruntime140_1" },
            { L"D:\\Games\\Colour Wars\\msvcp140.dll.evil", CrashModule::other, "a runtime lookalike suffix" },
            { L"D:\\Games\\Colour Wars\\msvcp14.dll", CrashModule::other, "a runtime lookalike prefix" },
            { L"C:\\Windows\\System32\\KERNELBASE.dll", CrashModule::system, "kernelbase" },
            { L"c:\\windows\\system32\\driverstore\\a\\nvd3dumx.dll", CrashModule::system, "a driver in the Windows tree" },
            { L"C:\\WindowsX\\a.dll", CrashModule::other, "a Windows-prefix lookalike" },
            { L"C:\\Windows", CrashModule::other, "the Windows folder itself" },
            { L"C:\\Program Files\\Overlay\\hook64.dll", CrashModule::other, "an overlay" },
            { L"", CrashModule::other, "an empty path" },
        };
        for (const Case& entry : cases)
        {
            require(classify_crash_module(entry.path, exe, windows) == entry.expected,
                std::string("Module classification is wrong for ") + entry.what);
        }
        require(classify_crash_module(L"C:\\Windows\\System32\\x.dll", exe, L"C:\\Windows\\") == CrashModule::system,
            "A Windows directory with a trailing separator broke classification");
        require(classify_crash_module(L"D:\\Games\\Colour Wars\\DirectXTK.dll", L"", windows) == CrashModule::other &&
            classify_crash_module(L"C:\\Windows\\System32\\x.dll", exe, L"") == CrashModule::other,
            "Classification without a known exe or Windows directory must fall to other");
    }

    void check_formatting()
    {
        CrashDetails other;
        other.has_code = true;
        other.code = 0xC0000005;
        other.module = CrashModule::other;
        std::snprintf(other.module_name, sizeof(other.module_name), "%s", "injected.dll");
        other.has_rva = true;
        other.rva = 0x1234;
        const std::string other_text = crash_record_text(FailurePhase::running, other, {});
        const Record other_record = parse_record(other_text);
        require(other_record.at("crash_module") == "other" && !other_record.contains("crash_module_name") &&
            !other_record.contains("crash_rva") && other_text.find("injected") == std::string::npos,
            "An other module was named or given an address");
        require(other_record.at("exception_code") == "0xC0000005" && other_record.at("crash_kind") == "structured_exception" &&
            other_record.at("causes_truncated") == "false" && other_record.at("message_truncated") == "false",
            "Structured-exception record fields are wrong");

        CrashDetails engine = other;
        engine.module = CrashModule::engine;
        std::snprintf(engine.module_name, sizeof(engine.module_name), "%s", "directxtk.dll");
        const Record engine_record = parse_record(crash_record_text(FailurePhase::manifest, engine, {}));
        require(engine_record.at("crash_module") == "engine" && engine_record.at("crash_module_name") == "directxtk.dll" &&
            engine_record.at("crash_rva") == "0x00001234" && engine_record.at("phase") == "manifest",
            "An engine module lost its name or RVA");

        CrashDetails game;
        game.kind = "terminate";
        game.module = CrashModule::game;
        game.has_rva = true;
        game.rva = 0x123456789ULL;
        const FatalReport caught(FailurePhase::validation,
            std::make_exception_ptr(std::out_of_range("SECRET_CAUSE_TEXT")));
        const std::string game_text = crash_record_text(FailurePhase::validation, game, caught.cause_fields());
        const Record game_record = parse_record(game_text);
        require(game_record.at("crash_kind") == "terminate" && !game_record.contains("exception_code") &&
            !game_record.contains("crash_module_name") && game_record.at("crash_rva") == "0x123456789" &&
            game_record.at("cause_0") == "out_of_range" && game_text.find("SECRET") == std::string::npos,
            "A terminate record's fields are wrong");
        const std::vector<std::string> expected_order{
            "report_format", "utc", "phase", "category", "version", "game_revision", "engine_revision",
            "configuration", "render_backend", "audio_backend", "crash_kind", "crash_module", "crash_rva",
            "cause_0", "causes_truncated", "message_truncated" };
        require(record_keys(game_text) == expected_order, "Crash record keys are out of order");

        char tight[64]{};
        require(format_crash_record(tight, sizeof(tight), FailurePhase::running, game, nullptr) == -1,
            "A crash record that does not fit was not refused");

        // The caught record is the header, the cause fields and its message flag, unchanged.
        const std::vector<std::string> caught_order{
            "report_format", "utc", "phase", "category", "version", "game_revision", "engine_revision",
            "configuration", "render_backend", "audio_backend", "cause_0", "causes_truncated", "message_truncated" };
        require(record_keys(caught.record_text()) == caught_order, "The caught record's keys moved");
        require(caught.cause_fields() == "\ncause_0=out_of_range\ncauses_truncated=false",
            "FatalReport::cause_fields changed shape");
    }

    class Environment
    {
    public:
        explicit Environment(const wchar_t* name) : name_(name)
        {
            wchar_t* previous = nullptr;
            size_t length = 0;
            require(_wdupenv_s(&previous, &length, name) == 0, "Cannot capture the environment");
            had_ = previous != nullptr;
            if (previous != nullptr) { previous_ = previous; std::free(previous); }
        }
        ~Environment() { _wputenv_s(name_, had_ ? previous_.c_str() : L""); }
        Environment(const Environment&) = delete;
        Environment& operator=(const Environment&) = delete;
        void set(const std::wstring& value)
        {
            require(_wputenv_s(name_, value.c_str()) == 0, "Cannot set the child's environment");
        }
    private:
        const wchar_t* name_;
        std::wstring previous_;
        bool had_ = false;
    };

    struct ChildResult
    {
        DWORD exit_code = 0;
        unsigned long long marker = 0;
    };

    ChildResult run_crash_child(const std::wstring& mode, const std::filesystem::path& scratch)
    {
        wchar_t own[32768]{};
        const DWORD length = GetModuleFileNameW(nullptr, own, static_cast<DWORD>(std::size(own)));
        require(length > 0 && length < std::size(own), "Could not find this probe's executable");
        const std::filesystem::path marker = scratch / (L"marker-" + mode + L".txt");
        std::error_code ignored;
        std::filesystem::remove(marker, ignored);
        std::wstring command = L"\"" + std::wstring(own) + L"\" " + child_argument + L" " + mode +
            L" \"" + marker.wstring() + L"\" " + argv_marker;
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process{};
        require(CreateProcessW(own, command.data(), nullptr, nullptr, TRUE, 0, nullptr,
            scratch.c_str(), &startup, &process) != 0, "Could not start a crash child");
        const DWORD waited = WaitForSingleObject(process.hProcess, 30000);
        if (waited != WAIT_OBJECT_0) { TerminateProcess(process.hProcess, 99); }
        ChildResult result;
        GetExitCodeProcess(process.hProcess, &result.exit_code);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        const std::string name = labrador::path_to_utf8(std::filesystem::path(mode));
        require(waited == WAIT_OBJECT_0, "The " + name + " crash child did not exit within 30 seconds");
        require(std::filesystem::exists(marker), "The " + name + " crash child left no marker");
        result.marker = hex_value(acceptance::read_text(marker), false);
        return result;
    }

    void no_temporaries(const std::filesystem::path& folder)
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(folder))
        {
            const std::wstring file = entry.path().filename().wstring();
            require(!(file.starts_with(L"last-error.log.") && file.ends_with(L".tmp")),
                "A crash write left a temporary file");
        }
    }

    void check_private(const std::string& text, const std::string& what)
    {
        for (const char* forbidden : { "SECRET", "PRIVATE", "\\", "/", ".exe", "crash_record.", "marker" })
        {
            require(text.find(forbidden) == std::string::npos,
                what + " record carries a path, argument, environment value or diagnostic");
        }
    }

    void require_near(unsigned long long rva, unsigned long long marker, const std::string& what)
    {
        require(rva >= marker && rva < marker + 128, what + " RVA is not just after its marker's return");
    }

    void check_children(const std::filesystem::path& scratch)
    {
        // Children inherit this: no fault dialog may wait for a click.
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
        Environment local_app_data(L"LOCALAPPDATA");
        Environment marker(L"COLOURWARS_CRASH_PROBE_SECRET");
        marker.set(labrador::path_from_utf8(env_marker).wstring());
        const std::filesystem::path profile = scratch / L"PRIVATE-profile-\u00e9-\u6771\u4eac-\U0001f3a8";
        std::filesystem::create_directories(profile);
        local_app_data.set(profile.wstring());
        const std::filesystem::path folder = profile / L"ColourWars";
        const std::filesystem::path log = folder / L"last-error.log";
        require(directory_consts::fatal_report_path() == labrador::path_to_utf8(log),
            "The private profile does not hold the record");

        // A caught record first, so every crash below must replace a complete one.
        const FatalReport caught(FailurePhase::preferences, std::make_exception_ptr(std::runtime_error("SECRET_CAUGHT")));
        require(caught.write_record(log), "Could not write the caught record the crashes replace");
        std::string previous = acceptance::read_text(log);

        const auto crash = [&](const wchar_t* mode, const char* phase, const char* category)
        {
            const ChildResult result = run_crash_child(mode, scratch);
            const std::string text = acceptance::read_text(log);
            const std::string name = labrador::path_to_utf8(std::filesystem::path(mode));
            require(text != previous, "The " + name + " crash did not replace the previous record");
            previous = text;
            check_private(text, name);
            no_temporaries(profile);
            const Record record = parse_record(text);
            require(record.at("phase") == phase && record.at("category") == category,
                "The " + name + " crash record has the wrong phase");
            require(record.at("causes_truncated") == "false" && record.at("message_truncated") == "false",
                "The " + name + " crash record's truncation flags are wrong");
            return std::tuple{ result, record };
        };

        {
            const auto [result, record] = crash(L"raise", "run_loop", "runtime");
            require(result.exit_code == raised_code, "A raised exception's exit code changed");
            require(record.at("crash_kind") == "structured_exception" && record.at("exception_code") == "0xE0C0FFEE" &&
                record.at("crash_module") == "system" && record.at("crash_module_name") == "kernelbase.dll" &&
                record.contains("crash_rva") && !record.contains("cause_0"),
                "A raised exception was not recorded at KernelBase's RaiseException:\n" + previous);
            hex_value(record.at("crash_rva"), true);
        }
        {
            const auto [result, record] = crash(L"terminate_in_catch", "content_validation", "startup");
            require(result.exit_code == abort_exit, "terminate no longer ends in abort's exit code");
            require(record.at("crash_kind") == "terminate" && !record.contains("exception_code") &&
                record.at("crash_module") == "game" && !record.contains("crash_module_name") &&
                record.at("cause_0") == "invalid_argument" && !record.contains("cause_1"),
                "terminate in a catch was not recorded with its caught cause:\n" + previous);
            require_near(hex_value(record.at("crash_rva"), true), result.marker, "terminate's");
        }
        {
            const auto [result, record] = crash(L"noexcept_violation", "launch_configuration", "startup");
            require(result.exit_code == abort_exit, "A noexcept violation no longer ends in abort's exit code");
            // MSVC calls terminate with the escaping exception current, so its
            // kind is recorded; the RVA is the throw, in the noexcept callee.
            require(record.at("crash_kind") == "terminate" && !record.contains("exception_code") &&
                record.at("crash_module") == "game" && record.at("cause_0") == "runtime_error" &&
                !record.contains("cause_1"),
                "A noexcept violation was not recorded as a terminate in the game:\n" + previous);
            require_near(hex_value(record.at("crash_rva"), true), result.marker, "The noexcept throw's");
        }
        {
            // A /GS frame's handler is main-image code called by ntdll; the
            // record must still name the throw, not that handler.
            const auto [result, record] = crash(L"guarded_noexcept_violation", "run_loop", "runtime");
            require(result.exit_code == abort_exit && record.at("crash_kind") == "terminate" &&
                record.at("crash_module") == "game" && record.at("cause_0") == "runtime_error",
                "A guarded noexcept violation was not recorded as a terminate in the game:\n" + previous);
            require_near(hex_value(record.at("crash_rva"), true), result.marker, "The guarded noexcept throw's");
        }
#if !defined(__SANITIZE_ADDRESS__)
        // AddressSanitizer reports an access violation itself and never
        // reaches the filter; the nightly asan leg runs the other children.
        {
            const auto [result, record] = crash(L"access_violation", "manifest", "startup");
            require(result.exit_code == 0xC0000005, "An access violation's exit code changed");
            require(record.at("crash_kind") == "structured_exception" && record.at("exception_code") == "0xC0000005" &&
                record.at("crash_module") == "game" && !record.contains("crash_module_name"),
                "An access violation was not recorded in the game:\n" + previous);
            require_near(hex_value(record.at("crash_rva"), true), result.marker, "The access violation's");
        }
#endif

        // A directory where the record goes: the crash still exits as it
        // would, the directory and its contents survive, nothing is left over.
        std::filesystem::remove(log);
        std::filesystem::create_directory(log);
        acceptance::write_text(log / L"sentinel.txt", "sentinel");
        const ChildResult blocked = run_crash_child(L"raise", scratch);
        require(blocked.exit_code == raised_code, "A blocked record changed the crash's exit code");
        require(std::filesystem::is_directory(log) && acceptance::read_text(log / L"sentinel.txt") == "sentinel",
            "A blocked record damaged what was in its place");
        no_temporaries(profile);
    }
}

int wmain(int argc, wchar_t* argv[])
{
    if (argc >= 4 && std::wstring(argv[1]) == child_argument)
    {
        return run_child(argv[2], argv[3]);
    }
    return acceptance::run_probe("crash record", [&]
    {
        const acceptance::ScratchDirectory fixtures(L"crash-record");
        check_classification();
        check_formatting();
        check_children(fixtures.root());
#if defined(__SANITIZE_ADDRESS__)
        const char* access = "the access violation is ASan's own under x64-asan";
#else
        const char* access = "an access violation in the game";
#endif
        std::cout << "PASS crash record: module classification, allocation-free formatting, the caught "
            "record unchanged, raised exception at KernelBase, terminate with its caught cause, a noexcept "
            "violation (plain and from a /GS frame), " << access << ", exit codes kept, no private values, blocked-record safety.\n";
        return 0;
    });
}
