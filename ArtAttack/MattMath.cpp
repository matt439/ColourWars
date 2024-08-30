#include "pch.h"
#include "MattMath.h"

#include "EricsonMath.h"

using namespace DirectX;
using namespace MattMath;

#pragma region Misc

float MattMath::min(float a, float b)
{
	if (a < b)
	{
		return a;
	}
	else
	{
		return b;
	}
}
float MattMath::max(float a, float b)
{
	if (a > b)
	{
		return a;
	}
	else
	{
		return b;
	}
}
float MattMath::clamp(float value, float min, float max)
{
	if (value < min)
	{
		return min;
	}
	else if (value > max)
	{
		return max;
	}
	else
	{
		return value;
	}
}
void MattMath::clamp_ref(float& value, float min, float max)
{
	if (value < min)
	{
		value = min;
	}
	else if (value > max)
	{
		value = max;
	}
}
int MattMath::clamp(int value, int min, int max)
{
	if (value < min)
	{
		return min;
	}
	else if (value > max)
	{
		return max;
	}
	else
	{
		return value;
	}
}
void MattMath::clamp_ref(int& value, int min, int max)
{
	if (value < min)
	{
		value = min;
	}
	else if (value > max)
	{
		value = max;
	}
}
int MattMath::sign(const Vector2F& p1,
	const Vector2F& p2, const Vector2F& p3)
{
	return (p1.x - p3.x) * (p2.y - p3.y) -
		(p2.x - p3.x) * (p1.y - p3.y);
}

bool MattMath::are_equal(float a, float b, float epsilon)
{
	return fabs(a - b) < epsilon;
}
float MattMath::to_radians(float degrees)
{
	return degrees * (PI / 180.0f);
}
float MattMath::to_degrees(float radians)
{
	return radians * (180.0f / PI);
}

float MattMath::lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

#pragma endregion Misc

#pragma region Shape

bool Shape::intersects(const Shape* other) const
{
	shape_type other_shape_type = other->get_shape_type();
	switch (other_shape_type)
	{
	case shape_type::CIRCLE:
		return this->intersects(static_cast<const Circle*>(other));
	case shape_type::RECTANGLE:
		return this->intersects(static_cast<const RectangleF*>(other));
	case shape_type::TRIANGLE:
		return this->intersects(static_cast<const Triangle*>(other));
	default:
		return false;
	}
}
bool Shape::intersects(const Shape& other) const
{
	shape_type other_shape_type = other.get_shape_type();
	switch (other_shape_type)
	{
	case shape_type::CIRCLE:
		return this->intersects(static_cast<const Circle&>(other));
	case shape_type::RECTANGLE:
		return this->intersects(static_cast<const RectangleF&>(other));
	case shape_type::TRIANGLE:
		return this->intersects(static_cast<const Triangle&>(other));
	default:
		return false;
	}
}
bool Shape::intersects(const Circle& other) const
{
	shape_type this_shape_type = this->get_shape_type();
	switch (this_shape_type)
	{
	case shape_type::CIRCLE:
		return static_cast<const Circle&>(*this).intersects(other);
	case shape_type::RECTANGLE:
		return static_cast<const RectangleF&>(*this).intersects(other);
	case shape_type::TRIANGLE:
		return static_cast<const Triangle&>(*this).intersects(other);
	default:
		return false;
	}
}
bool Shape::intersects(const Triangle& other) const
{
	shape_type this_shape_type = this->get_shape_type();
	switch (this_shape_type)
	{
	case shape_type::CIRCLE:
		return static_cast<const Circle&>(*this).intersects(other);
	case shape_type::RECTANGLE:
		return static_cast<const RectangleF&>(*this).intersects(other);
	case shape_type::TRIANGLE:
		return static_cast<const Triangle&>(*this).intersects(other);
	default:
		return false;
	}
}
bool Shape::intersects(const RectangleF& other) const
{
	shape_type this_shape_type = this->get_shape_type();
	switch (this_shape_type)
	{
	case shape_type::CIRCLE:
		return static_cast<const Circle&>(*this).intersects(other);
	case shape_type::RECTANGLE:
		return static_cast<const RectangleF&>(*this).intersects(other);
	case shape_type::TRIANGLE:
		return static_cast<const Triangle&>(*this).intersects(other);
	default:
		return false;
	}
}
bool Shape::AABB_intersects(const Shape* other) const
{
	return this->get_bounding_box().intersects(other->get_bounding_box());
}

#pragma endregion Shape

#pragma region RectangleF

