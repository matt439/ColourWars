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

class GameMenu final : public State
{
public:
	explicit GameMenu(GameData* game_data);
    GameMenu(GameData* game_data, const MainMenuMidwayLoadSettings& settings);
    void update() override;
    void draw() override;
    void init() override;

private:
    GameData* game_data_ = nullptr; // need this to pass to GameLevel
    std::unique_ptr<StateContext> menu_ = nullptr;
    std::unique_ptr<MenuInput> menu_input_ = nullptr;
    std::unique_ptr<MainMenuData> menu_data_ = nullptr;
    std::unique_ptr<MenuLevelSettings> menu_level_settings_ = nullptr;
    std::unique_ptr<bool> is_ready_to_load_level_ = nullptr;
    main_menu_screen screen_ = main_menu_screen::TITLE;
    void set_main_menu_data_ptrs();
    void order_level_creation();
    GameData* get_data() const;
};

enum class game_level_state
{
    FIRST_UPDATE,
    SECOND_UPDATE,
    ACTIVE,
    PAUSE_MENU,
    RESULTS,
    END_MENU,
};

class GameLevel final : public State
{
public:
    GameLevel(GameData* game_data, const MenuLevelSettings& settings);
    void update() override;
    void draw() override;
    void init() override;

private:
    GameData* game_data_ = nullptr;
    MenuLevelSettings settings_ = MenuLevelSettings();
    std::unique_ptr<Level> level_ = nullptr;
    std::unique_ptr<LevelBuilder> level_builder_ = nullptr;
    std::unique_ptr<PlayerInput> player_input_ = nullptr;
    std::unique_ptr<StateContext> pause_menu_ = nullptr;
    std::unique_ptr<pause_menu_action> pause_menu_action_ = nullptr;
    std::unique_ptr<PauseMenuData> pause_menu_data_ = nullptr;
    std::unique_ptr<MenuInput> menu_input_ = nullptr;
    std::unique_ptr<StateContext> results_menu_ = nullptr;
    std::unique_ptr<results_menu_action> results_menu_action_ = nullptr;
    std::unique_ptr<ResultsMenuData> results_menu_data_ = nullptr;
    std::unique_ptr<StateContext> end_menu_ = nullptr;
    std::unique_ptr<end_menu_action> end_menu_action_ = nullptr;
    std::unique_ptr<EndMenuData> end_menu_data_ = nullptr;
    game_level_state state_ = game_level_state::FIRST_UPDATE;
    // Sets the screen layout from the settings, then builds the level. Used by
    // init() and by every restart path, so they cannot drift apart.
    void build_and_enter_level();

    // Returns the XInput pad slot of the player who pressed pause, or -1.
    static int check_for_pause_input(
        const std::vector<PlayerInputData>& player_inputs);
    GameData* get_data() const;
};
