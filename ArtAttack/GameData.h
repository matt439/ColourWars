#ifndef GAMEDATA_H
#define GAMEDATA_H

#include "engine/render/resolution_manager.h"
#include "Save.h"
#include "ResourceLoader.h"
#include "LevelLoadedInfo.h"
#include "engine/assets/resource_manager.h"
#include "engine/assets/registry.h"
#include "engine/render/device_resources.h"
#include "engine/render/viewport_manager.h"
#include "engine/core/thread_pool.h"
#include "engine/collision/partitioner.h"

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
    void set_save(Save* save);
    Save* get_save() const;
    void set_resource_loader(ResourceLoader* texture_loader);
    ResourceLoader* get_resource_loader() const;
    void set_resource_manager(ResourceManager* resource_manager);
    ResourceManager* get_resource_manager() const;
    void set_level_infos(Registry<LevelLoadedInfo>* level_infos);
    Registry<LevelLoadedInfo>* get_level_infos() const;
    void set_gamepad(DirectX::GamePad* gamepad);
    DirectX::GamePad* get_gamepad() const;
    void set_device_resources(DX::DeviceResources* device_resources);
    DX::DeviceResources* get_device_resources() const;
    void set_viewport_manager(ViewportManager* viewport_manager);
    ViewportManager* get_viewport_manager() const;
    void set_common_states(DirectX::CommonStates* common_states);
    DirectX::CommonStates* get_common_states() const;
    std::vector<DirectX::SpriteBatch*>* get_sprite_batches() const;
    void set_sprite_batches(std::vector<DirectX::SpriteBatch*>* sprite_batches);
    void set_thread_pool(ThreadPool* thread_pool);
    ThreadPool* get_thread_pool() const;
	void set_partitioner(const Partitioner* partitioner);
	const Partitioner* get_partitioner() const;

    GameData* get_game_data();

private:
    Save* _save = nullptr;
    ResolutionManager* _resolution_manager = nullptr;
    HWND _window = nullptr;
    float* _dt = nullptr;
    ResourceLoader* _resource_loader = nullptr;
    ResourceManager* _resource_manager = nullptr;

    // The game's own resource registry, beside the engine's: a level
    // definition means nothing to the engine, so the engine does not cache it.
    Registry<LevelLoadedInfo>* _level_infos = nullptr;

    DirectX::GamePad* _gamepad = nullptr;
    DX::DeviceResources* _device_resources = nullptr;
    ViewportManager* _viewport_manager = nullptr;
    DirectX::CommonStates* _common_states = nullptr;
	std::vector<DirectX::SpriteBatch*>* _sprite_batches = nullptr;
    ThreadPool* _thread_pool = nullptr;
	const Partitioner* _partitioner = nullptr;
};
#endif // !GAMEDATA_H
