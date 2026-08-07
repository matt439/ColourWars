#pragma once

#include "engine/app/application.h"
#include "engine/core/registry.h"
#include "game/objects/level_loaded_info.h"
#include "game/save.h"

// The service bag every state, object and drawable in this game is handed at
// construction.
//
// Three things go in it, and only three: the engine's application shell, this
// game's save file, and this game's level registry. Everything below that
// looks like a service is a forward to the shell, so the accessors the whole
// game already calls keep working while the ownership sits where it belongs -
// a game that owned a ThreadPool would be a game doing the engine's job (T1).
//
// All three are borrowed and all three outlive it: main() declares them above
// the Application whose states hold this.
class GameData
{
public:
    GameData() = default;
    explicit GameData(const GameData* game_data);

    void set_application(artattack::Application* application);
    void set_save(Save* save);
    void set_level_infos(artattack::Registry<LevelLoadedInfo>* level_infos);

    // The game's own two.
    Save* save() const;
    artattack::Registry<LevelLoadedInfo>* level_infos() const;

    // The engine's, forwarded. Created once by the shell and never reseated,
    // so anything here may be held for an object's whole life.
    artattack::Application* application() const;
    artattack::ResolutionManager* resolution_manager() const;
    float* dt() const;
    artattack::RenderResources* render_resources() const;
    artattack::AudioResources* audio_resources() const;
    DirectX::GamePad* gamepad() const;
    artattack::DeviceResources* device_resources() const;
    artattack::ViewportManager* viewport_manager() const;
    artattack::ThreadPool* thread_pool() const;
    const artattack::Partitioner* partitioner() const;

    // Recreated on device loss, so unlike the rest these change identity
    // across one and must be read through this each time they are needed.
    DirectX::CommonStates* common_states() const;
    std::vector<DirectX::SpriteBatch*>* sprite_batches() const;

    GameData* game_data();

private:
    artattack::Application* application_ = nullptr;
    Save* save_ = nullptr;

    // The game's own resource registry, beside the engine's: a level
    // definition means nothing to the engine, so the engine does not cache it.
    artattack::Registry<LevelLoadedInfo>* level_infos_ = nullptr;
};
