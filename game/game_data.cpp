#include "game/pch.h"
#include "game/game_data.h"

GameData::GameData(const GameData* game_data) :
	application_(game_data->get_application()),
	save_(game_data->get_save()),
	level_infos_(game_data->get_level_infos())
{

}

void GameData::set_application(Application* application)
{
	this->application_ = application;
}

void GameData::set_save(Save* save)
{
	this->save_ = save;
}

void GameData::set_level_infos(Registry<LevelLoadedInfo>* level_infos)
{
	this->level_infos_ = level_infos;
}

Application* GameData::get_application() const
{
	return this->application_;
}

Save* GameData::get_save() const
{
	return this->save_;
}

Registry<LevelLoadedInfo>* GameData::get_level_infos() const
{
	return this->level_infos_;
}

// Everything below forwards. There is deliberately no cached copy of any of
// them: a copy would be a second place for a service pointer to be wrong.

ResolutionManager* GameData::get_resolution_manager() const
{
	return this->application_->resolution_manager();
}

float* GameData::get_dt() const
{
	// The shell hands out a const float* because nothing downstream should be
	// writing the frame's own timestep. The game's older signatures still take
	// a float*, so the cast lives here, in one place, rather than at each of
	// the several dozen call sites - and goes when those signatures do.
	return const_cast<float*>(this->application_->dt());
}

RenderResources* GameData::get_render_resources() const
{
	return this->application_->render_resources();
}

AudioResources* GameData::get_audio_resources() const
{
	return this->application_->audio_resources();
}

DirectX::GamePad* GameData::get_gamepad() const
{
	return this->application_->gamepad();
}

DX::DeviceResources* GameData::get_device_resources() const
{
	return this->application_->device_resources();
}

ViewportManager* GameData::get_viewport_manager() const
{
	return this->application_->viewport_manager();
}

ThreadPool* GameData::get_thread_pool() const
{
	return this->application_->thread_pool();
}

const Partitioner* GameData::get_partitioner() const
{
	return this->application_->partitioner();
}

DirectX::CommonStates* GameData::get_common_states() const
{
	return this->application_->common_states();
}

std::vector<DirectX::SpriteBatch*>* GameData::get_sprite_batches() const
{
	return this->application_->sprite_batches();
}

GameData* GameData::get_game_data()
{
	return this;
}
