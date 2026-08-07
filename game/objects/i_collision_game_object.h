#pragma once

#include "engine/core/i_game_object.h"
#include "game/objects/collision_object_type.h"
#include "engine/math/matt_math.h"

class ICollisionGameObject : public IGameObject
{
public:
	~ICollisionGameObject() override = default;
	virtual bool is_colliding(const ICollisionGameObject* other) const = 0;
	virtual void on_collision(const ICollisionGameObject* other) = 0;
	virtual CollisionObjectType get_collision_object_type() const = 0;
	virtual const mattmath::Shape* get_shape() const = 0;
	virtual bool get_for_deletion() const = 0;

	// Requests removal at the end of the frame. Objects that are part of the
	// level's fixed geometry (structures, paint tiles) cannot be removed and
	// ignore this by default.
	virtual void set_for_deletion(bool /*for_deletion*/) {}
};
