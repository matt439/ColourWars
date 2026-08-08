#pragma once

#include "game/objects/paintable_object.h"
#include "game/objects/structure.h"
#include "game/objects/paint_tile.h"
#include "engine/audio/audio_resources.h"
#include "game/objects/team_colour.h"
#include "engine/audio/sound_bank.h"

namespace structure_paintable_consts
{
	const std::string SOUND_BANK_NAME = "sound_bank_1";
	const std::string SOUND_NAME = "WaterSplashing";
	constexpr float SOUND_VOLUME = 0.1f;
}

// Which faces of a paintable structure generate paint tiles.
//
// These used to be encoded as a std::vector<Segment> of the matching edges and
// decoded back by matching positional indices against RectangleF::edges().
// That ordering is {top, bottom, left, right} but the decoder assumed
// {top, right, bottom, left}, so three of the four faces painted on the wrong
// side. Passing the booleans through directly removes the round trip.
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
	StructurePaintable() = default;
	StructurePaintable(const std::string& sheet_name,
		const std::string& frame_name,
		const mattmath::RectangleF& sprite_rectangle,
		const mattmath::Shape* collision_shape,
		artattack::RenderResources* render_resources,
		const artattack::AudioResources* audio_resources,
		CollisionObjectType collision_type,
		const TeamColour& team_colours,
		const PaintableFaces& faces,
		const mattmath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void update(float dt) override;
	void draw(DirectX::SpriteBatch* sprite_batch,
		const mattmath::Camera& camera) const override;
	void on_collision(const CollisionObject* other) override;

	PaintTotal paint_total() const override;

private:
	std::vector<PaintTile> paint_tiles_ = std::vector<PaintTile>();
	TeamColour team_colours_ = TeamColour();
	// By value. This was a reference bound to a vector local to
	// LevelObjectBuilder::build_collision_object, so it dangled for the whole
	// life of every paintable structure in the level.
	PaintableFaces faces_ = PaintableFaces();
	std::vector<PaintTile> generate_paint_tiles() const;
	artattack::SoundBank* sound_bank_ = nullptr;

	// Resolved once at construction. This plays from update(), on any tile
	// that was just painted, so it was a map descent per splash per frame.
	artattack::SoundBank::WaveHandle paint_sound_;


};
