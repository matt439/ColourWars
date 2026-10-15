#include "game/pch.h"
#include "game/save.h"
#include "engine/assets/json.h"
#include "engine/core/file_path.h"
#include <cstdio>
#include <filesystem>
#include <system_error>
// The engine reads JSON; the game uses RapidJSON to write player settings.
#include "rapidjson/document.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"

using namespace rapidjson;
using namespace directory_consts;
using namespace mattmath;
using namespace labrador;

SaveData Save::load_from_json(const char* json_path) const
{
    const JsonDocument document = read_json_file(json_path);
    const JsonValue root = document.root();

    auto data = SaveData();

    // Recover invalid or missing settings independently so one bad field does
    // not reset the other preferences.
    try
    {
        const JsonValue resolution = root.object("resolution");
        data.resolution = this->convert_ivec_to_resolution(
            Vector2I(resolution.integer("x"), resolution.integer("y")));
    }
    catch (const std::runtime_error&)
    {
    }

    try
    {
        data.fullscreen = root.boolean("fullscreen");
    }
    catch (const std::runtime_error&)
    {
    }

    try
    {
        const std::string palette = root.string("team_palette");
        for (int value = static_cast<int>(TeamColourPair::random);
            value < static_cast<int>(TeamColourPair::count); ++value)
        {
            const TeamColourPair pair = static_cast<TeamColourPair>(value);
            if (palette == TeamColourTools::palette_key(pair))
            {
                data.team_palette = pair;
                break;
            }
        }
    }
    catch (const std::runtime_error&)
    {
    }

    const auto read_preference = [&root](const char* key, int minimum,
        int maximum, int& destination)
    {
        try
        {
            const int value = root.integer(key);
            if (value >= minimum && value <= maximum)
            {
                destination = value;
            }
        }
        catch (const std::runtime_error&)
        {
        }
    };
    read_preference("aim_sensitivity", 1, 5, data.play.aim_sensitivity);
    read_preference("music_volume", 0, 100, data.play.music_volume);
    read_preference("sfx_volume", 0, 100, data.play.sfx_volume);

    try
    {
        data.play.view_shake = root.boolean("view_shake");
    }
    catch (const std::runtime_error&)
    {
    }

    try
    {
        data.play.aim_marker = root.boolean("aim_marker");
    }
    catch (const std::runtime_error&)
    {
    }

    try
    {
        data.play.reduce_effects = root.boolean("reduce_effects");
    }
    catch (const std::runtime_error&)
    {
    }

    return data;
}
bool Save::file_exists(const std::string& path)
{
    FILE* fp = nullptr;
    if (_wfopen_s(&fp, path_from_utf8(path).c_str(), L"rb") != 0 || fp == nullptr)
    {
		return false;
	}
	fclose(fp);
	return true;
}
void Save::load_save_file()
{
    const std::string path = save_file_path();

    if (!file_exists(path))
    {
        // If the per-user save cannot be opened, try the executable-relative legacy
        // save before writing defaults. The legacy file is left intact.
        const std::string legacy = legacy_save_file_path();
        if (file_exists(legacy))
        {
            try
            {
                this->save_data_ = this->load_from_json(legacy.c_str());
            }
            catch (const std::exception&)
            {
                this->save_data_ = SaveData();
            }
        }

        // There is no settings screen yet to report a failed first save.
        if (!this->save_to_file())
        {
            const std::wstring message = L"Could not write " +
                path_from_utf8(path).wstring() +
                L".\nYour settings will not be remembered.";
            MessageBoxW(nullptr, message.c_str(), L"ColourWars - save failure",
                MB_OK | MB_ICONWARNING);
        }
        return;
    }

    try
    {
        this->save_data_ = this->load_from_json(path.c_str());
    }
    catch (const std::exception&)
    {
        // Unreadable or corrupt saves use defaults; a later settings change retries saving.
        this->save_data_ = SaveData();
    }
}

