#ifndef SPRITESHEET_H
#define SPRITESHEET_H

#include "SpriteFrame.h"
#include "Colour.h"
#include "rapidjson/document.h"
#include <cstdio>
#include <map>
#include "AnimationStrip.h"

class SpriteSheet
{	
public:
	SpriteSheet(ID3D11ShaderResourceView* texture) :
		_texture(texture) {}

	const AnimationStrip* get_animation_strip(const std::string& name) const;
	
	void load_from_json(const char* json_path);

	void draw(DirectX::SpriteBatch* sprite_batch,
		const std::string& frame_name,
		const MattMath::Vector2F& position,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin =
			MattMath::Vector2F::ZERO,
		float scale = 1.0f,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void draw(DirectX::SpriteBatch* sprite_batch,
		const std::string& frame_name,
		const MattMath::RectangleI& destination_rectangle,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin =
			MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void draw(DirectX::SpriteBatch* sprite_batch,
		const RECT* source_rect,
		const MattMath::Vector2F& position,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin =
		MattMath::Vector2F::ZERO,
		float scale = 1.0f,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void draw(DirectX::SpriteBatch* sprite_batch,
		const RECT* source_rect,
		const MattMath::RectangleI& destination_rectangle,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin =
		MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);
private:
	std::map<std::string, SpriteFrame> _sprite_frames;
	std::map<std::string, std::unique_ptr<AnimationStrip>> _animation_strips;
	ID3D11ShaderResourceView* _texture = nullptr;

	std::map<std::string, SpriteFrame>
		decode_sprite_frames_json(const rapidjson::Value& json);
	std::map<std::string, std::unique_ptr<AnimationStrip>>
		decode_animation_strips_json(const rapidjson::Value& json);
};
#endif // !SPRITESHEET_H
