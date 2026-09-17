#include "game/pch.h"
#include "game/objects/interface_gameplay.h"
#include "game/input_slots.h"

using namespace mattmath;
using namespace labrador;

namespace
{
	// A match is four minutes, and a bare second count spends the whole of it
	// reading as a distance rather than a clock: "240", "239", "238". mm:ss is
	// the only thing on the HUD that says how long is left in a unit anyone
	// thinks in.
	//
	// Wide, because the narrow draw and measure overloads convert through a
	// buffer the shared font owns, and every player's render worker is in here
	// at once. Assembled by hand rather than through a stream for the same
	// reason draw_timer avoids one: this runs per player per frame.
	//
	// std::ceil to match draw_respawn_timer, and clamped at zero because
	// Level::update runs the tick that takes the clock past zero before the
	// state change that stops drawing it, so the HUD sees a small negative once.
	std::wstring format_clock(float seconds)
	{
		const int total = std::max(0, static_cast<int>(std::ceil(seconds)));
		const int minutes = total / 60;
		const int remainder = total % 60;

		return std::to_wstring(minutes) + (remainder < 10 ? L":0" : L":") +
			std::to_wstring(remainder);
	}
}

InterfaceGameplay::InterfaceGameplay(RenderResources* render_resources,
	const HudPresentation* look) :
	render_resources_(render_resources),
	look_(look),
	sheet_(render_resources->resolve_sprite_sheet(look->sprite_sheet)),
	empty_box_frame_(render_resources->sprite_sheet(this->sheet_)->
		resolve_sprite_frame(look->empty_box_frame)),
	fill_frame_(render_resources->sprite_sheet(this->sheet_)->
		resolve_sprite_frame(look->fill_frame)),
	timer_font_(render_resources->resolve_sprite_font(look->timer_font)),
	respawn_timer_font_(
		render_resources->resolve_sprite_font(look->respawn_font)),
	player_identity_font_(render_resources->resolve_sprite_font(look->player_identity_font))
{

}

RenderResources* InterfaceGameplay::render_resources() const
{
	return this->render_resources_;
}

void InterfaceGameplay::draw_gameplay_interface(DrawList& draw_list,
	const Vector2F& resolution,
	int player_index,
	int input_slot,
	float health,
	float ammo,
	float timer,
	const Colour& team_colour,
	float respawn_timer,
	bool show_respawn_timer) const
{
	const HudPresentation& look = *this->look_;

	// The clock is a large face and wants linear; the boxes are pixel art and
	// want point. That was two Begin/End pairs with two samplers, one of them
	// a raw ID3D11SamplerState* the caller had cached at construction and held
	// across a device loss. It is two set_filter calls now, and the state that
	// produces the filtering belongs to the backend.
	draw_list.set_filter(TextureFilter::linear);
	this->draw_timer(draw_list, resolution, timer);

	const std::wstring identity = L"P" + std::to_wstring(player_index + 1) +
		L" - " + input_slots::device_name(input_slot);
	const Vector2F identity_pos(resolution.x * look.player_identity_offset.x,
		resolution.y * look.player_identity_offset.y);
	const float identity_scale = resolution.x / look.player_identity_reference_width;
	const Vector2F padding = look.player_identity_padding * identity_scale;
	const Vector2F identity_size = this->render_resources()->measure_text(
		this->player_identity_font_, identity) * identity_scale;
	this->render_resources()->sprite_sheet(this->sheet_)->draw(draw_list,
		this->fill_frame_, RectangleF(identity_pos - padding,
			identity_size + padding * 2.0f), look.player_identity_background_colour);
	draw_list.draw_text(this->player_identity_font_, identity,
		identity_pos + look.player_identity_shadow_offset * identity_scale,
		look.player_identity_shadow_colour, identity_scale,
		0.0f, Vector2F::ZERO, 0.0f);
	draw_list.draw_text(this->player_identity_font_, identity, identity_pos,
		look.player_identity_colour, identity_scale,
		0.0f, Vector2F::ZERO, 0.0f);

	draw_list.set_filter(TextureFilter::point);

	float x_pos = resolution.x - resolution.x * look.right_margin;

	Vector2F ammo_pos = Vector2F(x_pos,
		resolution.y * look.ammo_top_margin);
	this->draw_ammo(draw_list, resolution, ammo, team_colour, ammo_pos);

	Vector2F health_pos = Vector2F(x_pos,
		resolution.y * look.health_top_margin);
	this->draw_health(draw_list, resolution, health, health_pos);

	if (show_respawn_timer)
	{
		draw_list.set_filter(TextureFilter::linear);
		this->draw_respawn_timer(draw_list, resolution, respawn_timer);
		draw_list.set_filter(TextureFilter::point);
	}
}

