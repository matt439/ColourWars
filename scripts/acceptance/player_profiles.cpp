#include "game/pch.h"
#include "game/save.h"
#include "engine/assets/json.h"
#include "scripts/acceptance/support/probe.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

// Player profiles (ROAD#21): the name rules, the "profiles" save section with
// its per-field recovery, and Save's overlay of a worn profile onto its pad
// slot, which is what lets a person's settings follow them between pads.
namespace
{
	using acceptance::require;

	template <typename Function>
	void rejects(Function function, const char* message)
	{
		try
		{
			function();
		}
		catch (const std::invalid_argument&)
		{
			return;
		}
		throw std::runtime_error(message);
	}

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
			require(GetTempFileNameW(temporary.c_str(), L"cwp", 0, unique) != 0,
				"Could not reserve the acceptance directory");
			this->root_ = unique;
			require(DeleteFileW(unique) != 0 && std::filesystem::create_directory(this->root_),
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

		void write(const std::string& text) const
		{
			std::filesystem::create_directories(this->save_path().parent_path());
			std::ofstream output(this->save_path(), std::ios::binary | std::ios::trunc);
			output << text;
			require(output.good(), "Could not write the profile save fixture");
		}

	private:
		std::filesystem::path root_;
		std::wstring previous_;
	};

	Save load()
	{
		Save save;
		save.load_save_file();
		return save;
	}

	void check_names()
	{
		using player_profiles::normalized_name;
		require(normalized_name("  ALEX ") == "ALEX" && normalized_name("P1 2") == "P1 2" &&
			normalized_name("ABCDEFGHIJ") == "ABCDEFGHIJ", "Profile names are not trimmed as written");
		rejects([] { normalized_name("   "); }, "A blank profile name was accepted");
		rejects([] { normalized_name("ABCDEFGHIJK"); }, "An eleven-character profile name was accepted");
		rejects([] { normalized_name("alex"); }, "A lowercase profile name was accepted");
		rejects([] { normalized_name("AL-X"); }, "A profile name with punctuation was accepted");
		require(player_profiles::name_characters().front() == ' ' &&
			player_profiles::name_characters().size() == 37, "The name editor's characters changed");

		PlayerProfiles profiles;
		require(profiles.next_free_name() == "PLAYER 1", "The first default name is not PLAYER 1");
		profiles.list.push_back({ "PLAYER 1" });
		profiles.list.push_back({ "PLAYER 3" });
		require(profiles.next_free_name() == "PLAYER 2", "The default name does not fill the first gap");
		profiles.validate();
		profiles.list.push_back({ "PLAYER 1" });
		rejects([&] { profiles.validate(); }, "Two profiles with one name validated");
		profiles.list.back().name = "SAM";
		profiles.list.back().boost = handicaps::count;
		rejects([&] { profiles.validate(); }, "A profile with an unknown Boost validated");
	}

