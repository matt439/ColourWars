#include "game/pch.h"
#include "game/states/menu_context.h"

MenuContext::MenuContext(GameData* data, MenuInput* input,
	MatchSetup* level_settings, MenuOwner* owner) :
	data_(data), input_(input), level_settings_(level_settings), owner_(owner)
{

}

GameData* MenuContext::data() const
{
	return this->data_;
}

MenuInput* MenuContext::input() const
{
	return this->input_;
}

MatchSetup* MenuContext::level_settings() const
{
	return this->level_settings_;
}

MenuOwner* MenuContext::owner() const
{
	return this->owner_;
}

void MenuContext::set_owner(MenuOwner* owner)
{
	this->owner_ = owner;
}

void MenuContext::reset_connection_audio()
{
	this->connection_audio_ = std::make_unique<ConnectionAudio>(this->data_->audio_resources(),
		*this->data_->presentation());
}

void MenuContext::update_music(float dt)
{
	if (this->owner_ != nullptr) { this->owner_->update_menu_music(dt); }
}

void MenuContext::suspend_music()
{
	if (this->owner_ != nullptr) { this->owner_->suspend_menu_music(); }
}

void MenuContext::rescan_stages()
{
	if (this->owner_ != nullptr) { this->owner_->rescan_stages(); }
}

const UserStageScan* MenuContext::stage_scan() const
{
	return this->owner_ != nullptr ? this->owner_->stage_scan() : nullptr;
}

void MenuContext::finish_with_music(std::function<void()> finish)
{
	if (this->music_exit_) { return; }
	if (this->owner_ == nullptr) { finish(); return; }
	this->music_exit_ = std::move(finish);
	// Begin at the current gain; a muted, missing or drained voice can leave
	// immediately, without making the player wait for inaudible music.
	this->update_music(0.0f);
}

bool MenuContext::music_exit_pending() const { return static_cast<bool>(this->music_exit_); }

void MenuContext::complete_music_exit()
{
	std::function<void()> finish = std::move(this->music_exit_);
	this->music_exit_ = nullptr;
	if (finish) { finish(); }
}

void MenuContext::cancel_music_exit() { this->music_exit_ = nullptr; }

void MenuContext::update_connection_audio()
{
	if (!this->connection_audio_) { return; }
	ConnectionAudio::Pads eligible{};
	ConnectionAudio::Pads connected{};
	// By seat: a seat a returning pad reclaimed sounds "back", not a new pad.
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		eligible[slot] = this->level_settings_->player_index_for_slot(slot) != -1 &&
			!this->data_->debug().drives_slot(slot);
		connected[slot] = this->data_->gamepads()->connected(
			this->level_settings_->device_slot(slot));
	}
	this->connection_audio_->update(eligible, connected);
}
