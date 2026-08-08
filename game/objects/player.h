#pragma once

#include "game/objects/player_consts.h"
#include "game/objects/weapon_type.h"
#include "game/objects/player_team.h"
#include "game/objects/player_input_data.h"
#include "game/objects/weapon.h"
#include "engine/math/matt_math.h"
#include "engine/render/animation_object.h"
#include "game/objects/collision_object.h"
#include "engine/core/moving_object.h"
#include "engine/audio/sound_bank.h"
#include "engine/audio/audio_resources.h"
#include "engine/math/collision_tools.h"

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

class Player final : public artattack::MovingObject, public CollisionObject,
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
        const mattmath::Colour& team_colour,
        const mattmath::Viewport& view_port,
        const mattmath::Vector2F& respawn_position,
        const mattmath::Vector2F& velocity = mattmath::Vector2F::ZERO,
        float rotation = 0.0f,
        const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
        DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
        float layer_depth = 0.0f);

    void update(float dt) override;
    std::vector<std::unique_ptr<CollisionObject>>
        update_weapon_and_get_projectiles(float dt) const;
    void draw(DirectX::SpriteBatch* sprite_batch,
        const mattmath::Camera& camera) const override;

    mattmath::RectangleF bounds() const override;

    bool is_colliding(const CollisionObject* other) const override;
    void on_collision(const CollisionObject* other) override;
    CollisionObjectType collision_object_type() const override;
    const mattmath::Shape* shape() const override;
    bool for_deletion() const override;

	void set_player_input(const PlayerInputData& input);
    void set_camera(const mattmath::Camera& camera);

	const mattmath::RectangleF& rectangle() const;
	mattmath::Vector2F center() const;
    // Republishes MovingObject's protected accessor. Not an override - it
    // never overrode anything meaningfully, and MovingObject's accessors are
    // no longer virtual.
    const mattmath::Vector2F& velocity() const;
	const mattmath::Camera& camera() const;
	const mattmath::Viewport& viewport() const;
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
    const mattmath::Colour& team_colour() const;

    const PlayerInputData& input() const;

    PlayerState state() const;
    void set_state(PlayerState state);

    PlayerTeam team() const;

    float respawn_timer() const;

    void update_weapon_position() const;
    void update_prev_rectangle();
    void stop_sounds() const;

	std::string player_move_state_string() const;

	void on_no_collision();

private:
    mattmath::Camera camera_ = mattmath::Camera::DEFAULT_CAMERA;

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
    mattmath::Colour team_colour_ = player_consts::DEFAULT_TEAM_COLOUR;
    mattmath::Viewport viewport_ = mattmath::Viewport();

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

    static bool is_matching_collision_object_type(
        const CollisionObject* other);

	void on_projectile_collision(const CollisionObject* other);
	void on_structure_collision(const CollisionObject* other);


	void update_movement(float dt);
	void do_jump(float dt);

    PlayerAnimationState calculate_animation_state() const;

    // Selects the animation clip for this frame. Called from update(), never
    // from draw() - draw runs concurrently on every render worker.
    void update_animation_state();

    static const PlayerAnimationInfo& animation_info(PlayerAnimationState state);

    void respawn();

    //player_collision_type calculate_collision_type(const CollisionObject* other) const;

    void on_top_collision(const CollisionObject* other);
    void on_bottom_collision(const CollisionObject* other);
    void on_left_collision(const CollisionObject* other);
    void on_right_collision(const CollisionObject* other);
    void on_top_left_collision(const CollisionObject* other);
    void on_top_right_collision(const CollisionObject* other);
    void on_bottom_left_collision(const CollisionObject* other);
    void on_bottom_right_collision(const CollisionObject* other);

	//void on_ground_collision();
	//void on_ceiling_collision();
	//void on_wall_collision();

    void on_structure_jump_through_collision(const CollisionObject* other);

	void on_structure_ramp_collision(const CollisionObject* other);

    void set_player_num(int player_num);


    void set_team(PlayerTeam team);

    WeaponType primary() const;
    void set_primary(WeaponType primary);

    WeaponType secondary() const;
    void set_secondary(WeaponType secondary);

    void set_team_colour(const mattmath::Colour& team_colour);

    void set_viewport(const mattmath::Viewport& viewport);

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
