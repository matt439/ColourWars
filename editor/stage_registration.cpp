#include "editor/stage_registration.h"
#include "editor/content_catalog.h"
#include "editor/level_document.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/reader.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <exception>
#include <fstream>
#include <iterator>
#include <limits>
#include <set>
#include <string_view>
#include <system_error>
#include <utility>

namespace colourwars_editor
{
    namespace
    {
        namespace fs = std::filesystem;
        using Json = rapidjson::Value;

        [[noreturn]] void fail(const std::string& text) { throw StageRegistrationError(text); }

        std::string shown(const fs::path& path)
        {
            const std::u8string bytes = path.u8string();
            return { reinterpret_cast<const char*>(bytes.data()), bytes.size() };
        }

        fs::path utf8_path(const std::string& text)
        {
            return fs::path(std::u8string(text.begin(), text.end()));
        }

        std::string read(const fs::path& path)
        {
            std::ifstream input(path, std::ios::binary);
            if (!input) fail("Cannot read " + shown(path));
            std::string text{ std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
            if (input.bad()) fail("Cannot read all of " + shown(path));
            if (text.find('\0') != std::string::npos) fail("NUL byte in " + shown(path));
            return text;
        }

        bool same_path(const fs::path& first, const fs::path& second)
        {
            const std::wstring a = first.lexically_normal().native();
            const std::wstring b = second.lexically_normal().native();
            return a.size() <= static_cast<std::size_t>(std::numeric_limits<int>::max()) &&
                b.size() <= static_cast<std::size_t>(std::numeric_limits<int>::max()) &&
                CompareStringOrdinal(a.data(), static_cast<int>(a.size()),
                    b.data(), static_cast<int>(b.size()), TRUE) == CSTR_EQUAL;
        }

        fs::path safe_existing(const fs::path& path, bool directory)
        {
            const fs::path absolute = fs::absolute(path).lexically_normal();
            fs::path at = absolute.root_path();
            for (const auto& component : absolute.relative_path())
            {
                at /= component;
                const DWORD attributes = GetFileAttributesW(at.c_str());
                if (attributes == INVALID_FILE_ATTRIBUTES)
                    throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                        "Cannot inspect " + shown(at));
                if ((attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
                    fail("Registration refuses a linked/reparse path: " + shown(at));
            }
            const fs::path canonical = fs::canonical(absolute);
            if (!same_path(absolute, canonical)) fail("Registration path is not canonical: " + shown(absolute));
            if (directory ? !fs::is_directory(canonical) : !fs::is_regular_file(canonical))
                fail("Registration needs an existing " + std::string(directory ? "directory: " : "file: ") + shown(canonical));
            return canonical;
        }

        std::string folded(std::string text)
        {
            for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return text;
        }

        bool valid_basename(const std::string& name)
        {
            if (name.empty() || name.size() > 250) return false;
            for (const char c : name)
                if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
            if (name.front() == '-') return false;
            const std::string lower = folded(name);
            if (lower == "con" || lower == "prn" || lower == "aux" || lower == "nul" ||
                lower == "clock$" || lower == "conin$" || lower == "conout$") return false;
            return !(lower.size() == 4 && (lower.starts_with("com") || lower.starts_with("lpt")) &&
                lower[3] >= '0' && lower[3] <= '9');
        }

        bool asset_path(const std::string& value)
        {
            if (value.empty() || value.front() == '/' || value.back() == '/') return false;
            for (const char c : value)
                if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == '/')) return false;
            std::size_t start = 0;
            while (start <= value.size())
            {
                const std::size_t end = value.find('/', start);
                const std::string_view part(value.data() + start,
                    (end == std::string::npos ? value.size() : end) - start);
                if (part.empty() || part == "." || part == "..") return false;
                if (end == std::string::npos) break;
                start = end + 1;
            }
            return true;
        }

        const Json& field(const Json& object, const char* name)
        {
            if (!object.IsObject()) fail("Manifest group is not an object");
            const Json* value = nullptr;
            for (auto member = object.MemberBegin(); member != object.MemberEnd(); ++member)
            {
                if (std::string_view(member->name.GetString(), member->name.GetStringLength()) != name) continue;
                if (value) fail(std::string("Duplicate manifest field: ") + name);
                value = &member->value;
            }
            if (!value) fail(std::string("Missing manifest field: ") + name);
            return *value;
        }

