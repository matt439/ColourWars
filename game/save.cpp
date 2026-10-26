#include "game/pch.h"
#include "game/save.h"
#include "engine/assets/json.h"
#include "engine/core/file_path.h"
#include <cstdio>
#include <filesystem>
#include <system_error>
#include <array>
#include <cmath>
// The engine reads JSON; the game uses RapidJSON to write player settings.
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"

using namespace rapidjson;
using namespace directory_consts;
using namespace mattmath;
using namespace labrador;

namespace
{
    ScreenResolution convert_ivec_to_resolution(const Vector2I& vec)
    {
        if (vec.x == 1280 && vec.y == 720)
        {
            return ScreenResolution::s_1280_720;
        }
        else if (vec.x == 1920 && vec.y == 1080)
        {
            return ScreenResolution::s_1920_1080;
        }
        else if (vec.x == 2560 && vec.y == 1440)
        {
            return ScreenResolution::s_2560_1440;
        }
        else if (vec.x == 3840 && vec.y == 2160)
        {
            return ScreenResolution::s_3840_2160;
        }
        else
        {
            return SaveData{}.resolution;
        }
    }

    Vector2I convert_resolution_to_ivec(ScreenResolution resolution)
    {
        switch (resolution)
        {
        case ScreenResolution::s_1280_720:
            return {1280, 720};
        case ScreenResolution::s_1920_1080:
            return {1920, 1080};
        case ScreenResolution::s_2560_1440:
            return {2560, 1440};
        case ScreenResolution::s_3840_2160:
            return {3840, 2160};
        default:
            return {-1, -1};
        }
    }

    // Resolution, window mode and team palette: flat root keys since version 0.
    class DisplaySection final : public SaveSection
    {
    public:
        const char* key() const override { return "display"; }

        void reset(SaveData& data) const override
        {
            const SaveData defaults;
            data.resolution = defaults.resolution;
            data.fullscreen = defaults.fullscreen;
            data.team_palette = defaults.team_palette;
        }

        void read(const JsonValue& root, SaveData& data) const override
        {
            // Recover invalid or missing settings independently so one bad
            // field does not reset the other preferences.
            try
            {
                const JsonValue resolution = root.object("resolution");
                data.resolution = convert_ivec_to_resolution(
                    Vector2I(resolution.integer("x"), resolution.integer("y")));
            }
            catch (const std::runtime_error&)
            {
            }

            try
            {
                data.fullscreen = root.boolean("fullscreen");
            }
            catch (const std::runtime_error&)
            {
            }

            try
            {
                const std::string palette = root.string("team_palette");
                for (int value = static_cast<int>(TeamColourPair::random);
                    value < static_cast<int>(TeamColourPair::count); ++value)
                {
                    const TeamColourPair pair = static_cast<TeamColourPair>(value);
                    if (palette == TeamColourTools::palette_key(pair))
                    {
                        data.team_palette = pair;
                        break;
                    }
                }
            }
            catch (const std::runtime_error&)
            {
            }
        }

        void write(SaveWriter& writer, const SaveData& data) const override
        {
            const Vector2I resolution = convert_resolution_to_ivec(data.resolution);
            writer.key("resolution");
            writer.begin_object();
            writer.key("x");
            writer.integer(resolution.x);
            writer.key("y");
            writer.integer(resolution.y);
            writer.end_object();
            writer.key("fullscreen");
            writer.boolean(data.fullscreen);
            writer.key("team_palette");
            writer.string(TeamColourTools::palette_key(data.team_palette));
        }
    };

    // Aim, volumes, per-pad sticks and comfort switches: flat root keys too.
    class PlaySection final : public SaveSection
    {
    public:
        const char* key() const override { return "play"; }

        void reset(SaveData& data) const override
        {
            data.play = PlaySettings();
        }

