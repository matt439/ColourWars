#pragma once

#include "engine/ui/widget.h"
#include "engine/ui/focus.h"
#include "engine/render/text_encoding.h"
#include "game/objects/player_team.h"
#include "game/states/confirmation_state.h"
#include "game/states/menu_page.h"
#include "game/states/menu_panel.h"
#include "game/states/option_row.h"
#include "game/states/stage_preview.h"
#include "game/states/ui_player_badge.h"
#include "game/states/wrapped_text_drop_shadow.h"
#include "engine/core/state_context.h"
#include "engine/render/colour.h"

#include <optional>

enum class DisplayTrialResult;

enum class MainMenuScreen
{
	title,
	home,
	options,
	credits,
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

	// Main-menu frames start a match with the shared plan when they pop.
	// Options also serves as a paused child and guards that route separately.
	bool debug_start_match(const MenuLevelSettings& plan) override;
protected:
	void update_page(float dt) override = 0;
	const MainMenuPresentation& look() const;

	int player_count() const;

	// Available input slots, including keyboard/mouse.
	std::vector<int> connected_slots() const;

	// Return true while the roster is missing a controller. Keep each player's
	// row and offer reconnect or return to the count screen to choose again.
	bool wait_for_players(const std::vector<ProcessedMenuInput>& inputs);
	void set_player_prompts(MenuPrompt prompts);

	// Family-specific sounds; the looping music is stopped before entering a match.
	MenuSound error_sound_;
	MenuSound ready_sound_;
	labrador::SoundBank::WaveHandle unready_sound_;
private:
	MenuPrompt player_prompts_ = MenuPrompt::select_back;
	bool waiting_for_players_ = false;
};

class MainMenuTitle final : public MainMenuPage
{
public:
	explicit MainMenuTitle(MenuContext* context);
	void update_page(float dt) override;
	void init() override;
	bool shows_brand() const override { return false; }
	bool shows_focus_backing() const override { return false; }
private:
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::unique_ptr<labrador::UiText> subtitle_;
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<labrador::UiText> title_ = nullptr;
	std::unique_ptr<labrador::UiText> start_ = nullptr;
	std::unique_ptr<labrador::UiText> build_id_ = nullptr;

	// Connection hint; the keyboard slot currently keeps it hidden.
	std::unique_ptr<labrador::UiText> no_pad_ = nullptr;
};

class MainMenuHome final : public MainMenuPage
{
public:
	enum class InitialFocus { play, options, credits };
	explicit MainMenuHome(MenuContext* context, InitialFocus initial_focus = InitialFocus::play);
	void update_page(float dt) override;
	void init() override;
	bool shows_brand() const override { return false; }
	bool shows_focus_backing() const override { return false; }
private:
	InitialFocus initial_focus_;
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::unique_ptr<labrador::UiText> subtitle_;
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<labrador::UiText> heading_ = nullptr;
	std::unique_ptr<labrador::UiText> play_ = nullptr;
	std::unique_ptr<labrador::UiText> options_ = nullptr;
	std::unique_ptr<labrador::UiText> credits_ = nullptr;
	std::unique_ptr<labrador::UiText> exit_ = nullptr;
};

// A separate pushed frame keeps cancellation on the same Home page.
class MainMenuExitConfirmation final : public MenuPage
{
public:
	explicit MainMenuExitConfirmation(MenuContext* context);
	void init() override;
	void update_page(float dt) override;
	bool debug_dismiss(bool leaving_match) override;
protected:
	bool shows_brand() const override { return false; }
	bool shows_focus_backing() const override { return false; }
private:
	void cancel();
	PauseMenuPresentation look_;
	std::unique_ptr<labrador::UiTexture> background_;
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::unique_ptr<labrador::UiText> heading_;
	std::unique_ptr<labrador::UiText> question_;
	std::unique_ptr<labrador::UiText> yes_;
	std::unique_ptr<labrador::UiText> no_;
};

