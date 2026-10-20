// Engine-only exhibit for shared music/SFX attenuation. No game headers,
// content files, Application, window, graphics device or audio hardware.
// Link ONLY engine/audio/null/audio_device.cpp as the AudioDevice backend.
#include "engine/audio/audio_device.h"
#include "engine/audio/audio_resources.h"
#include "engine/audio/null/recording.h"
#include "engine/audio/sound_bank.h"

#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace labrador;

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    // Synthetic client policy. These are ordinary local values, not an
    // engine service or a proposed API. Every emitter must read them itself.
    struct Preferences { float music = 1.0f; float sfx = 1.0f; };

    void menu_click(const SoundBank& bank, SoundBank::WaveHandle wave,
        const Preferences& preferences)
    {
        bank.play_wave(wave, 0.6f * preferences.sfx, 0.25f, -0.5f);
    }

    void weapon_shot(const SoundBank& bank, SoundBank::WaveHandle wave,
        const Preferences& preferences)
    {
        bank.play_wave(wave, 0.9f * preferences.sfx, 0.25f, -0.5f);
    }
}

int main()
{
    try
    {
        AudioDevice device;
        const auto waves = device.open_wave_bank("", "synthetic",
            { "music", "spray", "click", "shot" });
        const auto music_voice = device.create_voice(waves, device.wave_index(waves, "music"));
        const auto spray_voice = device.create_voice(waves, device.wave_index(waves, "spray"));
        Registry<SoundBank::SoundEffect> effects("SoundEffect");
        effects.add("music_loop", std::make_unique<SoundBank::SoundEffect>(SoundBank::SoundEffect{ music_voice }));
        effects.add("spray_loop", std::make_unique<SoundBank::SoundEffect>(SoundBank::SoundEffect{ spray_voice }));
        AudioResources resources;
        resources.add_sound_bank("synthetic", std::make_unique<SoundBank>(&device, waves, std::move(effects)));
        const SoundBank& bank = *resources.sound_bank("synthetic");
        const auto music = bank.resolve_effect("music_loop"), spray = bank.resolve_effect("spray_loop");
        const auto click = bank.resolve_wave("click"), shot = bank.resolve_wave("shot");
        static_assert(std::is_same_v<decltype(bank.play_wave(shot)), void>);
        require(bank.audible(), "The null backend bank must contain real names, not be SoundBank::silent()");
        require(recorded_wave_names(device, waves) == std::vector<std::string>{ "music", "spray", "click", "shot" },
            "Synthetic wave-name fixture drifted");

        Preferences preferences;
        bank.play_effect(music, true, 0.8f * preferences.music);
        bank.play_effect(spray, true, 0.4f * preferences.sfx);
        menu_click(bank, click, preferences);
        weapon_shot(bank, shot, preferences);
        require(recorded_sounds(device).size() == 4, "Initial calls were not recorded");

        preferences.music = 0.25f;
        preferences.sfx = 0.5f;
        device.update();
        // Changing local preferences naturally makes no engine request. There
        // is no public master/bus setter to call here; that absence is also
        // checked against AudioDevice/SoundBank/AudioResources in the report.
        require(recorded_sounds(device).size() == 4, "Local preference assignment unexpectedly emitted audio commands");
        bank.set_effect_volume(music, 0.8f * preferences.music);
        bank.set_effect_volume(spray, 0.4f * preferences.sfx);
        menu_click(bank, click, preferences);
        weapon_shot(bank, shot, preferences);
        // Negative control: a raw emitter which does not repeat that policy
        // still receives ordinary per-call semantics, not logical SFX gain.
        bank.play_wave(shot, 0.9f, 0.25f, -0.5f);

        const auto& calls = recorded_sounds(device);
        require(calls.size() == 9, "The bounded per-emitter exhibit changed its call budget");
        const float expected[] = { 0.8f, 0.4f, 0.6f, 0.9f, 0.2f, 0.2f, 0.3f, 0.45f, 0.9f };
        const SoundCall kinds[] = { SoundCall::play_voice, SoundCall::play_voice,
            SoundCall::play_wave, SoundCall::play_wave, SoundCall::set_voice_volume,
            SoundCall::set_voice_volume, SoundCall::play_wave, SoundCall::play_wave, SoundCall::play_wave };
        for (size_t index = 0; index < calls.size(); ++index)
        {
            require(calls[index].call == kinds[index] && calls[index].volume == expected[index],
                "Recorded kind/volume differed from the independently supplied literal result");
            require(calls[index].bank == waves, "An audio command changed banks");
        }
        require(calls[0].voice == music_voice && calls[1].voice == spray_voice &&
            calls[4].voice == music_voice && calls[5].voice == spray_voice && calls[0].loop && calls[1].loop,
            "Persistent voice targeting or loop policy changed");
        for (const size_t index : { size_t(2), size_t(3), size_t(6), size_t(7), size_t(8) })
        {
            require(calls[index].wave == (index == 2 || index == 6 ? click.index() : shot.index()) &&
                calls[index].pitch == 0.25f && calls[index].pan == -0.5f,
                "Per-call attenuation changed wave selection, pitch or pan");
        }
        require(calls[8].volume != 0.45f && calls[3].volume == 0.9f,
            "The raw-emitter negative control or historical one-shot call was rewritten");
        std::cout << std::fixed << std::setprecision(3);
        for (size_t index = 0; index < calls.size(); ++index)
            { std::cout << index << ',' << calls[index].volume << '\n'; }
        std::cout << "PASS synthetic per-emitter gain exhibit: 9 recorded commands; no hardware, audio clock or listening claim\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
