#pragma once

// Included after menu_content_checks.h's reserved presentation-variant helper.
namespace
{
	void check_hud_paper_content(const std::filesystem::path& content)
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
				named = message.find("hud") != std::string::npos &&
					message.find("paper") != std::string::npos &&
					(key == nullptr || message.find(key) != std::string::npos);
			}
			require(named, "Invalid HUD paper did not fail with its presentation section named");
		};
		rejected(nullptr, [](rapidjson::Document& json) { json["hud"].RemoveMember("paper"); });
		for (int type = 0; type < 5; ++type)
		{
			const auto wrong_type = [&](rapidjson::Value& value, auto& allocator)
			{
				switch (type)
				{
				case 0: value.SetNull(); break;
				case 1: value.SetBool(false); break;
				case 2: value.SetInt(0); break;
				case 3: value.SetString("not an object", allocator); break;
				case 4: value.SetArray(); break;
				}
			};
			rejected(nullptr, [&](rapidjson::Document& json)
				{ wrong_type(json["hud"]["paper"], json.GetAllocator()); });
		}

		struct Number { const char* key; float maximum; bool exclusive_minimum; };
		for (const Number number : { Number{ "keyline_width", 4.0f, true },
			Number{ "shadow_offset", 12.0f, false } })
		{
			rejected(number.key, [&](rapidjson::Document& json)
				{ json["hud"]["paper"].RemoveMember(number.key); });
			for (const double value : { -0.001, static_cast<double>(number.maximum) + 0.001, 1.0e100 })
			{
				rejected(number.key, [&](rapidjson::Document& json)
					{ json["hud"]["paper"][number.key].SetDouble(value); });
			}
			if (number.exclusive_minimum)
			{
				rejected(number.key, [&](rapidjson::Document& json)
					{ json["hud"]["paper"][number.key].SetInt(0); });
			}
			for (int type = 0; type < 5; ++type)
			{
				rejected(number.key, [&](rapidjson::Document& json)
				{
					auto& value = json["hud"]["paper"][number.key];
					switch (type)
					{
					case 0: value.SetNull(); break;
					case 1: value.SetBool(false); break;
					case 2: value.SetString("not a number", json.GetAllocator()); break;
					case 3: value.SetObject(); break;
					case 4: value.SetArray(); break;
					}
				});
			}
		}

		for (const char* key : { "ink", "paper", "shade" })
		{
			rejected(key, [&](rapidjson::Document& json) { json["hud"]["paper"].RemoveMember(key); });
			for (const char* value : { "not a colour", "12345", "GG0000", "" })
			{
				// The shared colour reader names the section for malformed colour text.
				rejected(nullptr, [&](rapidjson::Document& json)
					{ json["hud"]["paper"][key].SetString(value, json.GetAllocator()); });
			}
			for (int type = 0; type < 5; ++type)
			{
				rejected(key, [&](rapidjson::Document& json)
				{
					auto& value = json["hud"]["paper"][key];
					switch (type)
					{
					case 0: value.SetNull(); break;
					case 1: value.SetBool(false); break;
					case 2: value.SetInt(0); break;
					case 3: value.SetObject(); break;
					case 4: value.SetArray(); break;
					}
				});
			}
		}
		for (const char* key : { "ink", "paper" })
		{
			for (const char* value : { "FFFFFF00", "FFFFFF80", "FFFFFFFE" })
			{
				rejected(nullptr, [&](rapidjson::Document& json)
					{ json["hud"]["paper"][key].SetString(value, json.GetAllocator()); });
			}
		}
		rejected(nullptr, [](rapidjson::Document& json)
			{ json["hud"]["paper"]["shade"].SetString("00000000", json.GetAllocator()); });
		rejected(nullptr, [](rapidjson::Document& json)
		{
			json["hud"]["paper"]["ink"].SetString("WHITE", json.GetAllocator());
			json["hud"]["paper"]["paper"].SetString("FFFFFF", json.GetAllocator());
		});

		for (const bool upper : { false, true })
		{
			const float keyline = upper ? 4.0f : 0.001f;
			const float shadow = upper ? 12.0f : 0.0f;
			const Presentation look = variants.load([&](rapidjson::Document& json)
			{
				auto& paper = json["hud"]["paper"];
				paper["keyline_width"].SetFloat(keyline);
				paper["shadow_offset"].SetFloat(shadow);
				paper["ink"].SetString(upper ? "A0B1C2FF" : "BLACK", json.GetAllocator());
				paper["paper"].SetString(upper ? "D3E4F5" : "WHITE", json.GetAllocator());
				paper["shade"].SetString(upper ? "A0B1C201" : "BLACK", json.GetAllocator());
			});
			const auto& paper = look.hud().paper;
			require(paper.keyline_width == keyline && paper.shadow_offset == shadow,
				"Valid HUD paper dimensions were not retained");
			require(paper.ink == (upper ? Colour("A0B1C2FF") : Colour::black) &&
				paper.paper == (upper ? Colour("D3E4F5") : Colour::white) &&
				paper.shade == (upper ? Colour("A0B1C201") : Colour::black),
				"Valid HUD paper colours were not retained");
		}
	}
}
