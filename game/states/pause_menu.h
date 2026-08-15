#pragma once

#include "game/states/pause_menu_action.h"
#include "engine/ui/widget.h"
#include "game/states/menu_page.h"
#include "engine/core/state_context.h"
#include "engine/render/colour.h"


// Whether a pad slot is one this match is actually being played on.
//
// MenuPlayerSettings::num is the pad slot a player was built for; the index
// into the settings is that player's view, not its pad (level.h). Two callers
// want the same answer - the match, deciding whose Start button opens a pause
// menu, and the pause menu, deciding who may drive it once the pad that opened
// it has gone - so they ask it here instead of each assuming "any of the four
// slots".
bool match_owns_pad_slot(const MenuLevelSettings& settings, int slot);


class PauseMenuPage : public MenuPage
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
	//
	// With one exception, and it is what stands between an unplugged controller
	// and a match that can never be resumed, restarted or quit: nothing below
	// this page is updated while it is up, so a pausing pad that is no longer
	// connected leaves nobody able to answer at all. Then, and only then, any
	// pad still in the match speaks for it.
	ProcessedMenuInput pausing_player_input(
		const std::vector<ProcessedMenuInput>& inputs) const;

	enum class ConfirmationType
	{
		restart,
		quit
	};

	// The one sound only this family makes, beside the three every page shares
	// from MenuPage. A press plays an index, not a name (T7, T8).
	labrador::SoundBank::WaveHandle window_open_sound_;
private:
	int pausing_player_ = -1;
};


class PauseMenuInitial final : public PauseMenuPage
{
public:
	PauseMenuInitial(MenuContext* context, int player_num);
	void init() override;
	void update(float dt) override;
private:
	std::unique_ptr<labrador::UiTexture> box_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> player_num_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> resume_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> restart_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> quit_ = nullptr;
};

class PauseMenuConfirmation final : public PauseMenuPage
{
public:
	PauseMenuConfirmation(MenuContext* context, int player_num,
		ConfirmationType type);
	void init() override;
	void update(float dt) override;
private:
	std::unique_ptr<labrador::UiTexture> box_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> player_num_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> question_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> yes_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> no_ = nullptr;
	ConfirmationType type_ = ConfirmationType::restart;
	static std::wstring question_text(ConfirmationType type);
};
