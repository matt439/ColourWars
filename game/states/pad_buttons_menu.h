#pragma once

#include "game/states/pause_menu.h"
#include "game/states/option_row.h"
#include "game/states/menu_panel.h"
#include "game/input_bindings.h"
#include <optional>

// Per-XInput-slot gameplay Jump, Shoot and Pause controls, each with an
// optional alternate. Choosing skips controls another entry of the same pad
// already uses, so a pending profile is always valid. Only edited pad slots are
// merged on Apply; keyboard bindings and other pages' preferences stay independent.
class PadButtonsMenu final : public PauseMenuPage
{
public:
	explicit PadButtonsMenu(MenuContext* context, int player = -1);
	void init() override;
	void update_page(float dt) override;
	bool covers_screen() const override { return true; }
	bool debug_dismiss(bool leaving_match) override;
	int selected_slot() const { return this->slot_; }
	const PadProfile& pending_profile() const { return this->selection_.pads[this->slot_]; }
protected:
	bool shows_focus_backing() const override { return false; }
	bool accepts_stick_navigation() const override { return false; }
private:
	// One row per action entry, in PadAction order with the alternate after its primary.
	static constexpr size_t entry_count = pad_action_count * 2;
	static void fit_text(labrador::UiText& widget, const mattmath::RectangleF& available);
	void refresh_values();
	void set_status(const std::wstring& value, bool warning = false);
	void choose(size_t entry, labrador::Direction direction);
	void reset_pending();
	void apply_settings();
	void close();
	InputBindings selection_;
	std::array<bool, input_slots::pad_count> edited_{};
	int slot_ = 0;
	std::unique_ptr<labrador::UiTexture> background_;
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::vector<std::unique_ptr<labrador::UiText>> text_;
	std::unique_ptr<OptionRow> pad_;
	std::array<std::unique_ptr<OptionRow>, entry_count> entries_;
	labrador::UiText* help_ = nullptr;
	labrador::UiText* status_ = nullptr;
	mattmath::RectangleF status_limit_;
};
