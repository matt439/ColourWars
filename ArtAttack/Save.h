#ifndef SAVE_H
#define SAVE_H
//#include <string>
//#include <vector>
//#include <fstream>
#include <iostream>
#include <chrono>
#include "directory_consts.h"
//#include <cstdio>
#include <rapidjson/document.h>
#include "MattMath.h"
#include "screen_resolution.h"

struct save_data
{
	//std::string resolution = "1280x720";
	screen_resolution resolution = screen_resolution::S_1280_720;
	bool fullscreen = false;
	//std::string full_screen = "false";
};

class Save
{
private:
	save_data _save_data = save_data();
	//std::vector<std::string> read_raw_save_file();
	save_data load_from_json(const char* json_path);
	//save_data decode_save_file(const std::vector<std::string>& save_file);
	bool write_save_file(const save_data& data);
	bool check_if_save_file_exists();

	screen_resolution convert_ivec_to_resolution(
		const MattMath::Vector2I& vec) const;
	MattMath::Vector2I convert_resolution_to_ivec(
		screen_resolution resolution) const;
public:
	Save() = default;
	void load_save_file();
	void save_to_file();
	save_data get_save_data() const { return this->_save_data; }
	void set_resolution_and_save(screen_resolution resolution);
	//std::string get_resolution() const { return this->_save_data.resolution; }
	//MattMath::Vector2I get_resolution() const { return this->_save_data.resolution; }
	screen_resolution get_resolution() const { return this->_save_data.resolution; }
	bool get_fullscreen() const { return this->_save_data.fullscreen; }
	//bool get_full_screen() const { return this->_save_data.full_screen == "true"; }
	void set_full_screen_and_save(bool full_screen);
};

#endif // !SAVE_H