#include "pch.h"
#include "RectangleAnimation.h"

using namespace DirectX;
using namespace MattMath;

void RectangleAnimation::draw()
{
	this->AnimationObject::draw(RectangleObject::get_rectangle());
}