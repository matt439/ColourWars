#ifndef RESULTSMENU_H
#define RESULTSMENU_H

#include "ResultsMenuData.h"
#include "Colour.h"
#include "MenuPage.h"
#include "SoundBankObject.h"

namespace results_menu_consts
{
	const static MattMath::Vector2F DEFAULT_RESOLUTION = { 1920.0f, 1080.0f };
	
	const static MattMath::Vector2F RESULTS_MENU_BOX_SIZE = { 900.0f, 600.0f };
	const static MattMath::Colour RESULTS_MENU_BOX_COLOUR = colour_consts::LIGHT_SLATE_GRAY;
	const static MattMath::Colour RESULTS_MENU_HEADING_TEXT_COLOUR = colour_consts::GOLDENROD;

	const static MattMath::Vector2F RESULTS_MENU_FILL_BOX_SIZE = { 700.0f, 200.0f };
	constexpr float RESULTS_MENU_FILL_BOX_Y_OFFSET = 300.0f;

	const static MattMath::Vector2F RESULTS_MENU_TEAM_FILL_BORDER = { 10.0f, 10.0f };
	const static MattMath::Vector2F RESULTS_MENU_TEAM_FILL_SIZE =
		results_menu_consts::RESULTS_MENU_FILL_BOX_SIZE -
		results_menu_consts::RESULTS_MENU_TEAM_FILL_BORDER * 2.0f;

	constexpr float RESULTS_MENU_TEAM_FILL_Y_OFFSET = 310.0f;

	constexpr float RESULTS_MENU_TEAM_FILL_DELAY = 1.0f;
	constexpr float RESULTS_MENU_TEAM_FILL_TIME = 2.0f;

	constexpr float RESULTS_MENU_SHOW_RESULTS_DELAY = 1.0f;

	constexpr float RESULTS_MENU_TITLE_Y_OFFSET = 50.0f;

	constexpr float WINNER_TEXT_Y_OFFSET = 10.0f;

	const static MattMath::Colour PERCENTAGE_TEXT_COLOUR = colour_consts::DARK_GRAY;

	constexpr float TEAM_B_PERCENTAGE_X_OFFSET = 325.0f;

	const static std::string ITEM_FONT = "gill_sans_mt_bold_48";
	const static std::string HEADING_FONT = "gill_sans_mt_bold_72";
	const static std::string DETAIL_FONT = "gill_sans_mt_bold_36";

	const static MattMath::Vector2F PROCEED_POSITION = { 725.0f, 750.0f };
	const static MattMath::Vector2F HEADING_POSITION = { 800.0f, 235.0f };
	const static MattMath::Vector2F WINNER_POSITION = { 750.0f, 400.0f };

	const static MattMath::Colour SHADOW_COLOUR = colour_consts::BLACK;
	const static MattMath::Vector2F TITLE_SHADOW_OFFSET = { 6.0f, 6.0f };
	const static MattMath::Vector2F HEADING_SHADOW_OFFSET = { 2.5f, 2.5f };
	const static MattMath::Vector2F ITEM_SHADOW_OFFSET = { 1.5f, 1.5f };
	const static MattMath::Vector2F DETAIL_SHADOW_OFFSET = { 1.0f, 1.0f };

	const std::string SOUND_BANK = "sound_bank_1";
	const std::string DIRECTION_SOUND = "UI_Clicks14";
	const std::string CONFIRM_SOUND = "UI_Clicks01";
	const std::string CANCEL_SOUND = "UI_Clicks17";
	const std::string WINNER_SOUND = "party_horn";
	constexpr float WINNER_VOLUME = 1.0f;
	const std::string FILL_SOUND = "DrumRoll";
	constexpr float FILL_VOLUME = 1.0f;
}

class ResultsMenuPage : public MenuPage, public SoundBankObject
{
private:
	ResultsMenuData* _data = nullptr;
protected:
	ResultsMenuData* get_results_menu_data();
public:
	ResultsMenuPage(ResultsMenuData* data) : 
		SoundBankObject(results_menu_consts::SOUND_BANK,
			this->get_resource_manager()),
		MenuPage(data),
		_data(data) {}
	virtual ~ResultsMenuPage() {}
	virtual void init() = 0;
	virtual void update() = 0;
	virtual void draw() = 0;
};

class ResultsMenuInitial : public ResultsMenuPage
{
private:
	std::unique_ptr<MContainer> _texture_container = nullptr;
	std::unique_ptr<MContainer> _text_container = nullptr;
	std::unique_ptr<MTexture> _box = nullptr;
	std::unique_ptr<MTextDropShadow> _heading = nullptr;
	std::unique_ptr<MTexture> _fill_box = nullptr;
	std::unique_ptr<MTexture> _team_a_fill = nullptr;
	std::unique_ptr<MTexture> _team_b_fill = nullptr;
	std::unique_ptr<MTextDropShadow> _team_a_percentage = nullptr;
	std::unique_ptr<MTextDropShadow> _team_b_percentage = nullptr;
	std::unique_ptr<MTextDropShadow> _winner = nullptr;
	std::unique_ptr<MTextDropShadow> _proceed = nullptr;
	bool _winner_sound_played = false;
	float _delay_timer = 0.0f;
	float _fill_timer = 0.0f;
	float _show_results_timer = 0.0f;
	level_end_info get_level_end_info();
	float fill_time_ratio();
	MattMath::Vector2F calculate_team_b_fill_top_right_position() const;
	int check_for_continue_input(const std::vector<menu_input>& menu_input);
	void update_fill_box();
	void update_team_a_fill();
	void update_team_b_fill();
public:
	ResultsMenuInitial(ResultsMenuData* data) : ResultsMenuPage(data) {}
	void init() override;
	void update() override;
	void draw() override;
};

#endif // !RESULTSMENU_H
