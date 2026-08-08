#pragma once

#include "game/objects/weapon.h"
#include "engine/render/colour.h"

class WeaponRoller final : public Weapon
{
public:
	WeaponRoller(PlayerTeam team,
		int player_num,
		const artattack::Colour& team_colour,
		WeaponType type,
		const mattmath::Vector2F& player_center,
		artattack::RenderResources* render_resources,
		const artattack::AudioResources* audio_resources,
		const artattack::Colour& color = artattack::Colour::white,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		artattack::SpriteFlip flip = artattack::SpriteFlip::none,
		float layer_depth = 0.0f);

private:
	void update_movement_and_rotation(PlayerInputData input,
		const mattmath::Vector2F& player_center,
		const mattmath::Vector2F& player_velocity,
		bool player_facing_right) override;

	// The roller's draw was a verbatim copy of Weapon::draw apart from this one
	// colour choice, so it is expressed as a hook instead of a duplicate.
	artattack::Colour draw_colour() const override;
};
