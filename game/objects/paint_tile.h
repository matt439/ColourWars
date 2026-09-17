#pragma once

#include "game/objects/player_team.h"
#include "engine/render/texture_object.h"
#include "game/objects/team_colour.h"
#include "engine/core/game_object.h"
#include "engine/render/animation_object.h"
#include "engine/render/colour.h"
#include "game/tuning.h"


class PaintTileSplash final : public labrador::AnimationObject, public labrador::GameObject
{
public:
	PaintTileSplash() = default;
	PaintTileSplash(
		const mattmath::RectangleF& rectangle,
		const std::string& sheet_name,
		const std::string& animation_strip_name,
		labrador::RenderResources* render_resources,
		const labrador::Colour& color = labrador::Colour::white,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f);

	void set_rectangle_center(const mattmath::Vector2F& center);
	void reset_and_play();
	void set_colour(const labrador::Colour& colour) override;

	void update(float dt) override;
	void draw(labrador::DrawList& draw_list) const override;

	// Draws with the given tint without storing it. See TextureObject::draw_with.
	void draw_with_colour(labrador::DrawList& draw_list,
		const labrador::Colour& colour) const;

	mattmath::RectangleF bounds() const override;
private:
	mattmath::RectangleF rectangle_ = mattmath::RectangleF::ZERO;
};


// Not a CollisionObject, and never was one usefully.
//
// A paint tile has never appeared in the level's object list: a paintable
// structure owns its own tiles and tests them itself, four thousand at a time,
// only for the projectile that just hit the structure. Implementing the
// collision interface bought it a type tag nothing read, a for_deletion()
// nothing called, and a copy of the AABB-then-narrow block that six other
// classes also carried - all so a structure could ask a tile a question it
// could answer with a rectangle.
class PaintTile final : public labrador::TextureObject, public labrador::GameObject
{
public:
	PaintTile() = default;
	// The tuning is borrowed and outlives every level: main() owns it. A tile
	// reads its splash's sheet, animation and size off it, which is what the
	// three paint_tile_consts entries this replaces were.
	PaintTile(const mattmath::RectangleF& rectangle,
		const PaintTuning* paint,
		labrador::RenderResources* render_resources,
		const TeamColour& team_colours,
		const labrador::Colour& color = labrador::Colour::white,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f);
	void draw(labrador::DrawList& draw_list) const override;
	void update(float dt) override;
	float area() const;
	PlayerTeam team() const;

	// Claims the tile for a team and plays the splash. Credit belongs to the
	// player who captured it from neutral/enemy paint; friendly repaint keeps
	// that owner. The caller has established that the projectile reached it.
	void paint(PlayerTeam team, int player_num);
	int player_num() const;

	mattmath::RectangleF bounds() const override;
private:
	mattmath::RectangleF rectangle_ = mattmath::RectangleF::ZERO;
	PlayerTeam team_ = PlayerTeam::none;
	int player_num_ = -1;
	TeamColour team_colours_ = TeamColour();
	PaintTileSplash splash_;
};