        std::string text(const Json& value, const char* name)
        {
            if (!value.IsString()) fail(std::string("Manifest ") + name + " must be a string");
            return { value.GetString(), value.GetStringLength() };
        }

        rapidjson::Document manifest_document(const std::string& source)
        {
            std::string_view bytes = source;
            if (bytes.starts_with("\xEF\xBB\xBF")) bytes.remove_prefix(3);
            rapidjson::Document document;
            document.Parse<rapidjson::kParseValidateEncodingFlag>(bytes.data(), bytes.size());
            if (document.HasParseError()) fail("Invalid manifest JSON at byte " +
                std::to_string(document.GetErrorOffset()) + ": " + rapidjson::GetParseError_En(document.GetParseError()));
            if (!document.IsObject()) fail("manifest.json must be an object");
            return document;
        }

        // Only literal lists are editable. Other commands/comments survive byte
        // for byte; variables, escapes, lists and bracket arguments in the asset
        // list are refused instead of being interpreted as a CMake language.
        struct Token
        {
            std::string value;
            std::size_t begin = 0, end = 0;
            bool literal = true;
            bool quoted = false;
        };
        struct AssetList { std::vector<Token> entries; std::size_t closing = 0; };

        std::vector<Token> cmake_tokens(const std::string& source)
        {
            std::vector<Token> result;
            std::size_t at = source.starts_with("\xEF\xBB\xBF") ? 3 : 0;
            while (at < source.size())
            {
                if (std::isspace(static_cast<unsigned char>(source[at]))) { ++at; continue; }
                if (source[at] == '#')
                {
                    if (at + 1 < source.size() && source[at + 1] == '[')
                    {
                        std::size_t open = at + 2;
                        while (open < source.size() && source[open] == '=') ++open;
                        if (open < source.size() && source[open] == '[')
                        {
                            const std::string close = "]" + source.substr(at + 2, open - at - 2) + "]";
                            const std::size_t end = source.find(close, open + 1);
                            if (end == std::string::npos) fail("Unclosed CMake bracket comment");
                            at = end + close.size();
                            continue;
                        }
                    }
                    at = source.find('\n', at);
                    if (at == std::string::npos) break;
                    continue;
                }
                const std::size_t begin = at;
                if (source[at] == '(' || source[at] == ')')
                {
                    result.push_back({ source.substr(at, 1), at, at + 1, false });
                    ++at;
                    continue;
                }
                if (source[at] == '"')
                {
                    ++at;
                    const std::size_t value_begin = at;
                    bool literal = true;
                    while (at < source.size() && source[at] != '"')
                    {
                        if (source[at] == '\\') { literal = false; ++at; }
                        if (at < source.size()) ++at;
                    }
                    if (at == source.size()) fail("Unclosed CMake quoted argument");
                    result.push_back({ source.substr(value_begin, at - value_begin), begin, at + 1, literal, true });
                    ++at;
                    continue;
                }
                while (at < source.size() && !std::isspace(static_cast<unsigned char>(source[at])) &&
                    source[at] != '(' && source[at] != ')' && source[at] != '#') ++at;
                result.push_back({ source.substr(begin, at - begin), begin, at, true });
            }
            return result;
        }

        AssetList literal_list(const std::vector<Token>& tokens, std::string_view name)
        {
            AssetList result;
            bool found = false;
            for (std::size_t at = 0; at < tokens.size();)
            {
                if (at + 2 >= tokens.size() || folded(tokens[at].value) != "set" ||
                    !tokens[at].literal || tokens[at].quoted || tokens[at + 1].value != "(" ||
                    tokens[at + 1].quoted || tokens[at + 2].quoted)
                    fail("Registration supports only the reviewed literal set() declarations in runtime_assets.cmake");
                const std::string& variable = tokens[at + 2].value;
                if (variable != "COLOURWARS_RUNTIME_ASSETS" && variable != "COLOURWARS_RUNTIME_DIRECTORIES" &&
                    variable != "COLOURWARS_OPTIONAL_RUNTIME_ASSETS")
                    fail("Review the new runtime declaration before registering a stage: " + variable);
                const bool wanted = variable == name;
                if (wanted && found) fail("Runtime asset list is assigned more than once: " + std::string(name));
                found = found || wanted;
                at += 3;
                for (; at < tokens.size() && !(tokens[at].value == ")" && !tokens[at].quoted); ++at)
                {
                    if (!tokens[at].literal || !asset_path(tokens[at].value))
                        fail("Registration requires literal canonical paths in " + variable);
                    if (wanted) result.entries.push_back(tokens[at]);
                }
                if (at == tokens.size()) fail("Unclosed runtime asset list");
                if (wanted) result.closing = tokens[at].begin;
                ++at;
            }
            if (!found) fail("Missing literal runtime asset list: " + std::string(name));
            return result;
        }

