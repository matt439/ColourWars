#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <map>
#include <string>
//#include "SpriteFrame.h"
#include "SpriteSheet.h"
#include "level_stage.h"
#include "LevelLoadedInfo.h"
#include <Audio.h>
#include "SoundBank.h"


class ResourceManager
{
private:
	std::map<std::string,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> _textures;
	std::map<std::string, std::unique_ptr<DirectX::SpriteFont>> _sprite_fonts;
	std::map<std::string, std::unique_ptr<SpriteSheet>> _sprite_sheets;
	std::map<level_stage, std::unique_ptr<LevelLoadedInfo>> _level_infos;
	//std::map<std::string, std::unique_ptr<DirectX::WaveBank>> _wave_banks;

	std::map<std::string, std::unique_ptr<SoundBank>> _sound_banks;

public:
	ResourceManager() = default;

	ID3D11ShaderResourceView* get_texture(const std::string& texture_name);
	void add_texture(const std::string& texture_name,
		ID3D11ShaderResourceView* texture);

	DirectX::SpriteFont* get_sprite_font(const std::string& font_name);
	void add_sprite_font(const std::string& font_name,
		std::unique_ptr<DirectX::SpriteFont> font);

	SpriteSheet* get_sprite_sheet(const std::string& sprite_sheet_name);
	void add_sprite_sheet(const std::string& sprite_sheet_name,
		std::unique_ptr<SpriteSheet> sprite_sheet);

	const LevelLoadedInfo* get_level_info(level_stage stage) const;
	void add_level_info(level_stage stage,
		std::unique_ptr<LevelLoadedInfo> level_info);

	//DirectX::WaveBank* get_wave_bank(const std::string& wave_bank_name);
	//void add_wave_bank(const std::string& wave_bank_name,
	//			std::unique_ptr<DirectX::WaveBank> wave_bank);
	//std::unique_ptr<DirectX::SoundEffectInstance> create_sound_effect_instance(
	//	const std::string& wave_bank_name,
	//	const std::string& wave_name);

	void add_sound_bank(const std::string& sound_bank_name,
		std::unique_ptr<SoundBank> sound_bank);
	SoundBank* get_sound_bank(const std::string& sound_bank_name);

	void reset_all_sprite_fonts();
	void reset_all_textures();
	void reset_all_sounds();
};
#endif // !RESOURCEMANAGER_H
