#pragma once

#include "engine/render/render_resources.h"
#include "engine/render/resolution_manager.h"
#include "engine/render/text_encoding.h"
#include "engine/math/matt_math.h"
#include "game/objects/player.h"

namespace debug_text_consts
{
	const static std::string DEBUG_FONT = "courier_new_bold_16";
	constexpr float DEBUG_FONT_SCALE = 1.0f;
	constexpr float DEBUG_LINE_SPACING = 18.0f;
	const static mattmath::Vector2F DEBUG_POSITION = mattmath::Vector2F::ZERO;
	const static mattmath::Colour DEBUG_COLOR = colour_consts::LIGHT_GRAY;
	const static mattmath::Colour DEBUG_SHADOW_COLOR = colour_consts::BLACK;
	const static mattmath::Vector2F DEBUG_SHADOW_OFFSET = { 1.0f, 1.0f };
	constexpr float DEBUG_SHADOW_SCALE = 1.0f;
}

class DebugText
{
public:
	DebugText(artattack::RenderResources* render_resources,
		const artattack::ResolutionManager* resolution_manager);

	void draw_debug_info(DirectX::SpriteBatch* sprite_batch,
		const Player* player, int num_projectiles, float dt) const;

private:
	artattack::RenderResources* render_resources() const;

	// Borrowed, not owned - see InterfaceGameplay.
	artattack::RenderResources* render_resources_ = nullptr;
	const artattack::ResolutionManager* resolution_manager_ = nullptr;

	// Resolved once, like every other font name. The debug overlay is drawn
	// per player per frame when it is on at all.
	artattack::RenderResources::FontHandle font_;
};
