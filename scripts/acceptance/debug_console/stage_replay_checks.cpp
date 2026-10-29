#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"
#include "game/objects/level_loaded_info.h"
#include "engine/core/file_path.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace debug_console_acceptance
{
    namespace
    {
        void write_semantic_bytes(const std::filesystem::path& path, const std::string& bytes)
        {
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            file.close();
            require(!file.fail(), "Cannot write private semantic-stage fixture");
        }
        void check_semantic_stage_hash(const std::filesystem::path& root)
        {
            const auto path = root / L"semantic-stage.json";
            const auto hash = [&](const std::string& bytes)
            {
                write_semantic_bytes(path, bytes);
                return LevelLoadedInfo(labrador::path_to_utf8(path).c_str()).semantic_hash();
            };
            const std::string compact =
                R"({"format":1,"numbers":[1,1.5,18446744073709551615,-9223372036854775808,-0.0],"text":"A\u0000B","nested":{"first":true,"second":null}})";
            const std::string equivalent =
                "{\r\n\t\"format\" : 1,\r\n\t\"num\\u0062ers\" : [ 1e0, 1.50, 18446744073709551615, -9223372036854775808, -0e0 ],\r\n"
                "\t\"text\" : \"\\u0041\\u0000B\", \"nested\" : { \"first\" : true, \"second\" : null }\r\n}";
            require(hash(compact) == hash(equivalent),
                "Loaded semantic stage hash changed with whitespace, decoded escapes or equal numeric spellings");
            require(hash(R"({"format":1,"unknown":9007199254740992})") !=
                hash(R"({"format":1,"unknown":9007199254740993})") &&
                hash(R"({"format":1,"unknown":18446744073709551614})") !=
                hash(R"({"format":1,"unknown":18446744073709551615})"),
                "Loaded semantic stage hash rounded distinct exact integers through double");
            require(hash(R"({"format":1,"unknown":1})") == hash(R"({"format":1,"unknown":1.0})") &&
                hash(R"({"format":1,"unknown":1})") == hash(R"({"format":1,"unknown":1e0})") &&
                hash(R"({"format":1,"unknown":-9223372036854775808})") ==
                    hash(R"({"format":1,"unknown":-9223372036854775808.0})"),
                "Loaded semantic stage hash treated equal integer and floating spellings as different values");
            require(hash(R"({"format":1,"unknown":18446744073709551616.0})") !=
                hash(R"({"format":1,"unknown":0})"),
                "An out-of-integer-range floating value wrapped through an integer hash representation");
            require(hash(R"({"format":1,"unknown":0.0})") != hash(R"({"format":1,"unknown":-0.0})") &&
                hash(R"({"format":1,"unknown":0})") == hash(R"({"format":1,"unknown":-0})"),
                "Stage hash lost floating signed zero or invented a sign for parsed integer zero");
            require(hash(R"({"format":1,"first":1,"second":2})") !=
                hash(R"({"format":1,"second":2,"first":1})") &&
                hash(R"({"format":1,"objects":[1,2]})") != hash(R"({"format":1,"objects":[2,1]})") &&
                hash(R"({"format":1,"unknown":1})") != hash(R"({"format":1,"unknown":"1"})") &&
                hash(R"({"format":1,"unknown":[]})") != hash(R"({"format":1,"unknown":{}})") &&
                hash(R"({"format":1,"unknown":"A\u0000B"})") != hash(R"({"format":1,"unknown":"A"})"),
                "Stage hash discarded authored field/object order, unknown fields, node kinds or embedded string bytes");
            write_semantic_bytes(path, compact);
            const LevelLoadedInfo loaded(labrador::path_to_utf8(path).c_str());
            const std::uint64_t cached = loaded.semantic_hash();
            write_semantic_bytes(path, R"({"format":1,"different":true})");
            require(loaded.semantic_hash() == cached && hash(R"({"format":1,"different":true})") != cached,
                "A cached stage fingerprint reread newer disk contents");
        }

    }

    void check_loaded_stage_semantics()
    {
        const auto root = std::filesystem::temp_directory_path() /
            (L"colourwars_stage_semantics_" + std::to_wstring(GetCurrentProcessId()) + L"_" +
                std::to_wstring(GetTickCount64()));
        require(std::filesystem::create_directory(root), "Cannot create private semantic-stage directory");
        struct Cleanup
        {
            std::filesystem::path root;
            ~Cleanup()
            {
                std::error_code ignored;
                std::filesystem::remove(root / L"semantic-stage.json", ignored);
                std::filesystem::remove(root, ignored);
            }
        } cleanup{ root };
        check_semantic_stage_hash(root);
        std::cout << "PASS loaded-stage semantics: formatting/escapes/equal numbers, exact integers, source order, "
            "unknown fields, node kinds, embedded string bytes, floating signed zero and cached snapshot\n";
    }
}
#include "scripts/acceptance/debug_console/cpu_level.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/replay.h"
#include "game/directory_consts.h"
#include "game/stage_loading.h"
#include "engine/render/text_encoding.h"
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <bit>
#include <sstream>
#include <tuple>

