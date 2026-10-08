#pragma once

#include "engine/ui/widget.h"
#include "engine/render/text_encoding.h"
#include "game/objects/player_team.h"
#include "game/states/confirmation_state.h"
#include "game/states/menu_page.h"
#include "game/states/option_row.h"
#include "engine/core/state_context.h"
#include "engine/render/colour.h"


enum class MainMenuScreen
{
	title,
	home,
	options,
	mode_select,
	player_count,
	team_select,
	weapon_select,
	stage_select
};

class MainMenuPage : public MenuPage
{
public:
	explicit MainMenuPage(MenuContext* context);
	~MainMenuPage() override = default;
	void init() override = 0;
	void update(float dt) override = 0;
protected:
	const MainMenuPresentation& look() const;

	int player_count() const;

	// Available input slots, including keyboard/mouse.
	std::vector<int> connected_slots() const;

	// Return true while the roster is missing a controller. Keep each player's
	// row and offer reconnect or return to the count screen to choose again.
	bool wait_for_players(const std::vector<ProcessedMenuInput>& inputs);
	void set_player_prompts(const std::vector<PromptBar::Prompt>& prompts);

	// Family-specific sounds; the looping music is stopped before entering a match.
	labrador::SoundBank::WaveHandle error_sound_;
	labrador::SoundBank::WaveHandle ready_sound_;
	labrador::SoundBank::EffectHandle music_;
private:
	std::vector<PromptBar::Prompt> player_prompts_;
	bool waiting_for_players_ = false;
};

class MainMenuTitle final : public MainMenuPage
{
public:
	explicit MainMenuTitle(MenuContext* context);
	void update(float dt) override;
	void init() override;
private:
	std::unique_ptr<labrador::UiTextDropShadow> subtitle_;
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> title_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> start_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> build_id_ = nullptr;

	// Connection hint; the keyboard slot currently keeps it hidden.
	std::unique_ptr<labrador::UiTextDropShadow> no_pad_ = nullptr;
};

class MainMenuHome final : public MainMenuPage
{
public:
	explicit MainMenuHome(MenuContext* context);
	void update(float dt) override;
	void init() override;
private:
	std::unique_ptr<labrador::UiTextDropShadow> subtitle_;
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> heading_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> play_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> options_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> exit_ = nullptr;
};

class MainMenuOptions final : public MainMenuPage
{
public:
	explicit MainMenuOptions(MenuContext* context);
	void update(float dt) override;
	void init() override;
private:
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> heading_ = nullptr;

	std::unique_ptr<OptionRow> resolution_row_ = nullptr;
	std::unique_ptr<OptionRow> full_screen_row_ = nullptr;
	std::unique_ptr<OptionRow> team_palette_row_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> controls_ = nullptr;

	std::unique_ptr<labrador::UiTextDropShadow> apply_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> back_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> status_ = nullptr;
	labrador::ScreenResolution resolution_selection_ = labrador::ScreenResolution::s_1920_1080;
	bool full_screen_selection_ = true;
	TeamColourPair team_palette_selection_ = TeamColourPair::random;
	void cycle_resolution(labrador::Direction direction);
	void cycle_team_palette(labrador::Direction direction);
	void update_team_palette_selection_text();
	void adjust_focused_value(labrador::Direction direction);
	void apply_settings();
	void update_resolution_selection_text();

	// Shows the selected preset when windowed, or the live size in a disabled row
	// while fullscreen.
	void refresh_resolution_row();
	void update_full_screen_selection_text() const;
	void apply_fullscreen_setting(bool fullscreen);
};

class MainMenuModeSelect final : public MainMenuPage
{
public:
	explicit MainMenuModeSelect(MenuContext* context);
	void update(float dt) override;
	void init() override;
private:
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> heading_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> standard_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> best_of_three_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> best_of_five_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> practice_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> back_ = nullptr;
};

class MainMenuPlayerCount final : public MainMenuPage
{
public:
	explicit MainMenuPlayerCount(MenuContext* context);
	void update(float dt) override;
	void init() override;
private:
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> heading_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> _1_player = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> _2_players = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> _3_players = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> _4_players = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> back_ = nullptr;

	int activating_slot_ = -1;

	void start_join(int players, labrador::ScreenLayout layout);
};

