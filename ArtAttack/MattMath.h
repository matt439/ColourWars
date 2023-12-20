#ifndef MATTMATH_H
#define MATTMATH_H

#include "SimpleMath.h"
#include <cmath>
#include "shape_type.h"

namespace MattMath
{
	enum class direction
	{
		NONE,
		UP,
		DOWN,
		LEFT,
		RIGHT,
		UP_LEFT,
		UP_RIGHT,
		DOWN_LEFT,
		DOWN_RIGHT
	};
	
	struct Vector2I;
	struct Vector2F;
	typedef Vector2F Point2F;
	struct RectangleI;
	struct Circle;
	struct Triangle;
	struct Segment;
	struct RectangleF;
	struct Camera;

	constexpr float PI = 3.14159265358979323846f;

	float min(float a, float b);
	float max(float a, float b);

	float clamp(float value, float min, float max);
	//void clamp(float& value, float min, float max);
	int clamp(int value, int min, int max);
	//void clamp(int& value, int min, int max);

	int sign(const MattMath::Vector2F& p1,
		const MattMath::Vector2F& p2, const MattMath::Vector2F& p3);

	bool are_equal(float a, float b, float epsilon = 0.0001f);

	float to_radians(float degrees);
	float to_degrees(float radians);

	float lerp(float a, float b, float t);
	
	struct Shape
	{
		virtual ~Shape() = default;
		virtual const MattMath::RectangleF& get_bounding_box() const = 0;
		virtual shape_type get_shape_type() const = 0;
		virtual bool intersects(const Shape* other) const;
		virtual bool intersects(const Shape& other) const;
		virtual bool intersects(const MattMath::Circle& other) const;
		virtual bool intersects(const MattMath::Triangle& other) const;
		virtual bool intersects(const MattMath::RectangleF& other) const;
		virtual bool AABB_intersects(const Shape* other) const;
	};

	struct RectangleF : public Shape
	{
		float x = 0.0f;
		float y = 0.0f;
		float width = 0.0f;
		float height = 0.0f;

		RectangleF() = default;
		RectangleF(const RectangleF&) = default;
		RectangleF(float x, float y, float width, float height);
		RectangleF(const MattMath::Vector2F& position,
							const MattMath::Vector2F& size);
		RectangleF(const DirectX::SimpleMath::Vector2& position,
						const DirectX::SimpleMath::Vector2& size);
		RectangleF(const DirectX::SimpleMath::Rectangle& rectangle);
		RectangleF(const RECT& rectangle);

		const RectangleF& get_bounding_box() const override;
		shape_type get_shape_type() const override;

		float get_x() const;
		float get_y() const;
		float get_width() const;
		float get_height() const;
		MattMath::Vector2F get_center() const;
		MattMath::Vector2F get_position() const;
		MattMath::Vector2F get_size() const;
		float get_left() const;
		float get_right() const;
		float get_top() const;
		float get_bottom() const;
		MattMath::Vector2F get_top_left() const;
		MattMath::Vector2F get_bottom_right() const;
		MattMath::Vector2F get_top_right() const;
		MattMath::Vector2F get_bottom_left() const;
		MattMath::Segment get_top_edge() const;
		MattMath::Segment get_bottom_edge() const;
		MattMath::Segment get_left_edge() const;
		MattMath::Segment get_right_edge() const;
		float get_area() const;
		MattMath::RectangleI get_rectangle_i() const;
		DirectX::SimpleMath::Rectangle get_sm_rectangle() const;
		RECT get_win_rect() const;

		//RectangleF& operator=(const DirectX::SimpleMath::Rectangle& rectangle);
		//RectangleF& operator=(const RECT& rectangle);
		bool operator==(const RectangleF& other) const;
		bool operator!=(const RectangleF& other) const;

		bool contains(const MattMath::Vector2F& point) const;
		bool contains(const RectangleF& other) const;

