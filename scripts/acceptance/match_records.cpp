#include "game/pch.h"
#include "game/save.h"
#include "game/match_records.h"
#include "game/objects/level_end_info.h"
#include "game/states/results_menu.h"
#include "scripts/acceptance/support/probe.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <io.h>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>

// Per-stage records and the sectioned, versioned save they live in. Runs on
// CPU data only, against an isolated LOCALAPPDATA.
namespace
{
	using acceptance::require;

	// Save reads LOCALAPPDATA through the CRT. This process never opens the
	// player's real save, and removes only the paths it creates here.
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
			require(GetTempFileNameW(temporary.c_str(), L"cwr", 0, unique) != 0,
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
			for (int copy = 1; copy <= 9; ++copy) { std::filesystem::remove(this->side_copy(copy), ignored); }
			std::filesystem::remove(this->root_ / L"ColourWars", ignored);
			std::filesystem::remove(this->root_, ignored);
		}

		std::filesystem::path save_path() const
		{
			return this->root_ / L"ColourWars" / L"save_data.json";
		}

		// Where Save keeps its nth copy of an unreadable save.
		std::filesystem::path side_copy(int copy) const
		{
			return this->root_ / L"ColourWars" / (L"save_data.unreadable-" + std::to_wstring(copy) + L".json");
		}

		void write(const std::string& json) const
		{
			std::filesystem::create_directories(this->save_path().parent_path());
			std::ofstream output(this->save_path(), std::ios::binary | std::ios::trunc);
			output << json;
			require(output.good(), "Could not write a save fixture");
		}

		std::string contents() const
		{
			std::ifstream stream(this->save_path(), std::ios::binary);
			require(stream.good(), "Could not read the acceptance save");
			return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
		}

