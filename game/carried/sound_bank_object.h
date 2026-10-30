#pragma once

// Carried from Labrador v0.2.0, which removes this type on master (730dd99,
// "A game that used one should copy the class ... into its own source").
// Shape unchanged but for the play calls, no longer const (HYG#49); it lives
// in colourwars:: so it can never be ambiguous with an engine type. See
// docs/review/engine-reports.md, HYG#25.

#include "engine/audio/sound_bank.h"
#include "engine/audio/audio_resources.h"
#include <string>

namespace colourwars
{
	// Anything that makes noise, inherited for the plumbing: it holds the bank
	// handle and forwards the calls, so a subclass writes play_wave(x) instead of
	// audio_resources->sound_bank(handle)->play_wave(x).
	//
	// Names go in at construction and handles come out; the play calls take
	// handles only. A subclass therefore resolves what it can make a noise with
	// once, in its constructor, and keeps the handles as members - which is why
	// the resolve helpers are here and the name-taking play calls are not.
	class SoundBankObject
	{
	public:
		// A subclass that has not been given an AudioResources, which every
		// object drawn by this engine has to be capable of being: draw objects
		// are default-constructible. Nothing can fill the table in afterwards -
		// set_sound_bank changes which bank, not which table it comes out of -
		// so an object built this way is permanently unusable, and both ways of
		// touching it throw std::logic_error saying so. That is T6 rather than
		// a null dereference in somebody else's frame.
		SoundBankObject() = default;

		// Throws std::out_of_range naming the bank if nothing loaded it, and
		// std::logic_error for a null table.
		SoundBankObject(const std::string& sound_bank_name,
			const labrador::AudioResources* audio_resources);
	protected:
		labrador::SoundBank* sound_bank() const;

		// Load-time. Each throws std::out_of_range naming what was asked for if
		// the bank does not have it, so a misspelt sound fails while the menu is
		// being built rather than staying silent on the press that wanted it.
		labrador::SoundBank::WaveHandle resolve_wave(const std::string& wave_name) const;
		labrador::SoundBank::EffectHandle resolve_effect(const std::string& effect_name) const;

		// Non-const, unlike the engine's original: a call that makes or changes
		// a sound is never one a draw may make (HYG#49, docs/oop-guide.md).
		void play_wave(labrador::SoundBank::WaveHandle wave,
			float volume = 1.0f, float pitch = 0.0f, float pan = 0.0f);
		void play_effect(labrador::SoundBank::EffectHandle effect,
			bool loop = false, float volume = 1.0f, float pitch = 0.0f,
			float pan = 0.0f);
		void stop_effect(labrador::SoundBank::EffectHandle effect,
			bool immediate = false);
		void pause_effect(labrador::SoundBank::EffectHandle effect);
		void resume_effect(labrador::SoundBank::EffectHandle effect);
		void set_effect_volume(labrador::SoundBank::EffectHandle effect, float volume);
		void set_effect_pitch(labrador::SoundBank::EffectHandle effect, float pitch);
		void set_effect_pan(labrador::SoundBank::EffectHandle effect, float pan);
		labrador::SoundState effect_state(labrador::SoundBank::EffectHandle effect) const;
		bool is_effect_looping(labrador::SoundBank::EffectHandle effect) const;

		// Points this object at a different bank. Every handle resolved from the
		// old one is meaningless against the new one, so a caller doing this
		// re-resolves everything it holds.
		void set_sound_bank(const std::string& sound_bank_name);
	private:
		// The bank name is resolved once, at construction, and so is everything
		// played out of it: a handle is an index, where a name is a map descent
		// and a string compare per node.
		labrador::AudioResources::SoundBankHandle sound_bank_;
		const labrador::AudioResources* audio_resources_ = nullptr;
	};
}
