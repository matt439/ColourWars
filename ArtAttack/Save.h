#ifndef SAVE_H
#define SAVE_H
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <chrono>
#include "directory_consts.h"

struct save_data
{
	std::string resolution = "1280x720";
	std::string full_screen = "false";
};

class Save
{
private:
	save_data _save_data = save_data();
	std::vector<std::string> read_raw_save_file();
	save_data decode_save_file(const std::vector<std::string>& save_file);
	bool write_save_file(const save_data& data);
	bool check_if_save_file_exists();
public:
	Save() {}
	void load_save_file();
	void save_to_file();
	save_data get_save_data() const { return this->_save_data; }
	void set_resolution_and_save(const std::string& resolution);
	std::string get_resolution() const { return this->_save_data.resolution; }
	bool get_full_screen() const { return this->_save_data.full_screen == "true"; }
	void set_full_screen_and_save(bool full_screen);
};

#endif // !SAVE_H