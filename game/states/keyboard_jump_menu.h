#pragma once

#include "game/states/pause_menu.h"
#include "game/states/menu_panel.h"
#include "game/states/keyboard_jump_capture.h"
#include "game/input_bindings.h"

// The shared keyboard player's Jump preference. This child owns only the
// independent input-bindings section; pending parent play settings stay intact.
class KeyboardJumpMenu final : public PauseMenuPage
{
public:
	explicit KeyboardJumpMenu(MenuContext* context, int player = -1);
	~KeyboardJumpMenu() override;
	void init() override;
	void update_page(float dt) override;
	void on_suspend() override;
	void on_resume() override;
	void on_deactivated() override;
	bool covers_screen() const override { return true; }
	bool debug_dismiss(bool leaving_match) override;

	bool capture_active() const { return this->capture_active_; }
	labrador::Key pending_key() const { return this->selection_.keyboard_jump; }
protected:
	bool shows_focus_backing() const override { return false; }
	bool owns_raw_keyboard_input() const override { return this->capture_active_; }
private:
	void begin_capture();
	void update_capture();
	void finish_capture(bool accepted);
	void refresh_values();
	void refresh_capture_status();
	void set_status(const std::wstring& value, bool warning = false);
	void apply();
	void reset_pending();
	void close();
	void set_actions_enabled(bool enabled);
	bool capture_pad_back();
	static void fit_text(labrador::UiText& widget, const mattmath::RectangleF& available);

	InputBindings selection_;
	KeyboardJumpCapture capture_;
	// A separate armed reader supplies only pad Back during capture. Its
	// keyboard/menu aliases never affect the shared parent's input ownership.
	MenuInput capture_input_;
	bool capture_active_ = false;
	std::unique_ptr<labrador::UiTexture> background_;
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::vector<std::unique_ptr<labrador::UiText>> text_;
	std::vector<labrador::UiWidget*> actions_;
	labrador::UiText* help_ = nullptr;
	labrador::UiText* status_ = nullptr;
	// What the help text was last built from; it is rebuilt only on a change.
	bool help_shown_ = false;
	labrador::Key shown_saved_key_{};
	labrador::Key shown_pending_key_{};
	bool shown_collision_ = false;
	mattmath::RectangleF help_limit_;
	mattmath::RectangleF status_limit_;
	labrador::Colour status_ink_;
	labrador::Colour warning_ink_;
};