	// The worn profile replaces its slot's values in what every borrower reads,
	// and writes through the usual setters land in the profile, not the slot.
	void check_overlay(const IsolatedSaveDirectory& directory)
	{
		std::filesystem::remove(directory.save_path());
		Save save = load();
		const PlaySettings* borrowed = &save.play_settings();
		const InputBindings* borrowed_bindings = &save.input_bindings();

		PlaySettings pad_two = save.play_settings();
		pad_two.sticks[1] = { 25, 40 };
		pad_two.pad_rumble[1] = false;
		pad_two.pad_comfort[1].view_shake = SeatSwitch::off;
		require(save.set_play_settings_and_save(pad_two), "Could not seed pad 2's own settings");
		InputBindings buttons = save.input_bindings();
		buttons.pads[1][PadAction::jump] = { PadControl::b, PadControl::none };
		require(save.set_input_bindings_and_save(buttons), "Could not seed pad 2's own buttons");

		// A new profile carries the creating pad's settings.
		bool saved = false;
		const int alex = save.add_profile_and_save(" ALEX", 1, saved);
		require(saved && alex == 0 && save.profiles().list[0].name == "ALEX", "Could not add a profile");
		const PlayerProfile& copied = save.profiles().list[0];
		require(copied.sticks.movement_deadzone == 25 && copied.sticks.aim_deadzone == 40 && !copied.rumble &&
			copied.comfort.view_shake == SeatSwitch::off && copied.buttons[PadAction::jump].primary == PadControl::b,
			"A new profile did not carry its pad's settings");
		const int sam = save.add_profile_and_save("SAM", input_slots::keyboard_and_mouse, saved);
		require(sam == 1 && save.profiles().list[1].buttons == PadProfile{} && save.profiles().list[1].rumble,
			"A profile made from the keyboard did not start from the defaults");
		rejects([&] { save.add_profile_and_save("ALEX", 0, saved); }, "A taken profile name was added");

		// Pad 4 wears ALEX: it reads pad 2's old settings, pad 2 is unchanged.
		require(save.wear_profile(3, alex) && save.worn_profile(3) == alex && save.profile_wearer(alex) == 3,
			"Pad 4 could not wear ALEX");
		require(&save.play_settings() == borrowed && &save.input_bindings() == borrowed_bindings,
			"Wearing a profile moved the preferences borrowers hold");
		require(borrowed->stick_settings(3).movement_deadzone == 25 && !borrowed->rumble_enabled(3) &&
			!borrowed->view_shake_enabled(3) && borrowed_bindings->pads[3][PadAction::jump].primary == PadControl::b,
			"Pad 4 does not read the profile it wears");
		require(borrowed->stick_settings(1).movement_deadzone == 25, "Wearing changed another pad");
		require(!save.wear_profile(0, alex), "Two pads wore one profile");
		require(!save.wear_profile(input_slots::keyboard_and_mouse, sam), "The keyboard wore a profile");
		require(!save.wear_profile(0, 7), "A pad wore an unknown profile");

		// Editing pad 4 through the usual pages edits ALEX and leaves pad 4's own.
		PlaySettings edited = save.play_settings();
		edited.sticks[3].aim_deadzone = 60;
		edited.pad_rumble[3] = true;
		require(save.set_play_settings_and_save(edited), "Could not edit the worn slot");
		InputBindings edited_buttons = save.input_bindings();
		edited_buttons.pads[3][PadAction::shoot] = { PadControl::right_shoulder, PadControl::none };
		require(save.set_input_bindings_and_save(edited_buttons), "Could not edit the worn slot's buttons");
		require(save.profiles().list[0].sticks.aim_deadzone == 60 && save.profiles().list[0].rumble &&
			save.profiles().list[0].buttons[PadAction::shoot].primary == PadControl::right_shoulder,
			"An edit on the worn slot did not reach its profile");
		const SaveData own = save.save_data();
		require(own.play.sticks[3].aim_deadzone == -1 && own.play.pad_rumble[3] &&
			own.input_bindings.pads[3] == PadProfile{}, "An edit on the worn slot changed the pad's own settings");

		// Taking it off restores the pad's own; the profile keeps the edits.
		require(save.wear_profile(3, -1) && save.worn_profile(3) == -1, "Pad 4 could not take ALEX off");
		require(borrowed->stick_settings(3).aim_deadzone == -1 && borrowed_bindings->pads[3] == PadProfile{},
			"Taking a profile off did not restore the pad's own settings");

		// Profiles persist; who wears them does not.
		require(save.wear_profile(0, sam), "Pad 1 could not wear SAM");
		Save reloaded = load();
		require(reloaded.profiles() == save.profiles(), "Profiles did not survive reloading");
		require(reloaded.worn_profile(0) == -1, "Which pad wears a profile was remembered across launches");
		require(reloaded.save_data().play.sticks[1].movement_deadzone == 25,
			"A pad's own settings did not survive beside the profiles");
	}

