#pragma once

#include "game/states/display_settings_trial.h"
#include "game/states/menu_page.h"
#include "game/states/menu_panel.h"

#include <chrono>
#include <memory>
#include <optional>

// A separate child frame owns a temporary display change and returns its result
// to Options. Interruptions roll back immediately; only the top frame closes.
class MainMenuDisplayConfirmation final : public MenuPage
{
public:
	MainMenuDisplayConfirmation(MenuContext* context, DisplaySettingsChoice candidate,
		std::optional<int> pausing_slot = std::nullopt);
	~MainMenuDisplayConfirmation() override;
	void init() override;
	void update_page(float dt) override;
	void on_suspend() override;
	void on_resume() override;
	void on_deactivated() override;
	bool debug_dismiss(bool leaving_match) override;
	void debug_reset_input(int pad_slot) override;
protected:
	bool shows_brand() const override { return false; }
	bool shows_focus_backing() const override { return false; }
private:
	void cancel_trial();
	bool close_if_finished();
	void refresh_countdown();
	DisplaySettingsChoice candidate_;
	std::optional<int> pausing_slot_;
	PauseMenuPresentation look_;
	float timeout_;
	std::chrono::steady_clock::time_point started_at_;
	double simulated_seconds_ = 0.0;
	double advanced_seconds_ = 0.0;
	bool covered_ = false;
	bool closing_ = false;
	bool applying_ = false;
	bool destructing_ = false;
	std::unique_ptr<labrador::UiTexture> background_;
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::unique_ptr<labrador::UiText> heading_;
	std::unique_ptr<labrador::UiText> question_;
	std::unique_ptr<labrador::UiText> keep_;
	std::unique_ptr<labrador::UiText> revert_;
	std::unique_ptr<labrador::UiText> countdown_;
	std::unique_ptr<DisplaySettingsTrial> trial_;
};