        void read(const JsonValue& root, SaveData& data) const override
        {
            const auto read_preference = [&root](const char* key, int minimum,
                int maximum, int& destination)
            {
                try
                {
                    const int value = root.integer(key);
                    if (value >= minimum && value <= maximum)
                    {
                        destination = value;
                    }
                }
                catch (const std::runtime_error&)
                {
                }
            };
            read_preference("aim_sensitivity", 1, 5, data.play.aim_sensitivity);
            read_preference("music_volume", 0, 100, data.play.music_volume);
            read_preference("sfx_volume", 0, 100, data.play.sfx_volume);

            // Each saved profile belongs to its XInput slot. Missing legacy profiles
            // inherit the authored movement and the existing shared aim preference.
            // Recover each field independently, including malformed array entries.
            try
            {
                const JsonValue profiles = root.array("pad_sticks");
                const size_t count = std::min(profiles.size(), data.play.sticks.size());
                for (size_t slot = 0; slot < count; ++slot)
                {
                    const JsonValue profile = profiles.at(slot);
                    const auto read_deadzone = [&profile](const char* key, int& destination)
                    {
                        try
                        {
                            const int value = profile.integer(key);
                            if (value >= -1 && value <= 90) { destination = value; }
                        }
                        catch (const std::runtime_error&)
                        {
                        }
                    };
                    read_deadzone("movement_deadzone", data.play.sticks[slot].movement_deadzone);
                    read_deadzone("aim_deadzone", data.play.sticks[slot].aim_deadzone);
                }
            }
            catch (const std::runtime_error&)
            {
            }

            // Physical-slot association matches pad_sticks. Each missing or
            // malformed entry independently retains its default On value.
            try
            {
                const JsonValue slots = root.array("pad_rumble");
                const size_t count = std::min(slots.size(), data.play.pad_rumble.size());
                for (size_t slot = 0; slot < count; ++slot)
                {
                    try { data.play.pad_rumble[slot] = slots.at(slot).boolean("enabled"); }
                    catch (const std::runtime_error&) {}
                }
            }
            catch (const std::runtime_error&) {}

            const auto read_switch = [&root](const char* key, bool& destination)
            {
                try
                {
                    destination = root.boolean(key);
                }
                catch (const std::runtime_error&)
                {
                }
            };
            read_switch("view_shake", data.play.view_shake);
            read_switch("aim_marker", data.play.aim_marker);
            read_switch("reduce_effects", data.play.reduce_effects);
        }

        void write(SaveWriter& writer, const SaveData& data) const override
        {
            writer.key("aim_sensitivity");
            writer.integer(data.play.aim_sensitivity);
            writer.key("pad_sticks");
            writer.begin_array();
            for (const StickSettings& settings : data.play.sticks)
            {
                writer.begin_object();
                writer.key("movement_deadzone");
                writer.integer(settings.movement_deadzone);
                writer.key("aim_deadzone");
                writer.integer(settings.aim_deadzone);
                writer.end_object();
            }
            writer.end_array();
            writer.key("pad_rumble");
            writer.begin_array();
            for (const bool enabled : data.play.pad_rumble)
            {
                writer.begin_object();
                writer.key("enabled");
                writer.boolean(enabled);
                writer.end_object();
            }
            writer.end_array();
            writer.key("music_volume");
            writer.integer(data.play.music_volume);
            writer.key("sfx_volume");
            writer.integer(data.play.sfx_volume);
            writer.key("view_shake");
            writer.boolean(data.play.view_shake);
            writer.key("aim_marker");
            writer.boolean(data.play.aim_marker);
            writer.key("reduce_effects");
            writer.boolean(data.play.reduce_effects);
        }
    };

    // Every section, in the order the file is written. Sections hold no state,
    // so one shared list serves every Save and Save stays copyable.
    const std::array<const SaveSection*, 5>& save_sections()
    {
        static const DisplaySection display;
        static const PlaySection play;
        static const MatchRecordsSection records;
        static const LastMatchSetupSection last_match;
        static const InputBindingsSection input_bindings;
        static const std::array<const SaveSection*, 5> sections{
            &display, &play, &records, &last_match, &input_bindings };
        return sections;
    }

    class FileSaveWriter final : public SaveWriter
    {
    public:
        explicit FileSaveWriter(Writer<FileWriteStream>& writer) : writer_(writer) {}
        void key(const char* name) override { this->writer_.Key(name); }
        void integer(int value) override { this->writer_.Int(value); }
        void number(float value) override
        {
            // JSON has no NaN or infinity; a section never writes one, and a
            // zero keeps the document parseable if one ever does.
            this->writer_.Double(std::isfinite(value) ? static_cast<double>(value) : 0.0);
        }
        void boolean(bool value) override { this->writer_.Bool(value); }
        void string(const std::string& value) override
        {
            this->writer_.String(value.c_str(), static_cast<SizeType>(value.size()));
        }
        void begin_object() override { this->writer_.StartObject(); }
        void end_object() override { this->writer_.EndObject(); }
        void begin_array() override { this->writer_.StartArray(); }
        void end_array() override { this->writer_.EndArray(); }
    private:
        Writer<FileWriteStream>& writer_;
    };
}

