#pragma once

#include "game/objects/match_voices.h"
#include "game/objects/music_envelope.h"

namespace labrador
{
	class AudioResources;
}
class Presentation;
struct MatchTuning;
struct StageAudio;

// The match's voices and its arena music (HYG#40): the one MatchVoices list
// that pause, resume and teardown walk, the stage's music and end sting with
// their fade and duck, the match's fixed cues and the opening's room tone and
// story voices. The owning Level registers each player's voices after these
// and still decides when every cue plays and at what volume; this part keeps
// only how the music fades, ducks and comes back. It borrows the match tuning
// and the presentation and reads them on every call, so a `tune` shows at once.
// Not copyable or movable: the list and the music voice point into it.
class MatchAudio
{
public:
	// Resolves the music, the end sting, the match cues, the room tone and the
	// story voices in that order, then registers them in the order pause,
	// resume and teardown have always taken. Throws what MatchVoice throws for
	// the first effect the bank lacks. A silent stage leaves the music and the
	// sting unresolved; a voice with no effect does nothing.
	MatchAudio(const StageAudio& stage, const labrador::AudioResources* audio_resources,
		const MatchTuning* match, const Presentation* presentation);
	MatchAudio(const MatchAudio&) = delete;
	MatchAudio& operator=(const MatchAudio&) = delete;

	// Where the owner registers each player's voices after these.
	MatchVoices& voices();

	// The countdown's first tick: fades the arena loop in from silence. From
	// here on a lost loop is played again (see update_music).
	void start_music();
	// Time up: fades the loop out, after which update_music stops it.
	void end_music();
	void play_end_sting();
	// The overview: the loop and the sting stop, whatever their fade.
	void stop_arena();
	// One active real-time tick of the music's fade and duck.
	void update_music(float dt);
	void set_ducked(bool ducked);
	// Every voice in the list, in registration order.
	void pause();
	void resume();
	void stop();
	// The music voice alone, by its own rule (MusicVoice).
	void resume_music();
	// Starts or stops the room tone loop; a repeated state does nothing.
	void set_room_tone(bool playing);

	MatchVoice& zoom_out_sound();
	MatchVoice& countdown_sound();
	MatchVoice& go_sound();
	MatchVoice& time_up_sound();
	MatchVoice& last_ten_sound();
	MatchVoice& sigh_sound();
	MatchVoice& chair_sound();
	MatchVoice& glass_in_sound();
	MatchVoice& celebrate_sound();
	MatchVoice& slump_sound();
	MatchVoice& colour_in_sound();
	MatchVoice& skip_sound();
	MatchVoice& undo_sound();

private:
	// The arena loop resumes only from a pause, and at the gain the fade and
	// the duck have reached, not where it was left.
	class MusicVoice final : public MatchVoice
	{
	public:
		MusicVoice() = default;
		MusicVoice(const MatchAudio* audio, const std::string& sound_bank_name,
			const labrador::AudioResources* audio_resources,
			const std::string& effect_name);
		void resume() override;
	private:
		const MatchAudio* audio_ = nullptr;
	};

	bool restart_lost_music();
	// The arena loop's gain: the stage's volume through the fade and the duck.
	float music_gain() const;

	const MatchTuning* match_ = nullptr;
	const Presentation* presentation_ = nullptr;

	// Every voice below, then each player's, in the order the constructor
	// registers them. Pause, resume and teardown go through this alone.
	MatchVoices voices_;

	MusicVoice music_;
	MatchVoice end_sting_;
	MatchVoice zoom_out_sound_;
	MatchVoice countdown_sound_;
	MatchVoice go_sound_;
	MatchVoice time_up_sound_;
	MatchVoice last_ten_sound_;
	float music_volume_ = 0.0f;
	float end_sting_volume_ = 0.0f;
	MusicEnvelope music_fade_;
	MusicEnvelope music_duck_{ 1.0f };
	// Set by start_music, as Level sets countdown_started_.
	bool music_started_ = false;
	bool music_ending_ = false;

	// Room cues are owned voices so a paused or skipped opening cannot leave
	// its person making a noise after the silhouette has stopped or gone.
	MatchVoice sigh_sound_;
	MatchVoice chair_sound_;
	MatchVoice room_tone_sound_;
	bool room_tone_playing_ = false;
	MatchVoice glass_in_sound_;
	MatchVoice celebrate_sound_;
	MatchVoice slump_sound_;
	MatchVoice colour_in_sound_;
	MatchVoice skip_sound_;
	MatchVoice undo_sound_;
};
