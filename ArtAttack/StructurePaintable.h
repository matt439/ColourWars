#ifndef STRUCTUREPAINTABLE_H
#define STRUCTUREPAINTABLE_H

#include "IPaintableGameObject.h"
#include "Structure.h"
#include "PaintTile.h"
#include "engine/audio/audio_resources.h"
#include "TeamColour.h"
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
// decoded back by matching positional indices against RectangleF::get_edges().
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

class StructurePaintable final : public Structure, public IPaintableGameObject
{
public:
	StructurePaintable() = default;
	StructurePaintable(const std::string& sheet_name,
		const std::string& frame_name,
		const MattMath::RectangleF& sprite_rectangle,
		const MattMath::Shape* collision_shape,
		RenderResources* render_resources,
		const AudioResources* audio_resources,
		collision_object_type collision_type,
		const TeamColour& team_colours,
		const PaintableFaces& faces,
		const float* dt,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void update() override;
	void draw(DirectX::SpriteBatch* sprite_batch, const MattMath::Camera& camera) override;
	void draw(DirectX::SpriteBatch* sprite_batch) override;
	void on_collision(const ICollisionGameObject* other) override;

	PaintTotal get_paint_total() const override;

private:
	std::vector<PaintTile> _paint_tiles = std::vector<PaintTile>();
	TeamColour _team_colours = TeamColour();
	// By value. This was a reference bound to a vector local to
	// LevelObjectBuilder::build_collision_object, so it dangled for the whole
	// life of every paintable structure in the level.
	PaintableFaces _faces = PaintableFaces();
	const float* _dt = nullptr;
	std::vector<PaintTile> generate_paint_tiles() const;
	SoundBank* _sound_bank = nullptr;

	// Resolved once at construction. This plays from update(), on any tile
	// that was just painted, so it was a map descent per splash per frame.
	SoundBank::WaveHandle _paint_sound;


};
#endif // !STRUCTUREPAINTABLE_H
