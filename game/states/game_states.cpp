#include "game/pch.h"
#include "game/states/game_states.h"

GameMenu::GameMenu(GameData* game_data) :
    game_data_(game_data),
    menu_data_(std::make_unique<MainMenuData>(game_data))
{
    this->menu_level_settings_ = std::make_unique<MenuLevelSettings>();
}
GameMenu::GameMenu(GameData* game_data,
    const MainMenuMidwayLoadSettings& settings) :
    game_data_(game_data),
    menu_data_(std::make_unique<MainMenuData>(game_data))
{
    this->menu_level_settings_ = std::make_unique<MenuLevelSettings>(
        settings.settings);
    this->screen_ = settings.screen;
}
void GameMenu::init()
{
    this->menu_input_ = std::make_unique<MenuInput>(
        this->menu_data_->get_gamepad());
    this->is_ready_to_load_level_ = std::make_unique<bool>(false);
    this->set_main_menu_data_ptrs();

    this->menu_ = std::make_unique<StateContext>();

    switch (this->screen_)
    {
    case MainMenuScreen::title:
        this->menu_->transition_to(std::make_unique<MainMenuTitle>(
            this->menu_data_.get()));
        break;
    case MainMenuScreen::home:
        this->menu_->transition_to(std::make_unique<MainMenuHome>(
            this->menu_data_.get()));
        break;
    case MainMenuScreen::options:
        this->menu_->transition_to(std::make_unique<MainMenuOptions>(
            this->menu_data_.get()));
        break;
    case MainMenuScreen::mode_select:
        this->menu_->transition_to(std::make_unique<MainMenuModeSelect>(
            this->menu_data_.get()));
        break;
    case MainMenuScreen::player_count:
        this->menu_->transition_to(std::make_unique<MainMenuPlayerCount>(
            this->menu_data_.get()));
        break;
    case MainMenuScreen::team_select:
        this->menu_->transition_to(std::make_unique<MainMenuTeamSelect>(
            this->menu_data_.get(), this->menu_level_settings_.get()));
        break;
    case MainMenuScreen::weapon_select:
        this->menu_->transition_to(std::make_unique<MainMenuWeaponSelect>(
            this->menu_data_.get(), this->menu_level_settings_.get()));
        break;
    case MainMenuScreen::stage_select:
        this->menu_->transition_to(std::make_unique<MainMenuStageSelect>(
            this->menu_data_.get(), this->menu_level_settings_.get()));
        break;
    }
}
void GameMenu::set_main_menu_data_ptrs()
{
    this->menu_data_->set_level_settings(this->menu_level_settings_.get());
    this->menu_data_->set_input(this->menu_input_.get());
    this->menu_data_->set_is_ready_to_load_level(
        this->is_ready_to_load_level_.get());
}
void GameMenu::update()
{
    this->menu_->update();
    if (*this->is_ready_to_load_level_)
    {
        this->order_level_creation();
    }
}
void GameMenu::order_level_creation()
{
    this->get_context()->transition_to(
        std::make_unique<GameLevel>(this->game_data_,
            *this->menu_data_->get_level_settings()));
}
void GameMenu::draw()
{
    this->menu_->draw();
}

GameData* GameMenu::get_data() const
{
    return this->game_data_;
}

GameLevel::GameLevel(GameData* game_data, const MenuLevelSettings& settings) :
    game_data_(game_data), settings_(settings)
{

}

GameData* GameLevel::get_data() const
{
    return this->game_data_;
}
void GameLevel::init()
{
    this->player_input_ = std::make_unique<PlayerInput>(
        this->get_data()->get_gamepad());

    this->menu_input_ = std::make_unique<MenuInput>(
        this->get_data()->get_gamepad());

    this->level_builder_ = std::make_unique<LevelBuilder>(
        this->get_data()->get_viewport_manager(),
        this->get_data()->get_dt(),
        this->get_data()->get_render_resources(),
        this->get_data()->get_audio_resources(),
        this->get_data()->get_level_infos(),
        this->get_data()->get_common_states()->PointClamp(),
        this->get_data()->get_resolution_manager(),
        this->get_data()->get_thread_pool(),
		this->get_data()->get_partitioner());

    this->build_and_enter_level();
}

