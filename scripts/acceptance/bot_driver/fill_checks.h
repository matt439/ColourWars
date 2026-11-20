#pragma once

#include "game/ai/match_bot_driver.h"
#include "game/ai/match_bot_ballistics.h"
#include "scripts/acceptance/support/probe.h"

namespace bot_fill_acceptance
{
    class FillWorld final : public BotWorld
    {
    public:
        explicit FillWorld(const GameTuning& tuning) : tuning_(tuning) {}
        bool present = true;
        bool target_hidden = false;
        PlayerTeam leader = PlayerTeam::none;
        PlayerTeam last = PlayerTeam::none;
        PlayerTeam second_target = PlayerTeam::b;
        labrador::Vector2F trigger{ -120.0f, 0.0f };

        std::vector<labrador::Vector2F> more_triggers;

        int participant_count() const override { return 1; }
        BotBody body(int) const override
        {
            BotBody bot;
            bot.team = PlayerTeam::a;
            bot.alive = bot.grounded = bot.tool_ready = true;
            bot.health = 100.0f;
            bot.ammo = tuning_.starting_ammo();
            return bot;
        }
        std::optional<BotFillObservation> visible_fill(PlayerTeam,
            const std::function<bool(labrador::Vector2F)>& sees) const override
        {
            if (!present || !sees(trigger)) { return std::nullopt; }
            BotFillObservation observation{};
            observation.trigger_tiles = { { { 90.0f, 0.0f }, PlayerTeam::a, 48.0f },
                { trigger, PlayerTeam::none, 48.0f } };
            for (const auto point : more_triggers)
            {
                if (sees(point)) { observation.trigger_tiles.push_back({ point, PlayerTeam::b, 48.0f }); }
            }
            observation.target_tiles.push_back({ { 0.0f, -10.0f }, PlayerTeam::a, 48.0f });
            if (sees({ 0.0f, -20.0f }))
            { observation.target_tiles.push_back({ { 0.0f, -20.0f }, second_target, 48.0f }); }
            observation.leader = leader;
            observation.last_team = last;
            return observation;
        }
        bool line_of_sight(labrador::Vector2F, labrador::Vector2F point) const override
        { return !target_hidden || point.y != -20.0f; }
        bool traversal_line_of_sight(labrador::Vector2F, labrador::Vector2F, PlayerTeam) const override
        { return true; }
        std::vector<PlayerTeam> gate_permissions() const override { return {}; }
        std::vector<PaintSurfaceSample> paint_surfaces() const override { return {}; }
        std::vector<labrador::RectangleF> obstacles(PlayerTeam) const override { return {}; }
        std::vector<MatchBotSupport> supports(PlayerTeam) const override { return {}; }
        std::vector<MatchBotRamp> ramps() const override { return {}; }
        std::optional<PlayerTeam> top_paint_team(labrador::Vector2F) const override { return {}; }
        std::vector<BotPlantedSub> planted_subs() const override { return {}; }

    private:
        const GameTuning& tuning_;
    };

    inline void check_driver(const GameTuning& tuning)
    {
        using acceptance::require;
        FillWorld world(tuning);
        const auto react = [&]()
        {
            MatchBotDriver driver(&tuning, { BotDifficulty::hard }, 17);
            std::vector<PlayerInputData> seats(1);
            for (int i = 0; i < tuning.bot(BotDifficulty::hard).reaction_steps; ++i)
            { driver.drive(seats, world); }
            return seats.front();
        };
        world.present = false;
        const auto ordinary = react();
        world.present = true;
        const auto capture = react();
        require(capture.primary_shoot && capture.shoot_direction.x < -0.9f &&
            capture.shoot_direction != ordinary.shoot_direction &&
            capture.x_movement == ordinary.x_movement && capture.jump_pressed == ordinary.jump_pressed,
            "A visible partially enemy top did not redirect just the shot to its non-team trigger");
        world.target_hidden = true;
        require(react().shoot_direction == ordinary.shoot_direction, "A bot valued unseen enemy fill tiles");
        world.target_hidden = false;
        world.second_target = PlayerTeam::a;
        require(react().shoot_direction == ordinary.shoot_direction, "A bot sought a wholly friendly fill");
        world.second_target = PlayerTeam::none;
        world.last = PlayerTeam::a;
        require(react().shoot_direction == ordinary.shoot_direction, "A bot tried to repeat its own fill");
        world.last = PlayerTeam::b;
        world.leader = PlayerTeam::a;
        require(react().shoot_direction == ordinary.shoot_direction, "A bot targeted a trigger it already leads");
        world.leader = PlayerTeam::none;
        // A sprayer at (1913.9, 718) beside the Yard baffle: its nearest
        // left tile lies inside the muzzle, while the far end is hittable.
        world.trigger = { 26.0f, 4.0f };
        require(react().shoot_direction == ordinary.shoot_direction, "A bot shot a trigger inside its muzzle");
        world.more_triggers.emplace_back(26.0f, 80.0f);
        const auto fallback = react();
        require(fallback.primary_shoot && fallback.shoot_direction.y > 0.8f &&
            fallback.shoot_direction != ordinary.shoot_direction && fallback.x_movement == ordinary.x_movement,
            "An unreachable nearest fill tile hid a reachable tile farther along the same face");
        // More eligible tiles than the flight budget, all inside the muzzle
        // except the far end. The sampled ranks must retain that end.
        const WeaponTuning& tool = tuning.weapon(WeaponType::sprayer);
        const ProjectileTuning& projectile = tuning.projectile(tool.details.proj_type);
        require(match_bot_ballistics::paint_angle(tool, projectile, {}, world.more_triggers.front()).has_value(),
            "The sprayer fixture needs a reachable far tile");
        for (const float y : { 8.0f, 16.0f, 20.0f, 32.0f })
        {
            const labrador::Vector2F point(26.0f, y);
            require(!match_bot_ballistics::paint_angle(tool, projectile, {}, point),
                "The sprayer fixture needs unreachable intermediate tiles");
            world.more_triggers.push_back(point);
        }
        require(react().shoot_direction == fallback.shoot_direction,
            "The bounded fill shot search discarded the reachable far end of a long strip");
        world.more_triggers.clear();
        world.trigger.x = -10000.0f;
        require(react().shoot_direction == ordinary.shoot_direction, "A bot aimed at a distant trigger");
    }
}
