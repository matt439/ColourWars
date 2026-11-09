#pragma once

#include "game/ai/match_bot_support.h"
#include "game/objects/paintable_object.h"
#include "game/objects/player_team.h"
#include "game/objects/sub_type.h"
#include "game/objects/weapon_type.h"
#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"
#include <optional>
#include <vector>

// One participant as a shipping bot may observe it: public state only, by
// value, read at the moment body() is called.
struct BotBody
{
	labrador::Vector2F center;
	labrador::Vector2F velocity;
	PlayerTeam team = PlayerTeam::none;
	// The state is alive; health is separate, since a living body at zero
	// health is not yet a target.
	bool alive = false;
	float health = 0.0f;
	// On ground, a drop-down platform or a ramp; on_platform is the platform alone.
	bool grounded = false;
	bool on_platform = false;
	bool facing_right = true;
	// The tool's ammunition, unscaled.
	float ammo = 0.0f;
	int handicap = 0;
	bool underfoot_enabled = false;
	PlayerTeam underfoot_team = PlayerTeam::none;
	WeaponType weapon = WeaponType::sprayer;
	// The tool can fire its full shot now / has too little ammunition for one.
	bool tool_ready = false;
	bool tool_empty = false;
	// From the body's centre to its tool's nozzle.
	float muzzle_reach = 0.0f;
	// The sub it carries, if any, and whether a Sub press now would throw it:
	// past its cooldown with the whole cost in the tank (ROAD#11).
	std::optional<SubType> sub;
	bool sub_ready = false;
};

// What the shipping bots may ask of a running match (HYG#40). Nothing here
// returns a game object or a mutable reference, so game/ai reaches the world
// only through this and value headers. LevelBotWorld implements it.
class BotWorld
{
public:
	virtual ~BotWorld() = default;

	virtual int participant_count() const = 0;
	// By roster participant, as Level::player indexes them.
	virtual BotBody body(int participant) const = 0;
	// Each team gate's settled permission in Scene order, as of the last
	// settlement; empty when the stage has no team gates.
	virtual std::vector<PlayerTeam> gate_permissions() const = 0;
	virtual std::vector<PaintSurfaceSample> paint_surfaces() const = 0;
	// Conservative traversal bounds. A permitted team's open gates are neither
	// obstacles nor standing supports; none retains all structural bounds.
	virtual std::vector<labrador::RectangleF> obstacles(PlayerTeam permitted) const = 0;
	virtual std::vector<MatchBotSupport> supports(PlayerTeam permitted) const = 0;
	virtual std::vector<MatchBotRamp> ramps() const = 0;
	// The team whose paint covers a flat support's top at this foot point (a
	// body's bottom centre), as paint underfoot reads it: none where the top is
	// unpainted or two supports disagree, nullopt where no support top lies.
	virtual std::optional<PlayerTeam> top_paint_team(labrador::Vector2F foot) const = 0;
	// Projectile cover includes every gate, even one the bot can walk through.
	virtual bool line_of_sight(labrador::Vector2F from, labrador::Vector2F to) const = 0;
	// Candidate route visibility only; the policy separately sweeps its body.
	virtual bool traversal_line_of_sight(labrador::Vector2F from, labrador::Vector2F to,
		PlayerTeam team) const = 0;

protected:
	BotWorld() = default;
	BotWorld(const BotWorld&) = default;
	BotWorld& operator=(const BotWorld&) = default;
};
