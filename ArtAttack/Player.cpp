#include "pch.h"
#include "Player.h"

using namespace DirectX;
using namespace MattMath;
using namespace player_consts;
using namespace colour_consts;

Player::Player(const RectangleF& rectangle,
    const PlayerAnimationInfo& animation_info,
    SpriteBatch* sprite_batch,
    ResourceManager* resource_manager,
    int player_num,
    player_team team,
    wep_type primary,
    std::unique_ptr<Weapon> primary_weapon,
    wep_type secondary,
    const Colour& team_colour,
    const Viewport& view_port,
    const float* dt,
    const MattMath::Vector2F& respawn_position,
    const Vector2F& velocity,
    float rotation,
    const Vector2F& origin,
    SpriteEffects effects,
    float layer_depth) :
    MovingObject(velocity),
    AnimationObject(dt, animation_info.sprite_sheet,
                    animation_info.animation, sprite_batch,
                    resource_manager, DEFAULT_PLAYER_COLOUR, rotation, origin, effects, layer_depth),
    TextureObject(animation_info.sprite_sheet, animation_info.uniform_texture, sprite_batch,
                  resource_manager, team_colour, rotation, origin, effects, layer_depth),
    _primary(std::move(primary_weapon)),
    _player_num(player_num),
    _team(team),
    _primary_type(primary),
    _secondary_type(secondary),
    _team_colour(team_colour),
    _viewport(view_port),
    _rectangle(rectangle),
    _prev_rectangle(rectangle),
    _respawn_position(respawn_position),
    _dt(dt)
{
    this->_sound_bank = resource_manager->get_sound_bank(SOUND_BANK);
}

void Player::draw(const Camera& camera)
{
    if (this->_state == player_state::DEAD)
    {
        return;
    }
    
    player_animation_state current_animation_state = this->_animation_state;
    player_animation_state new_animation_state = this->calculate_animation_state();

    if (current_animation_state != new_animation_state)
    {
		this->_animation_state = new_animation_state;

        const PlayerAnimationInfo& info = this->get_animation_info(new_animation_state);
        TextureObject::set_sprite_sheet_name(info.sprite_sheet);
        TextureObject::set_element_name(info.uniform_texture);

        set_animation_strip_and_reset(info.sprite_sheet, info.animation);
        if (info.frame_time != FLT_MIN)
        {
            set_frame_time(info.frame_time);
		}
        else
        {
			set_frame_time_to_default();
        }
	}

    SpriteEffects effects = SpriteEffects_None;
    bool facing_right = this->get_facing_right();
    if (!facing_right)
    {
        effects = SpriteEffects_FlipHorizontally;
    }

    TextureObject::set_effects(effects);
    TextureObject::draw(this->_rectangle, camera);

    AnimationObject::set_effects(effects);
    AnimationObject::draw(this->_rectangle, camera);

    this->_primary->draw(camera, this->_showing_debug);
}
void Player::draw()
{
    AnimationObject::draw(this->_rectangle, Camera::DEFAULT_CAMERA);
	this->_primary->draw(Camera::DEFAULT_CAMERA);
}
collision_object_type Player::get_collision_object_type() const
{
    if (this->_state == player_state::ALIVE)
    {
        switch (this->_team)
        {
        case player_team::A:
            return collision_object_type::PLAYER_TEAM_A;
        case player_team::B:
            return collision_object_type::PLAYER_TEAM_B;
        default:
            throw std::exception("Invalid player team");
        };
    }
    else if (this->_state == player_state::DEAD)
	{
		switch (this->_team)
		{
		case player_team::A:
			return collision_object_type::PLAYER_TEAM_A_DEAD;
		case player_team::B:
			return collision_object_type::PLAYER_TEAM_B_DEAD;
		default:
			throw std::exception("Invalid player team");
		};
	}
	else
	{
		throw std::exception("Invalid player state");
	}

}
const Shape* Player::get_shape() const
{
	return this->get_collision_rectangle();
}
bool Player::get_for_deletion() const
{
	return false;
}

const RectangleF* Player::get_collision_rectangle() const
{
	return &this->_rectangle;
}

