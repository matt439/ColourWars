#pragma once

#include "game/states/pause_menu_data.h"
#include "engine/ui/widget.h"
#include "game/states/menu_highlight.h"
#include "game/states/menu_page.h"
#include "engine/core/state_context.h"
#include "engine/audio/sound_bank_object.h"

namespace pause_menu_consts
{
	const static mattmath::Vector2F DEFAULT_RESOLUTION = { 1920.0f, 1080.0f };

	const static mattmath::Vector2F PAUSE_MENU_BOX_SIZE = { 900.0f, 600.0f };
	const static mattmath::Colour PAUSE_MENU_BOX_COLOUR = colour_consts::DODGER_BLUE;

	const static mattmath::Colour PAUSE_MENU_HEADING_TEXT_COLOUR = colour_consts::TEAM_ORANGE; //colour_consts::DARK_KHAKI;

	const static mattmath::Colour STANDARD_HIGHLIGHT = colour_consts::WHITE;
	const static mattmath::Colour STANDARD_UNHIGHLIGHT = colour_consts::BLUE;

	const static mattmath::Vector2F PAUSE_MENU_INITIAL_WIDGET_SIZE = { 100.0f, 50.0f };
	const static mattmath::Vector2F PAUSE_MENU_INITIAL_WIDGET_POSITION = { 610.0f, 300.0f };
	const static mattmath::Vector2F PAUSE_MENU_INITIAL_WIDGET_SPACING = { 200.0f, 100.0f };

	const static std::string ITEM_FONT = "gill_sans_mt_bold_48";
	const static std::string HEADING_FONT = "gill_sans_mt_bold_72";
	const static std::string DETAIL_FONT = "gill_sans_mt_bold_36";

	const static mattmath::Colour SHADOW_COLOUR = colour_consts::BLACK;
	const static mattmath::Vector2F TITLE_SHADOW_OFFSET = { 6.0f, 6.0f };
	const static mattmath::Vector2F HEADING_SHADOW_OFFSET = { 2.5f, 2.5f };
	const static mattmath::Vector2F ITEM_SHADOW_OFFSET = { 1.5f, 1.5f };
	const static mattmath::Vector2F DETAIL_SHADOW_OFFSET = { 1.0f, 1.0f };

	const std::string SOUND_BANK = "sound_bank_1";
	const std::string DIRECTION_SOUND = "UI_Clicks14";
	const std::string CONFIRM_SOUND = "UI_Clicks01";
	const std::string CANCEL_SOUND = "UI_Clicks17";
	const std::string WINDOW_OPEN_SOUND = "UI_Clicks01";
}

class PauseMenuPage : public MenuPage, public MenuHighlight, public artattack::SoundBankObject
{
public:
	explicit PauseMenuPage(PauseMenuData* data);
	~PauseMenuPage() override = default;
	void init() override = 0;
	void update() override = 0;
	void draw() override = 0;
protected:
	static std::string player_number_text(int player_num);
	PauseMenuData* pause_menu_data() const;

	// The input for the pad that opened this pause menu, or a neutral input if
	// that slot is out of range. This is the only menu page that reads a single
	// player's input rather than looping every slot, so it is the only one that
	// needs the bound check.
	ProcessedMenuInput pausing_player_input(
		const std::vector<ProcessedMenuInput>& inputs) const;

	enum class ConfirmationType
	{
		restart,
		quit
	};

	// Every sound this page family can make, resolved once when the page is
	// built. A press then plays an index, not a name (T7, T8).
	artattack::SoundBank::WaveHandle direction_sound_;
	artattack::SoundBank::WaveHandle confirm_sound_;
	artattack::SoundBank::WaveHandle cancel_sound_;
	artattack::SoundBank::WaveHandle window_open_sound_;
private:
	PauseMenuData* data_ = nullptr;
};


class PauseMenuInitial final : public PauseMenuPage
{
public:
	explicit PauseMenuInitial(PauseMenuData* data);
	void init() override;
	void update() override;
	void draw() override;
private:
	std::unique_ptr<artattack::MContainer> texture_container_ = nullptr;
	std::unique_ptr<artattack::MContainer> text_container_ = nullptr;
	std::unique_ptr<artattack::MTexture> box_ = nullptr;
	std::unique_ptr<artattack::MTextDropShadow> player_num_ = nullptr;
	std::unique_ptr<artattack::MTextDropShadow> resume_ = nullptr;
	std::unique_ptr<artattack::MTextDropShadow> restart_ = nullptr;
	std::unique_ptr<artattack::MTextDropShadow> quit_ = nullptr;
};

class PauseMenuConfirmation final : public PauseMenuPage
{
public:
	PauseMenuConfirmation(PauseMenuData* data, ConfirmationType type);
	void init() override;
	void update() override;
	void draw() override;
private:
	std::unique_ptr<artattack::MContainer> texture_container_ = nullptr;
	std::unique_ptr<artattack::MContainer> text_container_ = nullptr;
	std::unique_ptr<artattack::MTexture> box_ = nullptr;
	std::unique_ptr<artattack::MTextDropShadow> player_num_ = nullptr;
	std::unique_ptr<artattack::MTextDropShadow> question_ = nullptr;
	std::unique_ptr<artattack::MTextDropShadow> yes_ = nullptr;
	std::unique_ptr<artattack::MTextDropShadow> no_ = nullptr;
	ConfirmationType type_ = ConfirmationType::restart;
	static std::string question_text(ConfirmationType type);
};
