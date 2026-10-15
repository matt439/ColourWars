#include "game/pch.h"
#include "game/fatal_report.h"
#include "game/build_info.h"
#include "game/directory_consts.h"
#include "engine/core/file_path.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <new>
#include <set>
#include <string>
#include <vector>

namespace
{
    void require(bool condition, const std::string& message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    using Record = std::map<std::string, std::string>;

    Record parse_record(const std::string& text)
    {
        require(!text.empty() && text.back() == '\n', "Metadata record is incomplete");
        const std::set<std::string> fixed{
            "report_format", "utc", "phase", "category", "game_revision", "engine_revision",
            "configuration", "render_backend", "audio_backend", "causes_truncated", "message_truncated"
        };
        Record fields;
        size_t position = 0;
        while (position < text.size())
        {
            const size_t end = text.find('\n', position);
            require(end != std::string::npos, "Metadata line has no terminator");
            const std::string line = text.substr(position, end - position);
            const size_t separator = line.find('=');
            require(separator != std::string::npos, "Metadata line has no field separator");
            const std::string key = line.substr(0, separator);
            const std::string value = line.substr(separator + 1);
            bool allowed = fixed.contains(key);
            for (size_t index = 0; index < FatalReport::max_causes && !allowed; ++index)
            {
                const std::string cause = "cause_" + std::to_string(index);
                allowed = key == cause || key == cause + "_code_category" ||
                    key == cause + "_code" || key == cause + "_reason";
            }
            require(allowed, "Metadata contains a field outside its explicit allowlist");
            require(fields.emplace(key, value).second, "Metadata repeats a field");
            position = end + 1;
        }
        for (const auto& key : fixed) { require(fields.contains(key), "Metadata omitted " + key); }
        require(fields.at("report_format") == "1", "Metadata format is not recognized");
        const std::string& utc = fields.at("utc");
        require(utc.size() == 20 && utc[4] == '-' && utc[7] == '-' && utc[10] == 'T' &&
            utc[13] == ':' && utc[16] == ':' && utc[19] == 'Z', "Record timestamp is not UTC ISO 8601");
        for (size_t index = 0; index < utc.size(); ++index)
        {
            if (index == 4 || index == 7 || index == 10 || index == 13 || index == 16 || index == 19) { continue; }
            require(utc[index] >= '0' && utc[index] <= '9', "UTC timestamp has a non-numeric component");
        }
        require(fields.at("game_revision") == build_info::game_revision &&
            fields.at("engine_revision") == build_info::engine_revision &&
            fields.at("configuration") == build_info::configuration &&
            fields.at("render_backend") == build_info::render_backend &&
            fields.at("audio_backend") == build_info::audio_backend,
            "Record metadata does not describe the actual configured build");
        return fields;
    }

    std::exception_ptr nested(const std::exception_ptr& inner, const std::string& outer)
    {
        try { std::rethrow_exception(inner); }
        catch (...)
        {
            try { std::throw_with_nested(std::runtime_error(outer)); }
            catch (...) { return std::current_exception(); }
        }
    }

    void check_phases()
    {
        struct Phase { FailurePhase value; const char* name; };
        for (const Phase phase : { Phase{FailurePhase::preferences, "preferences"},
            {FailurePhase::definitions, "content_definitions"},
            {FailurePhase::initialization, "application_initialization"},
            {FailurePhase::manifest, "manifest"}, {FailurePhase::validation, "content_validation"},
            {FailurePhase::launch, "launch_configuration"}, {FailurePhase::running, "run_loop"} })
        {
            const FatalReport report(phase.value, std::make_exception_ptr(std::runtime_error("exact original cause")));
            const bool running = phase.value == FailurePhase::running;
            require(report.message() == "exact original cause", "Phase reporting changed the original cause");
            require(std::string(report.stderr_prefix()) == (running ? "runtime" : "startup"),
                "Failure phase has the wrong stderr prefix");
            require(std::wstring(report.caption()) == (running ?
                L"ColourWars - runtime failure" : L"ColourWars - startup failure"),
                "Failure phase has the wrong dialog caption");
            const Record record = parse_record(report.record_text());
            require(record.at("phase") == phase.name && record.at("category") == report.stderr_prefix(),
                "Failure phase is missing from controlled metadata");
            require(report.dialog_text(false).find(L"exact original cause") != std::wstring::npos &&
                report.dialog_text(false).find(L"could not be saved") != std::wstring::npos &&
                report.dialog_text(true).find(L"%LOCALAPPDATA%\\ColourWars\\last-error.log") != std::wstring::npos,
                "Dialog omitted the original cause or gave the wrong persistence result");
        }
        const FatalReport no_hardware(FailurePhase::initialization,
            std::make_exception_ptr(std::runtime_error("No Direct3D hardware device found")));
        require(std::string(no_hardware.stderr_prefix()) + " failure: " + no_hardware.message() + '\n' ==
            "startup failure: No Direct3D hardware device found\n",
            "The exact missing-hardware diagnostic used by startup qualification changed");
        require(parse_record(no_hardware.record_text()).at("cause_0_reason") == "no_hardware_device",
            "Controlled missing-hardware reason was not classified");
        const FatalReport bounds(FailurePhase::running,
            std::make_exception_ptr(std::runtime_error("Player out of bounds")));
        require(parse_record(bounds.record_text()).at("cause_0_reason") == "player_out_of_bounds",
            "Controlled player-bounds reason was not classified");
    }

    class PrivateException final : public std::exception
    {
    public:
        const char* what() const noexcept override { return "SECRET_RTTI_DIAGNOSTIC C:\\PrivateUser\\save_data.json"; }
    };

    class PrivateCategory final : public std::error_category
    {
    public:
        const char* name() const noexcept override { return "SECRET_CATEGORY_NAME"; }
        std::string message(int) const override { return "SECRET_CATEGORY_MESSAGE private-token=abc"; }
    };

    void check_kinds_and_privacy()
    {
        const auto kind_is = [](const std::exception_ptr& failure, const char* kind)
        {
            const FatalReport report(FailurePhase::running, failure);
            const Record record = parse_record(report.record_text());
            require(record.at("cause_0") == kind && record.at("causes_truncated") == "false" &&
                record.at("message_truncated") == "false", "Exception category classification is wrong");
            return report;
        };
        kind_is(std::make_exception_ptr(std::invalid_argument("secret invalid argument")), "invalid_argument");
        kind_is(std::make_exception_ptr(std::out_of_range("secret range")), "out_of_range");
        kind_is(std::make_exception_ptr(std::length_error("secret length")), "length_error");
        kind_is(std::make_exception_ptr(std::logic_error("secret logic")), "logic_error");
        kind_is(std::make_exception_ptr(std::runtime_error("secret runtime")), "runtime_error");
        const std::bad_alloc allocation;
        require(kind_is(std::make_exception_ptr(allocation), "bad_alloc").message() == allocation.what(),
            "Allocation failure lost its original diagnostic");
        const auto unavailable = kind_is({}, "unavailable");
        require(unavailable.message() == "No exception details available.", "Null exception has no useful fallback");
        const auto non_standard = kind_is(std::make_exception_ptr(731928), "non_standard_exception");
        require(non_standard.message() == "Unknown non-standard exception.", "Non-standard exception has no fallback");
        const auto private_exception = kind_is(std::make_exception_ptr(PrivateException{}), "standard_exception");
        require(private_exception.message() == PrivateException{}.what(), "Private diagnostic was lost from local presentation");

        const PrivateCategory category;
        const auto private_system = kind_is(std::make_exception_ptr(std::system_error(
            std::error_code(731928, category), "SECRET_ARGUMENTS --save C:\\PrivateUser\\profile")), "system_error");
        const std::string private_record = private_system.record_text();
        require(private_system.message().find("SECRET_ARGUMENTS") != std::string::npos &&
            private_system.message().find("SECRET_CATEGORY_MESSAGE") != std::string::npos,
            "System exception's original local diagnostic was not retained");
        require(private_record.find("SECRET_") == std::string::npos &&
            private_record.find("731928") == std::string::npos &&
            private_record.find("_code") == std::string::npos &&
            private_record.find("PrivateCategory") == std::string::npos &&
            private_record.find("PrivateUser") == std::string::npos &&
            private_exception.record_text().find("SECRET_") == std::string::npos,
            "Automatic metadata leaked arbitrary diagnostics, custom category identity/code, paths or RTTI");

        for (const auto* known : { &std::system_category(), &std::generic_category() })
        {
            const auto report = kind_is(std::make_exception_ptr(std::system_error(
                std::error_code(12345, *known), "SECRET_SYSTEM_WHAT")), "system_error");
            const Record record = parse_record(report.record_text());
            require(record.at("cause_0_code") == "12345" && record.at("cause_0_code_category") ==
                (known == &std::system_category() ? "system" : "generic") &&
                report.record_text().find("SECRET_SYSTEM_WHAT") == std::string::npos,
                "Known system/generic codes must retain only their numeric identity");
        }

        const std::string sentinel = "SECRET_SAVE_JSON={profile:private}; --argv=private; ENV_SECRET=private";
        const auto chain = nested(std::make_exception_ptr(PrivateException{}), sentinel);
        const FatalReport nested_report(FailurePhase::manifest, chain);
        require(nested_report.message() == sentinel + "\nCaused by: " + PrivateException{}.what(),
            "Nested standard exceptions lost their exact original cause order");
        const Record nested_record = parse_record(nested_report.record_text());
        require(nested_record.at("cause_0") == "runtime_error" && nested_record.at("cause_1") == "standard_exception" &&
            nested_report.record_text().find("SECRET_") == std::string::npos,
            "Nested cause metadata did not preserve classification without raw diagnostics");
        const FatalReport nested_non_standard(FailurePhase::running,
            nested(std::make_exception_ptr(7), "outer exact diagnostic"));
        require(nested_non_standard.message() == "outer exact diagnostic\nCaused by: Unknown non-standard exception." &&
            parse_record(nested_non_standard.record_text()).at("cause_1") == "non_standard_exception",
            "Nested non-standard cause was omitted");
        std::exception_ptr mixed;
        try { throw std::runtime_error("original standard cause"); }
        catch (...)
        {
            try { std::throw_with_nested(std::string("SECRET_NON_STANDARD_WRAPPER")); }
            catch (...) { mixed = std::current_exception(); }
        }
        const FatalReport mixed_report(FailurePhase::running, mixed);
        const Record mixed_record = parse_record(mixed_report.record_text());
        require(mixed_report.message() == "Unknown non-standard exception.\nCaused by: original standard cause" &&
            mixed_record.at("cause_0") == "non_standard_exception" && mixed_record.at("cause_1") == "runtime_error" &&
            mixed_report.record_text().find("SECRET_NON_STANDARD_WRAPPER") == std::string::npos,
            "Non-standard nested wrapper lost its original standard cause or leaked arbitrary wrapper data");
    }

    void check_bounds_and_unicode()
    {
        std::exception_ptr chain = std::make_exception_ptr(std::runtime_error("leaf"));
        for (size_t index = 1; index < FatalReport::max_causes; ++index)
        {
            chain = nested(chain, "outer " + std::to_string(index));
        }
        const FatalReport full(FailurePhase::running, chain);
        const Record full_record = parse_record(full.record_text());
        require(full_record.at("causes_truncated") == "false" &&
            full_record.at("cause_15") == "runtime_error" && full.message().find("leaf") != std::string::npos,
            "An exactly bounded cause chain was truncated");
        const FatalReport overflow(FailurePhase::running, nested(chain, "beyond the cause bound"));
        const Record overflow_record = parse_record(overflow.record_text());
        require(overflow_record.at("causes_truncated") == "true" &&
            overflow_record.at("cause_15") == "runtime_error" &&
            overflow.message().find("[Further exception causes omitted]") != std::string::npos &&
            overflow.message().find("leaf") == std::string::npos,
            "An oversized cause chain was not bounded with a clear omission marker");

        const std::string exact(FatalReport::max_message_bytes, 'x');
        const FatalReport exact_report(FailurePhase::validation, std::make_exception_ptr(std::runtime_error(exact)));
        require(exact_report.message() == exact && parse_record(exact_report.record_text()).at("message_truncated") == "false",
            "An exactly bounded diagnostic was changed or falsely marked truncated");
        const FatalReport too_long(FailurePhase::validation,
            std::make_exception_ptr(std::runtime_error(exact + "SECRET_TAIL")));
        require(too_long.message() == exact && too_long.message().size() == FatalReport::max_message_bytes &&
            parse_record(too_long.record_text()).at("message_truncated") == "true" &&
            too_long.dialog_text(false).find(L"[Exception text truncated]") != std::wstring::npos &&
            too_long.record_text().find("SECRET_TAIL") == std::string::npos,
            "Oversized exception text was not bounded and clearly marked");

        const std::string utf8 = "caf\xC3\xA9 \xE6\x9D\xB1\xE4\xBA\xAC \xF0\x9F\x8E\xA8";
        const FatalReport unicode(FailurePhase::definitions, std::make_exception_ptr(std::runtime_error(utf8)));
        require(unicode.message() == utf8 &&
            unicode.dialog_text(false).find(L"caf\u00e9 \u6771\u4eac \U0001f3a8") != std::wstring::npos &&
            unicode.record_text().find(utf8) == std::string::npos,
            "UTF-8 exception text failed to reach the local Unicode dialog or leaked into metadata");
    }

    class LocalAppData
    {
    public:
        LocalAppData()
        {
            wchar_t* previous = nullptr;
            size_t length = 0;
            require(_wdupenv_s(&previous, &length, L"LOCALAPPDATA") == 0, "Cannot capture process-local LOCALAPPDATA");
            if (previous != nullptr)
            {
                previous_ = previous;
                std::free(previous);
            }
        }
        ~LocalAppData() { _wputenv_s(L"LOCALAPPDATA", previous_.c_str()); }
        void set(const std::filesystem::path& value)
        {
            require(_wputenv_s(L"LOCALAPPDATA", value.c_str()) == 0, "Cannot isolate process-local LOCALAPPDATA");
        }
    private:
        std::wstring previous_;
    };

    std::string contents(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        require(input.good(), "Cannot read isolated fatal report");
        return { std::istreambuf_iterator<char>(input), {} };
    }

    void write_text(const std::filesystem::path& path, const char* text)
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << text;
        output.close();
        require(output.good(), "Cannot create isolated failure fixture");
    }

