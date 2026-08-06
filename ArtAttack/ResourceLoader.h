#ifndef RESOURCELOADER_H
#define RESOURCELOADER_H

#include "ResourceManager.h"
#include "directory_consts.h"
#include "level_stage.h"
#include <Audio.h>

class ResourceLoader
{
public:
	ResourceLoader(ResourceManager* resource_manager, ID3D11Device1* device,
		DirectX::AudioEngine* audio_engine);

	void load_all_resources();

	// Rebuilds only what actually lives on the GPU (fonts, textures) after a
	// device loss, replaying exactly what load_all_resources() loaded.
	//
	// Sound banks, level info and the SpriteSheet objects themselves are NOT
	// recreated: they are not device resources, and callers hold raw
	// SoundBank*, LevelLoadedInfo* and SpriteSheet* pointers to them. Each
	// SpriteSheet has its texture re-seated in place instead.
	void reload_device_resources();

	// The device changes identity on restore, so the loader has to be told.
	void set_device(ID3D11Device1* device);

	void load_texture(const std::string& texture_path,
		const std::string& texture_name) const;
	void load_texture_from_directory(const std::string& directory,
		const std::string& texture_name) const;

	void load_sprite_font(const std::string& font_path,
		const std::string& font_name);
	void load_sprite_font_from_directory(const std::string& directory,
		const std::string& font_name);


	void load_sprite_sheet(const std::string& texture_path,
		const std::string& texture_name,
		const std::string& json_path,
		const std::string& sprite_sheet_name) const;
	void load_sprite_sheet_from_directory(const std::string& directory,
		const std::string& name) const;

	void load_level_info(const std::string& json_path,
		level_stage stage) const;
	void load_level_info_from_directory(const std::string& directory,
		const std::string& level_name, level_stage stage) const;

	void load_sound_bank(const std::string& wave_bank_path,
		const std::string& json_path,
		const std::string& sound_bank_name);
	void load_sound_bank_from_directory(const std::string& directory,
		const std::string& wave_bank_name);

private:
	ResourceManager* _resource_manager = nullptr;
	ID3D11Device1* _device = nullptr;
	DirectX::AudioEngine* _audio_engine = nullptr;

	// What was loaded, so a device restore can replay it without a second
	// hardcoded copy of the asset list.
	struct asset_record
	{
		std::string directory;
		std::string name;
	};
	std::vector<asset_record> _loaded_fonts;
	std::vector<asset_record> _loaded_sprite_sheets;

	void load_textures();
	void load_fonts();
	void load_level_info() const;
	void load_sounds();

	// Reloads the sheet's .dds and points the existing SpriteSheet at it. The
	// frame and animation-strip tables are device-independent, so they are left
	// alone and every cached SpriteSheet*/AnimationStrip* stays valid.
	void reload_sprite_sheet_texture(const std::string& directory,
		const std::string& name) const;
};

#endif // !RESOURCELOADER_H
