#pragma once

// Surface meaning belongs to exposed faces rather than the decorative prop.
// Native painter checks cover primer/ownership pixels; these fixtures pin the
// sealed substrate, including fractional masks and buried/shared edges.
namespace
{
	bool sealed_role(TerrainRole role)
	{
		return role == TerrainRole::metal_dark || role == TerrainRole::metal_light;
	}

	void check_surface_finish(World& world)
	{
		const RectangleF box(0.0f, 0.0f, 256.0f, 96.0f);
		const TerrainFaces faces = all_faces(box);
		const auto solid = TerrainMaterial::make("ground", CollisionObjectType::structure,
			box, faces, 0.0f, &world.render);
		const auto paintable = TerrainMaterial::make("ground", CollisionObjectType::structure_paintable,
			box, faces, 0.0f, &world.render);
		same_quads(solid->quads(), paintable->quads(),
			"ground substrate must not infer scoring faces from its collision tag");
		for (const TerrainQuad& quad : solid->quads())
		{
			require(quad.role != TerrainRole::ground_top,
				"legacy ground lip still advertises paint independently of scoring strips");
		}

		// These are material recipes used on the real arenas. Every exposed face
		// has the same dark/light finish before the scoring strips prime it.
		for (const char* name : { "ground", "pencil", "eraser", "paint_tin", "ruler" })
		{
			const CollisionObjectType type = std::string(name) == "ruler"
				? CollisionObjectType::structure_jump_through : CollisionObjectType::structure_paintable;
			const auto material = TerrainMaterial::make(name, type, box, faces, 128.0f, &world.render);
			for (int along : { 24, 72, 120, 168, 216 })
			{
				require(sealed_role(pixel_role(material->quads(), along, 6)) &&
					sealed_role(pixel_role(material->quads(), along, 90)),
					std::string(name) + ": exposed horizontal face lacks the shared sealed finish");
			}
			for (int along : { 24, 48, 72 })
			{
				require(sealed_role(pixel_role(material->quads(), 6, along)) &&
					sealed_role(pixel_role(material->quads(), 250, along)),
					std::string(name) + ": exposed vertical face lacks the shared sealed finish");
			}
		}

		// Fractional intervals emulate a top face split by touching neighbours.
		// Ground has no metal detail of its own, so every metal quad here must
		// belong entirely to one exposed strip, never bridge the buried gap.
		TerrainFaces partial;
		partial.top = { { 16.5f, 65.5f }, { 128.25f, 240.75f } };
		const GroundMaterial masked(box, partial, &world.render);
		const RectangleF allowed[]{ { 16.5f, 0.0f, 49.0f, 12.0f },
			{ 128.25f, 0.0f, 112.5f, 12.0f } };
		size_t dark = 0, hatch = 0;
		for (const TerrainQuad& quad : masked.quads())
		{
			if (!sealed_role(quad.role)) { continue; }
			require(inside(quad.rectangle, allowed[0]) || inside(quad.rectangle, allowed[1]),
				"sealed surface finish crossed an occluded span or escaped its inward edge band");
			dark += quad.role == TerrainRole::metal_dark ? 1 : 0;
			hatch += quad.role == TerrainRole::metal_light ? 1 : 0;
		}
		require(dark > 0 && hatch > 0, "sealed surface lost its dark band or sparse light hatching");
		for (int x : { 0, 16, 66, 100, 128, 241, 255 })
		{
			require(!sealed_role(pixel_role(masked.quads(), x, 6)),
				"sealed finish marks a face span hidden by another structure");
		}
		const GroundMaterial buried(box, TerrainFaces{}, &world.render);
		for (const TerrainQuad& quad : buried.quads())
		{
			require(!sealed_role(quad.role), "fully buried ground still draws an exposed surface finish");
		}
	}
}
