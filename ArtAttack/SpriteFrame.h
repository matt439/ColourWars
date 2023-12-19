#ifndef SPRITEFRAME_H
#define SPRITEFRAME_H

//#include "SimpleMath.h"
#include "MattMath.h"
#include <string>

//struct sprite_frame_reference
//{
//	sprite_frame_reference(const std::string& sprite_sheet_name,
//		const std::string& sprite_frame_name) :
//		sprite_sheet_name(sprite_sheet_name),
//		sprite_frame_name(sprite_frame_name) {}
//	sprite_frame_reference() {}
//	std::string sprite_sheet_name = "";
//	std::string sprite_frame_name = "";
//};

class SpriteFrame
{
private:
	RECT _source_rectangle = { 0, 0, 0, 0 };

	MattMath::RectangleI _source_rectangle2 = MattMath::RectangleI::ZERO;
	//MattMath::Vector2F _size = MattMath::Vector2F::Zero;
	MattMath::Vector2F _origin = MattMath::Vector2F::ZERO;
	bool _rotated = false;

	void set_source_rectangle(
		const MattMath::RectangleI& source_rectangle);
	void set_source_rectangle(const MattMath::Vector2F& position,
		const MattMath::Vector2F& size);
public:
	SpriteFrame() {}
	SpriteFrame(const MattMath::RectangleI& source_rectangle,
		const MattMath::Vector2F& origin =
			MattMath::Vector2F::ZERO,
		bool rotated = false);
	SpriteFrame(const MattMath::Vector2F& position,
		const MattMath::Vector2F& size,
		const MattMath::Vector2F& origin =
			MattMath::Vector2F::ZERO,
		bool rotated = false);
	const RECT* get_source_rectangle() const;

	//const RECT* get_source_rectangle2() const;
};

#endif // !SPRITEFRAME_H
