#include "game/pch.h"
#include "game/objects/match_audio.h"
#include "game/objects/level_inputs.h"
#include "game/presentation.h"
#include "game/tuning.h"
#include <cmath>

using namespace labrador;

MatchAudio::MatchAudio(const StageAudio& stage, const AudioResources* audio_resources,
	const MatchTuning* match, const Presentation* presentation) :
	match_(match),
	presentation_(presentation),
	music_volume_(stage.music_volume),
	end_sting_volume_(stage.end_sting_volume)
{
	const std::string& sound_bank_name = stage.sound_bank_name;
	// A silent stage leaves music_ unresolved; a voice with no effect does
	// nothing, and the music paths that would wait on it check first.
	const auto voice = [&](const std::string& effect)
	{ return MatchVoice(sound_bank_name, audio_resources, effect); };
	this->music_ = MusicVoice(this, sound_bank_name, audio_resources, stage.music_name);
	this->end_sting_ = voice(stage.end_sting_name);
	this->zoom_out_sound_ = voice(match->zoom_out_sound.name);
	this->countdown_sound_ = voice(match->countdown_sound.name);
	this->go_sound_ = voice(match->go_sound.name);
	this->time_up_sound_ = voice(match->time_up_sound.name);
	this->last_ten_sound_ = voice(match->last_ten_sound.name);
	this->room_tone_sound_ = voice(match->room_tone_sound.name);
	this->sigh_sound_ = voice(match->sigh_sound.name);
	this->chair_sound_ = voice(match->chair_sound.name);
	this->glass_in_sound_ = voice(match->glass_in_sound.name);
	this->celebrate_sound_ = voice(match->celebrate_sound.name);
	this->slump_sound_ = voice(match->slump_sound.name);
	this->colour_in_sound_ = voice(match->colour_in_sound.name);
	this->skip_sound_ = voice(match->skip_sound.name);
	this->undo_sound_ = voice(match->undo_sound.name);
	// The order every pause, resume and teardown has always taken: these
	// voices, then (registered by the Level) each player's tool loop and
	// redraw pen, in roster order.
	for (MatchVoice* own : { static_cast<MatchVoice*>(&this->end_sting_),
		static_cast<MatchVoice*>(&this->music_), &this->zoom_out_sound_,
		&this->countdown_sound_, &this->go_sound_, &this->time_up_sound_,
		&this->last_ten_sound_, &this->room_tone_sound_, &this->sigh_sound_,
		&this->chair_sound_, &this->glass_in_sound_, &this->celebrate_sound_,
		&this->slump_sound_, &this->colour_in_sound_, &this->skip_sound_,
		&this->undo_sound_ })
	{
		this->voices_.add(*own);
	}
}

MatchVoices& MatchAudio::voices()
{
	return this->voices_;
}

void MatchAudio::start_music()
{
	this->music_started_ = true;
	if (this->music_.valid())
	{
		this->music_fade_.fade_to(1.0f, this->presentation_->music_mix().fade_in);
		this->music_.play(true, 0.0f);
	}
}

void MatchAudio::end_music()
{
	if (this->music_.valid())
	{
		this->music_ending_ = true;
		this->music_fade_.fade_to(0.0f, this->presentation_->music_mix().fade_out);
	}
}

void MatchAudio::play_end_sting()
{
	if (this->end_sting_.valid())
	{
		this->end_sting_.play(false,
			this->end_sting_volume_);
	}
}

void MatchAudio::stop_arena()
{
	if (this->music_.valid()) { this->music_.stop(); }
	if (this->end_sting_.valid()) { this->end_sting_.stop(); }
}

void MatchAudio::update_music(float dt)
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

float MatchAudio::music_gain() const
{
	return this->music_volume_ * this->music_fade_.gain() * this->music_duck_.gain();
}

// After the countdown starts the arena loop, only the ending fade, the overview
// cut and teardown stop it. Any other stop came from outside - device recovery
// stops persistent voices without an event (ROAD#36) - so play it again from
// silence and fade in; the duck keeps its own target. A silent bank stays silent.
bool MatchAudio::restart_lost_music()
{
	if (!this->music_started_ || this->music_ending_ || !this->music_.audible()) { return false; }
	this->music_fade_ = MusicEnvelope();
	this->music_fade_.fade_to(1.0f, this->presentation_->music_mix().fade_in);
	this->music_.play(true, 0.0f);
	return true;
}

void MatchAudio::set_ducked(bool ducked)
{
	const MusicMixPresentation& mix = this->presentation_->music_mix();
	this->music_duck_.fade_to(ducked ? mix.pause_gain : 1.0f,
		ducked ? mix.duck_time : mix.restore_time);
}

void MatchAudio::pause()
{
	this->voices_.pause();
}

void MatchAudio::resume()
{
	this->voices_.resume();
}

void MatchAudio::stop()
{
	this->voices_.stop();
}

void MatchAudio::resume_music()
{
	this->music_.resume();
}

void MatchAudio::set_room_tone(bool playing)
{
	if (playing == this->room_tone_playing_)
	{
		return;
	}
	this->room_tone_playing_ = playing;
	if (playing)
	{
		this->room_tone_sound_.play(true,
			this->match_->room_tone_sound.volume);
	}
	else
	{
		this->room_tone_sound_.stop();
	}
}

MatchVoice& MatchAudio::zoom_out_sound() { return this->zoom_out_sound_; }
MatchVoice& MatchAudio::countdown_sound() { return this->countdown_sound_; }
MatchVoice& MatchAudio::go_sound() { return this->go_sound_; }
MatchVoice& MatchAudio::time_up_sound() { return this->time_up_sound_; }
MatchVoice& MatchAudio::last_ten_sound() { return this->last_ten_sound_; }
MatchVoice& MatchAudio::sigh_sound() { return this->sigh_sound_; }
MatchVoice& MatchAudio::chair_sound() { return this->chair_sound_; }
MatchVoice& MatchAudio::glass_in_sound() { return this->glass_in_sound_; }
MatchVoice& MatchAudio::celebrate_sound() { return this->celebrate_sound_; }
MatchVoice& MatchAudio::slump_sound() { return this->slump_sound_; }
MatchVoice& MatchAudio::colour_in_sound() { return this->colour_in_sound_; }
MatchVoice& MatchAudio::skip_sound() { return this->skip_sound_; }
MatchVoice& MatchAudio::undo_sound() { return this->undo_sound_; }

MatchAudio::MusicVoice::MusicVoice(const MatchAudio* audio, const std::string& sound_bank_name,
	const AudioResources* audio_resources, const std::string& effect_name) :
	MatchVoice(sound_bank_name, audio_resources, effect_name),
	audio_(audio)
{
}

void MatchAudio::MusicVoice::resume()
{
	if (!this->valid() || this->state() != SoundState::paused) { return; }
	this->set_volume(this->audio_->music_gain());
	MatchVoice::resume();
}
