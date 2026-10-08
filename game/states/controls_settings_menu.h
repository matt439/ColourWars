#pragma once

#include "game/states/pause_menu.h"
#include "game/states/option_row.h"

// Pushed above either Options or Pause. The underlying page and its pending
// selections survive, and leaving this page never resumes a paused match.
// A negative player allows the main menu's usual any-device navigation.
class ControlsSettingsMenu final : public PauseMenuPage
{
public:
	explicit ControlsSettingsMenu(MenuContext* context, int player = -1);
	void init() override;
	void update_page(float dt) override;
	bool covers_screen() const override { return true; }
private:
	void refresh_values();
	void apply_settings();
	void close();
	PlaySettings selection_;
	std::unique_ptr<labrador::UiTexture> background_;
	std::vector<std::unique_ptr<labrador::UiTextDropShadow>> text_;
	std::unique_ptr<OptionRow> aim_;
	std::unique_ptr<OptionRow> music_;
	std::unique_ptr<OptionRow> sfx_;
	labrador::UiTextDropShadow* status_ = nullptr;
};
