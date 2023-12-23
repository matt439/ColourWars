#ifndef MAINMENU_H
#define MAINMENU_H
#include "Mh.h"
#include "player_team.h"
#include "MainMenuData.h"
#include "confirmation_state.h"
#include "MenuHighlight.h"
#include "menu_element.h"
#include "MenuPage.h"
#include "StateContext.h"
//#include "WaveBankObject.h"
#include "SoundBankObject.h"

namespace main_menu_consts
{
	const static MattMath::Vector2F DEFAULT_RESOLUTION = { 1920.0f, 1080.0f };
	
	const static MattMath::Colour TITLE_TEXT_COLOUR = colour_consts::GOLDENROD;
	const static MattMath::Colour TITLE_START_TEXT_COLOUR = colour_consts::WHITE_SMOKE;
	const static MattMath::Colour TITLE_BACKGROUND_COLOUR = colour_consts::DARK_SLATE_BLUE;

	const static std::string TITLE_FONT = "gill_sans_ultra_bold_144";
	const static std::string ITEM_FONT = "gill_sans_mt_bold_48";
	const static std::string DETAIL_FONT = "gill_sans_mt_bold_36";
	const static std::string HEADING_FONT = "gill_sans_mt_bold_72";
	const static std::string ANNOUNCEMENT_FONT = "gill_sans_mt_bold_144";
	const static std::string WEAPON_DESCRIPTION_FONT = "gill_sans_mt_bold_24";

	const static MattMath::Colour HEADING_COLOUR = colour_consts::TEAM_ORANGE;

	const static MattMath::Colour STANDARD_HIGHLIGHT = colour_consts::WHITE;
	const static MattMath::Colour STANDARD_UNHIGHLIGHT = colour_consts::BLUE;

	const static MattMath::Colour OPTIONS_VALUE_COLOUR = colour_consts::DARK_TURQUOISE;

	const static MattMath::Colour WEAPON_DESCRIPTION_FONT_COLOUR = colour_consts::WHITE_SMOKE;

	const static MattMath::Colour SHADOW_COLOUR = colour_consts::BLACK;
	const static MattMath::Vector2F TITLE_SHADOW_OFFSET = { 6.0f, 6.0f };
	const static MattMath::Vector2F HEADING_SHADOW_OFFSET = { 2.5f, 2.5f };
	const static MattMath::Vector2F ITEM_SHADOW_OFFSET = { 1.5f, 1.5f };
	const static MattMath::Vector2F DETAIL_SHADOW_OFFSET = { 1.0f, 1.0f };
	const static MattMath::Vector2F WEAPON_DESCRIPTION_SHADOW_OFFSET = { 1.0f, 1.0f };

	//const static MattMath::Vector2F MODE_SELECT_WIDGET_POSITION = { 150.0f, 50.0f };
	const static MattMath::Vector2F MODE_SELECT_WIDGET_SPACING = { 250.0f, 100.0f };

	//const static MattMath::Vector2F PLAYER_COUNT_WIDGET_POSITION = { 150.0f, 50.0f };
	const static MattMath::Vector2F PLAYER_COUNT_WIDGET_SPACING = { 250.0f, 100.0f };

	const static MattMath::Vector2F TEAM_SELECT_WIDGET_SPACING = { 250.0f, 125.0f };
	const static MattMath::Vector2F TEAM_SELECT_TEAM_WIDGET_SIZE = { 50.0f, 50.0f };

	const static MattMath::Vector2F WEAPON_SELECT_WIDGET_SPACING = { 250.0f, 125.0f };
	const static MattMath::Vector2F WEAPON_SELECT_WEAPON_WIDGET_SIZE = { 50.0f, 50.0f };

	const static MattMath::Vector2F STAGE_SELECT_WIDGET_SPACING = { 0.0f, 150.0f };

	const static MattMath::Colour HOME_BACKGROUND_COLOUR = colour_consts::DODGER_BLUE;
	const static MattMath::Colour OPTIONS_BACKGROUND_COLOUR = colour_consts::DODGER_BLUE;
	const static MattMath::Colour PLAY_BACKGROUND_COLOUR = colour_consts::DODGER_BLUE;

