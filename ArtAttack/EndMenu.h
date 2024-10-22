#ifndef ENDMENU_H
#define ENDMENU_H

#include "EndMenuData.h"
#include "Mh.h"
#include "MenuHighlight.h"
#include "MenuPage.h"
#include "StateContext.h"
#include "SoundBankObject.h"

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
	EndMenuPage(EndMenuData* data) :
		MenuPage(data),
		SoundBankObject(end_menu_consts::SOUND_BANK,
			this->get_resource_manager()),
		_data(data) {}
	virtual ~EndMenuPage() {}
	virtual void init() = 0;
	virtual void update() = 0;
	virtual void draw() = 0;
protected:
	EndMenuData* get_end_menu_data();
private:
	EndMenuData* _data = nullptr;
};


class EndMenuInitial : public EndMenuPage
{
public:
	EndMenuInitial(EndMenuData* data) : EndMenuPage(data) {}
	void init() override;
	void update() override;
	void draw() override;
private:
	std::unique_ptr<MContainer> _texture_container = nullptr;
	std::unique_ptr<MContainer> _text_container = nullptr;
	std::unique_ptr<MTexture> _box = nullptr;
	std::unique_ptr<MTextDropShadow> _heading = nullptr;
	std::unique_ptr<MTextDropShadow> _change_teams = nullptr;
	std::unique_ptr<MTextDropShadow> _change_weapons = nullptr;
	std::unique_ptr<MTextDropShadow> _change_level = nullptr;
	std::unique_ptr<MTextDropShadow> _restart = nullptr;
	std::unique_ptr<MTextDropShadow> _exit = nullptr;
};


#endif // !ENDMENU_H
