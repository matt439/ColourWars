#pragma once

// Included after menu_content_checks.h's reserved presentation-variant helper.
namespace
{
	void check_end_menu_art_content(const std::filesystem::path& content)
	{
		const MenuPresentationVariant variants(content);
		const auto rejected = [&](const char* key, auto edit)
		{
			bool named = false;
			try
			{
				std::ignore = variants.load(edit);
			}
			catch (const std::runtime_error& error)
			{
				const std::string message = error.what();
				const bool box = key != nullptr && std::string(key) == "box_size";
				named = message.find("end_menu") != std::string::npos &&
					message.find(box ? "box_size" : "art") != std::string::npos &&
					(key == nullptr || message.find(key) != std::string::npos);
			}
			require(named, "Invalid end-menu artwork did not fail with its presentation field named");
		};
		rejected(nullptr, [](rapidjson::Document& json) { json["end_menu"].RemoveMember("art"); });
		rejected(nullptr, [](rapidjson::Document& json) { json["end_menu"]["art"].SetArray(); });
		for (const char* key : { "ink", "paper", "shade", "focus_fill", "focus_ink",
			"keyline_width", "shadow_offset", "focus_padding" })
		{
			rejected(key, [&](rapidjson::Document& json) { json["end_menu"]["art"].RemoveMember(key); });
			rejected(key, [&](rapidjson::Document& json) { json["end_menu"]["art"][key].SetNull(); });
		}
		rejected("box_size", [](rapidjson::Document& json) { json["end_menu"].RemoveMember("box_size"); });
		rejected("box_size", [](rapidjson::Document& json) { json["end_menu"]["box_size"].SetNull(); });
		const auto rejected_heading = [&](auto edit)
		{
			bool named = false;
			try { std::ignore = variants.load(edit); }
			catch (const std::runtime_error& error)
			{
				const std::string message = error.what();
				named = message.find("end_menu") != std::string::npos && message.find("heading_y") != std::string::npos;
			}
			require(named, "Invalid end-menu heading placement did not identify its field");
		};
		rejected_heading([](rapidjson::Document& json) { json["end_menu"].RemoveMember("heading_y"); });
		rejected_heading([](rapidjson::Document& json) { json["end_menu"]["heading_y"].SetNull(); });
		for (const double value : { -1.0, 0.0, 900.001, 1.0e100 })
		{
			rejected_heading([&](rapidjson::Document& json) { json["end_menu"]["heading_y"].SetDouble(value); });
		}
		for (const char* axis : { "x", "y" })
		{
			rejected("box_size", [&](rapidjson::Document& json)
				{ json["end_menu"]["box_size"].RemoveMember(axis); });
			for (const double value : { -0.001, 0.0, axis[0] == 'x' ? 1600.001 : 900.001, 1.0e100 })
			{
				rejected("box_size", [&](rapidjson::Document& json)
					{ json["end_menu"]["box_size"][axis].SetDouble(value); });
			}
			for (const float size : { 31.999f, 32.0f })
			{
				rejected("box_size", [&](rapidjson::Document& json)
				{
					json["end_menu"]["art"]["keyline_width"].SetFloat(4.0f);
					json["end_menu"]["art"]["shadow_offset"].SetFloat(8.0f);
					json["end_menu"]["box_size"][axis].SetFloat(size);
				});
			}
		}

		struct Number { const char* key; float maximum; bool positive; };
		for (const Number number : { Number{ "keyline_width", 8.0f, true },
			Number{ "shadow_offset", 24.0f, false } })
		{
			for (const double value : { -0.001, static_cast<double>(number.maximum) + 0.001, 1.0e100 })
			{
				rejected(number.key, [&](rapidjson::Document& json)
					{ json["end_menu"]["art"][number.key].SetDouble(value); });
			}
			if (number.positive)
			{
				rejected(number.key, [&](rapidjson::Document& json)
					{ json["end_menu"]["art"][number.key].SetInt(0); });
			}
		}
		for (const char* axis : { "x", "y" })
		{
			rejected("focus_padding", [&](rapidjson::Document& json)
				{ json["end_menu"]["art"]["focus_padding"].RemoveMember(axis); });
			for (const double value : { -0.001, 0.0, axis[0] == 'x' ? 48.001 : 24.001, 1.0e100 })
			{
				rejected("focus_padding", [&](rapidjson::Document& json)
					{ json["end_menu"]["art"]["focus_padding"][axis].SetDouble(value); });
			}
			rejected("focus_padding", [&](rapidjson::Document& json)
				{ json["end_menu"]["art"]["focus_padding"][axis].SetBool(false); });
			for (const float padding : { 11.999f, 12.0f })
			{
				rejected("focus_padding", [&](rapidjson::Document& json)
				{
					auto& art = json["end_menu"]["art"];
					art["keyline_width"].SetFloat(4.0f);
					art["shadow_offset"].SetFloat(8.0f);
					art["focus_padding"][axis].SetFloat(padding);
				});
			}
		}

		for (const char* key : { "ink", "paper", "shade", "focus_fill", "focus_ink" })
		{
			rejected(nullptr, [&](rapidjson::Document& json)
				{ json["end_menu"]["art"][key].SetString("not a colour", json.GetAllocator()); });
			if (std::string(key) == "shade")
			{
				rejected(nullptr, [&](rapidjson::Document& json)
					{ json["end_menu"]["art"][key].SetString("00000000", json.GetAllocator()); });
			}
			else
			{
				for (const char* value : { "FFFFFF00", "FFFFFF80", "FFFFFFFE" })
				{
					rejected(nullptr, [&](rapidjson::Document& json)
						{ json["end_menu"]["art"][key].SetString(value, json.GetAllocator()); });
				}
			}
		}
		struct Pair { const char* first; const char* second; };
		for (const Pair pair : { Pair{ "ink", "paper" }, Pair{ "ink", "focus_fill" }, Pair{ "focus_ink", "focus_fill" },
			Pair{ "focus_fill", "paper" } })
		{
			rejected(nullptr, [&](rapidjson::Document& json)
			{
				auto& art = json["end_menu"]["art"];
				art[pair.first].SetString("WHITE", json.GetAllocator());
				art[pair.second].SetString("FFFFFF", json.GetAllocator());
			});
		}

		for (const bool upper : { false, true })
		{
			const float keyline = upper ? 8.0f : 0.001f;
			const float shadow = 0.0f;
			const mattmath::Vector2F padding = upper ? mattmath::Vector2F(48.0f, 24.0f) :
				mattmath::Vector2F(0.002f, 0.002f);
			const Presentation look = variants.load([&](rapidjson::Document& json)
			{
				auto& art = json["end_menu"]["art"];
				json["end_menu"]["box_size"]["x"].SetFloat(upper ? 1600.0f : 0.007f);
				json["end_menu"]["box_size"]["y"].SetFloat(upper ? 900.0f : 0.007f);
				art["keyline_width"].SetFloat(keyline);
				art["shadow_offset"].SetFloat(shadow);
				art["focus_padding"]["x"].SetFloat(padding.x);
				art["focus_padding"]["y"].SetFloat(padding.y);
				art["ink"].SetString(upper ? "A0B1C2FF" : "BLACK", json.GetAllocator());
				art["paper"].SetString(upper ? "D3E4F5" : "WHITE", json.GetAllocator());
				art["shade"].SetString(upper ? "A0B1C201" : "BLACK", json.GetAllocator());
				art["focus_fill"].SetString("808080", json.GetAllocator());
				art["focus_ink"].SetString("112233", json.GetAllocator());
			});
			const auto& art = look.end_menu().art;
			require(look.end_menu().box_size == (upper ? mattmath::Vector2F(1600.0f, 900.0f) :
				mattmath::Vector2F(0.007f, 0.007f)), "Valid end-menu paper size was not retained");
			require(art.keyline_width == keyline && art.shadow_offset == shadow &&
				art.focus_padding == padding, "Valid end-menu artwork dimensions were not retained");
			require(art.ink == (upper ? Colour("A0B1C2FF") : Colour::black) &&
				art.paper == (upper ? Colour("D3E4F5") : Colour::white) &&
				art.shade == (upper ? Colour("A0B1C201") : Colour::black) &&
				art.focus_fill == Colour("808080") && art.focus_ink == Colour("112233"),
				"Valid end-menu artwork colours were not retained");
		}
		const Presentation near_shadow_limit = variants.load([](rapidjson::Document& json)
		{
			auto& art = json["end_menu"]["art"];
			art["keyline_width"].SetFloat(0.001f);
			art["shadow_offset"].SetFloat(23.998f);
			art["focus_padding"]["x"].SetFloat(48.0f);
			art["focus_padding"]["y"].SetFloat(24.0f);
		});
		require(near_shadow_limit.end_menu().art.shadow_offset == 23.998f,
			"A visible focus-card face near the shadow limit was rejected or changed");
	}
}
