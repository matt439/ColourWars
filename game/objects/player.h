#pragma once

#include "game/objects/player_consts.h"
#include "game/objects/weapon_type.h"
#include "game/objects/player_team.h"
#include "game/objects/player_input_data.h"
#include "game/objects/weapon.h"
#include "engine/math/matt_math.h"
#include "engine/render/animation_object.h"
#include "engine/collision/collision_object.h"
#include "game/objects/collision_object_type.h"
#include "engine/core/moving_object.h"
#include "engine/audio/sound_bank.h"
#include "engine/audio/audio_resources.h"
#include "engine/render/colour.h"
#include "engine/render/camera.h"
#include "engine/render/viewport.h"

enum class PlayerState
{
    alive,
    dead
};

enum class PlayerMoveState
{
	on_ground,
    on_drop_down_ground,
    on_ramp_left,
	on_ramp_right,
    dropping_down,
	on_ceiling,
    jumping,
    in_air,
};

enum class PlayerAnimationState
{
	idle,
	walking,
	jumping,
	falling,
    running,
};

//enum class player_collision_type
//{
//    top_edge,
//    bottom_edge,
//    left_edge,
//    right_edge,
//    top_and_bottom_edges,
//    left_and_right_edges,
//    top_and_left_edges,
//    top_and_right_edges,
//    bottom_and_left_edges,
//    bottom_and_right_edges,
//    top_and_bottom_and_left_edges,
//    top_and_bottom_and_right_edges,
//    left_and_right_and_top_edges,
//    left_and_right_and_bottom_edges,
//    contained_inside_other,
//    contains_other,
//};

