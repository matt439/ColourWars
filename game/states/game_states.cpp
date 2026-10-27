#include "game/pch.h"
#include "game/states/game_states.h"
#include "game/last_match_setup.h"
using namespace labrador;

GameMenu::GameMenu(GameData* game_data) : game_data_(game_data)
{

}

GameData* GameMenu::data() const
{
	return this->game_data_;
}

void GameMenu::init()
{
	this->menu_input_ = std::make_unique<MenuInput>(this->data()->gamepads(),
		this->data()->keyboard(), this->data()->tuning());
	this->menu_level_settings_ = std::make_unique<MenuLevelSettings>();
	if (const auto& remembered = this->data()->save()->last_match_setup(); remembered)
	{
		*this->menu_level_settings_ = remembered->restore(*this->data()->stages());
	}
	this->menu_context_ = std::make_unique<MenuContext>(this->data(),
		this->menu_input_.get(), this->menu_level_settings_.get());
	this->menu_context_->set_music_update([this](float dt) { this->update_music(dt); });
	this->menu_context_->set_music_suspend([this]
	{
		if (this->music_) { this->music_->suspend(); }
	});

	this->open_menu(MainMenuScreen::title);
}

// GameMenu owns shared setup state; pages and matches above it handle each frame.
void GameMenu::update(float /*dt*/)
{

}

void GameMenu::draw(Renderer& /*renderer*/) const
{

}

void GameMenu::open_menu(MainMenuScreen screen)
{
	const Presentation& look = *this->data()->presentation();
	this->music_ = std::make_unique<ScreenMusic>(look.menu().sound_bank,
		this->data()->audio_resources(), &this->data()->save()->play_settings(),
		look.main_menu().music, look.music_mix(), true);
	if (!this->context()->active()) { this->music_->suspend(); }
	this->menu_input_->reset_actions();
	// The frame retains provenance while pages replace each other during setup.
	this->context()->push<MatchStart>(this->build_page(screen),
		[this](const MatchStart& start) { this->start_match(start); });
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
	case MainMenuScreen::credits:
		return std::make_unique<MainMenuCredits>(context);
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

void GameMenu::start_match(MatchStart start)
{
	// Release resolved bank handles before a queued content reload can replace it.
	this->menu_context_->cancel_music_exit();
	this->music_.reset();
	// Keep the menu context and its settings alive beneath the match.
	this->context()->push<EndMenuAction>(
		std::make_unique<GameLevel>(this->data(), *this->menu_level_settings_, start),
		[this](const EndMenuAction& action) { this->on_match_ended(action); });
}

void GameMenu::on_deactivated()
{
	if (this->music_) { this->music_->suspend(); }
}

void GameMenu::update_music(float dt)
{
	if (!this->music_ || !this->context()->active()) { return; }
	// Only a normal page tick restores playback. Focus returning underneath a
	// developer overlay must not resume a track whose page is still covered.
	if (dt > 0.0f && std::isfinite(dt)) { this->music_->resume(); }
	if (this->menu_context_->music_exit_pending()) { this->music_->fade_out(); }
	this->music_->update(dt);
	if (this->menu_context_->music_exit_pending() && this->music_->finished())
	{
		this->menu_context_->complete_music_exit();
	}
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
		// GameLevel normally handles restart without leaving the match.
		this->start_match(MatchStart::developer);
		break;
	case EndMenuAction::watch_replay:
		// GameLevel plays it without leaving the match; never returned here.
		this->open_menu(MainMenuScreen::home);
		break;
	}
}

GameLevel::GameLevel(GameData* game_data, const MenuLevelSettings& settings, MatchStart start) :
	game_data_(game_data), settings_(settings), start_(start),
	session_(settings.player_count() == 1 ? 1 : settings.best_of()),
	rumble_sink_(make_rumble_sink())
{
	if (this->rumble_sink_ == nullptr) { throw std::runtime_error("Rumble sink factory returned no sink."); }
}

