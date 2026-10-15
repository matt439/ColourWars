#pragma once

// Included after menu_content_checks.h's reserved presentation-variant helper.
namespace
{
	void check_closing_art_content(const std::filesystem::path& content)
	{
		const MenuPresentationVariant variants(content);
		const auto rejected = [&](const char* owner, const char* section, const char* key, auto edit)
		{
			bool named = false;
			try
			{
				std::ignore = variants.load(edit);
			}
			catch (const std::runtime_error& error)
			{
				const std::string message = error.what();
				named = message.find(owner) != std::string::npos &&
					message.find(section) != std::string::npos &&
					(key == nullptr || message.find(key) != std::string::npos);
			}
			require(named, "Invalid closing artwork did not fail with its presentation section named");
		};
		struct Number
		{
			const char* key;
			float minimum;
			float maximum;
			bool exclusive_minimum;
		};
		const auto check_section = [&](const char* owner, const char* section,
			std::initializer_list<Number> numbers, std::initializer_list<const char*> colours)
		{
			rejected(owner, section, nullptr, [&](rapidjson::Document& json)
				{ json[owner].RemoveMember(section); });
			rejected(owner, section, nullptr, [&](rapidjson::Document& json)
				{ json[owner][section].SetNull(); });
			rejected(owner, section, nullptr, [&](rapidjson::Document& json)
				{ json[owner][section].SetInt(0); });
			for (const Number number : numbers)
			{
				for (const float value : { number.minimum - 0.001f, number.maximum + 0.001f })
				{
					rejected(owner, section, number.key, [&](rapidjson::Document& json)
						{ json[owner][section][number.key].SetFloat(value); });
				}
				if (number.exclusive_minimum)
				{
					rejected(owner, section, number.key, [&](rapidjson::Document& json)
						{ json[owner][section][number.key].SetFloat(number.minimum); });
				}
				rejected(owner, section, number.key, [&](rapidjson::Document& json)
					{ json[owner][section].RemoveMember(number.key); });
				rejected(owner, section, number.key, [&](rapidjson::Document& json)
					{ json[owner][section][number.key].SetNull(); });
				rejected(owner, section, number.key, [&](rapidjson::Document& json)
					{ json[owner][section][number.key].SetString("not a number", json.GetAllocator()); });
			}
			for (const char* key : colours)
			{
				// Invalid colour text is diagnosed at its owning section by the shared reader.
				for (const char* value : { "not a colour", "12345", "GG0000", "" })
				{
					rejected(owner, section, nullptr, [&](rapidjson::Document& json)
						{ json[owner][section][key].SetString(value, json.GetAllocator()); });
				}
				rejected(owner, section, key, [&](rapidjson::Document& json)
					{ json[owner][section].RemoveMember(key); });
				rejected(owner, section, key, [&](rapidjson::Document& json)
					{ json[owner][section][key].SetNull(); });
				rejected(owner, section, key, [&](rapidjson::Document& json)
					{ json[owner][section][key].SetInt(0); });
			}
		};
		check_section("hud", "closing_clock",
			{ { "keyline_width", 0.0f, 8.0f, true }, { "shadow_offset", 0.0f, 16.0f, false },
				{ "tick_reach", 0.0f, 24.0f, true }, { "beat_duration", 0.0f, 0.5f, true } },
			{ "ink", "paper", "shade", "text", "accent" });
		check_section("opening", "reveal_art",
			{ { "keyline_width", 0.0f, 0.06f, true }, { "padding", 0.08f, 0.18f, false },
				{ "shadow_offset", 0.0f, 0.1f, false }, { "tick_length", 0.0f, 0.2f, false },
				{ "beat_duration", 0.0f, 0.5f, true } },
			{ "ink", "paper", "shade" });

		for (const char* key : { "ink", "paper", "text", "accent" })
		{
			for (const char* value : { "FFFFFF00", "FFFFFF80" })
			{
				rejected("hud", "closing_clock", nullptr, [&](rapidjson::Document& json)
					{ json["hud"]["closing_clock"][key].SetString(value, json.GetAllocator()); });
			}
		}
		rejected("hud", "closing_clock", nullptr, [](rapidjson::Document& json)
			{ json["hud"]["closing_clock"]["shade"].SetString("00000000", json.GetAllocator()); });

		for (const bool upper : { false, true })
		{
			const float clock_keyline = upper ? 8.0f : 0.001f;
			const float clock_shadow = upper ? 16.0f : 0.0f;
			const float clock_tick = upper ? 24.0f : 0.001f;
			const float beat_duration = upper ? 0.5f : 0.001f;
			const float reveal_keyline = upper ? 0.06f : 0.001f;
			const float reveal_padding = upper ? 0.18f : 0.08f;
			const float reveal_shadow = upper ? 0.1f : 0.0f;
			const float reveal_tick = upper ? 0.2f : 0.0f;
			const Presentation look = variants.load([&](rapidjson::Document& json)
			{
				auto& clock = json["hud"]["closing_clock"];
				clock["keyline_width"].SetFloat(clock_keyline);
				clock["shadow_offset"].SetFloat(clock_shadow);
				clock["tick_reach"].SetFloat(clock_tick);
				clock["beat_duration"].SetFloat(beat_duration);
				for (const char* key : { "ink", "paper", "text", "accent" })
				{
					clock[key].SetString(upper ? "A0B1C2" : "WHITE", json.GetAllocator());
				}
				clock["shade"].SetString(upper ? "A0B1C280" : "BLACK", json.GetAllocator());
				auto& reveal = json["opening"]["reveal_art"];
				reveal["keyline_width"].SetFloat(reveal_keyline);
				reveal["padding"].SetFloat(reveal_padding);
				reveal["shadow_offset"].SetFloat(reveal_shadow);
				reveal["tick_length"].SetFloat(reveal_tick);
				reveal["beat_duration"].SetFloat(beat_duration);
				for (const char* key : { "ink", "paper", "shade" })
				{
					reveal[key].SetString(upper ? "A0B1C2" : "WHITE", json.GetAllocator());
				}
			});
			const auto& clock = look.hud().closing_clock;
			require(clock.keyline_width == clock_keyline && clock.shadow_offset == clock_shadow &&
				clock.tick_reach == clock_tick && clock.beat_duration == beat_duration,
				"Valid closing-clock artwork boundary was not retained");
			const auto& reveal = look.opening().reveal_art;
			require(reveal.keyline_width == reveal_keyline && reveal.padding == reveal_padding &&
				reveal.shadow_offset == reveal_shadow && reveal.tick_length == reveal_tick &&
				reveal.beat_duration == beat_duration,
				"Valid reveal artwork boundary was not retained");
		}
	}
}
