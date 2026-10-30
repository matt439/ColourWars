#include "game/pch.h"
#include "game/states/display_settings_trial.h"
#include "game/settings_session.h"
#include "scripts/acceptance/support/probe.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using labrador::ScreenResolution;

namespace
{
    using acceptance::require;

    // All Save calls happen after replacing LOCALAPPDATA in this process.
    // Cleanup removes only these individually created files/directories.
    class IsolatedSaveDirectory
    {
    public:
        IsolatedSaveDirectory()
        {
            wchar_t* previous = nullptr;
            size_t length = 0;
            require(_wdupenv_s(&previous, &length, L"LOCALAPPDATA") == 0,
                "Cannot read the original LOCALAPPDATA");
            if (previous != nullptr) { previous_ = previous; std::free(previous); }
            const std::filesystem::path temporary = std::filesystem::temp_directory_path();
            wchar_t unique[MAX_PATH] = {};
            require(GetTempFileNameW(temporary.c_str(), L"cwd", 0, unique) != 0,
                "Cannot reserve display-trial acceptance folder");
            root_ = unique;
            require(DeleteFileW(unique) != FALSE && std::filesystem::create_directory(root_),
                "Cannot create display-trial acceptance folder");
            require(_wputenv_s(L"LOCALAPPDATA", root_.c_str()) == 0,
                "Cannot isolate display-trial saves");
        }

        ~IsolatedSaveDirectory()
        {
            _wputenv_s(L"LOCALAPPDATA", previous_.c_str());
            std::error_code ignored;
            std::filesystem::remove(temporary_path(), ignored);
            std::filesystem::remove(save_path(), ignored);
            std::filesystem::remove(root_ / L"ColourWars", ignored);
            std::filesystem::remove(root_, ignored);
        }

        std::filesystem::path save_path() const { return root_ / L"ColourWars" / L"save_data.json"; }
        std::filesystem::path temporary_path() const { return save_path().wstring() + L".tmp"; }

    private:
        std::filesystem::path root_;
        std::wstring previous_;
    };

    std::string contents(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        require(stream.good(), "Cannot read isolated display-trial save");
        return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
    }

    bool same_play(const PlaySettings& actual, const PlaySettings& expected)
    {
        if (actual.aim_sensitivity != expected.aim_sensitivity ||
            actual.music_volume != expected.music_volume || actual.sfx_volume != expected.sfx_volume ||
            actual.view_shake != expected.view_shake || actual.aim_marker != expected.aim_marker ||
            actual.reduce_effects != expected.reduce_effects) { return false; }
        for (size_t slot = 0; slot < actual.sticks.size(); ++slot)
        {
            if (actual.sticks[slot].movement_deadzone != expected.sticks[slot].movement_deadzone ||
                actual.sticks[slot].aim_deadzone != expected.sticks[slot].aim_deadzone) { return false; }
        }
        return true;
    }

    bool same_setup(const std::optional<LastMatchSetup>& actual, const std::optional<LastMatchSetup>& expected)
    {
        if (actual.has_value() != expected.has_value()) { return false; }
        if (!actual) { return true; }
        if (actual->players.size() != expected->players.size() || actual->stage != expected->stage ||
            actual->layout != expected->layout || actual->duration != expected->duration ||
            actual->best_of != expected->best_of) { return false; }
        for (size_t seat = 0; seat < actual->players.size(); ++seat)
        {
            if (actual->players[seat].team != expected->players[seat].team ||
                actual->players[seat].weapon != expected->players[seat].weapon) { return false; }
        }
        return true;
    }

    bool same_save(const SaveData& actual, const SaveData& expected)
    {
        return actual.resolution == expected.resolution && actual.fullscreen == expected.fullscreen &&
            actual.team_palette == expected.team_palette && same_play(actual.play, expected.play) &&
            same_setup(actual.last_match, expected.last_match);
    }

    LastMatchSetup setup(bool alternate = false)
    {
        LastMatchSetup result;
        result.players = { {PlayerTeam::b, WeaponType::bucket}, {PlayerTeam::a, WeaponType::mister} };
        result.stage = "movement_course";
        result.layout = labrador::ScreenLayout::two_player;
        result.duration = 98.5f;
        result.best_of = 3;
        if (alternate)
        {
            result.players.push_back({PlayerTeam::b, WeaponType::sniper});
            result.players.push_back({PlayerTeam::a, WeaponType::roller});
            result.stage = "arena";
            result.layout = labrador::ScreenLayout::four_player;
            result.duration.reset();
            result.best_of = 5;
        }
        return result;
    }