        std::string newline(const std::string& source, std::size_t position)
        {
            const std::size_t lf = source.rfind('\n', position);
            return lf != std::string::npos && lf > 0 && source[lf - 1] == '\r' ? "\r\n" : "\n";
        }

        std::string indentation(const std::string& source, std::size_t token)
        {
            const std::size_t lf = source.rfind('\n', token);
            const std::size_t begin = lf == std::string::npos ? 0 : lf + 1;
            const std::string indent = source.substr(begin, token - begin);
            return indent.find_first_not_of(" \t") == std::string::npos ? indent : std::string();
        }

        class NamesSpan : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, NamesSpan>
        {
        public:
            NamesSpan(const rapidjson::StringStream& stream, std::size_t group) : stream_(stream),
                wanted_("/k6:assets/i" + std::to_string(group) + "/k5:names") {}
            bool Null() { advance(); return true; }
            bool Bool(bool) { advance(); return true; }
            bool Int(int) { advance(); return true; }
            bool Uint(unsigned) { advance(); return true; }
            bool Int64(std::int64_t) { advance(); return true; }
            bool Uint64(std::uint64_t) { advance(); return true; }
            bool Double(double) { advance(); return true; }
            bool String(const char*, rapidjson::SizeType, bool)
            {
                if (!frames_.empty() && frames_.back().path == wanted_)
                {
                    const std::size_t begin = stream_.Tell();
                    std::size_t end = begin + 1;
                    while (stream_.head_[end] != '"')
                    {
                        if (stream_.head_[end] == '\\') ++end;
                        ++end;
                    }
                    strings.emplace_back(begin, end + 1);
                }
                advance(); return true;
            }
            bool Key(const char* value, rapidjson::SizeType length, bool)
            { frames_.back().key.assign(value, length); return true; }
            bool StartObject() { open(false); return true; }
            bool StartArray() { open(true); return true; }
            bool EndObject(rapidjson::SizeType) { close(); return true; }
            bool EndArray(rapidjson::SizeType)
            {
                if (frames_.back().path == wanted_) closing = stream_.Tell() - 1;
                close(); return true;
            }
            std::vector<std::pair<std::size_t, std::size_t>> strings;
            std::size_t closing = std::string::npos;
        private:
            struct Frame { std::string path, key; std::size_t index = 0; bool array = false; };
            const rapidjson::StringStream& stream_;
            std::string wanted_;
            std::vector<Frame> frames_;
            std::string path() const
            {
                if (frames_.empty()) return {};
                const Frame& top = frames_.back();
                return top.path + (top.array ? "/i" + std::to_string(top.index) :
                    "/k" + std::to_string(top.key.size()) + ":" + top.key);
            }
            void advance() { if (!frames_.empty() && frames_.back().array) ++frames_.back().index; }
            void open(bool array) { frames_.push_back({ path(), {}, 0, array }); }
            void close() { frames_.pop_back(); advance(); }
        };

        std::string append_name(const std::string& source, std::size_t group, const std::string& name)
        {
            const std::size_t bom = source.starts_with("\xEF\xBB\xBF") ? 3 : 0;
            rapidjson::StringStream stream(source.c_str() + bom);
            NamesSpan handler(stream, group);
            rapidjson::Reader reader;
            if (!reader.Parse<rapidjson::kParseValidateEncodingFlag>(stream, handler) ||
                handler.closing == std::string::npos || handler.strings.empty())
                fail("Cannot locate the existing manifest level names array");
            const std::size_t first = bom + handler.strings.front().first;
            const std::size_t end = bom + handler.strings.back().second;
            const std::size_t close = bom + handler.closing;
            const bool multiline = source.find('\n', first) < close;
            const std::string addition = multiline ? "," + newline(source, end) +
                indentation(source, first) + "\"" + name + "\"" : ", \"" + name + "\"";
            std::string result = source;
            result.insert(end, addition);
            return result;
        }

