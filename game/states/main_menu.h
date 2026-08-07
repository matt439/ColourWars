#pragma once

#include "engine/ui/widget.h"
#include "game/objects/player_team.h"
#include "game/states/main_menu_data.h"
#include "game/states/confirmation_state.h"
#include "game/states/menu_highlight.h"
#include "game/states/menu_element.h"
#include "game/states/menu_page.h"
#include "engine/core/state_context.h"
#include "engine/audio/sound_bank_object.h"

namespace main_menu_consts
{
	const static mattmath::Vector2F DEFAULT_RESOLUTION = { 1920.0f, 1080.0f };
	
	const static mattmath::Colour TITLE_TEXT_COLOUR = colour_consts::GOLDENROD;
	const static mattmath::Colour TITLE_START_TEXT_COLOUR = colour_consts::WHITE_SMOKE;
	const static mattmath::Colour TITLE_BACKGROUND_COLOUR = colour_consts::DARK_SLATE_BLUE;

	const static std::string TITLE_FONT = "gill_sans_ultra_bold_144";
	const static std::string ITEM_FONT = "gill_sans_mt_bold_48";
	const static std::string DETAIL_FONT = "gill_sans_mt_bold_36";
	const static std::string HEADING_FONT = "gill_sans_mt_bold_72";
	const static std::string ANNOUNCEMENT_FONT = "gill_sans_mt_bold_144";
	const static std::string WEAPON_DESCRIPTION_FONT = "gill_sans_mt_bold_24";

	const static mattmath::Colour HEADING_COLOUR = colour_consts::TEAM_ORANGE;

	const static mattmath::Colour STANDARD_HIGHLIGHT = colour_consts::WHITE;
	const static mattmath::Colour STANDARD_UNHIGHLIGHT = colour_consts::BLUE;

	const static mattmath::Colour OPTIONS_VALUE_COLOUR = colour_consts::DARK_TURQUOISE;

	const static mattmath::Colour WEAPON_DESCRIPTION_FONT_COLOUR = colour_consts::WHITE_SMOKE;

	const static mattmath::Colour SHADOW_COLOUR = colour_consts::BLACK;
	const static mattmath::Vector2F TITLE_SHADOW_OFFSET = { 6.0f, 6.0f };
	const static mattmath::Vector2F HEADING_SHADOW_OFFSET = { 2.5f, 2.5f };
	const static mattmath::Vector2F ITEM_SHADOW_OFFSET = { 1.5f, 1.5f };
	const static mattmath::Vector2F DETAIL_SHADOW_OFFSET = { 1.0f, 1.0f };
	const static mattmath::Vector2F WEAPON_DESCRIPTION_SHADOW_OFFSET = { 1.0f, 1.0f };

	const static mattmath::Vector2F MODE_SELECT_WIDGET_SPACING = { 250.0f, 100.0f };

	const static mattmath::Vector2F PLAYER_COUNT_WIDGET_SPACING = { 250.0f, 100.0f };

	const static mattmath::Vector2F TEAM_SELECT_WIDGET_SPACING = { 250.0f, 125.0f };
	const static mattmath::Vector2F TEAM_SELECT_TEAM_WIDGET_SIZE = { 50.0f, 50.0f };

	const static mattmath::Vector2F WEAPON_SELECT_WIDGET_SPACING = { 250.0f, 125.0f };
	const static mattmath::Vector2F WEAPON_SELECT_WEAPON_WIDGET_SIZE = { 50.0f, 50.0f };

	const static mattmath::Vector2F STAGE_SELECT_WIDGET_SPACING = { 0.0f, 150.0f };

	const static mattmath::Colour HOME_BACKGROUND_COLOUR = colour_consts::DODGER_BLUE;
	const static mattmath::Colour OPTIONS_BACKGROUND_COLOUR = colour_consts::DODGER_BLUE;
	const static mattmath::Colour PLAY_BACKGROUND_COLOUR = colour_consts::DODGER_BLUE;

	const static mattmath::Colour TEAM_SELECT_SELECTED_COLOUR = colour_consts::LIGHT_CORAL;
	const static mattmath::Colour TEAM_SELECT_UNSELECTED_COLOUR = colour_consts::WHITE;

	const static mattmath::Colour WEAPON_SELECT_SELECTED_COLOUR = colour_consts::LIGHT_CORAL;
	const static mattmath::Colour WEAPON_SELECT_UNSELECTED_COLOUR = colour_consts::WHITE;

	const static mattmath::Colour STAGE_SELECT_SELECTED_COLOUR = colour_consts::FIRE_BRICK;
	const static mattmath::Colour STAGE_SELECT_UNSELECTED_COLOUR = colour_consts::WHITE;
	const static mattmath::Vector2F STAGE_SELECT_ICON_SIZE = { 800.0f, 600.0f };
	const static mattmath::Vector2F STAGE_SELECT_READY_SIZE = { 1500.0f, 300.0f };

	constexpr float WEAPON_DESC_X_OFFSET = 250.0f;

