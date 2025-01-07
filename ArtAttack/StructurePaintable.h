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
		ResourceManager* resource_manager,
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
	PaintableFaces _faces = PaintableFaces();
	const float* _dt = nullptr;
	std::vector<PaintTile> generate_paint_tiles() const;
	SoundBank* _sound_bank = nullptr;


};
#endif // !STRUCTUREPAINTABLE_H
