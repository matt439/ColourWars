#pragma once

#include <iostream>
#include <chrono>
#include "game/directory_consts.h"
#include "engine/math/vector2i.h"
#include "engine/render/screen_resolution.h"
#include "game/objects/team_colour.h"
#include "game/play_settings.h"

struct SaveData
{
	// Default to the menus' 1920x1080 design resolution.
	labrador::ScreenResolution resolution = labrador::ScreenResolution::s_1920_1080;
	bool fullscreen = false;
	TeamColourPair team_palette = TeamColourPair::random;
	PlaySettings play;
};

class Save
{
public:
	Save() = default;
	void load_save_file();
	void save_to_file() const;
	SaveData save_data() const;
	void set_resolution_and_save(labrador::ScreenResolution resolution);
	labrador::ScreenResolution resolution() const;
	bool fullscreen() const;
	void set_full_screen_and_save(bool full_screen);
	TeamColourPair team_palette() const;
	void set_team_palette_and_save(TeamColourPair palette);
	const PlaySettings& play_settings() const;
	void set_play_settings_and_save(const PlaySettings& settings);
private:
	SaveData save_data_ = SaveData();
	SaveData load_from_json(const char* json_path) const;
	bool write_save_file(const SaveData& data) const;

	static bool file_exists(const std::string& path);

	labrador::ScreenResolution convert_ivec_to_resolution(
		const mattmath::Vector2I& vec) const;

	mattmath::Vector2I convert_resolution_to_ivec(
		labrador::ScreenResolution resolution) const;
};
