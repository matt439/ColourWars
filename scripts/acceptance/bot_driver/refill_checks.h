#pragma once

#include "game/ai/match_bot_driver.h"
#include "scripts/acceptance/support/probe.h"

namespace bot_refill_acceptance
{
    class RefillWorld final : public BotWorld
    {
    public:
        explicit RefillWorld(const GameTuning& tuning) : size_(tuning.player().size) {}

        mutable int footing_queries = 0;

        int participant_count() const override { return 2; }
        BotBody body(int participant) const override
        {
            BotBody result;
            result.center.x = participant == 0 ? 100.0f : 0.0f;
            result.team = participant == 0 ? PlayerTeam::a : PlayerTeam::b;
            result.alive = result.grounded = true;
            result.health = 100.0f;
            result.underfoot_enabled = true;
            result.underfoot_team = PlayerTeam::a;
            result.tool_empty = true;
            return result;
        }
        std::vector<PlayerTeam> gate_permissions() const override { return {}; }
        std::vector<PaintSurfaceSample> paint_surfaces() const override { return {}; }
        std::vector<labrador::RectangleF> obstacles(PlayerTeam) const override
        { return { floor() }; }
        std::vector<MatchBotSupport> supports(PlayerTeam) const override
        { return { { floor(), false } }; }
        std::vector<MatchBotRamp> ramps() const override { return {}; }
        std::optional<PlayerTeam> top_paint_team(labrador::Vector2F) const override
        {
            ++this->footing_queries;
            return PlayerTeam::a;
        }
        bool line_of_sight(labrador::Vector2F, labrador::Vector2F) const override { return true; }
        bool traversal_line_of_sight(labrador::Vector2F, labrador::Vector2F, PlayerTeam) const override
        { return true; }

    private:
        labrador::Vector2F size_;

        labrador::RectangleF floor() const
        { return { -2000.0f, this->size_.y * 0.5f, 4000.0f, 100.0f }; }
    };

    inline void check_driver(const GameTuning& tuning)
    {
        using acceptance::require;
        for (const MatchVariant variant : { MatchVariant::standard, MatchVariant::endless_paint })
        {
            MatchSetup setup;
            setup.set_player_count(2);
            setup.set_player_bot(1, BotDifficulty::normal);
            setup.set_variant(variant);
            MatchBotDriver driver(&tuning, setup, 12);
            RefillWorld world(tuning);
            std::vector<PlayerInputData> inputs(2);
            inputs.front().x_movement = 0.25f;
            for (int step = 0; step < tuning.bot(BotDifficulty::normal).reaction_steps; ++step)
            { driver.drive(inputs, world); }
            require(inputs.front().x_movement == 0.25f,
                "A bot variant changed a human participant's input");
            if (variant == MatchVariant::endless_paint)
            {
                require(!driver.bot(1)->refilling() && inputs.back().primary_shoot &&
                    world.footing_queries == 0,
                    "Endless-paint setup failed to disable the driver's refill policy or ground-ahead scans");
            }
            else
            {
                require(driver.bot(1)->refilling() && !inputs.back().primary_shoot &&
                    world.footing_queries > 0,
                    "A standard setup lost its refill policy or ground-ahead scans");
            }
        }
    }
}
