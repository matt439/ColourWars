#include "pch.h"
#include "ResourceLoader.h"

using namespace directory_consts;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

ResourceLoader::ResourceLoader(RenderResources* render_resources,
    AudioResources* audio_resources,
    Registry<LevelLoadedInfo>* level_infos, ID3D11Device1* device,
    DirectX::AudioEngine* audio_engine) :
    _render_resources(render_resources),
    _audio_resources(audio_resources),
    _level_infos(level_infos),
    _device(device),
    _audio_engine(audio_engine)
{

}

void ResourceLoader::set_device(ID3D11Device1* device)
{
    this->_device = device;
}

void ResourceLoader::load_textures()
{
    this->load_sprite_sheet_from_directory(TEXTURE_DIRECTORY, "sprite_sheet_1");
    this->_loaded_sprite_sheets.push_back({ TEXTURE_DIRECTORY, "sprite_sheet_1" });
}
void ResourceLoader::load_fonts()
{
    const std::string font_names[] =
    {
        "gill_sans_ultra_bold_144",
        "gill_sans_mt_bold_24",
        "gill_sans_mt_bold_36",
        "gill_sans_mt_bold_48",
        "gill_sans_mt_bold_72",
        "gill_sans_mt_bold_144",
        "courier_new_bold_16",
    };

    for (const std::string& font_name : font_names)
    {
        this->load_sprite_font_from_directory(FONT_DIRECTORY, font_name);
        this->_loaded_fonts.push_back({ FONT_DIRECTORY, font_name });
    }
}

void ResourceLoader::reload_device_resources()
{
    // Fonts hold GPU textures, so they have to be rebuilt outright. Nothing
    // caches a SpriteFont* - every text object looks its font up by name.
    std::vector<asset_record> fonts = this->_loaded_fonts;
    this->_loaded_fonts.clear();
    for (const asset_record& font : fonts)
    {
        this->load_sprite_font_from_directory(font.directory, font.name);
        this->_loaded_fonts.push_back(font);
    }

    for (const asset_record& sheet : this->_loaded_sprite_sheets)
    {
        this->reload_sprite_sheet_texture(sheet.directory, sheet.name);
    }
}

void ResourceLoader::reload_sprite_sheet_texture(const std::string& directory,
    const std::string& name) const
{
    const std::string texture_path = directory + name + ".dds";
    this->load_texture(texture_path, name);

    this->_render_resources->get_sprite_sheet(name)->set_texture(
        this->_render_resources->get_texture(name));
}
void ResourceLoader::load_sounds()
{
    this->load_sound_bank_from_directory(SOUNDS_DIRECTORY, "sound_bank_1");
}

// The stages that have a level to load. level_asset_name owns the enum-to-name
// table, so the names live in one place instead of being spelt again here.
void ResourceLoader::load_levels() const
{
    const level_stage stages[] =
    {
        level_stage::KING_OF_THE_HILL,
        level_stage::TURBULENCE,
        level_stage::CLOSE_QUARTERS,
    };

    for (const level_stage stage : stages)
    {
        this->load_level_info_from_directory(LEVEL_DIRECTORY,
            level_asset_name(stage));
    }
}

void ResourceLoader::load_all_resources()
{
    this->load_fonts();
    this->load_textures();
    this->load_levels();
    this->load_sounds();
}


void ResourceLoader::load_texture(const std::string& texture_path,
    const std::string& texture_name) const
{
    ComPtr<ID3D11ShaderResourceView> m_texture;

    ComPtr<ID3D11Resource> resource;
    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(this->_device,
            std::wstring(texture_path.begin(), texture_path.end()).c_str(),
            resource.GetAddressOf(),
            m_texture.ReleaseAndGetAddressOf()));

    ComPtr<ID3D11Texture2D> texture;
    DX::ThrowIfFailed(resource.As(&texture));

    CD3D11_TEXTURE2D_DESC texture_desc;
    texture->GetDesc(&texture_desc);

    this->_render_resources->add_texture(texture_name, m_texture.Get());
}

void ResourceLoader::load_texture_from_directory(
    const std::string& directory,
    const std::string& texture_name) const
{
    std::string path = directory + texture_name + ".dds";
    this->load_texture(path, texture_name);
}

void ResourceLoader::load_sprite_font(const std::string& font_path,
    const std::string& font_name)
{
    try
    {
        this->_render_resources->add_sprite_font(font_name,
            std::make_unique<SpriteFont>(
                this->_device,
                std::wstring(font_path.begin(),
                    font_path.end()).c_str()));
    }
    catch (const std::exception&)
    {
        std::string error = "SpriteFont " + font_name +
            " not found at " + font_path + ".";
        throw std::out_of_range(error);
    }

}

void ResourceLoader::load_sprite_font_from_directory(
    const std::string& directory,
    const std::string& font_name)
{
    std::string path = directory + font_name + ".spritefont";
    this->load_sprite_font(path, font_name);
}

void ResourceLoader::load_sprite_sheet(
    const std::string& texture_path,
    const std::string& texture_name,
    const std::string& json_path,
    const std::string& sprite_sheet_name) const
{
    this->load_texture(texture_path, texture_name);

    this->_render_resources->add_sprite_sheet(sprite_sheet_name,
        sprite_sheet_loader::load(json_path.c_str(),
            this->_render_resources->get_texture(texture_name)));
}

void ResourceLoader::load_sprite_sheet_from_directory(
    const std::string& directory,
    const std::string& name) const
{
    std::string tex_path = directory + name + ".dds";
    std::string json_path = directory + name + ".json";
    this->load_sprite_sheet(tex_path, name, json_path, name);
}

void ResourceLoader::load_level_info(const std::string& json_path,
    const std::string& level_name) const
{
	auto lli = std::make_unique<LevelLoadedInfo>(json_path.c_str());
    this->_level_infos->add(level_name, std::move(lli));
}

void ResourceLoader::load_level_info_from_directory(
    const std::string& directory,
    const std::string& level_name) const
{
    std::string path = directory + level_name + ".json";
    this->load_level_info(path, level_name);
}

void ResourceLoader::load_sound_bank(const std::string& wave_bank_path,
    const std::string& json_path,
    const std::string& sound_bank_name)
{
	std::unique_ptr<WaveBank> wb;
	try
	{
		wb = std::make_unique<WaveBank>(
			this->_audio_engine,
			std::wstring(wave_bank_path.begin(),
			wave_bank_path.end()).c_str());
	}
	catch (const std::exception&)
	{
		// DirectXTK's what() is just "WaveBank" — T6 wants the path.
		throw std::runtime_error("Failed to load wave bank: " + wave_bank_path);
	}

    this->_audio_resources->add_sound_bank(sound_bank_name,
        sound_bank_loader::load(json_path.c_str(), std::move(wb)));

}
void ResourceLoader::load_sound_bank_from_directory(const std::string& directory,
    const std::string& wave_bank_name)
{
	std::string path = directory + wave_bank_name + ".xwb";
    std::string json_path = directory + wave_bank_name + ".json";
	this->load_sound_bank(path, json_path, wave_bank_name);
}
