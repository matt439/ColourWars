#include "pch.h"
#include "ResourceManager.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
	// Every getter needs the same three-part contract: the key must exist, the
	// stored resource must not have been released by a reset_all_* call, and a
	// failure must name what was missing. std::map::operator[] provided none of
	// them - it silently inserted a null and returned it, and the catch blocks
	// that were meant to report the failure could never fire because
	// operator[] does not throw.
	template <typename Map>
	auto find_or_throw(const Map& map, const typename Map::key_type& key,
		const char* kind, const std::string& name) -> decltype(map.begin()->second.get())
	{
		const auto it = map.find(key);
		if (it == map.end())
		{
			throw std::out_of_range(
				std::string(kind) + " '" + name + "' was never loaded.");
		}
		auto* resource = it->second.get();
		if (resource == nullptr)
		{
			throw std::out_of_range(
				std::string(kind) + " '" + name + "' has been released.");
		}
		return resource;
	}
}

ID3D11ShaderResourceView* ResourceManager::get_texture(
	const std::string& texture_name) const
{
	const auto it = this->_textures.find(texture_name);
	if (it == this->_textures.end())
	{
		throw std::out_of_range("Texture '" + texture_name + "' was never loaded.");
	}
	ID3D11ShaderResourceView* texture = it->second.Get();
	if (texture == nullptr)
	{
		throw std::out_of_range("Texture '" + texture_name + "' has been released.");
	}
	return texture;
}

void ResourceManager::add_texture(const std::string& texture_name,
	ID3D11ShaderResourceView* texture)
{
	this->_textures[texture_name] = texture;
}

SpriteFont* ResourceManager::get_sprite_font(const std::string& font_name) const
{
	return find_or_throw(this->_sprite_fonts, font_name, "SpriteFont", font_name);
}

void ResourceManager::add_sprite_font(const std::string& font_name,
	std::unique_ptr<SpriteFont> font)
{
	this->_sprite_fonts[font_name] = std::move(font);
}

SpriteSheet* ResourceManager::get_sprite_sheet(
	const std::string& sprite_sheet_name) const
{
	return find_or_throw(this->_sprite_sheets, sprite_sheet_name,
		"SpriteSheet", sprite_sheet_name);
}

void ResourceManager::add_sprite_sheet(const std::string& sprite_sheet_name,
	std::unique_ptr<SpriteSheet> sprite_sheet)
{
	this->_sprite_sheets[sprite_sheet_name] = std::move(sprite_sheet);
}

const LevelLoadedInfo* ResourceManager::get_level_info(
	level_stage stage) const
{
	return find_or_throw(this->_level_infos, stage, "LevelInfo",
		std::to_string(static_cast<int>(stage)));
}

void ResourceManager::add_level_info(level_stage stage,
	std::unique_ptr<LevelLoadedInfo> level_info)
{
	this->_level_infos[stage] = std::move(level_info);
}

void ResourceManager::add_sound_bank(const std::string& sound_bank_name,
	std::unique_ptr<SoundBank> sound_bank)
{
	this->_sound_banks[sound_bank_name] = std::move(sound_bank);
}
SoundBank* ResourceManager::get_sound_bank(
	const std::string& sound_bank_name) const
{
	return find_or_throw(this->_sound_banks, sound_bank_name,
		"SoundBank", sound_bank_name);
}

void ResourceManager::reset_all_sprite_fonts()
{
	// Erase rather than null out: a key mapped to nullptr is indistinguishable
	// from a loaded resource at every call site that only checks for presence.
	this->_sprite_fonts.clear();
}

void ResourceManager::reset_all_textures()
{
	this->_textures.clear();
}
void ResourceManager::reset_all_sounds()
{
	for (auto& sound_bank : this->_sound_banks)
	{
		if (sound_bank.second != nullptr)
		{
			sound_bank.second->reset_all_instances();
		}
	}
	this->_sound_banks.clear();
}