GameLevel::~GameLevel()
{
	// Stop before detaching callbacks or tearing down the Level. The borrowed
	// sink remains alive through this reset and through member unwind.
	this->rumble_.reset();
	if (this->level_ != nullptr) { this->level_->clear_rumble_events(); }
	this->data()->debug().detach_match(*this);
	this->data()->viewport_manager()->set_layout(this->previous_layout_);
}

GameData* GameLevel::data() const
{
	return this->game_data_;
}

void GameLevel::init()
{
	// Restore this layout on every exit through the destructor.
	this->previous_layout_ = this->data()->viewport_manager()->layout();

	this->player_input_ = std::make_unique<PlayerInput>(
		this->data()->gamepads(),
		this->data()->keyboard(),
		this->data()->mouse(),
		this->data()->tuning(), &this->data()->save()->play_settings(),
		&this->data()->save()->input_bindings());

	this->menu_input_ = std::make_unique<MenuInput>(
		this->data()->gamepads(),
		this->data()->keyboard(), this->data()->tuning());

	this->menu_context_ = std::make_unique<MenuContext>(this->data(),
		this->menu_input_.get(), &this->settings_);
	this->menu_context_->set_music_update([this](float dt)
	{
		this->update_music(dt);
	});

	this->level_builder_ = std::make_unique<LevelBuilder>(
		this->data()->viewport_manager(),
		this->data()->render_resources(),
		this->data()->audio_resources(),
		&this->data()->save()->play_settings(),
		this->data()->level_infos(),
		this->data()->tuning(),
		this->data()->presentation(),
		this->data()->resolution_manager(),
		this->data()->thread_pool(),
		this->data()->partitioner(),
		this->data()->mouse(), &this->data()->save()->input_bindings());

	// Content reloaded from the menus is swapped in by the first build.
	this->data()->debug().attach_match(*this);
	this->build_and_enter_level();
	// Remember only a normally confirmed setup, after its first complete build.
	// Later restarts/series rounds use build_and_enter_level without this write.
	const std::vector<MenuPlayerSettings> seats = this->settings_.player_settings();
	if (this->start_ == MatchStart::menu &&
		std::none_of(seats.begin(), seats.end(), [this](const MenuPlayerSettings& seat)
			{ return this->data()->debug().drives_slot(seat.num); }))
	{
		std::ignore = this->data()->save()->remember_match_setup_and_save(
			LastMatchSetup::capture(this->settings_));
		// This synchronous save is part of entering the match, not elapsed play.
		this->data()->application()->reset_elapsed_time();
	}
}

// LevelBuilder needs the match layout before it builds split-screen dividers.
void GameLevel::build_and_enter_level()
{
	this->data()->debug().sync_background_audio(this->context()->active());
	this->background_continuing_ = false;
	this->rumble_.reset();
	if (this->level_ != nullptr) { this->level_->clear_rumble_events(); }
	this->menu_context_->cancel_music_exit();
	// Stop the old coda before a content reload can replace its bank.
	this->results_audio_.reset();
	this->resume_countdown_.cancel();
	this->resume_audio_.reset();
	this->focus_interrupted_ = false;
	// Destroy the old level first: its destructor stops music in the shared bank
	// and would otherwise stop the new level's track.
	this->level_.reset();
	// Every build - first, restart from a menu, or a debug rebuild - applies
	// content `reload` validated and consumes a queued debug restart; a menu
	// restart also adopts a queued debug plan, so settings_ may change here.
	this->data()->debug().before_level_build(*this);
	// Each round draws its own seed from the running sequence, which a Debug
	// `seed` fixes, so its tape can build the same level again (ROAD#19).
	this->end_replay();
	const unsigned int seed = (static_cast<unsigned int>(std::rand()) << 15) ^
		static_cast<unsigned int>(std::rand());
	std::srand(seed);
	// A reload may replace the authored profile. No old pulse or pending event
	// crosses the round boundary; the sink belongs to the containing screen.
	this->rumble_ = std::make_unique<Rumble>(*this->rumble_sink_, this->data()->tuning()->rumble());
	// Panes belong to humans; scoring, spawns and the session use participants.
	this->settings_.set_screen_layout(static_cast<ScreenLayout>(this->settings_.human_count() - 1));
	this->data()->viewport_manager()->
		set_layout(this->settings_.screen_layout());
	// The first match of this screen has the whole intro; a rematch starts
	// with the colours being tried. The developer tools may choose otherwise.
	// Solo practice has no person at either end: it is not a real match.
	const bool solo = this->settings_.player_count() == 1;
	const OpeningMode opening = solo ? OpeningMode::off : this->data()->debug().opening_mode(
		this->builds_ == 0 ? OpeningMode::full : OpeningMode::quick);
	++this->builds_;
	// Untimed practice is what the menus' solo practice reaches on a stage
	// with lessons; a developer start keeps the timed course for testing.
	this->tape_.start(seed, this->settings_);
	this->level_ = this->level_builder_->build_level(this->settings_,
		this->data()->save()->team_palette(), opening, this->start_ == MatchStart::menu);
	this->data()->debug().after_level_build(*this);
	this->level_->set_pad_devices(this->settings_.pad_devices());
	this->menu_context_->reset_connection_audio();
	this->resume_audio_ = std::make_unique<ResumeAudio>(this->data()->audio_resources(),
		*this->data()->presentation());
	if (!this->context()->active()) { this->on_deactivated(); }
	this->sync_rumble();

	// Discard loading time so fixed-step updates do not advance the new match
	// to catch up with the time spent building it.
	this->data()->application()->reset_elapsed_time();
}

