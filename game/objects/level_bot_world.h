#pragma once

#include "game/ai/bot_world.h"
#include <vector>

namespace labrador
{
    class Scene;
}
class Player;
class LevelStructures;

// A Level's world as its shipping bots see it (HYG#40). Every member is a
// borrow from the owning Level, which never moves: the scene, its player
// vector (indexed by roster participant, never resized) and its structures,
// whose projectile targets are admitted after this is built. Reads go
// through the borrows on each call, so nothing here is a copy.
class LevelBotWorld final : public BotWorld
{
public:
    LevelBotWorld(const labrador::Scene& scene, const std::vector<Player*>& players,
        const LevelStructures& structures);
    LevelBotWorld(const LevelBotWorld&) = delete;
    LevelBotWorld& operator=(const LevelBotWorld&) = delete;
    LevelBotWorld(LevelBotWorld&&) = delete;
    LevelBotWorld& operator=(LevelBotWorld&&) = delete;

    int participant_count() const override;
    BotBody body(int participant) const override;
    std::vector<PlayerTeam> gate_permissions() const override;
    // Exact scoring tiles in Scene paintable-role order (Level::paint_surfaces).
    std::vector<PaintSurfaceSample> paint_surfaces() const override;
    std::vector<mattmath::RectangleF> obstacles(PlayerTeam permitted) const override;
    std::vector<MatchBotSupport> supports(PlayerTeam permitted) const override;
    std::vector<MatchBotRamp> ramps() const override;
    bool line_of_sight(mattmath::Vector2F from, mattmath::Vector2F to) const override;
    bool traversal_line_of_sight(mattmath::Vector2F from, mattmath::Vector2F to,
        PlayerTeam team) const override;

private:
    const labrador::Scene& scene_;
    const std::vector<Player*>& players_;
    const LevelStructures& structures_;
};
