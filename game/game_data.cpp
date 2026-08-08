#include "game/pch.h"
#include "game/game_data.h"
using namespace artattack;

GameData::GameData(const GameData* game_data) :
	application_(game_data->application()),
	save_(game_data->save()),
	level_infos_(game_data->level_infos()),
	stages_(game_data->stages())
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

void GameData::set_stages(const StageList* stages)
{
	this->stages_ = stages;
}

Application* GameData::application() const
{
	return this->application_;
}

Save* GameData::save() const
{
	return this->save_;
}

const StageList* GameData::stages() const
{
	return this->stages_;
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

GameData* GameData::game_data()
{
	return this;
}
