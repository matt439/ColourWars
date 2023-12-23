#include "pch.h"
#include "ResourceLoader.h"

using namespace directory_consts;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

void ResourceLoader::load_textures()
{
    this->load_sprite_sheet_from_directory(TEXTURE_DIRECTORY, "sprite_sheet_1");
}
void ResourceLoader::load_fonts()
{
    //this->load_sprite_font_from_directory(FONT_DIRECTORY, "calibri_bold_32");
    //this->load_sprite_font_from_directory(FONT_DIRECTORY, "pixel_fj_verdana_12");

    //this->load_sprite_font_from_directory(FONT_DIRECTORY, "gill_sans_ultra_bold_36");
    //this->load_sprite_font_from_directory(FONT_DIRECTORY, "gill_sans_ultra_bold_48");
    //this->load_sprite_font_from_directory(FONT_DIRECTORY, "gill_sans_ultra_bold_72");
    //this->load_sprite_font_from_directory(FONT_DIRECTORY, "gill_sans_ultra_bold_96");
    this->load_sprite_font_from_directory(FONT_DIRECTORY, "gill_sans_ultra_bold_144");

    this->load_sprite_font_from_directory(FONT_DIRECTORY, "gill_sans_mt_bold_24");
    this->load_sprite_font_from_directory(FONT_DIRECTORY, "gill_sans_mt_bold_36");
    this->load_sprite_font_from_directory(FONT_DIRECTORY, "gill_sans_mt_bold_48");
    this->load_sprite_font_from_directory(FONT_DIRECTORY, "gill_sans_mt_bold_72");
    this->load_sprite_font_from_directory(FONT_DIRECTORY, "gill_sans_mt_bold_144");

    //this->load_sprite_font_from_directory(FONT_DIRECTORY, "courier_new_bold_24");
    this->load_sprite_font_from_directory(FONT_DIRECTORY, "courier_new_bold_16");
}
void ResourceLoader::load_sounds()
{
    //this->load_wave_bank_from_directory(SOUNDS_DIRECTORY, "wave_bank_1");
    this->load_sound_bank_from_directory(SOUNDS_DIRECTORY, "sound_bank_1");
}

//void ResourceLoader::load_main_menu_textures()
//{
//    std::string dir = TEXTURE_DIRECTORY;
//
//    this->load_sprite_sheet_from_directory(dir, "sprite_sheet_1");
//
//}
//
//void ResourceLoader::load_level_textures()
//{
//    std::string dir = TEXTURE_DIRECTORY;
//
//}
//
//void ResourceLoader::load_pause_menu_textures()
//{
//    std::string dir = TEXTURE_DIRECTORY;
//
//}
//
//void ResourceLoader::load_results_menu_textures()
//{
//    std::string dir = TEXTURE_DIRECTORY;
//
//}
//
//void ResourceLoader::load_main_menu_fonts()
//{
//    std::string dir = FONT_DIRECTORY;
//    //ResourceManager* rm = this->_resource_manager;
//
//    this->load_sprite_font_from_directory(dir, "calibri_bold_32");
//    
//}
//
//void ResourceLoader::load_level_fonts()
//{
//    std::string dir = FONT_DIRECTORY;
//    //ResourceManager* rm = this->_resource_manager;
//
//    this->load_sprite_font_from_directory(dir, "pixel_fj_verdana_12");
//}
void ResourceLoader::load_level_info()
{
    //this->load_level_info_from_directory(LEVEL_DIRECTORY, "test_1", level_stage::TEST_1);
    this->load_level_info_from_directory(LEVEL_DIRECTORY, "king_of_the_hill", level_stage::KING_OF_THE_HILL);
    this->load_level_info_from_directory(LEVEL_DIRECTORY, "turbulence", level_stage::TURBULENCE);
    this->load_level_info_from_directory(LEVEL_DIRECTORY, "close_quarters", level_stage::CLOSE_QUARTERS);
    //this->load_level_info_from_directory(LEVEL_DIRECTORY, "structure_test", level_stage::STRUCTURE_TEST);
}

//void ResourceLoader::load_main_menu_resources()
//{
//    this->load_main_menu_textures();
//    this->load_main_menu_fonts();
//}
//void ResourceLoader::load_level_resources()
//{
//    this->load_level_textures();
//    this->load_level_fonts();
//}
//void ResourceLoader::load_pause_menu_resources()
//{
//    this->load_pause_menu_textures();
//}
//void ResourceLoader::load_results_menu_resources()
//{
//    this->load_results_menu_textures();
//}

