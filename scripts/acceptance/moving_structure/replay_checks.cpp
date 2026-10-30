#include "game/pch.h"
#include "game/debug/dev_tools.h"
#include "scripts/acceptance/moving_structure/fixtures.h"
#include <iostream>

#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#include "game/debug/replay.h"
#include "game/debug/tune_document.h"
#include <process.h>

namespace moving_structure_acceptance
{
namespace
{
    std::string read_text(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        require(stream.good(), "Cannot read moving replay fixture");
        return { std::istreambuf_iterator<char>(stream), {} };
    }
    void write_text(const std::filesystem::path& path, const std::string& text)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        stream.write(text.data(), static_cast<std::streamsize>(text.size())); stream.close();
        require(static_cast<bool>(stream), "Cannot write moving replay fixture");
    }
    ReplayFile header()
    {
        ReplayFile file;
        file.build = "moving-manual-fixture"; file.dt_bits = bits(dt); file.seed = 17u;
        MatchSetup settings;
        settings.set_player_count(1); settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_player_team(0, PlayerTeam::a); settings.set_player_weapon(0, WeaponType::sprayer);
        settings.set_stage("manual-moving-fixture"); settings.set_duration(120.0f);
        settings.set_screen_layout(labrador::ScreenLayout::one_player);
        file.tape.start(0, settings);
        return file;
    }
    std::unique_ptr<Fixture> ferry(World& world)
    {
        auto fixture = std::make_unique<Fixture>(world, 1,
            mattmath::RectangleF{ -1024.0f, -1024.0f, 8192.0f, 4096.0f }, 17u);
        fixture->tuning = world.variant([](rapidjson::Document& document)
        { set_countdown(document, dt / 4.0f); });
        // A static role precedes both movers: motion IDs are not paint-source
        // or all-support ordinals. All terrain and the full roster precede build.
        fixture->surface({ 900.0f, 1200.0f, 2000.0f, 20.0f });
        auto& boat = fixture->mover({ { 1000.0f, 800.0f, 320.0f, 16.0f }, 120.0f, 6.4f, 0.25f });
        fixture->mover({ { 2200.0f, 650.0f, 96.0f, 20.0f }, 64.0f, 4.0f, 0.75f });
        fixture->add_player(boat.bounds().center().x, boat.bounds().top(), PlayerTeam::a,
            WeaponType::sprayer, input_slots::keyboard_and_mouse);
        fixture->build(); fixture->start(); fixture->settle();
        return fixture;
    }

