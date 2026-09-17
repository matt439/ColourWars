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
	// This menu family's own presentation, beside MenuPage::menu_style()'s
	// shared one. Named for what a reader is looking for rather than for the
	// type it returns: `this->look().play_background_colour` at the call site.
	const MainMenuPresentation& look() const;

	int player_count() const;

	// Available devices, used only when choosing a new roster.
	std::vector<int> connected_slots() const;

	// Return true while the roster is missing a controller. Keep each player's
	// row and offer reconnect or return to the count screen to choose again.
	bool wait_for_players(const std::vector<ProcessedMenuInput>& inputs);
	void set_player_prompts(const std::vector<PromptBar::Prompt>& prompts);

	// The sounds only this family makes, beside the cursor, confirm and cancel
	// every page shares from MenuPage. The music is an effect rather than a
	// wave because it loops and is stopped on the way into a match.
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
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> title_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> start_ = nullptr;

	// Shown only while no pad is connected. Its visibility is refreshed every
	// frame rather than set once, because plugging one in is exactly what this
	// line is asking for and the page must notice when it happens.
	std::unique_ptr<labrador::UiTextDropShadow> no_pad_ = nullptr;
};

class MainMenuHome final : public MainMenuPage
{
public:
	explicit MainMenuHome(MenuContext* context);
	void update(float dt) override;
	void init() override;
private:
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

	// A label and its value, as one focusable thing each. They were four
	// loose widgets and a chain of pointer comparisons until the engine let a
	// container be focused.
	std::unique_ptr<OptionRow> resolution_row_ = nullptr;
	std::unique_ptr<OptionRow> full_screen_row_ = nullptr;
	std::unique_ptr<OptionRow> team_palette_row_ = nullptr;

	std::unique_ptr<labrador::UiTextDropShadow> apply_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> back_ = nullptr;
	labrador::ScreenResolution resolution_selection_ = labrador::ScreenResolution::s_1920_1080;
	bool full_screen_selection_ = true;
	TeamColourPair team_palette_selection_ = TeamColourPair::random;
	void cycle_resolution(labrador::Direction direction);
	void cycle_team_palette(labrador::Direction direction);
	void update_team_palette_selection_text();
	// Left and right on whichever row carries a value. The focus group has no
	// opinion about which rows those are: that is this game's content, and it
	// is now the row's own rather than a list kept here.
	void adjust_focused_value(labrador::Direction direction);
	void apply_settings();
	void update_resolution_selection_text();

	// Points the Resolution row at whichever of the two questions currently
	// applies - the preset being chosen, or the size fullscreen actually took -
	// and switches the row off while it is answering the second.
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
	std::unique_ptr<labrador::UiTextDropShadow> tdm_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> dm_ = nullptr;
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

	// Shown when the confirm that would have completed the roster would also
	// have left one of the two teams empty.
	std::unique_ptr<labrador::UiTextDropShadow> both_teams_needed_ = nullptr;

	std::vector<std::unique_ptr<PlayerWidgets>> player_widgets_;
	std::vector<TeamSelectState> select_states_;
	void update_team_select_visuals();
	void deselect_and_unconfirm_all_widgets();
	bool all_players_confirmed() const;
	bool all_players_unconfirmed() const;

	// Whether `player` is the last one still to confirm, and so whether their
	// press is the one that starts the match.
	bool confirm_completes_roster(int player) const;

	// Whether this roster could be a match. Both the refusal and the message
	// that explains it read this one predicate, so they cannot disagree.
	bool roster_is_playable() const;

	// Whether the teams as currently *chosen* - confirmed or not - have
	// somebody on each side. The team is picked before it is confirmed, so
	// this already includes the player who is pressing.
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
	// Not static any more: the descriptions are content, so they come off
	// this page's presentation rather than out of six constants in a header.
	std::wstring weapon_description(WeaponType type) const;
};

class MainMenuStageSelect final : public MainMenuPage
{
public:
	explicit MainMenuStageSelect(MenuContext* context);
	void update(float dt) override;
	void init() override;
private:
	// The cursor runs over every stage the manifest named and then one slot
	// past the end, which is Random - a menu affordance rather than a stage,
	// which is why it is a position here and not an entry in the list.
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
	// Where a stage name sits in the list. An empty or unrecognised name is
	// the first stage, which is what makes "no choice yet" mean "the first
	// one" without a default spelt anywhere.
	int slot_of(const std::string& stage_name) const;
	// The slot the Random option sits in - one past the last stage.
	int random_slot() const;
	// What a slot shows: the sprite frame and the label beneath it. They are
	// helpers rather than a block inside update_stage_select_visuals because
	// init() has to build the icon with a frame that is really in the sheet -
	// the engine resolves a frame name eagerly, and one that is not there
	// throws before the page has finished being constructed.
	std::string slot_icon_frame(int slot) const;
	std::wstring slot_display_name(int slot) const;
	// A real stage's slot, chosen uniformly.
	int pick_random_stage() const;
	void cycle_stages(labrador::Direction direction);
	void set_level_settings() const;
};