bool Player::is_matching_collision_object_type(
    const ICollisionGameObject* other)
{
    collision_object_type other_type = other->get_collision_object_type();

    bool structure_collision =
        other_type == collision_object_type::STRUCTURE ||
        other_type == collision_object_type::STRUCTURE_PAINTABLE ||
        other_type == collision_object_type::STRUCTURE_JUMP_THROUGH;

    return structure_collision;
}
bool Player::is_colliding(const ICollisionGameObject* other) const
{
    // dead check
    if (this->_state == player_state::DEAD)
	{
		return false;
	}
    
    // type check
    if (!is_matching_collision_object_type(other))
    {
        return false;
    }

    // aabb check
    if (!this->get_shape()->AABB_intersects(other->get_shape()))
    {
        return false;
    }
    else // AABBs are intersecting
    {
        // if the other object is a rectangle, then we have a collision
        // since the AABB check passed
        shape_type other_shape_type = other->get_shape()->get_shape_type();
        if (other_shape_type == shape_type::RECTANGLE)
        {
            return true;
        }
    }

    // narrow phase check
    if (this->get_shape()->intersects(other->get_shape()))
    {
        return true;
    }

    return false;
}
void Player::on_collision(const ICollisionGameObject* other)
{
    collision_object_type other_type = other->get_collision_object_type();

    bool structure_collision =
        other_type == collision_object_type::STRUCTURE ||
        other_type == collision_object_type::STRUCTURE_PAINTABLE;

    bool structure_jump_through_collision =
		other_type == collision_object_type::STRUCTURE_JUMP_THROUGH;

    player_team team = this->_team;
    bool projectile_collision = false;
    if (team == player_team::A)
    {
            bool projectile_team_b =
            other_type == collision_object_type::PROJECTILE_SPRAY_TEAM_B ||
            other_type == collision_object_type::PROJECTILE_JET_TEAM_B ||
            other_type == collision_object_type::PROJECTILE_ROLLING_TEAM_B ||
            other_type == collision_object_type::PROJECTILE_BALL_TEAM_B ||
            other_type == collision_object_type::PROJECTILE_MIST_TEAM_B;

        projectile_collision = projectile_team_b;
    }
    else if (team == player_team::B)
    {
        bool projectile_team_a =
            other_type == collision_object_type::PROJECTILE_SPRAY_TEAM_A ||
            other_type == collision_object_type::PROJECTILE_JET_TEAM_A ||
            other_type == collision_object_type::PROJECTILE_ROLLING_TEAM_A ||
            other_type == collision_object_type::PROJECTILE_BALL_TEAM_A ||
            other_type == collision_object_type::PROJECTILE_MIST_TEAM_A;

        projectile_collision = projectile_team_a;
    }

    if (projectile_collision)
	{
		this->on_projectile_collision(other);
	}
    else if (structure_collision)
    {
		this->on_structure_collision(other);
	}
    else if (structure_jump_through_collision)
	{
		this->on_structure_jump_through_collision(other);
	}
	else
	{
		throw std::exception("Invalid collision object type.");
	}
}
void Player::on_structure_jump_through_collision(const ICollisionGameObject* other)
{
    // only collide if player is moving down and on previous cycle
    // was above the structure
    
    Vector2F velocity = this->get_velocity();
    direction dir = velocity.get_direction();

    bool moving_down = dir == direction::DOWN || dir == direction::DOWN_LEFT ||
        dir == direction::DOWN_RIGHT;

    const RectangleF& other_rect = other->get_shape()->get_bounding_box();
    bool was_above = this->_prev_rectangle.get_bottom() <= other_rect.get_top();

    player_move_state move_state = this->get_move_state();

    if (moving_down && was_above && move_state != player_move_state::DROPPING_DOWN)
	{
        MovingObject::set_velocity_y(0.0f);

        const RectangleF& rect = other->get_shape()->get_bounding_box();
        this->_rectangle.set_position_y_from_bottom(rect.get_top());
        this->set_move_state(player_move_state::ON_DROP_DOWN_GROUND);
	}
	else
	{
		// no collision
	}
}
void Player::on_structure_collision(const ICollisionGameObject* other)
{
    player_collision_type type = this->calculate_collision_type(other);
    if (type == player_collision_type::LEFT_EDGE)
	{
		this->on_left_collision(other);
	}
	else if (type == player_collision_type::RIGHT_EDGE)
	{
		this->on_right_collision(other);
	}
	else if (type == player_collision_type::TOP_EDGE)
	{
		this->on_top_collision(other);
	}
	else if (type == player_collision_type::BOTTOM_EDGE)
	{
		this->on_bottom_collision(other);
	}
	else if (type == player_collision_type::LEFT_AND_RIGHT_EDGES)
	{
        throw std::exception("Player is colliding with both left and right edges.");
	}
	else if (type == player_collision_type::TOP_AND_BOTTOM_EDGES)
	{
        throw std::exception("Player is colliding with both top and bottom edges.");
	}
	else if (type == player_collision_type::TOP_AND_LEFT_EDGES)
	{
        this->on_top_left_collision(other);
	}
	else if (type == player_collision_type::TOP_AND_RIGHT_EDGES)
	{
        this->on_top_right_collision(other);
	}
	else if (type == player_collision_type::BOTTOM_AND_LEFT_EDGES)
	{
        this->on_bottom_left_collision(other);
	}
	else if (type == player_collision_type::BOTTOM_AND_RIGHT_EDGES)
	{
        this->on_bottom_right_collision(other);
	}
	else if (type == player_collision_type::LEFT_AND_RIGHT_AND_TOP_EDGES)
	{
		this->on_top_collision(other);
	}
	else if (type == player_collision_type::LEFT_AND_RIGHT_AND_BOTTOM_EDGES)
	{
		this->on_bottom_collision(other);
	}
	else if (type == player_collision_type::TOP_AND_BOTTOM_AND_RIGHT_EDGES)
	{
		this->on_right_collision(other);
	}
	else if (type == player_collision_type::TOP_AND_BOTTOM_AND_LEFT_EDGES)
	{
		this->on_left_collision(other);
	}
    else if (type == player_collision_type::CONTAINED_INSIDE_OTHER)
    {
		throw std::exception("Player is contained inside other object.");
	}
    else if (type == player_collision_type::CONTAINS_OTHER)
    {
        throw std::exception("Player contains other object.");
    }
    else
    {
		throw std::exception("Invalid collision type.");
	}
}
void Player::on_top_collision(const ICollisionGameObject* other)
{
    MovingObject::set_velocity_y(0.0f);

	const RectangleF& other_rect = other->get_shape()->get_bounding_box();
	this->_rectangle.set_position_y(other_rect.get_bottom());
	this->set_move_state(player_move_state::ON_CEILING);
}
void Player::on_bottom_collision(const ICollisionGameObject* other)
{
    MovingObject::set_velocity_y(0.0f);

    const RectangleF& other_rect = other->get_shape()->get_bounding_box();
    this->_rectangle.set_position_y_from_bottom(other_rect.get_top());
    this->set_move_state(player_move_state::ON_GROUND);
}
void Player::on_left_collision(const ICollisionGameObject* other)
{
    this->set_velocity_x(0.0f);

    const RectangleF& other_rect = other->get_shape()->get_bounding_box();
    this->_rectangle.set_position_x(other_rect.get_right());

}
void Player::on_right_collision(const ICollisionGameObject* other)
{
    this->set_velocity_x(0.0f);

	const RectangleF& other_rect = other->get_shape()->get_bounding_box();
	this->_rectangle.set_position_x_from_right(other_rect.get_left());
}
void Player::on_top_left_collision(const ICollisionGameObject* other)
{
    const RectangleF& other_rect = other->get_shape()->get_bounding_box();
    const RectangleF intersection = this->_rectangle.intersection(other_rect);

    if (intersection.get_width() > intersection.get_height())
	{
		this->on_top_collision(other);
	}
	else // intersection.get_width() < intersection.get_height()
	{
		this->on_left_collision(other);
	}
}
void Player::on_top_right_collision(const ICollisionGameObject* other)
{
    const RectangleF& other_rect = other->get_shape()->get_bounding_box();
	const RectangleF intersection = this->_rectangle.intersection(other_rect);

	if (intersection.get_width() > intersection.get_height())
	{
		this->on_top_collision(other);
	}
	else // intersection.get_width() < intersection.get_height()
	{
		this->on_right_collision(other);
	}
}
void Player::on_bottom_left_collision(const ICollisionGameObject* other)
{
    const RectangleF& other_rect = other->get_shape()->get_bounding_box();
    const RectangleF intersection = this->_rectangle.intersection(other_rect);

    direction dir = this->get_velocity().get_direction();
    bool moving_up = dir == direction::UP || dir == direction::UP_LEFT ||
		dir == direction::UP_RIGHT;

    if (intersection.get_width() > intersection.get_height() && !moving_up)
	{
		this->on_bottom_collision(other);
	}
	else // intersection.get_width() < intersection.get_height()
	{
		this->on_left_collision(other);
	}
}
void Player::on_bottom_right_collision(const ICollisionGameObject* other)
{
    const RectangleF& other_rect = other->get_shape()->get_bounding_box();
    const RectangleF intersection = this->_rectangle.intersection(other_rect);

    direction dir = this->get_velocity().get_direction();
    bool moving_up = dir == direction::UP || dir == direction::UP_LEFT ||
        dir == direction::UP_RIGHT;

    if (intersection.get_width() > intersection.get_height() && !moving_up)
    {
	    this->on_bottom_collision(other);
}
    else // intersection.get_width() < intersection.get_height()
    {
        this->on_right_collision(other);
    }
}
player_collision_type Player::calculate_collision_type(const ICollisionGameObject* other) const
{
    const RectangleF& other_rect = other->get_shape()->get_bounding_box();
    const RectangleF& this_rect = this->_rectangle;

    bool left_edge = this_rect.get_left_edge().intersects(other_rect);
    bool right_edge = this_rect.get_right_edge().intersects(other_rect);
    bool top_edge = this_rect.get_top_edge().intersects(other_rect);
	bool bottom_edge = this_rect.get_bottom_edge().intersects(other_rect);

    bool contained_inside_other = other_rect.contains(this->_rectangle);
    bool contains_other = this->_rectangle.contains(other_rect);

    if (left_edge && right_edge && top_edge)
    {
        return player_collision_type::LEFT_AND_RIGHT_AND_TOP_EDGES;
    }
    else if (left_edge && right_edge && bottom_edge)
    {
        return player_collision_type::LEFT_AND_RIGHT_AND_BOTTOM_EDGES;
    }
    else if (top_edge && bottom_edge && right_edge)
    {
        return player_collision_type::TOP_AND_BOTTOM_AND_RIGHT_EDGES;
    }
    else if (top_edge && bottom_edge && left_edge)
    {
        return player_collision_type::TOP_AND_BOTTOM_AND_LEFT_EDGES;
    }
	else if (left_edge && right_edge)
	{
		return player_collision_type::LEFT_AND_RIGHT_EDGES;
	}
	else if (top_edge && bottom_edge)
	{
		return player_collision_type::TOP_AND_BOTTOM_EDGES;
	}
	else if (left_edge && top_edge)
	{
		return player_collision_type::TOP_AND_LEFT_EDGES;
	}
	else if (left_edge && bottom_edge)
	{
		return player_collision_type::BOTTOM_AND_LEFT_EDGES;
	}
	else if (right_edge && top_edge)
	{
		return player_collision_type::TOP_AND_RIGHT_EDGES;
	}
	else if (right_edge && bottom_edge)
	{
		return player_collision_type::BOTTOM_AND_RIGHT_EDGES;
	}
	else if (left_edge)
	{
		return player_collision_type::LEFT_EDGE;
	}
	else if (right_edge)
	{
		return player_collision_type::RIGHT_EDGE;
	}
	else if (top_edge)
	{
		return player_collision_type::TOP_EDGE;
	}
	else if (bottom_edge)
	{
		return player_collision_type::BOTTOM_EDGE;
	}
    else if (contained_inside_other)
    {
        throw std::exception("Player is contained inside other object.");
    }
    else if (contains_other)
    {
		throw std::exception("Player contains other object.");
	}
    else
    {
		throw std::exception("Invalid collision type.");
	}
}
void Player::update_weapon_position() const
{
    this->_primary->set_player_center(this->get_center());
}
void Player::on_projectile_collision(const ICollisionGameObject* other)
{
    collision_object_type other_type = other->get_collision_object_type();

    if (other_type == collision_object_type::PROJECTILE_SPRAY_TEAM_A ||
        other_type == collision_object_type::PROJECTILE_SPRAY_TEAM_B)
    {
		this->_health -= SPRAY_DAMAGE;
        //this->_health_regen_timer = 0.0f;
	}
	else if (other_type == collision_object_type::PROJECTILE_JET_TEAM_A ||
		other_type == collision_object_type::PROJECTILE_JET_TEAM_B)
	{
		this->_health -= JET_DAMAGE;
        //this->_health_regen_timer = 0.0f;
	}
	else if (other_type == collision_object_type::PROJECTILE_ROLLING_TEAM_A ||
		other_type == collision_object_type::PROJECTILE_ROLLING_TEAM_B)
	{
		this->_health -= ROLLING_DAMAGE;
        //this->_health_regen_timer = 0.0f;
	}
	else if (other_type == collision_object_type::PROJECTILE_BALL_TEAM_A ||
		other_type == collision_object_type::PROJECTILE_BALL_TEAM_B)
	{
		this->_health -= BALL_DAMAGE;
        //this->_health_regen_timer = 0.0f;
	}
	else if (other_type == collision_object_type::PROJECTILE_MIST_TEAM_A ||
		other_type == collision_object_type::PROJECTILE_MIST_TEAM_B)
	{
		this->_health -= MIST_DAMAGE;
        //this->_health_regen_timer = 0.0f;
	}
	else
	{
		throw std::exception("Invalid projectile type.");
    }
    this->_health_regen_timer = 0.0f;

    if (this->_damage_sound_timer >= DAMAGE_SOUND_DELAY)
	{
		this->_sound_bank->play_wave(DAMAGE_SOUND, DAMAGE_SOUND_VOLUME);
		this->_damage_sound_timer = 0.0f;
	}
}

