#pragma once

// Shared paper validation has full coverage in end_menu_content_checks.h;
// results adds centered text, a bordered score track and reveal timings.
namespace
{
	void check_results_menu_art_content(const std::filesystem::path& content)
	{
		const MenuPresentationVariant variants(content);
		const Presentation shipped = variants.load([](rapidjson::Document&) {});
		const auto& menu = shipped.results_menu();
		require(menu.box_size == mattmath::Vector2F(1300.0f, 860.0f) &&
			menu.heading_y == 125.0f && menu.winner_y == 255.0f &&
			menu.session_score_y == 340.0f && menu.session_result_y == 405.0f &&
			menu.proceed_y == 880.0f &&
			menu.contribution_position == mattmath::Vector2F(480.0f, 635.0f) &&
			menu.contribution_spacing == 45.0f && menu.contribution_font == "gill_sans_mt_bold_24",
			"Shipped results text placement was not retained by the presentation reader");
		require(menu.art.ink == Colour("1B1C20") && menu.art.paper == Colour("F0F0ED") &&
			menu.art.shade == Colour("00000035") && menu.art.focus_fill == Colour("D5D5CF") &&
			menu.art.focus_ink == Colour("111216") && menu.art.keyline_width == 4.0f &&
			menu.art.shadow_offset == 8.0f && menu.art.focus_padding == mattmath::Vector2F(28.0f, 18.0f),
			"Shipped results paper palette and geometry were not retained");
		require(menu.track_colour == Colour("B8B8B8") &&
			menu.fill_box_size == mattmath::Vector2F(700.0f, 120.0f) && menu.fill_box_y_offset == 380.0f &&
			menu.team_fill_border == mattmath::Vector2F(10.0f, 10.0f) &&
			menu.team_fill_size == mattmath::Vector2F(680.0f, 100.0f) &&
			menu.team_fill_delay == 1.0f && menu.team_fill_time == 2.0f && menu.show_results_delay == 1.0f &&
			menu.team_mark_size == mattmath::Vector2F(56.0f, 56.0f) && menu.team_mark_gap == 20.0f,
			"Shipped results score track, reveal timings or team marks were not retained");

		const auto rejected = [&](const char* key, auto edit)
		{
			bool named = false;
			std::string diagnostic = "Presentation accepted the variant";
			try { std::ignore = variants.load(edit); }
			catch (const std::runtime_error& error)
			{
				diagnostic = error.what();
				named = diagnostic.find("results_menu") != std::string::npos &&
					diagnostic.find(key) != std::string::npos;
			}
			if (!named)
			{
				throw std::runtime_error(std::string("Invalid results presentation field '") + key +
					"' was accepted or lost its key: " + diagnostic);
			}
		};
		rejected("art", [](rapidjson::Document& json) { json["results_menu"].RemoveMember("art"); });
		rejected("art", [](rapidjson::Document& json) { json["results_menu"]["art"].SetArray(); });
		for (const char* key : { "ink", "paper", "shade", "focus_fill", "focus_ink",
			"keyline_width", "shadow_offset", "focus_padding" })
		{
			rejected(key, [&](rapidjson::Document& json) { json["results_menu"]["art"].RemoveMember(key); });
		}
		rejected("keyline_width", [](rapidjson::Document& json)
			{ json["results_menu"]["art"]["keyline_width"].SetInt(0); });
		rejected("focus_padding", [](rapidjson::Document& json)
			{ json["results_menu"]["art"]["focus_padding"]["y"].SetFloat(12.0f); });
		rejected("paper", [](rapidjson::Document& json)
			{ json["results_menu"]["art"]["paper"].SetString("F0F0ED80", json.GetAllocator()); });

		for (const char* key : { "box_size", "heading_y", "winner_y", "session_score_y", "session_result_y", "proceed_y" })
		{
			rejected(key, [&](rapidjson::Document& json) { json["results_menu"].RemoveMember(key); });
			rejected(key, [&](rapidjson::Document& json) { json["results_menu"][key].SetNull(); });
		}
		for (const char* key : { "heading_y", "winner_y", "session_score_y", "session_result_y", "proceed_y" })
		{
			for (const double value : { 0.0, 900.001, 1.0e100 })
			{
				rejected(key, [&](rapidjson::Document& json) { json["results_menu"][key].SetDouble(value); });
			}
		}
		struct Ordered { const char* earlier; const char* later; };
		for (const Ordered pair : { Ordered{ "heading_y", "winner_y" },
			Ordered{ "winner_y", "session_score_y" }, Ordered{ "session_score_y", "session_result_y" } })
		{
			rejected(pair.later, [&](rapidjson::Document& json)
			{
				auto& results = json["results_menu"];
				results[pair.later].SetFloat(results[pair.earlier].GetFloat());
			});
		}
		for (const char* axis : { "x", "y" })
		{
			rejected("box_size", [&](rapidjson::Document& json)
				{ json["results_menu"]["box_size"][axis].SetFloat(axis[0] == 'x' ? 1600.001f : 900.001f); });
			rejected("box_size", [&](rapidjson::Document& json)
				{ json["results_menu"]["fill"]["box_size"][axis].SetInt(0); });
			rejected("box_size", [&](rapidjson::Document& json)
				{ json["results_menu"]["fill"]["box_size"][axis].SetFloat(axis[0] == 'x' ? 1301.0f : 861.0f); });
			rejected("border", [&](rapidjson::Document& json)
				{ json["results_menu"]["fill"]["border"][axis].SetFloat(3.999f); });
			rejected("border", [&](rapidjson::Document& json)
			{
				auto& fill = json["results_menu"]["fill"];
				fill["border"][axis].SetFloat(fill["box_size"][axis].GetFloat() / 2.0f);
			});
			rejected("size", [&](rapidjson::Document& json)
				{ json["results_menu"]["team_mark"]["size"][axis].SetInt(0); });
		}
		rejected("track_colour", [](rapidjson::Document& json) { json["results_menu"]["fill"].RemoveMember("track_colour"); });
		rejected("fill", [](rapidjson::Document& json)
			{ json["results_menu"]["fill"]["track_colour"].SetString("not a colour", json.GetAllocator()); });
		for (const char* colour : { "B8B8B880", "1B1C20" })
		{
			rejected("track_colour", [&](rapidjson::Document& json)
				{ json["results_menu"]["fill"]["track_colour"].SetString(colour, json.GetAllocator()); });
		}
		rejected("box_y_offset", [](rapidjson::Document& json)
			{ json["results_menu"]["fill"]["box_y_offset"].SetInt(-1); });
		rejected("box_y_offset", [](rapidjson::Document& json)
			{ json["results_menu"]["fill"]["box_y_offset"].SetFloat(900.001f); });
		for (const char* key : { "delay", "time" })
		{
			for (const double value : { -0.001, 10.001, 1.0e100 })
			{
				rejected(key, [&](rapidjson::Document& json) { json["results_menu"]["fill"][key].SetDouble(value); });
			}
		}
		rejected("time", [](rapidjson::Document& json) { json["results_menu"]["fill"]["time"].SetInt(0); });
		for (const double value : { -0.001, 10.001, 1.0e100 })
		{
			rejected("show_results_delay", [&](rapidjson::Document& json)
				{ json["results_menu"]["show_results_delay"].SetDouble(value); });
		}
		for (const float value : { 0.0f, 100.001f })
		{
			rejected("contribution_spacing", [&](rapidjson::Document& json)
				{ json["results_menu"]["contribution_spacing"].SetFloat(value); });
		}
		for (const float value : { -0.001f, 100.001f })
		{
			rejected("gap", [&](rapidjson::Document& json) { json["results_menu"]["team_mark"]["gap"].SetFloat(value); });
		}

		const Presentation boundary = variants.load([](rapidjson::Document& json)
		{
			auto& results = json["results_menu"];
			results["proceed_y"].SetFloat(900.0f);
			results["show_results_delay"].SetInt(0);
			results["team_mark"]["gap"].SetInt(0);
			auto& fill = results["fill"];
			fill["border"]["x"].SetFloat(4.0f);
			fill["border"]["y"].SetFloat(4.0f);
			fill["box_y_offset"].SetInt(0);
			fill["delay"].SetInt(0);
			fill["time"].SetFloat(10.0f);
			fill["track_colour"].SetString("B8B8B8FF", json.GetAllocator());
		});
		const auto& valid = boundary.results_menu();
		require(valid.proceed_y == 900.0f && valid.show_results_delay == 0.0f && valid.team_mark_gap == 0.0f &&
			valid.team_fill_border == mattmath::Vector2F(4.0f, 4.0f) &&
			valid.team_fill_size == mattmath::Vector2F(692.0f, 112.0f) &&
			valid.fill_box_y_offset == 0.0f && valid.team_fill_delay == 0.0f && valid.team_fill_time == 10.0f &&
			valid.track_colour == menu.track_colour,
			"Legal results border, reveal or opaque track-colour boundaries were changed");
	}
}
