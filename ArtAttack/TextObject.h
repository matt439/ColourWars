#ifndef TEXTOBJECT_H
#define TEXTOBJECT_H

#include "DrawObject.h"

class TextObject : public DrawObject
{
public:
	TextObject() = default;
	TextObject(const std::string& text,
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

	virtual void draw(const MattMath::Camera& camera) const;
	virtual void draw() const;

protected:	
	const std::string& get_text() const;
	const std::string& get_font_name() const;
	const MattMath::Vector2F& get_position() const;
	float get_scale() const;

	void set_text(const std::string& text);
	void set_font_name(const std::string& font_name);
	void set_position(const MattMath::Vector2F& position);
	void set_scale(float scale);
private:
	std::string _text = "";
	std::string _font_name = "";
	MattMath::Vector2F _position = MattMath::Vector2F::ZERO;
	float _scale = 1.0f;
};
#endif // !TEXTOBJECT_H