    // Independent literal version-5 wire control; the dynamic domain stays absent.
    // The version-3 literal (with the old zero shorthand) and version 4, which
    // added roster metadata, are refused by version.
    void legacy_bytes(const std::filesystem::path& path)
    {
        ReplayFile file = header(); file.build = "legacy"; file.dt_bits = 0; file.seed.reset();
        MatchSetup untimed = file.tape.settings(); untimed.set_duration(std::nullopt); file.tape.start(0, untimed);
        file.final_print.seats.emplace_back();
        const std::string expected =
            "{\n  \"version\": 3,\n  \"build\": \"legacy\",\n  \"dt\": \"0\",\n"
            "  \"tuning_hash\": \"0000000000000000\",\n  \"presentation_hash\": \"0000000000000000\",\n"
            "  \"tune_diff\": [],\n  \"cheats\": { \"god\": false, \"infinite_ammo\": false },\n"
            "  \"settings\": {\n    \"stage\": \"manual-moving-fixture\",\n    \"best_of\": 1,\n"
            "    \"layout\": 0,\n    \"seats\": [\n      { \"slot\": 4, \"team\": \"a\", \"weapon\": \"sprayer\" }\n"
            "    ]\n  },\n  \"updates\": 0,\n  \"suspends\": [],\n  \"inputs\": [\n  ],\n  \"checkpoints\": [],\n"
            "  \"final\": {\n    \"updates\": 0,\n    \"hash\": \"0000000000000000\",\n"
            "    \"paint\": \"A 0 B 0 of 0\",\n    \"team_a\": \"0\",\n    \"team_b\": \"0\",\n"
            "    \"total\": \"0\",\n    \"level_state\": 0,\n    \"seats\": [\n"
            "      { \"x\": \"0\", \"y\": \"0\", \"vx\": \"0\", \"vy\": \"0\", \"health\": \"0\", "
            "\"ammo\": \"0\", \"respawn\": \"0\", \"move_state\": 0, \"alive\": 0 }\n    ]\n  }\n}\n";
        std::string version_4 = expected;
        version_4.replace(version_4.find("\"version\": 3"), 12, "\"version\": 4");
        const std::string old_seat = "\"weapon\": \"sprayer\" }";
        version_4.replace(version_4.find(old_seat), old_seat.size(),
            "\"weapon\": \"sprayer\", \"controller\": \"human\", \"difficulty\": \"normal\" }");
        // Version 5 lists gaps in place of suspends and one input array per participant.
        std::string current = version_4;
        current.replace(current.find("\"version\": 4"), 12, "\"version\": 5");
        const std::string old_tape = "\"suspends\": [],\n  \"inputs\": [\n  ],";
        current.replace(current.find(old_tape), old_tape.size(), "\"gaps\": [],\n  \"inputs\": [\n    []\n  ],");
        file.write(path); require(read_text(path) == current, "Static version-5 replay bytes changed");
        file.final_print.seats[0].paint_underfoot = true;
        file.final_print.seats[0].support_paint_team = 1; file.final_print.seats[0].step_paint_team = 2;
        std::string active = current;
        const std::string needle = "\"alive\": 0 }";
        active.replace(active.find(needle), needle.size(),
            "\"alive\": 0, \"paint_underfoot\": { \"support_team\": 1, \"step_team\": 2 } }");
        file.write(path); require(read_text(path) == active, "POL36-only version-5 replay bytes changed");
        const auto decoded = ReplayFile::read(path);
        require(!decoded.final_print.horizontal_motion &&
            MatchFingerprint::difference(file.final_print, decoded.final_print).empty(),
            "POL36-only static replay changed meaning");
        write_text(path, current);
        const ReplayFile static_read = ReplayFile::read(path);
        require(!static_read.final_print.horizontal_motion && static_read.tape.settings().human_count() == 1 &&
            static_read.tape.settings().bot_count() == 0, "Static domain or bot appeared on version-5 read");
        // A replay holds only on the build that recorded it: older versions are refused, not read.
        for (const auto& [text, version] : { std::pair{ expected, 3 }, std::pair{ version_4, 4 } })
        {
            write_text(path, text);
            throws_with([&] { static_cast<void>(ReplayFile::read(path)); },
                "format version " + std::to_string(version) + "; this build reads only 5");
        }
    }