	const std::string SOUND_BANK = "sound_bank_1";
	const std::string DIRECTION_SOUND = "UI_Clicks14";
	const std::string CONFIRM_SOUND = "UI_Clicks01";
	const std::string CANCEL_SOUND = "UI_Clicks17";
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

enum class MainMenuScreen
{
	title,
	home,
	options,
	mode_select,
	player_count,
	team_select,
	weapon_select,
	stage_select
};

struct MainMenuMidwayLoadSettings
{
	MainMenuScreen screen = MainMenuScreen::title;
	MenuLevelSettings settings = MenuLevelSettings();
};

class MainMenuPage : public MenuPage, public SoundBankObject
{
public:
	explicit MainMenuPage(MainMenuData* data);
	~MainMenuPage() override = default;
	void init() override = 0;
	void update() override = 0;
	void draw() override = 0;
protected:
	MainMenuData* get_main_menu_data() const;
	int get_player_count() const;

	// Every sound this page family can make, resolved once when the page is
	// built. The music is an effect rather than a wave because it loops and is
	// stopped on the way into a match; the rest are fire-and-forget.
	SoundBank::WaveHandle direction_sound_;
	SoundBank::WaveHandle confirm_sound_;
	SoundBank::WaveHandle cancel_sound_;
	SoundBank::WaveHandle error_sound_;
	SoundBank::WaveHandle ready_sound_;
	SoundBank::EffectHandle music_;
private:
	MainMenuData* data_ = nullptr;
};

class MainMenuTitle final : public MainMenuPage
{
public:
	explicit MainMenuTitle(MainMenuData* data);
	void update() override;
	void draw() override;
	void init() override;
private:
	std::unique_ptr<MContainer> texture_container_ = nullptr;
	std::unique_ptr<MContainer> text_container_ = nullptr;
	std::unique_ptr<MTexture> background_ = nullptr;
	std::unique_ptr<MTextDropShadow> title_ = nullptr;
	std::unique_ptr<MTextDropShadow> start_ = nullptr;
};

class MainMenuHome final : public MainMenuPage, public MenuHighlight
{
public:
	explicit MainMenuHome(MainMenuData* data);
	void update() override;
	void draw() override;
	void init() override;
private:
	std::unique_ptr<MContainer> texture_container_ = nullptr;
	std::unique_ptr<MContainer> text_container_ = nullptr;
	std::unique_ptr<MTexture> background_ = nullptr;
	std::unique_ptr<MTextDropShadow> heading_ = nullptr;
	std::unique_ptr<MTextDropShadow> play_ = nullptr;
	std::unique_ptr<MTextDropShadow> options_ = nullptr;
	std::unique_ptr<MTextDropShadow> exit_ = nullptr;
};

class MainMenuOptions final : public MainMenuPage, public MenuHighlight
{
public:
	explicit MainMenuOptions(MainMenuData* data);
	void update() override;
	void draw() override;
	void init() override;
private:
	std::unique_ptr<MContainer> texture_container_ = nullptr;
	std::unique_ptr<MContainer> text_container_ = nullptr;
	std::unique_ptr<MTexture> background_ = nullptr;
	std::unique_ptr<MTextDropShadow> heading_ = nullptr;
	std::unique_ptr<MTextDropShadow> resolution_element_ = nullptr;
	std::unique_ptr<MTextDropShadow> resolution_value_ = nullptr;
	std::unique_ptr<MTextDropShadow> full_screen_ = nullptr;
	std::unique_ptr<MTextDropShadow> full_screen_value_ = nullptr;
	std::unique_ptr<MTextDropShadow> apply_ = nullptr;
	std::unique_ptr<MTextDropShadow> back_ = nullptr;
	ScreenResolution resolution_selection_ = ScreenResolution::s_1920_1080;
	bool full_screen_selection_ = true;
	void cycle_resolution(MenuDirection direction);
	void update_resolution_selection_text();
	void update_full_screen_selection_text() const;
	void apply_fullscreen_setting(bool fullscreen);
};

class MainMenuModeSelect final : public MainMenuPage, public MenuHighlight
{
public:
	explicit MainMenuModeSelect(MainMenuData* data);
	void update() override;
	void draw() override;
	void init() override;
private:
	std::unique_ptr<MContainer> texture_container_ = nullptr;
	std::unique_ptr<MContainer> text_container_ = nullptr;
	std::unique_ptr<MTexture> background_ = nullptr;
	std::unique_ptr<MTextDropShadow> heading_ = nullptr;
	std::unique_ptr<MTextDropShadow> standard_ = nullptr;
	std::unique_ptr<MTextDropShadow> tdm_ = nullptr;
	std::unique_ptr<MTextDropShadow> dm_ = nullptr;
	std::unique_ptr<MTextDropShadow> practice_ = nullptr;
	std::unique_ptr<MTextDropShadow> back_ = nullptr;
	static MenuElement convert_mode_to_element(LevelMode mode);
};

class MainMenuPlayerCount final : public MainMenuPage, public MenuHighlight
{
public:
	explicit MainMenuPlayerCount(MainMenuData* data);
	void update() override;
	void draw() override;
	void init() override;
private:
	std::unique_ptr<MContainer> texture_container_ = nullptr;
	std::unique_ptr<MContainer> text_container_ = nullptr;
	std::unique_ptr<MTexture> background_ = nullptr;
	std::unique_ptr<MTextDropShadow> heading_ = nullptr;
	std::unique_ptr<MTextDropShadow> _1_player = nullptr;
	std::unique_ptr<MTextDropShadow> _2_players = nullptr;
	std::unique_ptr<MTextDropShadow> _3_players = nullptr;
	std::unique_ptr<MTextDropShadow> _4_players = nullptr;
	std::unique_ptr<MTextDropShadow> back_ = nullptr;
	static MenuElement convert_player_count_to_element(int player_count);
};

class MainMenuTeamSelect final : public MainMenuPage
{
public:
	explicit MainMenuTeamSelect(MainMenuData* data);
	MainMenuTeamSelect(MainMenuData* data, MenuLevelSettings* settings);
	void update() override;
	void draw() override;
	void init() override;
private:
	struct TeamSelectState
	{
		PlayerTeam team = PlayerTeam::none;
		ConfirmationState state = ConfirmationState::unconfirmed;
	};
	struct PlayerWidgets
	{
		std::unique_ptr<MTextDropShadow> player = nullptr;
		std::unique_ptr<MTexture> player_a = nullptr;
		std::unique_ptr<MTexture> player_center = nullptr;
		std::unique_ptr<MTexture> player_b = nullptr;
	};
	std::unique_ptr<MContainer> texture_container_ = nullptr;
	std::unique_ptr<MContainer> text_container_ = nullptr;
	std::unique_ptr<MTexture> background_ = nullptr;
	std::unique_ptr<MTextDropShadow> heading_ = nullptr;