	void check_cycling_and_delete(const IsolatedSaveDirectory& directory)
	{
		std::filesystem::remove(directory.save_path());
		Save save = load();
		bool saved = false;
		require(save.next_wearable_profile(0, 1) == -1, "A pad found a profile in an empty list");
		for (const char* name : { "ALEX", "SAM", "JO" }) { save.add_profile_and_save(name, -1, saved); }
		require(save.next_wearable_profile(0, 1) == 0 && save.next_wearable_profile(0, -1) == 2,
			"Cycling from none does not reach the first and last profiles");
		require(save.wear_profile(1, 1), "Pad 2 could not wear SAM");
		require(save.wear_profile(0, 0) && save.next_wearable_profile(0, 1) == 2,
			"Cycling did not skip a profile another pad wears");
		require(save.wear_profile(0, 2) && save.next_wearable_profile(0, 1) == -1,
			"Cycling past the last profile does not reach none");
		require(save.wear_profile(1, -1) && save.wear_profile(1, 1), "Pad 2 could not re-wear SAM");

		// Deleting ALEX shifts SAM and JO down; their wearers follow them.
		require(save.delete_profile_and_save(0), "Could not delete ALEX");
		require(save.profiles().list.size() == 2 && save.worn_profile(1) == 0 && save.worn_profile(0) == 1 &&
			save.profiles().list[0].name == "SAM", "Deleting a profile lost track of who wears the rest");
		require(save.delete_profile_and_save(0) && save.worn_profile(1) == -1 && save.worn_profile(0) == 0,
			"Deleting a worn profile left its pad wearing it");
		rejects([&] { save.delete_profile_and_save(4); }, "An unknown profile was deleted");

		require(save.rename_profile_and_save(0, "JOJO") && save.profiles().list[0].name == "JOJO",
			"Could not rename a profile");
		rejects([&] { save.rename_profile_and_save(0, "jo"); }, "A profile took an unsupported name");
		save.add_profile_and_save("KIM", -1, saved);
		rejects([&] { save.rename_profile_and_save(1, "JOJO"); }, "A profile took another's name");
		require(save.rename_profile_and_save(1, "KIM"), "A profile could not keep its own name");

		// Boosts: the setter, then a match carrying the seat's choice back.
		require(save.set_profile_boost_and_save(1, 2) && save.profiles().list[1].boost == 2, "Could not set Boost");
		rejects([&] { save.set_profile_boost_and_save(1, handicaps::count); }, "An unknown Boost was set");
		require(save.wear_profile(2, 1), "Pad 3 could not wear KIM");
		require(save.remember_profile_boosts_and_save({ 1, -1, 0, 2 }), "Could not remember Boosts");
		require(save.profiles().list[0].boost == 1 && save.profiles().list[1].boost == 0,
			"A match's Boost did not reach the worn profiles");

		while (!save.profiles().full())
		{
			save.add_profile_and_save(save.profiles().next_free_name(), -1, saved);
		}
		rejects([&] { save.add_profile_and_save("ONE MORE", -1, saved); }, "A ninth profile was added");
	}

	void check_recovery(const IsolatedSaveDirectory& directory)
	{
		directory.write("{\"save_version\":4,\"profiles\":["
			"{\"name\":\" ALEX \",\"movement_deadzone\":30,\"aim_deadzone\":95,\"rumble\":\"no\","
			"\"view_shake\":\"on\",\"aim_marker\":\"maybe\",\"reduce_effects\":\"off\","
			"\"buttons\":{\"jump\":[\"x\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]},\"boost\":2},"
			"{\"name\":\"ALEX\"},"
			"{\"name\":\"lower\"},"
			"{\"movement_deadzone\":10},"
			"7,"
			"{\"name\":\"SAM\",\"buttons\":{\"jump\":[\"a\"],\"shoot\":[\"a\"],\"pause\":[\"start\"]},\"boost\":9},"
			"{\"name\":\"P3\"},{\"name\":\"P4\"},{\"name\":\"P5\"},{\"name\":\"P6\"},"
			"{\"name\":\"P7\"},{\"name\":\"P8\"},{\"name\":\"P9\"}],"
			"\"aim_sensitivity\":4}");
		const Save save = load();
		const PlayerProfiles& profiles = save.profiles();
		require(profiles.list.size() == player_profiles::max_count, "Recovery did not stop at the limit");
		require(profiles.list[0].name == "ALEX" && profiles.list[1].name == "SAM" &&
			profiles.list[7].name == "P8", "Recovery kept the wrong entries");
		const PlayerProfile& alex = profiles.list[0];
		require(alex.sticks.movement_deadzone == 30 && alex.sticks.aim_deadzone == -1 && alex.rumble &&
			alex.comfort.view_shake == SeatSwitch::on && alex.comfort.aim_marker == SeatSwitch::shared &&
			alex.comfort.reduce_effects == SeatSwitch::off && alex.buttons[PadAction::jump].primary == PadControl::x &&
			alex.boost == 2, "A profile's fields did not recover independently");
		require(profiles.list[1].buttons == PadProfile{} && profiles.list[1].boost == 0,
			"An invalid button set or Boost was kept");
		require(save.play_settings().aim_sensitivity == 4, "A profile entry disturbed another section");
		profiles.validate();

		directory.write("{\"profiles\":{\"name\":\"ALEX\"},\"aim_sensitivity\":2}");
		const Save malformed = load();
		require(malformed.profiles().list.empty() && malformed.play_settings().aim_sensitivity == 2,
			"A malformed profile list was not reset alone");
	}
}

int main()
{
	return acceptance::run_probe("player profiles", [&]
	{
		IsolatedSaveDirectory directory;
		check_names();
		check_overlay(directory);
		check_cycling_and_delete(directory);
		check_recovery(directory);
		std::cout << "PASS player_profiles: name rules, profiles carrying sticks, rumble, comfort, "
			"buttons and Boost, the worn-profile overlay and its write-through, session-only wearing, "
			"cycling, delete and rename, Boost carry-back, and per-field recovery\n";
		return 0;
	});
}
