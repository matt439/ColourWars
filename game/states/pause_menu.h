#pragma once

#include "game/states/pause_menu_action.h"
#include "engine/ui/widget.h"
#include "game/states/menu_page.h"
#include "engine/core/state_context.h"
#include "engine/render/colour.h"


// Whether an input slot belongs to the match, including keyboard/mouse.
// MenuPlayerSettings::num is the device slot; its roster index is the view.
bool match_owns_pad_slot(const MenuLevelSettings& settings, int slot);
int disconnected_player_slot(const MenuLevelSettings& settings,
	const labrador::Gamepads& gamepads);


class PauseMenuPage : public MenuPage
{
public:
	// Retains the input slot that opened the menu for its lifetime.
	PauseMenuPage(MenuContext* context, int player_num);
	~PauseMenuPage() override = default;
	void init() override = 0;
	void update(float dt) override = 0;
protected:
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
	void refresh_connections();
	bool waiting_for_players_ = false;
	std::unique_ptr<labrador::UiTextDropShadow> connection_status_ = nullptr;
	std::unique_ptr<labrador::UiTexture> box_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> player_num_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> resume_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> restart_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> quit_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> controls_ = nullptr;
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
