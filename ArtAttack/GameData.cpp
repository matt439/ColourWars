#include "pch.h"
#include "GameData.h"

GameData::GameData(const GameData* game_data) :
    _save(game_data->get_save()),
    _resolution_manager(game_data->get_resolution_manager()),
    _window(game_data->get_window()),
    _dt(game_data->get_dt()),
    _resource_loader(game_data->get_resource_loader()),
    _resource_manager(game_data->get_resource_manager()),
    _gamepad(game_data->get_gamepad()),
    _device_resources(game_data->get_device_resources()),
    _viewport_manager(game_data->get_viewport_manager()),
    _common_states(game_data->get_common_states()),
	_sprite_batches(game_data->get_sprite_batches()),
	_thread_pool(game_data->get_thread_pool())
{

}

void GameData::set_window(HWND window)
{
	this->_window = window;
}

HWND GameData::get_window() const
{
	return this->_window;
}

void GameData::set_resolution_manager(ResolutionManager* resolution_manager)
{
	this->_resolution_manager = resolution_manager;
}

ResolutionManager* GameData::get_resolution_manager() const
{
	return this->_resolution_manager;
}

float* GameData::get_dt() const
{
	return this->_dt;
}

void GameData::set_dt(float* dt)
{
	this->_dt = dt;
}

void GameData::set_save(Save* save)
{
	this->_save = save;
}

Save* GameData::get_save() const
{
	return this->_save;
}

void GameData::set_resource_loader(ResourceLoader* texture_loader)
{
	this->_resource_loader = texture_loader;
}

ResourceLoader* GameData::get_resource_loader() const
{
	return this->_resource_loader;
}

void GameData::set_resource_manager(ResourceManager* resource_manager)
{
	this->_resource_manager = resource_manager;
}

ResourceManager* GameData::get_resource_manager() const
{
	return this->_resource_manager;
}

void GameData::set_gamepad(DirectX::GamePad* gamepad)
{
	this->_gamepad = gamepad;
}

DirectX::GamePad* GameData::get_gamepad() const
{
	return this->_gamepad;
}

void GameData::set_device_resources(DX::DeviceResources* device_resources)
{
	this->_device_resources = device_resources;
}

DX::DeviceResources* GameData::get_device_resources() const
{
	return this->_device_resources;
}

void GameData::set_viewport_manager(ViewportManager* viewport_manager)
{
	this->_viewport_manager = viewport_manager;
}

ViewportManager* GameData::get_viewport_manager() const
{
	return this->_viewport_manager;
}

void GameData::set_common_states(DirectX::CommonStates* common_states)
{
	this->_common_states = common_states;
}

DirectX::CommonStates* GameData::get_common_states() const
{
	return this->_common_states;
}

GameData* GameData::get_game_data()
{
	return this;
}

std::vector<DirectX::SpriteBatch*>* GameData::get_sprite_batches() const
{
	return this->_sprite_batches;
}

void GameData::set_sprite_batches(std::vector<DirectX::SpriteBatch*>* sprite_batches)
{
	this->_sprite_batches = sprite_batches;
}

void GameData::set_thread_pool(ThreadPool* thread_pool)
{
	this->_thread_pool = thread_pool;
}

ThreadPool* GameData::get_thread_pool() const
{
	return this->_thread_pool;
	return this->_thread_pool;
}