float Player::get_dt() const
{
	return *this->_dt;
}
void Player::update()
{
    if (this->_state == player_state::ALIVE)
    {   
        this->update_movement();

        AnimationObject::update();

        if (this->_health <= 0.0f)
        {
			this->_state = player_state::DEAD;
			this->_sound_bank->play_wave(DEATH_SOUND, DEATH_SOUND_VOLUME);
        }

        this->_damage_sound_timer += this->get_dt();
    }
    else if (this->_state == player_state::DEAD)
    {
        this->_respawn_timer -= this->get_dt();

        if (this->_respawn_timer <= 0.0f)
        {
            this->respawn();
            this->_respawn_timer = RESPAWN_DELAY;
        }
    }
	else
	{
		throw std::exception("Invalid player state.");
	}
    if (this->_input.toggle_debug)
    {
        this->_showing_debug = !this->_showing_debug;
    }

    //ammo regen
    if (this->_health_regen_timer >= HEALTH_REGEN_DELAY)
    {
        this->_health += HEALTH_REGEN_RATE * this->get_dt();
        if (this->_health > 1.0f)
        {
            this->_health = 1.0f;
        }
    }
    this->_health_regen_timer += this->get_dt();
}
std::vector<std::unique_ptr<ICollisionGameObject>>
    Player::update_weapon_and_get_projectiles() const
{
    return this->_primary->update_and_get_projectiles(
        this->get_input(),
        this->get_center(),
        MovingObject::get_velocity(),
        this->get_facing_right());
}
void Player::update_prev_rectangle()
{
	this->_prev_rectangle = this->_rectangle;
}

