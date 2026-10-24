#include "game/pch.h"
#include "game/display_awake.h"
#include "game/states/game_states.h"
#include "game/game_data.h"
#include "game/save.h"
#include "game/sound_groups.h"
#include "engine/app/application.h"
#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

using namespace labrador;
namespace display_awake_probe
{
    struct Event { const char* name = ""; bool keep = false, returned = false, client_after = false; DWORD thread = 0; };
    struct Case { const char* name = ""; size_t first = 0, last = 0; bool client_after = false; };
    std::array<Event, 32> events{}; std::array<Case, 9> cases{};
    size_t event_count = 0, case_count = 0, game_initializations = 0, real_restarts = 0, state_updates = 0, positive_updates = 0, keyboard_polls = 0, mouse_polls = 0, key_taps = 0;
    DWORD ui_thread = 0; std::atomic<DWORD> unexpected_thread{ 0 }; const char* phase = "setup";
    bool acquire_ok = true, release_ok = true, client_owned = false, overflow = false, violation = false;
    // This callback is noexcept: fixed capacity, no allocation/assertion/logging.
    // client_owned models this fixture's client lifetime, never an OS request.
    bool record_request(bool keep) noexcept
    {
        const DWORD thread = ::GetCurrentThreadId();
        if (thread != ui_thread) { unexpected_thread.store(thread, std::memory_order_relaxed); return false; }
        const bool result = keep ? acquire_ok : release_ok;
        if (event_count == events.size()) { overflow = true; return false; }
        violation = violation || thread != ui_thread || (keep ? client_owned : !client_owned);
        if (keep) { client_owned = result; } else { client_owned = false; }
        events[event_count++] = { phase, keep, result, client_owned, thread }; return result;
    }
}
// The only substituted production symbol. The SAME real inline helper owns
// acquisition/conditional cleanup; the thin Win32 default-ctor .obj is omitted.
DisplayAwake::DisplayAwake() noexcept : DisplayAwake(display_awake_probe::record_request) {}