namespace debug_console_acceptance
{
    namespace
    {
        constexpr float stage_replay_step = 1.0f / 60.0f;
        constexpr int stage_replay_length = 240;

        std::string json_text(const rapidjson::Document& document, bool pretty)
        {
            rapidjson::StringBuffer bytes;
            if (pretty)
            {
                rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(bytes);
                writer.SetIndent('\t', 1);
                document.Accept(writer);
            }
            else
            {
                rapidjson::Writer<rapidjson::StringBuffer> writer(bytes);
                document.Accept(writer);
            }
            return { bytes.GetString(), bytes.GetSize() };
        }

        std::string request_state(const MatchRequest& request)
        {
            std::ostringstream out;
            out << request.stage.has_value() << ':' << request.stage.value_or("") << '|'
                << request.players.has_value() << ':' << request.players.value_or(0) << '|'
                << request.restart << '|' << request.exact_label << '|' << request.weapons.size();
            for (const auto& weapon : request.weapons) { out << '|' << weapon.seat << ':' << weapon.weapon; }
            out << '|' << request.exact.has_value();
            if (request.exact)
            {
                const auto& settings = *request.exact;
                out << '|' << settings.stage() << '|' << settings.best_of() << '|'
                    << static_cast<int>(settings.series_stage_rule()) << '|'
                    << static_cast<int>(settings.screen_layout()) << '|' << settings.duration().has_value() << ':'
                    << (settings.duration() ? std::bit_cast<std::uint32_t>(*settings.duration()) : 0u);
                for (const auto& seat : settings.player_settings())
                {
                    out << '|' << seat.num << ':' << static_cast<int>(seat.team) << ':' << static_cast<int>(seat.weapon)
                        << ':' << static_cast<int>(seat.controller) << ':' << static_cast<int>(seat.difficulty)
                        << ':' << seat.handicap;
                }
                for (const int device : settings.pad_devices()) { out << '|' << device; }
            }
            return out.str();
        }

        std::size_t warning_count(const DebugLog& log)
        {
            return static_cast<std::size_t>(std::count_if(log.lines().begin(), log.lines().end(),
                [](const DebugLogLine& line) { return line.severity == DebugSeverity::warning; }));
        }

        void stage_replay_tick(DebugDirector& director, CpuMatch& match)
        {
            if (!director.before_match_update(match, stage_replay_step))
            {
                director.update_match(match, keyboard_step(director.impl()->level_updates()), stage_replay_step);
            }
        }

        void finish_stage_replay(DebugDirector& director, CpuMatch& match,
            const ReplayFile& file, const std::string& name)
        {
            auto& impl = *director.impl();
            for (int step = 0; step < file.updates + 10 && !impl.replay_finished(); ++step)
                stage_replay_tick(director, match);
            const auto print = impl.fingerprint();
            require(impl.replay_finished() && print && print->hash == file.final_print.hash &&
                MatchFingerprint::difference(file.final_print, *print).empty() &&
                log_contains(impl.log(), "replay " + name + ": MATCH (" + std::to_string(file.updates) +
                    " updates, " + std::to_string(file.checkpoints.size()) + " checkpoints", DebugSeverity::info),
                "Loaded-stage replay lost a checkpoint or final-state match: " + name);
        }
    }

