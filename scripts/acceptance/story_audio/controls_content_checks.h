#pragma once

// Controls/audio keeps eleven rows, a separate status and a measured help column.
// Shared paper-colour and card-shape limits are covered by the end-menu cases.
namespace
{
	void check_controls_art_content(World& world, const std::filesystem::path& content)
	{
		const MenuPresentationVariant variants(content);
		const Presentation shipped = variants.load([](rapidjson::Document&) {});
		const auto& menu = shipped.controls_settings();
		const float content_height = shipped.menu().footer_y;
		const auto rejected = [&](const char* key, auto edit)
		{
			std::string diagnostic = "Presentation accepted the variant";
			bool named = false;
			try
			{
				const Presentation look = variants.load(edit);
				look.validate_resources(world.render, world.audio);
			}
			catch (const std::runtime_error& error)
			{
				diagnostic = error.what();
				named = diagnostic.find("controls_settings") != std::string::npos &&
					diagnostic.find(key) != std::string::npos;
			}
			if (!named)
			{
				throw std::runtime_error(std::string("Invalid controls/audio presentation field '") + key +
					"' was accepted or lost its key: " + diagnostic);
			}
		};
		rejected("art", [](rapidjson::Document& json) { json["controls_settings"].RemoveMember("art"); });
		rejected("art", [](rapidjson::Document& json) { json["controls_settings"]["art"].SetArray(); });
		rejected("focus_ink", [](rapidjson::Document& json)
			{ json["controls_settings"]["art"].RemoveMember("focus_ink"); });
		rejected("art", [](rapidjson::Document& json)
			{ json["controls_settings"]["art"]["paper"].SetString("F0F0ED80", json.GetAllocator()); });
		rejected("focus_padding", [&](rapidjson::Document& json)
			{ json["controls_settings"]["art"]["focus_padding"]["y"].SetFloat(menu.art.shadow_offset + menu.art.keyline_width); });

		for (const char* key : { "box_size", "heading_position", "controls_position", "controls_size", "status_position" })
		{
			rejected(key, [&](rapidjson::Document& json) { json["controls_settings"].RemoveMember(key); });
			rejected(key, [&](rapidjson::Document& json) { json["controls_settings"][key].SetNull(); });
			for (const char* axis : { "x", "y" })
			{
				rejected(key, [&](rapidjson::Document& json) { json["controls_settings"][key].RemoveMember(axis); });
				for (const double value : { -0.001, 0.0, 1.0e100 })
				{
					rejected(key, [&](rapidjson::Document& json)
						{ json["controls_settings"][key][axis].SetDouble(value); });
				}
			}
		}
		for (const char* key : { "position", "size", "spacing" })
		{
			rejected(key, [&](rapidjson::Document& json) { json["controls_settings"]["layout"].RemoveMember(key); });
			rejected(key, [&](rapidjson::Document& json) { json["controls_settings"]["layout"][key].SetNull(); });
			for (const char* axis : { "x", "y" })
			{
				rejected(key, [&](rapidjson::Document& json)
					{ json["controls_settings"]["layout"][key].RemoveMember(axis); });
				for (const double value : { -0.001, 0.0, 1.0e100 })
				{
					rejected(key, [&](rapidjson::Document& json)
						{ json["controls_settings"]["layout"][key][axis].SetDouble(value); });
				}
			}
		}
		for (const char* key : { "item_font", "help_font" })
		{
			rejected(key, [&](rapidjson::Document& json) { json["controls_settings"].RemoveMember(key); });
			rejected(key, [&](rapidjson::Document& json) { json["controls_settings"][key].SetNull(); });
			for (const char* value : { "", "missing.controls_font" })
			{
				rejected(key, [&](rapidjson::Document& json)
					{ json["controls_settings"][key].SetString(value, json.GetAllocator()); });
			}
		}
		for (const char* axis : { "x", "y" })
		{
			rejected("box_size", [&](rapidjson::Document& json)
			{
				json["controls_settings"]["box_size"][axis].SetDouble(
					axis[0] == 'x' ? 1760.001 : static_cast<double>(content_height - 40.0f) + 0.001);
			});
			rejected("box_size", [&](rapidjson::Document& json)
			{
				json["controls_settings"]["box_size"][axis].SetFloat(
					menu.art.shadow_offset + menu.art.keyline_width * 6.0f);
			});
			const float extent = axis[0] == 'x' ? 1920.0f : content_height;
			const float length = axis[0] == 'x' ? menu.box_size.x : menu.box_size.y;
			const float edge = menu.art.keyline_width * 3.0f;
			for (const char* key : { "heading_position", "controls_position", "status_position" })
			{
				for (const float value : { (extent - length) / 2.0f + edge,
					(extent + length) / 2.0f - menu.art.shadow_offset - edge })
				{
					rejected(key, [&](rapidjson::Document& json)
						{ json["controls_settings"][key][axis].SetFloat(value); });
				}
			}
			rejected("controls_size", [&](rapidjson::Document& json)
			{
				const float origin = axis[0] == 'x' ? menu.controls_position.x : menu.controls_position.y;
				json["controls_settings"]["controls_size"][axis].SetFloat(
					(extent + length) / 2.0f - menu.art.shadow_offset - edge - origin);
			});
		}
		rejected("heading_position", [&](rapidjson::Document& json)
			{ json["controls_settings"]["heading_position"]["y"].SetFloat(menu.layout.position.y); });
		rejected("heading_position", [&](rapidjson::Document& json)
			{ json["controls_settings"]["heading_position"]["y"].SetFloat(menu.controls_position.y); });
		rejected("status_position", [&](rapidjson::Document& json)
		{
			json["controls_settings"]["status_position"]["y"].SetFloat(menu.layout.position.y +
				menu.layout.spacing.y * 8.0f + menu.layout.size.y + menu.art.focus_padding.y + menu.art.shadow_offset);
		});
		rejected("status_position", [&](rapidjson::Document& json)
			{ json["controls_settings"]["status_position"]["x"].SetFloat(menu.controls_position.x); });
		const float status_right = (1920.0f + menu.box_size.x) / 2.0f - menu.art.shadow_offset;
		const float narrow_help_x = status_right - menu.art.keyline_width * 3.0f - 6.0f;
		for (const float status_x : { status_right - menu.art.focus_padding.x, narrow_help_x - 1.0f })
		{
			rejected("status_position", [&](rapidjson::Document& json)
			{
				auto& controls = json["controls_settings"];
				controls["controls_position"]["x"].SetFloat(narrow_help_x);
				controls["controls_size"]["x"].SetFloat(1.0f);
				controls["status_position"]["x"].SetFloat(status_x);
			});
		}
		rejected("layout.spacing", [&](rapidjson::Document& json)
			{ json["controls_settings"]["layout"]["spacing"]["x"].SetFloat(menu.layout.size.x + menu.art.focus_padding.x); });
		rejected("layout.spacing", [&](rapidjson::Document& json)
		{
			json["controls_settings"]["layout"]["spacing"]["y"].SetFloat(
				menu.layout.size.y + menu.art.focus_padding.y * 2.0f + menu.art.shadow_offset - 0.001f);
		});
		rejected("controls_position", [&](rapidjson::Document& json)
		{
			json["controls_settings"]["controls_position"]["x"].SetFloat(menu.layout.position.x +
				menu.layout.spacing.x + menu.layout.size.x + menu.art.focus_padding.x + menu.art.shadow_offset);
		});
		rejected("layout.spacing", [&](rapidjson::Document& json)
		{
			json["controls_settings"]["layout"]["spacing"]["y"].SetFloat(
				((content_height + menu.box_size.y) / 2.0f - menu.layout.position.y) / 8.0f);
		});

		// The stick preview shares this page's authored right column. Reject
		// negative drawing geometry, escaped markers, overlapping regions and
		// misplaced readouts before opening either settings page.
		rejected("stick_preview", [](rapidjson::Document& json)
			{ json["controls_settings"].RemoveMember("stick_preview"); });
		rejected("stick_preview", [](rapidjson::Document& json)
			{ json["controls_settings"]["stick_preview"].SetArray(); });
		for (const char* key : { "marker_size", "line_width" })
		{
			rejected(key, [&](rapidjson::Document& json)
				{ json["controls_settings"]["stick_preview"].RemoveMember(key); });
			rejected(key, [&](rapidjson::Document& json)
				{ json["controls_settings"]["stick_preview"][key].SetString("3", json.GetAllocator()); });
			for (const double value : { -0.001, 0.0, 1.0e100 })
			{
				rejected(key, [&](rapidjson::Document& json)
					{ json["controls_settings"]["stick_preview"][key].SetDouble(value); });
			}
		}
		rejected("marker_size", [](rapidjson::Document& json)
			{ json["controls_settings"]["stick_preview"]["marker_size"].SetFloat(24.001f); });
		rejected("line_width", [](rapidjson::Document& json)
			{ json["controls_settings"]["stick_preview"]["line_width"].SetFloat(8.001f); });
		for (const float marker : { menu.stick_preview.line_width,
			menu.stick_preview.line_width * 2.0f })
		{
			rejected("marker_size", [&](rapidjson::Document& json)
				{ json["controls_settings"]["stick_preview"]["marker_size"].SetFloat(marker); });
		}
		for (const char* key : { "raw_movement", "movement_response", "aim" })
		{
			rejected(key, [&](rapidjson::Document& json)
				{ json["controls_settings"]["stick_preview"].RemoveMember(key); });
			rejected(key, [&](rapidjson::Document& json)
				{ json["controls_settings"]["stick_preview"][key].SetNull(); });
			for (const char* field : { "position", "size" })
			{
				rejected(key, [&](rapidjson::Document& json)
					{ json["controls_settings"]["stick_preview"][key].RemoveMember(field); });
				rejected(key, [&](rapidjson::Document& json)
					{ json["controls_settings"]["stick_preview"][key][field].SetNull(); });
				for (const char* axis : { "x", "y" })
				{
					rejected(key, [&](rapidjson::Document& json)
						{ json["controls_settings"]["stick_preview"][key][field].RemoveMember(axis); });
					for (const double value : { -0.001, 0.0, 1.0e100 })
					{
						rejected(key, [&](rapidjson::Document& json)
							{ json["controls_settings"]["stick_preview"][key][field][axis].SetDouble(value); });
					}
				}
			}
			rejected(key, [&](rapidjson::Document& json)
				{ json["controls_settings"]["stick_preview"][key]["position"]["x"].SetFloat(menu.controls_position.x - 1.0f); });
			rejected(key, [&](rapidjson::Document& json)
				{ json["controls_settings"]["stick_preview"][key]["size"]["x"].SetFloat(menu.controls_size.x + 1.0f); });
			rejected("marker_size", [&](rapidjson::Document& json)
				{ json["controls_settings"]["stick_preview"][key]["size"]["x"].SetFloat(menu.stick_preview.marker_size * 2.0f); });
			rejected("line_width", [&](rapidjson::Document& json)
				{ json["controls_settings"]["stick_preview"][key]["size"]["y"].SetFloat(menu.stick_preview.line_width * 2.0f); });
		}
		rejected("aim", [&](rapidjson::Document& json)
			{ json["controls_settings"]["stick_preview"]["aim"]["size"]["y"].SetFloat(menu.stick_preview.aim.height - 1.0f); });
		for (const char* key : { "aim_readout_position", "association_position" })
		{
			rejected(key, [&](rapidjson::Document& json)
				{ json["controls_settings"]["stick_preview"].RemoveMember(key); });
			rejected(key, [&](rapidjson::Document& json)
				{ json["controls_settings"]["stick_preview"][key].SetNull(); });
			for (const char* axis : { "x", "y" })
			{
				rejected(key, [&](rapidjson::Document& json)
					{ json["controls_settings"]["stick_preview"][key].RemoveMember(axis); });
				for (const double value : { -0.001, 0.0, 1.0e100 })
				{
					rejected(key, [&](rapidjson::Document& json)
						{ json["controls_settings"]["stick_preview"][key][axis].SetDouble(value); });
				}
				rejected(key, [&](rapidjson::Document& json)
					{ json["controls_settings"]["stick_preview"][key][axis].SetFloat(axis[0] == 'x' ?
						menu.controls_position.x + menu.controls_size.x : menu.controls_position.y + menu.controls_size.y); });
			}
		}
		rejected("raw_movement", [&](rapidjson::Document& json)
			{ json["controls_settings"]["stick_preview"]["raw_movement"]["position"]["y"].SetFloat(menu.controls_position.y); });
		rejected("movement_response", [&](rapidjson::Document& json)
			{ json["controls_settings"]["stick_preview"]["movement_response"]["position"]["y"].SetFloat(menu.stick_preview.raw_movement.bottom()); });
		rejected("aim", [&](rapidjson::Document& json)
			{ json["controls_settings"]["stick_preview"]["aim"]["position"]["y"].SetFloat(menu.stick_preview.movement_response.bottom()); });
		rejected("association_position", [&](rapidjson::Document& json)
			{ json["controls_settings"]["stick_preview"]["association_position"]["y"].SetFloat(menu.stick_preview.aim.bottom()); });
		rejected("aim_readout_position", [&](rapidjson::Document& json)
			{ json["controls_settings"]["stick_preview"]["aim_readout_position"]["x"].SetFloat(menu.stick_preview.aim.right()); });
		for (const float y : { menu.stick_preview.aim.y - 1.0f, menu.stick_preview.aim.bottom() })
		{
			rejected("aim_readout_position", [&](rapidjson::Document& json)
				{ json["controls_settings"]["stick_preview"]["aim_readout_position"]["y"].SetFloat(y); });
		}
		const Presentation widest_preview = variants.load([](rapidjson::Document& json)
		{
			json["controls_settings"]["stick_preview"]["marker_size"].SetFloat(24.0f);
			json["controls_settings"]["stick_preview"]["line_width"].SetFloat(8.0f);
		});
		require(widest_preview.controls_settings().stick_preview.marker_size == 24.0f &&
			widest_preview.controls_settings().stick_preview.line_width == 8.0f,
			"Valid maximum stick-preview marker and line widths were rejected");

		// The maximum paper size preserves the menu margins; the last row and its
		// focus card still end above the status. Text fitting uses real fonts.
		require(menu.box_size == mattmath::Vector2F(1760.0f, content_height - 40.0f) &&
			menu.layout.position.y + menu.layout.spacing.y * 10.0f + menu.layout.size.y +
				menu.art.focus_padding.y + menu.art.shadow_offset < menu.status_position.y &&
			menu.controls_position.x + menu.controls_size.x < (1920.0f + menu.box_size.x) / 2.0f &&
			menu.controls_position.y + menu.controls_size.y < (content_height + menu.box_size.y) / 2.0f,
			"Shipped controls/audio rows or help escaped their paper or overlapped status");
	}
}
