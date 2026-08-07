#ifndef GAMEDATA_H
#define GAMEDATA_H

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

    void set_application(Application* application);
    void set_save(Save* save);
    void set_level_infos(Registry<LevelLoadedInfo>* level_infos);

    // The game's own two.
    Save* get_save() const;
    Registry<LevelLoadedInfo>* get_level_infos() const;

    // The engine's, forwarded. Created once by the shell and never reseated,
    // so anything here may be held for an object's whole life.
    Application* get_application() const;
    ResolutionManager* get_resolution_manager() const;
    float* get_dt() const;
    RenderResources* get_render_resources() const;
    AudioResources* get_audio_resources() const;
    DirectX::GamePad* get_gamepad() const;
    DX::DeviceResources* get_device_resources() const;
    ViewportManager* get_viewport_manager() const;
    ThreadPool* get_thread_pool() const;
    const Partitioner* get_partitioner() const;

    // Recreated on device loss, so unlike the rest these change identity
    // across one and must be read through this each time they are needed.
    DirectX::CommonStates* get_common_states() const;
    std::vector<DirectX::SpriteBatch*>* get_sprite_batches() const;

    GameData* get_game_data();

private:
    Application* _application = nullptr;
    Save* _save = nullptr;

    // The game's own resource registry, beside the engine's: a level
    // definition means nothing to the engine, so the engine does not cache it.
    Registry<LevelLoadedInfo>* _level_infos = nullptr;
};
#endif // !GAMEDATA_H
