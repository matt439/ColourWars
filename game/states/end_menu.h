#pragma once

#include "game/states/end_menu_action.h"
#include "engine/ui/widget.h"
#include "game/states/menu_page.h"
#include "engine/core/state_context.h"
#include "engine/audio/sound_bank_object.h"
#include "engine/render/colour.h"

class EndMenuPage : public MenuPage, public artattack::SoundBankObject
{
public:
	explicit EndMenuPage(MenuContext* context);
	~EndMenuPage() override = default;
	void init() override = 0;
	void update(float dt) override = 0;
	void draw(artattack::Renderer& renderer) const override = 0;
protected:
	// This menu family's own presentation, beside MenuPage::menu_style()'s
	// shared one. Named for what a reader is looking for rather than for the
	// type it returns: `this->look().box_colour` at the call site.
	const EndMenuPresentation& look() const;

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
