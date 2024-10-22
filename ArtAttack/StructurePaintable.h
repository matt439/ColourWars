#ifndef STRUCTUREPAINTABLE_H
#define STRUCTUREPAINTABLE_H

#include "IPaintableGameObject.h"
#include "Structure.h"
#include "PaintTile.h"
#include "TeamColour.h"
#include "SoundBank.h"

namespace structure_paintable_consts
{
	const std::string SOUND_BANK_NAME = "sound_bank_1";
	const std::string SOUND_NAME = "WaterSplashing";
	constexpr float SOUND_VOLUME = 0.1f;
}

struct paintable_faces
{
	bool left = false;
	bool top = false;
	bool right = false;
	bool bottom = false;
};

class StructurePaintable : public Structure, public IPaintableGameObject
{
public:
	StructurePaintable() = default;
	StructurePaintable(const std::string& sheet_name,
		const std::string& frame_name,
		const MattMath::RectangleF& rectangle,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		collision_object_type collision_type,
		const team_colour& team_colours,
		const paintable_faces& faces,
		const float* dt,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	virtual void update() override;
	virtual void draw(const MattMath::Camera& camera) override;
	virtual void draw() override;
	virtual void on_collision(const ICollisionGameObject* other) override;

	paint_total get_paint_total() const override;

private:
	std::vector<PaintTile> _paint_tiles = std::vector<PaintTile>();
	team_colour _team_colours = team_colour();
	paintable_faces _faces = paintable_faces();
	const float* _dt = nullptr;
	std::vector<PaintTile> generate_paint_tiles() const;
	SoundBank* _sound_bank = nullptr;


};
#endif // !STRUCTUREPAINTABLE_H
