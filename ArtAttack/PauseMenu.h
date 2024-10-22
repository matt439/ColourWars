#ifndef PAUSEMENU_H
#define PAUSEMENU_H

#include "PauseMenuData.h"
#include "Mh.h"
#include "MenuHighlight.h"
#include "MenuPage.h"
#include "StateContext.h"
#include "SoundBankObject.h"

namespace pause_menu_consts
{
	const static MattMath::Vector2F DEFAULT_RESOLUTION = { 1920.0f, 1080.0f };

	const static MattMath::Vector2F PAUSE_MENU_BOX_SIZE = { 900.0f, 600.0f };
	const static MattMath::Colour PAUSE_MENU_BOX_COLOUR = colour_consts::DODGER_BLUE;

	const static MattMath::Colour PAUSE_MENU_HEADING_TEXT_COLOUR = colour_consts::TEAM_ORANGE; //colour_consts::DARK_KHAKI;

	const static MattMath::Colour STANDARD_HIGHLIGHT = colour_consts::WHITE;
	const static MattMath::Colour STANDARD_UNHIGHLIGHT = colour_consts::BLUE;

	const static MattMath::Vector2F PAUSE_MENU_INITIAL_WIDGET_SIZE = { 100.0f, 50.0f };
	const static MattMath::Vector2F PAUSE_MENU_INITIAL_WIDGET_POSITION = { 610.0f, 300.0f };
	const static MattMath::Vector2F PAUSE_MENU_INITIAL_WIDGET_SPACING = { 200.0f, 100.0f };

	const static std::string ITEM_FONT = "gill_sans_mt_bold_48";
	const static std::string HEADING_FONT = "gill_sans_mt_bold_72";
	const static std::string DETAIL_FONT = "gill_sans_mt_bold_36";

	const static MattMath::Colour SHADOW_COLOUR = colour_consts::BLACK;
	const static MattMath::Vector2F TITLE_SHADOW_OFFSET = { 6.0f, 6.0f };
	const static MattMath::Vector2F HEADING_SHADOW_OFFSET = { 2.5f, 2.5f };
	const static MattMath::Vector2F ITEM_SHADOW_OFFSET = { 1.5f, 1.5f };
	const static MattMath::Vector2F DETAIL_SHADOW_OFFSET = { 1.0f, 1.0f };

	const std::string SOUND_BANK = "sound_bank_1";
	const std::string DIRECTION_SOUND = "UI_Clicks14";
	const std::string CONFIRM_SOUND = "UI_Clicks01";
	const std::string CANCEL_SOUND = "UI_Clicks17";
	const std::string WINDOW_OPEN_SOUND = "UI_Clicks01";
}

class PauseMenuPage : public MenuPage, public MenuHighlight, public SoundBankObject
{
public:
	explicit PauseMenuPage(PauseMenuData* data) :
		MenuPage(data),
		SoundBankObject(pause_menu_consts::SOUND_BANK,
			this->get_resource_manager()),
		_data(data) {}

	~PauseMenuPage() override = default;
	void init() override = 0;
	void update() override = 0;
	void draw() override = 0;
protected:
	static std::string get_player_number_text(int player_num);
	PauseMenuData* get_pause_menu_data() const;
	enum class confirmation_type
	{
		RESTART,
		QUIT
	};
private:
	PauseMenuData* _data = nullptr;
};


class PauseMenuInitial final : public PauseMenuPage
{
public:
	explicit PauseMenuInitial(PauseMenuData* data) : PauseMenuPage(data) {}
	void init() override;
	void update() override;
	void draw() override;
private:
	std::unique_ptr<MContainer> _texture_container = nullptr;
	std::unique_ptr<MContainer> _text_container = nullptr;
	std::unique_ptr<MTexture> _box = nullptr;
	std::unique_ptr<MTextDropShadow> _player_num = nullptr;
	std::unique_ptr<MTextDropShadow> _resume = nullptr;
	std::unique_ptr<MTextDropShadow> _restart = nullptr;
	std::unique_ptr<MTextDropShadow> _quit = nullptr;
};

class PauseMenuConfirmation final : public PauseMenuPage
{
public:
	PauseMenuConfirmation(PauseMenuData* data, confirmation_type type) :
		PauseMenuPage(data), _type(type) {}
	void init() override;
	void update() override;
	void draw() override;
private:
	std::unique_ptr<MContainer> _texture_container = nullptr;
	std::unique_ptr<MContainer> _text_container = nullptr;
	std::unique_ptr<MTexture> _box = nullptr;
	std::unique_ptr<MTextDropShadow> _player_num = nullptr;
	std::unique_ptr<MTextDropShadow> _question = nullptr;
	std::unique_ptr<MTextDropShadow> _yes = nullptr;
	std::unique_ptr<MTextDropShadow> _no = nullptr;
	confirmation_type _type = confirmation_type::RESTART;
	static std::string get_question_text(confirmation_type type);
};

#endif // !PAUSEMENU_H
