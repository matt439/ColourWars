#include "game/pch.h"
#include "game/objects/level.h"
#include "game/presentation.h"
#include "game/ai/match_bot_driver.h"
#include "game/objects/level_bot_world.h"
#include "game/objects/teammate_markers.h"
#include "game/objects/player_identity_hud.h"
#include "game/objects/match_audio.h"

using namespace mattmath;
using namespace labrador;

// The match's audio lifecycle as Level presents it: teardown, suspend, pause
// and resume, and what each does to the players. MatchAudio
// (match_audio.cpp) owns the one voice list and the music, and is created
// and registered in level.cpp; these forward to it.

Level::~Level()
{
	this->audio_->stop();
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
	this->audio_->pause();
}

void Level::resume()
{
	this->set_music_ducked(false);
	// Nothing re-pushes the player's volume here: a change made while paused
	// reached these voices through their sound group (game/sound_groups.h).
	// The music voice resumes through resume_music's rule (MusicVoice).
	this->audio_->resume();
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
	this->audio_->update_music(dt);
}

void Level::set_music_ducked(bool ducked)
{
	this->audio_->set_ducked(ducked);
}

void Level::resume_music()
{
	this->audio_->resume_music();
}

void Level::stop_player_sounds()
{
	for (Player* player : this->players_)
	{
		player->stop_sounds();
	}
}
