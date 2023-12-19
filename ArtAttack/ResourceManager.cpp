#include "pch.h"
#include "ResourceManager.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;
//
//void ResourceManager::load_texture(const std::string& texture_path,
//	const std::string& texture_name)
//{
//	ComPtr<ID3D11ShaderResourceView> m_texture;
//
//	ComPtr<ID3D11Resource> resource;
//	DX::ThrowIfFailed(
//		CreateDDSTextureFromFile(this->_device,
//			std::wstring(texture_path.begin(), texture_path.end()).c_str(),
//				resource.GetAddressOf(),
//					m_texture.ReleaseAndGetAddressOf()));
//
//	ComPtr<ID3D11Texture2D> texture;
//	DX::ThrowIfFailed(resource.As(&texture));
//
//	CD3D11_TEXTURE2D_DESC texture_desc;
//	texture->GetDesc(&texture_desc);
//
//	this->_textures[texture_name] = m_texture;
//}
//
//void ResourceManager::load_texture_from_directory(
//	const std::string& directory,
//	const std::string& texture_name)
//{
//	std::string path = directory + texture_name + ".dds";
//	this->load_texture(path, texture_name);
//}

//ResourceManager::ResourceManager(AudioEngine* audio_engine)
//	: _audio_engine(audio_engine)
//{
//	this->_wave_bank = std::make_unique<WaveBank>(this->_audio_engine,
//		L"./sounds/wave_bank_1.xwb");
//	
//	this->_sound_effect_instance = this->_wave_bank->
//		CreateInstance("UI_Clicks01");
//
//
//}

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

//void ResourceManager::load_sprite_font(const std::string& font_path,
//	const std::string& font_name)
//{
//	try
//	{
//		this->_sprite_fonts[font_name] = std::make_unique<SpriteFont>(
//			this->_device,
//			std::wstring(font_path.begin(),
//				font_path.end()).c_str());
//	}
//	catch (const std::exception&)
//	{
//		std::string error = "SpriteFont " + font_name +
//			" not found at " + font_path + ".";
//		throw std::out_of_range(error);
//	}
//
//}
//
//void ResourceManager::load_sprite_font_from_directory(
//	const std::string& directory,
//	const std::string& font_name)
//{
//	std::string path = directory + font_name + ".spritefont";
//	this->load_sprite_font(path, font_name);
//}

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

//void ResourceManager::load_sprite_sheet(
//	const std::string& texture_path,
//	const std::string& texture_name,
//	const std::string& json_path,
//	const std::string& sprite_sheet_name)
//{
//	this->load_texture(texture_path, texture_name);
//	this->_sprite_sheets[sprite_sheet_name] = std::make_unique<SpriteSheet>(
//		this->_textures[texture_name].Get());
//	this->_sprite_sheets[sprite_sheet_name]->load_from_json(
//		json_path.c_str());
//}
//
//void ResourceManager::load_sprite_sheet_from_directory(
//	const std::string& directory,
//	const std::string& name)
//{
//	std::string tex_path = directory + name + ".dds";
//	std::string json_path = directory + name + ".json";
//	this->load_sprite_sheet(tex_path, name, json_path, name);
//}

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

//void ResourceManager::load_level_info(const std::string& json_path,
//	level_stage stage)
//{
//	this->_level_infos[stage] = std::make_unique<LevelLoadedInfo>();
//	this->_level_infos[stage]->load_from_json(json_path.c_str());
//}
//
//void ResourceManager::load_level_info_from_directory(
//	const std::string& directory,
//	const std::string& level_name,
//	level_stage stage)
//{
//	std::string path = directory + level_name + ".json";
//	this->load_level_info(path, stage);
//}
//
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

//WaveBank* ResourceManager::get_wave_bank(const std::string& wave_bank_name)
//{
//	try
//	{
//		return this->_wave_banks[wave_bank_name].get();
//	}
//	catch (const std::out_of_range&)
//	{
//		std::string error = "WaveBank " + wave_bank_name + " not found.";
//		throw std::out_of_range(error);
//	}
//}
//void ResourceManager::add_wave_bank(const std::string& wave_bank_name,
//	std::unique_ptr<WaveBank> wave_bank)
//{
//	this->_wave_banks[wave_bank_name] = std::move(wave_bank);
//}
//std::unique_ptr<SoundEffectInstance> ResourceManager::create_sound_effect_instance(
//	const std::string& wave_bank_name,
//	const std::string& wave_name)
//{
//	return std::move(this->_wave_banks[wave_bank_name]->
//			CreateInstance(wave_name.c_str()));
//
//}

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

//void ResourceManager::play()
//{
//	this->_sound_effect_instance->Play();
//}

//void ResourceManager::load_sprite_frame(const SpriteFrame& sprite_frame,
//	const std::string sprite_frame_name)
//{
//	this->_sprite_frames[sprite_frame_name] =
//		std::make_unique<SpriteFrame>(sprite_frame);
//}
//SpriteFrame* ResourceManager::get_sprite_frame(
//	const std::string& sprite_frame_name)
//{
//	return this->_sprite_frames[sprite_frame_name].get();
//}
//ID3D11ShaderResourceView* ResourceManager::
//	get_sprite_frames_base_texture(
//	const std::string& sprite_frame_name)
//{
//	SpriteFrame* frame = this->_sprite_frames[sprite_frame_name].get();
//	return this->get_texture(frame->sprite_sheet_name);
//}
//
//DirectX::SimpleMath::Rectangle
//	ResourceManager::get_sprite_frame_source_rectangle(
//	const std::string& sprite_frame_name)
//{
//	SpriteFrame* frame = this->_sprite_frames[sprite_frame_name].get();
//	return frame->source_rectangle;
//}

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
	//for (auto& wave_bank : this->_wave_banks)
	//{
	//	wave_bank.second.reset();
	//}

	for (auto& sound_bank : this->_sound_banks)
	{
		sound_bank.second->reset_all_instances();
		sound_bank.second.reset();
	}
}