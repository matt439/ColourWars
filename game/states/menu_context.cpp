#include "game/pch.h"
#include "game/states/menu_context.h"

MenuContext::MenuContext(GameData* data, MenuInput* input,
	MenuLevelSettings* level_settings) :
	data_(data), input_(input), level_settings_(level_settings)
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

MenuLevelSettings* MenuContext::level_settings() const
{
	return this->level_settings_;
}

void MenuContext::reset_connection_audio()
{
	this->connection_audio_ = std::make_unique<ConnectionAudio>(this->data_->audio_resources(),
		*this->data_->presentation());
}

void MenuContext::set_music_update(std::function<void(float)> update)
{
	this->music_update_ = std::move(update);
}

void MenuContext::update_music(float dt)
{
	if (this->music_update_) { this->music_update_(dt); }
}

void MenuContext::set_music_suspend(std::function<void()> suspend)
{
	this->music_suspend_ = std::move(suspend);
}

void MenuContext::suspend_music()
{
	if (this->music_suspend_) { this->music_suspend_(); }
}

void MenuContext::set_stage_rescan(std::function<void()> rescan)
{
	this->stage_rescan_ = std::move(rescan);
}

void MenuContext::rescan_stages()
{
	if (this->stage_rescan_) { this->stage_rescan_(); }
}

void MenuContext::finish_with_music(std::function<void()> finish)
{
	if (this->music_exit_) { return; }
	if (!this->music_update_) { finish(); return; }
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