RectangleF::RectangleF(float x, float y, float width, float height) :
	x(x), y(y), width(width), height(height)
{
}
RectangleF::RectangleF(const Vector2F& position,
	const Vector2F& size)
{
	this->x = position.x;
	this->y = position.y;
	this->width = size.x;
	this->height = size.y;
}
RectangleF::RectangleF(const SimpleMath::Vector2& position, const SimpleMath::Vector2& size)
{
	this->x = position.x;
	this->y = position.y;
	this->width = size.x;
	this->height = size.y;
}
RectangleF::RectangleF(const SimpleMath::Rectangle& rectangle)
{
	this->x = static_cast<float>(rectangle.x);
	this->y = static_cast<float>(rectangle.y);
	this->width = static_cast<float>(rectangle.width);
	this->height = static_cast<float>(rectangle.height);
}
RectangleF::RectangleF(const RECT& rectangle)
{
	this->x = static_cast<float>(rectangle.left);
	this->y = static_cast<float>(rectangle.top);
	this->width = static_cast<float>(rectangle.right - rectangle.left);
	this->height = static_cast<float>(rectangle.bottom - rectangle.top);
}
const RectangleF& RectangleF::get_bounding_box() const
{
	return *this;
}
shape_type RectangleF::get_shape_type() const
{
	return shape_type::RECTANGLE;
}
float RectangleF::get_x() const
{
	return this->x;
}
float RectangleF::get_y() const
{
	return this->y;
}
float RectangleF::get_width() const
{
	return this->width;
}
float RectangleF::get_height() const
{
	return this->height;
}
Vector2F RectangleF::get_center() const
{
	return Vector2F(this->x + this->width / 2.0f,
				this->y + this->height / 2.0f);
}
Vector2F RectangleF::get_position() const
{
	return Vector2F(this->x, this->y);
}
Vector2F RectangleF::get_size() const
{
	return Vector2F(this->width, this->height);
}
float RectangleF::get_left() const
{
	return this->x;
}
float RectangleF::get_right() const
{
	return this->x + this->width;
}
float RectangleF::get_top() const
{
	return this->y;
}
float RectangleF::get_bottom() const
{
	return this->y + this->height;
}
Vector2F RectangleF::get_top_left() const
{
	return Vector2F(this->x, this->y);
}
Vector2F RectangleF::get_bottom_right() const
{
	return Vector2F(this->x + this->width, this->y + this->height);
}
Vector2F RectangleF::get_top_right() const
{
	return Vector2F(this->x + this->width, this->y);
}
Vector2F RectangleF::get_bottom_left() const
{
	return Vector2F(this->x, this->y + this->height);
}
Segment RectangleF::get_top_edge() const
{
	return Segment(this->get_top_left(), this->get_top_right());
}
Segment RectangleF::get_bottom_edge() const
{
	return Segment(this->get_bottom_left(), this->get_bottom_right());
}
Segment RectangleF::get_left_edge() const
{
	return Segment(this->get_top_left(), this->get_bottom_left());
}
Segment RectangleF::get_right_edge() const
{
	return Segment(this->get_top_right(), this->get_bottom_right());
}
float RectangleF::get_area() const
{
	return this->width * this->height;
}
RectangleI RectangleF::get_rectangle_i() const
{
	return RectangleI(static_cast<int>(this->x),
		static_cast<int>(this->y),
		static_cast<int>(this->width),
		static_cast<int>(this->height));
}
SimpleMath::Rectangle RectangleF::get_sm_rectangle() const
{
	return SimpleMath::Rectangle(static_cast<long>(this->x),
				static_cast<long>(this->y),
				static_cast<long>(this->width),
				static_cast<long>(this->height));
}
RECT RectangleF::get_win_rect() const
{
	RECT result = {
		static_cast<long>(this->x),
		static_cast<long>(this->y),
		static_cast<long>(this->x + this->width),
		static_cast<long>(this->y + this->height)
	};
	return result;
}
//RectangleF& RectangleF::operator=(const Rectangle& rectangle)
//{
//	this->x = static_cast<float>(rectangle.x);
//	this->y = static_cast<float>(rectangle.y);
//	this->width = static_cast<float>(rectangle.width);
//	this->height = static_cast<float>(rectangle.height);
//	return *this;
//}
//RectangleF& RectangleF::operator=(const RECT& rectangle)
//{
//	this->x = static_cast<float>(rectangle.left);
//	this->y = static_cast<float>(rectangle.top);
//	this->width = static_cast<float>(rectangle.right - rectangle.left);
//	this->height = static_cast<float>(rectangle.bottom - rectangle.top);
//	return *this;
//}
bool RectangleF::operator==(const RectangleF& other) const
{
	return this->x == other.x &&
		this->y == other.y &&
		this->width == other.width &&
		this->height == other.height;
}
bool RectangleF::operator!=(const RectangleF& other) const
{
	return !(*this == other);
}
bool RectangleF::contains(const Vector2F& point) const
{
	return point.x >= this->x &&
		point.x <= this->x + this->width &&
		point.y >= this->y &&
		point.y <= this->y + this->height;
}
bool RectangleF::contains(const RectangleF& other) const
{
	return other.x >= this->x &&
		other.x + other.width <= this->x + this->width &&
		other.y >= this->y &&
		other.y + other.height <= this->y + this->height;
}
bool RectangleF::intersects(const RectangleF& other) const
{
	//return this->x < other.x + other.width &&
	//	this->x + this->width > other.x &&
	//	this->y < other.y + other.height &&
	//	this->y + this->height > other.y;
	//
	//pg 79
	float t;
	if ((t = this->x - other.x) > other.width || -t > this->width)
	{
		return false;
	}
	if ((t = this->y - other.y) > other.height || -t > this->height)
	{
		return false;
	}
	return true;
}
bool RectangleF::intersects(const Circle& other) const
{
	//circleDistance.x = abs(circle.x - rect.x);
	//circleDistance.y = abs(circle.y - rect.y);

	//if (circleDistance.x > (rect.width / 2 + circle.r)) { return false; }
	//if (circleDistance.y > (rect.height / 2 + circle.r)) { return false; }

	//if (circleDistance.x <= (rect.width / 2)) { return true; }
	//if (circleDistance.y <= (rect.height / 2)) { return true; }

	//cornerDistance_sq = (circleDistance.x - rect.width / 2) ^ 2 +
	//	(circleDistance.y - rect.height / 2) ^ 2;

	//return (cornerDistance_sq <= (circle.r ^ 2));

	return EricsonMath::test_circle_AABB(other, *this);
}
bool RectangleF::intersects(const Triangle& other) const
{
	// check each segment of the triangle against the rectangle
	if (EricsonMath::test_segment_AABB(other.point1, other.point2, *this))
	{
		return true;
	}
	if (EricsonMath::test_segment_AABB(other.point2, other.point3, *this))
	{
		return true;
	}
	if (EricsonMath::test_segment_AABB(other.point3, other.point1, *this))
	{
		return true;
	}
	return false;
}
bool RectangleF::intersects(const Segment& other) const
{
	return EricsonMath::test_segment_AABB(other.point1, other.point2, *this);
}
RectangleF RectangleF::intersection(const RectangleF& other) const
{
	return RectangleF::intersection(*this, other);
}
//void RectangleF::set_left(float left)
//{
//	this->width += this->x - left;
//	this->x = left;
//}
//void RectangleF::set_top(float top);
//void RectangleF::set_right(float right);
//void RectangleF::set_bottom(float bottom);
void RectangleF::inflate(float horizontal_amount, float vertical_amount)
{
	this->x -= horizontal_amount;
	this->y -= vertical_amount;
	this->width += horizontal_amount * 2.0f;
	this->height += vertical_amount * 2.0f;
}
void RectangleF::inflate(const Vector2F& amount)
{
	this->x -= amount.x;
	this->y -= amount.y;
	this->width += amount.x * 2.0f;
	this->height += amount.y * 2.0f;
}
void RectangleF::inflate_to_size(float width, float height)
{
	this->x -= (width - this->width) / 2.0f;
	this->y -= (height - this->height) / 2.0f;
	this->width = width;
	this->height = height;
}
void RectangleF::inflate_to_size(const Vector2F& size)
{
	this->x -= (size.x - this->width) / 2.0f;
	this->y -= (size.y - this->height) / 2.0f;
	this->width = size.x;
	this->height = size.y;
}
void RectangleF::scale_at_center(float scale)
{
	this->scale_at_center(scale, scale);
}
void RectangleF::scale_at_center(float horizontal_scale, float vertical_scale)
{
	Vector2F center = this->get_center();
	this->x = center.x - this->width * horizontal_scale / 2.0f;
	this->y = center.y - this->height * vertical_scale / 2.0f;
	this->width *= horizontal_scale;
	this->height *= vertical_scale;
}
void RectangleF::scale_at_center(const Vector2F& scale)
{
	this->scale_at_center(scale.x, scale.y);
}
void RectangleF::offset(float horizontal_amount, float vertical_amount)
{
	this->x += horizontal_amount;
	this->y += vertical_amount;
}
void RectangleF::offset(const MattMath::Vector2F& amount)
{
	this->x += amount.x;
	this->y += amount.y;
}
void RectangleF::scale(float horizontal_amount, float vertical_amount)
{
	this->width *= horizontal_amount;
	this->height *= vertical_amount;
}
void RectangleF::scale(const Vector2F& amount)
{
	this->width *= amount.x;
	this->height *= amount.y;
}
void RectangleF::scale_size_and_position(float horizontal_amount,
	float vertical_amount)
{
	this->x *= horizontal_amount;
	this->y *= vertical_amount;
	this->width *= horizontal_amount;
	this->height *= vertical_amount;
}
void RectangleF::scale_size_and_position(const Vector2F& amount)
{
	this->x *= amount.x;
	this->y *= amount.y;
	this->width *= amount.x;
	this->height *= amount.y;
}
void RectangleF::set_position(const Vector2F& position)
{
	this->x = position.x;
	this->y = position.y;
}
void RectangleF::set_position(float x, float y)
{
	this->x = x;
	this->y = y;
}
void RectangleF::set_position_at_center(const Vector2F& position)
{
	this->x = position.x - this->width / 2.0f;
	this->y = position.y - this->height / 2.0f;
}
void RectangleF::set_position_at_center(float x, float y)
{
	this->x = x - this->width / 2.0f;
	this->y = y - this->height / 2.0f;
}
void RectangleF::set_position_x(float x)
{
	this->x = x;
}
void RectangleF::set_position_x_from_right(float x)
{
	this->x = x - this->width;
}
void RectangleF::set_position_y(float y)
{
	this->y = y;
}
void RectangleF::set_position_y_from_bottom(float y)
{
	this->y = y - this->height;
}
void RectangleF::set_position_from_top_right(const Vector2F& position)
{
	this->x = position.x - this->width;
	this->y = position.y;
}
void RectangleF::set_position_from_top_right(float x, float y)
{
	this->x = x - this->width;
	this->y = y;
}
void RectangleF::set_position_from_bottom_left(const Vector2F& position)
{
	this->x = position.x;
	this->y = position.y - this->height;
}
void RectangleF::set_position_from_bottom_left(float x, float y)
{
	this->x = x;
	this->y = y - this->height;
}
void RectangleF::set_position_from_bottom_right(const Vector2F& position)
{
	this->x = position.x - this->width;
	this->y = position.y - this->height;
}
void RectangleF::set_position_from_bottom_right(float x, float y)
{
	this->x = x - this->width;
	this->y = y - this->height;
}
void RectangleF::set_size(const Vector2F& size)
{
	this->width = size.x;
	this->height = size.y;
}
void RectangleF::set_size(float width, float height)
{
	this->width = width;
	this->height = height;
}
void RectangleF::set_width(float width)
{
	this->width = width;
}
void RectangleF::set_height(float height)
{
	this->height = height;
}
RectangleF RectangleF::intersection(const RectangleF& a, const RectangleF& b)
{
	float x1 = std::max(a.x, b.x);
	float x2 = std::min(a.x + a.width, b.x + b.width);
	float y1 = std::max(a.y, b.y);
	float y2 = std::min(a.y + a.height, b.y + b.height);
	if (x2 >= x1 && y2 >= y1)
	{
		return RectangleF(x1, y1, x2 - x1, y2 - y1);
	}
	else
	{
		return RectangleF();
	}
}
RectangleF RectangleF::union_of(const RectangleF& a, const RectangleF& b)
{
	float x1 = std::min(a.x, b.x);
	float x2 = std::max(a.x + a.width, b.x + b.width);
	float y1 = std::min(a.y, b.y);
	float y2 = std::max(a.y + a.height, b.y + b.height);
	return RectangleF(x1, y1, x2 - x1, y2 - y1);
}
RectangleF RectangleF::from_top_left_bottom_right(const Vector2F& top_left,
	const Vector2F& bottom_right)
{
	return RectangleF(top_left.x, top_left.y,
		bottom_right.x - top_left.x,
		bottom_right.y - top_left.y);
}
RectangleF RectangleF::from_top_left_bottom_right(float top, float left,
	float bottom, float right)
{
	return RectangleF(left, top, right - left, bottom - top);
}
const RectangleF RectangleF::ZERO = { 0.0f, 0.0f, 0.0f, 0.0f };

#pragma endregion RectangleF

#pragma region Vector2I

Vector2I::Vector2I(int x, int y)
{
	this->x = x;
	this->y = y;
}
Vector2I::Vector2I(const Vector2F& vector)
{
	this->x = static_cast<int>(vector.x);
	this->y = static_cast<int>(vector.y);
}
Vector2I::Vector2I(const SimpleMath::Vector2& vector)
{
	this->x = static_cast<int>(vector.x);
	this->y = static_cast<int>(vector.y);
}
Vector2I::Vector2I(const XMINT2& vector)
{
	this->x = vector.x;
	this->y = vector.y;
}

SimpleMath::Vector2 Vector2I::get_sm_vector() const
{
	return SimpleMath::Vector2(static_cast<float>(this->x),
				static_cast<float>(this->y));
}
XMINT2 Vector2I::get_xm_vector() const
{
	return XMINT2(this->x, this->y);
}