void Player::update_movement()
{
    const PlayerInputData input = this->_input;
    const float x_input = input.x_movement;
    const player_move_state move_state = this->get_move_state();
    const float dt = this->get_dt();
    const bool analog_stick_down = this->_input.left_analog_stick.y >
        DROP_DOWN_ANALOG_THRESHOLD;
    const bool jump_pressed = this->_input.jump_pressed;

    //If the player's velocity is zero and the user is requesting to move the player
    if (are_equal(MovingObject::get_velocity_x(), 0.0f) && x_input != 0.0f)
    {
        MovingObject::set_velocity_x(X_INITIAL_VELOCITY * x_input);
    }
    //If accelerating in the same direction as currently moving. right
    else if (MovingObject::get_velocity_x() > 0.0f && x_input > 0.0f)
    {
        MovingObject::alter_velocity_x(X_ACCELERATION * dt * x_input);
        if (MovingObject::get_velocity_x() > MAX_VELOCITY.x)
        {
            MovingObject::set_velocity_x(MAX_VELOCITY.x);
        }
    }
    //If accelerating in the same direction as currently moving. left
    else if (MovingObject::get_velocity_x() < 0.0f && x_input < 0.0f)
    {
        MovingObject::alter_velocity_x(X_ACCELERATION * dt * x_input);
        if (MovingObject::get_velocity_x() < -MAX_VELOCITY.x)
        {
            MovingObject::set_velocity_x(-MAX_VELOCITY.x);
        }
    }
    //If accelerating in the opposite direction as currently moving. right
    else if (MovingObject::get_velocity_x() > 0.0f && x_input < 0.0f)
    {
        MovingObject::alter_velocity_x(X_DEACCELERATION * dt * x_input);
        MovingObject::alter_velocity_x(X_ACCELERATION * dt * x_input);
    }
    //If accelerating in the opposite direction as currently moving. left
    else if (MovingObject::get_velocity_x() < 0.0f && x_input > 0.0f)
    {
        MovingObject::alter_velocity_x(X_DEACCELERATION * dt * x_input);
        MovingObject::alter_velocity_x(X_ACCELERATION * dt * x_input);
    }
    //No key pressed deaccelaration right
    else if (MovingObject::get_velocity_x() > 0.0f && are_equal(x_input, 0.0f))
    {
        MovingObject::alter_velocity_x(-X_DEACCELERATION * dt);
        if (MovingObject::get_velocity_x() < 0.0f)
        {
            MovingObject::set_velocity_x(0.0f);
        }
    }
    //No key pressed deaccelaration left
    else if (MovingObject::get_velocity_x() < 0.0f && are_equal(x_input, 0.0f))
    {
        MovingObject::alter_velocity_x(X_DEACCELERATION * dt);
        if (MovingObject::get_velocity_x() > 0.0f)
        {
            MovingObject::set_velocity_x(0.0f);
        }
    }
    //gravity
    MovingObject::alter_velocity_y(GRAVITY * dt);
    if (MovingObject::get_velocity_y() > MAX_VELOCITY.y)
    {
        MovingObject::set_velocity_y(MAX_VELOCITY.y);
    }

    // DROPPING_DOWN state only lasts for one frame
    if (move_state == player_move_state::DROPPING_DOWN)
	{
		this->set_move_state(player_move_state::IN_AIR);
	}

    // check if dropping down from a platform
    if (move_state == player_move_state::ON_DROP_DOWN_GROUND && analog_stick_down &&
        !jump_pressed)
    {
        MovingObject::alter_velocity_y(DROP_DOWN_VELOCITY);
        this->set_move_state(player_move_state::DROPPING_DOWN);
    }
	else
	{
        this->do_jump();
	}

    MovingObject::set_dx_x(MovingObject::get_velocity_x() * dt);
    MovingObject::set_dx_y(MovingObject::get_velocity_y() * dt);

    this->_rectangle.offset(MovingObject::get_dx_x(),
        MovingObject::get_dx_y());

    if (x_input > 0.0f)
    {
        this->set_facing_right(true);
    }
    else if (x_input < 0.0f)
    {
        this->set_facing_right(false);
    }
}