class MainMenuJoin final : public MainMenuPage
{
public:
	explicit MainMenuJoin(MenuContext* context);
	void update(float dt) override;
	void init() override;
private:
	void refresh_players(const std::vector<ProcessedMenuInput>& inputs);
	std::unique_ptr<labrador::UiTexture> background_;
	std::unique_ptr<labrador::UiTextDropShadow> heading_;
	std::unique_ptr<labrador::UiTextDropShadow> status_;
	std::vector<std::unique_ptr<labrador::UiTextDropShadow>> players_;
};

class MainMenuTeamSelect final : public MainMenuPage
{
public:
	explicit MainMenuTeamSelect(MenuContext* context);
	void update(float dt) override;
	void init() override;
private:
	struct TeamSelectState
	{
		PlayerTeam team = PlayerTeam::none;
		ConfirmationState state = ConfirmationState::unconfirmed;
	};
	struct PlayerWidgets
	{
		std::unique_ptr<labrador::UiTextDropShadow> player = nullptr;
		std::unique_ptr<labrador::UiTexture> player_a = nullptr;
		std::unique_ptr<labrador::UiTexture> player_center = nullptr;
		std::unique_ptr<labrador::UiTexture> player_b = nullptr;
	};
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> heading_ = nullptr;

	// Shown when completing a multiplayer roster would leave a team empty.
	std::unique_ptr<labrador::UiTextDropShadow> both_teams_needed_ = nullptr;

	std::vector<std::unique_ptr<PlayerWidgets>> player_widgets_;
	std::vector<TeamSelectState> select_states_;
	void update_team_select_visuals();
	void deselect_and_unconfirm_all_widgets();
	bool all_players_confirmed() const;
	bool all_players_unconfirmed() const;

	// Whether this player is the last unconfirmed member of the roster.
	bool confirm_completes_roster(int player) const;

	// Shared predicate for confirmation and its validation message.
	bool roster_is_playable() const;

	// Counts chosen teams regardless of confirmation state.
	bool both_teams_chosen() const;

	void set_level_settings() const;
};

class MainMenuWeaponSelect final : public MainMenuPage
{
public:
	explicit MainMenuWeaponSelect(MenuContext* context);
	void update(float dt) override;
	void init() override;
private:
	struct SelectState
	{
		WeaponType type = WeaponType::sprayer;
		ConfirmationState state = ConfirmationState::unconfirmed;
	};
	struct Widgets
	{
		std::unique_ptr<labrador::UiTextDropShadow> player = nullptr;
		std::unique_ptr<labrador::UiTexture> weapon_icon = nullptr;
		std::unique_ptr<labrador::UiTextDropShadow> weapon_name = nullptr;
		std::unique_ptr<labrador::UiTextDropShadow> weapon_description = nullptr;
	};
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> heading_ = nullptr;
	std::vector<std::unique_ptr<Widgets>> player_widgets_;
	std::vector<SelectState> select_states_;
	void update_weapon_select_visuals();
	void unconfirm_all_widgets();
	bool all_players_confirmed() const;
	bool all_players_unconfirmed() const;
	static WeaponType random_weapon();
	void set_level_settings() const;
	void cycle_weapons(labrador::Direction direction, int player_index);
	std::wstring weapon_description(WeaponType type) const;
};

class MainMenuStageSelect final : public MainMenuPage
{
public:
	explicit MainMenuStageSelect(MenuContext* context);
	void update(float dt) override;
	void init() override;
private:
	// Stage slots are followed by Random, which is not a stage-list entry.
	struct SelectState
	{
		int slot = 0;
		ConfirmationState state = ConfirmationState::unconfirmed;
	};
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> heading_ = nullptr;
	std::unique_ptr<labrador::UiTexture> stage_icon_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> stage_name_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> ready_ = nullptr;
	SelectState select_state_ = SelectState();
	void update_stage_select_visuals();
	void unconfirm_all_widgets();
	// Returns the stage's slot, or the first slot for an empty or unknown name.
	int slot_of(const std::string& stage_name) const;
	// One past the final stage.
	int random_slot() const;
	// Presentation for stage and Random slots, shared by init and updates.
	std::string slot_icon_frame(int slot) const;
	std::wstring slot_display_name(int slot) const;
	// Chooses a random stage slot.
	int pick_random_stage() const;
	void cycle_stages(labrador::Direction direction);
	void set_level_settings() const;
};