Vector2I& Vector2I::operator=(const SimpleMath::Vector2& vector)
{
	this->x = static_cast<int>(vector.x);
	this->y = static_cast<int>(vector.y);
	return *this;
}
Vector2I& Vector2I::operator=(const XMINT2& vector)
{
	this->x = vector.x;
	this->y = vector.y;
	return *this;
}
bool Vector2I::operator==(const Vector2I& other) const
{
	return this->x == other.x && this->y == other.y;
}
bool Vector2I::operator!=(const Vector2I& other) const
{
	return !(*this == other);
}
Vector2I& Vector2I::operator+=(const Vector2I& other)
{
	this->x += other.x;
	this->y += other.y;
	return *this;
}
Vector2I& Vector2I::operator-=(const Vector2I& other)
{
	this->x -= other.x;
	this->y -= other.y;
	return *this;
}
Vector2I& Vector2I::operator*=(const Vector2I& other)
{
	this->x *= other.x;
	this->y *= other.y;
	return *this;
}
Vector2I& Vector2I::operator/=(const Vector2I& other)
{
	this->x /= other.x;
	this->y /= other.y;
	return *this;
}
Vector2I& Vector2I::operator*=(int other)
{
	this->x *= other;
	this->y *= other;
	return *this;
}
Vector2I& Vector2I::operator/=(int other)
{
	this->x /= other;
	this->y /= other;
	return *this;
}
//Vector2I Vector2I::operator+(const Vector2I& other) const
//{
//	return Vector2I(this->x + other.x, this->y + other.y);
//}
//Vector2I Vector2I::operator-(const Vector2I& other) const
//{
//	return Vector2I(this->x - other.x, this->y - other.y);
//}
void Vector2I::offset(int horizontal_amount, int vertical_amount)
{
	this->x += horizontal_amount;
	this->y += vertical_amount;
}
void Vector2I::scale(int horizontal_amount, int vertical_amount)
{
	this->x *= horizontal_amount;
	this->y *= vertical_amount;
}
void Vector2I::scale(const DirectX::SimpleMath::Vector2& amount)
{
	this->x *= static_cast<int>(amount.x);
	this->y *= static_cast<int>(amount.y);
}
void Vector2I::set(int x, int y)
{
	this->x = x;
	this->y = y;
}
void Vector2I::set(const SimpleMath::Vector2& vector)
{
	this->x = static_cast<int>(vector.x);
	this->y = static_cast<int>(vector.y);
}
void Vector2I::set(const XMINT2& vector)
{
	this->x = vector.x;
	this->y = vector.y;
}
const Vector2I Vector2I::ZERO = { 0, 0 };
Vector2I MattMath::operator+ (const Vector2I& V1, const Vector2I& V2)
{
	return Vector2I(V1.x + V2.x, V1.y + V2.y);
}
Vector2I MattMath::operator- (const Vector2I& V1, const Vector2I& V2)
{
	return Vector2I(V1.x - V2.x, V1.y - V2.y);
}
Vector2I MattMath::operator* (const Vector2I& V1, const Vector2I& V2)
{
	return Vector2I(V1.x * V2.x, V1.y * V2.y);
}
Vector2I MattMath::operator* (const Vector2I& V, int S)
{
	return Vector2I(V.x * S, V.y * S);
}
Vector2I MattMath::operator/ (const Vector2I& V1, const Vector2I& V2)
{
	return Vector2I(V1.x / V2.x, V1.y / V2.y);
}
Vector2I MattMath::operator/ (const Vector2I& V, int S)
{
	return Vector2I(V.x / S, V.y / S);
}
Vector2I MattMath::operator* (int S, const Vector2I& V)
{
	return Vector2I(V.x * S, V.y * S);
}

#pragma endregion Vector2I

#pragma region Vector2F