    std::uint64_t legacy_hash(const Level& level, int updates)
    {
        const auto fold = [](std::uint64_t hash, std::uint32_t value)
        {
            const char bytes[4] = { static_cast<char>(value & 255), static_cast<char>((value >> 8) & 255),
                static_cast<char>((value >> 16) & 255), static_cast<char>((value >> 24) & 255) };
            return fnv1a(std::string_view(bytes, 4), hash);
        };
        std::uint64_t hash = fold(fnv1a("colourwars-fingerprint-1"), static_cast<std::uint32_t>(updates));
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            for (const auto value : { bits(player.center().x), bits(player.center().y), bits(player.velocity().x),
                bits(player.velocity().y), bits(player.health()), bits(player.weapon_ammo()), bits(player.respawn_timer()),
                static_cast<std::uint32_t>(player.move_state()), player.state() == PlayerState::alive ? 1u : 0u })
            { hash = fold(hash, value); }
            const auto effect = player.paint_underfoot_state();
            if (effect.enabled)
            {
                hash = fnv1a("colourwars-underfoot-state-1", hash);
                hash = fold(hash, static_cast<std::uint32_t>(effect.support_team));
                hash = fold(hash, static_cast<std::uint32_t>(effect.step_team));
            }
        }
        const auto info = level.level_end_info();
        for (const float value : { info.team_a_area, info.team_b_area, info.total_area }) { hash = fold(hash, bits(value)); }
        for (const float value : info.player_areas) { hash = fold(hash, bits(value)); }
        return fold(hash, static_cast<std::uint32_t>(level.state()));
    }
    void static_hashes(World& world)
    {
        for (bool active : { false, true })
        {
            Fixture fixture(world);
            fixture.tuning = world.variant([&](rapidjson::Document& document)
            {
                set_countdown(document, dt / 4.0f);
                document["player"]["paint_underfoot"]["enabled"].SetBool(active);
            });
            auto& floor = fixture.surface({ 1000.0f, 800.0f, 320.0f, 16.0f });
            world.paint(floor, floor.bounds(), PlayerTeam::a, input_slots::keyboard_and_mouse);
            fixture.add_player(1160.0f, 800.0f); fixture.build(); fixture.start(); fixture.settle();
            for (int step = 0; step < 12; ++step)
            {
                fixture.tick(); const auto print = MatchFingerprint::of(*fixture.level, step);
                require(!print.horizontal_motion && print.hash == legacy_hash(*fixture.level, step),
                    "Static/POL36-only fingerprint appended new hash work");
                require(print.seats[0].paint_underfoot == active, "Static prototype oracle did not exercise requested state");
            }
        }
    }

    template<class Edit> void malformed(const std::filesystem::path& path,
        const std::string& original, Edit edit, const std::string& diagnostic = "horizontal_motion")
    {
        rapidjson::Document document; document.Parse(original.c_str());
        require(!document.HasParseError(), "Cannot parse valid moving fixture output"); edit(document);
        rapidjson::StringBuffer buffer; rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        require(document.Accept(writer), "Cannot encode malformed moving control"); write_text(path, buffer.GetString());
        throws_with([&] { static_cast<void>(ReplayFile::read(path)); }, diagnostic);
    }
    void codec_controls(const std::filesystem::path& path, const ReplayFile& file)
    {
        file.write(path); const std::string original = read_text(path);
        require(original.find("\"horizontal_motion\"") != std::string::npos &&
            original.find("\"offsets\"") != std::string::npos &&
            original.find("\"motion_id\"") != std::string::npos &&
            original.find("\"00000000\"") != std::string::npos, "Missing strict moving wire domain");
        const auto round = ReplayFile::read(path);
        require(MatchFingerprint::difference(file.final_print, round.final_print).empty(), "Dynamic full state did not round-trip");
        round.write(path); require(read_text(path) == original, "Dynamic canonical bytes did not round-trip");
        // Uppercase reads are accepted, but the writer restores canonical lowercase.
        {
            rapidjson::Document upper; upper.Parse(original.c_str());
            upper["final"]["horizontal_motion"]["structures"][0u]["spec"][0u].SetString("447A0000",upper.GetAllocator());
            rapidjson::StringBuffer buffer; rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            require(upper.Accept(writer),"Cannot encode uppercase dynamic control"); write_text(path,buffer.GetString());
            ReplayFile::read(path).write(path); require(read_text(path)==original,"Uppercase dynamic hex did not canonicalize");
        }
        // Every mutation starts from the same valid actual-Level final snapshot.
        malformed(path, original, [](auto& d) { d["final"]["horizontal_motion"]["schema"].SetInt(2); });
        malformed(path, original, [](auto& d) { d["final"]["horizontal_motion"].RemoveMember("structures"); });
        malformed(path, original, [](auto& d) { d["final"]["horizontal_motion"]["structures"].Clear(); });
        for (const char* text : { "0", "00000000000000000", "000000000000000g", "+000000000000000", "7ff0000000000000", "7ff8000000000000", "3ff0000000000000" })
        { malformed(path, original, [&](auto& d) { d["final"]["horizontal_motion"]["structures"][0u]["phase"].SetString(text, d.GetAllocator()); }); }
        for (const char* text : { "0", "000000000", "0000000g", "7f800000", "7fc00000" })
        { malformed(path, original, [&](auto& d) { d["final"]["horizontal_motion"]["structures"][0u]["spec"][0u].SetString(text, d.GetAllocator()); }); }
        for (const char* name : { "spec", "offsets", "pose" })
        { malformed(path, original, [&](auto& d) { d["final"]["horizontal_motion"]["structures"][0u][name].PopBack(); }); }
        for (int id : { -1, 0, 2 })
        { malformed(path, original, [&](auto& d) { d["final"]["horizontal_motion"]["structures"][0u]["id"].SetInt(id); }); }
        malformed(path, original, [](auto& d) { d["final"]["horizontal_motion"]["structures"][1u]["id"].SetInt(1); });
        malformed(path, original, [](auto& d) { d["final"]["horizontal_motion"]["structures"][0u]["direction"].SetInt(0); });
        malformed(path, original, [](auto& d) { auto& v=d["final"]["horizontal_motion"]["structures"][0u]["direction"]; v.SetInt(-v.GetInt()); });
        for (unsigned field : { 2u, 4u, 5u })
        { malformed(path, original, [&](auto& d) { d["final"]["horizontal_motion"]["structures"][0u]["spec"][field].SetString("00000000", d.GetAllocator()); }); }
        malformed(path, original, [](auto& d) { d["final"]["horizontal_motion"]["structures"][0u]["offsets"][1u].SetString("3f800000", d.GetAllocator()); });
        malformed(path, original, [](auto& d) { d["final"]["horizontal_motion"]["structures"][0u]["offsets"][2u].SetString("00000000", d.GetAllocator()); });
        malformed(path, original, [](auto& d) { d["final"]["horizontal_motion"]["structures"][0u]["pose"][2u].SetString("00000000", d.GetAllocator()); });
        for (const char* name : { "face", "team" })
        { malformed(path, original, [&](auto& d) { d["final"]["horizontal_motion"]["structures"][0u]["tiles"][0u][name].SetInt(7); }); }
        for (const char* area : { "bf800000", "7f800000", "7fc00000" })
        { malformed(path, original, [&](auto& d) { d["final"]["horizontal_motion"]["structures"][0u]["tiles"][0u]["area"].SetString(area, d.GetAllocator()); }); }
        malformed(path, original, [](auto& d) { d["final"]["horizontal_motion"]["structures"][0u]["tiles"][0u]["corners"].PopBack(); });
        malformed(path, original, [](auto& d) { d["final"]["horizontal_motion"]["structures"][0u]["tiles"][0u]["corners"][0u].SetString("7f800000", d.GetAllocator()); });
        for (int id : { -1, 3 })
        { malformed(path, original, [&](auto& d) { d["final"]["horizontal_motion"]["carriers"][0u]["motion_id"].SetInt(id); }); }
        malformed(path, original, [](auto& d) { d["final"]["horizontal_motion"]["carriers"].Clear(); });
        malformed(path, original, [](auto& d) { auto& a=d["final"]["horizontal_motion"]["carriers"]; rapidjson::Value extra(a[0u],d.GetAllocator()); a.PushBack(extra,d.GetAllocator()); });
        malformed(path, original, [](auto& d) { d["final"]["horizontal_motion"]["carriers"][0u].SetInt(0); });
        malformed(path, original, [](auto& d) { auto& a=d["final"]["seats"]; rapidjson::Value extra(a[0u],d.GetAllocator()); a.PushBack(extra,d.GetAllocator()); });
        malformed(path, original, [](auto& d) {
            auto& a=d["settings"]["seats"]; rapidjson::Value extra(a[0u],d.GetAllocator()); extra["slot"].SetInt(-1);
            a.PushBack(extra,d.GetAllocator()); d["settings"]["layout"].SetInt(1);
            auto& i=d["inputs"]; rapidjson::Value seat(i[0u],d.GetAllocator()); i.PushBack(seat,d.GetAllocator());
        });
        malformed(path, original, [](auto& d) { auto& t=d["final"]["horizontal_motion"]["structures"][0u]["tiles"]; t[0u].Swap(t[1u]); });
        for (int owner : { (std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)() })
        {
            ReplayFile raw = file; raw.final_print.horizontal_motion->structures[0].tiles[0].owner = owner;
            raw.write(path); const auto decoded = ReplayFile::read(path);
            require(decoded.final_print.horizontal_motion->structures[0].tiles[0].owner == owner,
                "Signed raw-owner boundary was normalized or truncated");
        }
        write_text(path, original);
        ReplayFile invalid = file; invalid.final_print.horizontal_motion->carriers.clear();
        throws_with([&] { invalid.write(path); }, "carrier count");
        require(read_text(path)==original,"Invalid dynamic write touched the existing target");
        // A missing domain must diverge even if its hash is copied from actual.
        MatchFingerprint absent = file.final_print; absent.horizontal_motion.reset();
        require(MatchFingerprint::difference(absent, file.final_print).find("presence") != std::string::npos &&
            MatchFingerprint::difference(file.final_print, absent).find("presence") != std::string::npos,
            "A copied hash concealed a missing horizontal-motion domain");
        MatchFingerprint changed = file.final_print;
        changed.horizontal_motion->structures[0].direction *= -1;
        require(MatchFingerprint::difference(file.final_print, changed).find("direction") != std::string::npos,
            "Equal-centre opposite direction was hidden by aggregate/hash");
        changed = file.final_print;
        auto& state = changed.horizontal_motion->structures[0];
        const float previous = float_from_bits(state.offsets[0]);
        const float anchor = float_from_bits(state.spec[0]);
        bool alias = false;
        for (const float toward : { 0.0f, float_from_bits(state.spec[4]) })
        {
            const float offset = std::nextafter(previous, toward);
            if (bits(offset) != state.offsets[0] && bits(anchor + offset) == state.pose[0])
            { state.offsets[0] = bits(offset); alias = true; break; }
        }
        require(alias && state.pose == file.final_print.horizontal_motion->structures[0].pose &&
            MatchFingerprint::difference(file.final_print, changed).find("offset") != std::string::npos,
            "Exact offset bits were reconstructed from equal rounded world poses");
        ReplayFile aliased = file; aliased.final_print = changed; aliased.write(path);
        require(MatchFingerprint::difference(changed,ReplayFile::read(path).final_print).empty(),
            "A valid distinct historical offset was reconstructed during codec round-trip");
    }

    void arrangement(World& world)
    {
        const auto make = [&](std::size_t painted)
        {
            auto fixture = std::make_unique<Fixture>(world);
            auto& moving = fixture->mover({ { 1000.0f, 800.0f, 320.0f, 16.0f }, 120.0f, 6.4f, 0.25f }, { false,true,false,false }); // Top face.
            const auto samples = snapshot(moving);
            require(samples.size() > painted && samples[0].scored_area == samples[painted].scored_area,
                "Local arrangement control needs equal full-sized actual tiles");
            const auto& tile = samples[painted];
            const float x = (tile.corners[0].x + tile.corners[1].x) / 2.0f;
            const float y = (tile.corners[0].y + tile.corners[3].y) / 2.0f;
            world.paint(moving, { x-0.25f,y-0.25f,0.5f,0.5f }, PlayerTeam::a, 4);
            fixture->add_player(moving.bounds().center().x, moving.bounds().top()); fixture->build();
            return fixture;
        };
        const auto left = make(2); const auto right = make(4);
        same_total(left->movers[0]->paint_total(), right->movers[0]->paint_total());
        require(left->movers[0]->paint_total().team_a > 0.0f, "Arrangement controls never painted a real tile");
        const auto a = MatchFingerprint::of(*left->level, 0); const auto b = MatchFingerprint::of(*right->level, 0);
        require(a.hash != b.hash && MatchFingerprint::difference(a,b).find("local tile") != std::string::npos,
            "Equal aggregate score hid different actual local tile arrangements");
    }
}

