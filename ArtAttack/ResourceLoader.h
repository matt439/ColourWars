#ifndef RESOURCELOADER_H
#define RESOURCELOADER_H

#include "ResourceManager.h"
#include "directory_consts.h"
#include "level_stage.h"
#include <Audio.h>

class ResourceLoader
{
private:
	ResourceManager* _resource_manager = nullptr;
	ID3D11Device1* _device = nullptr;
	DirectX::AudioEngine* _audio_engine = nullptr;

	//void load_main_menu_textures();
	//void load_level_textures();
	//void load_pause_menu_textures();
	//void load_results_menu_textures();

	//void load_main_menu_fonts();
	//void load_level_fonts();

	//void load_main_menu_resources();
	//void load_level_resources();
	//void load_pause_menu_resources();
	//void load_results_menu_resources();

	void load_textures();
	void load_fonts();
	void load_level_info();
	void load_sounds();
public:
	ResourceLoader(ResourceManager* resource_manager, ID3D11Device1* device,
		DirectX::AudioEngine* audio_engine) :
		_resource_manager(resource_manager),
		_device(device),
		_audio_engine(audio_engine) {}

	void load_all_resources();


	void load_texture(const std::string& texture_path,
		const std::string& texture_name);
	void load_texture_from_directory(const std::string& directory,
		const std::string& texture_name);

	void load_sprite_font(const std::string& font_path,
		const std::string& font_name);
	void load_sprite_font_from_directory(const std::string& directory,
		const std::string& font_name);


	void load_sprite_sheet(const std::string& texture_path,
		const std::string& texture_name,
		const std::string& json_path,
		const std::string& sprite_sheet_name);
	void load_sprite_sheet_from_directory(const std::string& directory,
		const std::string& name);

	void load_level_info(const std::string& json_path,
		level_stage stage);
	void load_level_info_from_directory(const std::string& directory,
		const std::string& level_name, level_stage stage);

	//void load_wave_bank(const std::string& wave_bank_path,
	//	const std::string& wave_bank_name);
	//void load_wave_bank_from_directory(const std::string& directory,
	//	const std::string& wave_bank_name);

	void load_sound_bank(const std::string& wave_bank_path,
		const std::string& json_path,
		const std::string& sound_bank_name);
	void load_sound_bank_from_directory(const std::string& directory,
		const std::string& wave_bank_name);

};

#endif // !RESOURCELOADER_H