Vector2F::Vector2F(float f)
{
	this->x = f;
	this->y = f;
}
Vector2F::Vector2F(float x, float y)
{
	this->x = x;
	this->y = y;
}
Vector2F::Vector2F(const SimpleMath::Vector2& vector)
{
	this->x = vector.x;
	this->y = vector.y;
}
Vector2F::Vector2F(const XMFLOAT2& vector)
{
	this->x = vector.x;
	this->y = vector.y;
}
Vector2F::Vector2F(const Vector2I vector)
{
	this->x = static_cast<float>(vector.x);
	this->y = static_cast<float>(vector.y);
}
SimpleMath::Vector2 Vector2F::get_sm_vector() const
{
	return SimpleMath::Vector2(this->x, this->y);
}
XMFLOAT2 Vector2F::get_xm_vector() const
{
	return XMFLOAT2(this->x, this->y);
}
Vector2F& Vector2F::operator=(const SimpleMath::Vector2& vector)
{
	this->x = vector.x;
	this->y = vector.y;
	return *this;
}
Vector2F& Vector2F::operator=(const XMFLOAT2& vector)
{
	this->x = vector.x;
	this->y = vector.y;
	return *this;
}
bool Vector2F::operator==(const Vector2F& other) const
{
	return this->x == other.x && this->y == other.y;
}
bool Vector2F::operator!=(const Vector2F& other) const
{
	return !(*this == other);
}
Vector2F& Vector2F::operator+=(const Vector2F& other)
{
	this->x += other.x;
	this->y += other.y;
	return *this;
}
Vector2F& Vector2F::operator-=(const Vector2F& other)
{
	this->x -= other.x;
	this->y -= other.y;
	return *this;
}
Vector2F& Vector2F::operator*=(const Vector2F& other)
{
	this->x *= other.x;
	this->y *= other.y;
	return *this;
}
Vector2F& Vector2F::operator/=(const Vector2F& other)
{
	this->x /= other.x;
	this->y /= other.y;
	return *this;
}
Vector2F& Vector2F::operator*=(float other)
{
	this->x *= other;
	this->y *= other;
	return *this;
}
Vector2F& Vector2F::operator/=(float other)
{
	this->x /= other;
	this->y /= other;
	return *this;
}
//Vector2F& Vector2F::operator/(float other)
//{
//	this->x /= other;
//	this->y /= other;
//	return *this;
//}
//Vector2F& Vector2F::operator*(float other)
//{
//	this->x *= other;
//	this->y *= other;
//	return *this;
//}
//Vector2F& Vector2F::operator+(const Vector2F& other)
//{
//	this->x += other.x;
//	this->y += other.y;
//	return *this;
//}
//Vector2F& Vector2F::operator-(const Vector2F& other)
//{
//	this->x -= other.x;
//	this->y -= other.y;
//	return *this;
//}
//Vector2F& Vector2F::operator*(const Vector2F& other)
//{
//	this->x *= other.x;
//	this->y *= other.y;
//	return *this;
//}
//Vector2F& Vector2F::operator/(const Vector2F& other)
//{
//	this->x /= other.x;
//	this->y /= other.y;
//	return *this;
//}
float Vector2F::length() const
{
	return std::sqrtf(this->x * this->x + this->y * this->y);
}
float Vector2F::length_squared() const
{
	return this->x * this->x + this->y * this->y;
}
direction Vector2F::get_direction() const
{
	if (are_equal(this->x, 0.0f) && are_equal(this->y, 0.0f))
	{
		return direction::NONE;
	}
	else if (are_equal(this->x, 0.0f))
	{
		if (this->y > 0.0f)
		{
			return direction::DOWN;
		}
		else
		{
			return direction::UP;
		}
	}
	else if (are_equal(this->y, 0.0f))
	{
		if (this->x > 0.0f)
		{
			return direction::RIGHT;
		}
		else
		{
			return direction::LEFT;
		}
	}
	else
	{
		if (this->x > 0.0f)
		{
			if (this->y > 0.0f)
			{
				return direction::DOWN_RIGHT;
			}
			else
			{
				return direction::UP_RIGHT;
			}
		}
		else
		{
			if (this->y > 0.0f)
			{
				return direction::DOWN_LEFT;
			}
			else
			{
				return direction::UP_LEFT;
			}
		}
	}
}
float Vector2F::dot(const Vector2F& other) const
{
	return this->x * other.x + this->y * other.y;
}
Vector2F Vector2F::cross(const Vector2F& other) const
{
	return Vector2F(this->x * other.y, this->y * other.x);
}
Vector2F Vector2F::normalized() const
{
	float length = this->length();
	return Vector2F(this->x / length, this->y / length);
}
void Vector2F::normalize()
{
	float length = this->length();
	this->x /= length;
	this->y /= length;
}
bool Vector2F::is_contained_within(const RectangleF& other) const
{
	return other.contains(*this);
}
void Vector2F::clamp(const Vector2F& min, const Vector2F& max)
{
	this->x = std::min(std::max(this->x, min.x), max.x);
	this->y = std::min(std::max(this->y, min.y), max.y);
}
Vector2F Vector2F::clamped(const Vector2F& min, const Vector2F& max) const
{
	return Vector2F(std::min(std::max(this->x, min.x), max.x),
		std::min(std::max(this->y, min.y), max.y));
}
float Vector2F::angle() const
{
	return std::atan2(this->y, this->x);
}
float Vector2F::angle(const Vector2F& vec)
{
	return std::atan2(vec.y, vec.x);
}
void Vector2F::rotate(float angle)
{
	float cos_angle = std::cos(angle);
	float sin_angle = std::sin(angle);

	float x = this->x * cos_angle - this->y * sin_angle;
	float y = this->x * sin_angle + this->y * cos_angle;

	this->x = x;
	this->y = y;
}
Vector2F Vector2F::rotate_vector(const Vector2F& vec, float angle)
{
	float cos_angle = std::cos(angle);
	float sin_angle = std::sin(angle);

	return Vector2F(vec.x * cos_angle - vec.y * sin_angle,
		vec.x * sin_angle + vec.y * cos_angle);
}
void Vector2F::rotate_vector_by_ref(Vector2F& vec, float angle)
{
	float cos_angle = std::cos(angle);
	float sin_angle = std::sin(angle);

	vec.x = vec.x * cos_angle - vec.y * sin_angle;
	vec.y = vec.x * sin_angle + vec.y * cos_angle;
}
Vector2F Vector2F::lerp(const Vector2F& a, const Vector2F& b, float t)
{
	return Vector2F(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}
Vector2F Vector2F::lerp(const Vector2F& a, const Vector2F& b, const Vector2F& t)
{
	return Vector2F(a.x + (b.x - a.x) * t.x, a.y + (b.y - a.y) * t.y);
}
float Vector2F::distance(const Vector2F& a, const Vector2F& b)
{
	return sqrtf((a.x - b.x) * (a.x - b.x) +
		(a.y - b.y) * (a.y - b.y));
}
float Vector2F::distance_squared(const Vector2F& a, const Vector2F& b)
{
	return (a.x - b.x) * (a.x - b.x) +
		(a.y - b.y) * (a.y - b.y);
}
float Vector2F::dot(const Vector2F& a, const Vector2F& b)
{
	return a.x * b.x + a.y * b.y;
}
Vector2F Vector2F::min(const Vector2F& a, const Vector2F& b)
{
	return Vector2F(std::min(a.x, b.x), std::min(a.y, b.y));
}
Vector2F Vector2F::max(const Vector2F& a, const Vector2F& b)
{
	return Vector2F(std::max(a.x, b.x), std::max(a.y, b.y));
}
Vector2F Vector2F::vec_from_angle_magnitude(float angle, float magnitude)
{
	float x = magnitude * std::cos(angle);
	float y = magnitude * std::sin(angle);
	return Vector2F(x, y);
}
Vector2F Vector2F::unit_vec_from_angle(float angle)
{
	return Vector2F(std::cos(angle), std::sin(angle));
}

const Vector2F Vector2F::ZERO = { 0.0f, 0.0f };
const Vector2F Vector2F::ONE = { 1.0f, 1.0f };
const Vector2F Vector2F::UNIT_X = { 1.0f, 0.0f };
const Vector2F Vector2F::UNIT_Y = { 0.0f, 1.0f };


Vector2F MattMath::operator+ (const Vector2F& V1, const Vector2F& V2)
{
	return Vector2F(V1.x + V2.x, V1.y + V2.y);
}
Vector2F MattMath::operator- (const Vector2F& V1, const Vector2F& V2)
{
	return Vector2F(V1.x - V2.x, V1.y - V2.y);
}
Vector2F MattMath::operator* (const Vector2F& V1, const Vector2F& V2)
{
	return Vector2F(V1.x * V2.x, V1.y * V2.y);
}
Vector2F MattMath::operator* (const Vector2F& V, float S)
{
	return Vector2F(V.x * S, V.y * S);
}
Vector2F MattMath::operator/ (const Vector2F& V1, const Vector2F& V2)
{
	return Vector2F(V1.x / V2.x, V1.y / V2.y);
}
Vector2F MattMath::operator/ (const Vector2F& V, float S)
{
	return Vector2F(V.x / S, V.y / S);
}
Vector2F MattMath::operator* (float S, const Vector2F& V)
{
	return Vector2F(V.x * S, V.y * S);
}

#pragma endregion Vector2F

#pragma region RectangleI

RectangleI::RectangleI(int x, int y, int width, int height)
{
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
}
RectangleI::RectangleI(const Vector2I& position,
	const Vector2I& size)
{
	this->x = position.x;
	this->y = position.y;
	this->width = size.x;
	this->height = size.y;
}
RectangleI::RectangleI(const Vector2F& position,const Vector2F& size)
{
	this->x = static_cast<int>(position.x);
	this->y = static_cast<int>(position.y);
	this->width = static_cast<int>(size.x);
	this->height = static_cast<int>(size.y);
}
RectangleI::RectangleI(const SimpleMath::Vector2& position,
	const SimpleMath::Vector2& size)
{
	this->x = static_cast<int>(position.x);
	this->y = static_cast<int>(position.y);
	this->width = static_cast<int>(size.x);
	this->height = static_cast<int>(size.y);
}
RectangleI::RectangleI(RectangleF rectangle)
{
	this->x = static_cast<int>(rectangle.x);
	this->y = static_cast<int>(rectangle.y);
	this->width = static_cast<int>(rectangle.width);
	this->height = static_cast<int>(rectangle.height);
}
RectangleI::RectangleI(const SimpleMath::Rectangle& rectangle)
{
	this->x = rectangle.x;
	this->y = rectangle.y;
	this->width = rectangle.width;
	this->height = rectangle.height;
}
RectangleI::RectangleI(const RECT& rectangle)
{
	this->x = rectangle.left;
	this->y = rectangle.top;
	this->width = rectangle.right - rectangle.left;
	this->height = rectangle.bottom - rectangle.top;
}

int RectangleI::get_left() const
{
	return this->x;
}
int RectangleI::get_top() const
{
	return this->y;
}
int RectangleI::get_right() const
{
	return this->x + this->width;
}
int RectangleI::get_bottom() const
{
	return this->y + this->height;
}
Vector2I RectangleI::get_position() const
{
	return Vector2I(this->x, this->y);
}
Vector2I RectangleI::get_size() const
{
	return Vector2I(this->width, this->height);
}
Vector2I RectangleI::get_top_left() const
{
	return Vector2I(this->x, this->y);
}
Vector2I RectangleI::get_bottom_right() const
{
	return Vector2I(this->x + this->width, this->y + this->height);
}
DirectX::SimpleMath::Rectangle RectangleI::get_sm_rectangle() const
{
	return SimpleMath::Rectangle(static_cast<long>(this->x),
		static_cast<long>(this->y),
		static_cast<long>(this->width),
		static_cast<long>(this->height));
}
RECT RectangleI::get_win_rect() const
{
	RECT result = {
		static_cast<long>(this->x),
		static_cast<long>(this->y),
		static_cast<long>(this->x + this->width),
		static_cast<long>(this->y + this->height)
	};
	return result;
}
bool RectangleI::operator==(const RectangleI& other) const
{
	return this->x == other.x &&
		this->y == other.y &&
		this->width == other.width &&
		this->height == other.height;
}
bool RectangleI::operator!=(const RectangleI& other) const
{
	return !(*this == other);
}
bool RectangleI::contains(const Vector2I& point) const
{
	return point.x >= this->x &&
		point.x <= this->x + this->width &&
		point.y >= this->y &&
		point.y <= this->y + this->height;

}
bool RectangleI::contains(const RectangleI& other) const
{
	return other.x >= this->x &&
		other.x + other.width <= this->x + this->width &&
		other.y >= this->y &&
		other.y + other.height <= this->y + this->height;

}
void RectangleI::offset(int horizontal_amount, int vertical_amount)
{
	this->x += horizontal_amount;
	this->y += vertical_amount;
}
void RectangleI::offset(const Vector2I& amount)
{
	this->x += amount.x;
	this->y += amount.y;
}
void RectangleI::set_left(int left)
{
	this->width += this->x - left;
	this->x = left;
}
void RectangleI::set_top(int top)
{
	this->height += this->y - top;
	this->y = top;
}
void RectangleI::set_right(int right)
{
	this->width = right - this->x;
}
void RectangleI::set_bottom(int bottom)
{
	this->height = bottom - this->y;
}
void RectangleI::set_position(const Vector2I& position)
{
	this->x = position.x;
	this->y = position.y;
}
void RectangleI::set_size(const Vector2I& size)
{
	this->width = size.x;
	this->height = size.y;
}
void RectangleI::set_top_left(const Vector2I& top_left)
{
	this->width += this->x - top_left.x;
	this->height += this->y - top_left.y;
	this->x = top_left.x;
	this->y = top_left.y;
}
void RectangleI::set_bottom_right(const Vector2I& bottom_right)
{
	this->width = bottom_right.x - this->x;
	this->height = bottom_right.y - this->y;
}
void RectangleI::set_top_left_and_bottom_right(const Vector2I& top_left,
	const Vector2I& bottom_right)
{
	this->width = bottom_right.x - top_left.x;
	this->height = bottom_right.y - top_left.y;
	this->x = top_left.x;
	this->y = top_left.y;
}
const RectangleI RectangleI::ZERO = { 0, 0, 0, 0 };

#pragma endregion RectangleI

#pragma region Vector4F

Vector4F::Vector4F(float x, float y, float z, float w)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}
Vector4F::Vector4F(const XMFLOAT4& vector)
{
	this->x = vector.x;
	this->y = vector.y;
	this->z = vector.z;
	this->w = vector.w;
}
Vector4F& Vector4F::operator=(const XMFLOAT4& vector)
{
	this->x = vector.x;
	this->y = vector.y;
	this->z = vector.z;
	this->w = vector.w;
	return *this;
}
bool Vector4F::operator==(const Vector4F& other) const
{
	return this->x == other.x &&
		this->y == other.y &&
		this->z == other.z &&
		this->w == other.w;
}
bool Vector4F::operator!=(const Vector4F& other) const
{
	return !(*this == other);
}
XMVECTOR Vector4F::get_xm_vector() const
{
	XMVECTOR result = XMVectorSet(this->x, this->y, this->z, this->w);
	return result;
}

#pragma endregion Vector4F

#pragma region Colour

