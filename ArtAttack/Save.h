#ifndef SAVE_H
#define SAVE_H

#include <iostream>
#include <chrono>
#include "directory_consts.h"
#include <rapidjson/document.h>
#include "MattMath.h"
#include "screen_resolution.h"

struct SaveData
{
	screen_resolution resolution = screen_resolution::S_1280_720;
	bool fullscreen = false;
};

class Save
{
public:
	Save() = default;
	void load_save_file();
	void save_to_file() const;
	SaveData get_save_data() const { return this->_save_data; }
	void set_resolution_and_save(screen_resolution resolution);
	screen_resolution get_resolution() const { return this->_save_data.resolution; }
	bool get_fullscreen() const { return this->_save_data.fullscreen; }
	void set_full_screen_and_save(bool full_screen);
private:
	SaveData _save_data = SaveData();
	SaveData load_from_json(const char* json_path) const;
	bool write_save_file(const SaveData& data) const;
	static bool check_if_save_file_exists();

	screen_resolution convert_ivec_to_resolution(
		const MattMath::Vector2I& vec) const;

	MattMath::Vector2I convert_resolution_to_ivec(
		screen_resolution resolution) const;
};

#endif // !SAVE_H