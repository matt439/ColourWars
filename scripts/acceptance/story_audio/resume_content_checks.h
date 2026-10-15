#pragma once

// Included after menu_content_checks.h inside the recording world's namespace.
void check_resume_content(World& world, const std::filesystem::path& content)
{
	const MenuPresentationVariant variants(content);
	const auto rejected = [&](auto edit, const char* field)
	{
		bool named = false;
		try
		{
			const Presentation look = variants.load(edit);
			look.validate_resources(world.render, world.audio);
		}
		catch (const std::runtime_error& error)
		{
			const std::string message = error.what();
			named = message.find("pause_menu") != std::string::npos &&
				message.find(field) != std::string::npos;
		}
		require(named, "Invalid resume content did not fail at startup with its field named");
	};
	for (const double duration : { -1.0, 0.0, 3.001, 1.0e100 })
	{
		rejected([&](rapidjson::Document& json)
			{ json["pause_menu"]["resume_countdown"].SetDouble(duration); }, "resume_countdown");
	}
	rejected([](rapidjson::Document& json)
		{ json["pause_menu"].RemoveMember("resume_countdown"); }, "resume_countdown");
	for (int type = 0; type < 5; ++type)
	{
		rejected([&](rapidjson::Document& json)
		{
			auto& duration = json["pause_menu"]["resume_countdown"];
			switch (type)
			{
			case 0: duration.SetNull(); break;
			case 1: duration.SetBool(false); break;
			case 2: duration.SetString("two seconds", json.GetAllocator()); break;
			case 3: duration.SetArray(); break;
			case 4: duration.SetObject(); break;
			}
		}, "resume_countdown");
	}
	for (const double volume : { -0.001, 1.001, 1.0e100 })
	{
		rejected([&](rapidjson::Document& json)
			{ json["pause_menu"]["sounds"]["resume"]["volume"].SetDouble(volume); }, "resume");
	}
	for (const char* name : { "", "missing.resume_count_in" })
	{
		rejected([&](rapidjson::Document& json)
			{ json["pause_menu"]["sounds"]["resume"]["name"].SetString(name, json.GetAllocator()); },
			"resume");
	}
	for (const char* field : { "name", "volume" })
	{
		rejected([&](rapidjson::Document& json)
			{ json["pause_menu"]["sounds"]["resume"].RemoveMember(field); }, "resume");
		for (int type = 0; type < 5; ++type)
		{
			rejected([&](rapidjson::Document& json)
			{
				auto& value = json["pause_menu"]["sounds"]["resume"][field];
				switch (type)
				{
				case 0: value.SetNull(); break;
				case 1: value.SetBool(false); break;
				case 2:
					if (std::string_view(field) == "name") { value.SetInt(0); }
					else { value.SetString("quiet", json.GetAllocator()); }
					break;
				case 3: value.SetArray(); break;
				case 4: value.SetObject(); break;
				}
			}, "resume");
		}
	}
	rejected([](rapidjson::Document& json)
		{ json["pause_menu"]["sounds"].RemoveMember("resume"); }, "resume");
	for (const bool upper : { false, true })
	{
		const float duration = upper ? 3.0f : 0.001f;
		const float volume = upper ? 1.0f : 0.0f;
		const Presentation look = variants.load([&](rapidjson::Document& json)
		{
			json["pause_menu"]["resume_countdown"].SetFloat(duration);
			json["pause_menu"]["sounds"]["resume"]["volume"].SetFloat(volume);
		});
		look.validate_resources(world.render, world.audio);
		require(look.pause_menu().resume_countdown == duration &&
			look.pause_menu().resume_sound.volume == volume,
			"Valid resume duration and gain boundaries were not retained");
	}
}
