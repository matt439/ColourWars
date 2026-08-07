#include "game/pch.h"
#include "game/game_data.h"

GameData::GameData(const GameData* game_data) :
	_application(game_data->get_application()),
	_save(game_data->get_save()),
	_level_infos(game_data->get_level_infos())
{

}

void GameData::set_application(Application* application)
{
	this->_application = application;
}

void GameData::set_save(Save* save)
{
	this->_save = save;
}

void GameData::set_level_infos(Registry<LevelLoadedInfo>* level_infos)
{
	this->_level_infos = level_infos;
}

Application* GameData::get_application() const
{
	return this->_application;
}

Save* GameData::get_save() const
{
	return this->_save;
}

Registry<LevelLoadedInfo>* GameData::get_level_infos() const
{
	return this->_level_infos;
}

// Everything below forwards. There is deliberately no cached copy of any of
// them: a copy would be a second place for a service pointer to be wrong.

ResolutionManager* GameData::get_resolution_manager() const
{
	return this->_application->resolution_manager();
}

float* GameData::get_dt() const
{
	// The shell hands out a const float* because nothing downstream should be
	// writing the frame's own timestep. The game's older signatures still take
	// a float*, so the cast lives here, in one place, rather than at each of
	// the several dozen call sites - and goes when those signatures do.
	return const_cast<float*>(this->_application->dt());
}

RenderResources* GameData::get_render_resources() const
{
	return this->_application->render_resources();
}

AudioResources* GameData::get_audio_resources() const
{
	return this->_application->audio_resources();
}

DirectX::GamePad* GameData::get_gamepad() const
{
	return this->_application->gamepad();
}

DX::DeviceResources* GameData::get_device_resources() const
{
	return this->_application->device_resources();
}

ViewportManager* GameData::get_viewport_manager() const
{
	return this->_application->viewport_manager();
}

ThreadPool* GameData::get_thread_pool() const
{
	return this->_application->thread_pool();
}

const Partitioner* GameData::get_partitioner() const
{
	return this->_application->partitioner();
}

DirectX::CommonStates* GameData::get_common_states() const
{
	return this->_application->common_states();
}

std::vector<DirectX::SpriteBatch*>* GameData::get_sprite_batches() const
{
	return this->_application->sprite_batches();
}

GameData* GameData::get_game_data()
{
	return this;
}