bool Save::write_save_file(const SaveData& data) const
{
    const std::string path = save_file_path();

    // Create the per-user directory on first save.
    const std::filesystem::path save_path = path_from_utf8(path);
    if (save_path.has_parent_path())
    {
        std::error_code ec;
        std::filesystem::create_directories(save_path.parent_path(), ec);
    }

    // Write a sibling temporary file before replacing the save, so a failed
    // write does not truncate the existing settings.
    std::filesystem::path temp_file_path = save_path;
    temp_file_path += L".tmp";

    FILE* fp = nullptr;
    if (_wfopen_s(&fp, temp_file_path.c_str(), L"wb") != 0 || fp == nullptr)
    {
        return false;
    }
    auto write_buffer = std::make_unique<char[]>(65536);
    FileWriteStream os(fp, write_buffer.get(), 65536);

    Document d;
    d.SetObject();
    Vector2I resolution_ivec = this->convert_resolution_to_ivec(data.resolution);
    Value resolution(kObjectType);
    resolution.AddMember("x", resolution_ivec.x, d.GetAllocator());
    resolution.AddMember("y", resolution_ivec.y, d.GetAllocator());
    d.AddMember("resolution", resolution, d.GetAllocator());
    d.AddMember("fullscreen", data.fullscreen, d.GetAllocator());
    Value palette(TeamColourTools::palette_key(data.team_palette), d.GetAllocator());
    d.AddMember("team_palette", palette, d.GetAllocator());
    d.AddMember("aim_sensitivity", data.play.aim_sensitivity, d.GetAllocator());
    d.AddMember("music_volume", data.play.music_volume, d.GetAllocator());
    d.AddMember("sfx_volume", data.play.sfx_volume, d.GetAllocator());
    d.AddMember("view_shake", data.play.view_shake, d.GetAllocator());
    d.AddMember("aim_marker", data.play.aim_marker, d.GetAllocator());
    d.AddMember("reduce_effects", data.play.reduce_effects, d.GetAllocator());

    // Writer flushes when the root object ends, before fclose below.
    Writer<FileWriteStream> writer(os);
    d.Accept(writer);

    // Check fclose before replacing the save. RapidJSON's FileWriteStream does
    // not report fwrite failures, so serialization alone cannot establish success.
    const bool closed = (fclose(fp) == 0);

    std::error_code rename_ec;
    if (closed)
    {
        std::filesystem::rename(temp_file_path, save_path, rename_ec);
        if (!rename_ec)
        {
            return true;
        }
    }

    std::error_code remove_ec;
    std::filesystem::remove(temp_file_path, remove_ec);
    return false;
}

bool Save::save_to_file() const
{
    auto start = std::chrono::system_clock::now();
    auto legacy_start = std::chrono::system_clock::to_time_t(start);
    // ctime_s includes a trailing newline.
    char buffer[30] = {};
    if (ctime_s(buffer, sizeof(buffer), &legacy_start) != 0)
    {
        buffer[0] = '\0';
    }
    std::cout << buffer << "Saving...\n";
    if (write_save_file(this->save_data_))
    {
		std::cout << "Save was successful.\n";
        return true;
	}

    // The caller owns visible feedback; settings remain active in memory.
    const std::string message = "Could not write " + save_file_path() +
        ".\nYour settings will not be remembered.";
    fprintf(stderr, "save failure: %s\n", message.c_str());
    return false;
}

bool Save::set_display_settings_and_save(ScreenResolution resolution,
    bool fullscreen, TeamColourPair palette)
{
    // Validate the whole selection before changing any active preference.
    if (this->convert_resolution_to_ivec(resolution).x <= 0)
    {
        throw std::invalid_argument("Invalid display resolution");
    }
    TeamColourTools::palette_key(palette);
    this->save_data_.resolution = resolution;
    this->save_data_.fullscreen = fullscreen;
    this->save_data_.team_palette = palette;
    return this->save_to_file();
}

ScreenResolution Save::convert_ivec_to_resolution(
    const Vector2I& vec) const
{
    if (vec.x == 1280 && vec.y == 720)
    {
        return ScreenResolution::s_1280_720;
    }
    else if (vec.x == 1920 && vec.y == 1080)
    {
        return ScreenResolution::s_1920_1080;
    }
    else if (vec.x == 2560 && vec.y == 1440)
    {
        return ScreenResolution::s_2560_1440;
    }
    else if (vec.x == 3840 && vec.y == 2160)
    {
        return ScreenResolution::s_3840_2160;
    }
    else
    {
        return SaveData{}.resolution;
    }
}
Vector2I Save::convert_resolution_to_ivec(
    ScreenResolution resolution) const
{
    switch (resolution)
    {
    case ScreenResolution::s_1280_720:
        return {1280, 720};
    case ScreenResolution::s_1920_1080:
        return {1920, 1080};
    case ScreenResolution::s_2560_1440:
        return {2560, 1440};
    case ScreenResolution::s_3840_2160:
        return {3840, 2160};
    default:
        return {-1, -1};
    }
}

SaveData Save::save_data() const
{
	return this->save_data_;
}

ScreenResolution Save::resolution() const
{
	return this->save_data_.resolution;
}

bool Save::fullscreen() const
{
	return this->save_data_.fullscreen;
}

TeamColourPair Save::team_palette() const
{
    return this->save_data_.team_palette;
}

const PlaySettings& Save::play_settings() const
{
    return this->save_data_.play;
}

bool Save::set_play_settings_and_save(const PlaySettings& settings)
{
    if (settings.aim_sensitivity < 1 || settings.aim_sensitivity > 5 ||
        settings.music_volume < 0 || settings.music_volume > 100 ||
        settings.sfx_volume < 0 || settings.sfx_volume > 100)
    {
        throw std::invalid_argument("Play settings are outside their supported range");
    }
    this->save_data_.play = settings;
    return this->save_to_file();
}