Colour::Colour(float r, float g, float b) :
	r(r), g(g), b(b), a(1.0f)
{
	this->clamp_colours();
}
Colour::Colour(float r, float g, float b, float a) :
	r(r), g(g), b(b), a(a)
{
	this->clamp_colours();
}
Colour::Colour(const DirectX::XMFLOAT4& vector) :
	r(vector.x), g(vector.y), b(vector.z), a(vector.w)
{
	this->clamp_colours();
}
Colour::Colour(const DirectX::SimpleMath::Color& color) :
	r(color.R()), g(color.G()), b(color.B()), a(color.A())
{
	this->clamp_colours();
}
Colour::Colour(int r, int g, int b, int a)
{
	this->set_from_int_rgba(r, g, b, a);
}
Colour::Colour(const std::string& hex)
{
	this->set_from_hex(hex);
}
Colour& Colour::operator=(const DirectX::XMFLOAT4& vector)
{
	this->r = vector.x;
	this->g = vector.y;
	this->b = vector.z;
	this->a = vector.w;
	this->clamp_colours();
	return *this;
}
Colour& Colour::operator=(const DirectX::SimpleMath::Color& colour)
{
	this->r = colour.R();
	this->g = colour.G();
	this->b = colour.B();
	this->a = colour.A();
	this->clamp_colours();
	return *this;
}
Colour& Colour::operator=(const MattMath::Vector4F& vector)
{
	this->r = vector.x;
	this->g = vector.y;
	this->b = vector.z;
	this->a = vector.w;
	this->clamp_colours();
	return *this;
}
bool Colour::operator==(const Colour& other) const
{
	return this->r == other.r &&
		this->g == other.g &&
		this->b == other.b &&
		this->a == other.a;
}
bool Colour::operator!=(const Colour& other) const
{
	return !(*this == other);
}
Colour& Colour::operator+=(const Colour& other)
{
	this->r += other.r;
	this->g += other.g;
	this->b += other.b;
	this->a += other.a;
	this->clamp_colours();
	return *this;
}
Colour& Colour::operator-=(const Colour& other)
{
	this->r -= other.r;
	this->g -= other.g;
	this->b -= other.b;
	this->a -= other.a;
	this->clamp_colours();
	return *this;
}
Colour& Colour::operator*=(const Colour& other)
{
	this->r *= other.r;
	this->g *= other.g;
	this->b *= other.b;
	this->a *= other.a;
	this->clamp_colours();
	return *this;
}
Colour& Colour::operator*=(float f)
{
	this->r *= f;
	this->g *= f;
	this->b *= f;
	this->a *= f;
	this->clamp_colours();
	return *this;
}
Colour& Colour::operator/=(const Colour& other)
{
	this->r /= other.r;
	this->g /= other.g;
	this->b /= other.b;
	this->a /= other.a;
	this->clamp_colours();
	return *this;
}
Colour& Colour::operator/=(float f)
{
	this->r /= f;
	this->g /= f;
	this->b /= f;
	this->a /= f;
	this->clamp_colours();
	return *this;
}
float Colour::get_red() const
{
	return this->r;
}
float Colour::get_green() const
{
	return this->g;
}
float Colour::get_blue() const
{
	return this->b;
}
float Colour::get_alpha() const
{
	return this->a;
}
void Colour::set_red(float red)
{
	this->r = red;
	this->clamp_colours();
}
void Colour::set_green(float green)
{
	this->g = green;
	this->clamp_colours();
}
void Colour::set_blue(float blue)
{
	this->b = blue;
	this->clamp_colours();
}
void Colour::set_alpha(float alpha)
{
	this->a = alpha;
	this->clamp_colours();
}
void Colour::set(float red, float green, float blue, float alpha)
{
	this->r = red;
	this->g = green;
	this->b = blue;
	this->a = alpha;
	this->clamp_colours();
}
void Colour::set_from_int_rgba(int r, int g, int b, int a)
{
	this->r = static_cast<float>(r) / 255.0f;
	this->g = static_cast<float>(g) / 255.0f;
	this->b = static_cast<float>(b) / 255.0f;
	this->a = static_cast<float>(a) / 255.0f;
	this->clamp_colours();
}
void Colour::set_from_hex(const std::string& hex)
{
	size_t hex_length = hex.length();
	if (hex_length == 6)
	{
		this->r = static_cast<float>(std::stoi(hex.substr(0, 2), nullptr, 16)) / 255.0f;
		this->g = static_cast<float>(std::stoi(hex.substr(2, 2), nullptr, 16)) / 255.0f;
		this->b = static_cast<float>(std::stoi(hex.substr(4, 2), nullptr, 16)) / 255.0f;
		this->a = 1.0f;
	}
	else if (hex_length == 8)
	{
		this->r = static_cast<float>(std::stoi(hex.substr(0, 2), nullptr, 16)) / 255.0f;
		this->g = static_cast<float>(std::stoi(hex.substr(2, 2), nullptr, 16)) / 255.0f;
		this->b = static_cast<float>(std::stoi(hex.substr(4, 2), nullptr, 16)) / 255.0f;
		this->a = static_cast<float>(std::stoi(hex.substr(6, 2), nullptr, 16)) / 255.0f;
	}
	else
	{
		this->r = 0.0f;
		this->g = 0.0f;
		this->b = 0.0f;
		this->a = 1.0f;
	}
	this->clamp_colours();
}
void Colour::saturate(float amount)
{
	float grey = this->r * 0.3f + this->g * 0.59f + this->b * 0.11f;
	this->r = grey + amount * (this->r - grey);
	this->g = grey + amount * (this->g - grey);
	this->b = grey + amount * (this->b - grey);
	this->clamp_colours();
}
void Colour::desaturate(float amount)
{
	float grey = this->r * 0.3f + this->g * 0.59f + this->b * 0.11f;
	this->r = grey + amount * (this->r - grey);
	this->g = grey + amount * (this->g - grey);
	this->b = grey + amount * (this->b - grey);
	this->clamp_colours();
}
void Colour::brighten(float amount)
{
	this->r += amount;
	this->g += amount;
	this->b += amount;
	this->clamp_colours();
}
void Colour::darken(float amount)
{
	this->r -= amount;
	this->g -= amount;
	this->b -= amount;
	this->clamp_colours();
}
void Colour::invert()
{
	this->r = 1.0f - this->r;
	this->g = 1.0f - this->g;
	this->b = 1.0f - this->b;
	this->clamp_colours();
}
void Colour::make_opaque()
{
	this->a = 1.0f;
}
void Colour::make_transparent()
{
	this->a = 0.0f;
}
DirectX::SimpleMath::Color Colour::get_sm_colour() const
{
	return DirectX::SimpleMath::Color(this->r, this->g, this->b, this->a);
}
DirectX::XMVECTOR Colour::get_xm_vector() const
{
	return DirectX::XMVectorSet(this->r, this->g, this->b, this->a);
}
void Colour::clamp_colours()
{
	this->r = clamp(this->r, 0.0f, 1.0f);
	this->g = clamp(this->g, 0.0f, 1.0f);
	this->b = clamp(this->b, 0.0f, 1.0f);
	this->a = clamp(this->a, 0.0f, 1.0f);
}
Colour MattMath::operator+ (const Colour& V1, const Colour& V2)
{
	return Colour(V1.r + V2.r, V1.g + V2.g, V1.b + V2.b, V1.a + V2.a);
}
Colour MattMath::operator- (const Colour& V1, const Colour& V2)
{
	return Colour(V1.r - V2.r, V1.g - V2.g, V1.b - V2.b, V1.a - V2.a);
}
Colour MattMath::operator* (const Colour& V1, const Colour& V2)
{
	return Colour(V1.r * V2.r, V1.g * V2.g, V1.b * V2.b, V1.a * V2.a);
}
Colour MattMath::operator* (const Colour& V, float S)
{
	return Colour(V.r * S, V.g * S, V.b * S, V.a * S);
}
Colour MattMath::operator/ (const Colour& V1, const Colour& V2)
{
	return Colour(V1.r / V2.r, V1.g / V2.g, V1.b / V2.b, V1.a / V2.a);
}
Colour MattMath::operator/ (const Colour& V, float S)
{
	return Colour(V.r / S, V.g / S, V.b / S, V.a / S);
}
Colour MattMath::operator* (float S, const Colour& V)
{
	return Colour(V.r * S, V.g * S, V.b * S, V.a * S);
}

#pragma endregion Colour

#pragma region MatrixF

MatrixF::MatrixF(int rows, int columns)
{
	this->_rows = rows;
	this->_columns = columns;
}
MatrixF::MatrixF(int rows, int columns, const std::vector<float>& elements)
{
	this->_rows = rows;
	this->_columns = columns;
	if (rows * columns != elements.size())
	{
		throw std::invalid_argument(
			"Number of elements does not match the dimensions of the matrix");
	}
	this->_elements = elements;
}
float MatrixF::get_element(int row, int column) const
{
	// check if the row and column are valid
	if (!row_valid(row) || !column_valid(column))
	{
		throw std::invalid_argument("Row or column is not valid");
	}
	int index = calculate_index(row, column);
	return this->_elements[index];
}
void MatrixF::set_element(int row, int column, float value)
{
	// check if the row and column are valid
	if (!row_valid(row) || !column_valid(column))
	{
		throw std::invalid_argument("Row or column is not valid");
	}
	int index = calculate_index(row, column);
	this->_elements[index] = value;
}

