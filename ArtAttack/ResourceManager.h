#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <map>
#include <string>
#include "engine/render/sprite_sheet.h"
#include "level_stage.h"
#include "LevelLoadedInfo.h"
#include <Audio.h>
#include "engine/audio/sound_bank.h"

class ResourceManager
{
public:
	ResourceManager() = default;

	// The getters are const and non-mutating: they are called per-draw from
	// thread-pool workers, so they must not touch the maps. Each throws
	// std::out_of_range naming the resource if it is absent or released.
	ID3D11ShaderResourceView* get_texture(const std::string& texture_name) const;

	void add_texture(const std::string& texture_name,
		ID3D11ShaderResourceView* texture);

	DirectX::SpriteFont* get_sprite_font(const std::string& font_name) const;

	void add_sprite_font(const std::string& font_name,
		std::unique_ptr<DirectX::SpriteFont> font);

	SpriteSheet* get_sprite_sheet(const std::string& sprite_sheet_name) const;

	void add_sprite_sheet(const std::string& sprite_sheet_name,
		std::unique_ptr<SpriteSheet> sprite_sheet);

	const LevelLoadedInfo* get_level_info(level_stage stage) const;

	void add_level_info(level_stage stage,
		std::unique_ptr<LevelLoadedInfo> level_info);

	void add_sound_bank(const std::string& sound_bank_name,
		std::unique_ptr<SoundBank> sound_bank);

	SoundBank* get_sound_bank(const std::string& sound_bank_name) const;

	void reset_all_sprite_fonts();
	void reset_all_textures();
	void reset_all_sounds();

private:
	std::map<std::string,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> _textures;

	std::map<std::string, std::unique_ptr<DirectX::SpriteFont>> _sprite_fonts;
	std::map<std::string, std::unique_ptr<SpriteSheet>> _sprite_sheets;
	std::map<level_stage, std::unique_ptr<LevelLoadedInfo>> _level_infos;
	std::map<std::string, std::unique_ptr<SoundBank>> _sound_banks;
};
#endif // !RESOURCEMANAGER_H
