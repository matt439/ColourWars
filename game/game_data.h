#pragma once

#include "engine/app/application.h"
#include "engine/core/registry.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include "game/save.h"
#include "game/tuning.h"

// The service bag every state, object and drawable in this game is handed at
// construction.
//
// Five things go in it, and only five: the engine's application shell, this
// game's save file, this game's level registry, the stage list built beside it
// and the numbers a match is balanced on. Everything below that
// looks like a service is a forward to the shell, so the accessors the whole
// game already calls keep working while the ownership sits where it belongs -
// a game that owned a ThreadPool would be a game doing the engine's job (T1).
//
// All five are borrowed and all five outlive it: main() declares them above
// the Application whose states hold this.
class GameData
{
public:
    GameData() = default;
    explicit GameData(const GameData* game_data);

    void set_application(artattack::Application* application);
    void set_save(Save* save);
    void set_level_infos(artattack::Registry<LevelLoadedInfo>* level_infos);
    void set_stages(const StageList* stages);
    void set_tuning(const GameTuning* tuning);

    // The game's own four.
    Save* save() const;
    artattack::Registry<LevelLoadedInfo>* level_infos() const;

    // Every stage the manifest named, in manifest order. The stage-select
    // menu walks it; nothing else needs to know how many there are.
    const StageList* stages() const;

    // Every number a paint match is balanced on, read from ./tuning.json at
    // startup. Borrowed by every player, weapon and projectile in a level, so
    // it outlives all of them by construction - main() owns it.
    const GameTuning* tuning() const;

    // The engine's, forwarded. Created once by the shell and never reseated,
    // so anything here may be held for an object's whole life.
    artattack::Application* application() const;
    artattack::ResolutionManager* resolution_manager() const;
    artattack::RenderResources* render_resources() const;
    artattack::AudioResources* audio_resources() const;
    artattack::Gamepads* gamepads() const;
    artattack::ViewportManager* viewport_manager() const;
    artattack::ThreadPool* thread_pool() const;
    const artattack::Partitioner* partitioner() const;

    GameData* game_data();

private:
    artattack::Application* application_ = nullptr;
    Save* save_ = nullptr;

    // The game's own resource registry, beside the engine's: a level
    // definition means nothing to the engine, so the engine does not cache it.
    artattack::Registry<LevelLoadedInfo>* level_infos_ = nullptr;
    const StageList* stages_ = nullptr;
    const GameTuning* tuning_ = nullptr;
};
