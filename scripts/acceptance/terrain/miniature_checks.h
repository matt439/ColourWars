#pragma once

// The miniature projection checks: quantized silhouettes, ruler ticks,
// exposed faces, set-square occupancy and camera refusals. Included by
// terrain.cpp after its shared helpers (ordinary_projection among them).
namespace
{
	void same_quads(const std::vector<TerrainQuad>& actual, const std::vector<TerrainQuad>& expected,
		const std::string& where)
	{
		require(actual.size() == expected.size(), where + ": quad count changed");
		for (size_t index = 0; index < actual.size(); ++index)
		{
			require(actual[index].role == expected[index].role &&
				actual[index].rectangle == expected[index].rectangle, where + ": quad order or geometry changed");
		}
	}

	TerrainRole pixel_role(const std::vector<TerrainQuad>& quads, int x, int y)
	{
		TerrainRole role = TerrainRole::count; // A clear pixel, outside all quads.
		for (const TerrainQuad& quad : quads)
		{
			const RectangleF& r = quad.rectangle;
			if (static_cast<float>(x) >= r.left() && static_cast<float>(x) < r.right() &&
				static_cast<float>(y) >= r.top() && static_cast<float>(y) < r.bottom())
			{
				role = quad.role;
			}
		}
		return role;
	}

	void check_miniature_projection(const TerrainMaterial& material, const Shape& shape,
		const std::string& where)
	{
		const auto authored = material.quads();
		const Camera cameras[]{ Camera(-13.25f, 9.75f, 0.04f),
			Camera(53.75f, -17.25f, 0.125f), Camera(-1.5f, 0.75f, 0.25f),
			Camera(7.25f, -4.5f, 0.7f), Camera(0.0f, 0.0f, 1.0f) };
		for (const Camera& camera : cameras)
		{
			const auto projected = material.projected_miniature_quads(camera);
			same_quads(projected, material.projected_miniature_quads(camera),
				where + ": repeated miniature projection");
			const RectangleF limit = ordinary_projection(shape.bounding_box(), camera);
			for (const TerrainQuad& quad : projected)
			{
				const RectangleF& r = quad.rectangle;
				require(std::isfinite(r.x) && std::isfinite(r.y) &&
					std::isfinite(r.width) && std::isfinite(r.height) &&
					r.width > 0.0f && r.height > 0.0f && inside(r, limit),
					where + ": miniature quad escaped its quantized collision bounds");
				require(r.x == std::trunc(r.x) && r.y == std::trunc(r.y) &&
					r.width == std::trunc(r.width) && r.height == std::trunc(r.height),
					where + ": miniature geometry is not in whole view pixels");
			}
			if (camera.scale >= 0.25f)
			{
				same_quads(projected, ordinary_projection(material, camera),
					where + ": native-size material changed");
			}
		}
		same_quads(material.quads(), authored, where + ": miniature projection mutated the authored art");
	}

	TerrainFaces all_faces(const RectangleF& rectangle)
	{
		return { { { rectangle.left(), rectangle.right() } },
			{ { rectangle.left(), rectangle.right() } },
			{ { rectangle.top(), rectangle.bottom() } },
			{ { rectangle.top(), rectangle.bottom() } } };
	}

	void check_miniature_rulers(World& world)
	{
		// Known 320-by-40 fixtures exercise both orientations. A one-pixel
		// ruler keeps its silhouette; two pixels retain the sealed substrate
		// below one keyline, while larger rulers leave a readable light body.
		for (bool vertical : { false, true })
		{
			const RectangleF box(0.0f, 0.0f, vertical ? 40.0f : 320.0f,
				vertical ? 320.0f : 40.0f);
			const RulerMaterial ruler(box, all_faces(box), &world.render);
			check_miniature_projection(ruler, box, "known miniature ruler");
			const auto at = [&](const std::vector<TerrainQuad>& quads, int along, int across)
			{
				return pixel_role(quads, vertical ? across : along, vertical ? along : across);
			};
			const auto thin = ruler.projected_miniature_quads(Camera(0.0f, 0.0f, 0.04f));
			for (int along = 0; along < 12; ++along)
			{
				require(at(thin, along, 0) == TerrainRole::ruler_fill,
					"one-pixel miniature ruler became a black bar or lost its light silhouette");
			}
			const auto two = ruler.projected_miniature_quads(Camera(0.0f, 0.0f, 0.05f));
			for (int along = 1; along < 15; ++along)
			{
				require(at(two, along, 0) == TerrainRole::keyline &&
					at(two, along, 1) == TerrainRole::metal_dark,
					"two-pixel miniature ruler lost its leading edge or sealed substrate");
			}
			const auto marked = ruler.projected_miniature_quads(Camera(0.0f, 0.0f, 0.1f));
			for (int along : { 8, 16, 24 })
			{
				require(at(marked, along, 0) == TerrainRole::keyline &&
					at(marked, along, 1) == TerrainRole::ruler_fill &&
					at(marked, along, 3) == TerrainRole::keyline,
					"miniature sealed edges crowded out the ruler interior or lost their silhouette");
			}
		}
	}

