#pragma once

#include "game/states/end_menu_action.h"
#include "engine/ui/widget.h"
#include "game/states/menu_page.h"
#include "engine/core/state_context.h"
#include "engine/audio/sound_bank_object.h"
#include "engine/render/colour.h"

namespace end_menu_consts
{
	const static mattmath::Vector2F END_MENU_BOX_SIZE = { 900.0f, 600.0f };
	const static artattack::Colour END_MENU_BOX_COLOUR = artattack::Colour::dodger_blue; //artattack::Colour::dark_cyan;

	const static mattmath::Vector2F END_MENU_INITIAL_WIDGET_SIZE = { 100.0f, 50.0f };
	const static mattmath::Vector2F END_MENU_INITIAL_WIDGET_POSITION = { 700.0f, 230.0f };
	const static mattmath::Vector2F END_MENU_INITIAL_WIDGET_SPACING = { 100.0f, 85.0f };

	// The value it always had, written where it is used. It was
	// colour_consts::TEAM_ORANGE - a menu heading is not a team, and it
	// only ever shared the number.
	const static artattack::Colour END_MENU_HEADING_TEXT_COLOUR = artattack::Colour(255, 127, 0); //artattack::Colour::dark_khaki;

	const static artattack::Colour STANDARD_HIGHLIGHT = artattack::Colour::white;
	const static artattack::Colour STANDARD_UNHIGHLIGHT = artattack::Colour::blue;

	const static std::string ITEM_FONT = "gill_sans_mt_bold_48";
	const static std::string HEADING_FONT = "gill_sans_mt_bold_72";

	const static artattack::Colour SHADOW_COLOUR = artattack::Colour::black;
	const static mattmath::Vector2F TITLE_SHADOW_OFFSET = { 6.0f, 6.0f };
	const static mattmath::Vector2F HEADING_SHADOW_OFFSET = { 2.5f, 2.5f };
	const static mattmath::Vector2F ITEM_SHADOW_OFFSET = { 1.5f, 1.5f };
	const static mattmath::Vector2F DETAIL_SHADOW_OFFSET = { 1.0f, 1.0f };

	const std::string SOUND_BANK = "sound_bank_1";
	const std::string DIRECTION_SOUND = "UI_Clicks14";
	const std::string CONFIRM_SOUND = "UI_Clicks01";
	const std::string CANCEL_SOUND = "UI_Clicks17";
}

class EndMenuPage : public MenuPage, public artattack::SoundBankObject
{
public:
	explicit EndMenuPage(MenuContext* context);
	~EndMenuPage() override = default;
	void init() override = 0;
	void update(float dt) override = 0;
	void draw(artattack::Renderer& renderer) const override = 0;
protected:
	// Closes the end menu with what the player chose. EndMenuData held nothing
	// but the out-parameter this replaces, so it is gone entirely.
	void finish(EndMenuAction action) const;

	// Every sound this page family can make, resolved once when the page is
	// built. A press then plays an index, not a name (T7, T8).
	artattack::SoundBank::WaveHandle direction_sound_;
	artattack::SoundBank::WaveHandle confirm_sound_;
	artattack::SoundBank::WaveHandle cancel_sound_;
};

class EndMenuInitial final : public EndMenuPage
{
public:
	explicit EndMenuInitial(MenuContext* context);
	void init() override;
	void update(float dt) override;
	void draw(artattack::Renderer& renderer) const override;
private:
	std::unique_ptr<artattack::UiContainer> texture_container_ = nullptr;
	std::unique_ptr<artattack::UiContainer> text_container_ = nullptr;
	std::unique_ptr<artattack::UiTexture> box_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> heading_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> change_teams_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> change_weapons_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> change_level_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> restart_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> exit_ = nullptr;
};