	const static MattMath::Colour TEAM_SELECT_SELECTED_COLOUR = colour_consts::LIGHT_CORAL;
	const static MattMath::Colour TEAM_SELECT_UNSELECTED_COLOUR = colour_consts::WHITE;

	const static MattMath::Colour WEAPON_SELECT_SELECTED_COLOUR = colour_consts::LIGHT_CORAL;
	const static MattMath::Colour WEAPON_SELECT_UNSELECTED_COLOUR = colour_consts::WHITE;

	const static MattMath::Colour STAGE_SELECT_SELECTED_COLOUR = colour_consts::FIRE_BRICK;
	const static MattMath::Colour STAGE_SELECT_UNSELECTED_COLOUR = colour_consts::WHITE;
	const static MattMath::Vector2F STAGE_SELECT_ICON_SIZE = { 800.0f, 600.0f };
	const static MattMath::Vector2F STAGE_SELECT_READY_SIZE = { 1500.0f, 300.0f };

	constexpr float WEAPON_DESC_X_OFFSET = 250.0f;

	//const std::string WAVE_BANK = "wave_bank_1";
	const std::string SOUND_BANK = "sound_bank_1";
	const std::string DIRECTION_SOUND = "UI_Clicks14";
	const std::string CONFIRM_SOUND = "UI_Clicks01";
	const std::string CANCEL_SOUND = "UI_Clicks17";
	const std::string TITLE_SOUND = "UI_Clicks01";
	const std::string ERROR_SOUND = "UIerror2";
	const std::string READY_SOUND = "Whoosh2";
	const std::string MUSIC = "Top_Of_The_Morning";
	constexpr float MUSIC_VOLUME = 0.03f;

	const static std::string SPRAYER_DESC = "Fires a powerful stream of paint in an arc pattern.\nGood for beginners.";
	const static std::string SNIPER_DESC = "Slowly fires powerful, accurate shots.\nCan dispatch players quickly if you have skill.";
	const static std::string ROLLER_DESC = "Rolls paint in a wide area in front of you.\nCan be used to cover large floor areas quickly.";
	const static std::string MISTER_DESC = "Produces a mist of slow-moving, diffusing paint particles.";
	const static std::string BUCKET_DESC = "Throws a bucket of paint in a short arc pattern.\nIdeal for defeating other players.";
	const static std::string RANDOM_DESC = "Randomly selects a weapon for you.\nIndecisiveness is nothing to be ashamed of.";
}

enum class main_menu_screen
{
	TITLE,
	HOME,
	OPTIONS,
	MODE_SELECT,
	PLAYER_COUNT,
	TEAM_SELECT,
	WEAPON_SELECT,
	STAGE_SELECT
};

struct main_menu_midway_load_settings
{
	main_menu_screen screen = main_menu_screen::TITLE;
	MenuLevelSettings settings = MenuLevelSettings();
};

class MainMenuPage : public MenuPage, public SoundBankObject
{
private:
	MainMenuData* _data = nullptr;
protected:
	MainMenuData* get_main_menu_data();
	int get_player_count();
public:
	MainMenuPage(MainMenuData* data) :
		MenuPage(data),
		SoundBankObject(main_menu_consts::SOUND_BANK,
			this->get_resource_manager()),
		_data(data) {}
	virtual ~MainMenuPage() {}
	virtual void init() = 0;
	virtual void update() = 0;
	virtual void draw() = 0;
};

class MainMenuTitle : public MainMenuPage
{
private:
	std::unique_ptr<MContainer> _texture_container = nullptr;
	std::unique_ptr<MContainer> _text_container = nullptr;
	std::unique_ptr<MTexture> _background = nullptr;
	std::unique_ptr<MTextDropShadow> _title = nullptr;
	std::unique_ptr<MTextDropShadow> _start = nullptr;