// Activation reaches covered states too. Returning to a paused window must
// leave the match's voices paused until the menu is also closed.
void GameLevel::on_activated()
{
	if (this->replay_phase_ != ReplayPhase::none)
	{
		this->data()->debug().sync_background_audio(true);
		this->level_->resume();
		this->sync_rumble();
		return;
	}
	this->data()->debug().sync_background_audio(true);
	this->background_continuing_ = false;
	this->level_->clear_rumble_events();
	// Results and End cover this state, but their shared coda follows focus.
	if (this->results_audio_ && !this->data()->debug().match_audio_paused())
	{
		this->results_audio_->resume();
	}
	if ((this->pause_menu_open_ && this->covered_) ||
		(!this->covered_ && this->resume_countdown_.active()))
	{
		if (!this->data()->debug().match_audio_paused()) { this->level_->resume_music(); }
	}
	if (!this->covered_ && this->focus_interrupted_)
	{
		this->begin_resume_countdown();
	}
	if (!this->covered_) { this->focus_interrupted_ = false; }
	if (!this->covered_) { this->level_->emphasise_players(); }
	if (this->audio_may_play())
	{
		this->level_->resume();
	}
	this->sync_rumble();
}

bool GameLevel::audio_may_play() const
{
	return !this->covered_ && !this->pause_menu_open_ &&
		!this->resume_countdown_.active() && this->context()->active() &&
		!this->data()->debug().match_audio_paused();
}
void GameLevel::on_deactivated()
{
	if (this->replay_phase_ != ReplayPhase::none)
	{
		// A replay has no pause to come back to; it waits, silent, for focus.
		this->data()->debug().sync_background_audio(false);
		this->stop_rumble();
		this->level_->pause_audio();
		return;
	}
	this->data()->debug().sync_background_audio(false);
	this->stop_rumble();
	if (this->results_audio_) { this->results_audio_->suspend(); }
	if (!this->covered_ && !this->pause_menu_open_ &&
		this->data()->debug().background_updates_allowed())
	{
		this->background_continuing_ = true;
		this->focus_interrupted_ = false;
		this->resume_audio_->stop();
		this->level_->pause_audio();
		return;
	}
	this->background_continuing_ = false;
	this->focus_interrupted_ = this->level_->state() == LevelState::active;
	this->resume_audio_->stop();
	this->suspend_level();
	this->data()->debug().match_suspended(*this);
}

