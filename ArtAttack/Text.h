#ifndef TEXT_H
#define TEXT_H

//#include "VisualText.h"
#include "TextObject.h"

class Text : public TextObject
{
public:
	Text() = default;
	Text(const std::string& text,
		const std::string& font_name,
		const MattMath::Vector2F& position,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const MattMath::Colour& color = colour_consts::WHITE,
		float scale = 1.0f,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	//virtual void update() override;
	//virtual void draw(const MattMath::Camera& camera) override;
	//virtual void draw() override;
	//virtual bool is_visible_in_viewport(
	//	const MattMath::RectangleF& view) const override;

	void set_text(const std::string& text);
	void set_colour(const MattMath::Colour& colour) override;
	void set_scale(float scale);
	void set_position(const MattMath::Vector2F& position);
};
#endif // !TEXT_H
