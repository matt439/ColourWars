#include "game/pch.h"
#include "game/objects/projectile_wall.h"

using namespace labrador;

Vector2F ProjectileWall::launch_origin(const Vector2F& owner_centre) const
{
	return { owner_centre.x, this->bounds().center().y };
}

std::vector<std::unique_ptr<Projectile>> ProjectileWall::spray(int /*release*/) const
{
	const SubTuning& sub = this->sub();
	const RectangleF wall = this->bounds();
	const int count = sub.burst_count;
	// Their centres sit just above its foot, so their own size reaches the floor.
	const float y = wall.bottom() - sub.burst.details.col_rect_size.y / 2.0f;
	std::vector<std::unique_ptr<Projectile>> drops;
	drops.reserve(static_cast<size_t>(count));
	for (int index = 0; index < count; ++index)
	{
		const float share = (static_cast<float>(index) + 0.5f) / static_cast<float>(count);
		drops.push_back(this->drop(Vector2F(wall.left() + wall.width * share, y),
			Vector2F(0.0f, sub.burst_speed)));
	}
	return drops;
}