void GameLevel::update(float dt)
{
	this->data()->debug().sync_background_audio(this->context()->active());
	this->sync_rumble();
	// Check current focus because a state created in the background has not
	// received a deactivation callback.
	if (!this->context()->active())
	{
		this->debug_background_changed();
		if (this->covered_ || this->pause_menu_open_ ||
			!this->data()->debug().background_updates_allowed()) { return; }
	}
	// An instant replay owns the screen: no console, pause or seat checks.
	if (this->update_replay(dt)) { return; }

	// Console and debug-menu keys, a pending screenshot, F-key binds and
	// queued debug requests, applied while no Level::update is on the call
	// stack. Inert in Release.
	if (this->data()->debug().before_match_update(*this, dt, this->context()->active()))
	{
		// StateContext defers destruction until this update returns. A hook
		// that just opened an overlay or rebuilt must publish its new gate now.
		this->sync_rumble();
		return;
	}
	// A script can disable its own background mode in this hook.
	this->data()->debug().sync_background_audio(this->context()->active());
	if (!this->context()->active() && !this->data()->debug().background_updates_allowed()) { return; }
	// The debug hook may have rebuilt the round or changed a driver/gate.
	// Motor time follows this real-time update once, not its 0..4 sim steps.
	this->sync_rumble();
	if (dt > 0.0f && std::isfinite(dt)) { this->rumble_->update(dt); }
	// This clock is outside the debug director's 0..4 simulation steps.
	this->update_music(dt);

	if (this->level_->state() == LevelState::finished)
	{
		this->open_results_menu();
		return;
	}

	std::vector<PlayerInputData> player_inputs;
	if (this->context()->active())
	{
		player_inputs = this->player_input_->player_inputs(this->settings_.pad_devices());
	}
	else
	{
		// Neutral physical baseline, then the normal seat filter supplies bots
		// or replay input. Actual pad presence is checked separately below.
		player_inputs.resize(input_slots::count);
		for (int slot = 0; slot < input_slots::count; ++slot)
		{
			player_inputs[static_cast<size_t>(slot)].connected =
				input_slots::is_keyboard_and_mouse(slot) ||
				this->data()->gamepads()->connected(this->settings_.device_slot(slot));
		}
	}
	// A pad's Back and Start edges consumed by the debug-menu chord reach
	// neither the pause check nor the players' F1 overlay. Inert in Release.
	this->data()->debug().filter_match_inputs(player_inputs);
	// Over the intro, Start skips it instead (Level::update), and that press
	// cannot open pause even if it is released after GO.
	latch_intro_pause(this->level_->intro_playing(), player_inputs,
		this->pause_over_intro_);
	this->resume_countdown_.filter_inputs(player_inputs);

	// Check current presence, not just a disconnect edge: focus loss can hide
	// the edge, and no participant may lose match time while reconnecting.
	// A seat a Debug bot or replay drives never waits for its pad.
	const int missing_slot = disconnected_player_slot(this->settings_,
		*this->data()->gamepads(), this->data()->debug());
	// The pause page may have handed a seat to a returning pad (ROAD#4).
	this->level_->set_pad_devices(this->settings_.pad_devices());
	if (dt > 0.0f && this->context()->active())
	{
		this->menu_context_->update_connection_audio();
	}
	if (missing_slot != -1)
	{
		this->open_pause_menu(missing_slot);
		return;
	}

	const int pause_menu_player = this->check_for_pause_input(player_inputs);
	if (pause_menu_player != -1)
	{
		this->open_pause_menu(pause_menu_player);
		return;
	}

	if (this->resume_countdown_.active())
	{
		// Enabling background work never consumes a manual/focus resume gate.
		if (!this->context()->active()) { return; }
		// Do not enter the debug clock, seat filters or Level at all: every
		// simulation timer and cosmetic stays at its completed pre-pause tick.
		if (this->resume_countdown_.advance(dt))
		{
			this->resume_audio_->stop();
			this->tape_.record_resume(player_inputs);
			this->level_->prepare_resume(player_inputs);
			this->level_->emphasise_players();
			if (this->audio_may_play()) { this->level_->resume(); }
		}
		else if (dt > 0.0f)
		{
			this->resume_audio_->update(this->resume_countdown_.remaining());
		}
		// Even a long final tick is entirely count-in time, never catch-up play.
		return;
	}

	// One Level::update, or the debug clock's 0..4 fixed-dt updates.
	this->data()->debug().update_match(*this, player_inputs, dt);
}