// The single way into a level. LevelBuilder reads the current screen layout to
// build the split-screen dividers, so the layout must be set first - every
// restart path used to skip that and rebuild the level under whatever layout
// happened to be in place.
void GameLevel::build_and_enter_level()
{
    this->get_data()->get_viewport_manager()->
        set_layout(this->settings_.get_screen_layout());

    this->level_ = this->level_builder_->build_level(this->settings_);
}
void GameLevel::update()
{

    switch (this->state_)
    {
    case GameLevelState::first_update:
        this->state_ = GameLevelState::second_update;
        return;
    case GameLevelState::second_update:
        this->state_ = GameLevelState::active;
        //no break here so it goes to active
    case GameLevelState::active:
    {
        std::vector<PlayerInputData> player_inputs =
            this->player_input_->update_and_get_player_inputs();

        // check for pause menu press
        int pause_menu_player = this->check_for_pause_input(player_inputs);
        if (pause_menu_player != -1)
        {
            this->state_ = GameLevelState::pause_menu;

            this->pause_menu_action_ = std::make_unique<PauseMenuAction>(
                PauseMenuAction::none);

            this->pause_menu_data_ = std::make_unique<PauseMenuData>(
                this->get_data(),
                this->menu_input_.get(),
                pause_menu_player,
                this->pause_menu_action_.get());

            this->pause_menu_ = std::make_unique<StateContext>();
            this->pause_menu_->transition_to(
                std::make_unique<PauseMenuInitial>(
                    this->pause_menu_data_.get()));
        }
        else if (this->level_->get_state() == LevelState::finished)
        {
            LevelEndInfo end_info = this->level_->get_level_end_info();
            this->state_ = GameLevelState::results;

            this->results_menu_action_ = std::make_unique<ResultsMenuAction>(
                ResultsMenuAction::none);

            this->results_menu_data_ = std::make_unique<ResultsMenuData>(
                this->get_data(),
                this->menu_input_.get(),
                end_info,
                this->results_menu_action_.get());

            this->results_menu_ = std::make_unique<StateContext>();
            this->results_menu_->transition_to(
                std::make_unique<ResultsMenuInitial>(
                    this->results_menu_data_.get()));
        }
        else
        {
            this->level_->update(player_inputs);
        }
        break;
    }
    case GameLevelState::pause_menu:
    {
        this->pause_menu_->update();

        PauseMenuAction action = *this->pause_menu_action_;

        switch (action)
        {
        case PauseMenuAction::resume:
            this->state_ = GameLevelState::active;
            break;
        case PauseMenuAction::restart:
            this->level_->stop_music();
            this->build_and_enter_level();
            this->state_ = GameLevelState::first_update;
            break;
        case PauseMenuAction::quit:
            this->level_->stop_music();
            MainMenuMidwayLoadSettings settings;
            settings.settings = MenuLevelSettings();
            settings.screen = MainMenuScreen::home;
            this->get_context()->transition_to(
                std::make_unique<GameMenu>(this->get_data(), settings));
            break;
        }
        break;
    }
    case GameLevelState::results:
    {
        this->results_menu_->update();
        ResultsMenuAction action = *this->results_menu_action_;
        if (action == ResultsMenuAction::continue_to_end_menu)
        {
            this->state_ = GameLevelState::end_menu;

            this->end_menu_action_ = std::make_unique<EndMenuAction>(
                EndMenuAction::none);

            this->end_menu_data_ = std::make_unique<EndMenuData>(
                this->get_data(),
                this->menu_input_.get(),
                this->end_menu_action_.get());

            this->end_menu_ = std::make_unique<StateContext>();
            this->end_menu_->transition_to(
                std::make_unique<EndMenuInitial>(
                    this->end_menu_data_.get()));
        }
        break;
    }
    case GameLevelState::end_menu:
    {
        this->end_menu_->update();
        EndMenuAction action = *this->end_menu_action_;
        switch (action)
        {
        case EndMenuAction::change_teams:
        {
            this->level_->stop_music();
            MainMenuMidwayLoadSettings settings;
            settings.settings = this->settings_;
            settings.screen = MainMenuScreen::team_select;
            this->get_context()->transition_to(
                std::make_unique<GameMenu>(this->get_data(), settings));
            break;
        }
        case EndMenuAction::change_weapons:
        {
            this->level_->stop_music();
            MainMenuMidwayLoadSettings settings;
            settings.settings = this->settings_;
            settings.screen = MainMenuScreen::weapon_select;
            this->get_context()->transition_to(
                std::make_unique<GameMenu>(this->get_data(), settings));
            break;
        }
        case EndMenuAction::change_level:
        {
            this->level_->stop_music();
            MainMenuMidwayLoadSettings settings;
            settings.settings = this->settings_;
            settings.screen = MainMenuScreen::stage_select;
            this->get_context()->transition_to(
                std::make_unique<GameMenu>(this->get_data(), settings));
            break;
        }
        case EndMenuAction::restart:
        {
            this->level_->stop_music();
            this->build_and_enter_level();
            this->state_ = GameLevelState::first_update;
            break;
        }
        case EndMenuAction::exit:
        {
            this->level_->stop_music();
            MainMenuMidwayLoadSettings settings;
			settings.settings = MenuLevelSettings();
			settings.screen = MainMenuScreen::home;
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
	if (this->state_ == GameLevelState::first_update ||
		this->state_ == GameLevelState::second_update)
	{
		return;
	}

    auto deferred_contexts = this->get_data()->get_device_resources()->get_deferred_contexts();

    std::vector<ID3D11CommandList*> command_lists(deferred_contexts->size(), nullptr);

    this->level_->draw(this->get_data()->get_device_resources()->get_deferred_contexts(),
        &command_lists,
        this->get_data()->get_sprite_batches());

    auto immediate_context = this->get_data()->get_device_resources()->GetD3DDeviceContext();

    for (int i = 0; i < command_lists.size(); i++)
    {
        if (command_lists[i] == nullptr)
        {
            continue;
        }

        immediate_context->ExecuteCommandList(command_lists[i], TRUE);
        command_lists[i]->Release();
    }

    switch (this->state_)
    {
    case GameLevelState::active:
        break;
    case GameLevelState::pause_menu:
        this->pause_menu_->draw();
        break;
    case GameLevelState::results:
        this->results_menu_->draw();
        break;
    case GameLevelState::end_menu:
        this->end_menu_->draw();
        break;
    }
}

int GameLevel::check_for_pause_input(
    const std::vector<PlayerInputData>& player_inputs)
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