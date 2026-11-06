#pragma once

#include "scripts/acceptance/paint_surfaces/fixtures.h"
#include "game/debug/dev_tools.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/replay/match_fingerprint.h"
#endif
#include <array>
#include <string>

namespace bot_navigation_acceptance
{
    using namespace labrador;
    using paint_surfaces_acceptance::require;

    // The bot seed srand(41731) gave before HYG#39: the round seed is now the
    // shipping bots' seed itself, so the trials' bots are unchanged.
    constexpr unsigned int trial_seed = 5242;
    constexpr float endpoint_tolerance = 0.05f;
    inline const float dt = static_cast<float>(StepTimer::TicksToSeconds(StepTimer::SecondsToTicks(1.0 / 60.0)));

    class World
    {
    public:
        explicit World(const std::filesystem::path& content, const char* registry_name = "bot-navigation",
            const char* support_error = "Bot navigation needs authored support '",
            const char* stage = "close_quarters") :
            assets(content), levels(registry_name), support_error_(support_error)
        {
            auto info = std::make_unique<LevelLoadedInfo>(path_to_utf8(content / "levels" /
                (std::string(stage) + ".json")).c_str());
            info->validate();
            const JsonValue entries = info->collision_objects_json();
            for (std::size_t index = 0; index < entries.size(); ++index)
            {
                const JsonValue entry = entries.at(index);
                if (!entry.has("name") || !entry.has("rectangle")) { continue; }
                const JsonValue shape = entry.object("rectangle");
                supports.emplace_back(entry.string("name"), RectangleF(shape.number("x"),
                    shape.number("y"), shape.number("width"), shape.number("height")));
            }
            levels.add(stage, std::move(info));
        }

        const RectangleF& named(const std::string& name) const
        {
            for (const auto& [key, rectangle] : supports)
            { if (key == name) { return rectangle; } }
            throw std::runtime_error(support_error_ + name + "'");
        }

        // The round seed, which is the shipping bots' seed (MatchRandom).
        std::unique_ptr<Level> build(const MatchSetup& settings, unsigned int seed = trial_seed)
        {
            viewports.set_layout(settings.screen_layout());
            LevelBuilder builder(&viewports, &assets.render, &assets.audio, &assets.preferences,
                &levels, &assets.tuning, &assets.presentation, &resolution, nullptr, &partitioner);
            return builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off,
                MatchRandom(seed));
        }

