#pragma once

// The shared paper reader is covered in end_menu_content_checks.h. These
// variants exercise the main-menu pages and their authored anchor bounds.
namespace
{
	void check_main_menu_art_content(const std::filesystem::path& content)
	{
		const MenuPresentationVariant variants(content);
		const Presentation shipped = variants.load([](rapidjson::Document&) {});
		const auto& menu = shipped.main_menu();
		MenuPaperArtPresentation options_art = menu.art;
		options_art.focus_padding = menu.setup_focus_padding;
		options_art.shadow_offset = menu.setup_shadow_offset;
		const float content_height = shipped.menu().footer_y;
		require(menu.display_trial_seconds == 15.0f, "Display trial timeout changed from the reviewed fifteen seconds");
		const auto rejected = [&](const char* key, auto edit)
		{
			std::string diagnostic = "Presentation accepted the variant";
			bool named = false;
			try { std::ignore = variants.load(edit); }
			catch (const std::runtime_error& error)
			{
				diagnostic = error.what();
				named = diagnostic.find("main_menu") != std::string::npos &&
					diagnostic.find(key) != std::string::npos;
			}
			if (!named)
			{
				throw std::runtime_error(std::string("Invalid main-menu presentation field '") + key +
					"' was accepted or lost its key: " + diagnostic);
			}
		};
		rejected("display_trial_seconds", [](rapidjson::Document& json) { json["main_menu"].RemoveMember("display_trial_seconds"); });
		rejected("display_trial_seconds", [](rapidjson::Document& json) { json["main_menu"]["display_trial_seconds"].SetNull(); });
		rejected("display_trial_seconds", [](rapidjson::Document& json) { json["main_menu"]["display_trial_seconds"].SetBool(true); });
		for (const double value : { -1.0, 0.0, 0.999, 60.001, 1.0e100 })
		{
			rejected("display_trial_seconds", [value](rapidjson::Document& json)
				{ json["main_menu"]["display_trial_seconds"].SetDouble(value); });
		}
		for (const float value : { 1.0f, 1.5f, 60.0f })
		{
			const Presentation valid_timeout = variants.load([value](rapidjson::Document& json)
				{ json["main_menu"]["display_trial_seconds"].SetFloat(value); });
			require(valid_timeout.main_menu().display_trial_seconds == value, "Inclusive or fractional display trial timeout was rejected");
		}
		rejected("art", [](rapidjson::Document& json) { json["main_menu"].RemoveMember("art"); });
		rejected("art", [](rapidjson::Document& json) { json["main_menu"]["art"].SetArray(); });
		rejected("focus_ink", [](rapidjson::Document& json)
			{ json["main_menu"]["art"].RemoveMember("focus_ink"); });
		rejected("art", [](rapidjson::Document& json)
			{ json["main_menu"]["art"]["paper"].SetString("F0F0ED80", json.GetAllocator()); });
		rejected("focus_padding", [&](rapidjson::Document& json)
			{ json["main_menu"]["art"]["focus_padding"]["y"].SetFloat(menu.art.shadow_offset + menu.art.keyline_width); });

		for (const char* key : { "title_box_size", "home_box_size", "options_box_size", "credits_box_size" })
		{
			rejected(key, [&](rapidjson::Document& json) { json["main_menu"].RemoveMember(key); });
			rejected(key, [&](rapidjson::Document& json) { json["main_menu"][key].SetNull(); });
			for (const char* axis : { "x", "y" })
			{
				for (const double value : { 0.0,
					axis[0] == 'x' ? 1760.001 : static_cast<double>(content_height - 40.0f) + 0.001, 1.0e100 })
				{
					rejected(key, [&](rapidjson::Document& json)
						{ json["main_menu"][key][axis].SetDouble(value); });
				}
				rejected("box_size", [&](rapidjson::Document& json)
				{
					json["main_menu"][key][axis].SetFloat(menu.art.shadow_offset + menu.art.keyline_width * 6.0f);
				});
			}
		}
		for (const char* key : { "title_y", "title_subtitle_y", "title_start_y", "title_no_pad_y", "build_id_y" })
		{
			rejected(key, [&](rapidjson::Document& json) { json["main_menu"].RemoveMember(key); });
			rejected(key, [&](rapidjson::Document& json) { json["main_menu"][key].SetNull(); });
			for (const double value : { -0.001, 1080.001, 1.0e100 })
			{
				rejected(key, [&](rapidjson::Document& json) { json["main_menu"][key].SetDouble(value); });
			}
		}
		for (const char* key : { "home_position", "home_subtitle_position" })
		{
			rejected(key, [&](rapidjson::Document& json) { json["main_menu"].RemoveMember(key); });
			rejected(key, [&](rapidjson::Document& json) { json["main_menu"][key].SetNull(); });
			for (const char* axis : { "x", "y" })
			{
				for (const double value : { -0.001, axis[0] == 'x' ? 1920.001 : 1080.001, 1.0e100 })
				{
					rejected(key, [&](rapidjson::Document& json)
						{ json["main_menu"][key][axis].SetDouble(value); });
				}
			}
		}
		struct Anchors { const char* earlier; const char* later; };
		for (const Anchors pair : { Anchors{ "title_y", "title_subtitle_y" },
			Anchors{ "title_subtitle_y", "title_start_y" },
			Anchors{ "title_start_y", "title_no_pad_y" },
			Anchors{ "title_no_pad_y", "build_id_y" } })
		{
			rejected(pair.later, [&](rapidjson::Document& json)
			{
				auto& main = json["main_menu"];
				main[pair.later].SetFloat(main[pair.earlier].GetFloat());
			});
		}
		rejected("home_subtitle_position", [&](rapidjson::Document& json)
			{ json["main_menu"]["home_subtitle_position"]["y"].SetFloat(menu.home_position.y); });
		rejected("title_box_size", [&](rapidjson::Document& json)
			{ json["main_menu"]["title_y"].SetFloat((content_height - menu.title_box_size.y) / 2.0f); });
		rejected("title_box_size", [&](rapidjson::Document& json)
			{ json["main_menu"]["build_id_y"].SetFloat((content_height + menu.title_box_size.y) / 2.0f); });
		rejected("home_box_size", [&](rapidjson::Document& json)
			{ json["main_menu"]["home_position"]["x"].SetFloat((1920.0f - menu.home_box_size.x) / 2.0f); });
		rejected("home_spacing", [&](rapidjson::Document& json)
		{
			json["main_menu"]["home_spacing"]["y"].SetFloat(
				((content_height + menu.home_box_size.y) / 2.0f - menu.home_position.y) / 5.0f);
		});

		for (const char* key : { "disabled_ink", "options_warning_colour" })
		{
			const auto owner = [&](rapidjson::Document& json) -> rapidjson::Value&
			{
				return std::string(key) == "disabled_ink" ? json["main_menu"]["art"] : json["main_menu"];
			};
			rejected(key, [&](rapidjson::Document& json) { owner(json).RemoveMember(key); });
			rejected(key, [&](rapidjson::Document& json) { owner(json)[key].SetNull(); });
			rejected(key, [&](rapidjson::Document& json)
				{ owner(json)[key].SetString("30313880", json.GetAllocator()); });
			for (const char* background : { "paper", "focus_fill" })
			{
				rejected(key, [&](rapidjson::Document& json)
					{ owner(json)[key].CopyFrom(json["main_menu"]["art"][background], json.GetAllocator()); });
			}
		}
		for (const char* active : { "ink", "focus_ink" })
		{
			rejected("disabled_ink", [&](rapidjson::Document& json)
			{
				auto& art = json["main_menu"]["art"];
				art["disabled_ink"].CopyFrom(art[active], json.GetAllocator());
			});
		}

		struct PageAnchor { const char* key; bool credits; };
		for (const PageAnchor anchor : { PageAnchor{ "options_heading_position", false },
			PageAnchor{ "options_position", false }, PageAnchor{ "options_status_position", false },
			PageAnchor{ "heading_position", true }, PageAnchor{ "position", true },
			PageAnchor{ "back_position", true } })
		{
			const auto owner = [&](rapidjson::Document& json) -> rapidjson::Value&
				{ return anchor.credits ? json["main_menu"]["credits"] : json["main_menu"]; };
			rejected(anchor.key, [&](rapidjson::Document& json) { owner(json).RemoveMember(anchor.key); });
			rejected(anchor.key, [&](rapidjson::Document& json) { owner(json)[anchor.key].SetNull(); });
			const auto& size = anchor.credits ? menu.credits_box_size : menu.options_box_size;
			for (const char* axis : { "x", "y" })
			{
				const float extent = axis[0] == 'x' ? 1920.0f : content_height;
				const float length = axis[0] == 'x' ? size.x : size.y;
				const float edge = menu.art.keyline_width * 3.0f;
				const float shadow = anchor.credits ? menu.art.shadow_offset : options_art.shadow_offset;
				for (const double value : { 1.0e100, static_cast<double>((extent - length) / 2.0f + edge),
					static_cast<double>((extent + length) / 2.0f - shadow - edge) })
				{
					rejected(anchor.key, [&](rapidjson::Document& json)
						{ owner(json)[anchor.key][axis].SetDouble(value); });
				}
			}
		}
		for (const char* key : { "options_value_x", "options_spacing" })
		{
			rejected(key, [&](rapidjson::Document& json) { json["main_menu"].RemoveMember(key); });
			rejected(key, [&](rapidjson::Document& json) { json["main_menu"][key].SetNull(); });
		}
		for (const double value : { 1.0e100, static_cast<double>(menu.options_position.x),
			static_cast<double>(menu.options_position.x + options_art.focus_padding.x),
			static_cast<double>((1920.0f + menu.options_box_size.x) / 2.0f -
				options_art.shadow_offset - options_art.focus_padding.x),
			static_cast<double>((1920.0f + menu.options_box_size.x) / 2.0f -
				options_art.shadow_offset - options_art.keyline_width * 3.0f) })
		{
			rejected("options_value_x", [&](rapidjson::Document& json)
				{ json["main_menu"]["options_value_x"].SetDouble(value); });
		}
		for (const char* axis : { "x", "y" })
		{
			for (const double value : { 0.0, 1.0e100 })
			{
				rejected("options_spacing", [&](rapidjson::Document& json)
					{ json["main_menu"]["options_spacing"][axis].SetDouble(value); });
			}
		}
		rejected("options_spacing", [&](rapidjson::Document& json)
			{ json["main_menu"]["options_spacing"]["x"].SetDouble(1920.001); });
		rejected("options_spacing", [&](rapidjson::Document& json)
			{ json["main_menu"]["options_spacing"]["y"].SetFloat(options_art.focus_padding.y * 2.0f); });
		rejected("options_heading_position", [&](rapidjson::Document& json)
			{ json["main_menu"]["options_heading_position"]["y"].SetFloat(menu.options_position.y); });
		rejected("options_status_position", [&](rapidjson::Document& json)
			{ json["main_menu"]["options_status_position"]["y"].SetFloat(menu.options_position.y + menu.options_spacing.y * 6.0f); });
		// Compact Options geometry makes the old shared-art right edges legal
		// reader anchors. Font-sized paper containment is checked separately.
		const Presentation compact_boundary = variants.load([&](rapidjson::Document& json)
		{
			json["main_menu"]["options_heading_position"]["x"].SetFloat(
				(1920.0f + menu.options_box_size.x) / 2.0f - menu.art.shadow_offset - menu.art.keyline_width * 3.0f);
			json["main_menu"]["options_value_x"].SetFloat(
				(1920.0f + menu.options_box_size.x) / 2.0f - menu.art.shadow_offset - menu.art.focus_padding.x);
		});
		require(compact_boundary.main_menu().options_heading_position.x == 1760.0f &&
			compact_boundary.main_menu().options_value_x == 1744.0f,
			"Compact Options reader rejected the former shared-art right boundaries");
		for (const Anchors pair : { Anchors{ "heading_position", "position" },
			Anchors{ "position", "back_position" } })
		{
			rejected(pair.later, [&](rapidjson::Document& json)
			{
				auto& credits = json["main_menu"]["credits"];
				credits[pair.later]["y"].SetFloat(credits[pair.earlier]["y"].GetFloat());
			});
		}
		for (const double value : { 0.0, 40.0, 1080.001, 1.0e100 })
		{
			rejected("footer_y", [&](rapidjson::Document& json)
				{ json["menu_style"]["chrome"]["footer_y"].SetDouble(value); });
		}

		// Maximum panels keep their design margins. Measured text fitting is
		// checked by the actual renderer rather than guessed by this reader.
		const Presentation boundary = variants.load([&](rapidjson::Document& json)
		{
			auto& main = json["main_menu"];
			for (const char* key : { "title_box_size", "home_box_size", "options_box_size", "credits_box_size" })
			{
				main[key]["x"].SetFloat(1760.0f);
				main[key]["y"].SetFloat(content_height - 40.0f);
			}
			main["options_spacing"]["x"].SetFloat(1920.0f);
			main["art"]["disabled_ink"].SetString("60616AFF", json.GetAllocator());
			main["options_warning_colour"].SetString("8A301CFF", json.GetAllocator());
		});
		const auto& valid = boundary.main_menu();
		require(valid.title_box_size == mattmath::Vector2F(1760.0f, content_height - 40.0f) &&
			valid.home_box_size == valid.title_box_size && valid.options_box_size == valid.title_box_size &&
			valid.credits_box_size == valid.title_box_size && valid.options_spacing.x == 1920.0f &&
			valid.art.disabled_ink == menu.art.disabled_ink &&
			valid.options_warning_colour == menu.options_warning_colour,
			"Legal main-menu panel boundaries or opaque ink colours were changed");
	}
}
