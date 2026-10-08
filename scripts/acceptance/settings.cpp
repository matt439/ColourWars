#include "game/pch.h"
#include "game/save.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace
{
	void require(bool condition, const char* message)
	{
		if (!condition)
		{
			throw std::runtime_error(message);
		}
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
		Save restored;
		restored.load_save_file();
		require(restored.resolution() == expected.resolution() &&
			restored.fullscreen() == expected.fullscreen() &&
			restored.team_palette() == expected.team_palette(),
			"Display preferences did not survive reloading");
		const PlaySettings& actual = restored.play_settings();
		const PlaySettings& wanted = expected.play_settings();
		require(actual.aim_sensitivity == wanted.aim_sensitivity &&
			actual.music_volume == wanted.music_volume &&
			actual.sfx_volume == wanted.sfx_volume,
			"Shared play preferences did not survive reloading");
	}
}

int main()
{
	try
	{
		const IsolatedSaveDirectory directory;
		Save save;
		PlaySettings play;
		play.aim_sensitivity = 5;
		play.music_volume = 30;
		play.sfx_volume = 70;
		require(save.set_play_settings_and_save(play), "Initial play save failed");
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
		const bool play_saved = save.set_play_settings_and_save(play);
		CloseHandle(locked);
		require(!display_saved && !play_saved,
			"Persistence failure was reported as success");
		require(save.resolution() == labrador::ScreenResolution::s_1280_720 &&
			!save.fullscreen() && save.team_palette() == TeamColourPair::pink_green &&
			save.play_settings().music_volume == 0,
			"Failed saves lost the active settings");
		require(contents(directory.save_path()) == original,
			"Failed replacement damaged the previous save");
		require(!std::filesystem::exists(directory.save_path().wstring() + L".tmp"),
			"Failed replacement left a temporary save");
		require(save.save_to_file(), "Save did not recover after the lock was released");
		check_round_trip(save);
		std::cout << "PASS settings: bulk apply, validation, persistence failure, recovery\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL settings: " << error.what() << '\n';
		return 1;
	}
}
