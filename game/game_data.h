#pragma once

#include "engine/app/application.h"
#include "engine/core/registry.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include "game/save/save.h"
#include "game/config/settings_session.h"
#include "game/config/stage_catalog.h"
#include "game/config/locale.h"
#include "game/config/presentation/presentation.h"
#include "game/config/tuning.h"
#include "game/debug/debug_director.h"

// Borrowed application and game services for the shell: states, menu pages,
// MenuContext, StagePreview and the developer tools. Nothing in game/objects,
// game/ai or game/carried names it; a state unpacks it into a builder, which
// hands each object only what it uses (docs/architecture.md).
// wWinMain owns the game data before Application, so it remains valid during
// state teardown. Engine services are forwarded through Application.
class GameData
{
public:
    GameData() = default;

    void set_application(labrador::Application* application);
    void set_save(Save* save);
    void set_settings_session(SettingsSession* settings_session);
    void set_stage_catalog(const StageCatalog* stage_catalog);
    void set_tuning(const GameTuning* tuning);
    void set_presentation(const Presentation* presentation);
    void set_locale(const ActiveLocale* locale);
    void set_debug(DebugDirector* debug);

    // Game-owned services. The save is what the file holds; the settings in
    // effect this run, with worn profiles laid over their pads, and which pad
    // wears which profile, are the settings session's.
    Save* save() const;
    SettingsSession* settings_session() const;

    // The stage list and the level definitions behind it, written only by
    // the manifest load and UserStageFolder. User-stage rescans occur only
    // while no match holds a definition.
    const StageCatalog* stage_catalog() const;

    // The catalog's two halves, or nullptr when no catalog was set. Manifest
    // stages in manifest order, then accepted user stages in file-name order.
    const StageList* stages() const;
    const labrador::Registry<LevelLoadedInfo>* level_infos() const;

    // Gameplay tuning loaded at startup and shared by live matches.
    const GameTuning* tuning() const;

    // Presentation settings loaded at startup and shared by menus and HUDs.
    const Presentation* presentation() const;

    // The language this run's presentation is in. Whatever rebuilds the
    // presentation (Debug `reload` and `tune`) composes it through this, so
    // the copy stays in that language. Null when none was set.
    const ActiveLocale* locale() const;

    // Developer tools (console, launch arguments, debug clock). Inert in
    // Release, and inert when no director was set.
    DebugDirector& debug() const;

    // Application and its services remain valid while its states use them.
    labrador::Application* application() const;
    labrador::ResolutionManager* resolution_manager() const;
    labrador::RenderResources* render_resources() const;
    labrador::AudioResources* audio_resources() const;
    labrador::Gamepads* gamepads() const;

    // Keyboard and mouse share one input slot; see game/input/input_slots.h.
    labrador::Keyboard* keyboard() const;
    labrador::Mouse* mouse() const;
    labrador::ViewportManager* viewport_manager() const;
    labrador::ThreadPool* thread_pool() const;
    const labrador::Partitioner* partitioner() const;

private:
    labrador::Application* application_ = nullptr;
    Save* save_ = nullptr;
    SettingsSession* settings_session_ = nullptr;

    const StageCatalog* stage_catalog_ = nullptr;
    const GameTuning* tuning_ = nullptr;
    const Presentation* presentation_ = nullptr;
    const ActiveLocale* locale_ = nullptr;
    DebugDirector* debug_ = nullptr;
};