void check_replay(World& world)
{
    const auto directory = std::filesystem::temp_directory_path() /
        (L"colourwars_horizontal_replay_" + std::to_wstring(_getpid()));
    require(!std::filesystem::exists(directory),"Refusing to overwrite an earlier owned replay fixture directory");
    std::filesystem::create_directories(directory);
    const auto path = directory / L"manual.json";
    legacy_bytes(path); static_hashes(world); arrangement(world);
    constexpr int length = 240;
    {
        const auto probe = ferry(world);
        std::srand(2718u); const int expected_random = std::rand(); std::srand(2718u);
        static_cast<void>(MatchFingerprint::of(*probe->level,0));
        require(std::rand()==expected_random,"Owned horizontal fingerprint consumed global RNG");
        // Destroy before the next seeded factory; do not advance a changed stream.
    }
    auto original = ferry(world);
    ReplayRecorder recorder("manual-moving", header(), nullptr, length);
    std::vector<MatchFingerprint> observations;
    const auto before = MatchFingerprint::of(*original->level, 0);
    require(before.horizontal_motion && before.horizontal_motion->structures.size() == 2 &&
        before.horizontal_motion->structures[0].id == 1 && before.horizontal_motion->structures[1].id == 2,
        "Actual admission did not yield canonical mover-only IDs");
    bool carried = false, painted = false, paid = false, translated = false;
    for (int step = 0; step < length; ++step)
    {
        // Both requested direction AND angle drive the actual Weapon launch.
        const auto action = aimed_input({0.0f,1.0f}, 1.5707963267948966f, step < length/2);
        original->tick(action, dt, &recorder);
        const auto print = MatchFingerprint::of(*original->level, step+1);
        observations.push_back(print);
        carried = carried || print.horizontal_motion->carriers[0] == 1;
        translated = translated || print.seats[0].x != before.seats[0].x;
        paid = paid || original->players[0]->weapon_ammo() < float_from_bits(before.seats[0].ammo);
        for (const auto& tile : print.horizontal_motion->structures[0].tiles) { painted = painted || tile.team == static_cast<int>(PlayerTeam::a); }
        if ((step+1) % ReplayFile::checkpoint_interval == 0) { recorder.add_checkpoint(print); }
    }
    require(carried && translated && paid && painted && observations.back().horizontal_motion->structures[0].phase !=
        before.horizontal_motion->structures[0].phase, "Manual replay never exercised actual carry, motion and paid paint");
    recorder.file().updates = length; recorder.file().final_print = observations.back(); recorder.file().write(path);
    const unsigned int terminal_random = static_cast<unsigned int>(std::rand());
    const auto decoded = ReplayFile::read(path);
    require(decoded.updates == length && decoded.inputs_by_seat().size() == 1 && decoded.inputs_by_seat()[0].size() == length &&
        decoded.checkpoints.size() == 4 && decoded.seed == 17u && decoded.dt_bits == bits(dt),
        "Manual actual-Level tape metadata/input/checkpoints are incomplete");
    auto playback = ferry(world); ReplayPlayer player("manual-moving", decoded);
    std::size_t checkpoint = 0;
    for (int step = 0; step < length; ++step)
    {
        playback->tick(input(), dt, &player);
        const auto actual = MatchFingerprint::of(*playback->level, step+1);
        const auto difference = MatchFingerprint::difference(observations[static_cast<std::size_t>(step)], actual);
        require(difference.empty(), "Actual manual-Level playback diverged at " + std::to_string(step+1) + ": " + difference);
        if ((step+1) % ReplayFile::checkpoint_interval == 0)
        {
            require(decoded.checkpoints[checkpoint].update == step+1 && decoded.checkpoints[checkpoint].hash == actual.hash,
                "Actual moving checkpoint differs"); ++checkpoint;
        }
    }
    require(MatchFingerprint::difference(decoded.final_print, MatchFingerprint::of(*playback->level,length)).empty() &&
        static_cast<unsigned int>(std::rand()) == terminal_random, "Actual manual replay final state or terminal RNG differs");
    codec_controls(path, recorder.file());
    std::error_code error; std::filesystem::remove(path,error); require(!error,"Cannot remove owned replay fixture");
    std::filesystem::remove(directory,error); require(!error,"Cannot remove owned replay directory");
    std::cout << "PASS moving manual Level Recorder/Player, strict optional schema and legacy bytes/hash\n";
}
}
#else
namespace moving_structure_acceptance
{
void check_replay(World&)
{ std::cout << "LIMIT moving replay requires Debug dev tools; Release retains unconditional value layout\n"; }
}
#endif