void GameLevel::draw(Renderer& renderer) const
{
	// The perf overlay's frame starts here. Inert in Release.
	this->data()->debug().begin_match_draw();
	// A covered match does not update, but its window can still resize. Prepare
	// its presentation on this thread before Scene::draw starts any workers.
	this->level_->refresh_layout();
	this->level_->set_debug_frame_time(this->data()->debug().frame_time_ms());
	this->level_->draw(renderer);
	if (this->resume_countdown_.active())
	{
		this->level_->draw_resume_countdown(renderer, this->resume_countdown_.remaining());
	}
	if (this->replay_phase_ != ReplayPhase::none) { this->level_->draw_replay_banner(renderer); }
	this->data()->debug().draw_status(renderer);
}

void GameLevel::update_music(float dt)
{
	if (dt < 0.0f || !std::isfinite(dt) || !this->context()->active()) { return; }
	if (this->data()->debug().match_audio_paused())
	{
		// A replay can finish with the developer clock paused. Results/End are
		// still interactive: leaving their suspended coda must not wait on a
		// frozen ramp. Only a normal page drives this context, never its cover.
		if (this->menu_context_->music_exit_pending())
		{
			this->results_audio_.reset();
			this->menu_context_->complete_music_exit();
		}
		return;
	}
	// A developer overlay can cover Results/End without uncovering GameLevel
	// when it closes. Their next normal page tick restores the existing coda.
	if (this->results_audio_)
	{
		if (dt > 0.0f) { this->results_audio_->resume(); }
		if (this->menu_context_->music_exit_pending()) { this->results_audio_->fade_out(); }
		this->results_audio_->update(dt);
	}
	if (this->menu_context_->music_exit_pending() &&
		(!this->results_audio_ || this->results_audio_->finished()))
	{
		this->menu_context_->complete_music_exit();
		return;
	}
	if ((this->pause_menu_open_ && this->covered_) ||
		(!this->covered_ && this->resume_countdown_.active()))
	{
		// Also recover the quiet voice after a developer clock pause is lifted
		// while a gameplay pause or its count-in still covers simulation.
		this->level_->set_music_ducked(true);
		this->level_->resume_music();
	}
	this->level_->update_music(dt);
}

void GameLevel::on_suspend()
{
	this->background_continuing_ = false;
	this->covered_ = true;
	this->stop_rumble();
	this->resume_audio_->stop();
	// Gameplay voices stop here; normal pause retains only quiet arena music.
	this->suspend_level();
	if (this->pause_menu_open_ && this->context()->active() &&
		!this->data()->debug().match_audio_paused())
	{
		this->level_->set_music_ducked(true);
		this->level_->resume_music();
	}
	this->data()->debug().match_suspended(*this);
}

void GameLevel::on_resume()
{
	this->covered_ = false;
	if (!this->context()->active()) { this->debug_background_changed(); }
	this->level_->clear_rumble_events();
	// StateContext uncovers us before delivering the menu's result. Only its
	// Resume result may start a count-in; Restart and Quit take other paths.
	if (this->pause_menu_open_) { this->sync_rumble(); return; }
	if (this->focus_interrupted_)
	{
		this->begin_resume_countdown();
		this->focus_interrupted_ = false;
	}
	this->level_->emphasise_players();
	if (this->audio_may_play())
	{
		this->level_->resume();
	}
	this->sync_rumble();
}

void GameLevel::open_pause_menu(int player_num)
{
	this->pause_menu_open_ = true;
	this->stop_rumble();
	this->menu_input_->reset_actions();
	this->context()->push<PauseMenuAction>(
		std::make_unique<PauseMenuInitial>(this->menu_context_.get(),
			player_num, true, this->level_->practice()),
		[this](const PauseMenuAction& action)
		{
			this->on_pause_menu_closed(action);
		});
}

