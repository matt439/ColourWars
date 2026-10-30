#include "game/pch.h"
#include "game/objects/level.h"
#include "game/ai/match_bot_driver.h"

using namespace mattmath;
using namespace labrador;

// The match's audio lifecycle: the teardown stop, suspend, pause and resume
// voice lists, music and the per-player sound fan-out. The constructor's
// sound resolution is in level.cpp.

Level::~Level()
{
	if (this->end_sting_.valid()) { this->sound_bank_->stop_effect(this->end_sting_, true); }
	if (this->music_.valid())
	{
		this->sound_bank_->stop_effect(this->music_, true);
	}
	this->sound_bank_->stop_effect(this->zoom_out_sound_, true);
	this->sound_bank_->stop_effect(this->countdown_sound_, true);
	this->sound_bank_->stop_effect(this->go_sound_, true);
	this->sound_bank_->stop_effect(this->time_up_sound_, true);
	this->sound_bank_->stop_effect(this->last_ten_sound_, true);
	this->sound_bank_->stop_effect(this->room_tone_sound_, true);
	this->sound_bank_->stop_effect(this->sigh_sound_, true);
	this->sound_bank_->stop_effect(this->chair_sound_, true);
	this->sound_bank_->stop_effect(this->glass_in_sound_, true);
	this->sound_bank_->stop_effect(this->celebrate_sound_, true);
	this->sound_bank_->stop_effect(this->slump_sound_, true);
	this->sound_bank_->stop_effect(this->colour_in_sound_, true);
	this->sound_bank_->stop_effect(this->skip_sound_, true);
	this->sound_bank_->stop_effect(this->undo_sound_, true);
	this->stop_player_sounds();
}

void Level::suspend()
{
	this->interrupt_players();
	this->pause_audio();
}

void Level::interrupt_players()
{
	for (Player* player : this->players_)
	{
		player->reset_jump_input();
		player->clear_view_shake();
	}
}

void Level::pause_audio()
{
	if (this->end_sting_.valid()) { this->sound_bank_->pause_effect(this->end_sting_); }
	if (this->music_.valid())
	{
		this->sound_bank_->pause_effect(this->music_);
	}
	this->sound_bank_->pause_effect(this->zoom_out_sound_);
	this->sound_bank_->pause_effect(this->countdown_sound_);
	this->sound_bank_->pause_effect(this->go_sound_);
	this->sound_bank_->pause_effect(this->time_up_sound_);
	this->sound_bank_->pause_effect(this->last_ten_sound_);
	this->sound_bank_->pause_effect(this->room_tone_sound_);
	this->sound_bank_->pause_effect(this->sigh_sound_);
	this->sound_bank_->pause_effect(this->chair_sound_);
	this->sound_bank_->pause_effect(this->glass_in_sound_);
	this->sound_bank_->pause_effect(this->celebrate_sound_);
	this->sound_bank_->pause_effect(this->slump_sound_);
	this->sound_bank_->pause_effect(this->colour_in_sound_);
	this->sound_bank_->pause_effect(this->skip_sound_);
	this->sound_bank_->pause_effect(this->undo_sound_);
	this->pause_player_sounds();
}

void Level::resume()
{
	this->set_music_ducked(false);
	// Nothing re-pushes the player's volume here: a change made while paused
	// reached these voices through their sound group (game/sound_groups.h).
	if (this->end_sting_.valid()) { this->sound_bank_->resume_effect(this->end_sting_); }
	this->resume_music();
	this->sound_bank_->resume_effect(this->zoom_out_sound_);
	this->sound_bank_->resume_effect(this->countdown_sound_);
	this->sound_bank_->resume_effect(this->go_sound_);
	this->sound_bank_->resume_effect(this->time_up_sound_);
	this->sound_bank_->resume_effect(this->last_ten_sound_);
	this->sound_bank_->resume_effect(this->room_tone_sound_);
	this->sound_bank_->resume_effect(this->sigh_sound_);
	this->sound_bank_->resume_effect(this->chair_sound_);
	this->sound_bank_->resume_effect(this->glass_in_sound_);
	this->sound_bank_->resume_effect(this->celebrate_sound_);
	this->sound_bank_->resume_effect(this->slump_sound_);
	this->sound_bank_->resume_effect(this->colour_in_sound_);
	this->sound_bank_->resume_effect(this->skip_sound_);
	this->sound_bank_->resume_effect(this->undo_sound_);
	this->resume_player_sounds();
}

void Level::prepare_resume(const std::vector<PlayerInputData>& seats)
{
	for (size_t participant = 0; participant < this->players_.size(); ++participant)
	{
		Player* player = this->players_[participant];
		player->stop_tool_sounds();
		// A seat no device drives (a bot, an unclaimed human) keeps its arming.
		const int slot = player->player_num();
		if (slot >= 0 && slot < input_slots::count && participant < seats.size())
		{
			player->observe_jump_release(seats[participant].jump_held || seats[participant].jump_pressed);
		}
	}
}

void Level::update_music(float dt)
{
	if (!(dt > 0.0f) || !std::isfinite(dt) || !this->music_.valid()) { return; }
	const SoundState state = this->sound_bank_->effect_state(this->music_);
	if (state == SoundState::stopped && this->restart_lost_music()) { return; }
	if (state != SoundState::playing) { return; }
	this->music_fade_.advance(dt);
	this->music_duck_.advance(dt);
	this->sound_bank_->set_effect_volume(this->music_, this->music_volume_ *
		this->music_fade_.gain() * this->music_duck_.gain());
	if (this->music_ending_ && this->music_fade_.gain() == 0.0f)
	{
		this->sound_bank_->stop_effect(this->music_, true);
	}
}

// After the countdown starts the arena loop, only the ending fade, the overview
// cut and teardown stop it. Any other stop came from outside - device recovery
// stops persistent voices without an event (ROAD#36) - so play it again from
// silence and fade in; the duck keeps its own target. A silent bank stays silent.
bool Level::restart_lost_music()
{
	if (!this->countdown_started_ || this->music_ending_ || !this->sound_bank_->audible()) { return false; }
	this->music_fade_ = MusicEnvelope();
	this->music_fade_.fade_to(1.0f, this->presentation_->music_mix().fade_in);
	this->sound_bank_->play_effect(this->music_, true, 0.0f);
	return true;
}

void Level::set_music_ducked(bool ducked)
{
	const MusicMixPresentation& mix = this->presentation_->music_mix();
	this->music_duck_.fade_to(ducked ? mix.pause_gain : 1.0f,
		ducked ? mix.duck_time : mix.restore_time);
}

void Level::resume_music()
{
	if (!this->music_.valid() || this->sound_bank_->effect_state(this->music_) != SoundState::paused) { return; }
	this->sound_bank_->set_effect_volume(this->music_, this->music_volume_ *
		this->music_fade_.gain() * this->music_duck_.gain());
	this->sound_bank_->resume_effect(this->music_);
}

void Level::stop_player_sounds()
{
	for (Player* player : this->players_)
	{
		player->stop_sounds();
	}
}

void Level::pause_player_sounds()
{
	for (Player* player : this->players_)
	{
		player->pause_sounds();
	}
}

void Level::resume_player_sounds()
{
	for (Player* player : this->players_)
	{
		player->resume_sounds();
	}
}