    PlaySettings preferences()
    {
        PlaySettings play;
        play.aim_sensitivity = 5;
        play.music_volume = 20;
        play.sfx_volume = 70;
        play.view_shake = false;
        play.aim_marker = false;
        play.reduce_effects = true;
        play.sticks = { StickSettings{ 0, 90 }, StickSettings{ 20, 35 },
            StickSettings{ 45, 65 }, StickSettings{ -1, -1 } };
        return play;
    }

    Save seed_save()
    {
        Save save; SettingsSession save_session(save);
        require(save_session.set_play_settings_and_save(preferences()), "Cannot seed play preferences");
        require(save.set_display_settings_and_save(ScreenResolution::s_2560_1440,
            false, TeamColourPair::blue_yellow), "Cannot seed display preferences");
        require(save.remember_match_setup_and_save(setup()), "Cannot seed remembered match choices");
        return save;
    }

    DisplaySettingsChoice choice(bool fullscreen = false)
    {
        return { ScreenResolution::s_1280_720, fullscreen, TeamColourPair::pink_green };
    }

    void require_choice(const Save& save, const DisplaySettingsChoice& expected)
    {
        require(save.resolution() == expected.resolution && save.fullscreen() == expected.fullscreen &&
            save.team_palette() == expected.palette, "Committed display choice differs from the candidate");
    }

    void require_reloaded(const Save& save)
    {
        Save restored; SettingsSession restored_session(restored);
        restored_session.load_save_file();
        require(same_save(restored.save_data(), save.save_data()),
            "Display trial failed to preserve display/play/four-pad preferences or remembered setup on reload");
    }

    struct DisplayCall
    {
        ScreenResolution resolution;
        bool fullscreen;
    };

    void require_call(const std::vector<DisplayCall>& calls, size_t index,
        ScreenResolution resolution, bool fullscreen)
    {
        require(index < calls.size() && calls[index].resolution == resolution &&
            calls[index].fullscreen == fullscreen, "Trial applied the wrong preset or actual fullscreen mode");
    }

    void require_result(const DisplaySettingsTrial& trial, DisplayTrialResult expected)
    {
        require(trial.result().has_value() && *trial.result() == expected,
            "Display trial returned an incorrect terminal result");
    }

    void require_unchanged(const Save& save, const SaveData& before,
        const IsolatedSaveDirectory& directory, const std::string& saved)
    {
        require(same_save(save.save_data(), before) && contents(directory.save_path()) == saved,
            "Unconfirmed display trial changed active or persisted preferences");
    }

    void terminal_noops(DisplaySettingsTrial& trial)
    {
        const auto result = trial.result();
        const float remaining = trial.remaining();
        for (int repeat = 0; repeat < 4; ++repeat)
        {
            trial.begin(); trial.cancel(); trial.keep(); trial.advance(120.0f);
        }
        require(trial.result() == result && trial.remaining() == remaining,
            "A terminal display trial changed result or countdown");
    }