class MainMenuCredits final : public MainMenuPage
{
public:
	explicit MainMenuCredits(MenuContext* context);
	void update_page(float dt) override;
	void init() override;
	bool shows_brand() const override { return false; }
	bool shows_focus_backing() const override { return false; }
private:
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::unique_ptr<labrador::UiTexture> background_;
	std::unique_ptr<labrador::UiText> heading_;
	std::unique_ptr<labrador::UiText> body_;
	std::unique_ptr<labrador::UiText> back_;
	void return_home();
};

class MainMenuOptions final : public MainMenuPage
{
public:
	explicit MainMenuOptions(MenuContext* context,
		std::optional<int> pausing_slot = std::nullopt);
	void update_page(float dt) override;
	void init() override;
	bool covers_screen() const override { return this->pausing_slot_.has_value(); }
	bool debug_start_match(const MenuLevelSettings& plan) override;
	bool debug_dismiss(bool leaving_match) override;
	bool shows_brand() const override { return false; }
	bool shows_focus_backing() const override { return false; }
private:
	std::optional<int> pausing_slot_;
	MenuPaperArtPresentation options_art_;
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<labrador::UiText> heading_ = nullptr;
	std::unique_ptr<labrador::UiText> palette_timing_;

	std::unique_ptr<OptionRow> resolution_row_ = nullptr;
	std::unique_ptr<OptionRow> full_screen_row_ = nullptr;
	std::unique_ptr<OptionRow> team_palette_row_ = nullptr;
	std::unique_ptr<labrador::UiText> controls_ = nullptr;
	std::unique_ptr<labrador::UiText> restore_defaults_ = nullptr;

	std::unique_ptr<labrador::UiText> apply_ = nullptr;
	std::unique_ptr<labrador::UiText> back_ = nullptr;
	std::unique_ptr<labrador::UiText> status_ = nullptr;
	labrador::ScreenResolution resolution_selection_ = labrador::ScreenResolution::s_1920_1080;
	bool full_screen_selection_ = true;
	TeamColourPair team_palette_selection_ = TeamColourPair::random;
	void cycle_resolution(labrador::Direction direction);
	void cycle_team_palette(labrador::Direction direction);
	void update_team_palette_selection_text();
	void adjust_focused_value(labrador::Direction direction);
	void set_status(const std::wstring& text, const labrador::Colour& colour);
	void apply_settings();
	void finish_display_trial(DisplayTrialResult result);
	void restore_defaults();
	void close();
	void update_resolution_selection_text();

	// Shows the selected preset when windowed, or the live size in a disabled row
	// while fullscreen.
	void refresh_resolution_row();
	void update_full_screen_selection_text() const;
};

class MainMenuModeSelect final : public MainMenuPage
{
public:
	explicit MainMenuModeSelect(MenuContext* context);
	void update_page(float dt) override;
	void init() override;
	// Candidate validation uses the same unscaled row and complete ornament
	// as a real Mode page, after the ordinary resource checks have passed.
	static std::unique_ptr<OptionRow> make_duration_row(labrador::RenderResources* render,
		const MenuStyle& style, const MainMenuPresentation& look, const MenuText& text,
		OptionRow::Adjust on_adjust = {});
	static void check_duration_row(const OptionRow& duration_row,
		const mattmath::RectangleF& panel, const MenuPaperArtPresentation& art, float row_spacing);
	static void validate_durations(labrador::RenderResources* render,
		const GameTuning& tuning, const Presentation& presentation);
protected:
	bool shows_brand() const override { return false; }
	bool shows_focus_backing() const override { return false; }
private:
	MenuPaperArtPresentation setup_art_;
	void cycle_duration(labrador::Direction direction);
	void refresh_duration();
	std::unique_ptr<OptionRow> duration_row_;
	labrador::FontHandle duration_font_{};
	std::string duration_font_name_;
	float duration_row_spacing_ = 0.0f;
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<MenuPaperArtwork> paper_;
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
	void update_page(float dt) override;
	void init() override;
protected:
	bool shows_brand() const override { return false; }
	bool shows_focus_backing() const override { return false; }
private:
	MenuPaperArtPresentation setup_art_;
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<MenuPaperArtwork> paper_;
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
	void update_page(float dt) override;
	void init() override;
protected:
	bool shows_brand() const override { return false; }
	bool shows_focus_backing() const override { return false; }
private:
	MenuPaperArtPresentation setup_art_;
	struct JoinMotionRow
	{
		explicit JoinMotionRow(const MenuFocusMotionPresentation& look) : motion(look) {}
		MenuFocusMotion motion;
		int slot = -1;
		bool connected = false;
		std::unique_ptr<MenuJoinAccent> accent;
	};
	void refresh_players(const std::vector<ProcessedMenuInput>& inputs);
	mattmath::RectangleF check_paper_bounds(const labrador::UiWidget& widget,
		const char* role, const labrador::UiWidget* accent = nullptr) const;
	std::unique_ptr<labrador::UiTexture> background_;
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::unique_ptr<labrador::UiTextDropShadow> heading_;
	std::unique_ptr<labrador::UiTextDropShadow> status_;
	std::vector<std::unique_ptr<labrador::UiTextDropShadow>> players_;
	std::vector<std::unique_ptr<JoinMotionRow>> join_motion_;
};

