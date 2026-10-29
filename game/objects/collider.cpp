#include "game/pch.h"
#include "game/objects/collider.h"

#include <stdexcept>

using namespace mattmath;
using namespace labrador;

Collider& Collider::of(CollisionObject& object)
{
	if (auto* collider = dynamic_cast<Collider*>(&object)) { return *collider; }
	throw std::logic_error("A ColourWars scene holds a collision object that is not a Collider.");
}

const Collider& Collider::of(const CollisionObject& object)
{
	if (const auto* collider = dynamic_cast<const Collider*>(&object)) { return *collider; }
	throw std::logic_error("A ColourWars scene holds a collision object that is not a Collider.");
}

void Collider::on_contact(const CollisionObject& other,
	const Vector2F& normal, float penetration)
{
	// touch is const on the other party: it only names its class, and every
	// response belongs to this object's handler.
	Collider::of(other).touch(*this, normal, penetration);
}

void Collider::on_structure_contact(const Structure& /*other*/,
	const Vector2F& /*normal*/, float /*penetration*/)
{
}

void Collider::on_player_contact(const Player& /*other*/,
	const Vector2F& /*normal*/, float /*penetration*/)
{
}

void Collider::on_projectile_contact(const Projectile& /*other*/,
	const Vector2F& /*normal*/, float /*penetration*/)
{
}
