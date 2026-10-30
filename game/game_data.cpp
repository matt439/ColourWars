#include "game/pch.h"
#include "game/game_data.h"
using namespace labrador;

void GameData::set_application(Application* application)
{
	this->application_ = application;
}

void GameData::set_save(Save* save)
{
	this->save_ = save;
}

void GameData::set_settings_session(SettingsSession* settings_session)
{
	this->settings_session_ = settings_session;
}

void GameData::set_stage_catalog(const StageCatalog* stage_catalog)
{
	this->stage_catalog_ = stage_catalog;
}

void GameData::set_tuning(const GameTuning* tuning)
{
	this->tuning_ = tuning;
}

void GameData::set_presentation(const Presentation* presentation)
{
	this->presentation_ = presentation;
}

void GameData::set_debug(DebugDirector* debug)
{
	this->debug_ = debug;
}

DebugDirector& GameData::debug() const
{
	return this->debug_ != nullptr ? *this->debug_ : DebugDirector::inert();
}

Application* GameData::application() const
{
	return this->application_;
}

Save* GameData::save() const
{
	return this->save_;
}

SettingsSession* GameData::settings_session() const
{
	return this->settings_session_;
}

const StageCatalog* GameData::stage_catalog() const
{
	return this->stage_catalog_;
}

const StageList* GameData::stages() const
{
	return this->stage_catalog_ != nullptr ? &this->stage_catalog_->stages() : nullptr;
}

const GameTuning* GameData::tuning() const
{
	return this->tuning_;
}

const Presentation* GameData::presentation() const
{
	return this->presentation_;
}

const Registry<LevelLoadedInfo>* GameData::level_infos() const
{
	return this->stage_catalog_ != nullptr ? &this->stage_catalog_->level_infos() : nullptr;
}


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

Gamepads* GameData::gamepads() const
{
	return this->application_->gamepads();
}

Keyboard* GameData::keyboard() const
{
	return this->application_->keyboard();
}

Mouse* GameData::mouse() const
{
	return this->application_->mouse();
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