class Player final : public artattack::MovingObject,
                     public artattack::CollisionObject,
                     public artattack::AnimationObject, public artattack::TextureObject
{
public:
    Player(const mattmath::RectangleF& rectangle,
        const PlayerAnimationInfo& animation_info,
        artattack::RenderResources* render_resources,
        const artattack::AudioResources* audio_resources,
        int player_num,
        PlayerTeam team,
        WeaponType primary,
        std::unique_ptr<Weapon> primary_weapon,
        WeaponType secondary,
        const artattack::Colour& team_colour,
        const artattack::Viewport& view_port,
        const mattmath::Vector2F& respawn_position,
        const mattmath::Vector2F& velocity = mattmath::Vector2F::ZERO,
        float rotation = 0.0f,
        const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
        artattack::SpriteFlip flip = artattack::SpriteFlip::none,
        float layer_depth = 0.0f);

    void update(float dt) override;
    std::vector<std::unique_ptr<artattack::CollisionObject>>
        update_weapon_and_get_projectiles(float dt) const;
    void draw(artattack::DrawList& draw_list) const override;

    mattmath::RectangleF bounds() const override;

    const mattmath::Shape* shape() const override;
    artattack::CollisionLayer layer() const override;
    artattack::CollisionMask mask() const override;
    artattack::CollisionTag tag() const override;
    void on_contact(const artattack::CollisionObject& other,
        const mattmath::Vector2F& normal, float penetration) override;
    // Always false, and Level rests on it: a player is in the same scene
    // object list as the projectiles it fires, and Level holds bare Player*
    // for the input, the cameras and the HUD. Returning true here would have
    // the scene retire the object those pointers name. A dead player respawns;
    // it does not leave the world.
    bool for_deletion() const override;

    // Closes the frame's contact phase for this player.
    //
    // "I touched no ground this tick" is not something a contact can say, so
    // it is the absence of one - and the absence is only known once they have
    // all been dispatched. The level used to work this out itself with a bool
    // hoisted above the inner loop, which is a property of the player kept by
    // whoever happened to be iterating.
    void end_contacts();

	void set_player_input(const PlayerInputData& input);
    void set_camera(const artattack::Camera& camera);

	const mattmath::RectangleF& rectangle() const;
	mattmath::Vector2F center() const;
    // Republishes MovingObject's protected accessor. Not an override - it
    // never overrode anything meaningfully, and MovingObject's accessors are
    // no longer virtual.
    const mattmath::Vector2F& velocity() const;
	const artattack::Camera& camera() const;
	const artattack::Viewport& viewport() const;
	float input_x_movement() const;
	const mattmath::Vector2F& input_shoot_direction() const;
	bool input_primary_shoot() const;
	bool input_jump_pressed() const;
	bool input_jump_held() const;
	PlayerMoveState move_state() const;
    int player_num() const;

	bool showing_debug() const;

    float health() const;

    float weapon_ammo() const;
    const artattack::Colour& team_colour() const;

    const PlayerInputData& input() const;

    PlayerState state() const;
    void set_state(PlayerState state);

    PlayerTeam team() const;

    float respawn_timer() const;

    void update_weapon_position() const;
    void update_prev_rectangle();
    void stop_sounds() const;

    // Everything this player is making a noise with, quieted while a menu is
    // over the match and brought back when it goes.
    void pause_sounds() const;
    void resume_sounds() const;

	std::string player_move_state_string() const;

private:
    artattack::Camera camera_ = artattack::Camera::DEFAULT_CAMERA;

    std::unique_ptr<Weapon> primary_ = nullptr;

    float health_ = player_consts::STARTING_HEALTH;
    float damage_sound_timer_ = 0.0f;
    float air_time_ = 0.0f;
    bool facing_right_ = true;
    int score_ = 0;
    bool showing_debug_ = false;
    PlayerState state_ = PlayerState::alive;
    float respawn_timer_ = player_consts::RESPAWN_DELAY;
    PlayerInputData input_ = PlayerInputData();
    PlayerMoveState move_state_ = PlayerMoveState::in_air;
    PlayerAnimationState animation_state_ = PlayerAnimationState::idle;

    int player_num_ = player_consts::DEFAULT_PLAYER_NUM;
    PlayerTeam team_ = player_consts::DEFAULT_TEAM;
    WeaponType primary_type_ = player_consts::DEFAULT_PRIMARY;
    WeaponType secondary_type_ = player_consts::DEFAULT_SECONDARY;
    artattack::Colour team_colour_ = player_consts::DEFAULT_TEAM_COLOUR;
    artattack::Viewport viewport_ = artattack::Viewport();

    mattmath::RectangleF rectangle_ = player_consts::DEFAULT_BOUNDS;
    mattmath::RectangleF prev_rectangle_ = player_consts::DEFAULT_BOUNDS;

    mattmath::Vector2F respawn_position_ = mattmath::Vector2F::ZERO;

    artattack::SoundBank* sound_bank_ = nullptr;

    // Resolved once at construction. These fire from update(), so the map
    // descent they used to cost was on the frame loop (T7, T8).
    artattack::SoundBank::WaveHandle damage_sound_;
    artattack::SoundBank::WaveHandle death_sound_;
    artattack::SoundBank::WaveHandle jump_sound_;
    artattack::SoundBank::WaveHandle respawn_sound_;


    float health_regen_timer_ = 0.0f;

	const mattmath::RectangleF* collision_rectangle() const;

	CollisionObjectType collision_type() const;

	// Set by any level-geometry contact and cleared by end_contacts().
	bool touched_structure_ = false;

	void on_projectile_contact(CollisionObjectType other_type);

	// One response for all four faces of a flat structure.
	//
	// The eight-way classification this replaces - top, bottom, left, right
	// and four corners, the corners re-measuring the depth along two axes to
	// pick between two of the others - was approximating the axis of least
	// penetration, having chosen its candidate axes by a different and worse
	// rule. The normal is that axis, exactly, and it arrives measured.
	void on_structure_contact(const mattmath::Vector2F& normal,
		float penetration);

	void on_ramp_contact(CollisionObjectType other_type,
		const mattmath::Vector2F& normal, float penetration);

	void on_jump_through_contact(const artattack::CollisionObject& other);

	void update_movement(float dt);
	void do_jump(float dt);

    PlayerAnimationState calculate_animation_state() const;

    // Selects the animation clip for this frame. Called from update(), never
    // from draw() - draw runs concurrently on every render worker.
    void update_animation_state();

    static const PlayerAnimationInfo& animation_info(PlayerAnimationState state);

    void respawn();

    void set_player_num(int player_num);


    void set_team(PlayerTeam team);

    WeaponType primary() const;
    void set_primary(WeaponType primary);

    WeaponType secondary() const;
    void set_secondary(WeaponType secondary);

    void set_team_colour(const artattack::Colour& team_colour);

    void set_viewport(const artattack::Viewport& viewport);

    bool facing_right() const;
    void set_facing_right(bool facing_right);

    void set_input(const PlayerInputData& input);

    void alter_health(float change);
    void set_health(float health);

    void set_respawn_timer(float respawn_timer);
    void alter_respawn_timer(float change);

    float air_time() const;
    void set_air_time(float air_time);
    void alter_air_time(float change);

    int score() const;
    void set_score(int score);
    void alter_score(int change);

    void set_showing_debug(bool showing_debug);

    void set_move_state(PlayerMoveState move_state);

	bool is_on_ground() const;
    bool is_on_ramp() const;
};