int MatrixF::get_rows() const
{
	return this->_rows;
}
int MatrixF::get_columns() const
{
	return this->_columns;
}
Vector2I MatrixF::get_dimensions() const
{
	return Vector2I(this->_columns, this->_rows);
}
bool MatrixF::is_square() const
{
	return this->_rows == this->_columns;
}
bool MatrixF::is_identity() const
{
	if (!this->is_square())
	{
		return false;
	}
	for (int i = 0; i < this->_rows; i++)
	{
		for (int j = 0; j < this->_columns; j++)
		{
			if (i == j)
			{
				if (this->get_element(i, j) != 1.0f)
				{
					return false;
				}
			}
			else
			{
				if (this->get_element(i, j) != 0.0f)
				{
					return false;
				}
			}
		}
	}
	return true;
}
bool MatrixF::is_symmetric() const
{
	if (!this->is_square())
	{
		return false;
	}
	for (int i = 0; i < this->_rows; i++)
	{
		for (int j = 0; j < this->_columns; j++)
		{
			if (this->get_element(i, j) != this->get_element(j, i))
			{
				return false;
			}
		}
	}
	return true;
}
bool MatrixF::is_diagonal() const
{
	if (!this->is_square())
	{
		return false;
	}
	for (int i = 0; i < this->_rows; i++)
	{
		for (int j = 0; j < this->_columns; j++)
		{
			if (i != j && this->get_element(i, j) != 0.0f)
			{
				return false;
			}
		}
	}
	return true;
}
bool MatrixF::is_upper_triangular() const
{
	if (!this->is_square())
	{
		return false;
	}
	for (int i = 0; i < this->_rows; i++)
	{
		for (int j = 0; j < i; j++)
		{
			if (this->get_element(i, j) != 0.0f)
			{
				return false;
			}
		}
	}
	return true;
}
bool MatrixF::is_lower_triangular() const
{
	if (!this->is_square())
	{
		return false;
	}
	for (int i = 0; i < this->_rows; i++)
	{
		for (int j = i + 1; j < this->_columns; j++)
		{
			if (this->get_element(i, j) != 0.0f)
			{
				return false;
			}
		}
	}
	return true;
}
bool MatrixF::is_invertible() const
{
	if (!this->is_square())
	{
		return false;
	}
	return MattMath::determinant(*this) != 0.0f;
}
//bool MatrixF::is_row_echelon_form() const
//{
//
//}
//bool MatrixF::is_reduced_row_echelon_form() const
//{
//
//}
bool MatrixF::operator==(const MatrixF& other) const
{
	Vector2I dimensions = this->get_dimensions();
	Vector2I other_dimensions = other.get_dimensions();
	if (dimensions != other_dimensions)
	{
		return false;
	}
	for (int i = 0; i < dimensions.x; i++)
	{
		for (int j = 0; j < dimensions.y; j++)
		{
			if (this->get_element(i, j) != other.get_element(i, j))
			{
				return false;
			}
		}
	}
	return true;
}
bool MatrixF::operator!=(const MatrixF& other) const
{
	return !(*this == other);
}
MatrixF& MatrixF::operator+=(const MatrixF& other)
{
	*this = MattMath::add(*this, other);
	return *this;
}
MatrixF& MatrixF::operator-=(const MatrixF& other)
{
	*this = MattMath::subtract(*this, other);
	return *this;
}
MatrixF& MatrixF::operator*=(const MatrixF& other)
{
	*this = MattMath::multiply(*this, other);
	return *this;
}
MatrixF& MatrixF::operator/=(const MatrixF& other)
{
	*this = MattMath::divide(*this, other);
	return *this;
}
MatrixF& MatrixF::operator*=(float other)
{
	*this = MattMath::multiply(*this, other);
	return *this;
}
MatrixF& MatrixF::operator/=(float other)
{
	*this = MattMath::divide(*this, other);
	return *this;
}
bool MatrixF::row_valid(int row) const
{
	return row >= 0 && row < this->_rows;
}
bool MatrixF::column_valid(int column) const
{
	return column >= 0 && column < this->_columns;
}
int MatrixF::calculate_index(int row, int column) const
{
	return row * this->_columns + column;
}
MatrixF MattMath::operator+ (const MatrixF& a, const MatrixF& b)
{
	return MattMath::add(a, b);
}
MatrixF MattMath::operator- (const MatrixF& a, const MatrixF& b)
{
	return MattMath::subtract(a, b);
}
MatrixF MattMath::operator* (const MatrixF& a, const MatrixF& b)
{
	return MattMath::multiply(a, b);
}
MatrixF MattMath::operator/ (const MatrixF& a, const MatrixF& b)
{
	return MattMath::divide(a, b);
}
bool MattMath::equal_dimensions(const MatrixF& a, const MatrixF& b)
{
	return a.get_dimensions() == b.get_dimensions();
}
MatrixF MattMath::add(const MatrixF& a, const MatrixF& b)
{
	// Check if the dimensions are the same
	if (!equal_dimensions(a, b))
	{
		throw std::invalid_argument("Matrix dimensions are not the same");
	}
	int rows = a.get_rows();
	int columns = a.get_columns();
	MatrixF result(rows, columns);
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			result.set_element(i, j, a.get_element(i, j) + b.get_element(i, j));
		}
	}
	return result;
}
MatrixF MattMath::subtract(const MatrixF& a, const MatrixF& b)
{
	// Check if the dimensions are the same
	if (!equal_dimensions(a, b))
	{
		throw std::invalid_argument("Matrix dimensions are not the same");
	}
	int rows = a.get_rows();
	int columns = a.get_columns();
	MatrixF result(rows, columns);
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			result.set_element(i, j, a.get_element(i, j) - b.get_element(i, j));
		}
	}
	return result;
}
MatrixF MattMath::multiply(const MatrixF& a, const MatrixF& b)
{
	// check if the dimensions are correct
	if (a.get_columns() != b.get_rows())
	{
		throw std::invalid_argument("Matrix dimensions are not correct");
	}
	MatrixF result(a.get_rows(), b.get_columns());
	for (int i = 0; i < a.get_rows(); i++)
	{
		for (int j = 0; j < b.get_columns(); j++)
		{
			float sum = 0.0f;
			for (int k = 0; k < a.get_columns(); k++)
			{
				sum += a.get_element(i, k) * b.get_element(k, j);
			}
			result.set_element(i, j, sum);
		}
	}
	return result;
}
MatrixF MattMath::divide(const MatrixF& a, const MatrixF& b)
{
	// check if the dimensions are correct
	if (a.get_columns() != b.get_rows())
	{
		throw std::invalid_argument("Matrix dimensions are not correct");
	}
	MatrixF result(a.get_rows(), b.get_columns());
	for (int i = 0; i < a.get_rows(); i++)
	{
		for (int j = 0; j < b.get_columns(); j++)
		{
			float sum = 0.0f;
			for (int k = 0; k < a.get_columns(); k++)
			{
				sum += a.get_element(i, k) / b.get_element(k, j);
			}
			result.set_element(i, j, sum);
		}
	}
	return result;
}
MatrixF MattMath::multiply(const MatrixF& a, float b)
{
	int rows = a.get_rows();
	int columns = a.get_columns();
	MatrixF result(rows, columns);
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			result.set_element(i, j, a.get_element(i, j) * b);
		}
	}
	return result;
}
MatrixF MattMath::divide(const MatrixF& a, float b)
{
	int rows = a.get_rows();
	int columns = a.get_columns();
	MatrixF result(rows, columns);
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			result.set_element(i, j, a.get_element(i, j) / b);
		}
	}
	return result;
}
MatrixF MattMath::gaussian_elimination(const MatrixF& matrix)
{
	MatrixF result = matrix;
	int rows = result.get_rows();
	int columns = result.get_columns();
	int i = 0;
	int j = 0;
	while (i < rows && j < columns)
	{
		// Find the pivot
		int pivot_row = i;
		for (int k = i + 1; k < rows; k++)
		{
			if (std::abs(result.get_element(k, j)) > std::abs(result.get_element(pivot_row, j)))
			{
				pivot_row = k;
			}
		}
		if (result.get_element(pivot_row, j) != 0.0f)
		{
			// Swap the rows
			if (pivot_row != i)
			{
				for (int k = 0; k < columns; k++)
				{
					float temp = result.get_element(i, k);
					result.set_element(i, k, result.get_element(pivot_row, k));
					result.set_element(pivot_row, k, temp);
				}
			}
			// Make the pivot 1
			float pivot = result.get_element(i, j);
			for (int k = 0; k < columns; k++)
			{
				result.set_element(i, k, result.get_element(i, k) / pivot);
			}
			// Make the other elements in the column 0
			for (int k = 0; k < rows; k++)
			{
				if (k != i)
				{
					float factor = result.get_element(k, j);
					for (int l = 0; l < columns; l++)
					{
						result.set_element(k, l, result.get_element(k, l) - factor * result.get_element(i, l));
					}
				}
			}
			i++;
		}
		j++;
	}
	return result;
}
MatrixF MattMath::transpose(const MatrixF& matrix)
{
	int rows = matrix.get_rows();
	int columns = matrix.get_columns();
	MatrixF result(columns, rows);
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			result.set_element(j, i, matrix.get_element(i, j));
		}
	}
	return result;
}
//MatrixF MattMath::inverse(const MatrixF& matrix)
//{
//}
MatrixF MattMath::identity(int size)
{
	MatrixF result(size, size);
	for (int i = 0; i < size; i++)
	{
		result.set_element(i, i, 1.0f);
	}
	return result;
}
MatrixF MattMath::zero(int rows, int columns)
{
	MatrixF result(rows, columns);
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			result.set_element(i, j, 0.0f);
		}
	}
	return result;
}
float MattMath::determinant(const MatrixF& matrix)
{
	if (!matrix.is_square())
	{
		throw std::invalid_argument("Matrix is not square");
	}
	int size = matrix.get_rows();
	if (size == 1)
	{
		return matrix.get_element(0, 0);
	}
	else if (size == 2)
	{
		return matrix.get_element(0, 0) * matrix.get_element(1, 1) -
			matrix.get_element(0, 1) * matrix.get_element(1, 0);
	}
	else
	{
		float result = 0.0f;
		for (int i = 0; i < size; i++)
		{
			MatrixF submatrix(size - 1, size - 1);
			for (int j = 1; j < size; j++)
			{
				for (int k = 0; k < size; k++)
				{
					if (k < i)
					{
						submatrix.set_element(j - 1, k, matrix.get_element(j, k));
					}
					else if (k > i)
					{
						submatrix.set_element(j - 1, k - 1, matrix.get_element(j, k));
					}
				}
			}
			result += matrix.get_element(0, i) * determinant(submatrix) * ((i % 2 == 0) ? 1 : -1);
		}
		return result;
	}
}
//std::vector<MatrixF> MattMath::eigenvectors(const MatrixF& matrix)
//{
//}
//std::vector<float> MattMath::eigenvalues(const MatrixF& matrix)
//{
//}

