#include "game/pch.h"
#include "game/save.h"
#include "engine/assets/json_loader.h"
#include <cstdio>
#include <filesystem>
#include <system_error>
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"

using namespace rapidjson;
using namespace directory_consts;
using namespace mattmath;

SaveData Save::load_from_json(const char* json_path) const
{
    Document d = json_loader::parse_file(json_path);

    // Every field is optional: a save written by an older build, or one a user
    // has hand-edited, must degrade to the default rather than crash.
    auto data = SaveData();

    if (d.IsObject() &&
        d.HasMember("resolution") && d["resolution"].IsObject() &&
        d["resolution"].HasMember("x") && d["resolution"]["x"].IsInt() &&
        d["resolution"].HasMember("y") && d["resolution"]["y"].IsInt())
    {
        data.resolution = this->convert_ivec_to_resolution(
            Vector2I(d["resolution"]["x"].GetInt(), d["resolution"]["y"].GetInt()));
    }

    if (d.IsObject() &&
        d.HasMember("fullscreen") && d["fullscreen"].IsBool())
    {
        data.fullscreen = d["fullscreen"].GetBool();
    }

    return data;
}
bool Save::check_if_save_file_exists()
{
    FILE* fp = nullptr;
    if (fopen_s(&fp, SAVE_FILE_PATH.c_str(), "rb") != 0 || fp == nullptr)
    {
		return false;
	}
	fclose(fp);
	return true;
}
void Save::load_save_file()
{
    if (!check_if_save_file_exists())
    {
        // First run, or the save directory was never created. Write the
        // defaults out so the file exists from here on.
        this->save_to_file();
    }

    try
    {
        this->save_data_ = this->load_from_json(SAVE_FILE_PATH.c_str());
    }
    catch (const std::exception&)
    {
        // A missing, unreadable or corrupt save is not fatal - the defaults are
        // perfectly playable, and the next settings change overwrites the file.
        this->save_data_ = SaveData();
    }
}

bool Save::write_save_file(const SaveData& data) const
{
    // fopen("wb") does not create directories, so a deployed build with no
    // save/ folder would fail here on every attempt.
    const std::filesystem::path save_path(SAVE_FILE_PATH);
    if (save_path.has_parent_path())
    {
        std::error_code ec;
        std::filesystem::create_directories(save_path.parent_path(), ec);
    }

    FILE* fp = nullptr;
    if (fopen_s(&fp, SAVE_FILE_PATH.c_str(), "wb") != 0 || fp == nullptr)
    {
        return false;
    }
    // Allocate buffer on the heap
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

    Writer<FileWriteStream> writer(os);
    d.Accept(writer);

    fclose(fp);
    return true;
}

void Save::save_to_file() const
{
    auto start = std::chrono::system_clock::now();
    auto legacy_start = std::chrono::system_clock::to_time_t(start);
    char buffer[30];
    std::cout << ctime_s(buffer, sizeof(buffer), &legacy_start) << " Saving...\n";
    if (write_save_file(this->save_data_))
    {
		std::cout << "Save was successful.\n";
	}
    else
    {
		std::cout << "Save was unsuccessful.\n";
	}
}

void Save::set_resolution_and_save(ScreenResolution resolution)
{
    this->save_data_.resolution = resolution;
    this->save_to_file();
}

void Save::set_full_screen_and_save(bool full_screen)
{
    this->save_data_.fullscreen = full_screen;
	this->save_to_file();
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
        return ScreenResolution::s_1280_720;
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
