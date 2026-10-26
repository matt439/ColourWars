#pragma once

#include "game/objects/player_team.h"
#include "engine/render/texture_object.h"
#include "game/objects/team_colour.h"
#include "engine/core/game_object.h"
#include "engine/render/animation_object.h"
#include "engine/render/colour.h"
#include "engine/render/render_resources.h"
#include "game/tuning.h"

#include <array>

class FixedSlabGeometry;

// PaintTuning's splash clips, resolved once against a RenderResources: the
// clip a tile is built with, and the one each ProjectileType restarts it on.
// Tuning is read before any sheet exists, so these live beside it, not in it.
struct PaintSplashClips
{
	labrador::RenderResources::SheetStrip initial;
	std::array<labrador::RenderResources::SheetStrip,
		static_cast<size_t>(ProjectileType::count)> by_type;

	static PaintSplashClips resolve(const PaintTuning& paint,
		const labrador::RenderResources& render_resources);
};

class PaintTileSplash final : public labrador::AnimationObject, public labrador::GameObject
{
public:
	PaintTileSplash() = delete;
	PaintTileSplash(
		const mattmath::RectangleF& rectangle,
		labrador::RenderResources::SheetStrip clip,
		labrador::RenderResources* render_resources,
		const labrador::Colour& color = labrador::Colour::white,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f);

	void set_rectangle_center(const mattmath::Vector2F& center);
	void reset_and_play();
	void reset_and_play(labrador::RenderResources::SheetStrip clip);
	bool active() const;
	void set_colour(const labrador::Colour& colour) override;

	void update(float dt) override;
	void draw(labrador::DrawList& draw_list) const override;

	// Draws with the given tint without storing it. See TextureObject::draw_with.
	void draw_with_colour(labrador::DrawList& draw_list,
		const labrador::Colour& colour) const;
	// Rigid local-frame submission; never changes this shared animation object.
	void draw_with_colour_rigid(labrador::DrawList& draw_list,
		const labrador::Colour& colour, const FixedSlabGeometry& geometry) const;

	mattmath::RectangleF bounds() const override;
private:
	mattmath::RectangleF rectangle_ = mattmath::RectangleF::ZERO;
	float age_ = 0.0f;
	float frame_time_ = 0.0f;
	int frame_count_ = 0;
	bool active_ = false;
	labrador::RenderResources::SheetStrip rigid_clip_;
	// Reads frame_time_, frame_count_ and the origin from the current clip.
	void read_clip(labrador::RenderResources::SheetStrip clip);
};


// Owned by StructurePaintable, which tests nearby tiles on projectile contact.
// Tiles are not registered separately with Scene.
class PaintTile final : public labrador::TextureObject, public labrador::GameObject
{
public:
	PaintTile() = delete;
	// Borrowed tuning and clips must outlive the tile; they supply the splash
	// size and the splash animations, resolved against render_resources.
	PaintTile(const mattmath::RectangleF& rectangle,
		const PaintTuning* paint,
		const PaintSplashClips* splash_clips,
		labrador::RenderResources* render_resources,
		const TeamColour& team_colours,
		const labrador::Colour& color = labrador::Colour::white,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f,
		bool along_x = true, bool leading = true);
	void draw(labrador::DrawList& draw_list) const override;
	// Structures draw all airborne splashes before their opaque surface pass,
	// preventing a neighbouring tile's splash from covering another team's tint.
	void draw_splash(labrador::DrawList& draw_list) const;
	void draw_surface(labrador::DrawList& draw_list) const;
	void draw_splash_rigid(labrador::DrawList& draw_list,
		const FixedSlabGeometry& geometry) const;
	void draw_surface_rigid(labrador::DrawList& draw_list,
		const FixedSlabGeometry& geometry) const;
	// A rigid structure may join same-team tiles into one opaque visual base.
	// Wet highlights stay per tile; scoring rectangles and credit never join.
	void draw_surface_base_rigid(labrador::DrawList& draw_list,
		const FixedSlabGeometry& geometry, const mattmath::RectangleF& local_band) const;
	void draw_surface_wet_rigid(labrador::DrawList& draw_list,
		const FixedSlabGeometry& geometry) const;
	void update(float dt) override;
	float area() const;
	PlayerTeam team() const;

	// Claims the tile for a team and plays the splash. Credit belongs to the
	// player who captured it from neutral/enemy paint; friendly repaint keeps
	// that owner. The caller has established that the projectile reached it.
	void paint(PlayerTeam team, int player_num,
		ProjectileType type = ProjectileType::spray);
	int player_num() const;

	mattmath::RectangleF bounds() const override;
	// Contact/score bounds remain exact; this includes temporary splash overhang
	// for the owning structure's Scene culling bounds.
	mattmath::RectangleF visual_bounds() const;
private:
	mattmath::RectangleF rectangle_ = mattmath::RectangleF::ZERO;
	PlayerTeam team_ = PlayerTeam::none;
	int player_num_ = -1;
	TeamColour team_colours_ = TeamColour();
	const PaintTuning* paint_ = nullptr;
	const PaintSplashClips* splash_clips_ = nullptr;
	float wet_age_ = 0.0f;
	bool along_x_ = true;
	bool leading_ = true;
	PaintTileSplash splash_;
	mattmath::RectangleF face_band(float offset, float depth) const;
	void draw_rectangle_rigid(labrador::DrawList& draw_list,
		const FixedSlabGeometry& geometry, const mattmath::RectangleF& local,
		const labrador::Colour& tint) const;
};
