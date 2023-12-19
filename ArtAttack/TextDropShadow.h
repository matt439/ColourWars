#ifndef TEXTDROPSHADOW_H
#define TEXTDROPSHADOW_H

#include "Text.h"

class TextDropShadow : public Text
{
public:
	TextDropShadow() = default;
	TextDropShadow(const std::string& text,
				const std::string& font_name,
				const MattMath::Vector2F& position,
				DirectX::SpriteBatch* sprite_batch,
				ResourceManager* resource_manager,
				const MattMath::Colour& color = colour_consts::WHITE,
				const MattMath::Colour& shadow_color = colour_consts::BLACK,
				const MattMath::Vector2F& shadow_offset = { 2.0f, 2.0f },
				float scale = 1.0f,
				float shadow_scale = 1.0f,
				float rotation = 0.0f,
				const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
				DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
				float layer_depth = 0.0f);

	void draw(const MattMath::Camera& camera);
	void draw();

private:
	MattMath::Vector2F _shadow_offset = { 2.0f, 2.0f };
	MattMath::Colour _shadow_color = colour_consts::BLACK;
	float _shadow_scale = 1.0f;
};
#endif // !TEXTDROPSHADOW_H