	void check_miniature_faces(World& world)
	{
		const RectangleF box(0.0f, 0.0f, 96.0f, 32.0f);
		TerrainFaces partial;
		partial.top = { { 16.0f, 48.0f } };
		const GroundMaterial ledge(box, partial, &world.render);
		const auto projected = ledge.projected_miniature_quads(Camera(0.0f, 0.0f, 0.1f));
		for (int x = 0; x < 9; ++x)
		{
			require(pixel_role(projected, x, 0) ==
				(x >= 1 && x < 4 ? TerrainRole::keyline : TerrainRole::ground_fill),
				"miniature exposed edge disappeared or grew beyond its authored span");
			require(pixel_role(projected, x, 1) == TerrainRole::ground_fill,
				"miniature edge widened instead of remaining one inward pixel");
		}

		// Two touching pieces expose only their outer ends. Sampling immediately
		// on either side of the shared face must find fill, not a new seam.
		const RectangleF touching_box(0.0f, 0.0f, 96.0f, 96.0f);
		TerrainFaces left = all_faces(touching_box);
		left.right.clear();
		const RectangleF neighbour(96.0f, 0.0f, 96.0f, 96.0f);
		TerrainFaces right = all_faces(neighbour);
		right.left.clear();
		const GroundMaterial first(touching_box, left, &world.render);
		const GroundMaterial second(neighbour, right, &world.render);
		const Camera camera(0.0f, 0.0f, 0.125f);
		const auto a = first.projected_miniature_quads(camera);
		const auto b = second.projected_miniature_quads(camera);
		for (int y : { 2, 6, 9 })
		{
			require(pixel_role(a, 11, y) == TerrainRole::ground_fill &&
				pixel_role(b, 12, y) == TerrainRole::ground_fill,
				"miniature outlined an unexposed shared face");
		}
	}

	void check_miniature_ramps(World& world)
	{
		// Both walking slopes, plus negative world coordinates and fractional
		// camera origins. The unchanged authored raster is an occupancy oracle;
		// comparing all pixels also catches holes hidden by a bounding-box test.
		const TriangleRightAxisAligned ramps[]{
			{ 0.0f, 128.0f, 256.0f, 128.0f, 256.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 128.0f, 256.0f, 128.0f },
			{ -256.0f, -64.0f, 0.0f, -64.0f, 0.0f, -192.0f } };
		for (const TriangleRightAxisAligned& triangle : ramps)
		{
			const SetSquareMaterial ramp(triangle, all_faces(triangle.bounding_box()), &world.render);
			check_miniature_projection(ramp, triangle, "known miniature set square");
			for (const Camera& camera : { Camera(0.0f, 0.0f, 0.1f),
				Camera(13.25f, -7.75f, 0.04f), Camera(-7.5f, 5.25f, 0.125f) })
			{
				const auto projected = ramp.projected_miniature_quads(camera);
				const auto original = ordinary_projection(ramp, camera);
				const RectangleF limit = ordinary_projection(triangle.bounding_box(), camera);
				size_t light = 0;
				size_t ink = 0;
				for (int y = static_cast<int>(limit.top()) - 1;
					y <= static_cast<int>(limit.bottom()); ++y)
				{
					for (int x = static_cast<int>(limit.left()) - 1;
						x <= static_cast<int>(limit.right()); ++x)
					{
						const TerrainRole actual = pixel_role(projected, x, y);
						require((actual == TerrainRole::count) ==
							(pixel_role(original, x, y) == TerrainRole::count),
							"miniature set square grew beyond its stepped triangle or lost filled pixels");
						light += actual == TerrainRole::metal_light ? 1 : 0;
						ink += actual == TerrainRole::keyline ? 1 : 0;
					}
				}
				require(light > 0 && ink > 0,
					"miniature set square lost its light body or resolvable walking edge");
			}
		}
	}

	void check_miniature_rejections(World& world)
	{
		const RectangleF box(0.0f, 0.0f, 320.0f, 40.0f);
		const RulerMaterial ruler(box, all_faces(box), &world.render);
		const auto authored = ruler.quads();
		const float infinity = std::numeric_limits<float>::infinity();
		const float nan = std::numeric_limits<float>::quiet_NaN();
		const Camera invalid[]{ Camera(0.0f, 0.0f, 0.0f), Camera(0.0f, 0.0f, -0.1f),
			Camera(0.0f, 0.0f, infinity), Camera(0.0f, 0.0f, nan),
			Camera(infinity, 0.0f, 0.1f), Camera(0.0f, -infinity, 0.1f),
			Camera(nan, 0.0f, 1.0f), Camera(0.0f, nan, 1.0f) };
		for (const Camera& camera : invalid)
		{
			bool refused = false;
			try { ruler.projected_miniature_quads(camera); }
			catch (const std::invalid_argument&) { refused = true; }
			require(refused, "miniature accepted a nonpositive/nonfinite camera");
		}
		same_quads(ruler.quads(), authored, "invalid miniature camera mutated the authored art");
	}
}
