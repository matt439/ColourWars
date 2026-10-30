#pragma once

#include "game/carried/sound_bank_object.h"
#include "engine/audio/audio_resources.h"
#include "engine/audio/sound_bank.h"
#include <string>
#include <vector>

// One effect voice a match owns: a bank instance its owner plays and stops,
// and that MatchVoices pauses, resumes and stops with every other one. Like
// every game object that makes a noise, it reaches its bank as a
// SoundBankObject. Built without an effect name it never sounds - a stage
// with no music, a tool with no loop - and every call on it does nothing.
class MatchVoice : public colourwars::SoundBankObject
{
public:
	// Unusable until assigned, for an owner that resolves its voices in its
	// constructor body, after the rest of its resources.
	MatchVoice() = default;
	// Throws what SoundBankObject throws for the bank, and std::out_of_range
	// naming the effect if the bank does not have it.
	MatchVoice(const std::string& sound_bank_name,
		const labrador::AudioResources* audio_resources,
		const std::string& effect_name);
	virtual ~MatchVoice() = default;
	MatchVoice(const MatchVoice&) = default;
	MatchVoice& operator=(const MatchVoice&) = default;

	bool valid() const;
	// Whether the bank makes a noise at all; false for a silent bank.
	bool audible() const;
	void play(bool loop, float volume);
	void set_volume(float volume);
	labrador::SoundState state() const;

	// What the match does to every voice it holds. These pause, resume and
	// immediately stop the instance, whatever it is doing; an owner whose
	// voice keeps its own playing state overrides them.
	virtual void pause();
	virtual void resume();
	virtual void stop();

private:
	labrador::SoundBank::EffectHandle effect_;
};

// The match's one list of voices. The Level registers its own and each
// player's at construction, in that order, and pause, resume and teardown go
// through here rather than through each owner in turn.
class MatchVoices
{
public:
	// Borrowed: each voice must outlive every later call on this list. The
	// Level's voices, its players' and their tools' all live for the match.
	void add(MatchVoice& voice);

	// In registration order.
	void pause();
	void resume();
	void stop();

private:
	std::vector<MatchVoice*> voices_;
};
