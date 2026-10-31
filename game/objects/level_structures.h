#pragma once

#include "game/objects/horizontal_motion.h"
#include <optional>
#include <span>
#include <vector>

namespace labrador
{
    class Scene;
    class CollisionObject;
}
class Player;
struct PlayerTuning;
class TranslatingStructurePaintable;
class TeamGateStructurePaintable;

// The stage structures a Level drives each step (HYG#40): its translating
// movers and their supports, its team gates and the projectile targets.
// Every pointer is a borrow from the Level's Scene, collected once by admit()
// when the builder's objects are admitted. Admitted fixed movers explicitly
// refuse retirement and the scene never retires a structure or a player, so
// the borrows live for the match; no terrain add/remove API is exposed after
// admission. The Scene and the players are passed in to each call that reads
// them; this keeps no pointer to either.
class LevelStructures
{
public:
    // One step's prepared movement, validated before any of it is committed.
    struct Frame
    {
        std::vector<HorizontalPreparedStep> motions;
        std::vector<std::optional<HorizontalCarryStep>> carriers;
    };

    // Collects the movers (validated before any is bound), then the gates,
    // then the projectile targets, each in Scene order. Throws
    // std::invalid_argument when the movers cannot be admitted.
    void admit(const labrador::Scene& scene, std::span<Player* const> players,
        const PlayerTuning& player_tuning);

    bool has_horizontal_motion() const;
    Frame prepare(float dt, const labrador::Scene& scene,
        std::span<Player* const> players, const PlayerTuning& player_tuning) const;
    void commit(const Frame& prepared, std::span<Player* const> players) noexcept;
    void publish_zero(std::span<Player* const> players) noexcept;
    std::vector<HorizontalStructureState> horizontal_structure_states() const;
    // Every structure in Scene order, with its mover's identity if it has one.
    std::span<const HorizontalSupportRef> supports() const;

    // Once a step, before any of the step's movement or paint.
    void settle_gates(std::span<Player* const> players);
    // The stage's team gates in Scene order.
    std::span<const TeamGateStructurePaintable* const> team_gates() const;

    // Scene-owned geometry and players, which live for the whole match. Flying
    // paint sweeps this list so its cost does not grow with other projectiles.
    std::span<labrador::CollisionObject* const> projectile_targets() const;

private:
    void admit_horizontal(const labrador::Scene& scene, std::span<Player* const> players,
        const PlayerTuning& player_tuning);
    void validate_admission(const labrador::Scene& scene,
        std::span<Player* const> players) const;
    void validate_budget(const Frame* prepared, const labrador::Scene& scene,
        std::span<Player* const> players, const PlayerTuning& player_tuning) const;

    std::vector<TranslatingStructurePaintable*> horizontal_structures_;
    std::vector<HorizontalSupportRef> horizontal_supports_;
    std::vector<const TeamGateStructurePaintable*> team_gates_view_;
    std::vector<TeamGateStructurePaintable*> team_gates_;
    std::vector<labrador::CollisionObject*> projectile_targets_;
};