void GameLevel::on_pause_menu_closed(const PauseMenuAction& action)
{
	this->pause_menu_open_ = false;
	switch (action)
	{
	case PauseMenuAction::resume:
		this->begin_resume_countdown();
		if (this->audio_may_play()) { this->level_->resume(); }
		break;
	case PauseMenuAction::restart:
		this->build_and_enter_level();
		break;
	case PauseMenuAction::quit:
		this->leave_match(EndMenuAction::exit);
		break;
	case PauseMenuAction::change_tool:
	{
		// The page wrote the tool into settings_. The rebuild cleans the
		// paint; a living player keeps their place and goes straight to GO.
		const Player& player = this->level_->player(this->level_->participant_for_view(0));
		const std::optional<mattmath::Vector2F> place = player.state() == PlayerState::alive ?
			std::optional<mattmath::Vector2F>(player.rectangle().position()) : std::nullopt;
		this->build_and_enter_level();
		this->level_->start_without_countdown();
		if (place) { this->level_->place_player(this->level_->participant_for_view(0), *place); }
		break;
	}
	}
}

void GameLevel::begin_resume_countdown()
{
	this->stop_rumble();
	this->focus_interrupted_ = false;
	if (this->level_->state() != LevelState::active) { return; }
	this->resume_countdown_.begin(this->data()->presentation()->pause_menu().resume_countdown);
	this->resume_audio_->stop();
	this->suspend_level();
	this->level_->emphasise_players();
	this->level_->set_music_ducked(true);
	if (this->context()->active() && !this->data()->debug().match_audio_paused())
	{
		this->level_->resume_music();
	}
}

const Level& GameLevel::level() const { return *this->level_; }
float GameLevel::resume_remaining() const { return this->resume_countdown_.remaining(); }

void GameLevel::open_results_menu()
{
	this->stop_rumble();
	const LevelEndInfo result = this->level_->level_end_info();
	this->session_.finish_round(result);
	// Every finished round counts toward its stage's record, unless a Debug
	// bot or replay played a seat (never in Release).
	RecordOutcome record;
	const std::vector<MenuPlayerSettings> seats = this->settings_.player_settings();
	if (std::none_of(seats.begin(), seats.end(), [this](const MenuPlayerSettings& seat)
		{ return this->data()->debug().drives_slot(seat.num); }))
	{
		record = this->data()->save()->record_result_and_save(this->settings_.stage(), result);
	}
	this->results_audio_ = std::make_unique<ResultsAudio>(this->data()->audio_resources(),
		&this->data()->save()->play_settings(), *this->data()->presentation());
	if (this->data()->debug().match_audio_paused()) { this->results_audio_->suspend(); }
	this->menu_input_->reset_actions();
	this->context()->push(
		std::make_unique<ResultsMenuInitial>(this->menu_context_.get(),
			result, this->session_, this->level_->revealed(), record),
		[this]() { this->open_end_menu(); });
}

void GameLevel::open_end_menu()
{
	this->context()->push<EndMenuAction>(
		std::make_unique<EndMenuInitial>(this->menu_context_.get(), this->session_,
			this->tape_.playable() && !this->level_->practice()),
		[this](const EndMenuAction& action)
		{
			this->on_end_menu_closed(action);
		});
}

void GameLevel::on_end_menu_closed(const EndMenuAction& action)
{
	if (action == EndMenuAction::watch_replay)
	{
		this->start_replay();
		return;
	}
	if (action == EndMenuAction::restart)
	{
		if (this->session_.complete())
		{
			this->session_ = Session(this->settings_.player_count() == 1 ?
				1 : this->settings_.best_of());
		}
		else
		{
			this->session_.next_round();
		}
		this->build_and_enter_level();
		return;
	}

	// The persistent GameMenu opens the requested setup page.
	this->leave_match(action);
}

void GameLevel::leave_match(EndMenuAction action) const
{
	this->stop_rumble();
	this->menu_context_->cancel_music_exit();
	this->context()->pop(action);
}

int GameLevel::check_for_pause_input(
	const std::vector<PlayerInputData>& player_inputs) const
{
	for (int i = 0; i < static_cast<int>(player_inputs.size()); i++)
	{
		if (player_inputs[i].toggle_pause_menu &&
			match_owns_pad_slot(this->settings_, i))
		{
			return i;
		}
	}
	return -1;
}
