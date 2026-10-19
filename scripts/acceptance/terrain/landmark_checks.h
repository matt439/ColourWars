#pragma once

// The landmark checks: the brush pot and tape roll atlas frames and their
// static placement on every shipped arena. Included by terrain.cpp after
// its shared helpers (ordinary_projection among them).
namespace
{
	void check_landmark_atlas(World& world, const std::filesystem::path& content)
	{
		const SpriteSheet* sheet = world.render.sprite_sheet("terrain");
		require(sheet != nullptr, "The landmark terrain sheet was not loaded");
		const auto brush = sheet->resolve_sprite_frame("brush_pot");
		const auto tape = sheet->resolve_sprite_frame("tape_roll");
		require(brush != tape && sheet->sprite_frame(brush).source_rectangle() == RectangleI(4, 596, 256, 320) &&
			sheet->sprite_frame(tape).source_rectangle() == RectangleI(264, 596, 256, 160) &&
			sheet->sprite_frame(brush).origin() == Vector2F::ZERO && sheet->sprite_frame(tape).origin() == Vector2F::ZERO,
			"Landmarks did not resolve to their distinct native-size, zero-origin atlas frames");

		// Read the actual staged runtime DDS through the CPU asset reader; no
		// texture device or unstaged working-image file is needed.
		const TextureData image = read_dds_file((content / "textures/terrain.dds").string());
		require(image.width == 2048 && image.height == 920 && image.format == TextureFormat::r8g8b8a8_unorm &&
			!image.levels.empty() && image.levels.front().width == 2048 && image.levels.front().height == 920 && !image.pixels.empty(),
			"The landmark frames require the complete staged 2048x920 RGBA terrain atlas");
	}

	void check_shipped_landmarks(World& world, const LevelObjectBuilder& builder,
		const JsonValue& decorations, const std::string& stage)
	{
		struct Expected
		{
			const char* stage;
			Vector2F brush;
			Vector2F tape;
			size_t index;
			size_t count;
			const char* previous;
		};
		const Expected expected[]{
			{"movement_course", {1088.0f, 680.0f}, {5040.0f, 840.0f}, 15, 22, "hill_6"},
			{"king_of_the_hill", {2376.0f, 4356.0f}, {3368.0f, 4516.0f}, 20, 22, "castle_background"},
			{"turbulence", {1488.0f, 5016.0f}, {4256.0f, 5176.0f}, 17, 24, "hill_7"},
			{"close_quarters", {1792.0f, 4632.0f}, {3952.0f, 4792.0f}, 17, 24, "hill_7"}
		};
		const auto found = std::find_if(std::begin(expected), std::end(expected),
			[&](const Expected& entry) { return stage == entry.stage; });
		if (found == std::end(expected)) { return; }
		require(decorations.size() == found->count, stage + ": missing, duplicate or unexpected backdrop decorations");
		require(decorations.at(found->index - 1).string("name") == found->previous,
			stage + ": landmarks lost their preceding backdrop layer");
		if (found->index + 2 < decorations.size())
		{
			require(decorations.at(found->index + 2).string("name") == "house",
				stage + ": landmarks must precede the house and foreground trees");
		}

		std::srand(9127);
		const int expected_random = std::rand();
		std::srand(9127);
		world.preferences.reduce_effects = false;
		const auto objects = builder.build_non_collision_objects(decorations);
		world.preferences.reduce_effects = true;
		const auto restarted = builder.build_non_collision_objects(decorations);
		require(objects.size() == decorations.size() && restarted.size() == decorations.size(),
			stage + ": landmark construction changed the decoration count");
		std::array<bool, 2> seen{};
		for (size_t index = 0; index < decorations.size(); ++index)
		{
			const JsonValue json = decorations.at(index);
			const std::string name = json.string("name");
			const std::string frame_name = json.string("frame_name");
			const bool brush = name == "brush_pot";
			const bool tape = name == "tape_roll";
			if (!brush && !tape && frame_name != "brush_pot" && frame_name != "tape_roll") { continue; }
			const size_t kind = brush ? 0 : 1;
			const std::string where = stage + " " + name;
			require((brush || tape) && !seen[kind] && frame_name == name && index == found->index + kind,
				where + ": landmark name, frame, count or painter order changed");
			seen[kind] = true;
			require(json.string("type") == "Visual" && json.string("sheet_name") == "terrain" &&
				json.string("colour") == "WHITE" && !json.has("drift"),
				where + ": a static landmark gained motion, a tint or another resource type");
			const RectangleF anchor(brush ? found->brush : found->tape, Vector2F(256.0f, brush ? 320.0f : 160.0f));
			same_rectangle(authored_rectangle(json), anchor, where + ": native-size landmark placement changed");
			auto* visual = dynamic_cast<Visual*>(objects[index].get());
			auto* again = dynamic_cast<Visual*>(restarted[index].get());
			require(visual != nullptr && again != nullptr && dynamic_cast<DriftingVisual*>(objects[index].get()) == nullptr,
				where + ": the builder did not retain an ordinary static Visual");
			same_rectangle(visual->bounds(), anchor, where + ": frame origin, scale or initial pose changed");
			same_rectangle(again->bounds(), anchor, where + ": restarting with Reduce effects changed the pose");
			same_rectangle(visual->cull_bounds(0.0f), anchor, where + ": a static landmark acquired a drift envelope");
			const float scales[]{0.04f, 0.125f, 0.5f, 1.0f, 2.0f};
			std::array<RectangleF, std::size(scales)> culls{};
			for (size_t zoom = 0; zoom < std::size(scales); ++zoom)
			{
				const float scale = scales[zoom];
				const RectangleF cull = visual->cull_bounds(1.0f / scale);
				culls[zoom] = cull;
				const Camera camera(anchor.x - 13.375f, anchor.y + 7.625f, scale);
				const RectangleF projected = ordinary_projection(anchor, camera);
				require(inside(anchor, cull) && inside(projected, camera.calculate_view_rectangle(cull)),
					where + ": culling lost native geometry or a fractionally translated view-pixel edge");
				require(culls[zoom].left() < anchor.left() && culls[zoom].right() > anchor.right() &&
					culls[zoom].top() < anchor.top() && culls[zoom].bottom() > anchor.bottom(),
					where + ": culling omitted view-pixel quantization padding");
			}
			for (bool reduced : {false, true, false})
			{
				world.preferences.reduce_effects = reduced;
				for (float dt : {0.0f, 1.0f / 60.0f, 1.0f, 60.0f, 3600.0f})
				{
					visual->update(dt);
					again->update(dt * 2.0f);
					require(visual->bounds() == anchor && again->bounds() == anchor,
						where + ": time, restart or Reduce effects changed the exact static pose");
					for (size_t zoom = 0; zoom < std::size(scales); ++zoom)
					{
						require(visual->cull_bounds(1.0f / scales[zoom]) == culls[zoom] &&
							again->cull_bounds(1.0f / scales[zoom]) == culls[zoom],
							where + ": time, restart or reduction changed the exact cull extent");
					}
				}
			}
		}
		require(seen[0] && seen[1], stage + ": each arena needs exactly one brush pot and tape roll");
		require(std::rand() == expected_random, stage + ": static landmark construction/update consumed gameplay randomness");
		world.preferences.reduce_effects = false;
	}
}
