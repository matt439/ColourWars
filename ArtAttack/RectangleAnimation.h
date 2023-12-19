#ifndef RECTANGLEANIMATION_H
#define RECTANGLEANIMATION_H

#include "AnimationObject.h"
#include "AnimatedSprite.h"
#include "RectangleObject.h"

class RectangleAnimation : public AnimationObject, public RectangleObject
{
private:
	//MattMath::RectangleF _rectangle = MattMath::RectangleF::ZERO;
	//const float* _dt = nullptr;
	//std::unique_ptr<AnimatedSprite> _animated_sprite = nullptr;
protected:
	//const MattMath::RectangleF& get_rectangle() const;
	//const MattMath::RectangleI get_rectangle_i() const;

	//void set_rectangle(const MattMath::RectangleF& rectangle);
public:
	RectangleAnimation() = default;
	RectangleAnimation(const float* dt,
		const std::string& sheet_name,
		const std::string& animation_strip_name,
		const MattMath::RectangleF& rectangle,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const MattMath::Colour& color = colour_consts::WHITE,
		//float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f) :
	AnimationObject(dt, sheet_name, animation_strip_name, sprite_batch,
		resource_manager, color, 0.0f, origin, effects, layer_depth) {}

	virtual void draw();
};
#endif // !RECTANGLEANIMATION_H