		bool intersects(const RectangleF& other) const;
		bool intersects(const MattMath::Circle& other) const;
		bool intersects(const MattMath::Triangle& other) const;
		bool intersects(const MattMath::Segment& other) const;
		RectangleF intersection(const RectangleF& other) const;
		
		//void set_left(float left);
		//void set_top(float top);
		//void set_right(float right);
		//void set_bottom(float bottom);

		void inflate(float horizontal_amount, float vertical_amount);
		void inflate(const MattMath::Vector2F& amount);
		void inflate_to_size(float width, float height);
		void inflate_to_size(const MattMath::Vector2F& size);
		void scale_at_center(float scale);
		void scale_at_center(float horizontal_scale, float vertical_scale);
		void scale_at_center(const MattMath::Vector2F& scale);
		void offset(float horizontal_amount, float vertical_amount);
		void offset(const MattMath::Vector2F& amount);
		void scale(float horizontal_amount, float vertical_amount);
		void scale(const MattMath::Vector2F& amount);
		void scale_size_and_position(float horizontal_amount,
			float vertical_amount);
		void scale_size_and_position(const MattMath::Vector2F& amount);
		void set_position(const MattMath::Vector2F& position);
		void set_position(float x, float y);
		void set_position_at_center(const MattMath::Vector2F& position);
		void set_position_at_center(float x, float y);
		void set_position_x(float x);
		void set_position_x_from_right(float x);
		void set_position_y(float y);
		void set_position_y_from_bottom(float y);
		void set_position_from_top_right(const MattMath::Vector2F& position);
		void set_position_from_top_right(float x, float y);
		void set_position_from_bottom_left(const MattMath::Vector2F& position);
		void set_position_from_bottom_left(float x, float y);
		void set_position_from_bottom_right(const MattMath::Vector2F& position);
		void set_position_from_bottom_right(float x, float y);
		void set_size(const MattMath::Vector2F& size);
		void set_size(float width, float height);
		void set_width(float width);
		void set_height(float height);

		static RectangleF intersection(const RectangleF& a, const RectangleF& b);
		static RectangleF union_of(const RectangleF& a, const RectangleF& b);

		static const RectangleF ZERO;
	};

	typedef MattMath::RectangleF AABB;

	struct Vector2I
	{
		int x = 0;
		int y = 0;

		Vector2I() = default;
		Vector2I(const Vector2I&) = default;
		Vector2I(int x, int y);
		Vector2I(const MattMath::Vector2F& vector);
		Vector2I(const DirectX::SimpleMath::Vector2& vector);
		Vector2I(const DirectX::XMINT2& vector);

		DirectX::SimpleMath::Vector2 get_sm_vector() const;
		DirectX::XMINT2 get_xm_vector() const;

		Vector2I& operator=(const DirectX::SimpleMath::Vector2& vector);
		Vector2I& operator=(const DirectX::XMINT2& vector);
		bool operator==(const Vector2I& other) const;
		bool operator!=(const Vector2I& other) const;

		Vector2I& operator+=(const Vector2I& other);
		Vector2I& operator-=(const Vector2I& other);
		Vector2I& operator*=(const Vector2I& other);
		Vector2I& operator/=(const Vector2I& other);
		Vector2I& operator*=(int other);
		Vector2I& operator/=(int other);

		//Vector2I operator+(const Vector2I& other) const;
		//Vector2I operator-(const Vector2I& other) const;

		void offset(int horizontal_amount, int vertical_amount);
		void scale(int horizontal_amount, int vertical_amount);
		void scale(const DirectX::SimpleMath::Vector2& amount);
		void set(int x, int y);
		void set(const DirectX::SimpleMath::Vector2& vector);
		void set(const DirectX::XMINT2& vector);

		static const Vector2I ZERO;
	};

