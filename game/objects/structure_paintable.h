#pragma once

#include "game/play_settings.h"

#include "game/objects/paintable_object.h"
#include "game/objects/structure.h"
#include "game/objects/paint_tile.h"
#include "engine/audio/audio_resources.h"
#include "game/objects/team_colour.h"
#include "engine/audio/sound_bank.h"
#include "engine/render/colour.h"
#include "game/tuning.h"
#include <span>


// Faces on which the structure generates paint tiles.
struct PaintableFaces
{
	bool left = false;
	bool top = false;
	bool right = false;
	bool bottom = false;
};

class StructurePaintable final : public Structure, public PaintableObject
{
public:
	StructurePaintable() = delete;
	// Clip authored faces against flat terrain while constructing the tiles.
	// The obstacle span is consumed immediately and is not retained.
	StructurePaintable(const std::string& sheet_name,
		const std::string& frame_name,
		const mattmath::RectangleF& sprite_rectangle,
		std::unique_ptr<mattmath::Shape> collision_shape,
		labrador::RenderResources* render_resources,
		const labrador::AudioResources* audio_resources,
		const PlaySettings* preferences,
		const PaintTuning* paint,
		CollisionObjectType collision_type,
		const TeamColour& team_colours,
		const PaintableFaces& faces,
		std::span<const mattmath::RectangleF> paint_obstacles = {},
		const labrador::Colour& color = labrador::Colour::white,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f,
		float tile_world_units_per_texel = 0.0f);

	void update(float dt) override;
	void draw(labrador::DrawList& draw_list) const override;
	void on_contact(const labrador::CollisionObject& other,
		const mattmath::Vector2F& normal, float penetration) override;

	PaintTotal paint_total() const override;

private:
	std::vector<PaintTile> paint_tiles_ = std::vector<PaintTile>();
	TeamColour team_colours_ = TeamColour();
	PaintableFaces faces_ = PaintableFaces();
	// Borrowed, and outlives the level: main() owns it.
	const PaintTuning* paint_ = nullptr;

	// One contiguous strip of equal-size tiles. origin is the leading edge of
	// tile begin; pitch is tile length along the strip. along_x selects x for
	// top/bottom faces or y for left/right faces.
	struct PaintStrip
	{
		int begin = 0;
		int count = 0;
		float origin = 0.0f;
		float pitch = 1.0f;
		bool along_x = true;
	};

	// A covered face can split into several exposed strips.
	std::vector<PaintStrip> paint_strips_ = std::vector<PaintStrip>();

	// Build tiles and record the strips used by on_contact for candidate lookup.
	std::vector<PaintTile> generate_paint_tiles(
		std::span<const mattmath::RectangleF> paint_obstacles);


};
