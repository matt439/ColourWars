#include "game/pch.h"
#include "engine/app/application.h"
#include "game/content_validation.h"
#include "game/debug/debug_director_impl.h"
#include "game/drawable_text.h"
#include "game/game_data.h"
#include "game/states/game_states.h"
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <filesystem>
#include <fstream>
#include <iostream>

#if !COLOURWARS_DEV_TOOLS
#error The frame-pacing diagnostic requires the actual Debug game objects.
#endif

using namespace labrador;

// Match the shipping executable's hybrid-GPU preference; record the adapter
// actually selected rather than assuming the preference was honoured.
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    int integer(const wchar_t* text, int minimum, int maximum)
    {
        const std::wstring value(text);
        std::size_t consumed = 0;
        const int result = std::stoi(value, &consumed);
        require(consumed == value.size() && result >= minimum && result <= maximum,
            "Frame-pacing argument is outside its supported range");
        return result;
    }

    ScreenResolution resolution(const std::wstring& text)
    {
        if (text == L"1280x720") { return ScreenResolution::s_1280_720; }
        if (text == L"1920x1080") { return ScreenResolution::s_1920_1080; }
        if (text == L"2560x1440") { return ScreenResolution::s_2560_1440; }
        throw std::runtime_error("Unsupported frame-pacing resolution");
    }

    void write_device_metadata(Application& app, const std::filesystem::path& path,
        bool visible_requested, bool synthetic_activation, const PlaySettings& play)
    {
        std::ofstream file(path);
        require(file.good(), "Cannot create frame-pacing device metadata");
        rapidjson::OStreamWrapper stream(file);
        rapidjson::Writer<rapidjson::OStreamWrapper> json(stream);
        const RenderDeviceInfo& device = app.renderer()->device_info();
        const auto optional = [&](const char* key, const std::optional<int>& value)
        {
            json.Key(key);
            if (value) { json.Int(*value); }
            else { json.Null(); }
        };
        json.StartObject();
        json.Key("backend"); json.String(device.backend.c_str());
        json.Key("api"); json.String(device.api.c_str());
        json.Key("device_name"); json.String(device.device_name.c_str());
        json.Key("vendor_id"); json.Uint(device.vendor_id);
        json.Key("device_id"); json.Uint(device.device_id);
        json.Key("kind"); json.String("hardware");
        json.Key("present_mode"); json.String(device.present_mode.c_str());
        optional("requested_swap_interval", device.requested_swap_interval);
        optional("reported_swap_interval", device.reported_swap_interval);
        const auto size = app.resolution_manager()->resolution_ivec();
        json.Key("width"); json.Int(size.x);
        json.Key("height"); json.Int(size.y);
        json.Key("fullscreen"); json.Bool(app.fullscreen());
        json.Key("min_threads"); json.Int(app.thread_pool()->min_num_threads());
        json.Key("max_threads"); json.Int(app.thread_pool()->max_num_threads());
        json.Key("visible_requested"); json.Bool(visible_requested);
        json.Key("window_visible_at_start");
        json.Bool(IsWindowVisible(static_cast<HWND>(app.window())) != FALSE);
        json.Key("state_context_active_at_start"); json.Bool(app.active());
        json.Key("keyboard_focused_at_start"); json.Bool(app.keyboard()->focused());
        json.Key("synthetic_activation"); json.Bool(synthetic_activation);
        json.Key("native_focus_actions"); json.Bool(false);
        json.Key("readback"); json.Bool(false);
        json.Key("play_preferences"); json.StartObject();
        json.Key("music_volume"); json.Int(play.music_volume);
        json.Key("sfx_volume"); json.Int(play.sfx_volume);
        json.Key("aim_marker"); json.Bool(play.aim_marker);
        json.Key("reduce_effects"); json.Bool(play.reduce_effects);
        json.Key("view_shake"); json.Bool(play.view_shake);
        json.EndObject();
        json.Key("baseline_accepted"); json.Bool(false);
        json.Key("limitation");
        json.String("Debug instrumentation; hidden/occluded Present may return rapidly. "
            "CPU draw intervals do not establish visible-display 60 Hz, GPU or scan-out acceptance.");
        json.EndObject();
        file.flush();
        require(file.good(), "Frame-pacing device metadata write failed");
    }
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        require(argc == 9,
            "usage: frame_pacing <content> <output> <players> <frames> <warmup-updates> <resolution> <max-threads> <visible:0|1>");
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        const std::filesystem::path output = std::filesystem::absolute(argv[2]);
        const int players = integer(argv[3], 1, 4);
        require(players == 1 || players == 2 || players == 4, "Capture supports 1/2/4 players");
        const int frames = integer(argv[4], 1, 36000);
        const int warmup = integer(argv[5], 1, 36000);
        const int max_threads = integer(argv[7], 0, 64);
        const bool visible = integer(argv[8], 0, 1) != 0;
        std::filesystem::create_directories(output / "debug");

        // These borrowed services outlive Application and every state it owns.
        Save preferences;
        preferences.load_save_file();
        require(preferences.play_settings().music_volume == 0 && preferences.play_settings().sfx_volume == 0,
            "Frame-pacing probe requires its isolated muted save");
        require(preferences.play_settings().aim_marker && !preferences.play_settings().reduce_effects &&
            preferences.play_settings().view_shake, "Frame-pacing probe requires the current visual feedback preferences");
        Registry<LevelLoadedInfo> levels("Level");
        StageList stages;
        GameTuning tuning((content / "tuning.json").string().c_str());
        Presentation presentation((content / "presentation.json").string().c_str());
        DebugDirector debug(&tuning, &presentation);
        GameData data;
        ApplicationOptions options;
        options.window_class_name = L"ColourWarsFramePacingProbe";
        options.window_title = L"ColourWars diagnostic frame capture";
        options.resolution = resolution(argv[6]);
        options.fullscreen = false;
        options.target_fps = 60;
        options.min_threads = 1;
        if (max_threads > 0) { options.max_threads = max_threads; }
        options.visible = visible;
        Application app(options);
        app.initialize();
        require(app.renderer()->device_info().kind == RenderDeviceKind::hardware,
            "Frame-pacing capture requires a hardware render device");
        app.resource_loader()->register_kind("level", {
            [&](const std::string& directory, const std::string& name, bool)
            {
                auto info = std::make_unique<LevelLoadedInfo>((directory + name + ".json").c_str());
                info->validate();
                drawable_text::widen_drawable(info->display_name(), "frame-pacing stage " + name);
                stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() });
                levels.add(name, std::move(info));
            }, nullptr });
        app.load_manifest((content / "manifest.json").string().c_str());
        validate_game_content(tuning, presentation, app.render_resources(), app.audio_resources(),
            levels, stages, preferences.play_settings());
        data.set_application(&app);
        data.set_save(&preferences);
        data.set_level_infos(&levels);
        data.set_stages(&stages);
        data.set_tuning(&tuning);
        data.set_presentation(&presentation);
        data.set_debug(&debug);
        debug.attach(&app, &data);
        debug.impl()->set_user_directory(output / "debug");
        debug.impl()->set_content_directories(content, content);
        debug.impl()->log().set_mirror_to_debugger(false);
        require(debug.impl()->log().set_transcript((output / "console.log").string()),
            "Cannot create isolated frame-pacing transcript");

        // This changes only the hidden fixture's logical state, never Windows
        // foreground/focus. The default shell already starts logically active.
        const bool synthetic_activation = !app.active();
        if (synthetic_activation) { app.notify_activation(true); }
        write_device_metadata(app, output / "device-metadata.json", visible, synthetic_activation,
            preferences.play_settings());
        debug.run_launch_arguments({ L"+players", std::to_wstring(players), L"paint",
            L"+bot", L"1", L"paint", L"+weapon", L"all", L"sprayer",
            L"+seed", L"439", L"+intro", L"off",
            L"+god", L"1", L"+ammo", L"inf", L"+timescale", L"1", L"+overlay", L"none",
            L"+stage", L"close_quarters", L"+wait", L"match", L"+wait", std::to_wstring(warmup),
            L"+perf.capture", std::to_wstring(frames), L"sample", L"+quit" });
        const int code = app.run(std::make_unique<GameMenu>(&data));
        require(code == 0, "Frame-pacing Application loop returned a failure");
        require(std::filesystem::is_regular_file(output / "debug/perf/sample.csv") &&
            std::filesystem::is_regular_file(output / "debug/perf/sample.json"),
            "The Application loop ended without producing its requested perf.capture files");
        std::cout << "CAPTURE 1x Close Quarters " << players << "P; " << frames
            << " requested rendered samples; Debug diagnostic, no baseline acceptance\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "frame-pacing capture failure: " << error.what() << '\n';
        return 1;
    }
}
