#include "game/pch.h"
#include "game/game_data.h"

GameData::GameData(const GameData* game_data) :
	application_(game_data->application()),
	save_(game_data->save()),
	level_infos_(game_data->level_infos())
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

Application* GameData::application() const
{
	return this->application_;
}

Save* GameData::save() const
{
	return this->save_;
}

Registry<LevelLoadedInfo>* GameData::level_infos() const
{
	return this->level_infos_;
}

// Everything below forwards. There is deliberately no cached copy of any of
// them: a copy would be a second place for a service pointer to be wrong.

ResolutionManager* GameData::resolution_manager() const
{
	return this->application_->resolution_manager();
}

float* GameData::dt() const
{
	// The shell hands out a const float* because nothing downstream should be
	// writing the frame's own timestep. The game's older signatures still take
	// a float*, so the cast lives here, in one place, rather than at each of
	// the several dozen call sites - and goes when those signatures do.
	return const_cast<float*>(this->application_->dt());
}

RenderResources* GameData::render_resources() const
{
	return this->application_->render_resources();
}

AudioResources* GameData::audio_resources() const
{
	return this->application_->audio_resources();
}

DirectX::GamePad* GameData::gamepad() const
{
	return this->application_->gamepad();
}

DX::DeviceResources* GameData::device_resources() const
{
	return this->application_->device_resources();
}

ViewportManager* GameData::viewport_manager() const
{
	return this->application_->viewport_manager();
}

ThreadPool* GameData::thread_pool() const
{
	return this->application_->thread_pool();
}

const Partitioner* GameData::partitioner() const
{
	return this->application_->partitioner();
}

DirectX::CommonStates* GameData::common_states() const
{
	return this->application_->common_states();
}

std::vector<DirectX::SpriteBatch*>* GameData::sprite_batches() const
{
	return this->application_->sprite_batches();
}

GameData* GameData::game_data()
{
	return this;
}