	Vector2I operator+ (const Vector2I& V1, const Vector2I& V2);
	Vector2I operator- (const Vector2I& V1, const Vector2I& V2);
	Vector2I operator* (const Vector2I& V1, const Vector2I& V2);	
	Vector2I operator* (const Vector2I& V, int S);
	Vector2I operator/ (const Vector2I& V1, const Vector2I& V2);
	Vector2I operator/ (const Vector2I& V, int S);
	Vector2I operator* (int S, const Vector2I& V);


	struct Vector2F
	{
		float x = 0.0f;
		float y = 0.0f;

		Vector2F() = default;
		Vector2F(const Vector2F&) = default;
		Vector2F(float f);
		Vector2F(float x, float y);
		Vector2F(const DirectX::SimpleMath::Vector2& vector);
		Vector2F(const DirectX::XMFLOAT2& vector);
		Vector2F(const MattMath::Vector2I vector);

		DirectX::SimpleMath::Vector2 get_sm_vector() const;
		DirectX::XMFLOAT2 get_xm_vector() const;

		Vector2F& operator=(const DirectX::SimpleMath::Vector2& vector);
		Vector2F& operator=(const DirectX::XMFLOAT2& vector);
		bool operator==(const Vector2F& other) const;
		bool operator!=(const Vector2F& other) const;

		Vector2F& operator+=(const Vector2F& other);
		Vector2F& operator-=(const Vector2F& other);
		Vector2F& operator*=(const Vector2F& other);
		Vector2F& operator/=(const Vector2F& other);
		Vector2F& operator*=(float other);
		Vector2F& operator/=(float other);

		//Vector2F& operator/(float other);
		//Vector2F& operator*(float other);

		//Vector2F& operator+(const Vector2F& other);
		//Vector2F& operator-(const Vector2F& other);
		//Vector2F& operator*(const Vector2F& other);
		//Vector2F& operator/(const Vector2F& other);

		float length() const;
		float length_squared() const;
		MattMath::direction get_direction() const;

		float dot(const Vector2F& other) const;
		Vector2F cross(const Vector2F& other) const;
		
		Vector2F normalized() const;
		void normalize();

		bool is_contained_within(const MattMath::RectangleF& other) const;

		void clamp(const Vector2F& min, const Vector2F& max);
		Vector2F clamped(const Vector2F& min, const Vector2F& max) const;

		float angle() const;
		static float angle(const Vector2F& vec);

		void rotate(float angle);

		static Vector2F rotate_vector(const Vector2F& vec, float angle);
		static void rotate_vector_by_ref(Vector2F& vec, float angle);

		static Vector2F lerp(const Vector2F& a, const Vector2F& b, float t);
		static Vector2F lerp(const Vector2F& a, const Vector2F& b, const Vector2F& t);

		static float distance(const Vector2F& a, const Vector2F& b);
		static float distance_squared(const Vector2F& a, const Vector2F& b);

		static float dot(const Vector2F& a, const Vector2F& b);

		static Vector2F min(const Vector2F& a, const Vector2F& b);
		static Vector2F max(const Vector2F& a, const Vector2F& b);

		static Vector2F vec_from_angle_magnitude(float angle, float magnitude);
		static Vector2F unit_vec_from_angle(float angle);

		static const Vector2F ZERO;
		static const Vector2F ONE;
		static const Vector2F UNIT_X;
		static const Vector2F UNIT_Y;
	};

	Vector2F operator+ (const Vector2F& V1, const Vector2F& V2);
	Vector2F operator- (const Vector2F& V1, const Vector2F& V2);
	Vector2F operator* (const Vector2F& V1, const Vector2F& V2);
	Vector2F operator* (const Vector2F& V, float S);
	Vector2F operator/ (const Vector2F& V1, const Vector2F& V2);
	Vector2F operator/ (const Vector2F& V, float S);
	Vector2F operator* (float S, const Vector2F& V);

	struct RectangleI
	{
		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;

