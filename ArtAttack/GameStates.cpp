#include "pch.h"
#include "GameStates.h"

GameMenu::GameMenu(GameData* game_data) :
    _game_data(game_data),
    _menu_data(std::make_unique<MainMenuData>(game_data))
{
    this->_menu_level_settings = std::make_unique<MenuLevelSettings>();
}
GameMenu::GameMenu(GameData* game_data,
    const MainMenuMidwayLoadSettings& settings) :
    _game_data(game_data),
    _menu_data(std::make_unique<MainMenuData>(game_data))
{
    this->_menu_level_settings = std::make_unique<MenuLevelSettings>(
        settings.settings);
    this->_screen = settings.screen;
}
void GameMenu::init()
{
    this->_menu_input = std::make_unique<MenuInput>(
        this->_menu_data->get_gamepad());
    this->_is_ready_to_load_level = std::make_unique<bool>(false);
    this->set_main_menu_data_ptrs();

    this->_menu = std::make_unique<StateContext>();

    switch (this->_screen)
    {
    case main_menu_screen::TITLE:
        this->_menu->transition_to(std::make_unique<MainMenuTitle>(
            this->_menu_data.get()));
        break;
    case main_menu_screen::HOME:
        this->_menu->transition_to(std::make_unique<MainMenuHome>(
            this->_menu_data.get()));
        break;
    case main_menu_screen::OPTIONS:
        this->_menu->transition_to(std::make_unique<MainMenuOptions>(
            this->_menu_data.get()));
        break;
    case main_menu_screen::MODE_SELECT:
        this->_menu->transition_to(std::make_unique<MainMenuModeSelect>(
            this->_menu_data.get()));
        break;
    case main_menu_screen::PLAYER_COUNT:
        this->_menu->transition_to(std::make_unique<MainMenuPlayerCount>(
            this->_menu_data.get()));
        break;
    case main_menu_screen::TEAM_SELECT:
        this->_menu->transition_to(std::make_unique<MainMenuTeamSelect>(
            this->_menu_data.get(), this->_menu_level_settings.get()));
        break;
    case main_menu_screen::WEAPON_SELECT:
        this->_menu->transition_to(std::make_unique<MainMenuWeaponSelect>(
            this->_menu_data.get(), this->_menu_level_settings.get()));
        break;
    case main_menu_screen::STAGE_SELECT:
        this->_menu->transition_to(std::make_unique<MainMenuStageSelect>(
            this->_menu_data.get(), this->_menu_level_settings.get()));
        break;
    }
}
void GameMenu::set_main_menu_data_ptrs()
{
    this->_menu_data->set_level_settings(this->_menu_level_settings.get());
    this->_menu_data->set_input(this->_menu_input.get());
    this->_menu_data->set_is_ready_to_load_level(
        this->_is_ready_to_load_level.get());
}
void GameMenu::update()
{
    this->_menu->update();
    if (*this->_is_ready_to_load_level)
    {
        this->order_level_creation();
    }
}
void GameMenu::order_level_creation()
{
    this->get_context()->transition_to(
        std::make_unique<GameLevel>(this->_game_data,
            *this->_menu_data->get_level_settings()));
}
void GameMenu::draw()
{
    this->_menu->draw();
}