SaveData Save::load_from_json(const char* json_path) const
{
    const JsonDocument document = read_json_file(json_path);
    const JsonValue root = document.root();

    // A file this build cannot have written still loads what it knows. The
    // next save writes this version and drops the newer sections.
    try
    {
        const int saved = root.has("save_version") ? root.integer("save_version") : 0;
        if (saved > Save::version)
        {
            fprintf(stderr, "save: %s is save_version %d; this build writes %d and "
                "will drop what it does not know on the next save.\n",
                json_path, saved, Save::version);
        }
    }
    catch (const std::runtime_error&)
    {
    }

    auto data = SaveData();
    // One malformed section resets only itself.
    for (const SaveSection* section : save_sections())
    {
        try
        {
            section->read(root, data);
        }
        catch (const std::runtime_error&)
        {
            section->reset(data);
        }
    }
    return data;
}
bool Save::file_exists(const std::string& path)
{
    FILE* fp = nullptr;
    if (_wfopen_s(&fp, path_from_utf8(path).c_str(), L"rb") != 0 || fp == nullptr)
    {
		return false;
	}
	fclose(fp);
	return true;
}
void Save::load_save_file()
{
    const std::string path = save_file_path();

    if (!file_exists(path))
    {
        // If the per-user save cannot be opened, try the executable-relative legacy
        // save before writing defaults. The legacy file is left intact.
        const std::string legacy = legacy_save_file_path();
        if (file_exists(legacy))
        {
            try
            {
                this->save_data_ = this->load_from_json(legacy.c_str());
            }
            catch (const std::exception&)
            {
                this->save_data_ = SaveData();
            }
        }

        // There is no settings screen yet to report a failed first save.
        if (!this->save_to_file())
        {
            const std::wstring message = L"Could not write " +
                path_from_utf8(path).wstring() +
                L".\nYour settings will not be remembered.";
            MessageBoxW(nullptr, message.c_str(), L"ColourWars - save failure",
                MB_OK | MB_ICONWARNING);
        }
        return;
    }

    try
    {
        this->save_data_ = this->load_from_json(path.c_str());
    }
    catch (const std::exception&)
    {
        // Unreadable or corrupt saves use defaults; a later settings change retries saving.
        this->save_data_ = SaveData();
    }
}

bool Save::write_save_file(const SaveData& data) const
{
    const std::string path = save_file_path();

    // Create the per-user directory on first save.
    const std::filesystem::path save_path = path_from_utf8(path);
    if (save_path.has_parent_path())
    {
        std::error_code ec;
        std::filesystem::create_directories(save_path.parent_path(), ec);
    }

    // Write a sibling temporary file before replacing the save, so a failed
    // write does not truncate the existing settings.
    std::filesystem::path temp_file_path = save_path;
    temp_file_path += L".tmp";

    FILE* fp = nullptr;
    if (_wfopen_s(&fp, temp_file_path.c_str(), L"wb") != 0 || fp == nullptr)
    {
        return false;
    }
    auto write_buffer = std::make_unique<char[]>(65536);
    FileWriteStream os(fp, write_buffer.get(), 65536);

    // Writer flushes when the root object ends, before fclose below.
    Writer<FileWriteStream> json(os);
    FileSaveWriter writer(json);
    json.StartObject();
    writer.key("save_version");
    writer.integer(Save::version);
    for (const SaveSection* section : save_sections())
    {
        section->write(writer, data);
    }
    const bool serialized = json.EndObject();

    // Ending the root flushes RapidJSON's buffer, but FileWriteStream ignores
    // short fwrite results. Flush stdio and inspect its sticky error before
    // always closing the file. Serialization, stream output and close must all
    // succeed before the temporary file may replace the existing save.
    const bool flushed = (fflush(fp) == 0);
    const bool stream_good = (ferror(fp) == 0);
    const bool closed = (fclose(fp) == 0);

    std::error_code rename_ec;
    if (serialized && flushed && stream_good && closed)
    {
        std::filesystem::rename(temp_file_path, save_path, rename_ec);
        if (!rename_ec)
        {
            return true;
        }
    }

    std::error_code remove_ec;
    std::filesystem::remove(temp_file_path, remove_ec);
    return false;
}

