#pragma once

// Included after story_audio.cpp's shared recording world.
namespace
{
	class MenuPresentationVariant
	{
	public:
		explicit MenuPresentationVariant(const std::filesystem::path& content)
		{
			std::ifstream stream(content / "presentation.json");
			require(stream.good(), "Cannot read menu-audio acceptance presentation");
			this->source_ = std::string(std::istreambuf_iterator<char>(stream), {});
			wchar_t name[MAX_PATH]{};
			require(GetTempFileNameW(std::filesystem::temp_directory_path().c_str(), L"cwm", 0, name) != 0,
				"Cannot reserve menu-audio acceptance presentation file");
			this->path_ = name;
		}

		~MenuPresentationVariant()
		{
			std::error_code ignored;
			std::filesystem::remove(this->path_, ignored);
		}

		template<typename Edit>
		Presentation load(Edit edit) const
		{
			rapidjson::Document json;
			json.Parse(this->source_.c_str());
			require(!json.HasParseError(), "Cannot parse menu-audio acceptance presentation");
			edit(json);
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			json.Accept(writer);
			std::ofstream stream(this->path_, std::ios::binary | std::ios::trunc);
			stream.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
			stream.close();
			require(stream.good(), "Cannot write menu-audio acceptance presentation variant");
			return Presentation(this->path_.string().c_str());
		}

	private:
		std::string source_;
		std::filesystem::path path_;
	};

	void check_menu_audio_content(World& world, const std::filesystem::path& content)
	{
		world.presentation.validate_resources(world.render, world.audio);
		const MenuPresentationVariant variants(content);
		struct Slot { const char* owner; const char* key; };
		for (const Slot slot : { Slot{ "main_menu", "unready" },
			Slot{ "controls_settings", "volume_preview" }, Slot{ "pause_menu", "pad_lost" },
			Slot{ "pause_menu", "pad_back" }, Slot{ "end_menu", "series" } })
		{
			const auto rejected = [&](auto edit)
			{
				bool named = false;
				try
				{
					const Presentation look = variants.load([&](rapidjson::Document& json)
						{ edit(json[slot.owner]["sounds"][slot.key], json.GetAllocator()); });
					look.validate_resources(world.render, world.audio);
				}
				catch (const std::runtime_error& error)
				{
					const std::string message = error.what();
					named = message.find(slot.owner) != std::string::npos &&
						message.find(slot.key) != std::string::npos;
				}
				require(named, "Invalid menu-audio cue did not fail at startup with its field named");
			};
			rejected([](rapidjson::Value& sound, auto&) { sound["volume"].SetFloat(-0.01f); });
			rejected([](rapidjson::Value& sound, auto&) { sound["volume"].SetFloat(1.01f); });
			rejected([](rapidjson::Value& sound, auto& allocator)
				{ sound["name"].SetString("", allocator); });
			rejected([](rapidjson::Value& sound, auto& allocator)
				{ sound["name"].SetString("missing.menu_audio", allocator); });
			for (const float volume : { 0.0f, 1.0f })
			{
				const Presentation look = variants.load([&](rapidjson::Document& json)
					{ json[slot.owner]["sounds"][slot.key]["volume"].SetFloat(volume); });
				look.validate_resources(world.render, world.audio);
			}
		}
	}

