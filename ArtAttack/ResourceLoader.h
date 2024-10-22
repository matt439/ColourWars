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
		DirectX::AudioEngine* audio_engine) :
		_resource_manager(resource_manager),
		_device(device),
		_audio_engine(audio_engine) {}

	void load_all_resources();

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

	void load_textures() const;
	void load_fonts();
	void load_level_info() const;
	void load_sounds();
};

#endif // !RESOURCELOADER_H
