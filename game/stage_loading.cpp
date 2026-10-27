#include "game/pch.h"
#include "game/stage_loading.h"
#include "engine/core/file_path.h"
#include "game/content_validation.h"
#include "game/drawable_text.h"
#include "game/objects/level_object_builder.h"
#include "game/sound_groups.h"

#include <algorithm>
#include <cctype>
#include <cwctype>
#include <system_error>

using namespace labrador;

namespace
{
	constexpr size_t MAX_USER_STAGE_NAME = 64;

	// The registration helper's basename rule (docs/level-editor.md), so a
	// user stage can later be registered under the same name.
	bool usable_name(const std::string& name)
	{
		if (name.empty() || name.size() > MAX_USER_STAGE_NAME || name.front() == '-')
		{
			return false;
		}
		return std::all_of(name.begin(), name.end(), [](char character)
		{
			const unsigned char value = static_cast<unsigned char>(character);
			return value < 0x80 && (std::isalnum(value) || character == '_' || character == '-');
		});
	}

	bool same_name(const std::string& first, const std::string& second)
	{
		return first.size() == second.size() && std::equal(first.begin(), first.end(),
			second.begin(), [](char a, char b)
			{
				return std::tolower(static_cast<unsigned char>(a)) ==
					std::tolower(static_cast<unsigned char>(b));
			});
	}

	bool json_extension(const std::filesystem::path& path)
	{
		std::wstring extension = path.extension().wstring();
		std::transform(extension.begin(), extension.end(), extension.begin(),
			[](wchar_t character) { return static_cast<wchar_t>(std::towlower(character)); });
		return extension == L".json";
	}

	// A refusal is one stderr line, whatever the diagnostic it carries.
	std::string one_line(std::string text)
	{
		std::replace_if(text.begin(), text.end(),
			[](char character) { return character == '\n' || character == '\r'; }, ' ');
		return text;
	}

	std::string refusal(const std::string& file, const std::string& reason)
	{
		return one_line("user stage refused: " + file + ": " + reason);
	}

	void check_user_stage(const UserStageChecks& checks, const LevelObjectBuilder& builder,
		const Registry<LevelLoadedInfo>& level_infos, const StageList& stages,
		const StageDefinition& candidate)
	{
		validate_stage_objects(*checks.tuning, builder, *checks.render, *checks.audio,
			*candidate.info);
		validate_stage_text(*checks.tuning, *checks.presentation, checks.render,
			*candidate.info, candidate.stage);
		sound_groups::check_candidate(*checks.tuning, *checks.presentation, level_infos,
			stages, *candidate.info, candidate.stage);
		// The arena counter must still fit with one more stage; a user stage
		// is never a Random candidate, so the eligible count is unchanged.
		checks.presentation->validate_stage_counts(*checks.render, stages.size() + 1,
			random_stage_indices(stages).size());
	}
}

StageDefinition read_stage_definition(const std::string& path, const std::string& name)
{
	auto info = std::make_unique<LevelLoadedInfo>(path.c_str());

	// Validate the definition before exposing it in stage selection.
	info->validate();

	// Validate authored display text now; the menu converts it again for display.
	drawable_text::widen_drawable(info->display_name(), "the display_name in " + path);

	Stage stage{ name, info->display_name(), info->icon_frame(), info->include_in_random() };
	return StageDefinition{ std::move(stage), std::move(info) };
}

UserStageScan load_user_stages(const std::filesystem::path& directory,
	const UserStageChecks& checks, Registry<LevelLoadedInfo>& level_infos, StageList& stages)
{
	UserStageScan scan;
	std::error_code error;
	if (!std::filesystem::exists(directory, error))
	{
		// No folder is the usual case: nobody has saved a stage there yet.
		return scan;
	}
	if (!std::filesystem::is_directory(directory, error))
	{
		scan.refused.push_back(refusal(path_to_utf8(directory), "is not a folder"));
		return scan;
	}

	std::vector<std::filesystem::path> files;
	std::filesystem::directory_iterator entries(directory, error);
	for (; !error && entries != std::filesystem::directory_iterator(); entries.increment(error))
	{
		const std::filesystem::directory_entry& entry = *entries;
		std::error_code kind_error;
		if (json_extension(entry.path()) && entry.is_regular_file(kind_error))
		{
			files.push_back(entry.path());
		}
	}
	if (error)
	{
		scan.refused.push_back(refusal(path_to_utf8(directory),
			"cannot list the folder: " + error.message()));
		return scan;
	}
	// File-name order, so the Stage Select order does not depend on the
	// file system's.
	std::sort(files.begin(), files.end(),
		[](const std::filesystem::path& first, const std::filesystem::path& second)
		{
			return first.filename().native() < second.filename().native();
		});

	const LevelObjectBuilder builder(checks.render, checks.audio, checks.play_settings,
		&checks.tuning->paint());
	for (const std::filesystem::path& file : files)
	{
		std::string label;
		try
		{
			label = path_to_utf8(file.filename());
			const std::string name = path_to_utf8(file.stem());
			if (!usable_name(name))
			{
				scan.refused.push_back(refusal(label, "the file name must be 1 to " +
					std::to_string(MAX_USER_STAGE_NAME) +
					" ASCII letters, digits, '_' or '-', not starting with '-'"));
				continue;
			}
			const auto taken = std::find_if(stages.begin(), stages.end(),
				[&name](const Stage& stage) { return same_name(stage.name, name); });
			if (taken != stages.end())
			{
				scan.refused.push_back(refusal(label, std::string("the name '") + name +
					"' is already " + (taken->user ? "another user stage's" : "a shipped stage's") +
					" ('" + taken->name + "'); rename the file"));
				continue;
			}

			StageDefinition candidate = read_stage_definition(path_to_utf8(file), name);
			candidate.stage.include_in_random = false;
			candidate.stage.user = true;
			check_user_stage(checks, builder, level_infos, stages, candidate);

			level_infos.add(name, std::move(candidate.info));
			stages.push_back(std::move(candidate.stage));
			scan.accepted.push_back(name);
		}
		catch (const std::exception& failure)
		{
			scan.refused.push_back(refusal(label.empty() ? "a file" : label, failure.what()));
		}
		catch (...)
		{
			scan.refused.push_back(refusal(label.empty() ? "a file" : label,
				"an unknown error while loading it"));
		}
	}
	return scan;
}