	void check_foreground_art_content(const std::filesystem::path& content)
	{
		const MenuPresentationVariant variants(content);
		struct Invalid { const char* section; const char* key; float value; };
		for (const Invalid invalid : { Invalid{ "silhouette_art", "pen_width", 0.0f },
			Invalid{ "silhouette_art", "pen_width", 0.0251f },
			Invalid{ "silhouette_art", "shade_width", 0.007f },
			Invalid{ "silhouette_art", "shade_width", 0.0801f },
			Invalid{ "cursor_art", "shadow_offset", -0.01f },
			Invalid{ "cursor_art", "shadow_offset", 0.201f },
			Invalid{ "cursor_art", "click_duration", 0.0f },
			Invalid{ "cursor_art", "click_duration", 0.501f },
			Invalid{ "cursor_art", "click_reach", 0.0f },
			Invalid{ "cursor_art", "click_reach", 0.601f } })
		{
			bool named = false;
			try
			{
				std::ignore = variants.load([&](rapidjson::Document& json)
					{ json["opening"][invalid.section][invalid.key].SetFloat(invalid.value); });
			}
			catch (const std::runtime_error& error)
			{
				const std::string message = error.what();
				named = message.find("opening") != std::string::npos &&
					message.find(invalid.section) != std::string::npos;
			}
			require(named, "Invalid foreground artwork did not fail with opening and its section named");
		}

		for (const float shade_width : { 0.025f, 0.08f })
		{
			const Presentation look = variants.load([&](rapidjson::Document& json)
			{
				auto& opening = json["opening"];
				opening["silhouette_art"]["pen_width"].SetFloat(0.025f);
				opening["silhouette_art"]["shade_width"].SetFloat(shade_width);
				opening["cursor_art"]["shadow_offset"].SetFloat(shade_width == 0.025f ? 0.0f : 0.2f);
				opening["cursor_art"]["click_duration"].SetFloat(0.5f);
				opening["cursor_art"]["click_reach"].SetFloat(0.6f);
			});
			require(look.opening().silhouette_art.shade_width == shade_width,
				"Valid foreground artwork boundary was not retained");
		}
	}