bool Save::save_to_file() const
{
    auto start = std::chrono::system_clock::now();
    auto legacy_start = std::chrono::system_clock::to_time_t(start);
    // ctime_s includes a trailing newline.
    char buffer[30] = {};
    if (ctime_s(buffer, sizeof(buffer), &legacy_start) != 0)
    {
        buffer[0] = '\0';
    }
    std::cout << buffer << "Saving...\n";
    if (write_save_file(this->save_data_))
    {
		std::cout << "Save was successful.\n";
        return true;
	}

    // The caller owns visible feedback; settings remain active in memory.
    const std::string message = "Could not write " + save_file_path() +
        ".\nYour settings will not be remembered.";
    fprintf(stderr, "save failure: %s\n", message.c_str());
    return false;
}

bool Save::set_display_settings_and_save(ScreenResolution resolution,
    bool fullscreen, TeamColourPair palette)
{
    // Validate the whole selection before changing any active preference.
    if (convert_resolution_to_ivec(resolution).x <= 0)
    {
        throw std::invalid_argument("Invalid display resolution");
    }
    TeamColourTools::palette_key(palette);
    this->save_data_.resolution = resolution;
    this->save_data_.fullscreen = fullscreen;
    this->save_data_.team_palette = palette;
    return this->save_to_file();
}

SaveData Save::save_data() const
{
	return this->save_data_;
}

ScreenResolution Save::resolution() const
{
	return this->save_data_.resolution;
}

bool Save::fullscreen() const
{
	return this->save_data_.fullscreen;
}

TeamColourPair Save::team_palette() const
{
    return this->save_data_.team_palette;
}

const PlaySettings& Save::play_settings() const
{
    return this->save_data_.play;
}

bool Save::set_play_settings_and_save(const PlaySettings& settings)
{
    if (settings.aim_sensitivity < 1 || settings.aim_sensitivity > 5 ||
        settings.music_volume < 0 || settings.music_volume > 100 ||
        settings.sfx_volume < 0 || settings.sfx_volume > 100)
    {
        throw std::invalid_argument("Play settings are outside their supported range");
    }
    PlaySettings normalized = settings;
    for (StickSettings& profile : normalized.sticks)
    {
        profile.movement_deadzone = StickSettings::normalized_deadzone(profile.movement_deadzone);
        profile.aim_deadzone = StickSettings::normalized_deadzone(profile.aim_deadzone);
    }
    this->save_data_.play = normalized;
    return this->save_to_file();
}

const InputBindings& Save::input_bindings() const
{
    return this->save_data_.input_bindings;
}

bool Save::set_input_bindings_and_save(const InputBindings& bindings)
{
    bindings.validate();
    this->save_data_.input_bindings = bindings;
    return this->save_to_file();
}

const MatchRecords& Save::records() const
{
    return this->save_data_.records;
}

RecordOutcome Save::record_result_and_save(const std::string& stage,
    const LevelEndInfo& result)
{
    const RecordOutcome outcome = this->save_data_.records.add(stage, result);
    if (outcome.recorded)
    {
        // save_to_file already reports a failure on stderr; the results
        // still show the record, which stays for the rest of this run.
        std::ignore = this->save_to_file();
    }
    return outcome;
}

const std::optional<LastMatchSetup>& Save::last_match_setup() const
{
    return this->save_data_.last_match;
}

bool Save::remember_match_setup_and_save(const LastMatchSetup& setup)
{
    setup.validate();
    SaveData candidate = this->save_data_;
    candidate.last_match = setup;
    if (!this->write_save_file(candidate))
    {
        fprintf(stderr, "save failure: Could not write %s.\n"
            "The previous remembered match setup is unchanged.\n",
            save_file_path().c_str());
        return false;
    }
    // Only this section changes. Other active preferences and record objects
    // keep their existing semantics and identity for their borrowers.
    this->save_data_.last_match.swap(candidate.last_match);
    return true;
}
