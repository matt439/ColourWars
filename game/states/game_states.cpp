#include "game/pch.h"
#include "game/states/game_states.h"
using namespace artattack;

GameMenu::GameMenu(GameData* game_data) : game_data_(game_data)
{

}

GameData* GameMenu::data() const
{
	return this->game_data_;
}

void GameMenu::init()
{
	this->menu_input_ = std::make_unique<MenuInput>(this->data()->gamepad());
	this->menu_level_settings_ = std::make_unique<MenuLevelSettings>();
	this->menu_context_ = std::make_unique<MenuContext>(this->data(),
		this->menu_input_.get(), this->menu_level_settings_.get());

	this->open_menu(MainMenuScreen::title);
}

// GameMenu never has a frame to itself: init() pushes a page, and every path
// that closes one pushes the next thing in the same drain. These two are what
// the base class demands, and the stack is what makes them empty.
void GameMenu::update(float /*dt*/)
{

}

void GameMenu::draw(Renderer& /*renderer*/) const
{

}

void GameMenu::open_menu(MainMenuScreen screen)
{
	// A button held on the way in - the A that confirmed the level's end menu,
	// say - must be released before it counts here.
	this->menu_input_->prime();

	// The page frame has one exit and it is the way out, so it closes rather
	// than answering: the settings it assembled are in the MenuContext this
	// state owns. That used to be a heap-allocated bool read every frame.
	this->context()->push(this->build_page(screen),
		[this]() { this->start_match(); });
}

std::unique_ptr<MainMenuPage> GameMenu::build_page(MainMenuScreen screen) const
{
	MenuContext* context = this->menu_context_.get();

	switch (screen)
	{
	case MainMenuScreen::home:
		return std::make_unique<MainMenuHome>(context);
	case MainMenuScreen::options:
		return std::make_unique<MainMenuOptions>(context);
	case MainMenuScreen::mode_select:
		return std::make_unique<MainMenuModeSelect>(context);
	case MainMenuScreen::player_count:
		return std::make_unique<MainMenuPlayerCount>(context);
	case MainMenuScreen::team_select:
		return std::make_unique<MainMenuTeamSelect>(context);
	case MainMenuScreen::weapon_select:
		return std::make_unique<MainMenuWeaponSelect>(context);
	case MainMenuScreen::stage_select:
		return std::make_unique<MainMenuStageSelect>(context);
	case MainMenuScreen::title:
	default:
		return std::make_unique<MainMenuTitle>(context);
	}
}

void GameMenu::start_match()
{
	// Above the menu, not instead of it. The menu is still there when the match
	// ends, which is why "change teams" is a screen to go back to rather than a
	// whole main menu rebuilt from a saved screen enum.
	this->context()->push<EndMenuAction>(
		std::make_unique<GameLevel>(this->data(), *this->menu_level_settings_),
		[this](const EndMenuAction& action) { this->on_match_ended(action); });
}

void GameMenu::on_match_ended(const EndMenuAction& action)
{
	switch (action)
	{
	case EndMenuAction::change_teams:
		this->open_menu(MainMenuScreen::team_select);
		break;
	case EndMenuAction::change_weapons:
		this->open_menu(MainMenuScreen::weapon_select);
		break;
	case EndMenuAction::change_level:
		this->open_menu(MainMenuScreen::stage_select);
		break;
	case EndMenuAction::exit:
		this->open_menu(MainMenuScreen::home);
		break;
	case EndMenuAction::restart:
		// A restart rebuilds the level in place and never leaves the match, so
		// this arm is unreachable through the end menu. It is here because the
		// enum has five values and a silent fifth case is how the missing
		// `break` in the switch this replaced went unnoticed.
		this->start_match();
		break;
	}
}

GameLevel::GameLevel(GameData* game_data, const MenuLevelSettings& settings) :
	game_data_(game_data), settings_(settings)
{

}

GameLevel::~GameLevel()
{
	this->data()->viewport_manager()->set_layout(this->previous_layout_);
}

GameData* GameLevel::data() const
{
	return this->game_data_;
}

void GameLevel::init()
{
	// Taken here and given back by the destructor. Whatever was in place is
	// what goes back, so a match started from a menu that was itself drawn
	// under some other layout leaves that layout alone.
	this->previous_layout_ = this->data()->viewport_manager()->layout();

	this->player_input_ = std::make_unique<PlayerInput>(
		this->data()->gamepad());

	this->menu_input_ = std::make_unique<MenuInput>(
		this->data()->gamepad());

	this->menu_context_ = std::make_unique<MenuContext>(this->data(),
		this->menu_input_.get(), &this->settings_);

	this->level_builder_ = std::make_unique<LevelBuilder>(
		this->data()->viewport_manager(),
		this->data()->render_resources(),
		this->data()->audio_resources(),
		this->data()->level_infos(),
		this->data()->resolution_manager(),
		this->data()->thread_pool(),
		this->data()->partitioner());

	this->build_and_enter_level();
}

