#include "pch.h"
#include "Player.h"

using namespace DirectX;
using namespace MattMath;
using namespace player_consts;
using namespace colour_consts;

Player::Player(const RectangleF& rectangle,
    const player_animation_info& animation_info,
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
    AnimationObject(dt, animation_info.sprite_sheet,
        animation_info.animation, sprite_batch,
        resource_manager, DEFAULT_PLAYER_COLOUR, rotation, origin, effects, layer_depth),
    TextureObject(animation_info.sprite_sheet, animation_info.uniform_texture, sprite_batch,
        		resource_manager, team_colour, rotation, origin, effects, layer_depth),
    MovingObject(velocity),
    _player_num(player_num),
    _team(team),
    _primary_type(primary),
    _primary(std::move(primary_weapon)),
    _secondary_type(secondary),
    _team_colour(team_colour),
    _viewport(view_port),
    _dt(dt),
    _rectangle(rectangle),
    _respawn_position(respawn_position)
{
    this->_sound_bank = resource_manager->get_sound_bank(SOUND_BANK);

    //// create countdown text

    //RectangleF viewport = this->_viewport.get_rectangle();
    //Vector2F position = viewport.get_size() / 2.0f;
    ////position = position - Vector2F(COUNTDOWN_TEXT_WIDTH / 2.0f,
    ////    COUNTDOWN_TEXT_HEIGHT / 2.0f);

    //this->_countdown_text = std::make_unique<TextDropShadow>(
    //    COUNTDOWN_TEXT,
    //    COUNTDOWN_FONT_NAME,
    //    position,
    //    sprite_batch,
    //    resource_manager,
    //    COUNTDOWN_COLOUR,
    //    COUNTDOWN_SHADOW_COLOUR,
    //    COUNTDOWN_SHADOW_OFFSET,
    //    COUNTDOWN_SCALE,
    //    COUNTDOWN_SCALE);
}

void Player::draw(const Camera& camera)
{
    if (this->_state == player_state::DEAD)
    {
        //// draw countdown text

        //this->_countdown_text->draw(camera);
        
        return;
    }
    
    player_animation_state current_animation_state = this->_animation_state;
    player_animation_state new_animation_state = this->calculate_animation_state();

    if (current_animation_state != new_animation_state)
    {
		this->_animation_state = new_animation_state;

        const player_animation_info& info = this->get_animation_info(new_animation_state);
        TextureObject::set_sprite_sheet_name(info.sprite_sheet);
        TextureObject::set_element_name(info.uniform_texture);

        AnimationObject::set_animation_strip_and_reset(info.sprite_sheet, info.animation);
        if (info.frame_time != FLT_MIN)
        {
            AnimationObject::set_frame_time(info.frame_time);
		}
        else
        {
			AnimationObject::set_frame_time_to_default();
        }
	}

    SpriteEffects effects = SpriteEffects::SpriteEffects_None;
    bool facing_right = this->get_facing_right();
    if (!facing_right)
    {
        effects = SpriteEffects::SpriteEffects_FlipHorizontally;
    }

    TextureObject::set_effects(effects);
    TextureObject::draw(this->_rectangle, camera);

    AnimationObject::set_effects(effects);
    AnimationObject::draw(this->_rectangle, camera);

    this->_primary->draw(camera);
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
    const ICollisionGameObject* other) const
{
    collision_object_type other_type = other->get_collision_object_type();

    bool structure_collision =
        other_type == collision_object_type::STRUCTURE_WALL ||
        other_type == collision_object_type::STRUCTURE_WALL_PAINTABLE ||
        other_type == collision_object_type::STRUCTURE_FLOOR ||
        other_type == collision_object_type::STRUCTURE_FLOOR_PAINTABLE;

    //   player_team team = this->_team;
    //   bool projectile_collision = false;

    //   if (team == player_team::A)
    //   {
    //       bool projectile_team_b =
    //           other_type == collision_object_type::PROJECTILE_SPRAY_TEAM_B ||
    //           other_type == collision_object_type::PROJECTILE_JET_TEAM_B ||
    //           other_type == collision_object_type::PROJECTILE_ROLLING_TEAM_B ||
    //           other_type == collision_object_type::PROJECTILE_BALL_TEAM_B ||
    //           other_type == collision_object_type::PROJECTILE_MIST_TEAM_B;

    //       projectile_collision = projectile_team_b;
    //   }
       //else if (team == player_team::B)
       //{
       //	bool projectile_team_a =
       //		other_type == collision_object_type::PROJECTILE_SPRAY_TEAM_A ||
       //		other_type == collision_object_type::PROJECTILE_JET_TEAM_A ||
       //		other_type == collision_object_type::PROJECTILE_ROLLING_TEAM_A ||
       //		other_type == collision_object_type::PROJECTILE_BALL_TEAM_A ||
       //		other_type == collision_object_type::PROJECTILE_MIST_TEAM_A;

    //       projectile_collision = projectile_team_a;
    //}

    return structure_collision; //|| projectile_collision;
}
bool Player::is_colliding(const ICollisionGameObject* other) const
{
    // dead check
    if (this->_state == player_state::DEAD)
	{
		return false;
	}
    
    // type check
    if (!this->is_matching_collision_object_type(other))
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

    bool wall_collision =
		other_type == collision_object_type::STRUCTURE_WALL ||
		other_type == collision_object_type::STRUCTURE_WALL_PAINTABLE;

    bool floor_collision = 
        other_type == collision_object_type::STRUCTURE_FLOOR ||
		other_type == collision_object_type::STRUCTURE_FLOOR_PAINTABLE;

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

	if (wall_collision)
	{
		this->on_wall_collision(other);
	}
	else if (floor_collision)
	{
		this->on_floor_collision(other);
	}
    else if (projectile_collision)
	{
		this->on_projectile_collision(other);
	}
	else
	{
		throw std::exception("Invalid collision object type.");
	}
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
void Player::on_wall_collision(const ICollisionGameObject* other)
{
    direction dir = this->get_velocity().get_direction();
    bool left = dir == direction::LEFT || dir == direction::UP_LEFT ||
		dir == direction::DOWN_LEFT;
    bool right = dir == direction::RIGHT || dir == direction::UP_RIGHT ||
	    dir == direction::DOWN_RIGHT;

    const RectangleF* other_rect =
        static_cast<const RectangleF*>(other->get_shape());

    this->set_velocity_x(0.0f);

    if (left)
	{
		this->_rectangle.set_position_x(other_rect->get_right());
	}
	else if (right)
	{
        this->_rectangle.set_position_x_from_right(other_rect->get_left());
	}
	else
	{
		//throw std::exception("Invalid direction value.");
	}
}
void Player::on_floor_collision(const ICollisionGameObject* other)
{
    direction dir = this->get_velocity().get_direction();
    bool up = dir == direction::UP || dir == direction::UP_LEFT ||
	    dir == direction::UP_RIGHT;
    bool down = dir == direction::DOWN || dir == direction::DOWN_LEFT ||
	    dir == direction::DOWN_RIGHT;

    const RectangleF* other_rect =
        static_cast<const RectangleF*>(other->get_shape());

    MovingObject::set_velocity_y(0.0f);

    if (up)
    {
        this->_rectangle.set_position_y(other_rect->get_bottom());
        this->set_move_state(player_move_state::ON_CEILING);
	}
	else if (down)
	{
        this->_rectangle.set_position_y_from_bottom(other_rect->get_top());
        this->set_move_state(player_move_state::ON_GROUND);
	}
	else
	{
		throw std::exception("Invalid direction value.");
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
        //else if (this->_health - this->_prev_health < )
        //{
        //    this->_sound_bank->play_wave(DAMAGE_SOUND, DAMAGE_SOUND_VOLUME);
        //}
    }
    else if (this->_state == player_state::DEAD)
    {
        this->_respawn_timer -= this->get_dt();
        //this->_countdown_text->set_text(std::to_string(
        //    3 - static_cast<int>(this->_respawn_timer)));
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

    //this->_prev_health = this->_health;
}
std::vector<std::unique_ptr<ICollisionGameObject>>
    Player::update_weapon_and_get_projectiles()
{
    return this->_primary->update_and_get_projectiles(
        this->get_input(),
        this->get_center(),
        MovingObject::get_velocity(),
        this->get_facing_right());
}

void Player::update_movement()
{
    const player_input input = this->_input;
    const float x_input = input.x_movement;
    const float dt = this->get_dt();
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

    //// for testing only. NOT FOR NORMAL PLAY
    //player->set_vel_y(0.0f);


    this->do_jump();

    MovingObject::set_dx_x(MovingObject::get_velocity_x() * dt);
    MovingObject::set_dx_y(MovingObject::get_velocity_y() * dt);

    //this->alter_position_y(MovingObject::get_dx_x());
    //this->alter_position_y(MovingObject::get_dx_y());

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
    player_move_state move_state = this->get_move_state();
    const bool jump_pressed = this->_input.jump_pressed;
    const bool jump_held = this->_input.jump_held;
    //const bool prev_req_jump = this->_input.prev_requesting_jump;
    const float dt = this->get_dt();

    switch (move_state)
    {
    case player_move_state::ON_GROUND:
        if (jump_pressed) //&& !prev_req_jump)
		{
			//this->set_velocity_y(player_consts::JUMP_LAUNCH_VELOCITY *
			//	(1 - pow(this->get_air_time() /
			//		player_consts::JUMP_MAX_TIME, player_consts::JUMP_POWER)));

            MovingObject::set_velocity_y(JUMP_LAUNCH_VELOCITY);
            this->set_air_time(0.0f);
            this->set_move_state(player_move_state::JUMPING);
            this->_sound_bank->play_wave(JUMP_SOUND, JUMP_SOUND_VOLUME);
		}
        break;
    case player_move_state::ON_CEILING:
        break;
    case player_move_state::JUMPING:
        if (jump_held)
		{
			if (this->get_air_time() <= JUMP_MAX_TIME)
			{
				//this->set_velocity_y(player_consts::JUMP_LAUNCH_VELOCITY *
				//	(1 - pow(this->get_air_time() /
				//		player_consts::JUMP_MAX_TIME, player_consts::JUMP_POWER)));

                MovingObject::alter_velocity_y(JUMP_ACCELERATION * dt);


				//this->set_jumping(true);
				//this->set_on_ground(false);
				//this->set_move_state(player_move_state::JUMPING);
			}
			else
			{
				//this->set_jumping(false);
				this->set_move_state(player_move_state::IN_AIR);
			}
		}
		else // !jump held
		{
			//this->set_velocity_y(0.0f);
			//this->set_jumping(false);
			//this->set_air_time(player_consts::JUMP_MAX_TIME);
			this->set_move_state(player_move_state::IN_AIR);
		}
        this->alter_air_time(dt);
        break;
    case player_move_state::IN_AIR:
        break;
    default:
        break;
    }

    if (this->get_velocity_y() < -MAX_VELOCITY.y)
    {
        this->set_velocity_y(-MAX_VELOCITY.y);
    }

    //const bool req_jump = this->get_input_requesting_jump();
    //const float dt = this->get_dt();
    //    
    //if (this->get_on_ground())
    //{
    //    this->set_air_time(0.0f);
    //    this->set_jumping(false);
    //}
    //else if (this->get_on_ceiling())
    //{
    //    this->set_jumping(false);
    //}
    //else
    //{
    //    this->alter_air_time(dt);
    //}
    //if (req_jump && !this->get_jumping() &&
    //    !this->get_prev_requesting_jump() && this->get_on_ground())
    //{
    //    this->set_velocity_y(player_consts::JUMP_LAUNCH_VELOCITY *
    //        (1 - pow(this->get_air_time() /
    //            player_consts::JUMP_MAX_TIME, player_consts::JUMP_POWER)));
    //    this->set_jumping(true);
    //    this->set_on_ground(false);
    //}
    //else if (req_jump && this->get_jumping())
    //{
    //    if (this->get_air_time() <= player_consts::JUMP_MAX_TIME)
    //    {
    //        this->set_velocity_y(player_consts::JUMP_LAUNCH_VELOCITY *
    //            (1 - pow(this->get_air_time() /
    //                player_consts::JUMP_MAX_TIME, player_consts::JUMP_POWER)));
    //        this->set_jumping(true);
    //        this->set_on_ground(false);
    //    }
    //    else
    //    {
    //        this->set_jumping(false);
    //    }
    //}
    //else if (!req_jump && this->get_jumping())
    //{
    //    this->set_velocity_y(0.0f);
    //    this->set_jumping(false);
    //    this->set_air_time(player_consts::JUMP_MAX_TIME);
    //}
    //if (this->get_velocity_y() < -player_consts::MAX_VELOCITY.y)
    //{
    //    this->set_velocity_y(-player_consts::MAX_VELOCITY.y);
    //}
    //this->set_prev_requesting_jump(req_jump);
}

bool Player::is_visible_in_viewport(const RectangleF& view) const
{
    RectangleF player_and_weapon_rect = this->_rectangle;
    player_and_weapon_rect.inflate(Vector2F(200.0f, 200.0f));
    return player_and_weapon_rect.intersects(view);
}
void Player::set_player_input(const player_input& input)
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
//direction_lock Player::get_input_shoot_direction_lock() const
//{
//	return this->_input.shoot_direction_lock;
//}
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
//bool Player::get_prev_requesting_jump() const
//{
//	return this->_input.prev_requesting_jump;
//}
player_move_state Player::get_move_state() const
{
	return this->_move_state;
}
bool Player::get_showing_debug() const
{
	return this->_showing_debug;
}
const player_animation_info& Player::get_animation_info(player_animation_state state) const
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
    if (move_state == player_move_state::ON_GROUND)
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
    else if (move_state == player_move_state::IN_AIR)
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



//Player::Player(const player_info& info) :
//    _info(info)
//{
//}

//Rectangle Player::get_rectangle() const
//{
//    Rectangle result(this->_info.position.x,
//        this->_info.position.y,
//        this->_info.size.x,
//        this->_info.size.y);
//    return result;
//}

//Vector2 Player::get_center() const
//{
//    Rectangle rect = get_rectangle();
//    float x = ((2.0f * rect.x) + rect.width) / 2.0f;
//    float y = ((2.0f * rect.y) + rect.height) / 2.0f;
//    return Vector2(x, y);
//}

//const RectangleF& Player::get_bounding_box() const
//{
//	return this->get_bounding_box();
//}
//const RectangleI Player::get_bounding_box_i() const
//{
//	return this->get_bounding_box_i();
//}
//const RectangleF& Player::get_collision_aabb() const
//{
//	return this->get_collision_aabb();
//}
//const std::vector<collidable_object_type>&
//	Player::get_collidable_object_types() const
//{
//	return this->get_collidable_object_types();
//}
//const std::vector<collidable_object_detail>&
//	Player::get_collidable_object_details() const
//{
//	return this->get_collidable_object_details();
//}
//const Vector2F& Player::get_velocity() const
//{
//	return this->get_velocity();
//}
//const Vector2F& Player::get_dx() const
//{
//	return this->get_dx();
//}
//float Player::get_rotation() const
//{
//	return this->get_rotation();
//}
