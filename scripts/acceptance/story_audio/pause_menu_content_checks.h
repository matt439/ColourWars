#pragma once

// Common paper rules are exercised in end_menu_content_checks.h; these
// variants keep pause's startup validation and unavailable-row ink covered.
namespace
{
	void check_pause_menu_art_content(const std::filesystem::path& content)
	{
		const MenuPresentationVariant variants(content);
		const Presentation shipped = variants.load([](rapidjson::Document&) {});
		const auto& pause = shipped.pause_menu();
		require(pause.box_size == mattmath::Vector2F(1050.0f, 900.0f) &&
			pause.heading_y == 180.0f && pause.status_y == 315.0f &&
			pause.art.disabled_ink == Colour("62635F"),
			"Shipped pause artwork was not retained by the presentation reader");
		const auto rejected = [&](const char* key, auto edit)
		{
			bool named = false;
			std::string diagnostic = "Presentation accepted the variant";
			try { std::ignore = variants.load(edit); }
			catch (const std::runtime_error& error)
			{
				const std::string message = error.what();
				diagnostic = message;
				named = message.find("pause_menu") != std::string::npos &&
					message.find(key) != std::string::npos;
			}
			if (!named)
			{
				throw std::runtime_error(std::string("Invalid pause artwork field '") + key +
					"' was accepted or lost its key: " + diagnostic);
			}
		};
		rejected("art", [](rapidjson::Document& json) { json["pause_menu"].RemoveMember("art"); });
		rejected("art", [](rapidjson::Document& json) { json["pause_menu"]["art"].SetArray(); });
		for (const char* key : { "ink", "paper", "shade", "focus_fill", "focus_ink",
			"disabled_ink", "keyline_width", "shadow_offset", "focus_padding" })
		{
			rejected(key, [&](rapidjson::Document& json) { json["pause_menu"]["art"].RemoveMember(key); });
		}
		for (const char* key : { "box_size", "heading_y", "status_y" })
		{
			rejected(key, [&](rapidjson::Document& json) { json["pause_menu"].RemoveMember(key); });
			rejected(key, [&](rapidjson::Document& json) { json["pause_menu"][key].SetNull(); });
		}
		for (const char* axis : { "x", "y" })
		{
			for (const double value : { -1.0, 0.0, axis[0] == 'x' ? 1600.001 : 900.001, 1.0e100 })
			{
				rejected("box_size", [&](rapidjson::Document& json)
					{ json["pause_menu"]["box_size"][axis].SetDouble(value); });
			}
			rejected("box_size", [&](rapidjson::Document& json)
			{
				auto& menu = json["pause_menu"];
				menu["box_size"][axis].SetFloat(pause.art.shadow_offset + pause.art.keyline_width * 6.0f);
			});
			rejected("focus_padding", [&](rapidjson::Document& json)
			{
				auto& art = json["pause_menu"]["art"];
				art["focus_padding"][axis].SetFloat(pause.art.shadow_offset + pause.art.keyline_width);
			});
		}
		for (const char* key : { "heading_y", "status_y" })
		{
			for (const double value : { -1.0, 0.0, 900.001, 1.0e100 })
			{
				rejected(key, [&](rapidjson::Document& json) { json["pause_menu"][key].SetDouble(value); });
			}
		}
		for (const float status_y : { pause.heading_y - 1.0f, pause.heading_y })
		{
			rejected("status_y", [&](rapidjson::Document& json)
				{ json["pause_menu"]["status_y"].SetFloat(status_y); });
		}
		rejected("art", [](rapidjson::Document& json)
			{ json["pause_menu"]["art"]["disabled_ink"].SetString("not a colour", json.GetAllocator()); });
		for (const char* value : { "62635F00", "62635F80", "62635FFE" })
		{
			rejected("disabled_ink", [&](rapidjson::Document& json)
				{ json["pause_menu"]["art"]["disabled_ink"].SetString(value, json.GetAllocator()); });
		}
		rejected("disabled_ink", [](rapidjson::Document& json)
			{ json["pause_menu"]["art"]["disabled_ink"].SetNull(); });
		for (const char* key : { "ink", "paper", "focus_fill", "focus_ink" })
		{
			rejected("disabled_ink", [&](rapidjson::Document& json)
			{
				auto& art = json["pause_menu"]["art"];
				art["disabled_ink"].CopyFrom(art[key], json.GetAllocator());
			});
		}
		rejected("art", [](rapidjson::Document& json)
			{ json["pause_menu"]["art"]["paper"].SetString("FFFFFF80", json.GetAllocator()); });

		// The endpoints are legal even when they leave no useful UI layout;
		// text-fit acceptance owns layout while this reader owns safe geometry.
		for (const bool upper : { false, true })
		{
			const Presentation boundary = variants.load([&](rapidjson::Document& json)
			{
				auto& menu = json["pause_menu"];
				menu["heading_y"].SetFloat(upper ? 899.999f : 0.001f);
				menu["status_y"].SetFloat(upper ? 900.0f : 0.002f);
				menu["box_size"]["x"].SetFloat(upper ? 1600.0f : 0.007f);
				menu["box_size"]["y"].SetFloat(upper ? 900.0f : 0.007f);
				auto& art = menu["art"];
				art["keyline_width"].SetFloat(0.001f);
				art["shadow_offset"].SetFloat(0.0f);
				art["focus_padding"]["x"].SetFloat(0.002f);
				art["focus_padding"]["y"].SetFloat(0.002f);
				art["disabled_ink"].SetString("62635FFF", json.GetAllocator());
			});
			const auto& menu = boundary.pause_menu();
			require(menu.heading_y == (upper ? 899.999f : 0.001f) &&
				menu.status_y == (upper ? 900.0f : 0.002f) &&
				menu.box_size == (upper ? mattmath::Vector2F(1600.0f, 900.0f) :
					mattmath::Vector2F(0.007f, 0.007f)) && menu.art.disabled_ink == pause.art.disabled_ink,
				"Valid pause artwork boundaries or opaque disabled ink were changed");
		}
	}
}
