#pragma once

#include "game/objects/weapon.h"

// One paid shot releases the whole fan. Holding Shoot operates the canopy
// independently of the tank and the firing interval.
class WeaponUmbrella final : public RelativeVelocityWeapon
{
public:
    WeaponUmbrella(const GameTuning* tuning, PlayerTeam team, int player_num, int sound_index,
        const labrador::Colour& team_colour, WeaponType type, const labrador::Vector2F& player_center,
        labrador::RenderResources* render_resources, const labrador::AudioResources* audio_resources,
        const PlaySettings* preferences);
    void prepare_canopy(PlayerInputData input, const labrador::Vector2F& player_center,
        bool facing_right, float dt) override;
    std::optional<WeaponCanopy> canopy() const override;
    WeaponCanopyState canopy_state() const override { return state_; }
    void take_canopy_hit() override;
    void reset_canopy() override;
    labrador::RectangleF canopy_bounds() const;
    void draw(labrador::DrawList& draw_list, bool debug = false) const override;
    labrador::RectangleF draw_bounds() const override;

protected:
    ToolMotionPose motion_pose() const override;
    std::vector<std::unique_ptr<Projectile>> shoot(const labrador::Vector2F& shoot_direction,
        const labrador::Vector2F& player_velocity) const override;
    bool check_if_shooting_and_ammo_update(PlayerInputData input, float dt,
        float ammo_regen_multiplier) override;
    void handle_shoot_sound(bool fired, bool holding) override;

private:
    void advance_canopy(bool held, float dt);
    void load_canopy_art();
    const labrador::RenderResources::SheetFrame* visible_canopy_frame() const;
    std::array<labrador::RenderResources::SheetFrame, 3> canopy_frames_{};
    WeaponCanopyState state_;
    // True only between the two halves of a live Level tick, so direct weapon
    // updates and the pre-contact sweep advance the same clock exactly once.
    bool prepared_ = false;
    labrador::SoundBank::WaveHandle open_sound_;
    labrador::SoundBank::WaveHandle block_sound_;
    labrador::SoundBank::WaveHandle break_sound_;
};
