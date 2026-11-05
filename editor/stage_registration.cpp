#include "editor/stage_registration.h"
#include "editor/content_catalog.h"
#include "editor/level_document.h"
#include "editor/path_text.h"
#include "editor/prepared_file.h"
#include "game/contracts/user_stage_contract.h"

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
#include <array>
#include <cctype>
#include <exception>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
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

        std::string read(const fs::path& path)
        {
            std::ifstream input(path, std::ios::binary);
            if (!input) fail("Cannot read " + path_text(path));
            std::string text{ std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
            if (input.bad()) fail("Cannot read all of " + path_text(path));
            if (text.find('\0') != std::string::npos) fail("NUL byte in " + path_text(path));
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
                        "Cannot inspect " + path_text(at));
                if ((attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
                    fail("Registration refuses a linked/reparse path: " + path_text(at));
            }
            const fs::path canonical = fs::canonical(absolute);
            if (!same_path(absolute, canonical)) fail("Registration path is not canonical: " + path_text(absolute));
            if (directory ? !fs::is_directory(canonical) : !fs::is_regular_file(canonical))
                fail("Registration needs an existing " + std::string(directory ? "directory: " : "file: ") + path_text(canonical));
            return canonical;
        }

        std::string folded(std::string text)
        {
            for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return text;
        }

        // The one stage-name rule, shared with the game's scan of a player's
        // own stages, so a saved user stage can be registered unchanged.
        bool valid_basename(const std::string& name)
        {
            return user_stage_contract::usable_name(name);
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

        rapidjson::Document json_document(const std::string& source, const std::string& label)
        {
            std::string_view bytes = source;
            if (bytes.starts_with("\xEF\xBB\xBF")) bytes.remove_prefix(3);
            rapidjson::Document document;
            document.Parse<rapidjson::kParseValidateEncodingFlag>(bytes.data(), bytes.size());
            if (document.HasParseError()) fail("Invalid " + label + " JSON at byte " +
                std::to_string(document.GetErrorOffset()) + ": " + rapidjson::GetParseError_En(document.GetParseError()));
            if (!document.IsObject()) fail(label + " must be an object");
            return document;
        }

        rapidjson::Document manifest_document(const std::string& source) { return json_document(source, "manifest.json"); }

        // presentation.json's menu_text.stage_names (docs/localisation.md):
        // each shipped stage's title by manifest id, which the game requires
        // to equal the level's display_name. Comment keys are skipped.
        struct StageNames
        {
            std::vector<std::pair<std::string, std::string>> entries;
        };

        StageNames stage_names(const std::string& source)
        {
            const rapidjson::Document presentation = json_document(source, "presentation.json");
            const auto member = [](const Json& object, const char* name) -> const Json&
            {
                const Json* found = nullptr;
                for (auto it = object.MemberBegin(); it != object.MemberEnd(); ++it)
                {
                    if (std::string_view(it->name.GetString(), it->name.GetStringLength()) != name) continue;
                    if (found) fail(std::string("Duplicate presentation.json field: ") + name);
                    found = &it->value;
                }
                if (!found || !found->IsObject()) fail(std::string("presentation.json needs a '") + name + "' object");
                return *found;
            };
            const Json& names = member(member(presentation, "menu_text"), "stage_names");
            StageNames result;
            std::set<std::string> seen;
            for (auto it = names.MemberBegin(); it != names.MemberEnd(); ++it)
            {
                const std::string id(it->name.GetString(), it->name.GetStringLength());
                if (!seen.insert(id).second) fail("Duplicate presentation.json menu_text.stage_names key: " + id);
                if (!id.empty() && id.front() == '_') continue;
                if (!it->value.IsString()) fail("presentation.json menu_text.stage_names." + id + " must be text");
                result.entries.emplace_back(id, std::string(it->value.GetString(), it->value.GetStringLength()));
            }
            return result;
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

        // Where menu_text.stage_names' members are, by NamesSpan's means: the
        // reader parses a copied StringStream, so Tell() is at a string's quote.
        class StageNamesSpan : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, StageNamesSpan>
        {
        public:
            explicit StageNamesSpan(const rapidjson::StringStream& stream) : stream_(stream) {}
            bool Null() { scalar(); return true; }
            bool Bool(bool) { scalar(); return true; }
            bool Int(int) { scalar(); return true; }
            bool Uint(unsigned) { scalar(); return true; }
            bool Int64(std::int64_t) { scalar(); return true; }
            bool Uint64(std::uint64_t) { scalar(); return true; }
            bool Double(double) { scalar(); return true; }
            bool String(const char*, rapidjson::SizeType, bool)
            {
                if (inside())
                {
                    std::size_t end = stream_.Tell() + 1;
                    while (stream_.head_[end] != '"')
                    {
                        if (stream_.head_[end] == '\\') ++end;
                        ++end;
                    }
                    value_end = end + 1;
                }
                return true;
            }
            bool Key(const char* value, rapidjson::SizeType length, bool)
            {
                if (inside())
                {
                    if (first_key == std::string::npos) first_key = stream_.Tell();
                    last_key = stream_.Tell();
                }
                frames_.back().key.assign(value, length);
                return true;
            }
            bool StartObject() { scalar(); open(); return true; }
            bool StartArray() { scalar(); open(); return true; }
            bool EndObject(rapidjson::SizeType)
            {
                if (frames_.back().path == wanted_) { closing = stream_.Tell() - 1; ++found; }
                frames_.pop_back(); return true;
            }
            bool EndArray(rapidjson::SizeType) { frames_.pop_back(); return true; }
            std::size_t first_key = std::string::npos, last_key = std::string::npos;
            std::size_t value_end = std::string::npos, closing = std::string::npos;
            std::size_t found = 0;
            bool other = false;
        private:
            struct Frame { std::string path, key; };
            const rapidjson::StringStream& stream_;
            const std::string wanted_ = "/k9:menu_text/k11:stage_names";
            std::vector<Frame> frames_;
            bool inside() const { return !frames_.empty() && frames_.back().path == wanted_; }
            void scalar() { if (inside()) other = true; }
            // Array elements are never under the wanted object's path.
            void open()
            {
                const std::string path = frames_.empty() ? std::string() :
                    frames_.back().path + "/k" + std::to_string(frames_.back().key.size()) + ":" + frames_.back().key;
                frames_.push_back({ path, {} });
            }
        };

        std::string json_string(const std::string& text)
        {
            constexpr char digits[] = "0123456789abcdef";
            std::string result = "\"";
            for (const char c : text)
            {
                const unsigned char byte = static_cast<unsigned char>(c);
                if (c == '"' || c == '\\') { result += '\\'; result += c; }
                else if (byte < 0x20) { result += "\\u00"; result += digits[byte >> 4]; result += digits[byte & 15]; }
                else result += c;
            }
            return result + "\"";
        }

        // The new entry as the table's last member, laid out like the one before it.
        std::string append_stage_name(const std::string& source, const std::string& name, const std::string& display_name)
        {
            const std::size_t bom = source.starts_with("\xEF\xBB\xBF") ? 3 : 0;
            rapidjson::StringStream stream(source.c_str() + bom);
            StageNamesSpan handler(stream);
            rapidjson::Reader reader;
            if (!reader.Parse<rapidjson::kParseValidateEncodingFlag>(stream, handler) || handler.found != 1 ||
                handler.closing == std::string::npos)
                fail("Cannot locate presentation.json's menu_text.stage_names object");
            if (handler.other) fail("presentation.json menu_text.stage_names may hold only text");
            if (handler.last_key == std::string::npos || handler.value_end == std::string::npos)
                fail("presentation.json menu_text.stage_names is empty; add its first entry by hand");
            const std::size_t first = bom + handler.first_key;
            const std::size_t last = bom + handler.last_key;
            const std::size_t end = bom + handler.value_end;
            const std::size_t close = bom + handler.closing;
            const std::string entry = "\"" + name + "\": " + json_string(display_name);
            const bool multiline = source.find('\n', first) < close;
            std::string result = source;
            result.insert(end, multiline ? "," + newline(source, end) + indentation(source, last) + entry : ", " + entry);
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
            if (read(path) != expected) fail("Changed since registration preview; preview again: " + path_text(path));
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
        result.name = path_text(result.stage.stem());
        if (!valid_basename(result.name)) fail("Invalid stage basename: " + user_stage_contract::name_rule());
        if (result.name.front() == '_') fail("A shipped stage's basename must not start with '_': "
            "presentation.json's menu_text.stage_names would read its entry as a comment");
        result.stage_original = read(result.stage);
        result.manifest.path = safe_existing(content / "manifest.json", false);
        result.runtime_assets.path = safe_existing(result.repository / "cmake/runtime_assets.cmake", false);
        result.presentation.path = safe_existing(content / "presentation.json", false);
        result.manifest.original = read(result.manifest.path);
        result.runtime_assets.original = read(result.runtime_assets.path);
        result.presentation.original = read(result.presentation.path);

        const rapidjson::Document manifest = manifest_document(result.manifest.original);
        const Json& groups = field(manifest, "assets");
        if (!groups.IsArray() || groups.Empty()) fail("Manifest assets must be a nonempty array");
        std::vector<std::string> expected{ "manifest.json", "tuning.json", "presentation.json" };
        std::set<std::string> identifiers;
        std::size_t level_group = 0, level_groups = 0, registered = 0;
        std::vector<std::string> level_names;
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
                if (kind == "level") level_names.push_back(name);
                if (kind == "level" && folded(name) == folded(result.name))
                {
                    if (name != result.name) fail("Stage basename has different case from its manifest identifier");
                    ++registered;
                }
                for (const std::string& extension : extensions) expected.push_back(directory + name + "." + extension);
            }
        }
        if (level_groups != 1) fail("Registration requires exactly one existing manifest level group");
        // Locales load before the manifest, so their own index names them
        // (docs/localisation.md): the index and one file per listed tag.
        const fs::path locale_index = content / "locales" / "locales.json";
        if (fs::exists(locale_index))
        {
            const rapidjson::Document locales = manifest_document(read(safe_existing(locale_index, false)));
            const Json& list = field(locales, "locales");
            if (!list.IsArray()) fail("locales/locales.json locales must be an array");
            expected.push_back("locales/locales.json");
            for (const Json& entry : list.GetArray())
                expected.push_back("locales/" + text(field(entry, "tag"), "tag") + ".json");
        }
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
        const StageNames titles = stage_names(result.presentation.original);
        std::vector<std::string> titled;
        const std::string* title = nullptr;
        for (const auto& [id, text] : titles.entries)
        {
            titled.push_back(id);
            if (folded(id) != folded(result.name)) continue;
            if (id != result.name) fail("Stage basename has different case in presentation.json's menu_text.stage_names");
            title = &text;
        }
        if ((registered == 0) != (staged == 0) || (registered == 0) != (title == nullptr))
            fail("Partial stage registration: the name is present in only some of manifest.json, "
                "runtime_assets.cmake and presentation.json's menu_text.stage_names; reconcile all three first");
        std::sort(expected.begin(), expected.end()); std::sort(actual.begin(), actual.end());
        if (expected != actual) fail("Manifest and runtime asset list already disagree; reconcile them before registering a stage");
        std::sort(level_names.begin(), level_names.end()); std::sort(titled.begin(), titled.end());
        if (level_names != titled) fail("Manifest levels and presentation.json's menu_text.stage_names already disagree; "
            "reconcile them before registering a stage");

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
        // The game draws a shipped stage by its stage_names entry, whose base
        // English must be the level's display_name byte for byte, on one line.
        const rapidjson::Document level = json_document(result.stage_original, path_text(result.stage.filename()));
        const std::string display_name = text(field(level, "display_name"), "display_name");
        if (display_name.find_first_not_of(' ') == std::string::npos ||
            display_name.find_first_of("\r\n") != std::string::npos)
            fail("A shipped stage's display_name must be one nonblank line: "
                "presentation.json's menu_text.stage_names carries it as the stage's title");
        if (title && *title != display_name)
            fail("presentation.json's menu_text.stage_names." + result.name + " is '" + *title +
                "' but the level's display_name is '" + display_name + "'; make them agree");
        recheck(result.stage, result.stage_original);
        recheck(result.manifest.path, result.manifest.original);
        recheck(result.runtime_assets.path, result.runtime_assets.original);
        recheck(result.presentation.path, result.presentation.original);
        result.already_registered = registered == 1;
        result.manifest.proposed = result.already_registered ? result.manifest.original :
            append_name(result.manifest.original, level_group, result.name);
        result.runtime_assets.proposed = result.already_registered ? result.runtime_assets.original :
            append_asset(result.runtime_assets.original, runtime, level_asset);
        result.presentation.proposed = result.already_registered ? result.presentation.original :
            append_stage_name(result.presentation.original, result.name, display_name);
        if (!result.already_registered)
        {
            const StageNames proposed = stage_names(result.presentation.proposed);
            if (proposed.entries.size() != titles.entries.size() + 1 || proposed.entries.back().first != result.name ||
                proposed.entries.back().second != display_name)
                fail("The proposed presentation.json does not read back the new stage name; add it by hand");
        }
        return result;
    }

    bool apply_stage_registration(const StageRegistrationPreview& preview, const StageRegistrationReplace& replace)
    {
        const StageRegistrationPreview fresh = preview_stage_registration(preview.repository, preview.stage);
        const std::array<const StageRegistrationFile*, 3> files{ &fresh.manifest, &fresh.runtime_assets, &fresh.presentation };
        const std::array<const StageRegistrationFile*, 3> previewed{ &preview.manifest, &preview.runtime_assets,
            &preview.presentation };
        bool same = fresh.name == preview.name && fresh.stage_original == preview.stage_original &&
            fresh.already_registered == preview.already_registered;
        for (std::size_t index = 0; index < files.size(); ++index)
            same = same && same_path(files[index]->path, previewed[index]->path) &&
                files[index]->original == previewed[index]->original && files[index]->proposed == previewed[index]->proposed;
        if (!same) fail("Registration inputs or proposal changed; preview again");
        if (fresh.already_registered) return false;
        std::vector<std::unique_ptr<PreparedFile>> candidates, originals;
        for (const StageRegistrationFile* file : files)
            candidates.push_back(std::make_unique<PreparedFile>(file->path, "registration", "-candidate",
                file->proposed));
        for (const StageRegistrationFile* file : files)
            originals.push_back(std::make_unique<PreparedFile>(file->path, "registration", "-original",
                file->original));
        const auto perform = [&](const fs::path& source, const fs::path& target)
        { if (replace) replace(source, target); else replace_prepared_file(source, target); };
        // Before each replacement, every file is what the order says it should
        // be by then: the ones done are proposed, the rest still original.
        const auto recheck_all = [&](std::size_t done)
        {
            recheck(fresh.stage, fresh.stage_original);
            for (std::size_t index = 0; index < files.size(); ++index)
                recheck(files[index]->path, index < done ? files[index]->proposed : files[index]->original);
        };
        std::size_t done = 0;
        try
        {
            for (; done < files.size(); ++done)
            {
                recheck_all(done);
                perform(candidates[done]->path(), files[done]->path);
            }
        }
        catch (...)
        {
            const std::exception_ptr failure = std::current_exception();
            if (done == 0) throw;
            // Restore the replaced files newest first. If any cannot be
            // restored, every original backup not used up by a restore is
            // retained and named, in file order.
            std::vector<bool> restored(files.size(), false);
            std::string rollback_failures;
            for (std::size_t index = done; index-- > 0;)
            {
                try
                {
                    recheck(files[index]->path, files[index]->proposed);
                    perform(originals[index]->path(), files[index]->path);
                    restored[index] = true;
                }
                catch (...)
                {
                    rollback_failures += (rollback_failures.empty() ? "" : "; ") + path_text(files[index]->path) + ": " +
                        failure_text(std::current_exception());
                }
            }
            if (!rollback_failures.empty())
            {
                std::vector<fs::path> retained;
                for (std::size_t index = 0; index < files.size(); ++index)
                {
                    if (restored[index]) continue;
                    originals[index]->retain();
                    retained.push_back(originals[index]->path());
                }
                // The caller says "Registration failed:"; the message is the cause.
                throw StageRegistrationError(failure_text(failure) +
                    "; rollback could not complete: " + rollback_failures +
                    ". Original backups retained; inspect all three content files before retrying.", retained);
            }
            throw StageRegistrationError(failure_text(failure) + "; the replaced files were restored.");
        }
        return true;
    }
}
