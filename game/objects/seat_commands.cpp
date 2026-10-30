#include "game/pch.h"
#include "game/objects/seat_commands.h"
#include "game/objects/level.h"
#include "game/objects/player_camera.h"
#include "game/input_slots.h"
#include "engine/input/mouse.h"
#include "engine/render/viewport_manager.h"

using namespace mattmath;

SeatCommandBuilder::SeatCommandBuilder(const labrador::Mouse* mouse,
	const labrador::ViewportManager* viewports) :
	mouse_(mouse), viewports_(viewports)
{
}

SeatCommands SeatCommandBuilder::build(const Level& level, const std::vector<SlotInput>& slots) const
{
	SeatCommands result;
	result.seats.resize(static_cast<size_t>(level.player_count()));
	for (int participant = 0; participant < level.player_count(); ++participant)
	{
		const Player& player = level.player(participant);
		const int slot = player.player_num();
		if (slot < 0 || static_cast<size_t>(slot) >= slots.size()) { continue; }

		const SlotInput& input = slots[static_cast<size_t>(slot)];
		PlayerInputData& seat = result.seats[static_cast<size_t>(participant)];
		seat = static_cast<const PlayerInputData&>(input);
		// Anyone in the match, by the device slot their input arrives on.
		result.skip_intro = result.skip_intro || input.skip_intro;
		if (input_slots::is_keyboard_and_mouse(slot))
		{
			this->resolve_mouse_aim(player, level.view_for_participant(participant), seat);
		}
	}
	return result;
}

void SeatCommandBuilder::resolve_mouse_aim(const Player& player, int view,
	PlayerInputData& input) const
{
	if (this->mouse_ == nullptr || !this->mouse_->focused())
	{
		// Without a valid cursor, leave aim unrequested rather than reuse a stale direction.
		return;
	}

	const labrador::Viewport viewport = this->viewports_->player_viewport(view);
	const Vector2F cursor(static_cast<float>(this->mouse_->state().position.x),
		static_cast<float>(this->mouse_->state().position.y));
	const Vector2F aim = PlayerCamera::mouse_direction(
		player.center(), cursor, viewport, player.camera());

	// A cursor exactly on the player requests no direction; do not normalize zero.
	if (aim == Vector2F::ZERO)
	{
		return;
	}

	input.shoot_direction_requested = true;
	input.shoot_direction = aim;
	input.shoot_angle = input.shoot_direction.angle();
}
