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
}
