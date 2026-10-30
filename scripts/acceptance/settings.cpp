#include "game/pch.h"
#include "game/save.h"
#include "game/settings_session.h"
#include "scripts/acceptance/support/probe.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
	using acceptance::require;

	// Save reads LOCALAPPDATA through the CRT. This process never opens the
	// player's real preferences, and removes only the paths it creates here.
	class IsolatedSaveDirectory
	{
	public:
		IsolatedSaveDirectory()
		{
			wchar_t* previous = nullptr;
			size_t length = 0;
			require(_wdupenv_s(&previous, &length, L"LOCALAPPDATA") == 0,
				"Could not read LOCALAPPDATA");
			if (previous != nullptr)
			{
				this->previous_ = previous;
				std::free(previous);
			}

			const std::filesystem::path temporary = std::filesystem::temp_directory_path();
			wchar_t unique[MAX_PATH] = {};
			require(GetTempFileNameW(temporary.c_str(), L"cws", 0, unique) != 0,
				"Could not reserve the acceptance directory");
			this->root_ = unique;
			require(DeleteFileW(unique) != 0 &&
				std::filesystem::create_directory(this->root_),
				"Could not create the acceptance directory");
			require(_wputenv_s(L"LOCALAPPDATA", this->root_.c_str()) == 0,
				"Could not isolate LOCALAPPDATA");
		}

		~IsolatedSaveDirectory()
		{
			_wputenv_s(L"LOCALAPPDATA", this->previous_.c_str());
			std::error_code ignored;
			std::filesystem::remove(this->save_path().wstring() + L".tmp", ignored);
			std::filesystem::remove(this->save_path(), ignored);
			std::filesystem::remove(this->root_ / L"ColourWars", ignored);
			std::filesystem::remove(this->root_, ignored);
		}

		std::filesystem::path save_path() const
		{
			return this->root_ / L"ColourWars" / L"save_data.json";
		}

	private:
		std::filesystem::path root_;
		std::wstring previous_;
	};

	std::string contents(const std::filesystem::path& path)
	{
		std::ifstream stream(path, std::ios::binary);
		require(stream.good(), "Could not read the acceptance save");
		return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
	}

	void check_round_trip(const Save& expected)
	{
		Save restored; SettingsSession restored_session(restored);
		restored_session.load_save_file();
		require(restored.resolution() == expected.resolution() &&
			restored.fullscreen() == expected.fullscreen() &&
			restored.team_palette() == expected.team_palette(),
			"Display preferences did not survive reloading");
		const PlaySettings& actual = restored_session.play_settings();
		const PlaySettings& wanted = expected.saved_play_settings();
		require(actual.aim_sensitivity == wanted.aim_sensitivity &&
			actual.music_volume == wanted.music_volume &&
			actual.sfx_volume == wanted.sfx_volume &&
			actual.view_shake == wanted.view_shake &&
			actual.aim_marker == wanted.aim_marker &&
			actual.reduce_effects == wanted.reduce_effects && actual.pad_rumble == wanted.pad_rumble &&
			actual.pad_comfort == wanted.pad_comfort,
			"Shared play preferences did not survive reloading");
		for (int slot = 0; slot < input_slots::pad_count; ++slot)
		{
			require(actual.stick_settings(slot).movement_deadzone ==
				wanted.stick_settings(slot).movement_deadzone &&
				actual.stick_settings(slot).aim_deadzone ==
				wanted.stick_settings(slot).aim_deadzone,
				"A pad slot's stick preferences did not survive reloading");
		}
	}

	void write_stick_fixture(const IsolatedSaveDirectory& directory,
		const std::string& stick_member, int sensitivity = 4)
	{
		std::ofstream output(directory.save_path(), std::ios::binary | std::ios::trunc);
		output << "{\"resolution\":{\"x\":1280,\"y\":720},\"fullscreen\":true,"
			"\"team_palette\":\"blue_yellow\",\"aim_sensitivity\":" << sensitivity <<
			",\"music_volume\":20,\"sfx_volume\":80,\"view_shake\":false,"
			"\"aim_marker\":false,\"reduce_effects\":true" << stick_member << '}';
		require(output.good(), "Could not write the stick-preference save fixture");
	}

	void require_fixture_preferences(const Save& restored)
	{
		const PlaySettings& play = restored.saved_play_settings();
		require(restored.resolution() == labrador::ScreenResolution::s_1280_720 &&
			restored.fullscreen() && restored.team_palette() == TeamColourPair::blue_yellow &&
			play.aim_sensitivity == 4 && play.music_volume == 20 && play.sfx_volume == 80 &&
			!play.view_shake && !play.aim_marker && play.reduce_effects,
			"Stick-preference recovery reset an unrelated saved preference");
	}

	void check_stick_loading(const IsolatedSaveDirectory& directory)
	{
		struct Fixture
		{
			const char* value;
			int expected;
		};
		const Fixture fixtures[] = {
			{ nullptr, -1 }, { "-1", -1 }, { "0", 0 }, { "90", 90 },
			{ "null", -1 }, { "true", -1 }, { "false", -1 }, { "\"25\"", -1 },
			{ "[]", -1 }, { "{}", -1 }, { "1.5", -1 }, { "-2", -1 },
			{ "91", -1 }, { "2147483648", -1 }
		};
		for (const bool movement : { false, true })
		{
			const char* key = movement ? "movement_deadzone" : "aim_deadzone";
			const char* other = movement ? "aim_deadzone" : "movement_deadzone";
			for (const Fixture& fixture : fixtures)
			{
				std::string profile = std::string("{\"") + other + "\":55";
				if (fixture.value != nullptr)
				{
					profile += std::string(",\"") + key + "\":" + fixture.value;
				}
				profile += '}';
				write_stick_fixture(directory, ",\"pad_sticks\":["
					"{\"movement_deadzone\":10,\"aim_deadzone\":15}," + profile + ',' +
					"{\"movement_deadzone\":35,\"aim_deadzone\":45},"
					"{\"movement_deadzone\":70,\"aim_deadzone\":80}]");
				Save restored; SettingsSession restored_session(restored);
				restored_session.load_save_file();
				require_fixture_preferences(restored);
				const PlaySettings& play = restored_session.play_settings();
				require(play.stick_settings(1).movement_deadzone == (movement ? fixture.expected : 55) &&
					play.stick_settings(1).aim_deadzone == (movement ? 55 : fixture.expected),
					"Missing, invalid or boundary stick field changed its sibling or used the wrong fallback");
				require(play.stick_settings(0).movement_deadzone == 10 &&
					play.stick_settings(0).aim_deadzone == 15 &&
					play.stick_settings(2).movement_deadzone == 35 &&
					play.stick_settings(2).aim_deadzone == 45 &&
					play.stick_settings(3).movement_deadzone == 70 &&
					play.stick_settings(3).aim_deadzone == 80,
					"One corrupt pad field changed another XInput slot's preferences");
				require(restored.save_to_file(), "Could not save recovered stick preferences");
				check_round_trip(restored);
			}
		}

		for (const char* member : { "", ",\"pad_sticks\":null", ",\"pad_sticks\":true",
			",\"pad_sticks\":{}", ",\"pad_sticks\":\"pads\"", ",\"pad_sticks\":[]" })
		{
			write_stick_fixture(directory, member);
			Save restored; SettingsSession restored_session(restored);
			restored_session.load_save_file();
			require_fixture_preferences(restored);
			for (int slot = 0; slot < input_slots::pad_count; ++slot)
			{
				require(restored_session.play_settings().stick_settings(slot).movement_deadzone == -1 &&
					restored_session.play_settings().stick_settings(slot).aim_deadzone == -1,
					"Missing or malformed stick profile container did not inherit defaults");
			}
		}

		write_stick_fixture(directory, ",\"pad_sticks\":[null,"
			"{\"movement_deadzone\":25,\"aim_deadzone\":40},true,"
			"{\"movement_deadzone\":65,\"aim_deadzone\":75},"
			"{\"movement_deadzone\":90,\"aim_deadzone\":90}]");
		Save recovered; SettingsSession recovered_session(recovered);
		recovered_session.load_save_file();
		require_fixture_preferences(recovered);
		const PlaySettings& recovered_play = recovered_session.play_settings();
		require(recovered_play.stick_settings(0).movement_deadzone == -1 &&
			recovered_play.stick_settings(0).aim_deadzone == -1 &&
			recovered_play.stick_settings(1).movement_deadzone == 25 &&
			recovered_play.stick_settings(1).aim_deadzone == 40 &&
			recovered_play.stick_settings(2).movement_deadzone == -1 &&
			recovered_play.stick_settings(2).aim_deadzone == -1 &&
			recovered_play.stick_settings(3).movement_deadzone == 65 &&
			recovered_play.stick_settings(3).aim_deadzone == 75,
			"Invalid or excess array entries changed valid XInput slot profiles");
		require(recovered.save_to_file(), "Could not save recovered profile array");
		check_round_trip(recovered);

		write_stick_fixture(directory, ",\"pad_sticks\":[{\"aim_deadzone\":20}]");
		Save shortened; SettingsSession shortened_session(shortened);
		shortened_session.load_save_file();
		require_fixture_preferences(shortened);
		require(shortened_session.play_settings().stick_settings(0).movement_deadzone == -1 &&
			shortened_session.play_settings().stick_settings(0).aim_deadzone == 20,
			"Partial first profile did not retain its valid field");
		for (int slot = 1; slot < input_slots::pad_count; ++slot)
		{
			require(shortened_session.play_settings().stick_settings(slot).movement_deadzone == -1 &&
				shortened_session.play_settings().stick_settings(slot).aim_deadzone == -1,
				"Absent trailing profile did not inherit defaults");
		}
	}

	void check_legacy_sticks(const IsolatedSaveDirectory& directory)
	{
		constexpr float legacy_deadzones[] = { 0.70f, 0.60f, 0.50f, 0.35f, 0.20f };
		for (int sensitivity = 1; sensitivity <= 5; ++sensitivity)
		{
			write_stick_fixture(directory, "", sensitivity);
			Save restored; SettingsSession restored_session(restored);
			restored_session.load_save_file();
			const PlaySettings& play = restored_session.play_settings();
			for (int slot = 0; slot < input_slots::pad_count; ++slot)
			{
				require(play.stick_settings(slot).movement_deadzone == -1 &&
					play.stick_settings(slot).aim_deadzone == -1 &&
					play.movement_deadzone(slot, 0.275f) == 0.275f &&
					play.aim_deadzone(slot) == legacy_deadzones[sensitivity - 1],
					"A legacy save changed its authored movement or shared aim deadzone");
			}
			require(restored.save_to_file(), "Could not round-trip a legacy aim preference");
			check_round_trip(restored);
			Save canonical; SettingsSession canonical_session(canonical);
			canonical_session.load_save_file();
			for (int slot = 0; slot < input_slots::pad_count; ++slot)
			{
				require(canonical_session.play_settings().aim_deadzone(slot) == legacy_deadzones[sensitivity - 1],
					"Saving legacy preferences changed an inherited aim deadzone");
			}
		}
	}

	void check_stick_api(const IsolatedSaveDirectory& directory)
	{
		Save save; SettingsSession save_session(save);
		PlaySettings play;
		play.sticks[0] = { 10, 20 };
		play.sticks[1] = { 30, 40 };
		play.sticks[2] = { 50, 60 };
		play.sticks[3] = { 70, 80 };
		require(save_session.set_play_settings_and_save(play), "Could not save four separate pad profiles");
		check_round_trip(save);
		require(contents(directory.save_path()).find("\"pad_sticks\":[") != std::string::npos,
			"Saving pad profiles did not write their canonical array");

		play.sticks[0] = { -999, 999 };
		play.sticks[1] = { 999, -999 };
		play.sticks[2] = { 0, 90 };
		play.sticks[3] = { 90, 0 };
		require(save_session.set_play_settings_and_save(play), "Could not save normalized pad profiles");
		const PlaySettings& normalized = save_session.play_settings();
		require(normalized.stick_settings(0).movement_deadzone == -1 &&
			normalized.stick_settings(0).aim_deadzone == 90 &&
			normalized.stick_settings(1).movement_deadzone == 90 &&
			normalized.stick_settings(1).aim_deadzone == -1 &&
			normalized.stick_settings(2).movement_deadzone == 0 &&
			normalized.stick_settings(2).aim_deadzone == 90 &&
			normalized.stick_settings(3).movement_deadzone == 90 &&
			normalized.stick_settings(3).aim_deadzone == 0,
			"Pad preference API did not normalize to inherit or the supported 0..90 percent range");
		check_round_trip(save);
		for (const int slot : { -1, input_slots::keyboard_and_mouse, input_slots::count,
			(std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)() })
		{
			require(normalized.stick_settings(slot).movement_deadzone == -1 &&
				normalized.stick_settings(slot).aim_deadzone == -1 &&
				normalized.movement_deadzone(slot, 0.275f) == 0.275f &&
				normalized.aim_deadzone(slot) == normalized.aim_deadzone(),
				"Invalid or keyboard slot did not safely inherit global stick defaults");
		}
	}

	void check_rumble_loading(const IsolatedSaveDirectory& directory)
	{
		const auto check = [&](const std::string& member, const std::array<bool, input_slots::pad_count>& expected)
		{
			write_stick_fixture(directory, ",\"pad_sticks\":[{\"movement_deadzone\":25,\"aim_deadzone\":35}]" + member);
			Save restored; SettingsSession restored_session(restored);
			restored_session.load_save_file();
			require_fixture_preferences(restored);
			require(restored_session.play_settings().pad_rumble == expected &&
				restored_session.play_settings().sticks[0].movement_deadzone == 25 &&
				restored_session.play_settings().sticks[0].aim_deadzone == 35,
				"Rumble recovery lost physical-slot association or reset unrelated stick preferences");
			for (int slot = 0; slot < input_slots::pad_count; ++slot)
			{
				require(restored_session.play_settings().rumble_enabled(slot) == expected[static_cast<size_t>(slot)],
					"Rumble preference accessor read a different device slot");
			}
			require(restored.save_to_file(), "Could not canonicalize the rumble save fixture");
			std::string canonical = "\"pad_rumble\":[";
			for (size_t slot = 0; slot < expected.size(); ++slot)
			{
				if (slot != 0) { canonical += ','; }
				canonical += expected[slot] ? "{\"enabled\":true}" : "{\"enabled\":false}";
			}
			canonical += ']';
			require(contents(directory.save_path()).find(canonical) != std::string::npos,
				"Rumble save did not write four canonical typed entries");
			check_round_trip(restored);
		};
		for (const char* member : { "", ",\"pad_rumble\":null", ",\"pad_rumble\":true",
			",\"pad_rumble\":0", ",\"pad_rumble\":{}", ",\"pad_rumble\":\"pads\"", ",\"pad_rumble\":[]" })
		{
			check(member, { true, true, true, true });
		}
		check(",\"pad_rumble\":[{\"enabled\":false}]", { false, true, true, true });
		check(",\"pad_rumble\":[{\"enabled\":false},null,{\"enabled\":false},{\"enabled\":\"false\"},{\"enabled\":false}]",
			{ false, true, false, true });
		for (const char* malformed : { "null", "false", "0", "\"false\"", "[]", "{}",
			"{\"enabled\":null}", "{\"enabled\":0}", "{\"enabled\":1}", "{\"enabled\":\"false\"}" })
		{
			for (int slot = 0; slot < input_slots::pad_count; ++slot)
			{
				std::string member = ",\"pad_rumble\":[";
				std::array<bool, input_slots::pad_count> expected{};
				for (int entry = 0; entry < input_slots::pad_count; ++entry)
				{
					if (entry != 0) { member += ','; }
					member += entry == slot ? malformed : "{\"enabled\":false}";
					expected[static_cast<size_t>(entry)] = entry == slot;
				}
				member += ']';
				check(member, expected);
			}
		}
		for (const int slot : { -1, input_slots::keyboard_and_mouse, input_slots::count,
			(std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)() })
		{
			require(!PlaySettings{}.rumble_enabled(slot), "Keyboard or invalid slot was eligible for rumble");
		}
	}

	// Per-slot comfort overrides: their resolution against the shared switches,
	// then field-independent recovery by physical slot and canonical writing.
	void check_comfort_resolution()
	{
		PlaySettings play;
		for (int slot = 0; slot < input_slots::pad_count; ++slot)
		{
			require(play.seat_comfort(slot) == SeatComfort{} && play.view_shake_enabled(slot) &&
				play.aim_marker_enabled(slot) && !play.reduce_effects_for(slot),
				"A pad slot does not follow the shared comfort switches by default");
		}
		play.pad_comfort[1] = { SeatSwitch::off, SeatSwitch::off, SeatSwitch::shared };
		play.pad_comfort[2] = { SeatSwitch::shared, SeatSwitch::shared, SeatSwitch::on };
		require(!play.view_shake_enabled(1) && !play.aim_marker_enabled(1) && !play.reduce_effects_for(1) &&
			!play.view_shake_enabled(2) && play.aim_marker_enabled(2) && play.reduce_effects_for(2) &&
			play.view_shake_enabled(0) && play.aim_marker_enabled(0) && !play.reduce_effects_for(0) &&
			play.view_shake_enabled(3) && play.aim_marker_enabled(3) && !play.reduce_effects_for(3),
			"One slot's comfort override reached another slot");
		play.view_shake = false;
		play.aim_marker = false;
		play.reduce_effects = true;
		play.pad_comfort[3] = { SeatSwitch::on, SeatSwitch::on, SeatSwitch::off };
		require(!play.view_shake_enabled(0) && !play.aim_marker_enabled(0) && play.reduce_effects_for(0) &&
			play.view_shake_enabled(3) && play.aim_marker_enabled(3) && !play.reduce_effects_for(3),
			"A slot override did not win over, or a shared slot did not follow, the Options switches");
		play.pad_comfort[3].reduce_effects = SeatSwitch::on;
		require(!play.view_shake_enabled(3) && play.aim_marker_enabled(3),
			"A slot's own Reduce effects did not suppress its own view shake");
		for (const int slot : { -1, input_slots::keyboard_and_mouse, input_slots::count,
			(std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)() })
		{
			require(play.seat_comfort(slot) == SeatComfort{} && !play.view_shake_enabled(slot) &&
				!play.aim_marker_enabled(slot) && play.reduce_effects_for(slot),
				"The keyboard, a bot or an invalid slot did not follow the shared switches");
		}
	}

	void check_comfort_loading(const IsolatedSaveDirectory& directory)
	{
		using Comfort = std::array<SeatComfort, input_slots::pad_count>;
		const auto key = [](SeatSwitch value)
		{
			return value == SeatSwitch::off ? "off" : (value == SeatSwitch::on ? "on" : "shared");
		};
		const auto check = [&](const std::string& member, const Comfort& expected)
		{
			write_stick_fixture(directory, ",\"pad_sticks\":[{\"movement_deadzone\":25,\"aim_deadzone\":35}]"
				",\"pad_rumble\":[{\"enabled\":false}]" + member);
			Save restored; SettingsSession restored_session(restored);
			restored_session.load_save_file();
			require_fixture_preferences(restored);
			require(restored_session.play_settings().pad_comfort == expected &&
				restored_session.play_settings().sticks[0].movement_deadzone == 25 &&
				!restored_session.play_settings().pad_rumble[0] && restored_session.play_settings().pad_rumble[1],
				"Comfort recovery lost physical-slot association or reset unrelated pad preferences");
			require(restored.save_to_file(), "Could not canonicalize the comfort save fixture");
			std::string canonical = "\"pad_comfort\":[";
			for (size_t slot = 0; slot < expected.size(); ++slot)
			{
				if (slot != 0) { canonical += ','; }
				canonical += std::string("{\"view_shake\":\"") + key(expected[slot].view_shake) +
					"\",\"aim_marker\":\"" + key(expected[slot].aim_marker) +
					"\",\"reduce_effects\":\"" + key(expected[slot].reduce_effects) + "\"}";
			}
			canonical += ']';
			require(contents(directory.save_path()).find(canonical) != std::string::npos,
				"Comfort save did not write four canonical entries");
			check_round_trip(restored);
		};
		for (const char* member : { "", ",\"pad_comfort\":null", ",\"pad_comfort\":true",
			",\"pad_comfort\":{}", ",\"pad_comfort\":\"off\"", ",\"pad_comfort\":[]" })
		{
			check(member, Comfort{});
		}
		Comfort first{};
		first[0] = { SeatSwitch::off, SeatSwitch::on, SeatSwitch::shared };
		check(",\"pad_comfort\":[{\"view_shake\":\"off\",\"aim_marker\":\"on\",\"reduce_effects\":\"shared\"}]", first);
		// Each field recovers alone: a bad value keeps shared, its neighbours load.
		Comfort mixed{};
		mixed[0].aim_marker = SeatSwitch::on;
		mixed[0].reduce_effects = SeatSwitch::on;
		mixed[2] = { SeatSwitch::off, SeatSwitch::off, SeatSwitch::off };
		mixed[3].view_shake = SeatSwitch::on;
		check(",\"pad_comfort\":[{\"view_shake\":false,\"aim_marker\":\"on\",\"reduce_effects\":\"on\"},"
			"null,{\"view_shake\":\"off\",\"aim_marker\":\"off\",\"reduce_effects\":\"off\"},"
			"{\"view_shake\":\"on\",\"aim_marker\":\"On\",\"reduce_effects\":1},"
			"{\"view_shake\":\"off\"}]", mixed);
		for (const char* malformed : { "null", "false", "0", "\"\"", "\"OFF\"", "\"yes\"", "[]", "{}" })
		{
			for (int slot = 0; slot < input_slots::pad_count; ++slot)
			{
				std::string member = ",\"pad_comfort\":[";
				Comfort expected{};
				for (int entry = 0; entry < input_slots::pad_count; ++entry)
				{
					if (entry != 0) { member += ','; }
					member += std::string("{\"view_shake\":") + (entry == slot ? malformed : "\"on\"") +
						",\"aim_marker\":\"off\",\"reduce_effects\":\"on\"}";
					expected[static_cast<size_t>(entry)] = { entry == slot ? SeatSwitch::shared : SeatSwitch::on,
						SeatSwitch::off, SeatSwitch::on };
				}
				member += ']';
				check(member, expected);
			}
		}
	}

	void check_boolean_loading(const IsolatedSaveDirectory& directory,
		const char* key, bool PlaySettings::* preference,
		const char* other_key, bool PlaySettings::* other_preference)
	{
		struct Fixture
		{
			const char* value;
			bool expected;
		};
		const bool default_value = PlaySettings{}.*preference;
		const bool other_value = !(PlaySettings{}.*other_preference);
		const Fixture fixtures[] = {
			{ nullptr, default_value },
			{ "true", true },
			{ "false", false },
			{ "null", default_value },
			{ "0", default_value },
			{ "1", default_value },
			{ "\"false\"", default_value },
			{ "[]", default_value },
			{ "{}", default_value }
		};
		for (const Fixture& fixture : fixtures)
		{
			{
				std::ofstream output(directory.save_path(), std::ios::binary | std::ios::trunc);
				output << "{\"resolution\":{\"x\":1280,\"y\":720},\"fullscreen\":true,"
					"\"team_palette\":\"blue_yellow\",\"aim_sensitivity\":4,"
					"\"music_volume\":20,\"sfx_volume\":80,\"" << other_key << "\":" <<
					(other_value ? "true" : "false");
				if (fixture.value != nullptr)
				{
					output << ",\"" << key << "\":" << fixture.value;
				}
				output << '}';
				require(output.good(), "Could not write the boolean-preference save fixture");
			}
			Save restored; SettingsSession restored_session(restored);
			restored_session.load_save_file();
			const PlaySettings& play = restored_session.play_settings();
			require(play.*preference == fixture.expected,
				"Absent or invalid boolean preference did not use its default");
			require(restored.resolution() == labrador::ScreenResolution::s_1280_720 &&
				restored.fullscreen() && restored.team_palette() == TeamColourPair::blue_yellow &&
				play.aim_sensitivity == 4 && play.music_volume == 20 && play.sfx_volume == 80 &&
				play.*other_preference == other_value,
				"Boolean-preference recovery reset an unrelated saved preference");
			require(restored.save_to_file(), "Could not canonicalize the boolean-preference save fixture");
			const std::string canonical = std::string("\"") + key + "\":" +
				(fixture.expected ? "true" : "false");
			require(contents(directory.save_path()).find(canonical) != std::string::npos,
				"Saving did not write the preference as a boolean");
			check_round_trip(restored);
		}
	}
}