void Player::do_jump()
{
    const player_move_state move_state = this->get_move_state();
    const bool jump_pressed = this->_input.jump_pressed;
    const bool jump_held = this->_input.jump_held;
    const float dt = this->get_dt();
   
    if ((move_state == player_move_state::ON_GROUND ||
        move_state == player_move_state::ON_DROP_DOWN_GROUND) &&
        jump_pressed)
    {
		MovingObject::set_velocity_y(JUMP_LAUNCH_VELOCITY);
		this->set_air_time(0.0f);
		this->set_move_state(player_move_state::JUMPING);
		this->_sound_bank->play_wave(JUMP_SOUND, JUMP_SOUND_VOLUME);
	}
	else if (move_state == player_move_state::ON_CEILING)
	{
		// do nothing
	}
	else if (move_state == player_move_state::JUMPING)
	{
		if (jump_held)
		{
			if (this->get_air_time() <= JUMP_MAX_TIME)
			{
				MovingObject::alter_velocity_y(JUMP_ACCELERATION * dt);
			}
			else
			{
				this->set_move_state(player_move_state::IN_AIR);
			}
		}
		else // !jump held
		{
			this->set_move_state(player_move_state::IN_AIR);
		}
		this->alter_air_time(dt);
	}
	else if (move_state == player_move_state::IN_AIR)
	{
		// do nothing
	}

    if (this->get_velocity_y() < -MAX_VELOCITY.y)
    {
        this->set_velocity_y(-MAX_VELOCITY.y);
    }
}