// The single way into a level. LevelBuilder reads the current screen layout to
// build the split-screen dividers, so the layout must be set first - every
// restart path used to skip that and rebuild the level under whatever layout
// happened to be in place.
void GameLevel::build_and_enter_level()
{
	this->data()->viewport_manager()->
		set_layout(this->settings_.screen_layout());

	// Destroyed before its replacement is built, not after. Both resolve the
	// same music out of the shared sound bank, so a level dying *after* its
	// successor was constructed would stop the successor's track - which is the
	// hazard that comes with ~Level owning the music instead of seven
	// hand-written stop_music() calls.
	this->level_.reset();
	this->level_ = this->level_builder_->build_level(this->settings_);

	// Gameplay takes the input back. Whoever pressed A to start the match is
	// probably still holding it, and without this it lands as a jump on frame
	// one; the same call covers both restart paths.
	this->player_input_->prime();

	// Building a level is a blocking call worth many frames, and the step is
	// fixed: the next tick would otherwise pay the backlog off by running the
	// opening seconds of the match at full speed. Two GameLevelState values
	// existed to swallow the first frame and hope, which cost a frame and fixed
	// nothing past it.
	this->data()->application()->reset_elapsed_time();
}

void GameLevel::update(float dt)
{
	const std::vector<PlayerInputData> player_inputs =
		this->player_input_->update_and_get_player_inputs();

	const int pause_menu_player = this->check_for_pause_input(player_inputs);
	if (pause_menu_player != -1)
	{
		this->open_pause_menu(pause_menu_player);
		return;
	}

	// Reachable exactly once: this runs only while nothing is above the match,
	// and the results menu goes straight to the end menu, which either rebuilds
	// the level or leaves. The state machine that used to say so is gone.
	if (this->level_->state() == LevelState::finished)
	{
		this->open_results_menu();
		return;
	}

	this->level_->update(player_inputs, dt);
}

void GameLevel::draw(Renderer& renderer) const
{
	// Just the match. Whatever menu is over it draws itself, one frame above
	// this one, because MenuPage does not cover the screen (state.h).
	this->level_->draw(renderer);
}

void GameLevel::on_suspend()
{
	// A menu is taking the input, so the menu's edge detector has to start from
	// what the pads look like now: "previous" only advances on frames the menu
	// is read, so a button already held reads as a press the menu received.
	this->menu_input_->prime();

	// And the half that not-calling-update() could never do. Level::update is
	// the only thing that ever stops a looping weapon voice, so pausing used to
	// leave a player who paused mid-burst with a sustained tone running under
	// the menu, over music still playing at full volume.
	this->level_->suspend();
}

void GameLevel::on_resume()
{
	this->player_input_->prime();
	this->level_->resume();
}

void GameLevel::open_pause_menu(int player_num)
{
	this->context()->push<PauseMenuAction>(
		std::make_unique<PauseMenuInitial>(this->menu_context_.get(),
			player_num),
		[this](const PauseMenuAction& action)
		{
			this->on_pause_menu_closed(action);
		});
}

void GameLevel::on_pause_menu_closed(const PauseMenuAction& action)
{
	switch (action)
	{
	case PauseMenuAction::resume:
		// Nothing: on_resume() has already primed the input and let the mix
		// back up, which is exactly what resuming means.
		break;
	case PauseMenuAction::restart:
		this->build_and_enter_level();
		break;
	case PauseMenuAction::quit:
		this->leave_match(EndMenuAction::exit);
		break;
	}
}

void GameLevel::open_results_menu()
{
	this->context()->push(
		std::make_unique<ResultsMenuInitial>(this->menu_context_.get(),
			this->level_->level_end_info()),
		[this]() { this->open_end_menu(); });
}

void GameLevel::open_end_menu()
{
	this->context()->push<EndMenuAction>(
		std::make_unique<EndMenuInitial>(this->menu_context_.get()),
		[this](const EndMenuAction& action)
		{
			this->on_end_menu_closed(action);
		});
}

void GameLevel::on_end_menu_closed(const EndMenuAction& action)
{
	if (action == EndMenuAction::restart)
	{
		this->build_and_enter_level();
		return;
	}

	// Everything else is a screen of the main menu, and the main menu is the
	// frame underneath: it knows which screen each of these means.
	this->leave_match(action);
}

void GameLevel::leave_match(EndMenuAction action) const
{
	this->context()->pop(action);
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