		RectangleI() = default;
		RectangleI(const RectangleI&) = default;
		RectangleI(int x, int y, int width, int height);
		RectangleI(const MattMath::Vector2I& position,
			const MattMath::Vector2I& size);
		RectangleI(const MattMath::Vector2F& position,
			const MattMath::Vector2F& size);
		RectangleI(const DirectX::SimpleMath::Vector2& position,
			const DirectX::SimpleMath::Vector2& size);
		RectangleI(MattMath::RectangleF rectangle);
		RectangleI(const DirectX::SimpleMath::Rectangle& rectangle);
		RectangleI(const RECT& rectangle);

		int get_left() const;
		int get_top() const;
		int get_right() const;
		int get_bottom() const;

		MattMath::Vector2I get_position() const;
		MattMath::Vector2I get_size() const;

		MattMath::Vector2I get_top_left() const;
		MattMath::Vector2I get_bottom_right() const;

		DirectX::SimpleMath::Rectangle get_sm_rectangle() const;
		RECT get_win_rect() const;

		bool operator==(const RectangleI& other) const;
		bool operator!=(const RectangleI& other) const;

		bool contains(const MattMath::Vector2I& point) const;
		bool contains(const RectangleI& other) const;

		void offset(int horizontal_amount, int vertical_amount);
		void offset(const MattMath::Vector2I& amount);

		void set_left(int left);
		void set_top(int top);
		void set_right(int right);
		void set_bottom(int bottom);

		void set_position(const MattMath::Vector2I& position);
		void set_size(const MattMath::Vector2I& size);

		void set_top_left(const MattMath::Vector2I& top_left);
		void set_bottom_right(const MattMath::Vector2I& bottom_right);
		void set_top_left_and_bottom_right(const MattMath::Vector2I& top_left,
			const MattMath::Vector2I& bottom_right);

		static const RectangleI ZERO;
	};

	struct Vector4F
	{
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float w = 0.0f;

		Vector4F() = default;
		Vector4F(const Vector4F&) = default;
		Vector4F(float x, float y, float z, float w);
		Vector4F(const DirectX::XMFLOAT4& vector);

		Vector4F& operator=(const DirectX::XMFLOAT4& vector);
		bool operator==(const Vector4F& other) const;
		bool operator!=(const Vector4F& other) const;

		DirectX::XMVECTOR get_xm_vector() const;
	};

	struct Colour
	{
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
		float a = 0.0f;
		
		Colour() = default;
		Colour(const Colour&) = default;
		Colour(float r, float g, float b);
		Colour(float r, float g, float b, float a);
		Colour(const DirectX::XMFLOAT4& vector);
		Colour(const DirectX::SimpleMath::Color& colour);
		Colour(int r, int g, int b, int a = 255);
		Colour(const std::string& hex);

		Colour& operator=(const DirectX::XMFLOAT4& vector);
		Colour& operator=(const DirectX::SimpleMath::Color& colour);
		Colour& operator=(const MattMath::Vector4F& vector);

		bool operator==(const Colour& other) const;
		bool operator!=(const Colour& other) const;

		Colour& operator+=(const Colour& other);
		Colour& operator-=(const Colour& other);
		Colour& operator*=(const Colour& other);
		Colour& operator*=(float f);
		Colour& operator/=(const Colour& other);
		Colour& operator/=(float f);

		float get_red() const;
		float get_green() const;
		float get_blue() const;
		float get_alpha() const;

		void set_red(float red);
		void set_green(float green);
		void set_blue(float blue);
		void set_alpha(float alpha);

		void set(float red, float green, float blue, float alpha = 1.0f);
		void set_from_int_rgba(int r, int g, int b, int a = 255);
		void set_from_hex(const std::string& hex);

		void saturate(float amount);
		void desaturate(float amount);

		void brighten(float amount);
		void darken(float amount);

		void invert();

		void make_opaque();
		void make_transparent();