	std::vector<std::unique_ptr<PlayerWidgets>> player_widgets_;
	std::vector<TeamSelectState> select_states_;
	void update_team_select_visuals();
	void deselect_and_unconfirm_all_widgets();
	bool all_players_confirmed() const;
	bool all_players_unconfirmed() const;
	void set_level_settings() const;
};

class MainMenuWeaponSelect final : public MainMenuPage
{
public:
	explicit MainMenuWeaponSelect(MainMenuData* data);
	MainMenuWeaponSelect(MainMenuData* data, MenuLevelSettings* settings);
	void update() override;
	void draw() override;
	void init() override;
private:
	struct SelectState
	{
		WeaponType type = WeaponType::sprayer;
		ConfirmationState state = ConfirmationState::unconfirmed;
	};
	struct Widgets
	{
		std::unique_ptr<MTextDropShadow> player = nullptr;
		std::unique_ptr<MTexture> weapon_icon = nullptr;
		std::unique_ptr<MTextDropShadow> weapon_name = nullptr;
		std::unique_ptr<MTextDropShadow> weapon_description = nullptr;
	};
	std::unique_ptr<MContainer> texture_container_ = nullptr;
	std::unique_ptr<MContainer> text_container_ = nullptr;
	std::unique_ptr<MTexture> background_ = nullptr;
	std::unique_ptr<MTextDropShadow> heading_ = nullptr;
	std::vector<std::unique_ptr<Widgets>> player_widgets_;
	std::vector<SelectState> select_states_;
	void update_weapon_select_visuals();
	void unconfirm_all_widgets();
	bool all_players_confirmed() const;
	bool all_players_unconfirmed() const;
	static WeaponType get_random_weapon();
	void set_level_settings() const;
	void cycle_weapons(MenuDirection direction, int player_index);
	static std::string weapon_description(WeaponType type);
};

class MainMenuStageSelect final : public MainMenuPage
{
public:
	explicit MainMenuStageSelect(MainMenuData* data);
	MainMenuStageSelect(MainMenuData* data, MenuLevelSettings* settings);
	void update() override;
	void draw() override;
	void init() override;
private:
	struct SelectState
	{
		LevelStage stage = LevelStage::king_of_the_hill;
		ConfirmationState state = ConfirmationState::unconfirmed;
	};
	std::unique_ptr<MContainer> texture_container_ = nullptr;
	std::unique_ptr<MContainer> text_container_ = nullptr;
	std::unique_ptr<MTexture> background_ = nullptr;
	std::unique_ptr<MTextDropShadow> heading_ = nullptr;
	std::unique_ptr<MTexture> stage_icon_ = nullptr;
	std::unique_ptr<MTextDropShadow> stage_name_ = nullptr;
	std::unique_ptr<MTextDropShadow> ready_ = nullptr;
	SelectState select_state_ = SelectState();
	void update_stage_select_visuals();
	void unconfirm_all_widgets();
	static LevelStage get_random_stage();
	void cycle_stages(MenuDirection direction);
	void set_level_settings() const;
};
