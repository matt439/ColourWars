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
public:
    GameData() = default;
    explicit GameData(const GameData* game_data);

    void set_window(HWND window);
    HWND get_window() const;
    void set_resolution_manager(ResolutionManager* resolution_manager);
    ResolutionManager* get_resolution_manager() const;
    float* get_dt() const;
    void set_dt(float* dt);
    DirectX::SpriteBatch* get_sprite_batch() const;
    void set_sprite_batch(DirectX::SpriteBatch* sprite_batch);
    void set_save(Save* save);
    Save* get_save() const;
    void set_resource_loader(ResourceLoader* texture_loader);
    ResourceLoader* get_resource_loader() const;
    void set_resource_manager(ResourceManager* resource_manager);
    ResourceManager* get_resource_manager() const;
    void set_gamepad(DirectX::GamePad* gamepad);
    DirectX::GamePad* get_gamepad() const;
    void set_device_resources(DX::DeviceResources* device_resources);
    DX::DeviceResources* get_device_resources() const;
    void set_viewport_manager(ViewportManager* viewport_manager);
    ViewportManager* get_viewport_manager() const;
    void set_common_states(DirectX::CommonStates* common_states);
    DirectX::CommonStates* get_common_states() const;
    std::vector<DirectX::SpriteBatch*>* get_sprite_batches() const { return this->_sprite_batches; }
    void set_sprite_batches(std::vector<DirectX::SpriteBatch*>* sprite_batches) { this->_sprite_batches = sprite_batches; }
    void set_thread_pool(ThreadPool* thread_pool) { this->_thread_pool = thread_pool; }
    ThreadPool* get_thread_pool() const { return this->_thread_pool; }

    GameData* get_game_data();

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
    std::vector<DirectX::SpriteBatch*>* _sprite_batches;
    ThreadPool* _thread_pool = nullptr;
};
#endif // !GAMEDATA_H