GameData* GameLevel::get_data() const
{
    return this->_game_data;
}
void GameLevel::init()
{
    this->_player_input = std::make_unique<PlayerInput>(
        this->get_data()->get_gamepad());
    this->_menu_input = std::make_unique<MenuInput>(
        this->get_data()->get_gamepad());

    this->_level_builder = std::make_unique<LevelBuilder>(
        this->get_data()->get_viewport_manager(),
        this->get_data()->get_dt(),
        this->get_data()->get_sprite_batch(),
        this->get_data()->get_resource_manager(),
        this->get_data()->get_common_states()->PointClamp(),
        this->get_data()->get_resolution_manager());

    this->get_data()->get_viewport_manager()->
        set_layout(this->_settings.get_screen_layout());

    this->_level = std::move(this->_level_builder->build_level(
        this->_settings));
}
void GameLevel::update()
{

    switch (this->_state)
    {
    case game_level_state::FIRST_UPDATE:
        this->_state = game_level_state::SECOND_UPDATE;
        return;
    case game_level_state::SECOND_UPDATE:
        this->_state = game_level_state::ACTIVE;
        //no break here so it goes to active
    case game_level_state::ACTIVE:
    {
        std::vector<player_input> player_inputs =
            this->_player_input->update_and_get_player_inputs();

        // check for pause menu press
        int pause_menu_player = this->check_for_pause_input(player_inputs);
        if (pause_menu_player != -1)
        {
            this->_state = game_level_state::PAUSE_MENU;

            this->_pause_menu_action = std::make_unique<pause_menu_action>(
                pause_menu_action::NONE);

            this->_pause_menu_data = std::make_unique<PauseMenuData>(
                this->get_data(),
                this->_menu_input.get(),
                pause_menu_player,
                this->_pause_menu_action.get());

            this->_pause_menu = std::make_unique<StateContext>();
            this->_pause_menu->transition_to(
                std::make_unique<PauseMenuInitial>(
                    this->_pause_menu_data.get()));
        }
        else if (this->_level->get_state() == level_state::FINISHED)
        {
            LevelEndInfo end_info = this->_level->get_level_end_info();
            this->_state = game_level_state::RESULTS;

            this->_results_menu_action = std::make_unique<results_menu_action>(
                results_menu_action::NONE);

            this->_results_menu_data = std::make_unique<ResultsMenuData>(
                this->get_data(),
                this->_menu_input.get(),
                end_info,
                this->_results_menu_action.get());

            this->_results_menu = std::make_unique<StateContext>();
            this->_results_menu->transition_to(
                std::make_unique<ResultsMenuInitial>(
                    this->_results_menu_data.get()));
        }
        else
        {
            this->_level->update(player_inputs);
        }
        break;
    }
    case game_level_state::PAUSE_MENU:
    {
        this->_pause_menu->update();

        pause_menu_action action = *this->_pause_menu_action;

        switch (action)
        {
        case pause_menu_action::RESUME:
            this->_state = game_level_state::ACTIVE;
            break;
        case pause_menu_action::RESTART:
            this->_level->stop_music();
            this->_level = std::move(this->_level_builder->build_level(
                this->_settings));
            this->_state = game_level_state::FIRST_UPDATE;
            break;
        case pause_menu_action::QUIT:
            this->_level->stop_music();
            MainMenuMidwayLoadSettings settings;
            settings.settings = MenuLevelSettings();
            settings.screen = main_menu_screen::HOME;
            this->get_context()->transition_to(
                std::make_unique<GameMenu>(this->get_data(), settings));
            break;
        }
        break;
    }
    case game_level_state::RESULTS:
    {
        this->_results_menu->update();
        results_menu_action action = *this->_results_menu_action;
        if (action == results_menu_action::CONTINUE_TO_END_MENU)
        {
            this->_state = game_level_state::END_MENU;

            this->_end_menu_action = std::make_unique<end_menu_action>(
                end_menu_action::NONE);

            this->_end_menu_data = std::make_unique<EndMenuData>(
                this->get_data(),
                this->_menu_input.get(),
                this->_end_menu_action.get());

            this->_end_menu = std::make_unique<StateContext>();
            this->_end_menu->transition_to(
                std::make_unique<EndMenuInitial>(
                    this->_end_menu_data.get()));
        }
        break;
    }
    case game_level_state::END_MENU:
    {
        this->_end_menu->update();
        end_menu_action action = *this->_end_menu_action;
        switch (action)
        {
        case end_menu_action::CHANGE_TEAMS:
        {
            this->_level->stop_music();
            MainMenuMidwayLoadSettings settings;
            settings.settings = this->_settings;
            settings.screen = main_menu_screen::TEAM_SELECT;
            this->get_context()->transition_to(
                std::make_unique<GameMenu>(this->get_data(), settings));
            break;
        }
        case end_menu_action::CHANGE_WEAPONS:
        {
            this->_level->stop_music();
            MainMenuMidwayLoadSettings settings;
            settings.settings = this->_settings;
            settings.screen = main_menu_screen::WEAPON_SELECT;
            this->get_context()->transition_to(
                std::make_unique<GameMenu>(this->get_data(), settings));
            break;
        }
        case end_menu_action::CHANGE_LEVEL:
        {
            this->_level->stop_music();
            MainMenuMidwayLoadSettings settings;
            settings.settings = this->_settings;
            settings.screen = main_menu_screen::STAGE_SELECT;
            this->get_context()->transition_to(
                std::make_unique<GameMenu>(this->get_data(), settings));
            break;
        }
        case end_menu_action::RESTART:
        {
            this->_level->stop_music();
            this->_level = std::move(this->_level_builder->build_level(
                this->_settings));
            this->_state = game_level_state::FIRST_UPDATE;
            break;
        }
        case end_menu_action::EXIT:
        {
            this->_level->stop_music();
            MainMenuMidwayLoadSettings settings;
			settings.settings = MenuLevelSettings();
			settings.screen = main_menu_screen::HOME;
			this->get_context()->transition_to(
				std::make_unique<GameMenu>(this->get_data(), settings));
			break;
        }
        default:
			break;
        }
    }
    default:
		break;
    }
}

void GameLevel::draw()
{
    switch (this->_state)
    {
    case game_level_state::FIRST_UPDATE:
        break;
    case game_level_state::SECOND_UPDATE:
        break;
    case game_level_state::ACTIVE:
        this->_level->draw();
        break;
    case game_level_state::PAUSE_MENU:
        this->_level->draw();
        this->_pause_menu->draw();
        break;
    case game_level_state::RESULTS:
        this->_level->draw();
        this->_results_menu->draw();
        break;
    case game_level_state::END_MENU:
        this->_level->draw();
        this->_end_menu->draw();
        break;
    }
}

int GameLevel::check_for_pause_input(
    const std::vector<player_input>& player_inputs)
{
    for (int i = 0; i < static_cast<int>(player_inputs.size()); i++)
    {
        if (player_inputs[i].toggle_pause_menu)
        {
            return i;
        }
    }
    return -1;
}