#ifndef PLAYER_H
#define PLAYER_H

#include "player_consts.h"
#include "wep_type.h"
#include "player_team.h"
#include "PlayerInputData.h"
#include "Weapon.h"
#include "MattMath.h"
#include "AnimationObject.h"
#include "ICollisionGameObject.h"
#include "MovingObject.h"
#include "SoundBank.h"

enum class player_state
{
    ALIVE,
    DEAD
};

enum class player_move_state
{
	ON_GROUND,
    ON_DROP_DOWN_GROUND,
    DROPPING_DOWN,
	ON_CEILING,
    JUMPING,
    IN_AIR,
};

enum class player_animation_state
{
	IDLE,
	WALKING,
	JUMPING,
	FALLING,
    RUNNING,
};

enum class player_collision_type
{
    TOP_EDGE,
    BOTTOM_EDGE,
    LEFT_EDGE,
    RIGHT_EDGE,
    TOP_AND_BOTTOM_EDGES,
    LEFT_AND_RIGHT_EDGES,
    TOP_AND_LEFT_EDGES,
    TOP_AND_RIGHT_EDGES,
    BOTTOM_AND_LEFT_EDGES,
    BOTTOM_AND_RIGHT_EDGES,
    TOP_AND_BOTTOM_AND_LEFT_EDGES,
    TOP_AND_BOTTOM_AND_RIGHT_EDGES,
    LEFT_AND_RIGHT_AND_TOP_EDGES,
    LEFT_AND_RIGHT_AND_BOTTOM_EDGES,
    CONTAINED_INSIDE_OTHER,
    CONTAINS_OTHER,
};

class Player final : public MovingObject, public ICollisionGameObject,
                     public AnimationObject, public TextureObject
{
public:
    Player(const MattMath::RectangleF& rectangle,
        const PlayerAnimationInfo& animation_info,
        DirectX::SpriteBatch* sprite_batch,
        ResourceManager* resource_manager,
        int player_num,
        player_team team,
        wep_type primary,
        std::unique_ptr<Weapon> primary_weapon,
        wep_type secondary,
        const MattMath::Colour& team_colour,
        const MattMath::Viewport& view_port,
        const float* dt,
        const MattMath::Vector2F& respawn_position,
        const MattMath::Vector2F& velocity = MattMath::Vector2F::ZERO,
        float rotation = 0.0f,
        const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
        DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
        float layer_depth = 0.0f);

    void update() override;
    std::vector<std::unique_ptr<ICollisionGameObject>>
        update_weapon_and_get_projectiles() const;
    void draw(const MattMath::Camera& camera) override;
    void draw() override;

    bool is_visible_in_viewport(const MattMath::RectangleF& view) const override;

    bool is_colliding(const ICollisionGameObject* other) const override;
    void on_collision(const ICollisionGameObject* other) override;
    collision_object_type get_collision_object_type() const override;
    const MattMath::Shape* get_shape() const override;
    bool get_for_deletion() const override;

	void set_player_input(const PlayerInputData& input);
    void set_camera(const MattMath::Camera& camera);

	const MattMath::RectangleF& get_rectangle() const;
	MattMath::Vector2F get_center() const;
    const MattMath::Vector2F& get_velocity() const override;
	const MattMath::Camera& get_camera() const;
	const MattMath::Viewport& get_viewport() const;
	float get_input_x_movement() const;
	const MattMath::Vector2F& get_input_shoot_direction() const;
	bool get_input_primary_shoot() const;
	bool get_input_jump_pressed() const;
	bool get_input_jump_held() const;
	player_move_state get_move_state() const;
    int get_player_num() const;

	bool get_showing_debug() const;

    float get_health() const;

    float get_weapon_ammo() const;
    const MattMath::Colour& get_team_colour() const;

    const PlayerInputData& get_input() const;

    player_state get_state() const;
    void set_state(player_state state);

    player_team get_team() const;

    float get_respawn_timer() const;

    void update_weapon_position() const;
    void update_prev_rectangle();
    void stop_sounds() const;

private:
    MattMath::Camera _camera = MattMath::Camera::DEFAULT_CAMERA;

    std::unique_ptr<Weapon> _primary = nullptr;

    float _health = player_consts::STARTING_HEALTH;
    float _damage_sound_timer = 0.0f;
    float _air_time = 0.0f;
    bool _facing_right = true;
    int _score = 0;
    bool _showing_debug = false;
    player_state _state = player_state::ALIVE;
    float _respawn_timer = player_consts::RESPAWN_DELAY;
    PlayerInputData _input = PlayerInputData();
    player_move_state _move_state = player_move_state::IN_AIR;
    player_animation_state _animation_state = player_animation_state::IDLE;

    int _player_num = player_consts::DEFAULT_PLAYER_NUM;
    player_team _team = player_consts::DEFAULT_TEAM;
    wep_type _primary_type = player_consts::DEFAULT_PRIMARY;
    wep_type _secondary_type = player_consts::DEFAULT_SECONDARY;
    MattMath::Colour _team_colour = player_consts::DEFAULT_TEAM_COLOUR;
    MattMath::Viewport _viewport = MattMath::Viewport();

    MattMath::RectangleF _rectangle = player_consts::DEFAULT_BOUNDS;
    MattMath::RectangleF _prev_rectangle = player_consts::DEFAULT_BOUNDS;

    MattMath::Vector2F _respawn_position = MattMath::Vector2F::ZERO;

    SoundBank* _sound_bank = nullptr;

    const float* _dt = nullptr;

    float _health_regen_timer = 0.0f;

	const MattMath::RectangleF* get_collision_rectangle() const;

    static bool is_matching_collision_object_type(
        const ICollisionGameObject* other);

	void on_projectile_collision(const ICollisionGameObject* other);
	void on_structure_collision(const ICollisionGameObject* other);

	float get_dt() const;

	void update_movement();
	void do_jump();

    player_animation_state calculate_animation_state() const;

    static const PlayerAnimationInfo& get_animation_info(player_animation_state state);

    void respawn();

    player_collision_type calculate_collision_type(const ICollisionGameObject* other) const;

    void on_top_collision(const ICollisionGameObject* other);
    void on_bottom_collision(const ICollisionGameObject* other);
    void on_left_collision(const ICollisionGameObject* other);
    void on_right_collision(const ICollisionGameObject* other);
    void on_top_left_collision(const ICollisionGameObject* other);
    void on_top_right_collision(const ICollisionGameObject* other);
    void on_bottom_left_collision(const ICollisionGameObject* other);
    void on_bottom_right_collision(const ICollisionGameObject* other);

    void on_structure_jump_through_collision(const ICollisionGameObject* other);

    void set_player_num(int player_num);


    void set_team(player_team team);

    wep_type get_primary() const;
    void set_primary(wep_type primary);

    wep_type get_secondary() const;
    void set_secondary(wep_type secondary);

    void set_team_colour(const MattMath::Colour& team_colour);

    void set_viewport(const MattMath::Viewport& viewport);

    bool get_facing_right() const;
    void set_facing_right(bool facing_right);

    void set_input(const PlayerInputData& input);

    void alter_health(float change);
    void set_health(float health);

    void set_respawn_timer(float respawn_timer);
    void alter_respawn_timer(float change);

    float get_air_time() const;
    void set_air_time(float air_time);
    void alter_air_time(float change);

    int get_score() const;
    void set_score(int score);
    void alter_score(int change);

    void set_showing_debug(bool showing_debug);

    void set_move_state(player_move_state move_state);
};

#endif // !PLAYER_H
