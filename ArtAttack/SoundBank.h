#ifndef SOUNDBANK_H
#define SOUNDBANK_H

#include <Audio.h>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <rapidjson/document.h>
#include "MattMath.h"


class SoundBank
{
public:
	SoundBank() = default;
	SoundBank(std::unique_ptr<DirectX::WaveBank> wave_bank);

	void load_from_json(const char* json_path);

	void play_wave(const std::string& wave_name, float volume = 1.0f,
		float pitch = 0.0f, float pan = 0.0f);

	void play_effect(const std::string& effect_name, bool loop = false, float volume = 1.0f,
		float pitch = 0.0f, float pan = 0.0f);
	void stop_effect(const std::string& effect_name, bool immediate = false);
	void pause_effect(const std::string& effect_name);
	void resume_effect(const std::string& effect_name);
	void set_effect_volume(const std::string& effect_name, float volume);
	void set_effect_pitch(const std::string& effect_name, float pitch);
	void set_effect_pan(const std::string& effect_name, float pan);
	DirectX::SoundState get_effect_state(const std::string& effect_name);
	bool is_effect_looping(const std::string& effect_name);

	DirectX::SoundEffectInstance* get_sound_effect_instance(const std::string& instance_name);
	DirectX::SoundEffectInstance* get_sei(const std::string& instance_name);

	void reset_all_instances();
private:
	std::unique_ptr<DirectX::WaveBank> _wave_bank = nullptr;
	std::vector<std::string> _wave_names;
	std::map<std::string, std::unique_ptr<DirectX::SoundEffectInstance>> _sound_effect_instances;

	std::vector<std::string>
		decode_wave_names_json(const rapidjson::Value& json);
	std::map<std::string, std::unique_ptr<DirectX::SoundEffectInstance>>
		decode_sound_effect_instances_json(const rapidjson::Value& json,
			bool create_effect_instance_for_each_wave);

	void clamp_levels(float& volume, float& pitch, float& pan);
};

#endif // !SOUNDBANK_H
