#ifndef WEAPONROLLER_H
#define WEAPONROLLER_H

#include "Weapon.h"

class WeaponRoller final : public Weapon
{
public:
	WeaponRoller(player_team team,
		int player_num,
		const MattMath::Colour& team_colour,
		wep_type type,
		const MattMath::Vector2F& player_center,
		RenderResources* render_resources,
		const AudioResources* audio_resources,
		const float* dt,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

private:
	void update_movement_and_rotation(PlayerInputData input,
		const MattMath::Vector2F& player_center,
		const MattMath::Vector2F& player_velocity,
		bool player_facing_right) override;

	// The roller's draw was a verbatim copy of Weapon::draw apart from this one
	// colour choice, so it is expressed as a hook instead of a duplicate.
	MattMath::Colour get_draw_colour() const override;
};

#endif // !WEAPONROLLER_H
