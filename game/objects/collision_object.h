#pragma once

#include "engine/core/game_object.h"
#include "game/objects/collision_object_type.h"
#include "engine/math/matt_math.h"

class CollisionObject : public artattack::GameObject
{
public:
	~CollisionObject() override = default;
	virtual bool is_colliding(const CollisionObject* other) const = 0;
	virtual void on_collision(const CollisionObject* other) = 0;
	virtual CollisionObjectType collision_object_type() const = 0;
	virtual const mattmath::Shape* shape() const = 0;
	virtual bool for_deletion() const = 0;

	// Requests removal at the end of the frame. Objects that are part of the
	// level's fixed geometry (structures, paint tiles) cannot be removed and
	// ignore this by default.
	virtual void set_for_deletion(bool /*for_deletion*/) {}
};
