#pragma once

// Each match's person has their own slips (docs/drawing-mistakes-research.md,
// section 9, decision 3): MatchRandom::hand keys the brick slips, mortar
// strokes and doubled edges. A hand moves decoration only. Collision, scoring
// faces and the material's bounds stay those of the house hand (0).
namespace
{
	bool same_drawing(const std::vector<TerrainQuad>& a, const std::vector<TerrainQuad>& b)
	{
		if (a.size() != b.size()) { return false; }
		for (size_t index = 0; index < a.size(); ++index)
		{
			if (a[index].role != b[index].role || !(a[index].rectangle == b[index].rectangle)) { return false; }
		}
		return true;
	}

	// The match's hand: a round seed's own value, the same for the same seed,
	// never the house hand, and independent of whether the persona was rolled.
	void check_match_hands()
	{
		std::vector<std::uint64_t> seen;
		for (unsigned int seed : { 0u, 1u, 2u, 7u, 1234u, 0xFFFFFFFFu })
		{
			const MatchRandom random(seed);
			const std::uint64_t hand = random.hand();
			require(hand != 0, "a match drew the house hand");
			BotRandom persona = random.persona();
			persona.next();
			require(MatchRandom(seed).hand() == hand, "a round seed's hand is not reproducible");
			require(random.hand() == hand, "rolling the persona moved the hand");
			require(hand != MatchRandom(seed).persona().next(), "the hand repeats the persona's first draw");
			require(std::find(seen.begin(), seen.end(), hand) == seen.end(), "two round seeds share a hand");
			seen.push_back(hand);
		}
	}

	// Ground on a box with room for every slip: hand 0 is the default
	// construction, one hand draws the same twice, and different hands differ.
	void check_ground_hands(World& world)
	{
		const RectangleF box(0.0f, 0.0f, 1024.0f, 320.0f);
		const TerrainFaces faces = all_faces(box);
		const GroundMaterial house(box, faces, &world.render);
		const GroundMaterial zero(box, faces, &world.render, 0);
		require(same_drawing(house.quads(), zero.quads()), "hand 0 is not the house hand");
		size_t differ = 0;
		for (std::uint64_t seed = 1; seed <= 8; ++seed)
		{
			const std::uint64_t hand = MatchRandom(static_cast<unsigned int>(seed)).hand();
			const GroundMaterial first(box, faces, &world.render, hand);
			const GroundMaterial again(box, faces, &world.render, hand);
			require(same_drawing(first.quads(), again.quads()), "one hand drew the ground two ways");
			differ += same_drawing(first.quads(), house.quads()) ? 0 : 1;
			for (const TerrainQuad& quad : first.quads())
			{
				require(inside(quad.rectangle, box), "a hand drew ground outside its rectangle");
			}
		}
		require(differ == 8, "a match's hand drew the house hand's ground");
		// Materials without slips ignore the hand.
		for (const char* name : { "pencil", "eraser", "paint_tin", "ruler" })
		{
			const CollisionObjectType type = std::string(name) == "ruler"
				? CollisionObjectType::structure_jump_through : CollisionObjectType::structure_paintable;
			const auto plain = TerrainMaterial::make(name, type, box, faces, 512.0f, &world.render);
			const auto handed = TerrainMaterial::make(name, type, box, faces, 512.0f, &world.render,
				MatchRandom(7).hand());
			require(same_drawing(plain->quads(), handed->quads()), std::string(name) + " took a hand it has no slips for");
		}
	}

	// A shipped stage drawn by a match's hand: same objects, shapes and scoring
	// as the house hand, every drawing inside its collision and within budget,
	// and at least one ground drawn differently.
	void check_stage_hands(const LevelObjectBuilder& builder, const JsonValue& collision_json,
		const std::vector<std::unique_ptr<CollisionObject>>& house, const std::string& stage)
	{
		bool ground = false;
		bool changed = false;
		for (unsigned int seed : { 7u, 8u })
		{
			const auto handed = builder.build_collision_objects(collision_json, TeamColour{},
				MatchRandom(seed).hand());
			require(handed.size() == house.size(), stage + ": a hand changed the object count");
			for (size_t index = 0; index < house.size(); ++index)
			{
				const JsonValue json = collision_json.at(index);
				const std::string where = stage + " " + (json.has("name") ? json.string("name") : std::to_string(index)) +
					" (round seed " + std::to_string(seed) + ")";
				same_rectangle(handed[index]->shape()->bounding_box(), house[index]->shape()->bounding_box(),
					where + ": a hand moved collision");
				const auto* paintable = dynamic_cast<const PaintableObject*>(handed[index].get());
				const auto* house_paintable = dynamic_cast<const PaintableObject*>(house[index].get());
				require((paintable == nullptr) == (house_paintable == nullptr), where + ": a hand changed scoring");
				if (paintable != nullptr)
				{
					require(paintable->paint_total().total == house_paintable->paint_total().total,
						where + ": a hand changed the scored area");
				}
				const auto* structure = dynamic_cast<const Structure*>(handed[index].get());
				const auto* house_structure = dynamic_cast<const Structure*>(house[index].get());
				if (structure == nullptr || structure->material() == nullptr) { continue; }
				check_drawing(*structure, where);
				const bool is_ground = structure->material()->name() == "ground";
				ground |= is_ground;
				const bool same = same_drawing(structure->material()->quads(), house_structure->material()->quads());
				require(is_ground || same, where + ": a material without slips took the hand");
				changed |= !same;
			}
		}
		require(!ground || changed, stage + ": no ground on the stage takes the match's hand");
	}
}
