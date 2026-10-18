#pragma once

// The monitor borrows the level kit's swatch texels, keeping palette edits in
// one atlas recipe. Resource checks reject frames whose pivot would displace
// their slot, before a level can be built. No graphics device is needed.
namespace
{
	std::unique_ptr<SpriteSheet> make_palette_sheet(int width, int height,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO)
	{
		NameTable<SpriteFrame> frames("sprite frame");
		frames.add("fixture_swatch", SpriteFrame(mattmath::RectangleI(4, 8, width, height), origin));
		return std::make_unique<SpriteSheet>(TextureHandle{}, std::move(frames),
			NameTable<AnimationStrip>("animation strip"));
	}

	void check_opening_palette_content(World& world, const std::filesystem::path& content)
	{
		const MenuPresentationVariant variants(content);
		const OpeningPresentation& shipped = world.presentation.opening();
		require(shipped.palette_sheet == TerrainMaterial::SHEET_NAME &&
			shipped.palette_frames.size() == static_cast<size_t>(TerrainRole::count),
			"The monitor palette does not use all roles from the level's terrain kit");
		for (size_t index = 0; index < shipped.palette_frames.size(); ++index)
		{
			require(shipped.palette_frames[index] == std::string("swatch_") +
				terrain_role_name(static_cast<TerrainRole>(index)),
				"The shipped monitor palette lost the terrain kit's authored role order");
		}
		world.presentation.validate_resources(world.render, world.audio);

		const auto rejected = [&](const char* key, auto edit, bool resources = false)
		{
			std::string diagnostic = "Presentation accepted the variant";
			bool named = false;
			try
			{
				const Presentation look = variants.load(edit);
				if (resources) { look.validate_resources(world.render, world.audio); }
			}
			catch (const std::runtime_error& error)
			{
				diagnostic = error.what();
				named = diagnostic.find("opening") != std::string::npos &&
					diagnostic.find(key) != std::string::npos;
			}
			if (!named)
			{
				throw std::runtime_error(std::string("Invalid monitor palette field '") + key +
					"' was accepted or lost its key: " + diagnostic);
			}
		};
		for (const char* key : { "palette_sheet", "palette_frames" })
		{
			rejected(key, [&](rapidjson::Document& json) { json["opening"].RemoveMember(key); });
			rejected(key, [&](rapidjson::Document& json) { json["opening"][key].SetNull(); });
			rejected(key, [&](rapidjson::Document& json) { json["opening"][key].SetInt(2); });
		}
		rejected("palette_sheet", [](rapidjson::Document& json)
			{ json["opening"]["palette_sheet"].SetString("", json.GetAllocator()); });
		rejected("palette_frames", [](rapidjson::Document& json) { json["opening"]["palette_frames"].SetArray(); });
		rejected("palette_frames", [](rapidjson::Document& json)
			{ json["opening"]["palette_frames"].SetString("swatch_paper", json.GetAllocator()); });
		rejected("palette_frames", [](rapidjson::Document& json) { json["opening"]["palette_frames"][0].SetNull(); });
		rejected("palette_frames", [](rapidjson::Document& json) { json["opening"]["palette_frames"][0].SetInt(2); });
		rejected("palette_frames", [](rapidjson::Document& json)
			{ json["opening"]["palette_frames"][0].SetString("", json.GetAllocator()); });
		rejected("palette_sheet", [](rapidjson::Document& json)
			{ json["opening"]["palette_sheet"].SetString("missing_palette_sheet", json.GetAllocator()); }, true);
		rejected("palette_frames[0]", [](rapidjson::Document& json)
			{ json["opening"]["palette_frames"][0].SetString("missing_palette_frame", json.GetAllocator()); }, true);

		const auto fixture = [](rapidjson::Document& json)
		{
			auto& opening = json["opening"];
			opening["palette_sheet"].SetString("palette_fixture", json.GetAllocator());
			opening["palette_frames"].SetArray();
			opening["palette_frames"].PushBack(rapidjson::Value("fixture_swatch", json.GetAllocator()), json.GetAllocator());
		};
		for (const mattmath::Vector2I size : { mattmath::Vector2I(0, 4), mattmath::Vector2I(-1, 4),
			mattmath::Vector2I(4, 0), mattmath::Vector2I(4, -1), mattmath::Vector2I(4, 8), mattmath::Vector2I(8, 4) })
		{
			world.render.add_sprite_sheet("palette_fixture", make_palette_sheet(size.x, size.y));
			rejected("palette_frames[0]", fixture, true);
		}
		for (const mattmath::Vector2F origin : { mattmath::Vector2F(1.0f, 0.0f), mattmath::Vector2F(0.0f, -1.0f) })
		{
			world.render.add_sprite_sheet("palette_fixture", make_palette_sheet(4, 4, origin));
			rejected("palette_frames[0]", fixture, true);
		}
		for (const int size : { 1, 4, 8 })
		{
			world.render.add_sprite_sheet("palette_fixture", make_palette_sheet(size, size));
			const Presentation single = variants.load(fixture);
			single.validate_resources(world.render, world.audio);
			require(single.opening().palette_frames.size() == 1 &&
				single.opening().palette_frames[0] == "fixture_swatch",
				"A valid custom one-swatch palette was not retained");
		}
		const Presentation reordered = variants.load([](rapidjson::Document& json)
		{
			auto& frames = json["opening"]["palette_frames"];
			frames[0].Swap(frames[frames.Size() - 1]);
		});
		reordered.validate_resources(world.render, world.audio);
		require(reordered.opening().palette_frames.front() == shipped.palette_frames.back() &&
			reordered.opening().palette_frames.back() == shipped.palette_frames.front(),
			"A valid custom palette order was not retained");
	}
}
