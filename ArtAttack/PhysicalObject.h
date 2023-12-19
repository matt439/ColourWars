#ifndef PHYSICALOBJECT_H
#define PHYSICALOBJECT_H

#include "MattMath.h"
#include "IPhysical.h"

class PhysicalObject
{
private:
	MattMath::Vector2F _position = MattMath::Vector2F::ZERO;
	MattMath::Vector2F _size = MattMath::Vector2F::ZERO;
public:
	PhysicalObject() {}
	PhysicalObject(const MattMath::Vector2F& position,
		const MattMath::Vector2F& size) :
		_position(position), _size(size) {}

	virtual MattMath::Vector2F get_position() const;
	virtual float get_position_x() const;
	virtual float get_position_y() const;

	virtual void set_position(const MattMath::Vector2F& position);
	virtual void set_position_x(float x);
	virtual void set_position_y(float y);

	virtual void alter_position(const MattMath::Vector2F& position);
	virtual void alter_position_x(float x);
	virtual void alter_position_y(float y);

	virtual void set_position_from_top_right_origin(
		const MattMath::Vector2F& top_right_origin);
	virtual void set_position_from_origin(const MattMath::Vector2F& origin,
		const MattMath::Vector2F& move);

	virtual MattMath::Vector2F get_size() const;
	virtual float get_width() const;
	virtual float get_height() const;

	virtual void set_size(const MattMath::Vector2F& size);
	virtual void set_width(float x);
	virtual void set_height(float y);

	virtual void alter_size(const MattMath::Vector2F& size);
	virtual void alter_size_x(float x);
	virtual void alter_size_y(float y);

	virtual void scale_size(const MattMath::Vector2F& scale);

	//virtual const MattMath::RectangleF& get_bounding_box() const override;
	virtual MattMath::RectangleI get_bounding_box_i() const;
	//virtual DirectX::SimpleMath::Rectangle get_sm_bounding_box() const;
	//virtual RECT get_win_rect() const;
	virtual MattMath::Vector2F get_center() const;
};

#endif // !2DPHYSICALOBJECT