        paint_surfaces_acceptance::World assets;
        std::vector<std::pair<std::string, RectangleF>> supports;
        Registry<LevelLoadedInfo> levels;
        ResolutionManager resolution;
        ViewportManager viewports{ &resolution };
        Partitioner partitioner;
    private:
        const char* support_error_;
    };

    inline PlayerTeam opposing(PlayerTeam team)
    {
        return team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a;
    }

    // The route and trial cases' roster: an idle human on the keyboard in
    // seat 0, opposing the one bot in seat 1, both carrying `tool`.
    inline MatchSetup one_bot_roster(const std::string& stage, float duration, PlayerTeam bot_team,
        WeaponType tool, BotDifficulty difficulty)
    {
        MatchSetup settings;
        settings.set_player_count(2);
        settings.set_player_bot(1, difficulty);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_stage(stage); settings.set_duration(duration);
        settings.set_player_team(0, opposing(bot_team));
        settings.set_player_team(1, bot_team);
        settings.set_player_weapon(0, tool); settings.set_player_weapon(1, tool);
        return settings;
    }

    // Two a side: seats 0 and 1 on `near_team`, seats 2 and 3 against them.
    // Seat 0 is an idle human on the keyboard. `bots` marks the bot seats; a
    // human seat 3 is a second idle human on pad slot 0, with its own pane.
    inline MatchSetup two_a_side_roster(const std::string& stage, float duration, PlayerTeam near_team,
        const std::array<WeaponType, 4>& tools, const std::array<bool, 4>& bots, BotDifficulty difficulty)
    {
        MatchSetup settings;
        settings.set_player_count(4);
        for (int seat = 1; seat < 4; ++seat)
        { if (bots[static_cast<std::size_t>(seat)]) { settings.set_player_bot(seat, difficulty); } }
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        if (!bots[3]) { require(settings.claim_input_slot(0), "A two-a-side roster could not seat its second human"); }
        settings.set_screen_layout(bots[3] ? ScreenLayout::one_player : ScreenLayout::two_player);
        settings.set_stage(stage); settings.set_duration(duration);
        for (int seat = 0; seat < 4; ++seat)
        {
            settings.set_player_team(seat, seat < 2 ? near_team : opposing(near_team));
            settings.set_player_weapon(seat, tools[static_cast<std::size_t>(seat)]);
        }
        return settings;
    }

    inline std::vector<PlayerInputData> physical_inputs()
    {
        std::vector<PlayerInputData> physical(input_slots::count);
        for (PlayerInputData& input : physical) { input.connected = true; }
        return physical;
    }

    // Top-face paint the bot in seat 1 owns on `support`'s top.
    inline float owned_top_paint(const Level& level, const RectangleF& support, float thickness)
    {
        const int owner = paint_owners::index(-1, 1);
        float area = 0.0f;
        for (const PaintSurfaceSample& surface : level.paint_surfaces())
        {
            if (surface.face != PaintSurfaceFace::top || surface.paint_owner != owner) { continue; }
            bool belongs = true;
            for (const Vector2F& corner : surface.corners)
            {
                belongs = belongs && corner.x >= support.left() - endpoint_tolerance &&
                    corner.x <= support.right() + endpoint_tolerance &&
                    std::fabs(corner.y - support.top()) <= thickness + endpoint_tolerance;
            }
            if (belongs) { area += surface.scored_area; }
        }
        return area;
    }

    inline bool grounded(const Player& player)
    {
        return player.grounded();
    }

    inline bool on_support(const Player& player, const RectangleF& support)
    {
        const RectangleF& body = player.rectangle();
        return grounded(player) && std::fabs(body.bottom() - support.top()) <= endpoint_tolerance &&
            body.right() > support.left() + endpoint_tolerance &&
            body.left() < support.right() - endpoint_tolerance;
    }

    class Tape final : public SeatInputFilter
    {
    public:
        explicit Tape(const char* exhaustion_error = "Navigation playback exhausted its finite final-input tape") :
            exhaustion_error_(exhaustion_error) {}
        void filter_seats(std::vector<PlayerInputData>& seats, const Level&) override
        {
            if (playback)
            {
                require(next < steps.size(), exhaustion_error_);
                seats = steps[next++];
            }
            else { steps.push_back(seats); }
        }
        bool replaces_bot_inputs() const override { return playback; }
        std::vector<std::vector<PlayerInputData>> steps;
        bool playback = false;
        std::size_t next = 0;
    private:
        const char* exhaustion_error_;
    };

    inline std::vector<std::uint64_t> snapshot(const Level& level)
    {
        std::vector<std::uint64_t> state = paint_surfaces_acceptance::sample_state(level.paint_surfaces());
        for (int seat = 0; seat < level.player_count(); ++seat)
        {
            const Player& player = level.player(seat);
            for (const float value : { player.center().x, player.center().y, player.velocity().x,
                player.velocity().y, player.weapon_ammo(), player.health() })
            { state.push_back(paint_surfaces_acceptance::bits(value)); }
            state.push_back(static_cast<std::uint64_t>(player.state()));
            state.push_back(static_cast<std::uint64_t>(player.move_state()));
            for (const std::uint64_t count : player.lethal_projectiles().enemy_by_owner) { state.push_back(count); }
            state.push_back(player.lethal_projectiles().uncredited);
        }
        state.push_back(static_cast<std::uint64_t>(level.projectile_count()));
        return state;
    }

    // Replays a recorded trial: rebuilds its level through `rebuild`, plays
    // `tape` back for `steps` steps and requires the same public state and, in
    // Debug, the same complete fingerprint. The public final-input seam records
    // after the shipping driver; playback tells Level to bypass that driver.
    template <typename Rebuild>
    void check_final_input_replay(std::unique_ptr<Level>& level, Tape& tape, int steps,
        const std::vector<PlayerInputData>& physical, Rebuild&& rebuild, const std::string& trial)
    {
        const std::vector<std::uint64_t> expected = snapshot(*level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint expected_full = MatchFingerprint::of(*level, steps);
#endif
        // Both levels borrow the same partitioner, so destroy before rebuilding.
        level.reset();
        level = rebuild();
        tape.playback = true;
        for (int tick = 0; tick < steps; ++tick) { level->update(physical, dt, &tape); }
        require(tape.next == static_cast<std::size_t>(steps) && snapshot(*level) == expected,
            trial + " final-input replay changed players, projectiles or exact paint state");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(expected_full, MatchFingerprint::of(*level, steps)).empty(),
            trial + " final-input replay changed the complete Debug fingerprint");
#endif
    }
}