void InterfaceGameplay::draw_ammo(DrawList& draw_list,
	const Vector2F& resolution,
	float ammo,
	const Colour& team_colour,
	const Vector2F& position) const
{
	const HudPresentation& look = *this->look_;

	//draw empty box
	Vector2F size = Vector2F(
		resolution.x * look.box_size.x,
		resolution.y * look.box_size.y);

	const RectangleF empty_box_rect = RectangleF(position, size);

	const SpriteSheet* sprite_sheet = this->render_resources()->
		sprite_sheet(this->sheet_);

	sprite_sheet->draw(
		draw_list,
		this->empty_box_frame_,
		empty_box_rect,
		Colour::white);

	//fill box
	float border = resolution.x * look.box_fill_border;
	Vector2F fill_pos = position + Vector2F(border, border);
	Vector2F fill_size = Vector2F(size.x - 2.0f * border,
		size.y - 2.0f * border);
	// The fill is a fraction of the inside of the frame, so anything outside
	// [0, 1] paints outside the box that is meant to contain it. Clamped here
	// rather than trusted from the caller: the HUD is handed a raw ammo count
	// by whichever weapon the player is holding, and one weapon over-filling
	// its magazine must not be able to draw over the rest of the screen.
	fill_size.x *= std::clamp(ammo, 0.0f, 1.0f);

	sprite_sheet->draw(
		draw_list,
		this->fill_frame_,
		RectangleF(fill_pos, fill_size),
		team_colour);
}
void InterfaceGameplay::draw_health(DrawList& draw_list,
	const Vector2F& resolution,
	float health,
	const Vector2F& position) const
{
	const HudPresentation& look = *this->look_;

	//draw empty box
	Vector2F size = Vector2F(
		resolution.x * look.box_size.x,
		resolution.y * look.box_size.y);

	const SpriteSheet* sprite_sheet = this->render_resources()->
		sprite_sheet(this->sheet_);

	sprite_sheet->draw(
		draw_list,
		this->empty_box_frame_,
		RectangleF(position, size),
		Colour::white);

	//fill box
	float border = resolution.x * look.box_fill_border;
	Vector2F fill_pos = position + Vector2F(border, border);
	Vector2F fill_size = Vector2F(size.x - 2.0f * border,
		size.y - 2.0f * border);
	// Clamped for the same reason as the ammo bar above: health is a fraction
	// of the frame, and nothing on the way here guarantees it stays in [0, 1].
	fill_size.x *= std::clamp(health, 0.0f, 1.0f);

	sprite_sheet->draw(
		draw_list,
		this->fill_frame_,
		RectangleF(fill_pos, fill_size),
		look.health_colour);
}
void InterfaceGameplay::draw_timer(DrawList& draw_list,
	const Vector2F& resolution, float timer) const
{
	const HudPresentation& look = *this->look_;

	Vector2F pos = Vector2F(
		resolution.x / 2.0f - (resolution.x * look.timer_offset.x),
		resolution.y * look.timer_offset.y);

	float scale = resolution.x / look.timer_reference_width;

	const std::wstring text = format_clock(timer);

	draw_list.draw_text(this->timer_font_, text,
		pos + (look.timer_shadow_offset * scale), look.timer_shadow_colour, scale,
		0.0f, Vector2F::ZERO, 0.0f);

	draw_list.draw_text(this->timer_font_, text, pos, look.timer_colour, scale,
		0.0f, Vector2F::ZERO, 0.0f);
}

void InterfaceGameplay::draw_respawn_timer(DrawList& draw_list,
	const Vector2F& resolution,
	float timer) const
{
	const HudPresentation& look = *this->look_;

	Vector2F pos = Vector2F(
		resolution.x / 2.0f - (resolution.x * look.respawn_offset.x),
		resolution.y * look.respawn_offset.y);

	// std::ceil, as draw_timer above does, and not truncate-then-add-one: that
	// is a ceil which is wrong at exactly the integer values, and the first
	// frame of a respawn is exactly one. Player::update flips state_ to dead in
	// its *alive* arm, while respawn_timer_ -= dt lives in the dead arm that
	// does not run until the next tick - so the HUD first reads the delay
	// undecremented, and a three second respawn opened on "4".
	const std::wstring text =
		std::to_wstring(static_cast<int>(std::ceil(timer)));

	draw_list.draw_text(this->respawn_timer_font_, text,
		pos + look.respawn_shadow_offset, look.respawn_shadow_colour,
		1.0f, 0.0f, Vector2F::ZERO, 0.0f);

	draw_list.draw_text(this->respawn_timer_font_, text, pos,
		look.respawn_colour, 1.0f, 0.0f, Vector2F::ZERO, 0.0f);
}
