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


// Owned by StructurePaintable, which tests nearby tiles on projectile contact.
// Tiles are not registered separately with Scene.
class PaintTile final : public labrador::TextureObject, public labrador::GameObject
{
public:
	PaintTile() = default;
	// Borrowed tuning must outlive the tile; it supplies the splash assets and size.
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