namespace
{
    using namespace display_awake_probe;
    void require(bool condition, const char* message) { if (!condition) { throw std::runtime_error(message); } }
    void held(size_t count, bool owned)
    { require(event_count == count && client_owned == owned && !overflow && !violation && unexpected_thread.load(std::memory_order_relaxed) == 0, "Recorded display lifetime/thread/order differs"); }
    void finish_case(const char* name, size_t first, size_t last)
    {
        held(last, false); require(case_count < cases.size(), "Unexpected lifecycle case"); cases[case_count++] = { name, first, last, client_owned };
    }
    class EmptyState final : public State
    {
    public: void update(float) override {} void draw(Renderer&) const override {} void init() override {}
    };
    // GameLevel is final. This synthetic StateContext init failure owns a REAL
    // valid but unentered GameLevel; it is not a GameLevel::init failure claim.
    class FailingOwner final : public State
    {
    public:
        FailingOwner(GameData& data, const MenuLevelSettings& settings) : game_(std::make_unique<GameLevel>(&data, settings)) {}
        void update(float) override {} void draw(Renderer&) const override {}
        void init() override { throw std::runtime_error("fixture owner init failure"); }
    private: std::unique_ptr<GameLevel> game_;
    };
    class QueueFailure final : public State
    {
    public:
        QueueFailure(GameData& data, const MenuLevelSettings& settings) : data_(data), settings_(settings) {}
        void draw(Renderer&) const override {} void init() override {}
        void update(float) override
        { context()->push(std::make_unique<GameLevel>(&data_, settings_)); throw std::runtime_error("fixture queued update failure"); }
    private: GameData& data_; MenuLevelSettings settings_;
    };
    void write_evidence(const std::filesystem::path& output, bool passed)
    {
        std::filesystem::create_directories(output);
        std::ofstream trace(output / "display-awake-events.csv"), case_file(output / "display-awake-cases.csv");
        trace << "ordinal,case,keep,returned,thread_id,client_owned_after\n" << std::boolalpha;
        size_t acquires = 0, acquired = 0, releases = 0, released = 0;
        for (size_t i = 0; i < event_count; ++i)
        {
            const auto& e = events[i]; trace << i + 1 << ',' << e.name << ',' << e.keep << ',' << e.returned << ',' << e.thread << ',' << e.client_after << '\n';
            if (e.keep) { ++acquires; acquired += e.returned ? 1U : 0U; } else { ++releases; released += e.returned ? 1U : 0U; }
        }
        case_file << "case,start_event,end_event,logical_clients_after,ui_thread_only\n";
        for (size_t i = 0; i < case_count; ++i)
        { case_file << cases[i].name << ',' << cases[i].first << ',' << cases[i].last << ',' << (cases[i].client_after ? 1 : 0) << ",true\n"; }
        std::ofstream summary(output / "display-awake-summary.json");
        summary << "{\"scope\":\"actual-GameLevel-display-awake-lifecycle\",\"outcome\":\"" << (passed ? "passed" : "failed") << "\",\"cases\":" << case_count <<
            ",\"actual_GameLevel_constructions\":" << acquires << ",\"actual_GameLevel_initializations\":" << game_initializations << ",\"actual_restarts\":" << real_restarts <<
            ",\"events\":" << event_count << ",\"acquire_attempts\":" << acquires << ",\"successful_acquires\":" << acquired << ",\"release_attempts\":" << releases <<
            ",\"successful_recorder_releases\":" << released << ",\"logical_clients_after\":" << (client_owned ? 1 : 0) << ",\"ui_thread\":" << ui_thread <<
            ",\"unexpected_thread\":" << unexpected_thread.load(std::memory_order_relaxed) << ",\"callback_capacity\":32,\"overflow\":" << (overflow ? "true" : "false") << ",\"thread_or_ownership_violation\":" << (violation || unexpected_thread.load(std::memory_order_relaxed) != 0 ? "true" : "false") <<
            ",\"probe_key_taps\":" << key_taps << ",\"probe_keyboard_polls\":" << keyboard_polls << ",\"probe_mouse_polls\":" << mouse_polls <<
            ",\"probe_state_updates\":" << state_updates << ",\"positive_state_updates\":" << positive_updates << ",\"zero_state_updates\":" << state_updates - positive_updates <<
            ",\"GPU_reads\":0,\"GPU_draws\":0,\"crops\":0,\"real_display_requests\":0,\"OS_setting_changes\":0,\"game_saves\":0,\"request_semantics\":\"recorded bool keep and scripted result, not observed Win32 flags or OS request state\",\"release_failure_scope\":\"cleanup attempt and nonthrowing teardown only; physical OS clear unverified\"}\n";
        require(trace.good() && case_file.good() && summary.good(), "Cannot retain display lifecycle evidence");
    }
    void lifecycle(const std::filesystem::path& content)
    {
        // Every GameData borrow is declared BEFORE Application and outlives its
        // destructor, including the final live GameLevel during app shutdown.
        Registry<LevelLoadedInfo> levels("Level"); StageList stages;
        GameTuning tuning((content / "tuning.json").string().c_str()); Presentation presentation((content / "presentation.json").string().c_str());
        Save preferences; auto& play = const_cast<PlaySettings&>(preferences.play_settings()); play.music_volume = play.sfx_volume = 0; play.aim_marker = false;
        GameData data; data.set_save(&preferences); data.set_level_infos(&levels); data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
        MenuLevelSettings settings; settings.set_player_count(1); settings.set_stage("movement_course"); settings.set_duration(60.0f);
        settings.begin_input_claims(input_slots::keyboard_and_mouse); settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_player_team(0, PlayerTeam::a); settings.set_player_weapon(0, WeaponType::sprayer);
        ApplicationOptions options; options.window_class_name = L"ColourWarsDisplayAwakeProbe"; options.window_title = L"ColourWars display lifetime validation";
        options.resolution = ScreenResolution::s_1280_720; options.fullscreen = false; options.visible = false; options.max_threads = 4;
        {
            Application app(options); data.set_application(&app); app.initialize(); StateContext& stack = app;
            app.resource_loader()->register_kind("level", { [&](const std::string& directory, const std::string& name, bool)
                { auto info = std::make_unique<LevelLoadedInfo>((directory + name + ".json").c_str()); info->validate();
                    stages.push_back(Stage{ name }); levels.add(name, std::move(info)); }, nullptr });
            app.load_manifest((content / "manifest.json").string().c_str());
            sound_groups::assign(*app.audio_resources(), tuning, presentation, levels, stages); sound_groups::apply(*app.audio_resources(), play);
            tuning.validate_resources(*app.render_resources(), *app.audio_resources()); presentation.validate_resources(*app.render_resources(), *app.audio_resources());
            require(levels.get("movement_course") != nullptr, "Required valid fixture stage missing");
            app.viewport_manager()->set_layout(ScreenLayout::one_player); stack.notify_activation(true);
            app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
            app.keyboard()->poll(); app.keyboard()->poll(); keyboard_polls += 2;
            app.mouse()->poll(); app.mouse()->poll(); mouse_polls += 2;
            const auto tick = [&](float elapsed = 0.0f)
            { app.keyboard()->poll(); ++keyboard_polls; app.mouse()->poll(); ++mouse_polls; ++state_updates; positive_updates += elapsed > 0 ? 1U : 0U; stack.update(elapsed); };
            const auto tap = [&](Key value)
            { ++key_taps; app.keyboard()->on_key_down(value); tick(); app.keyboard()->on_key_up(value); tick(); };
            const auto push_game = [&]()
            { stack.push(std::make_unique<EmptyState>()); auto game = std::make_unique<GameLevel>(&data, settings); GameLevel* borrowed = game.get();
                stack.push<EndMenuAction>(std::move(game), [](const EndMenuAction&) {}); ++game_initializations; require(borrowed->level().state() == LevelState::start_countdown, "Actual solo GameLevel did not initialize its authored countdown"); return borrowed; };
            phase = "standalone-menu"; stack.push(std::make_unique<GameMenu>(&data)); require(stack.depth() == 2, "Actual GameMenu/title did not initialize"); held(0, false); stack.clear(); finish_case(phase, 0, 0);
            phase = "normal-pause-focus-restart-quit"; stack.push(std::make_unique<EmptyState>());
            auto owned = std::make_unique<GameLevel>(&data, settings); GameLevel* game = owned.get(); held(1, true); bool ended = false;
            stack.push<EndMenuAction>(std::move(owned), [&](const EndMenuAction& action) { require(action == EndMenuAction::exit, "Actual Quit returned wrong action"); ended = true; }); ++game_initializations;
            const auto enter_live = [&]()
            {
                // The first large portion stays STRICTLY in countdown, so
                // only the final exact binary step can enter world logic.
                constexpr float step = 1.0f / 64.0f; const float countdown = tuning.match().countdown;
                require(countdown > step && countdown <= 10.0f && std::floor(countdown * 64.0f) == countdown * 64.0f &&
                    game->level().state() == LevelState::start_countdown, "Source countdown requires the declared exact split");
                tick(countdown - step); require(game->level().state() == LevelState::start_countdown, "Countdown crossed before the small step");
                tick(step); require(game->level().state() == LevelState::active, "Small countdown step did not enter live play"); held(1, true);
            };
            enter_live();
            const float resume_duration = presentation.pause_menu().resume_countdown; require(resume_duration > 0, "Source count-in must be positive");
            const auto pause = [&]() { tap(Key::escape); require(stack.depth() == 3, "Actual Escape did not open Pause"); tick(); held(1, true); };
            pause(); stack.notify_activation(false); stack.notify_activation(true); held(1, true);
            tap(Key::enter); require(stack.depth() == 2 && game->resume_remaining() == resume_duration, "Actual Resume did not begin count-in"); tick(resume_duration + 1.0f); held(1, true);
            stack.notify_activation(false); stack.notify_activation(true); tick(resume_duration + 1.0f); held(1, true);
            stack.push(std::make_unique<EmptyState>()); stack.notify_activation(false); stack.notify_activation(true); held(1, true); stack.pop(); tick(resume_duration + 1.0f); held(1, true);
            require(const_cast<Level&>(game->level()).kill_player(0), "Cannot mark a valid fixture player for restart proof"); tick(1.0f / 60.0f);
            require(game->level().player(0).health() <= 0, "Restart proof did not retain a dead/empty-health player");
            pause(); tap(Key::down); tap(Key::enter); tick(); tap(Key::up); tap(Key::enter);
            require(stack.depth() == 2 && game->level().state() == LevelState::start_countdown && game->level().player(0).health() == tuning.player().starting_health,
                "Actual Restart did not rebuild/reset the same GameLevel"); ++real_restarts; held(1, true); enter_live();
            pause(); tap(Key::down); tap(Key::down); tap(Key::down); tap(Key::enter); tick(); tap(Key::up); tap(Key::enter);
            require(ended && stack.depth() == 1, "Actual Quit did not pop GameLevel"); finish_case(phase, 0, 2); stack.clear();
            phase = "sequential-clear"; (void)push_game(); held(3, true); stack.clear(); finish_case(phase, 2, 4);
            phase = "acquire-failure"; acquire_ok = false; (void)push_game(); held(5, false); stack.clear(); finish_case(phase, 4, 5); acquire_ok = true;
            phase = "release-failure"; release_ok = false; (void)push_game(); held(6, true); stack.clear(); finish_case(phase, 5, 7); release_ok = true;
            phase = "owner-unwind"; bool caught = false;
            try { auto unentered = std::make_unique<GameLevel>(&data, settings); held(8, true); throw std::runtime_error("fixture owner unwind"); }
            catch (const std::runtime_error& error) { require(std::string_view(error.what()) == "fixture owner unwind", "Unexpected owner-unwind failure"); caught = true; }
            require(caught, "Owner unwind did not throw"); finish_case(phase, 7, 9);
            phase = "owner-init-failure"; stack.push(std::make_unique<EmptyState>()); caught = false;
            try { stack.push(std::make_unique<FailingOwner>(data, settings)); }
            catch (const std::runtime_error& error) { require(std::string_view(error.what()) == "fixture owner init failure", "Unexpected synthetic init failure"); caught = true; }
            require(caught && stack.depth() == 2, "StateContext did not retain synthetic failed-init owner"); held(10, true); stack.clear(); finish_case(phase, 9, 11);
            phase = "queued-cancel"; stack.push(std::make_unique<QueueFailure>(data, settings)); caught = false;
            try { ++state_updates; stack.update(0); }
            catch (const std::runtime_error& error) { require(std::string_view(error.what()) == "fixture queued update failure", "Unexpected synthetic queued failure"); caught = true; }
            require(caught && stack.depth() == 1, "Queued actual GameLevel unexpectedly entered the stack"); held(12, true); stack.clear(); finish_case(phase, 11, 13);
            phase = "application-exit"; (void)push_game(); held(14, true);
            // Deliberately no clear. ~Application must clear while its services
            // and every borrowed GameData source above still exist.
        }
        finish_case(phase, 13, 15);
        require(case_count == 9 && key_taps == 14 && state_updates == 42 && positive_updates == 8 && game_initializations == 5 && real_restarts == 1 && keyboard_polls == 43 && mouse_polls == 43,
            "Lifecycle fixture's finite cases/input/update counters changed");
    }
}
int wmain(int argc, wchar_t* argv[])
{
    if (argc != 3) { std::cerr << "usage: display_awake_lifecycle <staged-content> <output>\n"; return 2; }
    const std::filesystem::path output = std::filesystem::absolute(argv[2]); display_awake_probe::ui_thread = ::GetCurrentThreadId();
    try { lifecycle(std::filesystem::absolute(argv[1])); write_evidence(output, true); std::cout << "PASS actual display-awake lifecycle: 9 cases, 8 GameLevel constructions and 15 recorded bool requests; zero real display requests/GPU reads/crops.\n"; return 0; }
    catch (const std::exception& error)
    { std::cerr << error.what() << '\n'; try { write_evidence(output, false); } catch (const std::exception& nested) { std::cerr << nested.what() << '\n'; } return 1; }
}
