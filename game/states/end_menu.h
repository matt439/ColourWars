#pragma once

#include "game/states/end_menu_data.h"
#include "engine/ui/widget.h"
#include "game/states/menu_highlight.h"
#include "game/states/menu_page.h"
#include "engine/core/state_context.h"
#include "engine/audio/sound_bank_object.h"

namespace end_menu_consts
{
	const static MattMath::Vector2F DEFAULT_RESOLUTION = { 1920.0f, 1080.0f };

	const static MattMath::Vector2F END_MENU_BOX_SIZE = { 900.0f, 600.0f };
	const static MattMath::Colour END_MENU_BOX_COLOUR = colour_consts::DODGER_BLUE; //colour_consts::DARK_CYAN;

	const static MattMath::Vector2F END_MENU_INITIAL_WIDGET_SIZE = { 100.0f, 50.0f };
	const static MattMath::Vector2F END_MENU_INITIAL_WIDGET_POSITION = { 700.0f, 230.0f };
	const static MattMath::Vector2F END_MENU_INITIAL_WIDGET_SPACING = { 100.0f, 85.0f };

	const static MattMath::Colour END_MENU_HEADING_TEXT_COLOUR = colour_consts::TEAM_ORANGE; //colour_consts::DARK_KHAKI;

	const static MattMath::Colour STANDARD_HIGHLIGHT = colour_consts::WHITE;
	const static MattMath::Colour STANDARD_UNHIGHLIGHT = colour_consts::BLUE;

	const static std::string ITEM_FONT = "gill_sans_mt_bold_48";
	const static std::string HEADING_FONT = "gill_sans_mt_bold_72";

	const static MattMath::Colour SHADOW_COLOUR = colour_consts::BLACK;
	const static MattMath::Vector2F TITLE_SHADOW_OFFSET = { 6.0f, 6.0f };
	const static MattMath::Vector2F HEADING_SHADOW_OFFSET = { 2.5f, 2.5f };
	const static MattMath::Vector2F ITEM_SHADOW_OFFSET = { 1.5f, 1.5f };
	const static MattMath::Vector2F DETAIL_SHADOW_OFFSET = { 1.0f, 1.0f };

	const std::string SOUND_BANK = "sound_bank_1";
	const std::string DIRECTION_SOUND = "UI_Clicks14";
	const std::string CONFIRM_SOUND = "UI_Clicks01";
	const std::string CANCEL_SOUND = "UI_Clicks17";
}

class EndMenuPage : public MenuPage, public MenuHighlight, public SoundBankObject
{
public:
	explicit EndMenuPage(EndMenuData* data);
	~EndMenuPage() override = default;
	void init() override = 0;
	void update() override = 0;
	void draw() override = 0;
protected:
	EndMenuData* get_end_menu_data() const;

	// Every sound this page family can make, resolved once when the page is
	// built. A press then plays an index, not a name (T7, T8).
	SoundBank::WaveHandle direction_sound_;
	SoundBank::WaveHandle confirm_sound_;
	SoundBank::WaveHandle cancel_sound_;
private:
	EndMenuData* data_ = nullptr;
};

class EndMenuInitial final : public EndMenuPage
{
public:
	explicit EndMenuInitial(EndMenuData* data);
	void init() override;
	void update() override;
	void draw() override;
private:
	std::unique_ptr<MContainer> texture_container_ = nullptr;
	std::unique_ptr<MContainer> text_container_ = nullptr;
	std::unique_ptr<MTexture> box_ = nullptr;
	std::unique_ptr<MTextDropShadow> heading_ = nullptr;
	std::unique_ptr<MTextDropShadow> change_teams_ = nullptr;
	std::unique_ptr<MTextDropShadow> change_weapons_ = nullptr;
	std::unique_ptr<MTextDropShadow> change_level_ = nullptr;
	std::unique_ptr<MTextDropShadow> restart_ = nullptr;
	std::unique_ptr<MTextDropShadow> exit_ = nullptr;
};
