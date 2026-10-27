#pragma once

#include "game/states/pause_menu.h"
#include "game/states/option_row.h"
#include "game/states/menu_panel.h"
#include <optional>
#include <string>

// Player profiles (ROAD#21): choose one, rename it, set its Boost, choose the
// pad that wears it, make a new one or delete it. Every change saves at once,
// so there is no Apply; wearing is for this run only. The settings a profile
// carries are edited on Pad sticks and Pad buttons by the pad wearing it.
class ProfilesMenu final : public PauseMenuPage
{
public:
	explicit ProfilesMenu(MenuContext* context, int player = -1);
	void init() override;
	void update_page(float dt) override;
	bool covers_screen() const override { return true; }
	bool debug_dismiss(bool leaving_match) override;
	int selected_profile() const { return this->profile_; }
	bool editing_name() const { return this->name_edit_.has_value(); }
protected:
	bool shows_focus_backing() const override { return false; }
	bool accepts_stick_navigation() const override { return false; }
private:
	// The name editor's buffer and cursor, while a name is being edited.
	struct NameEdit
	{
		std::string buffer;
		size_t cursor = 0;
	};
	static void fit_text(labrador::UiText& widget, const mattmath::RectangleF& available);
	void refresh_values();
	void set_status(const std::wstring& value, bool warning = false);
	void report_save(bool saved, const std::wstring& success);
	void begin_name_edit();
	void update_name_edit(const ProcessedMenuInput& input);
	void finish_name_edit(bool keep);
	void change_profile(labrador::Direction direction);
	void change_boost(labrador::Direction direction);
	void change_wearer(labrador::Direction direction);
	void add_profile();
	void delete_profile();
	void close();
	// -1 when there are no profiles.
	int profile_ = -1;
	// The slot whose press reached this page last: a new profile copies it.
	int pressing_slot_ = -1;
	bool delete_armed_ = false;
	std::optional<NameEdit> name_edit_;
	std::unique_ptr<labrador::UiTexture> background_;
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::vector<std::unique_ptr<labrador::UiText>> text_;
	std::unique_ptr<OptionRow> profile_row_;
	std::unique_ptr<OptionRow> name_row_;
	std::unique_ptr<OptionRow> boost_row_;
	std::unique_ptr<OptionRow> wearer_row_;
	labrador::UiText* delete_ = nullptr;
	labrador::UiText* new_ = nullptr;
	labrador::UiText* help_ = nullptr;
	labrador::UiText* status_ = nullptr;
	mattmath::RectangleF status_limit_;
};