	//std::unique_ptr<SoundLibrary> _title_sound = nullptr;
	//std::unique_ptr<SoundSingle> _title_sound_effect = nullptr;
	//std::unique_ptr<DirectX::SoundEffectInstance> _title_sound_effect_instance = nullptr;
	std::unique_ptr<DirectX::SoundEffectInstance> _music = nullptr;

public:
	MainMenuTitle(MainMenuData* data) : MainMenuPage(data) {}
	void update() override;
	void draw() override;
	void init() override;
};
class MainMenuHome : public MainMenuPage, public MenuHighlight
{
private:
	std::unique_ptr<MContainer> _texture_container = nullptr;
	std::unique_ptr<MContainer> _text_container = nullptr;
	std::unique_ptr<MTexture> _background = nullptr;
	std::unique_ptr<MTextDropShadow> _heading = nullptr;
	std::unique_ptr<MTextDropShadow> _play = nullptr;
	std::unique_ptr<MTextDropShadow> _options = nullptr;
	std::unique_ptr<MTextDropShadow> _exit = nullptr;
public:
	MainMenuHome(MainMenuData* data) : MainMenuPage(data) {}
	void update() override;
	void draw() override;
	void init() override;
};

class MainMenuOptions : public MainMenuPage, public MenuHighlight
{
private:
	std::unique_ptr<MContainer> _texture_container = nullptr;
	std::unique_ptr<MContainer> _text_container = nullptr;
	std::unique_ptr<MTexture> _background = nullptr;
	std::unique_ptr<MTextDropShadow> _heading = nullptr;
	std::unique_ptr<MTextDropShadow> _resolution_element = nullptr;
	std::unique_ptr<MTextDropShadow> _resolution_value = nullptr;
	std::unique_ptr<MTextDropShadow> _full_screen = nullptr;
	std::unique_ptr<MTextDropShadow> _full_screen_value = nullptr;
	std::unique_ptr<MTextDropShadow> _apply = nullptr;
	std::unique_ptr<MTextDropShadow> _back = nullptr;
	screen_resolution _resolution_selection = screen_resolution::S_1920_1080;
	bool _full_screen_selection = true;
	void cycle_resolution(menu_direction direction);
	void update_resolution_selection_text();
	void update_full_screen_selection_text();
	void apply_fullscreen_setting(bool fullscreen);
public:
	MainMenuOptions(MainMenuData* data) : MainMenuPage(data) {}
	void update() override;
	void draw() override;
	void init() override;
};

class MainMenuModeSelect : public MainMenuPage, public MenuHighlight
{
private:
	std::unique_ptr<MContainer> _texture_container = nullptr;
	std::unique_ptr<MContainer> _text_container = nullptr;
	std::unique_ptr<MTexture> _background = nullptr;
	std::unique_ptr<MTextDropShadow> _heading = nullptr;
	std::unique_ptr<MTextDropShadow> _standard = nullptr;
	std::unique_ptr<MTextDropShadow> _tdm = nullptr;
	std::unique_ptr<MTextDropShadow> _dm = nullptr;
	std::unique_ptr<MTextDropShadow> _practice = nullptr;
	std::unique_ptr<MTextDropShadow> _back = nullptr;
	menu_element convert_mode_to_element(level_mode mode);
public:
	MainMenuModeSelect(MainMenuData* data) : MainMenuPage(data) {}
	void update() override;
	void draw() override;
	void init() override;
};

class MainMenuPlayerCount : public MainMenuPage, public MenuHighlight
{
private:
	std::unique_ptr<MContainer> _texture_container = nullptr;
	std::unique_ptr<MContainer> _text_container = nullptr;
	std::unique_ptr<MTexture> _background = nullptr;
	std::unique_ptr<MTextDropShadow> _heading = nullptr;
	std::unique_ptr<MTextDropShadow> _1_player = nullptr;
	std::unique_ptr<MTextDropShadow> _2_players = nullptr;
	std::unique_ptr<MTextDropShadow> _3_players = nullptr;
	std::unique_ptr<MTextDropShadow> _4_players = nullptr;
	std::unique_ptr<MTextDropShadow> _back = nullptr;
	menu_element convert_player_count_to_element(int player_count);
public:
	MainMenuPlayerCount(MainMenuData* data) : MainMenuPage(data) {}
	void update() override;
	void draw() override;
	void init() override;
};