void ResourceLoader::load_all_resources()
{
 //   this->load_main_menu_resources();
	//this->load_level_resources();
	//this->load_pause_menu_resources();
	//this->load_results_menu_resources();

    this->load_fonts();
    this->load_textures();
    this->load_level_info();
    this->load_sounds();
}


void ResourceLoader::load_texture(const std::string& texture_path,
    const std::string& texture_name)
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

    this->_resource_manager->add_texture(texture_name, m_texture.Get());
}

void ResourceLoader::load_texture_from_directory(
    const std::string& directory,
    const std::string& texture_name)
{
    std::string path = directory + texture_name + ".dds";
    this->load_texture(path, texture_name);
}

void ResourceLoader::load_sprite_font(const std::string& font_path,
    const std::string& font_name)
{
    try
    {
        //this->_sprite_fonts[font_name] = std::make_unique<SpriteFont>(
        //    this->_device,
        //    std::wstring(font_path.begin(),
        //        font_path.end()).c_str());

        this->_resource_manager->add_sprite_font(font_name,
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
    const std::string& sprite_sheet_name)
{
    this->load_texture(texture_path, texture_name);
    //this->_sprite_sheets[sprite_sheet_name] = std::make_unique<SpriteSheet>(
    //    this->_textures[texture_name].Get());


    std::unique_ptr<SpriteSheet> ss = std::make_unique<SpriteSheet>(
        this->_resource_manager->get_texture(texture_name));

    ss->load_from_json(json_path.c_str());

    this->_resource_manager->add_sprite_sheet(sprite_sheet_name, std::move(ss));

   // this->_resource_manager->add_sprite_sheet(sprite_sheet_name,
   //     std::make_unique<SpriteSheet>(
			//this->_resource_manager->get_texture(texture_name)));

    //this->_sprite_sheets[sprite_sheet_name]->load_from_json(
    //    json_path.c_str());
}

void ResourceLoader::load_sprite_sheet_from_directory(
    const std::string& directory,
    const std::string& name)
{
    std::string tex_path = directory + name + ".dds";
    std::string json_path = directory + name + ".json";
    this->load_sprite_sheet(tex_path, name, json_path, name);
}

void ResourceLoader::load_level_info(const std::string& json_path,
    level_stage stage)
{
    //this->_level_infos[stage] = std::make_unique<LevelLoadedInfo>();
    //this->_level_infos[stage]->load_from_json(json_path.c_str());

    std::unique_ptr<LevelLoadedInfo> lli = std::make_unique<LevelLoadedInfo>(json_path.c_str());
    //lli->load_from_json(json_path.c_str());
    this->_resource_manager->add_level_info(stage, std::move(lli));
}

void ResourceLoader::load_level_info_from_directory(
    const std::string& directory,
    const std::string& level_name,
    level_stage stage)
{
    std::string path = directory + level_name + ".json";
    this->load_level_info(path, stage);
}

//void ResourceLoader::load_wave_bank(const std::string& wave_bank_path,
//    const std::string& wave_bank_name)
//{
//	this->_resource_manager->add_wave_bank(wave_bank_name,
//        std::move(std::make_unique<WaveBank>(
//			this->_audio_engine,
//			std::wstring(wave_bank_path.begin(),
//                				wave_bank_path.end()).c_str())));
//}
//void ResourceLoader::load_wave_bank_from_directory(
//    const std::string& directory,
//    const std::string& wave_bank_name)
//{
//    std::string path = directory + wave_bank_name + ".xwb";
//	this->load_wave_bank(path, wave_bank_name);
//}

void ResourceLoader::load_sound_bank(const std::string& wave_bank_path,
    const std::string& json_path,
    const std::string& sound_bank_name)
{
	std::unique_ptr<WaveBank> wb = std::make_unique<WaveBank>(
        this->_audio_engine,
        std::wstring(wave_bank_path.begin(),
        wave_bank_path.end()).c_str());
   
    std::unique_ptr<SoundBank> sb = std::make_unique<SoundBank>(
        std::move(wb));

    sb->load_from_json(json_path.c_str());

    this->_resource_manager->add_sound_bank(sound_bank_name, std::move(sb));

}
void ResourceLoader::load_sound_bank_from_directory(const std::string& directory,
    const std::string& wave_bank_name)
{
	std::string path = directory + wave_bank_name + ".xwb";
    std::string json_path = directory + wave_bank_name + ".json";
	this->load_sound_bank(path, json_path, wave_bank_name);
}

//const LevelLoadedInfo* ResourceManager::get_level_info(
//    level_stage stage) const
//{
//    try
//    {
//        return this->_level_infos.at(stage).get();
//    }
//    catch (const std::out_of_range&)
//    {
//        std::string error = "LevelInfo not found.";
//        throw std::out_of_range(error);
//    }
//}