int main()
{
	return acceptance::run_probe("settings", [&]
	{
		const IsolatedSaveDirectory directory;
		Save save; SettingsSession save_session(save);
		PlaySettings play;
		require(play.pad_rumble == std::array<bool, input_slots::pad_count>{ true, true, true, true },
			"Rumble is not enabled independently on all four pad slots by default");
		play.pad_rumble = { false, true, false, true };
		play.aim_sensitivity = 5;
		play.music_volume = 30;
		play.sfx_volume = 70;
		require(play.view_shake, "View shake is not enabled by default");
		require(play.aim_marker, "Aim marker is not enabled by default");
		require(!play.reduce_effects, "Reduce effects is not disabled by default");
		require(play.view_shake_enabled(), "Default settings suppressed view shake");
		play.reduce_effects = true;
		require(!play.view_shake_enabled() && play.view_shake,
			"Reduce effects did not suppress shake while retaining the individual preference");
		play.reduce_effects = false;
		play.view_shake = false;
		require(!play.view_shake_enabled(), "Individual view-shake setting did not suppress shake");
		play.reduce_effects = true;
		require(!play.view_shake_enabled(), "Reduce effects re-enabled disabled view shake");
		play.aim_marker = false;
		require(save_session.set_play_settings_and_save(play), "Initial play save failed");
		require(save.set_display_settings_and_save(
			labrador::ScreenResolution::s_2560_1440, true, TeamColourPair::blue_yellow),
			"Bulk display save failed");
		check_round_trip(save);
		const std::string original = contents(directory.save_path());

		bool rejected = false;
		try
		{
			save.set_display_settings_and_save(labrador::ScreenResolution::s_1280_720,
				false, static_cast<TeamColourPair>(999));
		}
		catch (const std::invalid_argument&)
		{
			rejected = true;
		}
		require(rejected && save.resolution() == labrador::ScreenResolution::s_2560_1440 &&
			save.fullscreen() && save.team_palette() == TeamColourPair::blue_yellow &&
			contents(directory.save_path()) == original,
			"Invalid bulk settings partially changed active or saved preferences");

		// Prevent replacing the destination while still allowing the temporary
		// write. This exercises the failure after serialization, not a mock.
		const HANDLE locked = CreateFileW(directory.save_path().c_str(), GENERIC_READ,
			FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		require(locked != INVALID_HANDLE_VALUE, "Could not lock the acceptance save");
		const bool display_saved = save.set_display_settings_and_save(
			labrador::ScreenResolution::s_1280_720, false, TeamColourPair::pink_green);
		play.music_volume = 0;
		play.view_shake = true;
		play.aim_marker = true;
		play.reduce_effects = false;
		play.pad_rumble = { true, false, true, false };
		const bool play_saved = save_session.set_play_settings_and_save(play);
		CloseHandle(locked);
		require(!display_saved && !play_saved,
			"Persistence failure was reported as success");
		require(save.resolution() == labrador::ScreenResolution::s_1280_720 &&
			!save.fullscreen() && save.team_palette() == TeamColourPair::pink_green &&
			save_session.play_settings().music_volume == 0 && save_session.play_settings().view_shake &&
			save_session.play_settings().aim_marker && !save_session.play_settings().reduce_effects &&
			save_session.play_settings().pad_rumble == play.pad_rumble,
			"Failed saves lost the active settings");
		require(contents(directory.save_path()) == original,
			"Failed replacement damaged the previous save");
		require(!std::filesystem::exists(directory.save_path().wstring() + L".tmp"),
			"Failed replacement left a temporary save");
		require(save.save_to_file(), "Save did not recover after the lock was released");
		check_round_trip(save);
		check_boolean_loading(directory, "view_shake", &PlaySettings::view_shake,
			"aim_marker", &PlaySettings::aim_marker);
		check_boolean_loading(directory, "aim_marker", &PlaySettings::aim_marker,
			"reduce_effects", &PlaySettings::reduce_effects);
		check_boolean_loading(directory, "reduce_effects", &PlaySettings::reduce_effects,
			"view_shake", &PlaySettings::view_shake);
		check_stick_loading(directory);
		check_legacy_sticks(directory);
		check_stick_api(directory);
		check_rumble_loading(directory);
		check_comfort_resolution();
		check_comfort_loading(directory);
		std::cout << "PASS settings: bulk apply, validation, persistence failure, recovery, "
			"boolean recovery, four pad profiles, typed field recovery, legacy aim inheritance, "
			"stick normalization, independent per-slot rumble recovery and persistence, "
			"per-slot comfort overrides, their resolution and field-independent recovery\n";
		return 0;
	});
}
