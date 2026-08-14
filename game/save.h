#pragma once

#include <iostream>
#include <chrono>
#include "game/directory_consts.h"
#include "engine/math/matt_math.h"
#include "engine/render/screen_resolution.h"

struct SaveData
{
	// 1080p, which is what the game has actually started at for as long as
	// anyone has run it: a tracked save_data.json saying so was copied beside
	// the exe by the build, so the 720p written here was the stated default
	// and never the effective one. The seed is gone, so this is the only place
	// that answers the question now, and it answers it the way the game has
	// always behaved rather than quietly dropping every fresh build to 720p.
	//
	// It is also the resolution the menus are authored at - MenuPage scales
	// every widget from a 1920x1080 design space - so it is the one value that
	// needs no scaling at all.
	artattack::ScreenResolution resolution = artattack::ScreenResolution::s_1920_1080;
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
