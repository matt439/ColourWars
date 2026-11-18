#pragma once

#include "engine/render/renderer.h"
#include "game/debug/commands/debug_command.h"

class DebugRegistry;

// One material for the whole opt-in launch. Commands change its values rather
// than its selection, so they also work after a screenshot's unpresented frame.
class DebugShaderPlayground
{
public:
	static bool requested();
	void initialize(labrador::Renderer& renderer);
	void register_command(DebugRegistry& registry);

private:
	labrador::Renderer* renderer_ = nullptr;
	labrador::MaterialHandle material_;
	std::string look_ = "off";
	float strength_ = 0.0f;
	int detail_ = 6;

	DebugStatus command(const DebugArgs& args);
	std::string status() const;
};
