#pragma once

#include "game/states/settings_panel_page.h"
#include "game/states/option_row.h"

// Pushed above either Options or Pause. The underlying page and its pending
// selections survive, and leaving this page never resumes a paused match.
// A negative player allows the main menu's usual any-device navigation.
class ControlsSettingsMenu final : public SettingsPanelPage
{
public:
	explicit ControlsSettingsMenu(MenuContext* context, int player = -1);
	void init() override;
	void update_page(float dt) override;
	void on_resume() override;
	// What Apply saves: the settings in effect now with this page's own
	// fields (both volumes and the three shared comfort switches) taken from
	// `page`. Child pages commit the rest while this one is open, so nothing
	// else comes from the page's older copy.
	static PlaySettings applied_settings(PlaySettings current, const PlaySettings& page);
private:
	void refresh_values();
	void refresh_help();
	void set_status(const std::wstring& text, const labrador::Colour& colour);
	void apply_settings();
	PlaySettings selection_;
	labrador::SoundBank::WaveHandle volume_preview_sound_;
	std::unique_ptr<OptionRow> music_;
	std::unique_ptr<OptionRow> sfx_;
	std::unique_ptr<OptionRow> view_shake_;
	std::unique_ptr<OptionRow> aim_marker_;
	std::unique_ptr<OptionRow> reduce_effects_;
	labrador::UiText* controls_ = nullptr;
	labrador::RectangleF controls_limit_;
};