#pragma endregion MatrixF

#pragma region Matrix3x3F

Matrix3x3F::Matrix3x3F(const std::vector<float>& elements) :
	MatrixF(3, 3, elements)
{
	if (elements.size() != 9)
	{
		throw std::invalid_argument("Elements vector must have 9 elements");
	}
}
Matrix3x3F::Matrix3x3F(float e11, float e12, float e13,
	float e21, float e22, float e23,
	float e31, float e32, float e33) :
	MatrixF(3, 3)
{
	this->set_element(0, 0, e11);
	this->set_element(0, 1, e12);
	this->set_element(0, 2, e13);
	this->set_element(1, 0, e21);
	this->set_element(1, 1, e22);
	this->set_element(1, 2, e23);
	this->set_element(2, 0, e31);
	this->set_element(2, 1, e32);
	this->set_element(2, 2, e33);
}
Matrix3x3F Matrix3x3F::rotation(float angle)
{
	float cos_angle = std::cos(angle);
	float sin_angle = std::sin(angle);
	return Matrix3x3F(cos_angle, -sin_angle, 0.0f,
		sin_angle, cos_angle, 0.0f,
		0.0f, 0.0f, 1.0f);
}
Matrix3x3F Matrix3x3F::scale(float x, float y)
{
	return Matrix3x3F(x, 0.0f, 0.0f,
		0.0f, y, 0.0f,
		0.0f, 0.0f, 1.0f);
}
Matrix3x3F Matrix3x3F::scale(float scale)
{
	return Matrix3x3F(scale, 0.0f, 0.0f,
		0.0f, scale, 0.0f,
		0.0f, 0.0f, 1.0f);
}
Matrix3x3F Matrix3x3F::scale(const Vector2F& scale)
{
	return Matrix3x3F(scale.x, 0.0f, 0.0f,
		0.0f, scale.y, 0.0f,
		0.0f, 0.0f, 1.0f);

}
Matrix3x3F Matrix3x3F::translation(float x, float y)
{
	return Matrix3x3F(1.0f, 0.0f, x,
		0.0f, 1.0f, y,
		0.0f, 0.0f, 1.0f);
}
Matrix3x3F Matrix3x3F::translation(const Vector2F& translation)
{
	return Matrix3x3F(1.0f, 0.0f, translation.x,
		0.0f, 1.0f, translation.y,
		0.0f, 0.0f, 1.0f);
}

#pragma endregion Matrix3x3F

#pragma region Vector3F

Vector3F::Vector3F(float x, float y, float z) :
	x(x), y(y), z(z)
{
}
bool Vector3F::operator==(const Vector3F& other) const
{
	return this->x == other.x &&
		this->y == other.y &&
		this->z == other.z;
}
bool Vector3F::operator!=(const Vector3F& other) const
{
	return !(*this == other);
}
Vector3F& Vector3F::operator+=(const Vector3F& other)
{
	this->x += other.x;
	this->y += other.y;
	this->z += other.z;
	return *this;
}
Vector3F& Vector3F::operator-=(const Vector3F& other)
{
	this->x -= other.x;
	this->y -= other.y;
	this->z -= other.z;
	return *this;
}
Vector3F& Vector3F::operator*=(const Vector3F& other)
{
	this->x *= other.x;
	this->y *= other.y;
	this->z *= other.z;
	return *this;
}
Vector3F& Vector3F::operator/=(const Vector3F& other)
{
	this->x /= other.x;
	this->y /= other.y;
	this->z /= other.z;
	return *this;
}
Vector3F& Vector3F::operator*=(float other)
{
	this->x *= other;
	this->y *= other;
	this->z *= other;
	return *this;
}
Vector3F& Vector3F::operator/=(float other)
{
	this->x /= other;
	this->y /= other;
	this->z /= other;
	return *this;
}
Vector3F MattMath::operator+ (const Vector3F& V1, const Vector3F& V2)
{
	return Vector3F(V1.x + V2.x, V1.y + V2.y, V1.z + V2.z);
}
Vector3F MattMath::operator- (const Vector3F& V1, const Vector3F& V2)
{
	return Vector3F(V1.x - V2.x, V1.y - V2.y, V1.z - V2.z);
}
Vector3F MattMath::operator* (const Vector3F& V1, const Vector3F& V2)
{
	return Vector3F(V1.x * V2.x, V1.y * V2.y, V1.z * V2.z);
}
Vector3F MattMath::operator* (const Vector3F& V, float S)
{
	return Vector3F(V.x * S, V.y * S, V.z * S);
}
Vector3F MattMath::operator/ (const Vector3F& V1, const Vector3F& V2)
{
	return Vector3F(V1.x / V2.x, V1.y / V2.y, V1.z / V2.z);
}
Vector3F MattMath::operator/ (const Vector3F& V, float S)
{
	return Vector3F(V.x / S, V.y / S, V.z / S);
}
Vector3F MattMath::operator* (float S, const Vector3F& V)
{
	return Vector3F(V.x * S, V.y * S, V.z * S);
}
//Vector3F Vector3F::operator+(const Vector3F& other) const
//{
//	return Vector3F(this->x + other.x, this->y + other.y, this->z + other.z);
//}
//Vector3F Vector3F::operator-(const Vector3F& other) const
//{
//	return Vector3F(this->x - other.x, this->y - other.y, this->z - other.z);
//}

#pragma endregion Vector3F

#pragma region Viewport

Viewport::Viewport(float x, float y, float width, float height,
	float minDepth, float maxDepth) :
	x(x), y(y), width(width), height(height),
	minDepth(minDepth), maxDepth(maxDepth)
{

}
Viewport::Viewport(const SimpleMath::Viewport& viewport)
{
	this->x = viewport.x;
	this->y = viewport.y;
	this->width = viewport.width;
	this->height = viewport.height;
	this->minDepth = viewport.minDepth;
	this->maxDepth = viewport.maxDepth;
}
Viewport::Viewport(const RectangleF& rectangle,
	float minDepth, float maxDepth)
{
	this->x = rectangle.x;
	this->y = rectangle.y;
	this->width = rectangle.width;
	this->height = rectangle.height;
	this->minDepth = minDepth;
	this->maxDepth = maxDepth;
}
Viewport::Viewport(const RectangleI& rectangle,
	float minDepth, float maxDepth)
{
	this->x = static_cast<float>(rectangle.x);
	this->y = static_cast<float>(rectangle.y);
	this->width = static_cast<float>(rectangle.width);
	this->height = static_cast<float>(rectangle.height);
	this->minDepth = minDepth;
	this->maxDepth = maxDepth;
}
Viewport::Viewport(const D3D11_VIEWPORT& viewport)
{
	this->x = viewport.TopLeftX;
	this->y = viewport.TopLeftY;
	this->width = viewport.Width;
	this->height = viewport.Height;
	this->minDepth = viewport.MinDepth;
	this->maxDepth = viewport.MaxDepth;
}
SimpleMath::Viewport Viewport::get_sm_viewport() const
{
	return SimpleMath::Viewport(this->x, this->y, this->width, this->height,
		this->minDepth, this->maxDepth);
}
D3D11_VIEWPORT Viewport::get_d3d_viewport() const
{
	D3D11_VIEWPORT result = {
		this->x,
		this->y,
		this->width,
		this->height,
		this->minDepth,
		this->maxDepth
	};
	return result;
}
const D3D11_VIEWPORT* Viewport::get_d3d_viewport_ptr() const
{
	return reinterpret_cast<const D3D11_VIEWPORT*>(this);
}
RectangleF Viewport::get_rectangle() const
{
	return RectangleF(this->x, this->y, this->width, this->height);
}
RectangleF Viewport::get_rectangle(float minDepth, float maxDepth) const
{
	return RectangleF(this->x, this->y, this->width, this->height);
}
Vector2F Viewport::get_position() const
{
	return Vector2F(this->x, this->y);
}
Vector2F Viewport::get_size() const
{
	return Vector2F(this->width, this->height);
}
//Viewport& Viewport::operator=(const Viewport& viewport)
//{
//	this->x = viewport.x;
//	this->y = viewport.y;
//	this->width = viewport.width;
//	this->height = viewport.height;
//	this->minDepth = viewport.minDepth;
//	this->maxDepth = viewport.maxDepth;
//	return *this;
//}
Viewport& Viewport::operator=(const SimpleMath::Viewport& viewport)
{
	this->x = viewport.x;
	this->y = viewport.y;
	this->width = viewport.width;
	this->height = viewport.height;
	this->minDepth = viewport.minDepth;
	this->maxDepth = viewport.maxDepth;
	return *this;
}
Viewport& Viewport::operator=(const D3D11_VIEWPORT& viewport)
{
	this->x = viewport.TopLeftX;
	this->y = viewport.TopLeftY;
	this->width = viewport.Width;
	this->height = viewport.Height;
	this->minDepth = viewport.MinDepth;
	this->maxDepth = viewport.MaxDepth;
	return *this;
}
Viewport& Viewport::operator=(const MattMath::RectangleF& rectangle)
{
	this->x = rectangle.x;
	this->y = rectangle.y;
	this->width = rectangle.width;
	this->height = rectangle.height;
	return *this;
}
Viewport& Viewport::operator=(const MattMath::RectangleI& rectangle)
{
	this->x = static_cast<float>(rectangle.x);
	this->y = static_cast<float>(rectangle.y);
	this->width = static_cast<float>(rectangle.width);
	this->height = static_cast<float>(rectangle.height);
	return *this;
}
Viewport& Viewport::operator=(const RECT& rect)
{
	this->x = static_cast<float>(rect.left);
	this->y = static_cast<float>(rect.top);
	this->width = static_cast<float>(rect.right - rect.left);
	this->height = static_cast<float>(rect.bottom - rect.top);
	return *this;
}
bool Viewport::operator==(const Viewport& other) const
{
	return this->x == other.x &&
		this->y == other.y &&
		this->width == other.width &&
		this->height == other.height &&
		this->minDepth == other.minDepth &&
		this->maxDepth == other.maxDepth;
}
bool Viewport::operator!=(const Viewport& other) const
{
	return !(*this == other);
}

