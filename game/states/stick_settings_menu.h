#pragma once

#include "game/states/settings_panel_page.h"
#include "game/states/option_row.h"
#include "game/input_slots.h"

#include <array>
#include <optional>

class StickResponsePreview;

// Profiles belong to XInput slots, independently of match roster/view numbers.
// Pending changes affect only the preview until Apply; closing never resumes play.
class StickSettingsMenu final : public SettingsPanelPage
{
public:
	explicit StickSettingsMenu(MenuContext* context, int player = -1);
	~StickSettingsMenu() override;
	void init() override;
	void update_page(float dt) override;
protected:
	bool accepts_stick_navigation() const override { return false; }
private:
	void refresh_values();
	void refresh_preview();
	int preview_device(int slot) const;
	void set_status(const std::wstring& text, bool warning = false);
	void apply_settings();
	PlaySettings selection_;
	int slot_ = 0;
	std::unique_ptr<StickResponsePreview> preview_;
	std::unique_ptr<OptionRow> pad_;
	std::unique_ptr<OptionRow> movement_;
	std::unique_ptr<OptionRow> aim_;
	std::unique_ptr<OptionRow> rumble_;
	std::unique_ptr<OptionRow> view_shake_;
	std::unique_ptr<OptionRow> aim_marker_;
	std::unique_ptr<OptionRow> reduce_effects_;
	std::array<bool, input_slots::pad_count> rumble_edited_{};
	std::array<bool, input_slots::pad_count> comfort_edited_{};
	labrador::UiText* connection_ = nullptr;
	labrador::UiText* raw_movement_ = nullptr;
	labrador::UiText* movement_response_ = nullptr;
	labrador::UiText* aim_readout_ = nullptr;
	// The integer inputs each live readout was last written from. Empty until
	// the first write, so that one always happens.
	std::optional<std::array<int, 2>> shown_connection_;
	std::optional<int> shown_raw_movement_;
	std::optional<int> shown_movement_response_;
	std::optional<std::array<int, 5>> shown_aim_;
};
