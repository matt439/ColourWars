#pragma once

// Materials remain anchored while their owner-bound shadow wrappers publish
// world bounds. These checks inspect the actual authored initial poses; Level
// advancement and repeated native drawing are covered by the mover probes.
namespace
{
	void check_moving_shadows(World& world, const LevelObjectBuilder& builder,
		const JsonValue& collision_json,
		const std::vector<std::unique_ptr<CollisionObject>>& objects, const std::string& stage)
	{
		std::vector<const TranslatingStructurePaintable*> casters;
		for (const auto& object : objects)
		{
			const auto* mover = dynamic_cast<const TranslatingStructurePaintable*>(object.get());
			if (mover == nullptr) { continue; }
			const RectangleF anchor = mover->motion_state().spec.anchor;
			if (anchor.width > anchor.height && anchor.height < 200.0f) { casters.push_back(mover); }
		}
		// The constructed collision owners above outlive these borrowed wrappers.
		const auto shadows = builder.build_moving_shadows(collision_json, objects);
		require(shadows.size() == casters.size(), stage + ": moving shadow/caster count differs");
		if (stage == "turbulence_mover_trial" || stage == "turbulence_ferry_trial")
		{ require(casters.size() == 2, stage + ": comparison lost its paired moving shadow owners"); }
		for (size_t index = 0; index < shadows.size(); ++index)
		{
			const auto* shadow = dynamic_cast<const TranslatedAirbrushShadow*>(shadows[index].get());
			require(shadow != nullptr, stage + ": moving shadow is not an owner-following wrapper");
			const auto& owner = *casters[index];
			const RectangleF anchor = owner.motion_state().spec.anchor;
			const float offset = owner.translation().x;
			const auto translated = [offset](RectangleF rectangle)
			{
				if (offset != 0.0f) { rectangle.offset(offset, 0.0f); }
				return rectangle;
			};
			const AirbrushShadow local(anchor.left(), anchor.right(), anchor.bottom(), &world.render);
			const RectangleF bounds = shadow->bounds();
			const RectangleF collider = owner.shape()->bounding_box();
			same_rectangle(collider, translated(anchor), stage + ": mover collider lost its anchor translation");
			same_rectangle(bounds, translated(local.bounds()), stage + ": moving shadow lost its owner translation");
			require(bounds.width > 0.0f && bounds.height > 0.0f &&
				std::abs(collider.bottom() - bounds.top()) < EPSILON &&
				bounds.left() >= collider.left() - EPSILON && bounds.right() <= collider.right() + EPSILON,
				stage + ": moving shadow is not under its own world underside");
			for (const float units_per_pixel : { 0.5f, 1.0f, 2.0f })
			{
				const RectangleF cull = shadow->cull_bounds(units_per_pixel);
				same_rectangle(cull, translated(local.cull_bounds(units_per_pixel)),
					stage + ": moving shadow cull bounds lost their owner translation");
				require(inside(bounds, cull), stage + ": moving shadow escapes its world cull extent");
			}
		}
	}
}
