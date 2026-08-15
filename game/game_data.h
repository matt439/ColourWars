#pragma once

#include "engine/app/application.h"
#include "engine/core/registry.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include "game/save.h"
#include "game/presentation.h"
#include "game/tuning.h"

// The service bag every state, object and drawable in this game is handed at
// construction.
//
// Six things go in it, and only six: the engine's application shell, this
// game's save file, this game's level registry, the stage list built beside it,
// the numbers a match is balanced on and what the shell looks like. Everything
// below that
// looks like a service is a forward to the shell, so the accessors the whole
// game already calls keep working while the ownership sits where it belongs -
// a game that owned a ThreadPool would be a game doing the engine's job (T1).
//
// All six are borrowed and all six outlive it: main() declares them above
// the Application whose states hold this.
class GameData
{
public:
    GameData() = default;

    void set_application(labrador::Application* application);
    void set_save(Save* save);
    void set_level_infos(labrador::Registry<LevelLoadedInfo>* level_infos);
    void set_stages(const StageList* stages);
    void set_tuning(const GameTuning* tuning);
    void set_presentation(const Presentation* presentation);

    // The game's own four.
    Save* save() const;
    labrador::Registry<LevelLoadedInfo>* level_infos() const;

    // Every stage the manifest named, in manifest order. The stage-select
    // menu walks it; nothing else needs to know how many there are.
    const StageList* stages() const;

    // Every number a paint match is balanced on, read from ./tuning.json at
    // startup. Borrowed by every player, weapon and projectile in a level, so
    // it outlives all of them by construction - main() owns it.
    const GameTuning* tuning() const;

    // Every font, colour, offset and box size the menus, the HUD and the debug
    // overlay are laid out from, read from ./presentation.json at startup.
    // Borrowed for the same reason the tuning is, and by the same owner.
    const Presentation* presentation() const;

    // The engine's, forwarded. Created once by the shell and never reseated,
    // so anything here may be held for an object's whole life.
    labrador::Application* application() const;
    labrador::ResolutionManager* resolution_manager() const;
    labrador::RenderResources* render_resources() const;
    labrador::AudioResources* audio_resources() const;
    labrador::Gamepads* gamepads() const;
    labrador::ViewportManager* viewport_manager() const;
    labrador::ThreadPool* thread_pool() const;
    const labrador::Partitioner* partitioner() const;

private:
    labrador::Application* application_ = nullptr;
    Save* save_ = nullptr;

    // The game's own resource registry, beside the engine's: a level
    // definition means nothing to the engine, so the engine does not cache it.
    labrador::Registry<LevelLoadedInfo>* level_infos_ = nullptr;
    const StageList* stages_ = nullptr;
    const GameTuning* tuning_ = nullptr;
    const Presentation* presentation_ = nullptr;
};