class MainMenuTeamSelect final : public MainMenuPage
{
public:
	explicit MainMenuTeamSelect(MenuContext* context);
	void update_page(float dt) override;
	void init() override;
protected:
	bool shows_brand() const override { return false; }
	bool shows_focus_backing() const override { return false; }
private:
	MenuPaperArtPresentation setup_art_;
	MenuPaperArtPresentation row_art_;
	struct PlayerWidgets
	{
		explicit PlayerWidgets(const MenuFocusMotionPresentation& look) : motion(look) {}
		MenuFocusMotion motion;
		std::unique_ptr<MenuPaperArtwork> paper;
		std::unique_ptr<labrador::UiTextDropShadow> player = nullptr;
		std::unique_ptr<UiPlayerBadge> badge;
		std::unique_ptr<MenuChoiceIcon> player_a = nullptr;
		std::unique_ptr<MenuChoiceIcon> player_center = nullptr;
		std::unique_ptr<MenuChoiceIcon> player_b = nullptr;
	};
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::unique_ptr<labrador::UiTextDropShadow> heading_ = nullptr;

	// Shown when completing a multiplayer roster would leave a team empty.
	std::unique_ptr<labrador::UiTextDropShadow> both_teams_needed_ = nullptr;

	std::vector<std::unique_ptr<PlayerWidgets>> player_widgets_;
	// Roster seats are cursor slots, independent of each seat's input device.
	// Declared after its borrowed icons so this group is destroyed first.
	labrador::FocusGroup team_focus_;
	std::vector<ConfirmationState> confirmations_;
	// Pair frame names with the atlas captured by this page's UiTextures.
	// A presentation rebase reaches newly opened pages, never this pairing.
	std::array<std::string, 3> team_select_frames_;
	std::array<std::string, 3> team_select_selected_frames_;
	void shuffle_teams();
	void update_team_select_visuals();
	void deselect_and_unconfirm_all_widgets();
	MenuChoiceIcon* team_widget(int player, PlayerTeam team) const;
	PlayerTeam selected_team(int player) const;
	bool all_players_confirmed() const;
	bool all_players_unconfirmed() const;

	// Whether this player is the last unconfirmed member of the roster.
	bool confirm_completes_roster(int player) const;

	// Shared predicate for confirmation and its validation message.
	bool roster_is_playable() const;

	// Counts chosen teams regardless of confirmation state.
	bool both_teams_chosen() const;

	void set_level_settings() const;
	void check_paper_geometry() const;
};

