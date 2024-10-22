#ifndef PLAYER_H
#define PLAYER_H

#include "player_consts.h"
#include "wep_type.h"
#include "player_team.h"
#include "player_input.h"
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

class Player : public MovingObject, public ICollisionGameObject,
    public AnimationObject, public TextureObject
{
public:
    Player(const MattMath::RectangleF& rectangle,
        const player_animation_info& animation_info,
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

    virtual void update() override;
    virtual std::vector<std::unique_ptr<ICollisionGameObject>>
        update_weapon_and_get_projectiles();
    virtual void draw(const MattMath::Camera& camera) override;
    virtual void draw() override;

    virtual bool is_visible_in_viewport(const MattMath::RectangleF& view) const override;

    virtual bool is_colliding(const ICollisionGameObject* other) const override;
    virtual void on_collision(const ICollisionGameObject* other) override;
    virtual CollisionObjectType get_collision_object_type() const override;
    virtual const MattMath::Shape* get_shape() const override;
    virtual bool get_for_deletion() const override;

    virtual void set_player_input(const player_input& input);
    virtual void set_camera(const MattMath::Camera& camera) { this->_camera = camera; }

    virtual const MattMath::RectangleF& get_rectangle() const;
    virtual MattMath::Vector2F get_center() const;
    virtual const MattMath::Vector2F& get_velocity() const;
    virtual const MattMath::Camera& get_camera() const;
    virtual const MattMath::Viewport& get_viewport() const;
    virtual float get_input_x_movement() const;
    virtual const MattMath::Vector2F& get_input_shoot_direction() const;
    virtual bool get_input_primary_shoot() const;
    virtual bool get_input_jump_pressed() const;
    virtual bool get_input_jump_held() const;
    virtual player_move_state get_move_state() const;
    virtual int get_player_num() const { return this->_player_num; }

    virtual bool get_showing_debug() const;

    virtual float get_health() const { return this->_health; }

    virtual float get_weapon_ammo() const { return this->_primary->get_ammo(); }
    virtual const MattMath::Colour& get_team_colour() const { return this->_team_colour; }

    virtual const player_input& get_input() const { return this->_input; }

    virtual player_state get_state() const { return this->_state; }
    virtual void set_state(player_state state) { this->_state = state; }

    virtual player_team get_team() const { return this->_team; }

    float get_respawn_timer() const { return this->_respawn_timer; }

    void update_weapon_position();
    void update_prev_rectangle();
    void stop_sounds();

protected:
    virtual void set_player_num(int player_num) { this->_player_num = player_num; }


    virtual void set_team(player_team team) { this->_team = team; }

    virtual wep_type get_primary() const { return this->_primary_type; }
    virtual void set_primary(wep_type primary) { this->_primary_type = primary; }

    virtual wep_type get_secondary() const { return this->_secondary_type; }
    virtual void set_secondary(wep_type secondary) { this->_secondary_type = secondary; }

    virtual void set_team_colour(const MattMath::Colour& team_colour) { this->_team_colour = team_colour; }

    virtual void set_viewport(const MattMath::Viewport& viewport) { this->_viewport = viewport; }

    virtual bool get_facing_right() const { return this->_facing_right; }
    virtual void set_facing_right(bool facing_right) { this->_facing_right = facing_right; }

    virtual void set_input(const player_input& input) { this->_input = input; }

    virtual void alter_health(float change) { this->_health += change; }
    virtual void set_health(float health) { this->_health = health; }

    virtual void set_respawn_timer(float respawn_timer) { this->_respawn_timer = respawn_timer; }
    virtual void alter_respawn_timer(float change) { this->_respawn_timer += change; }

    virtual float get_air_time() const { return this->_air_time; }
    virtual void set_air_time(float air_time) { this->_air_time = air_time; }
    virtual void alter_air_time(float change) { this->_air_time += change; }

    virtual int get_score() const { return this->_score; }
    virtual void set_score(int score) { this->_score = score; }
    virtual void alter_score(int change) { this->_score += change; }

    virtual void set_showing_debug(bool showing_debug) { this->_showing_debug = showing_debug; }

    virtual void set_move_state(player_move_state move_state) { this->_move_state = move_state; }
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
    player_input _input = player_input();
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

    virtual const MattMath::RectangleF* get_collision_rectangle() const;

    virtual bool is_matching_collision_object_type(
        const ICollisionGameObject* other) const;

    virtual void on_projectile_collision(const ICollisionGameObject* other);
    virtual void on_structure_collision(const ICollisionGameObject* other);

    virtual float get_dt() const;

    virtual void update_movement();
    virtual void do_jump();

    player_animation_state calculate_animation_state() const;

    const player_animation_info& get_animation_info(player_animation_state state) const;

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
};

#endif // !PLAYER_H
