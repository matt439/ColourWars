#pragma once

#include "engine/core/state.h"
#include "game/states/menu_level_settings.h"
#include "game/game_data.h"
#include "game/objects/level_builder.h"
#include "game/states/pause_menu.h"
#include "game/states/results_menu.h"
#include "game/states/main_menu.h"
#include "game/states/end_menu.h"
#include "game/objects/player_input.h"

// The two states this game is made of, and the flow between them is now the
// engine's stack rather than four nested StateContexts and an enum.
//
// The stack, in full, at every point in a session:
//
//     [GameMenu]                                     - never on its own for a
//     [GameMenu, MainMenu*Page]                        whole frame; it pushes
//     [GameMenu, GameLevel]                            a page in init()
//     [GameMenu, GameLevel, PauseMenu*Page]
//     [GameMenu, GameLevel, ResultsMenuInitial]
//     [GameMenu, GameLevel, EndMenuInitial]
//
// GameMenu is at the bottom for the whole session and owns what every main
// menu page shares. The match is pushed *above* it rather than in place of it,
// which is what makes "change teams" a return to a screen that still exists
// rather than a reconstruction of the whole menu from a saved screen enum -
// MainMenuMidwayLoadSettings is what that used to cost.

// The main menu, and the owner of everything its pages share.
//
// It draws nothing and updates nothing: there is always a page above it. What
// it is for is ownership and flow - the pages come and go, the settings they
// are assembling do not.
class GameMenu final : public labrador::State
{
public:
	explicit GameMenu(GameData* game_data);
	void update(float dt) override;
	void draw(labrador::Renderer& renderer) const override;
	void init() override;

private:
	GameData* game_data_ = nullptr;
	std::unique_ptr<MenuInput> menu_input_ = nullptr;
	std::unique_ptr<MenuLevelSettings> menu_level_settings_ = nullptr;
	std::unique_ptr<MenuContext> menu_context_ = nullptr;

	// Opens the main menu at a screen. Called once from init() and once per
	// match that ends, with whatever screen the end menu asked to go back to.
	void open_menu(MainMenuScreen screen);
	std::unique_ptr<MainMenuPage> build_page(MainMenuScreen screen) const;

	void start_match();
	void on_match_ended(const EndMenuAction& action);
	GameData* data() const;
};

// One match, from the countdown to the end menu.
class GameLevel final : public labrador::State
{
public:
	GameLevel(GameData* game_data, const MenuLevelSettings& settings);

	// Puts back the screen layout it took. This is the fix for the main menu
	// being drawn four times into quarter-screen viewports after a four-player
	// match: the layout was set on the way in by one line and put back by none,
	// because five separate exits each had to remember. A thing that is taken
	// and given back wants an owner, and the owner is the state that took it.
	~GameLevel() override;

	void update(float dt) override;
	void draw(labrador::Renderer& renderer) const override;
	void init() override;

	// A menu went up over the match. The simulation stopping is what not being
	// the top of the stack already does; these two are for everything that
	// does not live in update() - which, as the shipped game demonstrated, is
	// the entire audio mix.
	void on_suspend() override;
	void on_resume() override;

private:
	GameData* game_data_ = nullptr;
	MenuLevelSettings settings_ = MenuLevelSettings();
	std::unique_ptr<Level> level_ = nullptr;
	std::unique_ptr<LevelBuilder> level_builder_ = nullptr;
	std::unique_ptr<PlayerInput> player_input_ = nullptr;
	std::unique_ptr<MenuInput> menu_input_ = nullptr;

	// What the three menus this state puts over the match share. They are all
	// built from the settings this match was built from, so it is the same
	// composed context the main menu uses.
	std::unique_ptr<MenuContext> menu_context_ = nullptr;

	// The layout that was in place before this match took it, restored by the
	// destructor.
	labrador::ScreenLayout previous_layout_ = labrador::ScreenLayout::one_player;

	// Whether the window had the foreground when update() last looked.
	//
	// Remembered rather than tested, because the match is suspended on the edge
	// into false and resumed on the edge back, and those two are not idempotent
	// in the way calling them every frame would need: pause_effect and
	// resume_effect carry no depth, so "suspended twice" and "suspended" are
	// the same state and one resume answers both.
	//
	// Polled rather than notified. The shell handles WM_ACTIVATEAPP privately
	// and forwards nothing to the state stack, which is the report filed
	// against the engine; when a State::on_deactivated exists this member is
	// what it sets, and the poll in update() is the only part that goes.
	bool window_active_ = true;

	// Follows the window's foreground state, suspending or resuming the match
	// on the edge. Returns whether the match should tick at all this frame.
	bool match_has_the_foreground();

	// Sets the screen layout from the settings, then builds the level. Used by
	// init() and by every restart path, so they cannot drift apart.
	void build_and_enter_level();

	void open_pause_menu(int player_num);
	void on_pause_menu_closed(const PauseMenuAction& action);
	void open_results_menu();
	void open_end_menu();
	void on_end_menu_closed(const EndMenuAction& action);

	// Hands the match back to the menu underneath, with what the player chose.
	void leave_match(EndMenuAction action) const;

	// Returns the XInput pad slot of the player who pressed pause, or -1.
	//
	// Only slots this match is being played on are asked, which is why it needs
	// the settings and cannot be static: player_inputs carries an entry for
	// every connected pad whether or not a player is on it, and a spare
	// controller down the back of the sofa is not one of the people playing.
	// Letting one pause was letting it own the menu (pause_menu.h) and walk
	// Quit -> Yes on everybody else's behalf.
	int check_for_pause_input(
		const std::vector<PlayerInputData>& player_inputs) const;
	GameData* data() const;
};
