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
	// A failed write leaves the active preferences intact and returns false.
	// Interactive callers report that outcome in their own settings screen.
	bool save_to_file() const;
	SaveData save_data() const;
	bool set_display_settings_and_save(labrador::ScreenResolution resolution,
		bool fullscreen, TeamColourPair palette);
	labrador::ScreenResolution resolution() const;
	bool fullscreen() const;
	TeamColourPair team_palette() const;
	const PlaySettings& play_settings() const;
	bool set_play_settings_and_save(const PlaySettings& settings);
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