#pragma endregion Viewport

#pragma region Circle

Circle::Circle(const Vector2F& center, float radius) :
	center(center), radius(radius)
{
}
Circle::Circle(const DirectX::SimpleMath::Vector2& center, float radius) :
	center(center), radius(radius)
{
}
const RectangleF& Circle::get_bounding_box() const
{
	return MattMath::RectangleF(this->center.x - this->radius,
				this->center.y - this->radius,
				this->radius * 2.0f, this->radius * 2.0f);

}
shape_type Circle::get_shape_type() const
{
	return shape_type::CIRCLE;
}
bool Circle::operator==(const Circle& other) const
{
	return this->center == other.center &&
		this->radius == other.radius;
}
bool Circle::operator!=(const Circle& other) const
{
	return !(*this == other);
}
bool Circle::contains(const Vector2F& point) const
{
	return Vector2F::distance(this->center, point) <= this->radius;

}
bool Circle::intersects(const Circle& other) const
{
	return Vector2F::distance(this->center, other.center) <=
		this->radius + other.radius;
}
bool Circle::intersects(const RectangleF& other) const
{
	return other.intersects(*this);
}
bool Circle::intersects(const Triangle& other) const
{
	Point2F closest_point;
	return EricsonMath::test_circle_triangle(*this, other.point1,
		other.point2, other.point3, closest_point);
}

#pragma endregion Circle

#pragma region Triangle

Triangle::Triangle(const Vector2F& point1,
	const Vector2F& point2,
	const Vector2F& point3) :
	point1(point1), point2(point2), point3(point3)
{
}
Triangle::Triangle(const DirectX::SimpleMath::Vector2& point1,
	const DirectX::SimpleMath::Vector2& point2,
	const DirectX::SimpleMath::Vector2& point3) :
	point1(point1), point2(point2), point3(point3)
{
}
const RectangleF& Triangle::get_bounding_box() const
{
	float x1 = std::min(std::min(this->point1.x, this->point2.x),
					this->point3.x);
	float x2 = std::max(std::max(this->point1.x, this->point2.x),
				this->point3.x);
	float y1 = std::min(std::min(this->point1.y, this->point2.y),
				this->point3.y);
	float y2 = std::max(std::max(this->point1.y, this->point2.y),
				this->point3.y);
	return RectangleF(x1, y1, x2 - x1, y2 - y1);
}
shape_type Triangle::get_shape_type() const
{
	return shape_type::TRIANGLE;
}
Segment Triangle::get_segment1() const
{
	return Segment(this->point1, this->point2);
}
Segment Triangle::get_segment2() const
{
	return Segment(this->point2, this->point3);
}
Segment Triangle::get_segment3() const
{
	return Segment(this->point3, this->point1);
}
bool Triangle::operator==(const Triangle& other) const
{
	return this->point1 == other.point1 &&
		this->point2 == other.point2 &&
		this->point3 == other.point3;
}
bool Triangle::operator!=(const Triangle& other) const
{
	return !(*this == other);
}
bool Triangle::intersects(const Triangle& other) const
{
	// check if any of the segments intersect
	if (this->get_segment1().intersects(other.get_segment1())) return true;
	if (this->get_segment1().intersects(other.get_segment2())) return true;
	if (this->get_segment1().intersects(other.get_segment3())) return true;
	if (this->get_segment2().intersects(other.get_segment1())) return true;
	if (this->get_segment2().intersects(other.get_segment2())) return true;
	if (this->get_segment2().intersects(other.get_segment3())) return true;
	if (this->get_segment3().intersects(other.get_segment1())) return true;
	if (this->get_segment3().intersects(other.get_segment2())) return true;
	if (this->get_segment3().intersects(other.get_segment3())) return true;
	return false;
}
bool Triangle::intersects(const RectangleF& other) const
{
	return other.intersects(*this);
}
bool Triangle::intersects(const Circle& other) const
{
	return other.intersects(*this);
}
bool Triangle::contains(const Vector2F& point) const
{
	//float d1, d2, d3;
	//bool has_neg, has_pos;

	//d1 = sign(point, this->point1, this->point2);
	//d2 = sign(point, this->point2, this->point3);
	//d3 = sign(point, this->point3, this->point1);

	//has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
	//has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

	//return !(has_neg && has_pos);

	return EricsonMath::test_point_triangle(point, this->point1,
		this->point2, this->point3);
}

#pragma endregion Triangle

#pragma region Segment

Segment::Segment(const Point2F& point1, const Point2F& point2) :
	point1(point1), point2(point2)
{
}
Segment::Segment(float x1, float y1, float x2, float y2) :
	point1(x1, y1), point2(x2, y2)
{
}
bool Segment::operator==(const Segment& other) const
{
	return this->point1 == other.point1 &&
		this->point2 == other.point2;
}
bool Segment::operator!=(const Segment& other) const
{
	return !(*this == other);
}
bool Segment::intersects(const Segment& other) const
{
	float t;
	Point2F intersection_point;
	return EricsonMath::test_2D_segment_segment(this->point1, this->point2,
		other.point1, other.point2, t, intersection_point);
}
bool Segment::intersects(const RectangleF& other) const
{
	return other.intersects(*this);

}
//bool Segment::intersects(const RectangleF& other) const
//{
//
//}
//bool Segment::intersects(const Circle& other) const
//{
//
//}
//bool Segment::intersects(const Triangle& other) const
//{
//
//}

#pragma endregion Segment

#pragma region Camera

Camera::Camera(const Vector2F& translation, float scale) :
	translation(translation), scale(scale)
{
}
Camera::Camera(float x, float y, float scale) :
	translation(x, y), scale(scale)
{
}
Camera::Camera(const Viewport& viewport, float scale)
{
	this->translation.x = viewport.x;
	this->translation.y = viewport.y;
	this->scale = scale;
}

bool Camera::operator==(const Camera& other) const
{
	return this->translation == other.translation &&
		this->scale == other.scale;
}
bool Camera::operator!=(const Camera& other) const
{
	return !(*this == other);
}
RectangleF Camera::calculate_view_rectangle(
	const RectangleF& world_rectangle) const
{
	return RectangleF((world_rectangle.x - this->translation.x) * this->scale,
		(world_rectangle.y - this->translation.y) * this->scale,
		world_rectangle.width * this->scale,
		world_rectangle.height * this->scale);
}
void Camera::calculate_view_rectangle(RectangleF& rectangle) const
{
	rectangle.x = (rectangle.x - this->translation.x) * this->scale;
	rectangle.y = (rectangle.y - this->translation.y) * this->scale;
	rectangle.width *= this->scale;
	rectangle.height *= this->scale;

}
RectangleF Camera::calculate_view_rectangle(
	const RectangleF& world_rectangle,
	const Camera& camera)
{
	return RectangleF((world_rectangle.x - camera.translation.x) * camera.scale,
		(world_rectangle.y - camera.translation.y) * camera.scale,
		world_rectangle.width * camera.scale,
		world_rectangle.height * camera.scale);
}
void Camera::calculate_view_rectangle(
	const RectangleF& world_rectangle,
	const Camera& camera,
	RectangleF& view_rectangle)
{
	view_rectangle.x = (world_rectangle.x - camera.translation.x) * camera.scale;
	view_rectangle.y = (world_rectangle.y - camera.translation.y) * camera.scale;
	view_rectangle.width = world_rectangle.width * camera.scale;
	view_rectangle.height = world_rectangle.height * camera.scale;
}
Camera Camera::calculate_intermediate_camera(const Camera& first, const Camera& last, float amount)
{
	return Camera(Vector2F::lerp(first.translation, last.translation, amount),
		lerp(first.scale, last.scale, amount));
}
Camera Camera::calculate_camera_from_view_rectangle(
	const RectangleF& view_rectangle,
	const RectangleF& world_rectangle)
{
	//return Camera(world_rectangle.x - view_rectangle.x,
	//	world_rectangle.y - view_rectangle.y,
	//	world_rectangle.width / view_rectangle.width);


	return Camera(view_rectangle.x - world_rectangle.x,
		view_rectangle.y - world_rectangle.y,
		world_rectangle.width / view_rectangle.width);

}
Vector2F Camera::calculate_view_position(
	const Vector2F& world_position) const
{
	return Vector2F((world_position.x - this->translation.x) * this->scale,
		(world_position.y - this->translation.y) * this->scale);
}
float Camera::calculate_view_scale(float world_scale) const
{
	return world_scale * this->scale;
}
const Camera Camera::DEFAULT_CAMERA = { Vector2F::ZERO, 1.0f };

#pragma endregion Camera