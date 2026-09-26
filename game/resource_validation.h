#pragma once

#include "engine/audio/audio_resources.h"
#include "engine/render/render_resources.h"

#include <stdexcept>
#include <string>

// Resolve content references after the manifest is loaded, with the authored
// key attached to any failure. Resolution does not play audio; optional silent
// banks keep the engine's usual permissive lookup behavior.
class ResourceValidation
{
public:
	ResourceValidation(const std::string& source,
		const labrador::RenderResources& render,
		const labrador::AudioResources& audio) :
		source_(source), render_(render), audio_(audio) {}

	void font(const std::string& key, const std::string& name) const
	{
		check(key, [&] { render_.resolve_sprite_font(name); });
	}

	void frame(const std::string& key, const std::string& sheet,
		const std::string& name) const
	{
		check(key, [&] { render_.sprite_sheet(sheet)->resolve_sprite_frame(name); });
	}

	void animation(const std::string& key, const std::string& sheet,
		const std::string& name) const
	{
		check(key, [&] { render_.sprite_sheet(sheet)->resolve_animation_strip(name); });
	}

	void wave(const std::string& key, const std::string& bank,
		const std::string& name) const
	{
		check(key, [&] { audio_.sound_bank(bank)->resolve_wave(name); });
	}

	void effect(const std::string& key, const std::string& bank,
		const std::string& name) const
	{
		check(key, [&] { audio_.sound_bank(bank)->resolve_effect(name); });
	}

private:
	template<typename Resolve>
	void check(const std::string& key, Resolve resolve) const
	{
		try
		{
			resolve();
		}
		catch (const std::exception& error)
		{
			throw std::runtime_error(source_ + ": " + key + ": " + error.what());
		}
	}

	std::string source_;
	const labrador::RenderResources& render_;
	const labrador::AudioResources& audio_;
};
