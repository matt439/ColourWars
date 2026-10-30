#pragma once

#include "game/states/settings_panel_page.h"
#include "game/states/option_row.h"
#include "game/states/keyboard_jump_capture.h"
#include "game/input_bindings.h"

// The shared keyboard player's gameplay keys (the Keyboard Keys page, which
// grew from Keyboard Jump): one capture row per KeyboardAction, then Reset,
// Apply and Back. This child owns only the input-bindings section's keyboard
// choices; pending parent play settings and pad profiles stay intact.
class KeyboardJumpMenu final : public SettingsPanelPage
{
public:
	explicit KeyboardJumpMenu(MenuContext* context, int player = -1);
	~KeyboardJumpMenu() override;
	void init() override;
	void update_page(float dt) override;
	void on_suspend() override;
	void on_resume() override;
	void on_deactivated() override;
	// Closes like Back, except during a capture, which owns input.
	bool debug_dismiss(bool leaving_match) override;

	bool capture_active() const { return this->capture_active_; }
	labrador::Key pending_key() const { return this->selection_.keyboard_jump; }
	const InputBindings& pending_bindings() const { return this->selection_; }
	KeyboardAction capture_action() const { return this->capture_.action(); }
protected:
	bool owns_raw_keyboard_input() const override { return this->capture_active_; }
private:
	void begin_capture(KeyboardAction action);
	void update_capture();
	void finish_capture(bool accepted);
	void refresh_values();
	void refresh_capture_status();
	void set_status(const std::wstring& value, bool warning = false);
	void apply();
	void reset_pending();
	void set_actions_enabled(bool enabled);
	bool capture_pad_back();
	bool debug_collision() const;

	InputBindings selection_;
	KeyboardJumpCapture capture_;
	// A separate armed reader supplies only pad Back during capture. Its
	// keyboard/menu aliases never affect the shared parent's input ownership.
	MenuInput capture_input_;
	bool capture_active_ = false;
	std::array<std::unique_ptr<OptionRow>, keyboard_action_count> rows_;
	std::vector<labrador::UiWidget*> actions_;
	labrador::UiText* help_ = nullptr;
	// What the help text was last built from; it is rebuilt only on a change.
	bool help_shown_ = false;
	bool shown_collision_ = false;
	mattmath::RectangleF help_limit_;
	mattmath::RectangleF status_limit_;
	labrador::Colour status_ink_;
	labrador::Colour warning_ink_;
};
