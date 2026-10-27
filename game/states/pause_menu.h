#pragma once

#include "game/states/pause_menu_action.h"
#include "engine/ui/widget.h"
#include "game/states/menu_page.h"
#include "engine/core/state_context.h"
#include "engine/render/colour.h"
#include "game/objects/player_input_data.h"
#include "game/input_slots.h"
#include "game/objects/weapon_type.h"

#include <optional>


class DebugDirector;

// Whether an input slot belongs to the match, including keyboard/mouse.
// MenuPlayerSettings::num is the device slot; its roster index is the view.
bool match_owns_pad_slot(const MenuLevelSettings& settings, int slot);

// A pad the match waits for: a pad slot that is not connected and whose seat
// the developer tools do not drive (a Debug bot or replay; never in Release).
// The one rule behind the missing-pad pause, the pause page's "Controller
// lost" and the restart confirmation's fallback.
bool pad_seat_missing(const MenuLevelSettings& settings, int slot,
	const labrador::Gamepads& gamepads, const DebugDirector& debug);
int disconnected_player_slot(const MenuLevelSettings& settings,
	const labrador::Gamepads& gamepads, const DebugDirector& debug);

// ROAD#4. XInput cannot tell a controller that came back in another slot from
// a spare one, so the press is the claim: while the match waits, Start on a
// pad that no seat reads takes the first missing seat in roster order
// (missing_seats has a bit per roster index). Returns that seat's input slot,
// or -1 with the settings unchanged.
int reclaim_missing_seat(MenuLevelSettings& settings,
	const std::vector<ProcessedMenuInput>& inputs, unsigned int missing_seats);

// Over the intro, Start (Escape) skips it rather than pausing. A press that
// begins while the intro plays is latched for its device slot, and its
// release clears toggle_pause_menu however late it comes: a skip held past
// GO, or a Start held while another button skipped, never opens pause.
void latch_intro_pause(bool intro_playing, std::vector<PlayerInputData>& player_inputs,
	std::array<bool, input_slots::count>& latched);


class PauseMenuPage : public MenuPage
{
public:
	// Retains the input slot that opened the menu for its lifetime. A
	// practice page belongs to untimed solo practice (POL#19).
	PauseMenuPage(MenuContext* context, int player_num, bool practice = false);
	~PauseMenuPage() override = default;
	void init() override = 0;
	void update_page(float dt) override = 0;
protected:
	bool shows_brand() const override { return false; }
	const PauseMenuPresentation& look() const;

	std::wstring player_number_text() const;

	int pausing_player() const;
	bool practice() const;

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
	bool practice_ = false;
};


class PauseMenuInitial final : public PauseMenuPage
{
public:
	// Returning from a confirmation keeps the same pause session. Practice
	// adds a Tool row under Resume: left/right chooses, select rebuilds the
	// course clean with that tool where the player stands.
	PauseMenuInitial(MenuContext* context, int player_num, bool opening = true,
		bool practice = false);
	void init() override;
	void update_page(float dt) override;
	// Resume (or quit, to leave the match) through the PauseMenuAction frame.
	bool debug_dismiss(bool leaving_match) override;
private:
	bool opening_ = true;
	void refresh_connections();
	void cycle_tool(labrador::Direction direction);
	WeaponType tool_choice_ = WeaponType::sprayer;
	bool waiting_for_players_ = false;
	// Bit per roster seat whose pad was missing when the texts were last
	// built. Empty until the first build, so that one always happens.
	std::optional<unsigned int> shown_missing_seats_;
	std::unique_ptr<labrador::UiText> connection_status_ = nullptr;
	std::unique_ptr<labrador::UiTexture> box_ = nullptr;
	std::unique_ptr<labrador::UiText> player_num_ = nullptr;
	std::unique_ptr<labrador::UiText> resume_ = nullptr;
	std::unique_ptr<labrador::UiText> restart_ = nullptr;
	std::unique_ptr<labrador::UiText> quit_ = nullptr;
	std::unique_ptr<labrador::UiText> options_ = nullptr;
	std::unique_ptr<labrador::UiText> tool_ = nullptr;
};

class PauseMenuConfirmation final : public PauseMenuPage
{
public:
	PauseMenuConfirmation(MenuContext* context, int player_num,
		ConfirmationType type, bool practice = false);
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