bool Player::is_visible_in_viewport(const RectangleF& view) const
{
    RectangleF player_and_weapon_rect = this->_rectangle;
    player_and_weapon_rect.inflate(Vector2F(200.0f, 200.0f));
    return player_and_weapon_rect.intersects(view);
}
void Player::set_player_input(const PlayerInputData& input)
{
	this->_input = input;
}

const RectangleF& Player::get_rectangle() const
{
	return this->_rectangle;
}
Vector2F Player::get_center() const
{
	return this->_rectangle.get_center();
}
const Vector2F& Player::get_velocity() const
{
    return MovingObject::get_velocity();
}
const Camera& Player::get_camera() const
{
	return this->_camera;
}
const Viewport& Player::get_viewport() const
{
	return this->_viewport;
}
float Player::get_input_x_movement() const
{
	return this->_input.x_movement;
}
const Vector2F& Player::get_input_shoot_direction() const
{
	return this->_input.shoot_direction;
}
bool Player::get_input_primary_shoot() const
{
	return this->_input.primary_shoot;
}
bool Player::get_input_jump_pressed() const
{
	return this->_input.jump_pressed;
}
bool Player::get_input_jump_held() const
{
	return this->_input.jump_held;
}
player_move_state Player::get_move_state() const
{
	return this->_move_state;
}
bool Player::get_showing_debug() const
{
	return this->_showing_debug;
}
const PlayerAnimationInfo& Player::get_animation_info(player_animation_state state)
{
    switch (state)
    {
    case player_animation_state::IDLE:
        return ANIMATION_INFO_IDLE;
    case player_animation_state::WALKING:
        return ANIMATION_INFO_WALK;
    case player_animation_state::RUNNING:
        return ANIMATION_INFO_RUNNING;
    case player_animation_state::JUMPING:
        return ANIMATION_INFO_JUMPING;
    case player_animation_state::FALLING:
        return ANIMATION_INFO_FALLING;
    default:
        throw std::exception("Invalid player animation state.");
    }
}
void Player::respawn()
{
    this->_rectangle.set_position(this->_respawn_position);
    MovingObject::set_rotation(0.0f);
    MovingObject::set_velocity(Vector2F::ZERO);
    MovingObject::set_dx(Vector2F::ZERO);
	this->_health = 1.0f;
	this->_primary->reset_ammo();
	this->_state = player_state::ALIVE;
    this->_sound_bank->play_wave(RESPAWN_SOUND, RESPAWN_SOUND_VOLUME);
}
player_animation_state Player::calculate_animation_state() const
{
    player_move_state move_state = this->get_move_state();
    if (move_state == player_move_state::ON_GROUND ||
        move_state == player_move_state::ON_DROP_DOWN_GROUND)
    {
        float velocity_x = this->get_velocity_x();
        if (velocity_x == 0.0f)
        {
			return player_animation_state::IDLE;
		}
        else if (std::fabs(velocity_x) > 600.0f)
        {
			return player_animation_state::RUNNING;
		}
        else
        {
			return player_animation_state::WALKING;
		}
	}
    else if (move_state == player_move_state::ON_CEILING)
    {
		return player_animation_state::IDLE;
	}
    else if (move_state == player_move_state::JUMPING)
    {
		return player_animation_state::JUMPING;
	}
    else if (move_state == player_move_state::IN_AIR ||
		move_state == player_move_state::DROPPING_DOWN)
    {
        if (this->get_velocity_y() > 0.0f)
        {
			return player_animation_state::FALLING;
		}
        else
        {
			return player_animation_state::JUMPING;
		}
	}
    else
    {
		throw std::exception("Invalid player move state.");
	}
}
void Player::stop_sounds() const
{
    this->_primary->stop_sounds();
}
