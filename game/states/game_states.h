#pragma once

#include "engine/core/state.h"
#include "game/session.h"
#include "game/states/menu_level_settings.h"
#include "game/game_data.h"
#include "game/objects/level_builder.h"
#include "game/states/pause_menu.h"
#include "game/states/results_menu.h"
#include "game/states/main_menu.h"
#include "game/states/end_menu.h"
#include "game/objects/player_input.h"

// Stack shapes during a session:
//
//     [GameMenu, MainMenu*Page]
//     [GameMenu, MainMenuOptions, ControlsSettingsMenu]
//     [GameMenu, GameLevel]
//     [GameMenu, GameLevel, PauseMenu*Page | ResultsMenuInitial | EndMenuInitial]
//     [GameMenu, GameLevel, PauseMenuInitial, ControlsSettingsMenu]
//
// GameMenu is alone only during transitions. Its context and settings survive
// matches; menu pages are rebuilt when returning from a match.

// Owns shared menu state and routes between setup pages and matches.
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

	void open_menu(MainMenuScreen screen);
	std::unique_ptr<MainMenuPage> build_page(MainMenuScreen screen) const;

	void start_match();
	void on_match_ended(const EndMenuAction& action);
	GameData* data() const;
};

// A match or series, with a fresh Level for each round.
class GameLevel final : public labrador::State
{
public:
	GameLevel(GameData* game_data, const MenuLevelSettings& settings);

	// Restores the screen layout that was active before the match.
	~GameLevel() override;

	void update(float dt) override;
	void draw(labrador::Renderer& renderer) const override;
	void init() override;

	// Stack coverage stops simulation updates; these callbacks suspend and
	// resume the audio that continues independently.
	void on_suspend() override;
	void on_resume() override;

	// Focus and stack coverage are independent reasons to suspend audio.
	// Regaining either resumes only when the other also permits playback.
	void on_activated() override;
	void on_deactivated() override;

private:
	GameData* game_data_ = nullptr;
	MenuLevelSettings settings_ = MenuLevelSettings();
	Session session_;
	std::unique_ptr<Level> level_ = nullptr;
	std::unique_ptr<LevelBuilder> level_builder_ = nullptr;
	std::unique_ptr<PlayerInput> player_input_ = nullptr;
	std::unique_ptr<MenuInput> menu_input_ = nullptr;
	bool covered_ = false;

	// Shared context for menus above the match; settings_ outlives those pages.
	std::unique_ptr<MenuContext> menu_context_ = nullptr;

	// Restored by the destructor.
	labrador::ScreenLayout previous_layout_ = labrador::ScreenLayout::one_player;

	// Sets the layout before building a level, including every restart.
	void build_and_enter_level();

	void open_pause_menu(int player_num);
	void on_pause_menu_closed(const PauseMenuAction& action);
	void open_results_menu();
	void open_end_menu();
	void on_end_menu_closed(const EndMenuAction& action);

	void leave_match(EndMenuAction action) const;

	// Returns the input slot of a participant requesting pause, or -1.
	// Unclaimed devices cannot open the pause menu.
	int check_for_pause_input(
		const std::vector<PlayerInputData>& player_inputs) const;
	GameData* data() const;
};
