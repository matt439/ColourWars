#ifndef GAMEDATA_H
#define GAMEDATA_H

#include "ResolutionManager.h"
#include "Save.h"
#include "ResourceLoader.h"
#include "ResourceManager.h"
#include "DeviceResources.h"
#include "ViewportManager.h"

class GameData
{
private:
    Save* _save = nullptr;
    ResolutionManager* _resolution_manager = nullptr;
    HWND _window = nullptr;
    float* _dt = nullptr;
    DirectX::SpriteBatch* _sprite_batch = nullptr;
    ResourceLoader* _resource_loader = nullptr;
    ResourceManager* _resource_manager = nullptr;
    DirectX::GamePad* _gamepad = nullptr;
    DX::DeviceResources* _device_resources = nullptr;
    ViewportManager* _viewport_manager = nullptr;
    DirectX::CommonStates* _common_states = nullptr;

public:
    GameData() {}
    GameData(GameData* game_data) :
        _save(game_data->get_save()),
        _resolution_manager(game_data->get_resolution_manager()),
        _window(game_data->get_window()),
        _dt(game_data->get_dt()),
        _sprite_batch(game_data->get_sprite_batch()),
        _resource_loader(game_data->get_resource_loader()),
        _resource_manager(game_data->get_resource_manager()),
        _gamepad(game_data->get_gamepad()),
        _device_resources(game_data->get_device_resources()),
        _viewport_manager(game_data->get_viewport_manager()),
        _common_states(game_data->get_common_states()) {}

    void set_window(HWND window) { this->_window = window; }
    HWND get_window() { return this->_window; }
    void set_resolution_manager(ResolutionManager* resolution_manager) { this->_resolution_manager = resolution_manager; }
    ResolutionManager* get_resolution_manager() { return this->_resolution_manager; }
    float* get_dt() { return this->_dt; }
    void set_dt(float* dt) { this->_dt = dt; }
    DirectX::SpriteBatch* get_sprite_batch() { return this->_sprite_batch; }
    void set_sprite_batch(DirectX::SpriteBatch* sprite_batch) { this->_sprite_batch = sprite_batch; }
    void set_save(Save* save) { this->_save = save; }
    Save* get_save() { return this->_save; }
    void set_resource_loader(ResourceLoader* texture_loader) { this->_resource_loader = texture_loader; }
    ResourceLoader* get_resource_loader() { return this->_resource_loader; }
    void set_resource_manager(ResourceManager* resource_manager) { this->_resource_manager = resource_manager; }
    ResourceManager* get_resource_manager() { return this->_resource_manager; }
    void set_gamepad(DirectX::GamePad* gamepad) { this->_gamepad = gamepad; }
    DirectX::GamePad* get_gamepad() { return this->_gamepad; }
    void set_device_resources(DX::DeviceResources* device_resources) { this->_device_resources = device_resources; }
    DX::DeviceResources* get_device_resources() { return this->_device_resources; }
    void set_viewport_manager(ViewportManager* viewport_manager) { this->_viewport_manager = viewport_manager; }
    ViewportManager* get_viewport_manager() { return this->_viewport_manager; }
    void set_common_states(DirectX::CommonStates* common_states) { this->_common_states = common_states; }
    DirectX::CommonStates* get_common_states() { return this->_common_states; }

    GameData* get_game_data() { return this; }
};
#endif // !GAMEDATA_H