    void check_configuration(const IsolatedSaveDirectory& directory)
    {
        Save save = seed_save(); SettingsSession save_session(save);
        const SaveData before = save.save_data();
        const std::string saved = contents(directory.save_path());
        int calls = 0;
        const std::function<void(ScreenResolution, bool)> apply = [&](ScreenResolution, bool) { ++calls; };
        const auto rejects = [&](Save* target, DisplaySettingsChoice candidate,
            std::function<void(ScreenResolution, bool)> callback, float timeout)
        {
            bool rejected = false;
            try { DisplaySettingsTrial invalid(target, candidate, false, std::move(callback), timeout); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "Malformed trial configuration was accepted");
        };
        rejects(nullptr, choice(), apply, 15.0f);
        rejects(&save, choice(), {}, 15.0f);
        for (float timeout : { -1.0f, 0.0f, 0.999f, 60.001f,
            std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN() }) { rejects(&save, choice(), apply, timeout); }
        for (int value : { -1, static_cast<int>(ScreenResolution::max) + 1,
            (std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)() })
        {
            auto invalid = choice(); invalid.resolution = static_cast<ScreenResolution>(value);
            rejects(&save, invalid, apply, 15.0f);
        }
        for (int value : { static_cast<int>(TeamColourPair::random) - 1,
            static_cast<int>(TeamColourPair::count), (std::numeric_limits<int>::min)(),
            (std::numeric_limits<int>::max)() })
        {
            auto invalid = choice(); invalid.palette = static_cast<TeamColourPair>(value);
            rejects(&save, invalid, apply, 15.0f);
        }
        for (float timeout : { 1.0f, 60.0f })
        {
            DisplaySettingsTrial low(&save,
                { ScreenResolution::s_1280_720, false, TeamColourPair::random }, false, apply, timeout);
            DisplaySettingsTrial high(&save,
                { ScreenResolution::max, true, TeamColourPair::blue_yellow }, true, apply, timeout);
            require(!low.result() && !high.result(), "Valid inclusive trial limits produced a terminal result");
        }
        require(calls == 0, "Constructing or destroying an unstarted trial applied display settings");
        {
            DisplaySettingsTrial canceled(&save, choice(), false, apply, 15.0f);
            canceled.cancel(); canceled.begin(); canceled.keep(); canceled.advance(60.0f);
            require_result(canceled, DisplayTrialResult::reverted);
            require(calls == 0, "A trial canceled before Begin subsequently applied or saved its candidate");
        }
        require_unchanged(save, before, directory, saved);
    }

    void check_cancel_and_lifetime(const IsolatedSaveDirectory& directory)
    {
        for (bool previous_actual_fullscreen : { false, true })
        {
            for (bool candidate_fullscreen : { false, true })
            {
                Save save = seed_save(); SettingsSession save_session(save);
                const SaveData before = save.save_data();
                const std::string saved = contents(directory.save_path());
                std::vector<DisplayCall> calls;
                auto candidate = choice(candidate_fullscreen);
                const auto captured = candidate;
                {
                    DisplaySettingsTrial trial(&save, candidate, previous_actual_fullscreen,
                        [&](ScreenResolution resolution, bool fullscreen) { calls.push_back({ resolution, fullscreen }); }, 15.0f);
                    candidate = { ScreenResolution::s_3840_2160, !candidate_fullscreen, TeamColourPair::random };
                    trial.begin();
                    require(!trial.result() && calls.size() == 1, "Begin failed to apply exactly one unconfirmed trial");
                    require_call(calls, 0, captured.resolution, captured.fullscreen);
                    require_unchanged(save, before, directory, saved);
                    trial.advance(3.0f);
                    const float remaining = trial.remaining();
                    trial.begin();
                    require(calls.size() == 1 && trial.remaining() == remaining,
                        "Repeated Begin applied again or reset the countdown");
                    trial.cancel();
                    require_result(trial, DisplayTrialResult::reverted);
                    require(calls.size() == 2, "Cancel failed to restore exactly one display target");
                    require_call(calls, 1, before.resolution, previous_actual_fullscreen);
                    terminal_noops(trial);
                    require(calls.size() == 2, "Terminal cancel/keep/begin applied display again");
                }
                require(calls.size() == 2, "Destruction repeated an already completed rollback");
                require_unchanged(save, before, directory, saved);
                {
                    DisplaySettingsTrial active(&save, captured, previous_actual_fullscreen,
                        [&](ScreenResolution resolution, bool fullscreen) { calls.push_back({ resolution, fullscreen }); }, 15.0f);
                    active.begin();
                }
                require(calls.size() == 4, "Destruction of a started trial did not restore its display once");
                require_call(calls, 3, before.resolution, previous_actual_fullscreen);
                require_unchanged(save, before, directory, saved);
            }
        }
    }