	private:
		std::filesystem::path root_;
		std::wstring previous_;
	};

	// The flat version-0 layout every existing save has.
	const std::string flat_preferences =
		"\"resolution\":{\"x\":1280,\"y\":720},\"fullscreen\":true,"
		"\"team_palette\":\"blue_yellow\",\"aim_sensitivity\":4,"
		"\"pad_sticks\":[{\"movement_deadzone\":10,\"aim_deadzone\":15},"
		"{\"movement_deadzone\":20,\"aim_deadzone\":25},"
		"{\"movement_deadzone\":30,\"aim_deadzone\":35},"
		"{\"movement_deadzone\":40,\"aim_deadzone\":45}],"
		"\"music_volume\":20,\"sfx_volume\":80,\"view_shake\":false,"
		"\"aim_marker\":false,\"reduce_effects\":true";

	void require_flat_preferences(const Save& save, const char* message)
	{
		const PlaySettings& play = save.saved_play_settings();
		require(save.resolution() == labrador::ScreenResolution::s_1280_720 &&
			save.fullscreen() && save.team_palette() == TeamColourPair::blue_yellow &&
			play.aim_sensitivity == 4 && play.music_volume == 20 && play.sfx_volume == 80 &&
			!play.view_shake && !play.aim_marker && play.reduce_effects &&
			play.stick_settings(0).movement_deadzone == 10 && play.stick_settings(0).aim_deadzone == 15 &&
			play.stick_settings(3).movement_deadzone == 40 && play.stick_settings(3).aim_deadzone == 45,
			message);
	}

	LevelEndInfo versus(float a, float b)
	{
		LevelEndInfo info;
		info.total_area = 200.0f;
		info.team_a_area = a;
		info.team_b_area = b;
		return info;
	}

	LevelEndInfo practice(float painted, bool team_b = false)
	{
		LevelEndInfo info;
		info.total_area = 100.0f;
		info.team_a_played = !team_b;
		info.team_b_played = team_b;
		(team_b ? info.team_b_area : info.team_a_area) = painted;
		return info;
	}

	bool same(const std::optional<float>& left, const std::optional<float>& right)
	{
		return left.has_value() == right.has_value() && (!left || *left == *right);
	}

	bool same(const StageRecord& left, const StageRecord& right)
	{
		return left.stage == right.stage && left.versus_played == right.versus_played &&
			left.team_a_wins == right.team_a_wins && left.team_b_wins == right.team_b_wins &&
			left.draws == right.draws && same(left.best_win_percentage, right.best_win_percentage) &&
			left.practice_played == right.practice_played &&
			same(left.best_coverage_percentage, right.best_coverage_percentage);
	}

	void check_display_precision()
	{
		for (int tenths = 0; tenths <= 1000; ++tenths)
		{
			const float value = static_cast<float>(tenths) / 10.0f;
			const std::wstring shown = percentage_text(value);
			std::wstring digits = shown.substr(0, shown.size() - 1);
			digits.erase(digits.find(L'.'), 1);
			require(percentage_tenths(value) == std::stoi(digits),
				"percentage_tenths disagrees with the percentage the results print");
		}
		require(percentage_text(68.46f) == L"68.4%" && percentage_tenths(68.46f) == 684,
			"Displayed percentages stopped truncating to one decimal");
	}

	void check_bookkeeping()
	{
		MatchRecords records;
		// Team A wins 60%: the first result on a stage is a new best.
		RecordOutcome first = records.add("arena", versus(120.0f, 80.0f));
		require(first.recorded && first.new_best && !first.coverage && first.value && first.best &&
			percentage_tenths(*first.best) == 600, "A first versus result did not set the stage's best");
		// 60.04% shows as 60.0%: not visibly higher, so not a new best.
		RecordOutcome invisible = records.add("arena", versus(79.9f, 120.08f));
		require(invisible.recorded && !invisible.new_best && invisible.best &&
			percentage_tenths(*invisible.best) == 600,
			"A gain hidden by the one-decimal display was called a new best");
		// Expectations come from the shown percentage, whatever float rounding
		// does to the raw share.
		const int better_tenths = percentage_tenths(versus(130.0f, 70.0f).team_a_percentage());
		require(better_tenths > 600, "The better fixture is not visibly higher");
		RecordOutcome better = records.add("arena", versus(130.0f, 70.0f));
		require(better.new_best && percentage_tenths(*better.best) == better_tenths,
			"A visibly higher winning share did not become the new best");
		RecordOutcome lower = records.add("arena", versus(70.0f, 130.0f));
		require(!lower.new_best && lower.value && percentage_tenths(*lower.value) == better_tenths &&
			percentage_tenths(*lower.best) == better_tenths, "An equal share replaced or lost the stage's best");
		RecordOutcome draw = records.add("arena", versus(100.0f, 100.0f));
		require(draw.recorded && !draw.new_best && !draw.value && draw.best &&
			percentage_tenths(*draw.best) == better_tenths, "A draw set a best or hid the existing one");
		const StageRecord* arena = records.find("arena");
		require(arena != nullptr && arena->versus_played == 5 && arena->team_a_wins == 2 &&
			arena->team_b_wins == 2 && arena->draws == 1 && arena->practice_played == 0 &&
			!arena->best_coverage_percentage, "Versus results were not tallied by side and draw");

		// Solo practice keeps a separate coverage best, on either team.
		RecordOutcome empty = records.add("arena", practice(0.0f));
		require(empty.recorded && empty.coverage && !empty.new_best && !empty.best,
			"An unpainted practice run became a best");
		RecordOutcome painted = records.add("arena", practice(37.0f, true));
		const int painted_tenths = percentage_tenths(100.0f * practice(37.0f, true).coverage_ratio());
		require(painted.coverage && painted.new_best && percentage_tenths(*painted.best) == painted_tenths,
			"Practice coverage did not set its own best");
		require(arena->practice_played == 2 && percentage_tenths(*arena->best_win_percentage) == better_tenths &&
			percentage_tenths(*arena->best_coverage_percentage) == painted_tenths,
			"Practice and versus records were not kept apart");

		// A first draw on a stage counts without inventing a best.
		RecordOutcome first_draw = records.add("other", versus(0.0f, 0.0f));
		require(first_draw.recorded && !first_draw.best && records.find("other")->draws == 1,
			"A first-match draw set a best");
		require(!records.add("", versus(1.0f, 0.0f)).recorded && records.stages().size() == 2,
			"A result without a stage was recorded");
	}

	void check_results_line()
	{
		MenuRecordText text{ L"NEW BEST", L"STAGE BEST", L"BEST WIN", L"BEST COVER", L"PLAYED" };
		require(ResultsMenuInitial::record_text(RecordOutcome(), text).empty(),
			"An unrecorded result showed a record line");
		RecordOutcome outcome;
		outcome.recorded = true;
		outcome.new_best = true;
		outcome.value = outcome.best = 68.46f;
		require(ResultsMenuInitial::record_text(outcome, text) == L"NEW BEST 68.4%",
			"A new best did not read as one, at the results' precision");
		outcome.new_best = false;
		outcome.value = 51.0f;
		require(ResultsMenuInitial::record_text(outcome, text) == L"STAGE BEST 68.4%",
			"A result under the best did not show the stage's best");
		outcome.value.reset();
		outcome.best.reset();
		require(ResultsMenuInitial::record_text(outcome, text).empty(),
			"A first-match draw showed an empty record line");
	}

	void check_versions(const IsolatedSaveDirectory& directory)
	{
		// Version 0: the flat file every existing install has.
		directory.write("{" + flat_preferences + "}");
		Save legacy;
		legacy.load_save_file();
		require_flat_preferences(legacy, "An unversioned save lost a preference");
		require(legacy.records().stages().empty(), "An unversioned save gained records");
		require(legacy.save_to_file(), "Could not rewrite an unversioned save");
		const std::string rewritten = directory.contents();
		require(rewritten.rfind("{\"save_version\":" + std::to_string(Save::version) + ",", 0) == 0 &&
			rewritten.find("\"aim_sensitivity\":4") != std::string::npos &&
			rewritten.find("\"records\":[]") != std::string::npos,
			"Saving did not write the version key, the flat preferences and the records section");
		Save reread;
		reread.load_save_file();
		require_flat_preferences(reread, "A rewritten current-version save lost a preference");

		// A newer layout still loads what this build knows; unknown parts go.
		directory.write("{\"save_version\":7," + flat_preferences +
			",\"records\":[{\"stage\":\"arena\",\"versus_played\":3}],\"future\":{\"x\":1}}");
		Save newer;
		newer.load_save_file();
		require_flat_preferences(newer, "A newer save lost a preference this build knows");
		require(newer.records().find("arena") != nullptr && newer.records().find("arena")->versus_played == 3,
			"A newer save lost records this build knows");
		require(newer.save_to_file() && directory.contents().find("future") == std::string::npos &&
			directory.contents().rfind("{\"save_version\":" + std::to_string(Save::version) + ",", 0) == 0,
			"Saving over a newer layout did not write this build's version");

		for (const char* version : { "\"one\"", "null", "-1", "1.5" })
		{
			directory.write(std::string("{\"save_version\":") + version + "," + flat_preferences + "}");
			Save odd;
			odd.load_save_file();
			require_flat_preferences(odd, "A malformed version key reset a preference");
		}
	}

	void check_section_isolation(const IsolatedSaveDirectory& directory)
	{
		const std::string valid_records =
			"\"records\":[{\"stage\":\"arena\",\"versus_played\":4,\"team_a_wins\":3,"
			"\"team_b_wins\":1,\"draws\":0,\"best_win_percentage\":71.25,"
			"\"practice_played\":2,\"best_coverage_percentage\":12.5}]";
		// A broken records section leaves every preference.
		for (const char* broken : { "{}", "true", "null", "\"records\"", "7" })
		{
			directory.write("{\"save_version\":1," + flat_preferences + ",\"records\":" + broken + "}");
			Save save;
			save.load_save_file();
			require_flat_preferences(save, "A corrupt records section reset a preference");
			require(save.records().stages().empty(), "A corrupt records section produced records");
		}
		// Broken preferences leave the records.
		directory.write("{\"save_version\":1,\"resolution\":true,\"fullscreen\":3,"
			"\"team_palette\":[],\"aim_sensitivity\":\"x\",\"pad_sticks\":7,\"view_shake\":null," +
			valid_records + "}");
		Save broken_preferences;
		broken_preferences.load_save_file();
		const StageRecord* arena = broken_preferences.records().find("arena");
		require(arena != nullptr && arena->versus_played == 4 && arena->team_a_wins == 3 &&
			arena->team_b_wins == 1 && arena->best_win_percentage == 71.25f &&
			arena->practice_played == 2 && arena->best_coverage_percentage == 12.5f,
			"Corrupt preferences reset the records section");
		require(broken_preferences.resolution() == SaveData{}.resolution &&
			broken_preferences.saved_play_settings().aim_sensitivity == PlaySettings{}.aim_sensitivity,
			"Corrupt preferences did not use their defaults");

		// Malformed entries are skipped one by one; values are bounded.
		directory.write("{\"save_version\":1," + flat_preferences + ",\"records\":["
			"null,7,{\"versus_played\":3},{\"stage\":\"\"},{\"stage\":5},"
			"{\"stage\":\"kept\",\"versus_played\":-4,\"team_a_wins\":1.5,\"draws\":\"2\","
			"\"best_win_percentage\":150,\"best_coverage_percentage\":-3,\"practice_played\":6},"
			"{\"stage\":\"kept\",\"versus_played\":99},"
			"{\"stage\":\"partial\",\"best_win_percentage\":true,\"team_b_wins\":2}]}");
		Save bounded;
		bounded.load_save_file();
		require_flat_preferences(bounded, "Malformed record entries reset a preference");
		require(bounded.records().stages().size() == 2, "Malformed or duplicate record entries were kept");
		const StageRecord* kept = bounded.records().find("kept");
		require(kept != nullptr && kept->versus_played == 0 && kept->team_a_wins == 0 &&
			kept->draws == 0 && kept->best_win_percentage == 100.0f &&
			kept->best_coverage_percentage == 0.0f && kept->practice_played == 6,
			"Record fields were not recovered and bounded one by one");
		const StageRecord* partial = bounded.records().find("partial");
		require(partial != nullptr && !partial->best_win_percentage && partial->team_b_wins == 2,
			"A wrong-typed best discarded the rest of its record");
	}

	void check_round_trip(const IsolatedSaveDirectory& directory)
	{
		// A stage the manifest no longer lists keeps its record through a write.
		directory.write("{" + flat_preferences + ",\"records\":[{\"stage\":\"retired\","
			"\"versus_played\":9,\"team_a_wins\":5,\"team_b_wins\":3,\"draws\":1,"
			"\"best_win_percentage\":88.5,\"practice_played\":0}]}");
		Save save;
		save.load_save_file();
		const RecordOutcome won = save.record_result_and_save("movement_course", versus(150.0f, 50.0f));
		require(won.recorded && won.new_best, "Recording a result did not report a new best");
		save.record_result_and_save("movement_course", versus(100.0f, 100.0f));
		save.record_result_and_save("close_quarters", practice(42.42f));
		require(directory.contents().find("\"close_quarters\"") != std::string::npos,
			"Recording a result did not write the save");

		Save restored;
		restored.load_save_file();
		require_flat_preferences(restored, "Recording results changed a preference");
		require(restored.records().stages().size() == save.records().stages().size(),
			"Records did not survive reloading");
		for (const StageRecord& record : save.records().stages())
		{
			const StageRecord* reloaded = restored.records().find(record.stage);
			require(reloaded != nullptr && same(record, *reloaded),
				"A stage record did not survive reloading exactly");
		}
		const StageRecord* retired = restored.records().find("retired");
		require(retired != nullptr && retired->versus_played == 9 && retired->best_win_percentage == 88.5f,
			"A record for a stage outside the manifest was dropped");
		const StageRecord* course = restored.records().find("movement_course");
		require(course != nullptr && course->versus_played == 2 && course->draws == 1 &&
			course->team_a_wins == 1 && percentage_tenths(*course->best_win_percentage) == 750,
			"Recorded versus results did not reload");
		const StageRecord* quarters = restored.records().find("close_quarters");
		require(quarters != nullptr && quarters->practice_played == 1 &&
			percentage_tenths(*quarters->best_coverage_percentage) == 424,
			"Recorded practice results did not reload");
	}

	std::string read_bytes(const std::filesystem::path& path)
	{
		std::ifstream stream(path, std::ios::binary);
		require(stream.good(), "Could not read a save side copy");
		return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
	}

	// What `body` writes to stderr (the C stream), with \r\n read back as \n.
	template <typename Body>
	std::string captured_stderr(const IsolatedSaveDirectory& directory, Body body)
	{
		const std::filesystem::path path = directory.save_path().parent_path() / L"stderr.txt";
		std::fflush(stderr);
		const int saved = _dup(_fileno(stderr));
		require(saved != -1, "Could not duplicate stderr");
		FILE* file = nullptr;
		if (_wfopen_s(&file, path.c_str(), L"wb") != 0 || file == nullptr)
		{
			_close(saved);
			throw std::runtime_error("Could not open the stderr capture file");
		}
		_dup2(_fileno(file), _fileno(stderr));
		body();
		std::fflush(stderr);
		_dup2(saved, _fileno(stderr));
		_close(saved);
		std::fclose(file);
		std::string text = read_bytes(path);
		std::error_code ignored;
		std::filesystem::remove(path, ignored);
		std::erase(text, '\r');
		return text;
	}

	// A save that fails as a whole keeps its defaults, as before, but is first
	// copied aside under a name never reused, so the next write cannot lose it.
	void check_unreadable_save(const IsolatedSaveDirectory& directory)
	{
		const std::string corrupt[] = {
			"{\"save_version\":5," + flat_preferences.substr(0, flat_preferences.size() / 2),
			"not a save {\"save_version\":5}" };
		std::unique_ptr<Save> last;
		for (int copy = 1; copy <= 2; ++copy)
		{
			const std::string& bytes = corrupt[copy - 1];
			directory.write(bytes);
			auto save = std::make_unique<Save>();
			const std::string errors = captured_stderr(directory, [&] { save->load_save_file(); });
			require(save->resolution() == SaveData{}.resolution &&
				save->saved_play_settings().aim_sensitivity == PlaySettings{}.aim_sensitivity &&
				save->records().stages().empty(), "An unreadable save did not leave the defaults");
			require(std::count(errors.begin(), errors.end(), '\n') == 1 &&
				errors.find("save_data.json") != std::string::npos &&
				errors.find("save_data.unreadable-" + std::to_string(copy) + ".json") != std::string::npos,
				"An unreadable save did not print one stderr line naming both paths");
			require(std::filesystem::exists(directory.side_copy(copy)) && read_bytes(directory.side_copy(copy)) == bytes,
				"An unreadable save was not copied aside byte for byte");
			require(directory.contents() == bytes, "Loading an unreadable save changed it");
			// The earlier copy survives the second corrupt load untouched.
			require(read_bytes(directory.side_copy(1)) == corrupt[0], "A later corrupt load overwrote an earlier copy");
			require(!std::filesystem::exists(directory.side_copy(copy + 1)), "An unreadable save was copied twice");
			last = std::move(save);
		}
		require(last->save_to_file(), "A save after an unreadable load did not write");
		Save restored;
		const std::string errors = captured_stderr(directory, [&] { restored.load_save_file(); });
		require(errors.empty() && directory.contents().find("\"save_version\":5") != std::string::npos,
			"A save after an unreadable load did not write the real path");
		require(read_bytes(directory.side_copy(1)) == corrupt[0] && read_bytes(directory.side_copy(2)) == corrupt[1] &&
			!std::filesystem::exists(directory.side_copy(3)), "Writing the save disturbed its side copies");
	}
}

int main()
{
	return acceptance::run_probe("match records", [&]
	{
		const IsolatedSaveDirectory directory;
		check_display_precision();
		check_bookkeeping();
		check_results_line();
		check_versions(directory);
		check_section_isolation(directory);
		check_round_trip(directory);
		check_unreadable_save(directory);
		std::cout << "PASS match records: display-precision bests, side/draw/practice tallies, "
			"results line, version 0/1/newer saves, per-section and per-entry recovery, "
			"bounded values, retired-stage retention, exact round trip and unreadable saves kept aside\n";
		return 0;
	});
}