		DirectX::SimpleMath::Color get_sm_colour() const;
		DirectX::XMVECTOR get_xm_vector() const;

		void clamp_colours();
	};

	Colour operator+ (const Colour& V1, const Colour& V2);
	Colour operator- (const Colour& V1, const Colour& V2);
	Colour operator* (const Colour& V1, const Colour& V2);
	Colour operator* (const Colour& V, float S);
	Colour operator/ (const Colour& V1, const Colour& V2);
	Colour operator/ (const Colour& V, float S);
	Colour operator* (float S, const Colour& V);

	struct Vector3F
	{
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;

		Vector3F() = default;
		Vector3F(const Vector3F&) = default;
		Vector3F(float x, float y, float z);

		bool operator==(const Vector3F& other) const;
		bool operator!=(const Vector3F& other) const;

		Vector3F& operator+=(const Vector3F& other);
		Vector3F& operator-=(const Vector3F& other);
		Vector3F& operator*=(const Vector3F& other);
		Vector3F& operator/=(const Vector3F& other);
		Vector3F& operator*=(float other);
		Vector3F& operator/=(float other);
	};

	static Vector3F operator+ (const Vector3F& V1, const Vector3F& V2);
	static Vector3F operator- (const Vector3F& V1, const Vector3F& V2);
	static Vector3F operator* (const Vector3F& V1, const Vector3F& V2);
	static Vector3F operator* (const Vector3F& V, float S);
	static Vector3F operator/ (const Vector3F& V1, const Vector3F& V2);
	static Vector3F operator/ (const Vector3F& V, float S);
	static Vector3F operator* (float S, const Vector3F& V);

	struct Viewport
	{
	public:
		float x = 0.0f;
		float y = 0.0f;
		float width = 0.0f;
		float height = 0.0f;
		float minDepth = 0.0f;
		float maxDepth = 1.0f;

		Viewport() {}
		Viewport(const Viewport&) = default;
		Viewport(float x, float y, float width, float height,
			float minDepth = 0.0f, float maxDepth = 1.0f);
		Viewport(const DirectX::SimpleMath::Viewport& viewport);
		Viewport(const MattMath::RectangleF& rectangle,
			float minDepth = 0.0f, float maxDepth = 1.0f);
		Viewport(const MattMath::RectangleI& rectangle,
			float minDepth = 0.0f, float maxDepth = 1.0f);
		Viewport(const D3D11_VIEWPORT& viewport);

		DirectX::SimpleMath::Viewport get_sm_viewport() const;
		D3D11_VIEWPORT get_d3d_viewport() const;
		const D3D11_VIEWPORT* get_d3d_viewport_ptr() const;

		MattMath::RectangleF get_rectangle() const;
		MattMath::RectangleF get_rectangle(float minDepth, float maxDepth) const;
		MattMath::Vector2F get_position() const;
		MattMath::Vector2F get_size() const;

		//Viewport& operator=(const Viewport& viewport);
		Viewport& operator=(const DirectX::SimpleMath::Viewport& viewport);
		Viewport& operator=(const D3D11_VIEWPORT& viewport);
		Viewport& operator=(const MattMath::RectangleF& rectangle);
		Viewport& operator=(const MattMath::RectangleI& rectangle);
		Viewport& operator=(const RECT& rect);
		
		bool operator==(const Viewport& other) const;
		bool operator!=(const Viewport& other) const;
	};

	struct Circle : public Shape
	{
		MattMath::Vector2F center = MattMath::Vector2F::ZERO;
		float radius = 0.0f;

		Circle() = default;
		Circle(const Circle&) = default;
		Circle(const MattMath::Vector2F& center, float radius);
		Circle(const DirectX::SimpleMath::Vector2& center, float radius);

		const MattMath::RectangleF& get_bounding_box() const override;
		shape_type get_shape_type() const override;

		bool operator==(const Circle& other) const;
		bool operator!=(const Circle& other) const;

