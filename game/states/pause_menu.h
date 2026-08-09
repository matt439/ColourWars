#pragma once

#include "game/states/pause_menu_action.h"
#include "engine/ui/widget.h"
#include "game/states/menu_page.h"
#include "engine/core/state_context.h"
#include "engine/audio/sound_bank_object.h"
#include "engine/render/colour.h"


class PauseMenuPage : public MenuPage, public artattack::SoundBankObject
{
public:
	// The pad slot that opened the menu is a constant for the whole time it is
	// up, so it is a constructor parameter. It used to live on a PauseMenuData
	// that existed mostly to carry the *result* out - and the result comes back
	// down the stack now.
	PauseMenuPage(MenuContext* context, int player_num);
	~PauseMenuPage() override = default;
	void init() override = 0;
	void update(float dt) override = 0;
	void draw(artattack::Renderer& renderer) const override = 0;
protected:
	// This menu family's own presentation, beside MenuPage::menu_style()'s
	// shared one. Named for what a reader is looking for rather than for the
	// type it returns: `this->look().box_colour` at the call site.
	const PauseMenuPresentation& look() const;

	static std::wstring player_number_text(int player_num);

	// Not player_num_, which is the name both pages of this family already use
	// for the heading widget that spells it out.
	int pausing_player() const;

	// Closes the whole pause menu with an answer for the match underneath.
	// Every page of this family ends in one of these, including the one two
	// transitions deep: the result belongs to the pushed frame, not to the page
	// that happens to be in it (state_context.h).
	void finish(PauseMenuAction action) const;

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
	int pausing_player_ = -1;
};


class PauseMenuInitial final : public PauseMenuPage
{
public:
	PauseMenuInitial(MenuContext* context, int player_num);
	void init() override;
	void update(float dt) override;
	void draw(artattack::Renderer& renderer) const override;
private:
	std::unique_ptr<artattack::UiContainer> texture_container_ = nullptr;
	std::unique_ptr<artattack::UiContainer> text_container_ = nullptr;
	std::unique_ptr<artattack::UiTexture> box_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> player_num_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> resume_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> restart_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> quit_ = nullptr;
};

class PauseMenuConfirmation final : public PauseMenuPage
{
public:
	PauseMenuConfirmation(MenuContext* context, int player_num,
		ConfirmationType type);
	void init() override;
	void update(float dt) override;
	void draw(artattack::Renderer& renderer) const override;
private:
	std::unique_ptr<artattack::UiContainer> texture_container_ = nullptr;
	std::unique_ptr<artattack::UiContainer> text_container_ = nullptr;
	std::unique_ptr<artattack::UiTexture> box_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> player_num_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> question_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> yes_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> no_ = nullptr;
	ConfirmationType type_ = ConfirmationType::restart;
	static std::wstring question_text(ConfirmationType type);
};
