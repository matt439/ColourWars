#pragma once

#include "engine/collision/collision_object.h"
#include "engine/math/vector2f.h"

class PaintableObject;
class Player;
class Projectile;
class Structure;

// Every collision object a ColourWars scene holds: a Structure, a Player or a
// Projectile. The engine reports the other party of a contact, and hands back
// its scene's objects, only as a labrador::CollisionObject; Collider is where
// the game learns which of its own classes that is, and then asks the object
// rather than reading its collision tag and casting.
//
// A contact is dispatched twice. The engine calls on_contact on each party;
// on_contact asks the other party to touch this one, and touch calls the
// handler named for the toucher's class: on_structure_contact,
// on_player_contact or on_projectile_contact. A class answers only the
// contacts it cares about; the defaults ignore the rest.
class Collider : public labrador::CollisionObject
{
public:
	// The one conversion back from the engine's type. A ColourWars scene holds
	// nothing that is not a Collider; anything else is refused with
	// std::logic_error. Typed scene access is engine report 29.
	static Collider& of(labrador::CollisionObject& object);
	static const Collider& of(const labrador::CollisionObject& object);

	// Roles. Each answers null unless this object plays it.
	virtual Structure* structure() { return nullptr; }
	virtual const Structure* structure() const { return nullptr; }
	virtual Projectile* projectile() { return nullptr; }
	virtual const Projectile* projectile() const { return nullptr; }
	// Scored by Level, whatever the collision tag.
	virtual PaintableObject* paintable() { return nullptr; }
	virtual const PaintableObject* paintable() const { return nullptr; }

	// The engine's contact. Final: a contact is answered by the handlers below.
	void on_contact(const labrador::CollisionObject& other,
		const mattmath::Vector2F& normal, float penetration) final;

	// Second dispatch, called by the other party's touch. other is what this
	// object touched; normal points from this object towards it, as in
	// on_contact. Call on_contact rather than these.
	virtual void on_structure_contact(const Structure& other,
		const mattmath::Vector2F& normal, float penetration);
	virtual void on_player_contact(const Player& other,
		const mattmath::Vector2F& normal, float penetration);
	virtual void on_projectile_contact(const Projectile& other,
		const mattmath::Vector2F& normal, float penetration);

protected:
	// Name this object's class to receiver by calling the receiver's handler
	// for it with *this.
	virtual void touch(Collider& receiver,
		const mattmath::Vector2F& normal, float penetration) const = 0;
};