		bool contains(const MattMath::Vector2F& point) const;

		bool intersects(const Circle& other) const;
		bool intersects(const MattMath::RectangleF& other) const;
		bool intersects(const MattMath::Triangle& other) const;
	};
	struct Triangle : public Shape
	{
		MattMath::Vector2F point1 = MattMath::Vector2F::ZERO;
		MattMath::Vector2F point2 = MattMath::Vector2F::ZERO;
		MattMath::Vector2F point3 = MattMath::Vector2F::ZERO;

		Triangle() = default;
		Triangle(const Triangle&) = default;
		Triangle(const MattMath::Vector2F& point1,
			const MattMath::Vector2F& point2,
			const MattMath::Vector2F& point3);
		Triangle(const DirectX::SimpleMath::Vector2& point1,
			const DirectX::SimpleMath::Vector2& point2,
			const DirectX::SimpleMath::Vector2& point3);

		const MattMath::RectangleF& get_bounding_box() const override;
		shape_type get_shape_type() const override;

		Segment get_segment1() const;
		Segment get_segment2() const;
		Segment get_segment3() const;

		bool operator==(const Triangle& other) const;
		bool operator!=(const Triangle& other) const;

		bool contains(const MattMath::Vector2F& point) const;

		bool intersects(const Triangle& other) const;
		bool intersects(const MattMath::RectangleF& other) const;
		bool intersects(const MattMath::Circle& other) const;
	};
	struct Segment
	{
		MattMath::Point2F point1 = MattMath::Point2F::ZERO;
		MattMath::Point2F point2 = MattMath::Point2F::ZERO;

		Segment() = default;
		Segment(const Segment&) = default;
		Segment(const MattMath::Point2F& point1,
			const MattMath::Point2F& point2);
		Segment(float x1, float y1, float x2, float y2);

		bool operator==(const Segment& other) const;
		bool operator!=(const Segment& other) const;

		bool intersects(const Segment& other) const;
		bool intersects(const MattMath::RectangleF& other) const;
		//bool intersects(const MattMath::Circle& other) const;
		//bool intersects(const MattMath::Triangle& other) const;
	};
	struct Camera
	{
		MattMath::Vector2F translation = MattMath::Vector2F::ZERO;
		float scale = 1.0f;

		Camera() = default;
		Camera(const Camera&) = default;
		Camera(const MattMath::Vector2F& translation, float scale);
		Camera(float x, float y, float scale);
		Camera(const MattMath::Viewport& viewport, float scale = 1.0f);

		bool operator==(const Camera& other) const;
		bool operator!=(const Camera& other) const;

		MattMath::RectangleF calculate_view_rectangle(
			const MattMath::RectangleF& world_rectangle) const;
		void calculate_view_rectangle(
			MattMath::RectangleF& rectangle) const;
		static MattMath::RectangleF calculate_view_rectangle(
			const MattMath::RectangleF& world_rectangle,
			const Camera& camera);
		static void calculate_view_rectangle(
			const MattMath::RectangleF& world_rectangle,
			const Camera& camera,
			MattMath::RectangleF& view_rectangle);

		static Camera calculate_intermediate_camera(
			const Camera& first, const Camera& last, float amount);

		static Camera calculate_camera_from_view_rectangle(
			const MattMath::RectangleF& view_rectangle,
			const MattMath::RectangleF& world_rectangle);

		MattMath::Vector2F calculate_view_position(
			const MattMath::Vector2F& world_position) const;
		float calculate_view_scale(float world_scale) const;

		static const Camera DEFAULT_CAMERA;
	};

	


	//struct RotatedRectangle
	//{
	//	MattMath::Point2F c; // OBB center point
	//	MattMath::Vector2F u[2]; // Local x-, y-, and z-axes
	//	MattMath::Vector2F e; // Positive halfwidth extents of OBB along each axis
	//};
}

#endif // !MATTMATH_H