class MainMenuWeaponSelect final : public MainMenuPage
{
public:
	explicit MainMenuWeaponSelect(MenuContext* context);
	void update_page(float dt) override;
	void init() override;
protected:
	bool shows_brand() const override { return false; }
	bool shows_focus_backing() const override { return false; }
private:
	MenuPaperArtPresentation paper_art_;
	MenuPaperArtPresentation icon_art_;
	struct SelectState
	{
		WeaponType type = WeaponType::sprayer;
		ConfirmationState state = ConfirmationState::unconfirmed;
	};
	struct Widgets
	{
		explicit Widgets(const MenuFocusMotionPresentation& look) : motion(look) {}
		MenuFocusMotion motion;
		std::unique_ptr<labrador::UiTextDropShadow> player = nullptr;
		std::unique_ptr<UiPlayerBadge> badge;
		std::unique_ptr<MenuIcon> weapon_icon = nullptr;
		std::unique_ptr<labrador::UiTextDropShadow> weapon_name = nullptr;
		std::unique_ptr<WrappedTextDropShadow> weapon_description = nullptr;
	};
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::unique_ptr<labrador::UiTextDropShadow> heading_ = nullptr;
	std::vector<std::unique_ptr<Widgets>> player_widgets_;
	std::vector<SelectState> select_states_;
	// Keep names, frames and descriptions paired with the atlas captured on entry.
	WeaponSelectArtworkPresentation weapon_select_art_;
	std::array<std::wstring, MainMenuPresentation::DESCRIBED_WEAPONS> weapon_descriptions_;
	void update_weapon_select_visuals();
	void unconfirm_all_widgets();
	bool all_players_confirmed() const;
	bool all_players_unconfirmed() const;
	static WeaponType random_weapon();
	void set_level_settings() const;
	void cycle_weapons(labrador::Direction direction, int player_index);
	std::wstring weapon_description(WeaponType type) const;
	void check_paper_geometry() const;
};

class MainMenuStageSelect final : public MainMenuPage
{
public:
	explicit MainMenuStageSelect(MenuContext* context);
	void update_page(float dt) override;
	void init() override;
protected:
	bool shows_brand() const override { return false; }
	bool shows_focus_backing() const override { return false; }
private:
	MenuPaperArtPresentation paper_art_;
	MenuPaperArtPresentation briefing_art_;
	mattmath::RectangleF briefing_layout_bounds_;
	float briefing_accent_motion_height_ = 0.0f;
	MenuFocusMotion stage_motion_;
	// Stage slots are followed by Random, which is not a stage-list entry.
	struct SelectState
	{
		int slot = 0;
		ConfirmationState state = ConfirmationState::unconfirmed;
	};
	std::unique_ptr<labrador::UiTexture> background_ = nullptr;
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::unique_ptr<labrador::UiTextDropShadow> heading_ = nullptr;
	std::unique_ptr<StagePreview> stage_preview_;
	std::unique_ptr<labrador::UiTexture> preview_edge_;
	std::unique_ptr<labrador::UiTexture> preview_background_;
	std::unique_ptr<MenuPaperArtwork> briefing_background_;
	std::unique_ptr<labrador::UiTexture> briefing_accent_;
	std::unique_ptr<labrador::UiTextDropShadow> stage_name_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> navigation_;
	// The shown stage's saved record; hidden for Random and unplayed stages.
	std::unique_ptr<labrador::UiTextDropShadow> stage_record_;
	std::unique_ptr<labrador::UiTextDropShadow> selection_status_;
	std::unique_ptr<labrador::UiTextDropShadow> selection_hint_;
	std::unique_ptr<labrador::UiTextDropShadow> random_mark_;
	std::vector<std::unique_ptr<labrador::UiTextDropShadow>> briefing_text_;
	SelectState select_state_ = SelectState();
	int displayed_slot_ = -1;
	ConfirmationState displayed_state_ = ConfirmationState::unconfirmed;
	void update_stage_select_visuals();
	// The record line for this roster's kind of match on `slot`, or empty.
	std::wstring record_line(int slot) const;
	void refresh_briefing_paper();
	void check_paper_geometry() const;
	// Returns the stage's slot, or the first slot for an empty or unknown name.
	int slot_of(const std::string& stage_name) const;
	// One past the final stage.
	int random_slot() const;
	// Presentation for stage and Random slots, shared by init and updates.
	std::wstring slot_display_name(int slot) const;
	// Chooses a random stage slot.
	int pick_random_stage() const;
	void cycle_stages(labrador::Direction direction);
	void set_level_settings() const;
};
