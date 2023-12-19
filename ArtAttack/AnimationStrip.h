#ifndef ANIMATIONSTRIP_H
#define ANIMATIONSTRIP_H

//#include "SimpleMath.h"

//struct animation_strip_reference
//{
//	animation_strip_reference(const std::string& sprite_sheet_name,
//		const std::string& animation_strip_name) :
//		sprite_sheet_name(sprite_sheet_name),
//		animation_strip_name(animation_strip_name) {}
//	animation_strip_reference() {}
//	std::string sprite_sheet_name = "";
//	std::string animation_strip_name = "";
//};

#include "MattMath.h"

class AnimationStrip
{
private:
	//DirectX::SimpleMath::Vector2 _position =
	//	DirectX::SimpleMath::Vector2::Zero;
	//DirectX::SimpleMath::Vector2 _frame_size =
	//	DirectX::SimpleMath::Vector2::Zero;

	std::vector<RECT> _frame_rects;
	MattMath::RectangleI _first_frame = { 0, 0, 0, 0 };
	int _frame_count = 0;
	float _frame_time = 0.0f;
	bool _looping = false;

	//void set_first_frame_rect(const DirectX::SimpleMath::Rectangle& rectangle);

	std::vector<RECT> calculate_all_frame_rects() const;
	RECT calculate_frame(int frame_index) const;
public:
	AnimationStrip() {}
	AnimationStrip(const MattMath::RectangleI& first_frame,
		int frame_count, float frame_time, bool looping);

	const RECT* get_frame_rect(int frame_index) const;

	//const DirectX::SimpleMath::Rectangle& get_first_frame() const;

	//const DirectX::SimpleMath::Vector2& get_position() const;
	//const DirectX::SimpleMath::Vector2& get_frame_size() const;
	int get_frame_count() const;
	float get_frame_time() const;
	bool get_looping() const;


};
#endif // !ANIMATIONSTRIP_H