	void check_countdown_art_content(const std::filesystem::path& content)
	{
		const MenuPresentationVariant variants(content);
		const Presentation defaults = variants.load([](rapidjson::Document&) {});
		const MatchPresentation& layout = defaults.match();
		const auto stage_entry_rejected = [&](auto edit, const char* key, bool cross_field = false)
		{
			bool named = false;
			try
			{
				std::ignore = variants.load([&](rapidjson::Document& json)
					{ edit(json["match"], json.GetAllocator()); });
			}
			catch (const std::runtime_error& error)
			{
				const std::string message = error.what();
				named = message.find("match") != std::string::npos &&
					message.find(key) != std::string::npos &&
					(!cross_field || (message.find("stage_entry_size") != std::string::npos &&
						message.find("stage_entry_top") != std::string::npos));
			}
			require(named, "Invalid stage-entry layout did not fail with its match field named");
		};
		for (const double invalid : { 0.0, -1.0, 1.0e100 })
		{
			for (const char* axis : { "x", "y" })
			{
				stage_entry_rejected([&](rapidjson::Value& match, auto&)
					{ match["stage_entry_size"][axis].SetDouble(invalid); }, "stage_entry_size");
			}
			stage_entry_rejected([&](rapidjson::Value& match, auto&)
				{ match["stage_entry_top"].SetDouble(invalid); }, "stage_entry_top");
		}
		for (const char* key : { "stage_entry_size", "stage_entry_top" })
		{
			stage_entry_rejected([&](rapidjson::Value& match, auto&) { match.RemoveMember(key); }, key);
			stage_entry_rejected([&](rapidjson::Value& match, auto& allocator)
				{ match[key].SetString("invalid layout", allocator); }, key);
		}
		stage_entry_rejected([&](rapidjson::Value& match, auto&)
			{ match["stage_entry_size"]["x"].SetFloat(layout.controls_padding.x * 2.0f +
				layout.countdown_art.shadow_offset); }, "stage_entry_size", true);
		stage_entry_rejected([&](rapidjson::Value& match, auto&)
			{ match["stage_entry_size"]["y"].SetFloat(layout.controls_padding.y * 2.0f +
				layout.countdown_art.shadow_offset); }, "stage_entry_size", true);
		stage_entry_rejected([&](rapidjson::Value& match, auto&)
			{ match["stage_entry_size"]["x"].SetFloat(layout.reference_size.x -
				layout.controls_margin.x * 2.0f + 1.0f); }, "stage_entry_size", true);
		stage_entry_rejected([&](rapidjson::Value& match, auto&)
			{ match["stage_entry_top"].SetFloat(layout.reference_size.y * layout.countdown_center_y -
				layout.countdown_size.y / 2.0f - layout.stage_entry_size.y - layout.controls_gap + 1.0f); },
			"stage_entry_top", true);
		for (const char* axis : { "x", "y" })
		{
			stage_entry_rejected([&](rapidjson::Value& match, auto&)
			{
				match["controls_padding"][axis].SetFloat(1.0f);
				match["stage_entry_size"][axis].SetFloat(layout.countdown_art.keyline_width * 6.0f +
					layout.countdown_art.shadow_offset);
			}, "stage_entry_size", true);
		}
		const auto rejected = [&](auto edit, const char* key = "countdown_art")
		{
			bool named = false;
			try
			{
				std::ignore = variants.load([&](rapidjson::Document& json)
					{ edit(json["match"]["countdown_art"], json.GetAllocator()); });
			}
			catch (const std::runtime_error& error)
			{
				const std::string message = error.what();
				named = message.find("match") != std::string::npos &&
					message.find("countdown_art") != std::string::npos &&
					message.find(key) != std::string::npos;
			}
			require(named, "Invalid countdown artwork did not fail with its match field named");
		};
		struct Invalid { const char* key; float value; };
		for (const Invalid invalid : { Invalid{ "keyline_width", -0.01f },
			Invalid{ "keyline_width", 0.0f }, Invalid{ "keyline_width", 8.001f },
			Invalid{ "shadow_offset", -0.01f }, Invalid{ "shadow_offset", 16.001f },
			Invalid{ "beat_duration", -0.01f }, Invalid{ "beat_duration", 0.0f },
			Invalid{ "beat_duration", 0.501f },
			Invalid{ "pulse", -0.01f }, Invalid{ "pulse", 0.151f } })
		{
			rejected([&](rapidjson::Value& art, auto&)
				{ art[invalid.key].SetFloat(invalid.value); }, invalid.key);
		}
		for (const char* key : { "keyline_width", "shadow_offset", "beat_duration", "pulse" })
		{
			rejected([&](rapidjson::Value& art, auto& allocator)
				{ art[key].SetString("not a number", allocator); }, key);
			rejected([&](rapidjson::Value& art, auto&) { art.RemoveMember(key); }, key);
		}
		for (const char* key : { "ink", "paper", "shade", "number", "go" })
		{
			rejected([&](rapidjson::Value& art, auto& allocator)
				{ art[key].SetString("not a colour", allocator); });
			rejected([&](rapidjson::Value& art, auto&) { art[key].SetInt(0); }, key);
			rejected([&](rapidjson::Value& art, auto&) { art.RemoveMember(key); }, key);
		}
		rejected([](rapidjson::Value& art, auto&) { art.SetNull(); });

		for (const bool upper : { false, true })
		{
			const float keyline_width = upper ? 8.0f : 0.001f;
			const float shadow_offset = upper ? 16.0f : 0.0f;
			const float beat_duration = upper ? 0.5f : 0.001f;
			const float pulse = upper ? 0.15f : 0.0f;
			const Presentation look = variants.load([&](rapidjson::Document& json)
			{
				auto& art = json["match"]["countdown_art"];
				art["keyline_width"].SetFloat(keyline_width);
				art["shadow_offset"].SetFloat(shadow_offset);
				art["beat_duration"].SetFloat(beat_duration);
				art["pulse"].SetFloat(pulse);
				for (const char* key : { "ink", "paper", "shade", "number", "go" })
				{
					art[key].SetString(upper ? "A0B1C280" : "WHITE", json.GetAllocator());
				}
			});
			const auto& art = look.match().countdown_art;
			require(art.keyline_width == keyline_width && art.shadow_offset == shadow_offset &&
				art.beat_duration == beat_duration && art.pulse == pulse,
				"Valid countdown artwork boundary was not retained");
		}
	}
}
