#include "game/pch.h"
#include "game/states/game_states.h"
#include "game/sound_groups.h"
#include <chrono>

using namespace labrador;

// The end-of-round instant replay (ROAD#19). The round's tape (game/round_tape.h)
// holds the seed its level was built from and every seat's input on every
// logic step, so the replay is the same round built again: the level is
// rebuilt, its earlier steps run with the screen showing them at speed and
// the sound groups silent, and its last seconds play at the normal rate. Any
// confirm, back or pause press ends it early. Afterwards the End menu opens
// again over the replay's last frame, which is the moment time ran out.

namespace
{
	// Wall time the hidden steps may take per frame: a whole round costs
	// tens of milliseconds a second of play in Release, so a long one
	// prepares in about a second while the screen keeps drawing.
	constexpr std::chrono::milliseconds preparation_budget{ 12 };
}

void GameLevel::start_replay()
{
	this->stop_rumble();
	this->menu_context_->cancel_music_exit();
	this->results_audio_.reset();
	this->resume_countdown_.cancel();
	this->resume_audio_->stop();
	// As build_and_enter_level: the old level's destructor stops the shared
	// bank's music, so it goes before the replay's starts.
	this->level_.reset();
	// The hidden steps start voices too; nothing is heard until the shown ones.
	PlaySettings silent = this->data()->settings_session()->play_settings();
	silent.music_volume = 0;
	silent.sfx_volume = 0;
	sound_groups::apply(*this->data()->audio_resources(), silent);

	// The round's own setup: a re-pick page may have changed settings_ and
	// been backed out of.
	const MatchSetup& round = this->tape_.settings();
	this->data()->viewport_manager()->set_layout(round.screen_layout());
	std::srand(this->tape_.seed());
	// The tape starts at the first step of play, which needs no intro.
	this->level_ = this->level_builder_->build_level(round,
		this->data()->save()->team_palette(), OpeningMode::off, this->start_ == MatchStart::menu);
	this->level_->set_pad_devices(round.pad_devices());
	this->level_->set_player_cheats(this->tape_.damage_immune(), this->tape_.infinite_ammo());
	this->level_->start_without_countdown();
	this->replay_player_ = std::make_unique<RoundTapePlayer>(&this->tape_, 0);
	this->replay_first_ = this->tape_.step_before_end(this->data()->tuning()->match().replay);
	this->replay_phase_ = ReplayPhase::preparing;
	this->menu_input_->reset_actions();
	if (!this->context()->active()) { this->level_->pause_audio(); }
	this->sync_rumble();
	this->data()->application()->reset_elapsed_time();
}

bool GameLevel::update_replay(float dt)
{
	if (this->replay_phase_ == ReplayPhase::none) { return false; }
	this->update_music(dt);
	if (this->context()->active())
	{
		for (const ProcessedMenuInput& input : this->menu_input_->menu_inputs())
		{
			if (input.action == MenuInputAction::proceed || input.action == MenuInputAction::back ||
				input.action == MenuInputAction::pause)
			{
				this->finish_replay();
				return true;
			}
		}
	}
	if (!(dt > 0.0f) || !std::isfinite(dt)) { return true; }

	if (this->replay_phase_ == ReplayPhase::preparing)
	{
		const auto deadline = std::chrono::steady_clock::now() + preparation_budget;
		while (this->replay_player_->step() < this->replay_first_ &&
			std::chrono::steady_clock::now() < deadline)
		{
			this->step_replay();
		}
		if (this->replay_player_->step() >= this->replay_first_)
		{
			this->replay_phase_ = ReplayPhase::playing;
			sound_groups::apply(*this->data()->audio_resources(), this->data()->settings_session()->play_settings());
			this->level_->emphasise_players();
		}
		// The budget was spent on the replay, not on time the round must catch up.
		this->data()->application()->reset_elapsed_time();
		return true;
	}

	// One taped step per fixed step: the replay runs at the round's own rate.
	this->step_replay();
	if (this->replay_player_->finished()) { this->finish_replay(); }
	return true;
}

void GameLevel::step_replay()
{
	const int step = this->replay_player_->step();
	this->tape_.apply_gaps(*this->level_, step);
	// Every seat's input comes from the tape; the devices are never read.
	std::vector<SlotInput> devices(input_slots::count);
	for (SlotInput& device : devices) { device.connected = true; }
	this->level_->update(this->seat_commands(devices).seats, this->tape_.dt(step),
		this->replay_player_.get());
	this->level_->clear_rumble_events();
}

void GameLevel::finish_replay()
{
	this->end_replay();
	// Back to the End menu, with the results' coda again.
	this->results_audio_ = std::make_unique<ResultsAudio>(this->data()->audio_resources(),
		&this->data()->settings_session()->play_settings(), *this->data()->presentation());
	if (this->data()->debug().match_audio_paused() || !this->context()->active())
	{
		this->results_audio_->suspend();
	}
	this->menu_input_->reset_actions();
	this->open_end_menu();
}

void GameLevel::end_replay()
{
	if (this->replay_phase_ == ReplayPhase::none) { return; }
	this->replay_phase_ = ReplayPhase::none;
	this->replay_player_.reset();
	sound_groups::apply(*this->data()->audio_resources(), this->data()->settings_session()->play_settings());
}

void GameLevel::suspend_level()
{
	if (this->replay_phase_ != ReplayPhase::none)
	{
		// The replay's players keep their jump state, as they did in the round.
		this->level_->pause_audio();
		return;
	}
	this->tape_.record_suspend();
	this->level_->suspend();
}
