#include "game/pch.h"
#include "game/objects/projectile_sprinkler.h"
#include "engine/math/scalar.h"

using namespace labrador;

std::vector<std::unique_ptr<Projectile>> ProjectileSprinkler::spray(int release) const
{
	const SubTuning& sub = this->sub();
	const int count = sub.burst_count;
	const bool turned = release % 2 == 1;
	const Vector2F centre = this->bounds().center();
	std::vector<std::unique_ptr<Projectile>> drops;
	drops.reserve(static_cast<size_t>(count));
	for (int index = 0; index < count; ++index)
	{
		// Screen y grows downward, so pi to 2pi runs from straight left over
		// the top to straight right. An even spray spans both ends; a turned
		// one sits each drop in the middle of its share of the half circle.
		const float share = turned ? (static_cast<float>(index) + 0.5f) / static_cast<float>(count) :
			count > 1 ? static_cast<float>(index) / static_cast<float>(count - 1) : 0.5f;
		const float angle = pi + pi * share;
		drops.push_back(this->drop(centre, Vector2F::unit_vec_from_angle(angle) * sub.burst_speed));
	}
	return drops;
}
