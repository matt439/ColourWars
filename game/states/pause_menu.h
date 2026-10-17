#pragma once

#include "game/states/pause_menu_action.h"
#include "engine/ui/widget.h"
#include "game/states/menu_page.h"
#include "engine/core/state_context.h"
#include "engine/render/colour.h"
#include "game/objects/player_input_data.h"
#include "game/input_slots.h"


class DebugDirector;

// Whether an input slot belongs to the match, including keyboard/mouse.
// MenuPlayerSettings::num is the device slot; its roster index is the view.
bool match_owns_pad_slot(const MenuLevelSettings& settings, int slot);

// A pad the match waits for: a pad slot that is not connected and whose seat
// the developer tools do not drive (a Debug bot or replay; never in Release).
// The one rule behind the missing-pad pause, the pause page's "Controller
// lost" and the restart confirmation's fallback.
bool pad_seat_missing(int slot, const labrador::Gamepads& gamepads,
	const DebugDirector& debug);
int disconnected_player_slot(const MenuLevelSettings& settings,
	const labrador::Gamepads& gamepads, const DebugDirector& debug);

// Over the intro, Start (Escape) skips it rather than pausing. A press that
// begins while the intro plays is latched for its device slot, and its
// release clears toggle_pause_menu however late it comes: a skip held past
// GO, or a Start held while another button skipped, never opens pause.
void latch_intro_pause(bool intro_playing, std::vector<PlayerInputData>& player_inputs,
	std::array<bool, input_slots::count>& latched);


class PauseMenuPage : public MenuPage
{
public:
	// Retains the input slot that opened the menu for its lifetime.
	PauseMenuPage(MenuContext* context, int player_num);
	~PauseMenuPage() override = default;
	void init() override = 0;
	void update_page(float dt) override = 0;
protected:
	bool shows_brand() const override { return false; }
	const PauseMenuPresentation& look() const;

	std::wstring player_number_text() const;

	int pausing_player() const;

	// Returns an action through the pushed frame, including after page transitions.
	void finish(PauseMenuAction action) const;

	// Reads the opening device, or returns neutral input for an invalid slot.
	// If disconnected, accepts another participant; if none are connected,
	// accepts any device. Resume and Restart require all participants reconnected.
	ProcessedMenuInput pausing_player_input(
		const std::vector<ProcessedMenuInput>& inputs) const;

	enum class ConfirmationType
	{
		restart,
		quit
	};

	MenuSound window_open_sound_;
private:
	int pausing_player_ = -1;
};


class PauseMenuInitial final : public PauseMenuPage
{
public:
	// Returning from a confirmation keeps the same pause session.
	PauseMenuInitial(MenuContext* context, int player_num, bool opening = true);
	void init() override;
	void update_page(float dt) override;
	// Resume (or quit, to leave the match) through the PauseMenuAction frame.
	bool debug_dismiss(bool leaving_match) override;
private:
	bool opening_ = true;
	void refresh_connections();
	bool waiting_for_players_ = false;
	std::unique_ptr<labrador::UiText> connection_status_ = nullptr;
	std::unique_ptr<labrador::UiTexture> box_ = nullptr;
	std::unique_ptr<labrador::UiText> player_num_ = nullptr;
	std::unique_ptr<labrador::UiText> resume_ = nullptr;
	std::unique_ptr<labrador::UiText> restart_ = nullptr;
	std::unique_ptr<labrador::UiText> quit_ = nullptr;
	std::unique_ptr<labrador::UiText> options_ = nullptr;
};

class PauseMenuConfirmation final : public PauseMenuPage
{
public:
	PauseMenuConfirmation(MenuContext* context, int player_num,
		ConfirmationType type);
	void init() override;
	void update_page(float dt) override;
	bool debug_dismiss(bool leaving_match) override;
private:
	std::unique_ptr<labrador::UiTexture> box_ = nullptr;
	std::unique_ptr<labrador::UiText> player_num_ = nullptr;
	std::unique_ptr<labrador::UiText> question_ = nullptr;
	std::unique_ptr<labrador::UiText> yes_ = nullptr;
	std::unique_ptr<labrador::UiText> no_ = nullptr;
	ConfirmationType type_ = ConfirmationType::restart;
	std::wstring question_text(ConfirmationType type) const;
};
