#include "pch.h"
#include "ResourceManager.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

ID3D11ShaderResourceView* ResourceManager::get_texture(
	const std::string& texture_name)
{
	try
	{
		return this->_textures[texture_name].Get();
	}
	catch (const std::out_of_range&)
	{
		std::string error = "Texture " + texture_name + " not found.";
		throw std::out_of_range(error);
	}
}

void ResourceManager::add_texture(const std::string& texture_name,
	ID3D11ShaderResourceView* texture)
{
	this->_textures[texture_name] = texture;
}

SpriteFont* ResourceManager::get_sprite_font(const std::string& font_name)
{
	try
	{
		return this->_sprite_fonts[font_name].get();
	}
	catch (const std::out_of_range&)
	{
		std::string error = "SpriteFont " + font_name + " not found.";
		throw std::out_of_range(error);
	}
}

void ResourceManager::add_sprite_font(const std::string& font_name,
	std::unique_ptr<SpriteFont> font)
{
	this->_sprite_fonts[font_name] = std::move(font);
}

SpriteSheet* ResourceManager::get_sprite_sheet(
	const std::string& sprite_sheet_name)
{
	try
	{
		return this->_sprite_sheets[sprite_sheet_name].get();
	}
	catch (const std::out_of_range&)
	{
		std::string error = "SpriteSheet " + sprite_sheet_name + " not found.";
		throw std::out_of_range(error);
	}
}

void ResourceManager::add_sprite_sheet(const std::string& sprite_sheet_name,
	std::unique_ptr<SpriteSheet> sprite_sheet)
{
	this->_sprite_sheets[sprite_sheet_name] = std::move(sprite_sheet);
}

const LevelLoadedInfo* ResourceManager::get_level_info(
	level_stage stage) const
{
	try
	{
		return this->_level_infos.at(stage).get();
	}
	catch (const std::out_of_range&)
	{
		std::string error = "LevelInfo not found.";
		throw std::out_of_range(error);
	}
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
SoundBank* ResourceManager::get_sound_bank(const std::string& sound_bank_name)
{
	try
	{
		return this->_sound_banks[sound_bank_name].get();
	}
	catch (const std::out_of_range&)
	{
		std::string error = "SoundBank " + sound_bank_name + " not found.";
		throw std::out_of_range(error);
	}
}

void ResourceManager::reset_all_sprite_fonts()
{
	for (auto& font : this->_sprite_fonts)
	{
		font.second.reset();
	}
}

void ResourceManager::reset_all_textures()
{
	for (auto& texture : this->_textures)
	{
		texture.second.Reset();
	}
}
void ResourceManager::reset_all_sounds()
{
	for (auto& sound_bank : this->_sound_banks)
	{
		sound_bank.second->reset_all_instances();
		sound_bank.second.reset();
	}
}