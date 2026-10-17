#include "editor/content_catalog.h"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string_view>

namespace colourwars_editor
{
    namespace
    {
        using Json = rapidjson::Value;

        // `name` is the path as the manifest spells it, for the problem line.
        rapidjson::Document read(const std::filesystem::path& path, const std::string& name)
        {
            std::ifstream stream(path, std::ios::binary);
            if (!stream) throw std::runtime_error("cannot read " + name);
            const std::string bytes{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
            std::string_view text = bytes;
            if (text.starts_with("\xEF\xBB\xBF")) text.remove_prefix(3);
            rapidjson::Document document;
            document.Parse<rapidjson::kParseValidateEncodingFlag>(text.data(), text.size());
            if (document.HasParseError())
                throw std::runtime_error(name + ": invalid JSON at byte " + std::to_string(document.GetErrorOffset()) +
                    ": " + rapidjson::GetParseError_En(document.GetParseError()));
            if (!document.IsObject()) throw std::runtime_error(name + " is not a JSON object");
            return document;
        }

        const Json& array(const Json& object, const char* field, const std::string& name)
        {
            if (!object.IsObject() || !object.HasMember(field) || !object[field].IsArray())
                throw std::runtime_error(name + " needs a '" + field + "' array");
            return object[field];
        }

        std::string text(const Json& value, const std::string& name)
        {
            if (!value.IsString()) throw std::runtime_error(name + " has a name that is not text");
            return {value.GetString(), value.GetStringLength()};
        }

        // SpriteSheet::resolve_sprite_frame looks only in `sprite_frames`; a strip
        // name is not a frame even though both live in one file.
        std::set<std::string> sprite_frames(const Json& sheet, const std::string& name)
        {
            std::set<std::string> result;
            for (const Json& frame : array(sheet, "sprite_frames", name).GetArray())
            {
                if (!frame.IsObject() || !frame.HasMember("name"))
                    throw std::runtime_error(name + " has a sprite frame without a name");
                result.insert(text(frame["name"], name));
            }
            return result;
        }

        // The engine's read_sound_bank_definition: each wave when the flag is
        // set, then every named instance.
        std::set<std::string> sound_effects(const Json& bank, const std::string& name)
        {
            std::set<std::string> result;
            const Json& waves = array(bank, "waves", name);
            if (!bank.HasMember("create_effect_instance_for_each_wave") ||
                !bank["create_effect_instance_for_each_wave"].IsBool())
                throw std::runtime_error(name + " needs create_effect_instance_for_each_wave true or false");
            if (bank["create_effect_instance_for_each_wave"].GetBool())
                for (const Json& wave : waves.GetArray()) result.insert(text(wave, name));
            for (const Json& effect : array(bank, "sound_effect_instances", name).GetArray())
            {
                if (!effect.IsObject() || !effect.HasMember("name"))
                    throw std::runtime_error(name + " has a sound effect instance without a name");
                result.insert(text(effect["name"], name));
            }
            return result;
        }
    }

    ContentCatalog load_content_catalog(const std::filesystem::path& content_directory)
    {
        ContentCatalog catalog;
        rapidjson::Document manifest;
        try { manifest = read(content_directory / "manifest.json", "manifest.json"); }
        catch (const std::exception& error)
        {
            catalog.problems.push_back(error.what());
            return catalog;
        }
        try
        {
            for (const Json& group : array(manifest, "assets", "manifest.json").GetArray())
            {
                if (!group.IsObject() || !group.HasMember("kind") || !group["kind"].IsString()) continue;
                const std::string kind = text(group["kind"], "manifest.json");
                const bool sheet = kind == "sprite_sheet";
                if (!sheet && kind != "sound_bank") continue;
                if (!group.HasMember("directory"))
                    throw std::runtime_error("manifest.json has a " + kind + " group without a directory");
                const std::string directory = text(group["directory"], "manifest.json");
                auto& names = sheet ? catalog.sprite_frames : catalog.sound_effects;
                for (const Json& value : array(group, "names", "manifest.json").GetArray())
                {
                    const std::string member = text(value, "manifest.json");
                    const std::string file = directory + member + ".json";
                    std::optional<std::set<std::string>>& entry = names[member];
                    try
                    {
                        const rapidjson::Document json = read(content_directory /
                            std::filesystem::path(std::u8string(file.begin(), file.end())), file);
                        entry = sheet ? sprite_frames(json, file) : sound_effects(json, file);
                    }
                    catch (const std::exception& error) { catalog.problems.push_back(error.what()); }
                }
            }
            catalog.manifest_read = true;
        }
        catch (const std::exception& error)
        {
            catalog.problems.push_back(error.what());
            catalog.sprite_frames.clear();
            catalog.sound_effects.clear();
        }
        return catalog;
    }
}
