#pragma once

#include "game/states/pause_menu.h"
#include "game/states/option_row.h"
#include "game/states/menu_panel.h"

// Pushed above either Options or Pause. The underlying page and its pending
// selections survive, and leaving this page never resumes a paused match.
// A negative player allows the main menu's usual any-device navigation.
class ControlsSettingsMenu final : public PauseMenuPage
{
public:
	explicit ControlsSettingsMenu(MenuContext* context, int player = -1);
	void init() override;
	void update_page(float dt) override;
	void on_resume() override;
	bool covers_screen() const override { return true; }
	// Closes like Back. Not PauseMenuPage's way out: this page is pushed with
	// no result, and pop(PauseMenuAction) on its frame would throw.
	bool debug_dismiss(bool leaving_match) final;
protected:
	bool shows_focus_backing() const override { return false; }
private:
	void refresh_values();
	void refresh_help();
	void set_status(const std::wstring& text, const labrador::Colour& colour);
	void apply_settings();
	void close();
	PlaySettings selection_;
	labrador::SoundBank::WaveHandle volume_preview_sound_;
	std::unique_ptr<labrador::UiTexture> background_;
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::vector<std::unique_ptr<labrador::UiText>> text_;
	std::unique_ptr<OptionRow> music_;
	std::unique_ptr<OptionRow> sfx_;
	std::unique_ptr<OptionRow> view_shake_;
	std::unique_ptr<OptionRow> aim_marker_;
	std::unique_ptr<OptionRow> reduce_effects_;
	labrador::UiText* status_ = nullptr;
	labrador::UiText* controls_ = nullptr;
	mattmath::RectangleF controls_limit_;
};
