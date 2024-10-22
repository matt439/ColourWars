#include "pch.h"
#include "Save.h"
#include <cstdio>
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"

using namespace rapidjson;
using namespace directory_consts;
using namespace MattMath;

save_data Save::load_from_json(const char* json_path)
{
    FILE* fp = fopen(json_path, "rb");

    std::unique_ptr<char> readBuffer = std::make_unique<char>();
    FileReadStream is(fp, readBuffer.get(), sizeof(readBuffer));

    Document d;
    d.ParseStream(is);

    fclose(fp);
    readBuffer.release();

    Vector2I resolution = Vector2I(d["resolution"]["x"].GetInt(), d["resolution"]["y"].GetInt());
    bool fullscreen = d["fullscreen"].GetBool();

    save_data data = save_data();
    data.resolution = this->convert_ivec_to_resolution(resolution);
    data.fullscreen = fullscreen;
    return data;
}
bool Save::check_if_save_file_exists()
{
    FILE* fp = fopen(SAVE_FILE_PATH.c_str(), "rb");
    if (fp == NULL)
    {
		return false;
	}
	fclose(fp);
	return true;
}
void Save::load_save_file()
{
    if (this->check_if_save_file_exists())
    {
		std::cout << "Save file exists.\n";
	}
    else
    {
        std::cout << "Save file does not exist.\n";
        this->save_to_file();
    }
    this->_save_data =
        this->load_from_json(SAVE_FILE_PATH.c_str());
}

bool Save::write_save_file(const save_data& data)
{
    FILE* fp = fopen(SAVE_FILE_PATH.c_str(), "wb");
    if (fp == NULL)
    {
        return false;
    }
    // Allocate buffer on the heap
    std::unique_ptr<char[]> writeBuffer = std::make_unique<char[]>(65536);
    FileWriteStream os(fp, writeBuffer.get(), 65536);

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

void Save::save_to_file()
{
    auto start = std::chrono::system_clock::now();
    auto legacyStart = std::chrono::system_clock::to_time_t(start);
    char buffer[30];
    std::cout << ctime_s(buffer, sizeof(buffer), &legacyStart) << " Saving...\n";
    if (write_save_file(this->_save_data))
    {
		std::cout << "Save was successful.\n";
	}
    else
    {
		std::cout << "Save was unsuccessful.\n";
	}
}

void Save::set_resolution_and_save(screen_resolution resolution)
{
    this->_save_data.resolution = resolution;
    this->save_to_file();
}

void Save::set_full_screen_and_save(bool full_screen)
{
    this->_save_data.fullscreen = full_screen;
	this->save_to_file();
}
screen_resolution Save::convert_ivec_to_resolution(
    const Vector2I& vec) const
{
    if (vec.x == 1280 && vec.y == 720)
    {
        return screen_resolution::S_1280_720;
    }
    else if (vec.x == 1920 && vec.y == 1080)
    {
        return screen_resolution::S_1920_1080;
    }
    else if (vec.x == 2560 && vec.y == 1440)
    {
        return screen_resolution::S_2560_1440;
    }
    else if (vec.x == 3840 && vec.y == 2160)
    {
        return screen_resolution::S_3840_2160;
    }
    else
    {
        return screen_resolution::S_1280_720;
    }
}
Vector2I Save::convert_resolution_to_ivec(
    screen_resolution resolution) const
{
    switch (resolution)
    {
    case screen_resolution::S_1280_720:
        return Vector2I(1280, 720);
    case screen_resolution::S_1920_1080:
        return Vector2I(1920, 1080);
    case screen_resolution::S_2560_1440:
        return Vector2I(2560, 1440);
    case screen_resolution::S_3840_2160:
        return Vector2I(3840, 2160);
    default:
        return Vector2I(-1, -1);
    }
}
