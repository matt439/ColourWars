#include "editor/user_stage_save.h"
#include "game/user_stage_contract.h"

#include <algorithm>
#include <cstdlib>
#include <cwctype>
#include <memory>
#include <stdexcept>
#include <system_error>

namespace colourwars_editor
{
    namespace
    {
        namespace fs = std::filesystem;

        std::string shown(const fs::path& path)
        {
            const std::u8string bytes = path.u8string();
            return { reinterpret_cast<const char*>(bytes.data()), bytes.size() };
        }

        std::wstring folded(std::wstring text)
        {
            std::transform(text.begin(), text.end(), text.begin(),
                [](wchar_t character) { return static_cast<wchar_t>(std::towlower(character)); });
            return text;
        }

        // Windows paths compare without case; the folder need not exist yet.
        bool same_folder(const fs::path& first, const fs::path& second)
        {
            const auto normal = [](const fs::path& path)
            {
                fs::path result = fs::absolute(path).lexically_normal();
                if (!result.has_filename()) result = result.parent_path();
                return folded(result.native());
            };
            return normal(first) == normal(second);
        }

        bool shipped_name(const std::vector<std::string>& shipped, const std::string& name,
            std::string& taken)
        {
            const auto match = std::find_if(shipped.begin(), shipped.end(),
                [&name](const std::string& stage) { return user_stage_contract::same_name(stage, name); });
            if (match == shipped.end()) return false;
            taken = *match;
            return true;
        }

        // The stem as UTF-8 when it is plain ASCII; anything else fails the
        // name rule, so its exact bytes do not matter.
        std::string stem_of(const fs::path& path)
        {
            const std::wstring wide = path.stem().native();
            std::string narrow;
            for (const wchar_t character : wide)
                narrow.push_back(character < 0x80 ? static_cast<char>(character) : '?');
            return narrow;
        }
    }

    fs::path user_stage_folder()
    {
        size_t length = 0;
        wchar_t* raw = nullptr;
        const errno_t result = _wdupenv_s(&raw, &length, L"LOCALAPPDATA");
        const std::unique_ptr<wchar_t, decltype(&std::free)> buffer(raw, &std::free);
        if (result != 0 || buffer == nullptr || length <= 1)
            throw std::runtime_error("LOCALAPPDATA is not set, so the editor cannot find the folder "
                "the game reads your stages from. Use Save As and copy the file there instead.");
        return user_stage_contract::folder_under(fs::path(buffer.get()));
    }

    UserStageTarget check_user_stage_target(const fs::path& folder, const fs::path& chosen,
        const std::vector<std::string>& shipped)
    {
        if (!same_folder(chosen.parent_path(), folder))
            throw std::runtime_error("Save to My Stages saves directly in " + shown(folder) +
                ", which the game reads at startup and when Stage Select opens. "
                "Use Save As for any other folder.");
        if (folded(chosen.extension().native()) != L".json")
            throw std::runtime_error("The game reads only .json files from My Stages.");
        const std::string name = stem_of(chosen);
        if (!user_stage_contract::usable_name(name))
            throw std::runtime_error("The game would refuse '" + shown(chosen.filename()) + "': " +
                user_stage_contract::name_rule() + ".");
        std::string taken;
        if (shipped_name(shipped, name, taken))
            throw std::runtime_error("'" + name + "' is a shipped stage's name ('" + taken +
                "'), and the game would refuse a stage of yours with it. Choose another name.");

        UserStageTarget target{ folder / chosen.filename(), name, false };
        std::error_code error;
        if (!fs::is_directory(folder, error)) return target;
        for (fs::directory_iterator entries(folder, error); !error && entries != fs::directory_iterator();
            entries.increment(error))
        {
            const fs::directory_entry& entry = *entries;
            if (folded(entry.path().extension().native()) != L".json") continue;
            if (!user_stage_contract::same_name(stem_of(entry.path()), name)) continue;
            std::error_code kind;
            if (!entry.is_regular_file(kind))
                throw std::runtime_error(shown(entry.path()) + " is not a file; move it out of the way first.");
            target.file = entry.path();
            target.name = stem_of(entry.path());
            target.replaces = true;
            break;
        }
        if (error)
            throw std::runtime_error("Cannot list " + shown(folder) + ": " + error.message());
        return target;
    }

    std::string suggested_user_stage_name(const LevelDocument& document,
        const std::vector<std::string>& shipped)
    {
        std::string taken;
        const std::string stem = document.path().empty() ? std::string() : stem_of(document.path());
        if (user_stage_contract::usable_name(stem) && !shipped_name(shipped, stem, taken)) return stem;
        const std::string mine = "my_" + stem;
        if (!stem.empty() && user_stage_contract::usable_name(mine) && !shipped_name(shipped, mine, taken))
            return mine;
        return "my_stage";
    }

    void save_user_stage(LevelDocument& document, const fs::path& folder, const UserStageTarget& target)
    {
        std::error_code error;
        fs::create_directories(folder, error);
        if (error)
            throw std::runtime_error("Cannot create " + shown(folder) + ": " + error.message());
        document.save(target.file);
    }
}