class MainMenuTeamSelect : public MainMenuPage
{
private:
	struct team_select_state
	{
		player_team team = player_team::NONE;
		confirmation_state state = confirmation_state::UNCONFIRMED;
	};
	struct player_widgets
	{
		std::unique_ptr<MTextDropShadow> _player = nullptr;
		std::unique_ptr<MTexture> _player_a = nullptr;
		std::unique_ptr<MTexture> _player_center = nullptr;
		std::unique_ptr<MTexture> _player_b = nullptr;
	};
	//int _num_players;
	std::unique_ptr<MContainer> _texture_container = nullptr;
	std::unique_ptr<MContainer> _text_container = nullptr;
	std::unique_ptr<MTexture> _background = nullptr;
	std::unique_ptr<MTextDropShadow> _heading = nullptr;

	std::vector<std::unique_ptr<
		MainMenuTeamSelect::player_widgets>> _player_widgets;
	std::vector<MainMenuTeamSelect::team_select_state> _select_states;
	void update_team_select_visuals();
	void deselect_and_unconfirm_all_widgets();
	bool all_players_confirmed();
	bool all_players_unconfirmed();
	void set_level_settings();
public:
	MainMenuTeamSelect(MainMenuData* data) : MainMenuPage(data) {}
	MainMenuTeamSelect(MainMenuData* data, MenuLevelSettings* settings);
	void update() override;
	void draw() override;
	void init() override;
};

class MainMenuWeaponSelect : public MainMenuPage
{
private:
	struct select_state
	{
		wep_type type = wep_type::SPRAYER;
		confirmation_state state = confirmation_state::UNCONFIRMED;
	};
	struct widgets
	{
		std::unique_ptr<MTextDropShadow> _player = nullptr;
		std::unique_ptr<MTexture> _weapon_icon = nullptr;
		std::unique_ptr<MTextDropShadow> _weapon_name = nullptr;
		std::unique_ptr<MTextDropShadow> _weapon_description = nullptr;
	};
	std::unique_ptr<MContainer> _texture_container = nullptr;
	std::unique_ptr<MContainer> _text_container = nullptr;
	std::unique_ptr<MTexture> _background = nullptr;
	std::unique_ptr<MTextDropShadow> _heading = nullptr;
	std::vector<std::unique_ptr<
		MainMenuWeaponSelect::widgets>> _player_widgets;
	std::vector<MainMenuWeaponSelect::select_state> _select_states;
	void update_weapon_select_visuals();
	void unconfirm_all_widgets();
	bool all_players_confirmed();
	bool all_players_unconfirmed();
	wep_type get_random_weapon();
	void set_level_settings();
	void cycle_weapons(menu_direction direction, int player_index);
	std::string weapon_description(wep_type type) const;
public:
	MainMenuWeaponSelect(MainMenuData* data) : MainMenuPage(data) {}
	MainMenuWeaponSelect(MainMenuData* data, MenuLevelSettings* settings);
	void update() override;
	void draw() override;
	void init() override;
};

class MainMenuStageSelect : public MainMenuPage
{
private:
	struct select_state
	{
		level_stage stage = level_stage::KING_OF_THE_HILL;
		confirmation_state state = confirmation_state::UNCONFIRMED;
	};
	std::unique_ptr<MContainer> _texture_container = nullptr;
	std::unique_ptr<MContainer> _text_container = nullptr;
	std::unique_ptr<MTexture> _background = nullptr;
	std::unique_ptr<MTextDropShadow> _heading = nullptr;
	std::unique_ptr<MTexture> _stage_icon = nullptr;
	std::unique_ptr<MTextDropShadow> _stage_name = nullptr;
	std::unique_ptr<MTextDropShadow> _ready = nullptr;
	MainMenuStageSelect::select_state _select_state =
		MainMenuStageSelect::select_state();
	void update_stage_select_visuals();
	void unconfirm_all_widgets();
	level_stage get_random_stage();
	void cycle_stages(menu_direction direction);
	void set_level_settings();
public:
	MainMenuStageSelect(MainMenuData* data) : MainMenuPage(data) {}
	MainMenuStageSelect(MainMenuData* data, MenuLevelSettings* settings);
	void update() override;
	void draw() override;
	void init() override;
};

#endif