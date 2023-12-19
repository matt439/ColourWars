#include "pch.h"
#include "AnimationStrip.h"

using namespace DirectX;
//using namespace DirectX::SimpleMath;

using namespace MattMath;

//const Rectangle& AnimationStrip::get_first_frame() const
//{
//	return this->_first_frame;
//}

AnimationStrip::AnimationStrip(const RectangleI& first_frame,
	int frame_count, float frame_time, bool looping) :
	_first_frame(first_frame),
	_frame_count(frame_count),
	_frame_time(frame_time),
	_looping(looping)
{
	//this->set_first_frame_rect(first_frame);
	this->_frame_rects = this->calculate_all_frame_rects();
}

RECT AnimationStrip::calculate_frame(int frame_index) const
{
	if (frame_index < 0 || frame_index >= this->_frame_count)
	{
		throw std::exception("frame_index out of range");
	}
	RectangleI frame = this->_first_frame;
	//frame.x += static_cast<long>(frame_index) * frame.width;
	//frame.operator RECT();
	frame.offset(frame_index * frame.width, 0);
	return frame.get_win_rect();
}
//const Vector2& AnimationStrip::get_position() const
//{
//	return this->_position;
//}
//const Vector2& AnimationStrip::get_frame_size() const
//{
//	return this->_frame_size;
//}

const RECT* AnimationStrip::get_frame_rect(int frame_index) const
{
	if (frame_index < 0 || frame_index >= this->_frame_count)
	{
		throw std::exception("frame_index out of range");
	}
	return &this->_frame_rects[frame_index];
	//return this->_test_rect.get();
}

int AnimationStrip::get_frame_count() const
{
	return this->_frame_count;
}
float AnimationStrip::get_frame_time() const
{
	return this->_frame_time;
}
bool AnimationStrip::get_looping() const
{
	return this->_looping;
}

//void AnimationStrip::set_first_frame_rect(const Rectangle& rectangle)
//{
//	Rectangle source_rectan = rectangle;
//	source_rectan.operator RECT();
//	this->_first_frame = source_rectan;
//}
std::vector<RECT> AnimationStrip::calculate_all_frame_rects() const
{
	std::vector<RECT> rects;
	for (int i = 0; i < this->_frame_count; i++)
	{
		rects.push_back(this->calculate_frame(i));
	}
	return rects;
}