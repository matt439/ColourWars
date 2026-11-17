#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/shader_playground.h"
#include "game/debug/commands/debug_registry.h"
#include "materials/colourwars_shader_playground.h"
#include <cstdlib>

namespace
{
	struct ShaderLook
	{
		const char* name;
		int mode;
		float strength;
		int detail;
		int minimum;
		int maximum;
	};

	constexpr ShaderLook looks[] = {
		{ "off", 0, 0.0f, 6, 0, 0 },
		{ "posterise", 1, 0.75f, 6, 2, 16 },
		{ "ink", 2, 0.65f, 1, 1, 4 },
		{ "halftone", 3, 0.65f, 6, 3, 24 },
		{ "crt", 4, 0.5f, 4, 4, 12 },
		{ "highcolour", 5, 1.0f, 0, 0, 0 },
		{ "dither", 6, 0.6f, 8, 2, 16 },
		{ "paint90", 7, 0.5f, 4, 4, 12 },
	};

	std::vector<std::string> look_names()
	{
		std::vector<std::string> result;
		for (const ShaderLook& look : looks) { result.emplace_back(look.name); }
		return result;
	}
}

bool DebugShaderPlayground::requested()
{
	size_t size = 0;
	if (getenv_s(&size, nullptr, 0, "COLOURWARS_SHADER_PLAYGROUND") != 0 || size != 2)
	{
		return false;
	}
	char value[2]{};
	return getenv_s(&size, value, sizeof(value), "COLOURWARS_SHADER_PLAYGROUND") == 0 &&
		value[0] == '1';
}

void DebugShaderPlayground::initialize(labrador::Renderer& renderer)
{
	if (this->renderer_ != nullptr) { return; }
	const labrador::MaterialHandle material = renderer.add_material(labrador::colourwars_shader_playground);
	renderer.set_post_material(material);
	renderer.set_post_values(material, {});
	this->material_ = material;
	this->renderer_ = &renderer;
}

void DebugShaderPlayground::register_command(DebugRegistry& registry)
{
	registry.add_command("shader", "Debug shader playground; strength 0-1, detail: colour levels, pixel radius/cell size or CRT scanline pitch",
		{ { "look", true, look_names },
			{ "strength", true, {} }, { "detail", true, {} } },
		[this](const DebugArgs& args, DebugLog&) { return this->command(args); },
		DebugMenuTraits::of("view", DebugMenuTraits::Form::pick));
}

std::string DebugShaderPlayground::status() const
{
	if (this->renderer_ == nullptr)
	{
		return "shader playground disabled; restart Debug with COLOURWARS_SHADER_PLAYGROUND=1";
	}
	return "shader " + this->look_ + " strength " + std::to_string(this->strength_) +
		(this->look_ == "highcolour" ? " RGB565" : " detail " + std::to_string(this->detail_));
}

DebugStatus DebugShaderPlayground::command(const DebugArgs& args)
{
	if (!args.has(0)) { return DebugStatus::success(this->status()); }
	if (this->renderer_ == nullptr) { return DebugStatus::failure(this->status()); }

	const std::string& look = args.text(0);
	const ShaderLook* selected = nullptr;
	for (const ShaderLook& candidate : looks)
	{
		if (look == candidate.name) { selected = &candidate; break; }
	}
	if (selected == nullptr)
	{
		return DebugStatus::failure("shader: expected " + join_words(look_names(), ", ", " or "));
	}
	if (look == "off" && args.has(1)) { return DebugStatus::failure("usage: shader off"); }
	if (look == "highcolour" && args.has(2))
	{
		return DebugStatus::failure("shader highcolour: RGB565 is fixed; usage: shader highcolour [strength]");
	}
	float strength = selected->strength;
	int detail = selected->detail;
	if (args.has(1) && (!DebugArgs::parse_float(args.text(1), strength) || strength < 0.0f || strength > 1.0f))
	{
		return DebugStatus::failure("shader: strength must be a finite number from 0 to 1");
	}
	if (args.has(2) && (!DebugArgs::parse_int(args.text(2), detail) ||
		detail < selected->minimum || detail > selected->maximum))
	{
		return DebugStatus::failure("shader " + look + ": detail must be a whole number from " +
			std::to_string(selected->minimum) + " to " + std::to_string(selected->maximum));
	}

	labrador::PostValues values;
	values.values[0] = static_cast<float>(selected->mode);
	values.values[1] = strength;
	values.values[2] = static_cast<float>(detail);
	this->renderer_->set_post_values(this->material_, values);
	this->look_ = look;
	this->strength_ = strength;
	this->detail_ = detail;
	return DebugStatus::success(this->status());
}
#endif