    void check_loaded_stage_replay(const std::filesystem::path& game_directory)
    {
        ScratchContent scratch(game_directory, L"loaded_stage_replay");
        GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
        Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
        CpuWorld world(game_directory, tuning, presentation);
        const auto user_folder = scratch.root / L"user-stages";
        std::filesystem::create_directories(user_folder);
        const auto stage_path = user_folder / L"user_replay.json";
        rapidjson::Document authored;
        const std::string base = read_text(game_directory / L"levels/close_quarters.json");
        authored.Parse(base.data(), base.size());
        require(!authored.HasParseError(), "Cannot read private stage replay fixture");
        authored["display_name"].SetString("Replay Yard", authored.GetAllocator());
        const std::string original_bytes = json_text(authored, false);
        write_text(stage_path, original_bytes);
        const UserStageChecks checks{ &tuning, &presentation, &world.render, &world.audio, &world.preferences };
        UserStageFolder folder;
        folder.attach(user_folder, checks, &world.audio, &world.levels, &world.stages);
        const auto scanned = folder.scan();
        require(scanned.accepted == std::vector<std::string>{ "user_replay" } && scanned.refused.empty(),
            "Actual user-stage loader refused the private replay arena");
        const std::uint64_t original_hash = world.levels.get("user_replay")->semantic_hash();
        const LevelLoadedInfo* const shipped_definition = world.levels.get("movement_course");
        GameData data;
        data.set_stages(&world.stages);
        data.set_level_infos(&world.levels);
        DebugDirector director(&tuning, &presentation);
        director.attach(nullptr, &data);
        auto& impl = *director.impl();
        impl.log().set_mirror_to_debugger(false);
        impl.set_content_directories(scratch.source, scratch.staged);
        CpuMatch match(world, director);
        const auto settings_for = [&](const std::string& stage)
        {
            MatchRequest request; request.stage = stage; request.players = 2;
            MatchPlan plan = MatchPlanner(&world.stages).resolve(nullptr, request, {}, [] { return 0u; });
            require(plan.ok, "Cannot plan loaded-stage replay fixture: " + plan.error);
            plan.settings.set_player_bot(1, BotDifficulty::easy);
            plan.settings.set_screen_layout(MatchPlanner::layout_for(plan.settings.human_count()));
            plan.settings.set_duration(20.0f);
            return plan.settings;
        };
        match.settings = settings_for("movement_course");
        director.attach_match(match);
        match.build();
        const std::string suffix = std::to_string(_getpid());
        const std::string shipped_name = "stage_shipped_" + suffix;
        const std::string user_name = "stage_user_" + suffix;
        const auto replay_path = [](const std::string& name)
        {
            return std::filesystem::path(labrador::widen(directory_consts::replay_directory())) /
                labrador::path_from_utf8(name + ".json");
        };
        const auto record = [&](const std::string& stage, const std::string& name)
        {
            if (impl.playing()) { require(impl.execute("play stop").ok, "Cannot stop fixture replay"); }
            require(impl.execute("pause off").ok && impl.execute("seed 211").ok,
                "Cannot prepare loaded-stage recording");
            match.settings = settings_for(stage);
            match.build();
            const auto started = impl.execute("record " + name + " " + std::to_string(stage_replay_length));
            require(started.ok && director.before_match_update(match, stage_replay_step) && impl.recording(),
                "Cannot record fresh loaded-stage fixture: " + started.message);
            for (int step = 0; step < stage_replay_length + 10 && impl.recording(); ++step)
                stage_replay_tick(director, match);
            const auto file = ReplayFile::read(replay_path(name));
            require(!impl.recording() && file.stage_hash &&
                *file.stage_hash == world.levels.get(stage)->semantic_hash() &&
                file.updates == stage_replay_length &&
                file.checkpoints.size() == static_cast<std::size_t>(stage_replay_length / ReplayFile::checkpoint_interval),
                "Recording did not capture its actual loaded stage and complete checkpoint sequence");
            return file;
        };
        const auto play = [&](const std::string& name, const ReplayFile& file)
        {
            require(impl.execute("pause off").ok, "Cannot resume loaded-stage replay clock");
            const auto started = impl.execute("play " + name);
            require(started.ok && director.before_match_update(match, stage_replay_step) && impl.playing(),
                "Cannot start matching loaded-stage replay: " + started.message);
            finish_stage_replay(director, match, file, name);
        };
        const ReplayFile shipped = record("movement_course", shipped_name);
        play(shipped_name, shipped);
        const ReplayFile user = record("user_replay", user_name);
        play(user_name, user);
        require(impl.execute("play stop").ok, "Cannot release user replay before private disk edit");
        // A disk edit is allowed during play. It is not a rescan: the registry
        // and the definition borrowed by this match still describe the old file.
        authored["collision_objects"][0]["rectangle"]["width"].SetDouble(101.0);
        const std::string changed_bytes = json_text(authored, false);
        write_text(stage_path, changed_bytes);
        require(world.levels.get("user_replay")->semantic_hash() == original_hash,
            "Disk-only edit replaced the loaded user-stage definition");
        play(user_name, user);
        require(impl.execute("play stop").ok, "Cannot release user replay before rescan");
        director.detach_match(match);
        match.level.reset();
        // The real menu rescan boundary: no Level holds any stage definition.
        write_text(stage_path, original_bytes);
        {
            rapidjson::Document same;
            same.Parse(original_bytes.data(), original_bytes.size());
            write_text(stage_path, json_text(same, true));
        }
        const auto formatted = folder.rescan();
        require(formatted.accepted == std::vector<std::string>{ "user_replay" } && formatted.refused.empty() &&
            world.levels.get("user_replay")->semantic_hash() == original_hash &&
            world.levels.get("movement_course") == shipped_definition,
            "Formatting-only actual rescan changed the semantic stage or reread a shipped definition");
        director.attach_match(match);
        match.settings = settings_for("user_replay");
        match.build();
        play(user_name, user);
        require(impl.execute("play stop").ok, "Cannot release formatted replay before geometry rescan");
        director.detach_match(match);
        match.level.reset();
        write_text(stage_path, changed_bytes);
        const auto changed = folder.rescan();
        require(changed.accepted == std::vector<std::string>{ "user_replay" } && changed.refused.empty() &&
            world.levels.get("user_replay")->semantic_hash() != original_hash,
            "Actual rescan did not publish the changed user-stage geometry");
        director.attach_match(match);
        match.settings = settings_for("movement_course");
        match.build();
        const auto state = [&]
        {
            const auto print = impl.fingerprint();
            require(print.has_value(), "Stage guard witness requires a live match");
            return std::tuple{ match.level.get(), match.builds, print->hash, impl.level_updates(),
                impl.recording(), impl.playing(), impl.replay_finished(), impl.drives_slot(input_slots::keyboard_and_mouse), impl.status_strip(),
                request_state(impl.request()), impl.clock().paused(), impl.clock().timescale(),
                impl.clock().pending_steps(), impl.seed(), match.god, match.seat_labels };
        };
        const auto refuse = [&](const std::string& command, const std::string& diagnostic)
        {
            const auto before = state();
            const auto warnings = warning_count(impl.log());
            const auto status = impl.execute(command);
            require(!status.ok && status.message.find(diagnostic) != std::string::npos &&
                state() == before && warning_count(impl.log()) == warnings,
                "Stage refusal changed match/playback/request state, warned before guarding, or lost context: " + status.message);
            return status.message;
        };
        require(impl.execute("players 3").ok && impl.request().players == 3,
            "Cannot seed an unrelated pending match request");
        const std::string mismatch = refuse("play " + user_name, "stage 'user_replay' differs");
        require(mismatch.find("restore the recorded stage definition") != std::string::npos &&
            mismatch.find("record a new run") != std::string::npos,
            "Changed-stage refusal did not tell the author how to repair the replay");
        ReplayFile changed_header = user;
        changed_header.tuning_hash ^= 1u;
        changed_header.presentation_hash ^= 1u;
        changed_header.build = "a different build";
        const std::string forced_name = "stage_changed_force_" + suffix;
        changed_header.write(replay_path(forced_name));
        refuse("play " + forced_name + " force", "stage 'user_replay' differs");
        impl.drop_request("stage replay acceptance completed its unrelated-request witness");
        require(impl.execute("play " + shipped_name).ok && impl.request().exact_label == shipped_name,
            "Cannot arm the valid pending replay witness");
        refuse("play " + forced_name + " force", "stage 'user_replay' differs");
        require(director.before_match_update(match, stage_replay_step) && impl.playing(),
            "Refused replacement discarded the previously armed replay");
        for (int step = 0; step < 12; ++step) { stage_replay_tick(director, match); }
        refuse("play " + forced_name + " force", "stage 'user_replay' differs");
        ReplayFile legacy = user;
        legacy.stage_hash.reset();
        const std::string legacy_name = "stage_legacy_" + suffix;
        legacy.write(replay_path(legacy_name));
        require(!ReplayFile::read(replay_path(legacy_name)).stage_hash,
            "Legacy codec fixture unexpectedly acquired a stage hash");
        const auto legacy_message = refuse("play " + legacy_name + " force", "legacy recording");
        require(legacy_message.find("user_replay") != std::string::npos &&
            legacy_message.find("record a new run") != std::string::npos,
            "Legacy refusal lost the affected stage or actionable new-recording instruction");
        ReplayFile zero = user; zero.stage_hash = 0;
        const std::string zero_name = "stage_zero_hash_" + suffix;
        zero.write(replay_path(zero_name));
        require(ReplayFile::read(replay_path(zero_name)).stage_hash == std::optional<std::uint64_t>(0),
            "A present zero hash was conflated with a legacy missing fingerprint");
        for (const bool wrong_type : { false, true })
        {
            rapidjson::Document malformed;
            const auto valid = read_text(replay_path(user_name));
            malformed.Parse(valid.data(), valid.size());
            require(!malformed.HasParseError() && malformed.HasMember("stage_hash"),
                "Cannot find the recorded stage hash for malformed-codec witness");
            if (wrong_type) { malformed["stage_hash"].SetInt(7); }
            else { malformed["stage_hash"].SetString("not-hex", malformed.GetAllocator()); }
            const auto malformed_path = scratch.root /
                (wrong_type ? L"stage_hash_wrong_type.json" : L"stage_hash_nonhex.json");
            write_text(malformed_path, json_text(malformed, false));
            std::string diagnostic;
            try { static_cast<void>(ReplayFile::read(malformed_path)); }
            catch (const std::exception& error) { diagnostic = error.what(); }
            require(diagnostic.find("stage_hash") != std::string::npos &&
                diagnostic.find(malformed_path.filename().string()) != std::string::npos,
                "Malformed stage-hash codec diagnostic lost file/field context: " + diagnostic);
        }
        finish_stage_replay(director, match, shipped, shipped_name);
        require(impl.execute("play stop").ok, "Cannot release stage-guard active playback");
        // Missing services must fail explicitly; the running Level remains
        // alive in its real world rather than being replaced by a zero hash.
        data.set_level_infos(nullptr);
        refuse("play " + shipped_name, "has no loaded definition");
        data.set_level_infos(&world.levels);
        director.detach_match(match);
        match.level.reset();
        std::filesystem::remove(stage_path);
        const auto removed = folder.rescan();
        require(removed.accepted.empty() && removed.refused.empty() && world.levels.contains("user_replay") &&
            std::none_of(world.stages.begin(), world.stages.end(),
                [](const Stage& stage) { return stage.name == "user_replay"; }),
            "Removed-stage witness did not retain the registry's documented stale entry");
        director.attach_match(match);
        match.settings = settings_for("movement_course");
        match.build();
        refuse("play " + user_name + " force", "stage 'user_replay' is not loaded");
        director.detach_match(match);
        match.level.reset();
        std::cout << "PASS loaded-stage replay: shipped/user checkpoints and final state; one loaded snapshot, "
            "format/escape/numeric equivalence, exact integers/source order/signed zero; disk-only edit and real "
            "format/geometry/removal rescans; force/legacy/catalog/service guards preserve current match, queued "
            "request, pending and active playback before warnings\n";
    }
}
#endif
