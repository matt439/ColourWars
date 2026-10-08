#pragma once

#include "engine/app/application.h"
#include "engine/core/registry.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include "game/save.h"
#include "game/presentation.h"
#include "game/tuning.h"
#include "game/debug/debug_director.h"

// Borrowed application and game services shared with states and objects.
// wWinMain owns the game data before Application, so it remains valid during
// state teardown. Engine services are forwarded through Application.
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
    void set_debug(DebugDirector* debug);

    // Game-owned services.
    Save* save() const;
    labrador::Registry<LevelLoadedInfo>* level_infos() const;

    // Stages in manifest order, for stage selection.
    const StageList* stages() const;

    // Gameplay tuning loaded at startup and shared by live matches.
    const GameTuning* tuning() const;

    // Presentation settings loaded at startup and shared by menus and HUDs.
    const Presentation* presentation() const;

    // Developer tools (console, launch arguments, debug clock). Inert in
    // Release, and inert when no director was set.
    DebugDirector& debug() const;

    // Application and its services remain valid while its states use them.
    labrador::Application* application() const;
    labrador::ResolutionManager* resolution_manager() const;
    labrador::RenderResources* render_resources() const;
    labrador::AudioResources* audio_resources() const;
    labrador::Gamepads* gamepads() const;

    // Keyboard and mouse share one input slot; see game/input_slots.h.
    labrador::Keyboard* keyboard() const;
    labrador::Mouse* mouse() const;
    labrador::ViewportManager* viewport_manager() const;
    labrador::ThreadPool* thread_pool() const;
    const labrador::Partitioner* partitioner() const;

private:
    labrador::Application* application_ = nullptr;
    Save* save_ = nullptr;

    labrador::Registry<LevelLoadedInfo>* level_infos_ = nullptr;
    const StageList* stages_ = nullptr;
    const GameTuning* tuning_ = nullptr;
    const Presentation* presentation_ = nullptr;
    DebugDirector* debug_ = nullptr;
};
