#pragma once

#include "scripts/acceptance/paint_surfaces/fixtures.h"

namespace bot_navigation_acceptance
{
    using namespace labrador;
    using namespace mattmath;
    using paint_surfaces_acceptance::require;

    constexpr unsigned int trial_seed = 41731;
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

        // The whole-match seed reaches stage draws and then the bot seed.
        std::unique_ptr<Level> build(const MatchSetup& settings, unsigned int seed = trial_seed)
        {
            std::srand(seed);
            viewports.set_layout(settings.screen_layout());
            LevelBuilder builder(&viewports, &assets.render, &assets.audio, &assets.preferences,
                &levels, &assets.tuning, &assets.presentation, &resolution, nullptr, &partitioner, &mouse);
            return builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
        }

        paint_surfaces_acceptance::World assets;
        std::vector<std::pair<std::string, RectangleF>> supports;
        Registry<LevelLoadedInfo> levels;
        ResolutionManager resolution;
        ViewportManager viewports{ &resolution };
        Mouse mouse;
        Partitioner partitioner;
    private:
        const char* support_error_;
    };

    inline std::vector<PlayerInputData> physical_inputs()
    {
        std::vector<PlayerInputData> physical(input_slots::count);
        for (PlayerInputData& input : physical) { input.connected = true; }
        return physical;
    }

    inline bool grounded(const Player& player)
    {
        return player.move_state() == PlayerMoveState::on_ground ||
            player.move_state() == PlayerMoveState::on_drop_down_ground ||
            player.move_state() == PlayerMoveState::on_ramp_left ||
            player.move_state() == PlayerMoveState::on_ramp_right;
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
}
