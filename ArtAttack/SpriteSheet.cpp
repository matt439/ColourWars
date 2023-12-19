#include "pch.h"
#include "SpriteSheet.h"
#include "rapidjson/filereadstream.h"

using namespace DirectX;
//using namespace DirectX::SimpleMath;
using namespace MattMath;
using namespace rapidjson;

//void SpriteSheet::add_frame(const SpriteFrame& frame)
//{
//	this->_sprite_frames.push_back(std::make_unique<SpriteFrame>(frame));
//}
//void SpriteSheet::add_frame(const Rectangle& source_rectangle)
//{
//	//this->_sprite_frames.push_back(
//	//	std::make_unique<SpriteFrame>(source_rectangle));
//	this->_sprite_frames.push_back(SpriteFrame(source_rectangle));
//}
//void SpriteSheet::add_frame(const Vector2& position, const Vector2& size)
//{
//	//this->_sprite_frames.push_back(
//	//	std::make_unique<SpriteFrame>(position, size));
//	this->_sprite_frames.push_back(SpriteFrame(position, size));
//}
//SpriteFrame& SpriteSheet::get_frame(int index) const
//{
//	return this->_sprite_frames[index];
//}
//int SpriteSheet::get_frame_count() const
//{
//	return static_cast<int>(this->_sprite_frames.size());
//}

const AnimationStrip* SpriteSheet::get_animation_strip(const std::string& name) const
{
	//AnimationStrip strip = this->_animation_strips.at(name);
	//return strip;
	return this->_animation_strips.at(name).get();
}

std::map<std::string, SpriteFrame>
	SpriteSheet::decode_sprite_frames_json(const rapidjson::Value& json)
{
	std::map<std::string, SpriteFrame> sprite_frames;
	for (auto& frame : json.GetArray())
	{
		std::string name = frame["name"].GetString();
		Vector2F origin = Vector2F::ZERO;
		bool rotated = false;
		RectangleI source_rectangle(
			frame["position"]["x"].GetInt(),
			frame["position"]["y"].GetInt(),
			frame["size"]["w"].GetInt(),
			frame["size"]["h"].GetInt());
		if (frame.HasMember("origin"))
		{
			origin.x = frame["origin"]["x"].GetFloat();
			origin.y = frame["origin"]["y"].GetFloat();
		}
		if (frame.HasMember("rotated"))
		{
			rotated = frame["rotated"].GetBool();
		}
		sprite_frames[name] = SpriteFrame(source_rectangle, origin,
			rotated);
	}
	return sprite_frames;
}

std::map<std::string, std::unique_ptr<AnimationStrip>>
	SpriteSheet::decode_animation_strips_json(const rapidjson::Value& json)
{
	std::map<std::string, std::unique_ptr<AnimationStrip>> animation_strips;
	for (auto& strip : json.GetArray())
	{
		std::string name = strip["name"].GetString();
		RectangleI first_frame = RectangleI(
			strip["first_frame"]["x"].GetInt(),
			strip["first_frame"]["y"].GetInt(),
			strip["first_frame"]["w"].GetInt(),
			strip["first_frame"]["h"].GetInt());
		int frame_count = strip["frame_count"].GetInt();
		float frame_time = strip["frame_time"].GetFloat();
		bool looping = strip["looping"].GetBool();
		animation_strips[name] = std::make_unique<AnimationStrip>(
			first_frame, frame_count, frame_time, looping);
	}
	return animation_strips;
}

void SpriteSheet::load_from_json(const char* json_path)
{
	FILE* fp = fopen(json_path, "rb");

	std::unique_ptr<char> readBuffer = std::make_unique<char>();
	FileReadStream is(fp, readBuffer.get(), sizeof(readBuffer));

	Document d;
	d.ParseStream(is);

	fclose(fp);
	readBuffer.release();

	Value& sprite_frames = d["sprite_frames"];
	Value& animation_strips = d["animation_strips"];

	this->_sprite_frames = this->decode_sprite_frames_json(sprite_frames);
	this->_animation_strips =
		this->decode_animation_strips_json(animation_strips);

}

void SpriteSheet::draw(SpriteBatch* sprite_batch,
	const std::string& frame_name,
	const Vector2F& position,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	float scale,
	SpriteEffects effects,
	float layer_depth)
{
	sprite_batch->Draw(
		this->_texture,
		position.get_xm_vector(),
		this->_sprite_frames[frame_name].get_source_rectangle(),
		color.get_xm_vector(),
		rotation,
		origin.get_xm_vector(),
		scale,
		effects,
		layer_depth);
}

void SpriteSheet::draw(SpriteBatch* sprite_batch,
	const std::string& frame_name,
	const RectangleI& destination_rectangle,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth)
{
	sprite_batch->Draw(
		this->_texture,
		destination_rectangle.get_win_rect(),
		this->_sprite_frames[frame_name].get_source_rectangle(),
		color.get_xm_vector(),
		rotation,
		origin.get_xm_vector(),
		effects,
		layer_depth);
}

void SpriteSheet::draw(SpriteBatch* sprite_batch,
	const RECT* source_rect,
	const Vector2F& position,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	float scale,
	SpriteEffects effects,
	float layer_depth)
{
	sprite_batch->Draw(
		this->_texture,
		position.get_xm_vector(),
		source_rect,
		color.get_xm_vector(),
		rotation,
		origin.get_xm_vector(),
		scale,
		effects,
		layer_depth);
}

void SpriteSheet::draw(SpriteBatch* sprite_batch,
	const RECT* source_rect,
	const RectangleI& destination_rectangle,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth)
{
	sprite_batch->Draw(
		this->_texture,
		destination_rectangle.get_win_rect(),
		source_rect,
		color.get_xm_vector(),
		rotation,
		origin.get_xm_vector(),
		effects,
		layer_depth);
}

//void SpriteSheet::draw(SpriteBatch* sprite_batch,
//	const std::string& animation_strip_name,
//	int frame_index,
//	const Vector2& position,
//	const Color& color,
//	float rotation,
//	const Vector2& origin,
//	float scale,
//	SpriteEffects effects,
//	float layer_depth)
//{
//	sprite_batch->Draw(
//		this->_texture,
//		position,
//		this->_animation_strips[animation_strip_name].
//			get_frame_rect(frame_index),
//		color,
//		rotation,
//		origin,
//		scale,
//		effects,
//		layer_depth);
//}
//
//void SpriteSheet::draw(SpriteBatch* sprite_batch,
//	const std::string& animation_strip_name,
//	int frame_index,
//	const Rectangle& destination_rectangle,
//	const Color& color,
//	float rotation,
//	const Vector2& origin,
//	SpriteEffects effects,
//	float layer_depth)
//{
//	Rectangle des_rectan = destination_rectangle;
//	RECT dest_rect = des_rectan.operator RECT();
//
//	sprite_batch->Draw(
//		this->_texture,
//		dest_rect,
//		this->_animation_strips[animation_strip_name].
//			get_frame_rect(frame_index),
//		color,
//		rotation,
//		origin,
//		effects,
//		layer_depth);
//}