        std::string append_asset(const std::string& source, const AssetList& list, const std::string& path)
        {
            if (list.entries.empty()) fail("The reviewed runtime asset list is empty");
            const std::size_t lf = source.rfind('\n', list.closing);
            const std::size_t line = lf == std::string::npos ? 0 : lf + 1;
            const bool own_line = source.substr(line, list.closing - line).find_first_not_of(" \t") == std::string::npos;
            std::string result = source;
            if (own_line) result.insert(line, indentation(source, list.entries.back().begin) +
                path + newline(source, list.closing));
            else result.insert(list.closing, " " + path);
            return result;
        }

        void recheck(const fs::path& path, const std::string& expected)
        {
            safe_existing(path, false);
            if (read(path) != expected) fail("Changed since registration preview; preview again: " + shown(path));
        }

        class Prepared
        {
        public:
            Prepared(const fs::path& destination, std::string_view label, const std::string& bytes)
            {
                path = destination;
                path += ".registration-" + std::to_string(GetCurrentProcessId()) + "-" +
                    std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
                    std::string(label);
                HANDLE output = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL, nullptr);
                if (output == INVALID_HANDLE_VALUE) throw std::system_error(static_cast<int>(GetLastError()),
                    std::system_category(), "Cannot create registration sibling");
                try
                {
                    std::size_t offset = 0;
                    while (offset < bytes.size())
                    {
                        const DWORD size = static_cast<DWORD>(std::min(bytes.size() - offset,
                            static_cast<std::size_t>(std::numeric_limits<DWORD>::max())));
                        DWORD written = 0;
                        if (!WriteFile(output, bytes.data() + offset, size, &written, nullptr))
                            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                                "Cannot write registration sibling");
                        if (written == 0) fail("The registration sibling stopped accepting data");
                        offset += written;
                    }
                    if (!FlushFileBuffers(output)) throw std::system_error(static_cast<int>(GetLastError()),
                        std::system_category(), "Cannot flush registration sibling");
                    const BOOL closed = CloseHandle(output);
                    output = INVALID_HANDLE_VALUE;
                    if (!closed) throw std::system_error(static_cast<int>(GetLastError()),
                        std::system_category(), "Cannot close registration sibling");
                }
                catch (...)
                {
                    if (output != INVALID_HANDLE_VALUE) CloseHandle(output);
                    DeleteFileW(path.c_str());
                    throw;
                }
            }
            ~Prepared() { if (!keep_) DeleteFileW(path.c_str()); }
            Prepared(const Prepared&) = delete;
            Prepared& operator=(const Prepared&) = delete;
            void retain() { keep_ = true; }
            fs::path path;
        private:
            bool keep_ = false;
        };

        void replace_file(const fs::path& from, const fs::path& to)
        {
            if (!MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                    "Cannot replace " + shown(to));
        }

        std::string failure_text(const std::exception_ptr& error)
        {
            try { std::rethrow_exception(error); }
            catch (const std::exception& named) { return named.what(); }
            catch (...) { return "unnamed replacement failure"; }
        }
    }

    StageRegistrationError::StageRegistrationError(std::string message, std::vector<fs::path> recovery_files) :
        std::runtime_error(std::move(message)), recovery_files_(std::move(recovery_files)) {}

    StageRegistrationPreview preview_stage_registration(const fs::path& repository, const fs::path& stage)
    {
        StageRegistrationPreview result;
        result.repository = safe_existing(repository, true);
        const fs::path content = safe_existing(result.repository / "game/content", true);
        const fs::path levels = safe_existing(content / "levels", true);
        result.stage = safe_existing(stage.is_absolute() ? stage : result.repository / stage, false);
        if (!same_path(result.stage.parent_path(), levels) || result.stage.extension() != ".json")
            fail("Choose an existing .json file directly inside this source tree's game/content/levels");
        result.name = shown(result.stage.stem());
        if (!valid_basename(result.name)) fail("Invalid stage basename: use ASCII letters/digits/_/-, "
            "no leading hyphen or Windows device name");
        result.stage_original = read(result.stage);
        result.manifest.path = safe_existing(content / "manifest.json", false);
        result.runtime_assets.path = safe_existing(result.repository / "cmake/runtime_assets.cmake", false);
        result.manifest.original = read(result.manifest.path);
        result.runtime_assets.original = read(result.runtime_assets.path);

        const rapidjson::Document manifest = manifest_document(result.manifest.original);
        const Json& groups = field(manifest, "assets");
        if (!groups.IsArray() || groups.Empty()) fail("Manifest assets must be a nonempty array");
        std::vector<std::string> expected{ "manifest.json", "tuning.json", "presentation.json" };
        std::set<std::string> identifiers;
        std::size_t level_group = 0, level_groups = 0, registered = 0;
        for (rapidjson::SizeType index = 0; index < groups.Size(); ++index)
        {
            const Json& group = groups[index];
            const std::string kind = text(field(group, "kind"), "kind");
            std::string directory = text(field(group, "directory"), "directory");
            if (directory.starts_with("./")) directory.erase(0, 2);
            if (directory.empty() || directory.back() != '/' ||
                !asset_path(directory.substr(0, directory.size() - 1))) fail("Manifest directory is not canonical");
            std::vector<std::string> extensions;
            if (kind == "font") extensions = { "spritefont" };
            else if (kind == "sprite_sheet") extensions = { "json", "dds" };
            else if (kind == "level")
            {
                extensions = { "json" }; level_group = index; ++level_groups;
                if (directory != "levels/") fail("Registration requires one level group for ./levels/");
            }
            else if (kind == "sound_bank")
            {
                extensions = { "json", "xwb" };
                const Json& optional = field(group, "optional");
                if (!optional.IsBool() || !optional.GetBool()) fail("Sound banks must remain optional");
            }
            else fail("Unknown manifest kind; review its runtime assets before registering a stage: " + kind);
            const Json& names = field(group, "names");
            if (!names.IsArray() || names.Empty()) fail("Manifest names must be nonempty arrays");
            for (const Json& value : names.GetArray())
            {
                const std::string name = text(value, "name");
                if (!valid_basename(name)) fail("Invalid manifest basename: " + name);
                if (!identifiers.insert(kind + ":" + folded(name)).second)
                    fail("Duplicate/case-alias manifest identifier: " + name);
                if (kind == "level" && folded(name) == folded(result.name))
                {
                    if (name != result.name) fail("Stage basename has different case from its manifest identifier");
                    ++registered;
                }
                for (const std::string& extension : extensions) expected.push_back(directory + name + "." + extension);
            }
        }
        if (level_groups != 1) fail("Registration requires exactly one existing manifest level group");
        const auto tokens = cmake_tokens(result.runtime_assets.original);
        const AssetList runtime = literal_list(tokens, "COLOURWARS_RUNTIME_ASSETS");
        const AssetList optional = literal_list(tokens, "COLOURWARS_OPTIONAL_RUNTIME_ASSETS");
        const std::string level_asset = "levels/" + result.name + ".json";
        std::vector<std::string> actual;
        std::set<std::string> runtime_names;
        std::size_t staged = 0;
        for (const Token& token : runtime.entries)
        {
            if (!runtime_names.insert(folded(token.value)).second) fail("Duplicate/case-alias runtime asset: " + token.value);
            if (folded(token.value) == folded(level_asset))
            {
                if (token.value != level_asset) fail("Stage path has different case in the runtime asset list");
                ++staged;
            }
            actual.push_back(token.value);
            const fs::path path = content / utf8_path(token.value);
            if (!fs::exists(path))
            {
                const bool allowed = std::any_of(optional.entries.begin(), optional.entries.end(),
                    [&](const Token& entry) { return entry.value == token.value; });
                if (!allowed) fail("Missing runtime asset: " + token.value);
            }
            else safe_existing(path, false);
        }
        if ((registered == 0) != (staged == 0))
            fail("Partial stage registration: the name is present in only one content list; reconcile both lists first");
        std::sort(expected.begin(), expected.end()); std::sort(actual.begin(), actual.end());
        if (expected != actual) fail("Manifest and runtime asset list already disagree; reconcile them before registering a stage");

        const ContentCatalog catalog = load_content_catalog(content);
        if (!catalog.manifest_read || !catalog.problems.empty())
            fail("Cannot validate a complete content catalog" +
                (catalog.problems.empty() ? std::string() : ": " + catalog.problems.front()));
        for (const auto& [name, frames] : catalog.sprite_frames)
            if (!frames) fail("Incomplete sprite-sheet catalog: " + name);
        for (const auto& [name, effects] : catalog.sound_effects)
            if (!effects) fail("Incomplete sound-bank catalog: " + name);
        LevelDocument document(content / "tuning.json");
        document.load(result.stage);
        const std::vector<std::string> unresolved = document.unresolved_resources(catalog);
        if (!unresolved.empty()) fail("Stage resource validation failed: " + unresolved.front());
        recheck(result.stage, result.stage_original);
        recheck(result.manifest.path, result.manifest.original);
        recheck(result.runtime_assets.path, result.runtime_assets.original);
        result.already_registered = registered == 1;
        result.manifest.proposed = result.already_registered ? result.manifest.original :
            append_name(result.manifest.original, level_group, result.name);
        result.runtime_assets.proposed = result.already_registered ? result.runtime_assets.original :
            append_asset(result.runtime_assets.original, runtime, level_asset);
        return result;
    }

    bool apply_stage_registration(const StageRegistrationPreview& preview, const StageRegistrationReplace& replace)
    {
        const StageRegistrationPreview fresh = preview_stage_registration(preview.repository, preview.stage);
        if (fresh.name != preview.name || fresh.stage_original != preview.stage_original ||
            !same_path(fresh.manifest.path, preview.manifest.path) ||
            !same_path(fresh.runtime_assets.path, preview.runtime_assets.path) ||
            fresh.manifest.original != preview.manifest.original || fresh.manifest.proposed != preview.manifest.proposed ||
            fresh.runtime_assets.original != preview.runtime_assets.original ||
            fresh.runtime_assets.proposed != preview.runtime_assets.proposed ||
            fresh.already_registered != preview.already_registered)
            fail("Registration inputs or proposal changed; preview again");
        if (fresh.already_registered) return false;
        Prepared manifest_new(fresh.manifest.path, "candidate", fresh.manifest.proposed);
        Prepared runtime_new(fresh.runtime_assets.path, "candidate", fresh.runtime_assets.proposed);
        Prepared manifest_original(fresh.manifest.path, "original", fresh.manifest.original);
        Prepared runtime_original(fresh.runtime_assets.path, "original", fresh.runtime_assets.original);
        recheck(fresh.stage, fresh.stage_original);
        recheck(fresh.manifest.path, fresh.manifest.original);
        recheck(fresh.runtime_assets.path, fresh.runtime_assets.original);
        const auto perform = [&](const fs::path& source, const fs::path& target)
        { if (replace) replace(source, target); else replace_file(source, target); };
        perform(manifest_new.path, fresh.manifest.path);
        try
        {
            recheck(fresh.stage, fresh.stage_original);
            recheck(fresh.manifest.path, fresh.manifest.proposed);
            recheck(fresh.runtime_assets.path, fresh.runtime_assets.original);
            perform(runtime_new.path, fresh.runtime_assets.path);
        }
        catch (...)
        {
            const std::exception_ptr second = std::current_exception();
            try
            {
                recheck(fresh.manifest.path, fresh.manifest.proposed);
                perform(manifest_original.path, fresh.manifest.path);
            }
            catch (...)
            {
                const std::exception_ptr rollback = std::current_exception();
                manifest_original.retain(); runtime_original.retain();
                throw StageRegistrationError(std::string("Registration failed: ") + failure_text(second) +
                    "; first-file rollback could not complete: " + failure_text(rollback) +
                    ". Original backups retained; inspect both content lists before retrying.",
                    { manifest_original.path, runtime_original.path });
            }
            throw StageRegistrationError(std::string("Registration failed; the first file was restored: ") + failure_text(second));
        }
        return true;
    }
}
