#pragma once

#include <iostream>
#include <chrono>
#include "game/directory_consts.h"
#include <rapidjson/document.h>
#include "engine/math/matt_math.h"
#include "engine/render/screen_resolution.h"

struct SaveData
{
	artattack::ScreenResolution resolution = artattack::ScreenResolution::s_1280_720;
	bool fullscreen = false;
};

class Save
{
public:
	Save() = default;
	void load_save_file();
	void save_to_file() const;
	SaveData save_data() const;
	void set_resolution_and_save(artattack::ScreenResolution resolution);
	artattack::ScreenResolution resolution() const;
	bool fullscreen() const;
	void set_full_screen_and_save(bool full_screen);
private:
	SaveData save_data_ = SaveData();
	SaveData load_from_json(const char* json_path) const;
	bool write_save_file(const SaveData& data) const;
	static bool check_if_save_file_exists();

	artattack::ScreenResolution convert_ivec_to_resolution(
		const mattmath::Vector2I& vec) const;

	mattmath::Vector2I convert_resolution_to_ivec(
		artattack::ScreenResolution resolution) const;
};
