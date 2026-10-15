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
		&this->data_->save()->play_settings(), *this->data_->presentation());
}

void MenuContext::update_connection_audio()
{
	if (!this->connection_audio_) { return; }
	ConnectionAudio::Pads eligible{};
	ConnectionAudio::Pads connected{};
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		eligible[slot] = this->level_settings_->player_index_for_slot(slot) != -1 &&
			!this->data_->debug().drives_slot(slot);
		connected[slot] = this->data_->gamepads()->connected(slot);
	}
	this->connection_audio_->update(eligible, connected);
}
