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
#include <optional>
#include <vector>


// Faces on which the structure generates paint tiles.
struct PaintableFaces
{
	bool left = false;
	bool top = false;
	bool right = false;
	bool bottom = false;
};

// Read-only geometry for the short cosmetic phrase, including its waiting phase.
// The two drawn rectangles are clipped to owned_band, never scoring geometry.
struct PaintRunoffVisual
{
	PlayerTeam team = PlayerTeam::none;
	float age = 0.0f;
	float source_y = 0.0f;
	float head_y = 0.0f;
	mattmath::RectangleF owned_band = mattmath::RectangleF::ZERO;
	mattmath::RectangleF trail = mattmath::RectangleF::ZERO;
	mattmath::RectangleF bead = mattmath::RectangleF::ZERO;
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
		float layer_depth = 0.0f);

	void update(float dt) override;
	void draw(labrador::DrawList& draw_list) const override;
	// Surface splashes affect rendering extent, never layout/contact geometry.
	mattmath::RectangleF cull_bounds(float units_per_pixel) const override;
	void on_contact(const labrador::CollisionObject& other,
		const mattmath::Vector2F& normal, float penetration) override;

	PaintTotal paint_total() const override;
	// Ownership directly under a confirmed top-support contact. Tile intervals
	// are half open, so a seam has one answer; side/bottom paint never counts.
	PlayerTeam top_paint_team(float x) const;
	std::optional<labrador::Colour> top_paint_colour(float x) const;
	std::vector<PaintRunoffVisual> runoff_visuals() const;

private:
	std::vector<PaintTile> paint_tiles_ = std::vector<PaintTile>();
	TeamColour team_colours_ = TeamColour();
	PaintableFaces faces_ = PaintableFaces();
	// Borrowed, and outlives the level: main() owns it.
	const PaintTuning* paint_ = nullptr;
	const PlaySettings* preferences_ = nullptr;
	labrador::RenderResources::SpriteSheetHandle runoff_sheet_;
	labrador::SpriteSheet::frame_handle runoff_frame_;

	struct Runoff
	{
		PlayerTeam team = PlayerTeam::none;
		int source_tile = 0;
		float source_y = 0.0f;
		float age = 0.0f;
		float limit_y = 0.0f;
	};

	// One contiguous strip of equal-size tiles. origin is the leading edge of
	// tile begin; pitch is tile length along the strip. along_x selects x for
	// top/bottom faces or y for left/right faces.
	struct PaintStrip
	{
		int begin = 0;
		int count = 0;
		float origin = 0.0f;
		float pitch = 1.0f;
		float end = 0.0f;
		bool along_x = true;
		bool top = false;
		bool leading = true;
		Runoff runoff;
	};

	// A covered face can split into several exposed strips.
	std::vector<PaintStrip> paint_strips_ = std::vector<PaintStrip>();
	std::optional<PaintRunoffVisual> runoff_visual(const PaintStrip& strip) const;
	void draw_runoff(labrador::DrawList& draw_list, const PaintStrip& strip) const;

	// Build tiles and record the strips used by on_contact for candidate lookup.
	std::vector<PaintTile> generate_paint_tiles(
		std::span<const mattmath::RectangleF> paint_obstacles);


};
