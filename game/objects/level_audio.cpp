#include "game/pch.h"
#include "game/objects/level.h"
#include "game/presentation.h"
#include "game/ai/match_bot_driver.h"
#include "game/objects/level_bot_world.h"

using namespace mattmath;
using namespace labrador;

// The match's audio lifecycle: teardown, suspend, pause and resume through
// the one voice list, and the music. The constructor resolves and registers
// the voices in level.cpp.

Level::~Level()
{
	this->voices_.stop();
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
	this->voices_.pause();
}

void Level::resume()
{
	this->set_music_ducked(false);
	// Nothing re-pushes the player's volume here: a change made while paused
	// reached these voices through their sound group (game/sound_groups.h).
	// The music voice resumes through resume_music's rule (MusicVoice).
	this->voices_.resume();
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
	const SoundState state = this->music_.state();
	if (state == SoundState::stopped && this->restart_lost_music()) { return; }
	if (state != SoundState::playing) { return; }
	this->music_fade_.advance(dt);
	this->music_duck_.advance(dt);
	this->music_.set_volume(this->music_gain());
	if (this->music_ending_ && this->music_fade_.gain() == 0.0f)
	{
		this->music_.stop();
	}
}

float Level::music_gain() const
{
	return this->music_volume_ * this->music_fade_.gain() * this->music_duck_.gain();
}

// After the countdown starts the arena loop, only the ending fade, the overview
// cut and teardown stop it. Any other stop came from outside - device recovery
// stops persistent voices without an event (ROAD#36) - so play it again from
// silence and fade in; the duck keeps its own target. A silent bank stays silent.
bool Level::restart_lost_music()
{
	if (!this->countdown_started_ || this->music_ending_ || !this->music_.audible()) { return false; }
	this->music_fade_ = MusicEnvelope();
	this->music_fade_.fade_to(1.0f, this->presentation_->music_mix().fade_in);
	this->music_.play(true, 0.0f);
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
	this->music_.resume();
}

Level::MusicVoice::MusicVoice(const Level* level, const std::string& sound_bank_name,
	const AudioResources* audio_resources, const std::string& effect_name) :
	MatchVoice(sound_bank_name, audio_resources, effect_name),
	level_(level)
{
}

void Level::MusicVoice::resume()
{
	if (!this->valid() || this->state() != SoundState::paused) { return; }
	this->set_volume(this->level_->music_gain());
	MatchVoice::resume();
}

void Level::stop_player_sounds()
{
	for (Player* player : this->players_)
	{
		player->stop_sounds();
	}
}