    void no_temporaries(const std::filesystem::path& scratch)
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(scratch))
        {
            require(entry.path().extension() != L".tmp", "Failed report write left a temporary file");
        }
    }

    void check_paths_and_persistence(const std::filesystem::path& scratch)
    {
        LocalAppData environment;
        environment.set({});
        require(directory_consts::fatal_report_path().empty(), "Missing LOCALAPPDATA must not fall back to executable or working directory");
        const FatalReport first(FailurePhase::preferences,
            std::make_exception_ptr(std::runtime_error("SECRET_FIRST_LOCAL_CAUSE")));
        require(!first.write_record({}), "An unavailable profile path was reported saved");
        const auto profile = scratch / L"profile-\u00e9-\u6771\u4eac-\U0001f3a8";
        environment.set(profile);
        const auto path = profile / L"ColourWars" / L"last-error.log";
        require(directory_consts::fatal_report_path() == labrador::path_to_utf8(path) &&
            !std::filesystem::exists(profile), "Profile lookup must preserve Unicode and must not create a file");
        require(first.write_record(labrador::path_from_utf8(directory_consts::fatal_report_path())),
            "Could not write isolated Unicode per-user diagnostic metadata");
        require(parse_record(contents(path)).at("phase") == "preferences" &&
            contents(path).find("SECRET_FIRST_LOCAL_CAUSE") == std::string::npos,
            "Saved diagnostic lost phase metadata or leaked its original text");

        const FatalReport latest(FailurePhase::running,
            std::make_exception_ptr(std::runtime_error("SECRET_ORIGINAL_RUNTIME_CAUSE")));
        require(latest.write_record(path), "Replacing a complete previous report failed");
        const std::string old = contents(path);
        require(parse_record(old).at("phase") == "run_loop" && parse_record(old).at("category") == "runtime" &&
            old.find("SECRET_ORIGINAL_RUNTIME_CAUSE") == std::string::npos,
            "Latest report was appended, incomplete or contained original diagnostic text");
        const auto failed_preserves = [&]
        {
            require(contents(path) == old && latest.message() == "SECRET_ORIGINAL_RUNTIME_CAUSE",
                "A failed write replaced the complete prior record or original cause");
            no_temporaries(scratch);
        };

        // Deny replacement after the temporary file has been fully written.
        const HANDLE locked = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        require(locked != INVALID_HANDLE_VALUE, "Cannot lock isolated report destination");
        const bool locked_saved = latest.write_record(path);
        require(CloseHandle(locked) != FALSE, "Cannot release isolated report lock");
        require(!locked_saved, "An unwritable locked destination was reported saved");
        failed_preserves();

        const DWORD attributes = GetFileAttributesW(path.c_str());
        require(attributes != INVALID_FILE_ATTRIBUTES &&
            SetFileAttributesW(path.c_str(), attributes | FILE_ATTRIBUTE_READONLY), "Cannot make isolated report read-only");
        const bool readonly_saved = latest.write_record(path);
        const bool restored = SetFileAttributesW(path.c_str(), attributes) != FALSE;
        require(restored && !readonly_saved, "A read-only destination was reported saved or could not be restored");
        failed_preserves();

        const auto directory_destination = scratch / L"directory-destination";
        std::filesystem::create_directory(directory_destination);
        write_text(directory_destination / L"retained.txt", "directory sentinel");
        require(!latest.write_record(directory_destination) &&
            contents(directory_destination / L"retained.txt") == "directory sentinel",
            "A directory destination was replaced or reported saved");
        failed_preserves();

        const auto blocked_parent = scratch / L"blocked-parent";
        write_text(blocked_parent, "blocked parent sentinel");
        require(!latest.write_record(blocked_parent / L"last-error.log") &&
            contents(blocked_parent) == "blocked parent sentinel", "A blocked parent was damaged or reported saved");
        failed_preserves();
        require(first.write_record(path) && parse_record(contents(path)).at("phase") == "preferences",
            "Report writing did not recover after isolated destination failures");
        no_temporaries(scratch);
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: fatal_report.exe <staged content directory>");
        const auto scratch = std::filesystem::absolute(argv[0]).parent_path() /
            (L"fatal-report-fixtures-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
        require(std::filesystem::create_directory(scratch), "Cannot reserve isolated fatal-report fixtures");
        check_phases();
        check_kinds_and_privacy();
        check_bounds_and_unicode();
        check_paths_and_persistence(scratch);
        std::cout << "PASS fatal report: startup/runtime phases, exact nested causes, bounded local Unicode diagnostics, "
            "controlled metadata privacy, per-user paths and atomic replacement/failure recovery.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL fatal report: " << error.what() << '\n';
        return 1;
    }
}
