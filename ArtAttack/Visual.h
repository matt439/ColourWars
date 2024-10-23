#ifndef VISUAL_H
#define VISUAL_H

#include "IGameObject.h"
#include "TextureObject.h"
#include "MattMath.h"

class Visual final : public IGameObject, public TextureObject
{
public:
	Visual() = default;
	Visual(const std::string& sheet_name,
		const std::string& frame_name,
		const MattMath::RectangleF& rectangle,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void update() override;
	void draw(const MattMath::Camera& camera) override;
	void draw() override;

	bool is_visible_in_viewport(const MattMath::RectangleF& view) const override;

protected:
	MattMath::RectangleF _rectangle = MattMath::RectangleF::ZERO;
};
#endif // !VISUAL_H
