#ifndef DEBUGTEXT_H
#define DEBUGTEXT_H

#include "Drawer.h"
#include "ResolutionManager.h"
#include "MattMath.h"
#include "Player.h"

namespace debug_text_consts
{
	const static std::string DEBUG_FONT = "courier_new_bold_16";
	constexpr float DEBUG_FONT_SCALE = 1.0f;
	constexpr float DEBUG_LINE_SPACING = 18.0f;
	const static MattMath::Vector2F DEBUG_POSITION = MattMath::Vector2F::ZERO;
	const static MattMath::Colour DEBUG_COLOR = colour_consts::LIGHT_GRAY;
	const static MattMath::Colour DEBUG_SHADOW_COLOR = colour_consts::BLACK;
	const static MattMath::Vector2F DEBUG_SHADOW_OFFSET = { 1.0f, 1.0f };
	constexpr float DEBUG_SHADOW_SCALE = 1.0f;
}

class DebugText : public Drawer
{
public:
	DebugText(DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const float* dt,
		const ResolutionManager* resolution_manager);

	void draw_debug_info(const Player* player, int num_projectiles) const;

private:
	const ResolutionManager* _resolution_manager = nullptr;
};

#endif // !DEBUGTEXT_H