    void check_countdown(const IsolatedSaveDirectory& directory)
    {
        for (float duration : { 1.0f, 10.0f, 60.0f })
        {
            Save save = seed_save(); SettingsSession save_session(save);
            const SaveData before = save.save_data();
            const std::string saved = contents(directory.save_path());
            std::vector<DisplayCall> calls;
            DisplaySettingsTrial trial(&save, choice(), false,
                [&](ScreenResolution resolution, bool fullscreen) { calls.push_back({ resolution, fullscreen }); }, duration);
            trial.begin();
            for (float invalid : { 0.0f, -0.5f, std::numeric_limits<float>::infinity(),
                -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
            {
                const float remaining = trial.remaining(); trial.advance(invalid);
                require(trial.remaining() == remaining && !trial.result() && calls.size() == 1,
                    "Invalid or nonpositive time advanced or canceled a trial");
            }
            trial.advance(duration * 0.25f); trial.advance(duration * 0.25f);
            require(std::fabs(trial.remaining() - duration * 0.5f) < 0.0001f && !trial.result(),
                "Positive active time did not advance the display countdown");
            trial.advance(duration * 0.5f);
            require_result(trial, DisplayTrialResult::reverted);
            require(calls.size() == 2 && trial.remaining() >= 0.0f,
                "Exact expiry failed to restore once or left a negative countdown");
            terminal_noops(trial);
            require(calls.size() == 2, "Expired trial accepted another terminal action");
            require_unchanged(save, before, directory, saved);
        }
        Save save = seed_save(); SettingsSession save_session(save);
        const SaveData before = save.save_data();
        const std::string saved = contents(directory.save_path());
        int calls = 0;
        DisplaySettingsTrial hitch(&save, choice(), false, [&](ScreenResolution, bool) { ++calls; }, 15.0f);
        hitch.begin(); hitch.advance((std::numeric_limits<float>::max)());
        require_result(hitch, DisplayTrialResult::reverted);
        require(calls == 2 && hitch.remaining() >= 0.0f, "A finite hitch escaped the bounded timeout");
        require_unchanged(save, before, directory, saved);
    }

    void check_keep_and_current_preferences(const IsolatedSaveDirectory& directory)
    {
        for (bool fullscreen : { false, true })
        {
            Save save = seed_save(); SettingsSession save_session(save);
            const SaveData before = save.save_data();
            const std::string saved = contents(directory.save_path());
            const auto candidate = choice(fullscreen);
            std::vector<DisplayCall> calls;
            {
                DisplaySettingsTrial trial(&save, candidate, !fullscreen,
                    [&](ScreenResolution resolution, bool mode) { calls.push_back({ resolution, mode }); }, 15.0f);
                trial.begin(); trial.advance(2.0f);
                require_unchanged(save, before, directory, saved);
                // Another preference page may commit independently while this
                // trial exists. Keep must not restore a captured whole SaveData.
                PlaySettings changed = save_session.play_settings();
                changed.aim_sensitivity = 2; changed.music_volume = 80; changed.sfx_volume = 10;
                changed.view_shake = true; changed.aim_marker = true; changed.reduce_effects = false;
                changed.sticks = { StickSettings{ -1, 25 }, StickSettings{ 30, 40 },
                    StickSettings{ 55, 75 }, StickSettings{ 90, 0 } };
                require(save_session.set_play_settings_and_save(changed), "Cannot seed an independent preferences change");
                const LastMatchSetup changed_setup = setup(true);
                require(save.remember_match_setup_and_save(changed_setup), "Cannot seed an independent remembered setup");
                const std::string changed_file = contents(directory.save_path());
                trial.advance(1.0f);
                require(!trial.result(),
                    "Independent preference commit completed the display trial");
                SaveData current = before; current.play = changed;
                current.last_match = changed_setup;
                require_unchanged(save, current, directory, changed_file);
                trial.keep();
                require_result(trial, DisplayTrialResult::kept);
                require_choice(save, candidate);
                require(same_play(save_session.play_settings(), changed), "Keep overwrote current shared or four-pad preferences");
                require(same_setup(save.last_match_setup(), current.last_match), "Keep overwrote current remembered choices");
                require(calls.size() == 1, "Keep reapplied or rolled back the accepted display");
                require_reloaded(save);
                const std::string kept_file = contents(directory.save_path());
                terminal_noops(trial);
                require(calls.size() == 1 && contents(directory.save_path()) == kept_file,
                    "Repeated terminal actions changed a kept trial or its file");
            }
            require(calls.size() == 1, "Destruction rolled back a kept display");
            require_reloaded(save);
        }
    }

    void check_revert_preserves_current_setup(const IsolatedSaveDirectory& directory)
    {
        Save save = seed_save(); SettingsSession save_session(save);
        int calls = 0;
        DisplaySettingsTrial trial(&save, choice(true), false,
            [&](ScreenResolution, bool) { ++calls; }, 15.0f);
        trial.begin();
        require(save.remember_match_setup_and_save(setup(true)), "Cannot update remembered setup during display trial");
        const SaveData current = save.save_data();
        const std::string bytes = contents(directory.save_path());
        trial.cancel();
        require_result(trial, DisplayTrialResult::reverted);
        require(calls == 2, "Revert did not restore the display exactly once");
        require_unchanged(save, current, directory, bytes);
        require_reloaded(save);
    }

    class SaveReplacementLock
    {
    public:
        explicit SaveReplacementLock(const std::filesystem::path& path)
        {
            handle_ = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            require(handle_ != INVALID_HANDLE_VALUE, "Cannot lock isolated save replacement");
        }
        ~SaveReplacementLock() { CloseHandle(handle_); }
    private:
        HANDLE handle_ = INVALID_HANDLE_VALUE;
    };

    void check_failed_save(const IsolatedSaveDirectory& directory)
    {
        for (bool after_serialization : { false, true })
        {
            Save save = seed_save(); SettingsSession save_session(save);
            const PlaySettings before_play = save_session.play_settings();
            const std::string old_file = contents(directory.save_path());
            const auto candidate = choice(true);
            int calls = 0;
            {
                DisplaySettingsTrial trial(&save, candidate, false,
                    [&](ScreenResolution, bool) { ++calls; }, 15.0f);
                trial.begin();
                if (after_serialization)
                {
                    const SaveReplacementLock lock(directory.save_path());
                    trial.keep();
                }
                else
                {
                    require(std::filesystem::create_directory(directory.temporary_path()),
                        "Cannot create isolated temporary-file blocker");
                    trial.keep();
                    require(std::filesystem::remove(directory.temporary_path()), "Cannot remove own save blocker");
                }
                require_result(trial, DisplayTrialResult::save_failed);
                require_choice(save, candidate);
                require(same_play(save_session.play_settings(), before_play) && contents(directory.save_path()) == old_file,
                    "A failed Keep damaged the old save or unrelated preferences");
                terminal_noops(trial);
            }
            require(calls == 1, "Failed persistence rolled back the accepted active display");
            require(contents(directory.save_path()) == old_file &&
                !std::filesystem::exists(directory.temporary_path()), "Failed Keep left a partial persisted file");
            Save old; SettingsSession old_session(old);
            old_session.load_save_file();
            require(old.resolution() == ScreenResolution::s_2560_1440 && !old.fullscreen() &&
                old.team_palette() == TeamColourPair::blue_yellow && same_play(old_session.play_settings(), before_play),
                "Failed Keep overwrote persisted display preferences");
            DisplaySettingsChoice recovered{ ScreenResolution::s_3840_2160, false, TeamColourPair::random };
            DisplaySettingsTrial recovery(&save, recovered, true, [&](ScreenResolution, bool) { ++calls; }, 15.0f);
            recovery.begin(); recovery.keep();
            require_result(recovery, DisplayTrialResult::kept);
            require_choice(save, recovered);
            require_reloaded(save);
        }
    }

    void check_reentrant_cancel(const IsolatedSaveDirectory& directory)
    {
        Save save = seed_save(); SettingsSession save_session(save);
        const SaveData before = save.save_data();
        const std::string saved = contents(directory.save_path());
        std::vector<DisplayCall> calls;
        DisplaySettingsTrial* live = nullptr;
        int depth = 0, maximum_depth = 0;
        DisplaySettingsTrial trial(&save, choice(), true,
            [&](ScreenResolution resolution, bool fullscreen)
            {
                ++depth; maximum_depth = std::max(maximum_depth, depth);
                calls.push_back({ resolution, fullscreen });
                if (calls.size() == 1)
                {
                    live->keep();
                    live->cancel(); live->cancel(); live->begin();
                    require(calls.size() == 1 && same_save(save.save_data(), before),
                        "Reentrant cancellation entered rollback before candidate application returned");
                }
                else
                {
                    live->cancel(); live->keep(); live->begin(); live->advance(60.0f);
                }
                --depth;
            }, 15.0f);
        live = &trial;
        trial.begin();
        require_result(trial, DisplayTrialResult::reverted);
        require(calls.size() == 2 && maximum_depth == 1,
            "Reentrant display callbacks nested or repeated rollback");
        require_call(calls, 0, choice().resolution, false);
        require_call(calls, 1, before.resolution, true);
        require_unchanged(save, before, directory, saved);
        terminal_noops(trial);
        require(calls.size() == 2, "Reentrant terminal events applied display again");
    }

    void check_display_exceptions(const IsolatedSaveDirectory& directory)
    {
        for (bool unknown : { false, true })
        {
            for (bool rollback_fails : { false, true })
            {
                Save save = seed_save(); SettingsSession save_session(save);
                const SaveData before = save.save_data();
                const std::string saved = contents(directory.save_path());
                std::vector<DisplayCall> calls;
                DisplaySettingsTrial trial(&save, choice(true), true,
                    [&](ScreenResolution resolution, bool fullscreen)
                    {
                        calls.push_back({ resolution, fullscreen });
                        if (calls.size() == 1 || rollback_fails)
                        {
                            if (unknown) { throw 7; }
                            throw std::runtime_error("controlled display application failure");
                        }
                    }, 15.0f);
                trial.begin();
                require_result(trial, rollback_fails ? DisplayTrialResult::rollback_failed : DisplayTrialResult::apply_failed);
                require(calls.size() == 2, "Failed application did not attempt its rollback exactly once");
                require_call(calls, 1, before.resolution, true);
                terminal_noops(trial);
                require(calls.size() == 2, "A terminal display failure retried its side effects");
                require_unchanged(save, before, directory, saved);
            }

            Save save = seed_save(); SettingsSession save_session(save);
            const SaveData before = save.save_data();
            const std::string saved = contents(directory.save_path());
            int calls = 0;
            {
                DisplaySettingsTrial trial(&save, choice(), false,
                    [&](ScreenResolution, bool)
                    {
                        if (++calls == 2)
                        {
                            if (unknown) { throw 11; }
                            throw std::runtime_error("controlled destructor rollback failure");
                        }
                    }, 15.0f);
                trial.begin();
            }
            require(calls == 2, "Noexcept destructor rollback was absent or retried after failure");
            require_unchanged(save, before, directory, saved);

            calls = 0;
            DisplaySettingsTrial cancel(&save, choice(), false,
                [&](ScreenResolution, bool)
                {
                    if (++calls == 2)
                    {
                        if (unknown) { throw 13; }
                        throw std::runtime_error("controlled explicit rollback failure");
                    }
                }, 15.0f);
            cancel.begin(); cancel.cancel();
            require_result(cancel, DisplayTrialResult::rollback_failed);
            terminal_noops(cancel);
            require(calls == 2, "Failed explicit rollback was retried by terminal actions");
            require_unchanged(save, before, directory, saved);
        }
    }
}

int main(int argc, char** /*argv*/)
{
    static_assert(std::is_nothrow_destructible_v<DisplaySettingsTrial>);
    return acceptance::run_probe("display trial", [&]
    {
        require(argc == 2, "Usage: display_trial.exe <staged content directory>");
        const IsolatedSaveDirectory directory;
        check_configuration(directory);
        check_cancel_and_lifetime(directory);
        check_countdown(directory);
        check_keep_and_current_preferences(directory);
        check_revert_preserves_current_setup(directory);
        check_failed_save(directory);
        check_reentrant_cancel(directory);
        std::srand(7349); const int expected_random = std::rand(); std::srand(7349);
        check_display_exceptions(directory);
        require(std::rand() == expected_random, "Display trial behavior consumed gameplay RNG");
        std::cout << "PASS display trial: validated configuration, configured-preset/actual-mode rollback, "
            "unstarted/active/terminal lifetimes, finite countdown/expiry/hitch, reentrant cancellation, "
            "apply/rollback exceptions, no writes before Keep, current shared/four-pad/remembered-setup preservation, "
            "real isolated save failures and recovery; fake display callback only, no window/device\n";
        return 0;
    });
}
