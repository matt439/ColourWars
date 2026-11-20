#include "game/pch.h"
#include "game/objects/weapon_umbrella.h"

using namespace labrador;

WeaponUmbrella::WeaponUmbrella(const GameTuning* tuning, PlayerTeam team, int player_num, int sound_index,
    const Colour& team_colour, WeaponType type, const Vector2F& player_center,
    RenderResources* render_resources, const AudioResources* audio_resources, const PlaySettings* preferences) :
    RelativeVelocityWeapon(tuning, team, player_num, sound_index, team_colour, type, player_center,
        render_resources, audio_resources, preferences)
{
    const auto& canopy = weapon_tuning_->canopy;
    open_sound_ = resolve_wave(canopy.open_sound.name);
    block_sound_ = resolve_wave(canopy.block_sound.name);
    break_sound_ = resolve_wave(canopy.break_sound.name);
    reset_canopy();
    load_canopy_art();
}

void WeaponUmbrella::reset_canopy()
{
    state_ = { false, weapon_tuning_->canopy.hits, 0.0f };
    prepared_ = false;
}

void WeaponUmbrella::advance_canopy(bool held, float dt)
{
    if (!(dt > 0.0f)) { return; }
    if (state_.recovery_remaining > 0.0f)
    {
        state_.recovery_remaining = std::max(0.0f, state_.recovery_remaining - dt);
        if (state_.recovery_remaining == 0.0f) { state_.hits_left = weapon_tuning_->canopy.hits; }
    }
    const bool open = held && state_.hits_left > 0;
    if (open && !state_.open) { play_feedback_sound(open_sound_, weapon_tuning_->canopy.open_sound); }
    state_.open = open;
}

void WeaponUmbrella::prepare_canopy(PlayerInputData input, const Vector2F& player_center,
    bool facing_right, float dt)
{
    update_movement_and_rotation(input, player_center, facing_right);
    advance_canopy(input.primary_shoot, dt);
    prepared_ = true;
}

bool WeaponUmbrella::check_if_shooting_and_ammo_update(PlayerInputData input, float dt, float multiplier)
{
    if (!prepared_) { advance_canopy(input.primary_shoot, dt); }
    prepared_ = false;
    return Weapon::check_if_shooting_and_ammo_update(input, dt, multiplier);
}

RectangleF WeaponUmbrella::canopy_bounds() const
{
    const auto& canopy = weapon_tuning_->canopy;
    const Vector2F centre = player_center() + Vector2F(invert_y() ? -canopy.reach : canopy.reach, 0.0f);
    return RectangleF(centre - canopy.size * 0.5f, canopy.size);
}

std::optional<WeaponCanopy> WeaponUmbrella::canopy() const
{
    if (!state_.open) { return std::nullopt; }
    return WeaponCanopy{ canopy_bounds(), team() };
}

void WeaponUmbrella::take_canopy_hit()
{
    if (!state_.open) { return; }
    if (--state_.hits_left == 0)
    {
        state_.open = false;
        state_.recovery_remaining = weapon_tuning_->canopy.recover;
        play_feedback_sound(break_sound_, weapon_tuning_->canopy.break_sound);
    }
    else { play_feedback_sound(block_sound_, weapon_tuning_->canopy.block_sound); }
}

void WeaponUmbrella::handle_shoot_sound(bool fired, bool /*holding*/)
{
    play_one_shot(fired);
}

std::vector<std::unique_ptr<Projectile>> WeaponUmbrella::shoot(const Vector2F& direction,
    const Vector2F& player_velocity) const
{
    const int count = weapon_tuning_->pellets;
    std::vector<std::unique_ptr<Projectile>> fan;
    fan.reserve(static_cast<size_t>(count));
    for (int pellet = 0; pellet < count; ++pellet)
    {
        const float fraction = count == 1 ? 0.0f :
            static_cast<float>(pellet) / static_cast<float>(count - 1) - 0.5f;
        const auto heading = Vector2F::rotate_vector(direction, fraction * weapon_tuning_->spread);
        const auto velocity = calculate_projectile_launch_velocity(heading, starting_vel_length(), player_velocity);
        fan.push_back(projectile_builder().build_pellet(nozzle_pos(), velocity, team(), player_num(),
            team_colour(), render_resources()));
    }
    return fan;
}
