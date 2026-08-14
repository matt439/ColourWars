#include "game/pch.h"
#include "game/save.h"
#include "engine/assets/json.h"
#include <cstdio>
#include <filesystem>
#include <system_error>
// The save file is the one JSON this project writes rather than reads, and the
// engine's assets module only reads. So rapidjson is here on the game's own
// account - which is what game/CMakeLists.txt asks for it for.
#include "rapidjson/document.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"

using namespace rapidjson;
using namespace directory_consts;
using namespace mattmath;
using namespace artattack;

SaveData Save::load_from_json(const char* json_path) const
{
    const JsonDocument document = read_json_file(json_path);
    const JsonValue root = document.root();

    auto data = SaveData();

    // This is the one file in the project where a bad value is not an error, so
    // it is the one place that turns the loaders' loud failure (T6) back into a
    // shrug. A save written by an older build, or one a user has hand-edited,
    // degrades to the default - and per field, so a garbled resolution does not
    // also cost the fullscreen setting.
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

    return data;
}
bool Save::file_exists(const std::string& path)
{
    FILE* fp = nullptr;
    if (fopen_s(&fp, path.c_str(), "rb") != 0 || fp == nullptr)
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
        // Nothing at the real location. Before writing defaults over the
        // question, look where the save used to be kept - beside the exe,
        // under the build directory - and take those settings if they are
        // there. Read only, and never removed: a build tree is not this
        // game's to tidy up, the copy left behind is harmless, and one that
        // was deleted after a failed write would be a setting lost to a
        // migration nobody asked for.
        //
        // This is a one-way door by construction rather than by a flag. Once
        // anything writes the new file, this arm is never taken again.
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

        // Either way the file now exists at the new path, with whatever was
        // recovered or with the defaults.
        this->save_to_file();
        return;
    }

    try
    {
        this->save_data_ = this->load_from_json(path.c_str());
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
    const std::string path = save_file_path();

    // fopen("wb") does not create directories, so a deployed build with no
    // save/ folder would fail here on every attempt. Now that the save lives
    // under %LOCALAPPDATA% this is not an edge case at all: the game's folder
    // there does not exist until the first time it is written.
    const std::filesystem::path save_path(path);
    if (save_path.has_parent_path())
    {
        std::error_code ec;
        std::filesystem::create_directories(save_path.parent_path(), ec);
    }

    // Written beside the save and renamed over it, never straight into it.
    // Opening the real file "wb" truncates it before the replacement exists,
    // so a write that then fails - a full disk, a read-only Program Files -
    // has destroyed the settings it was replacing as well as failing to store
    // the new ones. A rename either happens or does not.
    const std::string temp_file_path = path + ".tmp";

    FILE* fp = nullptr;
    if (fopen_s(&fp, temp_file_path.c_str(), "wb") != 0 || fp == nullptr)
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

    // Accept() flushes the stream when the top-level value ends, so every byte
    // has reached fwrite by the time fclose is called below.
    Writer<FileWriteStream> writer(os);
    d.Accept(writer);

    // fclose is the only call in this function that can report a failed write
    // for a document this small: FileWriteStream::Flush discards the result of
    // its fwrite by design, so everything above can "succeed" on a full disk.
    // Its result is the save's result, which is why it is not discarded.
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

void Save::save_to_file() const
{
    auto start = std::chrono::system_clock::now();
    auto legacy_start = std::chrono::system_clock::to_time_t(start);
    // ctime_s writes the time into `buffer` and returns an errno_t; streaming
    // that return value printed "0 Saving..." and never read the time at all.
    // What it writes ends in a newline of its own.
    char buffer[30] = {};
    if (ctime_s(buffer, sizeof(buffer), &legacy_start) != 0)
    {
        buffer[0] = '\0';
    }
    std::cout << buffer << "Saving...\n";
    if (write_save_file(this->save_data_))
    {
		std::cout << "Save was successful.\n";
        return;
	}

    // A settings change that never reached the disk looks applied - the window
    // resizes - and then comes back at the default on the next launch, which is
    // exactly the failure a user cannot diagnose. std::cout above is a trace in
    // a debugger and nothing else: this is a WIN32-subsystem exe with no
    // console, so a report that has to be seen goes the way main() sends its
    // startup failures, stderr paired with a message box (T6).
    const std::string message = "Could not write " + save_file_path() +
        ".\nYour settings will not be remembered.";
    fprintf(stderr, "save failure: %s\n", message.c_str());
    MessageBoxA(nullptr, message.c_str(), "ArtAttack - save failure",
        MB_OK | MB_ICONWARNING);
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
