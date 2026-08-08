#include "game/pch.h"
#include "game/objects/player.h"

using namespace DirectX;
using namespace mattmath;
using namespace player_consts;
using namespace colour_consts;
using namespace artattack;

Player::Player(const RectangleF& rectangle,
    const PlayerAnimationInfo& animation_info,
    RenderResources* render_resources,
    const AudioResources* audio_resources,
    int player_num,
    PlayerTeam team,
    WeaponType primary,
    std::unique_ptr<Weapon> primary_weapon,
    WeaponType secondary,
    const Colour& team_colour,
    const Viewport& view_port,
    const mattmath::Vector2F& respawn_position,
    const Vector2F& velocity,
    float rotation,
    const Vector2F& origin,
    SpriteFlip flip,
    float layer_depth) :
    MovingObject(velocity),
    AnimationObject(animation_info.sprite_sheet,
                    animation_info.animation,
                    render_resources, DEFAULT_PLAYER_COLOUR, rotation, origin, flip, layer_depth),
    TextureObject(animation_info.sprite_sheet, animation_info.uniform_texture,
                  render_resources, team_colour, rotation, origin, flip, layer_depth),
    primary_(std::move(primary_weapon)),
    player_num_(player_num),
    team_(team),
    primary_type_(primary),
    secondary_type_(secondary),
    team_colour_(team_colour),
    viewport_(view_port),
    rectangle_(rectangle),
    prev_rectangle_(rectangle),
    respawn_position_(respawn_position)
{
    this->sound_bank_ = audio_resources->sound_bank(SOUND_BANK);

    this->damage_sound_ = this->sound_bank_->resolve_wave(DAMAGE_SOUND);
    this->death_sound_ = this->sound_bank_->resolve_wave(DEATH_SOUND);
    this->jump_sound_ = this->sound_bank_->resolve_wave(JUMP_SOUND);
    this->respawn_sound_ = this->sound_bank_->resolve_wave(RESPAWN_SOUND);
}

void Player::update_animation_state()
{
    const PlayerAnimationState new_animation_state =
        this->calculate_animation_state();

    if (this->animation_state_ == new_animation_state)
    {
        return;
    }

    this->animation_state_ = new_animation_state;

    const PlayerAnimationInfo& info = this->animation_info(new_animation_state);
    TextureObject::set_frame(info.sprite_sheet, info.uniform_texture);

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

void Player::draw(DrawList& draw_list) const
{
    if (this->state_ == PlayerState::dead)
    {
        return;
    }

    // Pure read: nothing below assigns a member. update_animation_state() has
    // already chosen the clip, and the flip is passed as a parameter.
    const SpriteFlip flip = this->facing_right()
        ? SpriteFlip::none
        : SpriteFlip::horizontal;

    // Every base call is explicitly qualified: Player inherits DrawObject
    // twice, once through TextureObject and once through AnimationObject, and
    // the two subobjects carry independent colour/origin/rotation.
    TextureObject::draw_with(draw_list, this->rectangle_,
        this->TextureObject::frame(),
        this->TextureObject::colour(),
        this->TextureObject::origin(), flip,
        this->TextureObject::draw_rotation());

    AnimationObject::draw_with(draw_list, this->rectangle_,
        this->AnimationObject::colour(), flip);

    this->primary_->draw(draw_list, this->showing_debug_);
}
CollisionObjectType Player::collision_object_type() const
{
    if (this->state_ == PlayerState::alive)
    {
        switch (this->team_)
        {
        case PlayerTeam::a:
            return CollisionObjectType::player_team_a;
        case PlayerTeam::b:
            return CollisionObjectType::player_team_b;
        default:
            throw std::exception("Invalid player team");
        };
    }
    else if (this->state_ == PlayerState::dead)
	{
		switch (this->team_)
		{
		case PlayerTeam::a:
			return CollisionObjectType::player_team_a_dead;
		case PlayerTeam::b:
			return CollisionObjectType::player_team_b_dead;
		default:
			throw std::exception("Invalid player team");
		};
	}
	else
	{
		throw std::exception("Invalid player state");
	}

}
const Shape* Player::shape() const
{
	return this->collision_rectangle();
}
bool Player::for_deletion() const
{
	return false;
}

const RectangleF* Player::collision_rectangle() const
{
	return &this->rectangle_;
}

bool Player::is_matching_collision_object_type(
    const CollisionObject* other)
{
    CollisionObjectType other_type = other->collision_object_type();
    return is_structure(other_type);
}
bool Player::is_colliding(const CollisionObject* other) const
{
    // dead check
    if (this->state_ == PlayerState::dead)
	{
		return false;
	}
    
    // type check
    if (!is_matching_collision_object_type(other))
    {
        return false;
    }

    // aabb check
    if (!this->shape()->AABB_intersects(other->shape()))
    {
        return false;
    }
    else // AABBs are intersecting
    {
        // if the other object is a rectangle, then we have a collision
        // since the AABB check passed
        ShapeType other_shape_type = other->shape()->shape_type();
        if (other_shape_type == ShapeType::rectangle)
        {
            return true;
        }
    }

    // narrow phase check
    if (this->shape()->intersects(other->shape()))
    {
        return true;
    }

    return false;
}
void Player::on_collision(const CollisionObject* other)
{
    CollisionObjectType other_type = other->collision_object_type();

    bool structure_collision =
        other_type == CollisionObjectType::structure ||
        other_type == CollisionObjectType::structure_paintable;

    bool structure_jump_through_collision =
		other_type == CollisionObjectType::structure_jump_through;

	bool structure_ramp_collision = is_structure_ramp(other_type);

    PlayerTeam team = this->team_;
    bool projectile_collision = false;
    if (team == PlayerTeam::a)
    {
            bool projectile_team_b =
            other_type == CollisionObjectType::projectile_spray_team_b ||
            other_type == CollisionObjectType::projectile_jet_team_b ||
            other_type == CollisionObjectType::projectile_rolling_team_b ||
            other_type == CollisionObjectType::projectile_ball_team_b ||
            other_type == CollisionObjectType::projectile_mist_team_b;

        projectile_collision = projectile_team_b;
    }
    else if (team == PlayerTeam::b)
    {
        bool projectile_team_a =
            other_type == CollisionObjectType::projectile_spray_team_a ||
            other_type == CollisionObjectType::projectile_jet_team_a ||
            other_type == CollisionObjectType::projectile_rolling_team_a ||
            other_type == CollisionObjectType::projectile_ball_team_a ||
            other_type == CollisionObjectType::projectile_mist_team_a;

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
	else if (structure_ramp_collision)
	{
		this->on_structure_ramp_collision(other);
	}
	else
	{
		throw std::exception("Invalid collision object type.");
	}
}
void Player::on_structure_ramp_collision(const CollisionObject* other)
{
    Vector2F direction = CollisionTools::calculate_object_collision_direction(
        this->shape(), other->shape());

	direction = Vector2F::direction_to_8_cardinal_direction(direction);

	CollisionObjectType other_type = other->collision_object_type();

	bool bottom_edge_collision = direction == Vector2F::DIRECTION_UP ||
		direction == Vector2F::DIRECTION_UP_LEFT ||
		direction == Vector2F::DIRECTION_UP_RIGHT;

	bool side_wall_collision = (direction == Vector2F::DIRECTION_RIGHT &&
        other_type == CollisionObjectType::structure_ramp_left) 
		|| (direction == Vector2F::DIRECTION_LEFT &&
			other_type == CollisionObjectType::structure_ramp_right);

	bool perform_structure_collision = bottom_edge_collision || side_wall_collision;

	if (perform_structure_collision)
	{
		this->on_structure_collision(other);
		return;
    }
    
	if (other_type == CollisionObjectType::structure_ramp_left)
	{
        // Mirror of the RAMP_RIGHT case below. This was an empty `// TODO`, so
        // left ramps had no collision response at all - players fell straight
        // through them. side_wall_collision above already routes a DIRECTION_RIGHT
        // hit to the flat-structure path, which is a left ramp's wall side, so
        // the remaining directions are all ramp-surface contacts.
        if (direction == Vector2F::DIRECTION_DOWN ||
            direction == Vector2F::DIRECTION_LEFT ||
            direction == Vector2F::DIRECTION_DOWN_LEFT ||
            direction == Vector2F::DIRECTION_DOWN_RIGHT)
        {
            CollisionTools::resolve_object_collision(&this->rectangle_,
                other->shape(), Vector2F::DIRECTION_DOWN);

            this->set_move_state(PlayerMoveState::on_ramp_left);

            this->set_velocity_y(0.0f);
        }
        else if (direction == Vector2F::ZERO)
        {
            throw std::runtime_error("No collision direction.");
        }
        else
        {
            throw std::runtime_error("Invalid collision direction.");
        }
	}
	else if (other_type == CollisionObjectType::structure_ramp_right)
	{
        if (direction == Vector2F::DIRECTION_DOWN)
        {
            CollisionTools::resolve_object_collision(&this->rectangle_,
                other->shape(), Vector2F::DIRECTION_DOWN);

            this->set_move_state(PlayerMoveState::on_ramp_right);

            this->set_velocity_y(0.0f);
        }
        else if (direction == Vector2F::DIRECTION_RIGHT)
        {
            CollisionTools::resolve_object_collision(&this->rectangle_,
                other->shape(), Vector2F::DIRECTION_DOWN);

            this->set_move_state(PlayerMoveState::on_ramp_right);

            this->set_velocity_y(0.0f);
        }
        else if (direction == Vector2F::DIRECTION_DOWN_LEFT)
        {
            CollisionTools::resolve_object_collision(&this->rectangle_,
                other->shape(), Vector2F::DIRECTION_DOWN);

            this->set_move_state(PlayerMoveState::on_ramp_right);

            this->set_velocity_y(0.0f);
            
            /*PlayerMoveState move_state = this->move_state();

			RectangleF ramp_rect = other->shape()->bounding_box();

			Point2F player_center = this->center();
            Point2F ramp_top = ramp_rect.top();

            mattmath::Direction dir = this->velocity().direction();
            bool moving_up = dir == Direction::up || dir == Direction::up_left ||
                dir == Direction::up_right;

            if (move_state == PlayerMoveState::on_ramp_right ||
				(player_center.y < ramp_top.y && !moving_up))

            {
                CollisionTools::resolve_object_collision(&this->rectangle_,
                    other->shape(), Vector2F::DIRECTION_DOWN);

                this->set_move_state(PlayerMoveState::on_ramp_right);

                this->set_velocity_y(0.0f);
            }
            else
            {
                CollisionTools::resolve_object_collision(&this->rectangle_,
                    other->shape(), Vector2F::DIRECTION_LEFT);

				this->set_velocity_x(0.0f);
            }*/
        }
        else if (direction == Vector2F::DIRECTION_DOWN_RIGHT)
        {
            CollisionTools::resolve_object_collision(&this->rectangle_,
                other->shape(), Vector2F::DIRECTION_DOWN);

            this->set_move_state(PlayerMoveState::on_ramp_right);

            this->set_velocity_y(0.0f);
        }
        else if (direction == Vector2F::ZERO)
        {
            throw std::exception("No collision direction.");
        }
		else
        {
            throw std::exception("Invalid collision direction.");
        }
	}
	else
	{
		throw std::exception("Invalid ramp type.");
	}   
}
void Player::on_structure_jump_through_collision(const CollisionObject* other)
{
    // only collide if player is moving down and on previous cycle
    // was above the structure
    
    Vector2F velocity = this->velocity();
    Direction dir = velocity.direction();

    bool moving_down = dir == Direction::down || dir == Direction::down_left ||
        dir == Direction::down_right;

    const RectangleF& other_rect = other->shape()->bounding_box();
    bool was_above = this->prev_rectangle_.bottom() <= other_rect.top();

    PlayerMoveState move_state = this->move_state();

    if (moving_down && was_above && move_state != PlayerMoveState::dropping_down)
	{
        MovingObject::set_velocity_y(0.0f);

        const RectangleF& rect = other->shape()->bounding_box();
        this->rectangle_.set_position_y_from_bottom(rect.top());
        this->set_move_state(PlayerMoveState::on_drop_down_ground);
	}
	else
	{
		// no collision
	}
}
void Player::on_structure_collision(const CollisionObject* other)
{
	//Vector2F col_direction = CollisionTools::calculate_object_collision_direction(
	//	this->shape(), other->shape());

	//if (col_direction == Vector2F::ZERO)
 //   {
	//	throw std::exception("Player and structure are not colliding.");
	//}
 //   
	//Vector2F cardinal_col_direction = Vector2F::direction_to_8_cardinal_direction(col_direction);

 //   Vector2F player_direction = Vector2F::unit_vector(this->dx());

 //   
	//// check for wall collision
	//if (cardinal_col_direction == Vector2F::DIRECTION_LEFT ||
	//	cardinal_col_direction == Vector2F::DIRECTION_RIGHT)
	//{
 //       CollisionTools::resolve_object_collision(&this->rectangle_,
 //           other->shape(), Vector2F(col_direction.x, 0.0f));

 ////           CollisionTools::resolve_object_collision(&this->rectangle_,
 ////               other->shape(), Vector2F(0.0f, col_direction.y));
	////	}
	////	else if (player_direction.y < 0.0f)
	////	{
	////		this->on_ceiling_collision();

	// check if the player is not moving in the same direction as the collision


    Vector2F direction = CollisionTools::calculate_object_collision_direction_by_edge(
    	this->shape(), other->shape());

    if (direction == Vector2F::DIRECTION_UP)
    {
		this->on_top_collision(other);
	}
	else if (direction == Vector2F::DIRECTION_DOWN)
	{
		this->on_bottom_collision(other);
    }
	else if (direction == Vector2F::DIRECTION_LEFT)
	{
		this->on_left_collision(other);
    }
	else if (direction == Vector2F::DIRECTION_RIGHT)
	{
		this->on_right_collision(other);
	}
	else if (direction == Vector2F::DIRECTION_UP_LEFT)
	{
		this->on_top_left_collision(other);
	}
	else if (direction == Vector2F::DIRECTION_UP_RIGHT)
	{
		this->on_top_right_collision(other);
	}
	else if (direction == Vector2F::DIRECTION_DOWN_LEFT)
	{
		this->on_bottom_left_collision(other);
	}
	else if (direction == Vector2F::DIRECTION_DOWN_RIGHT)
	{
		this->on_bottom_right_collision(other);
	}
	else
	{
		throw std::exception("Invalid collision direction.");   
	}
}

void Player::on_top_collision(const CollisionObject* other)
{
    MovingObject::set_velocity_y(0.0f);

    CollisionTools::resolve_object_collision(&this->rectangle_,
        other->shape(), Vector2F::DIRECTION_UP);

	this->set_move_state(PlayerMoveState::on_ceiling);
}
void Player::on_bottom_collision(const CollisionObject* other)
{
    MovingObject::set_velocity_y(0.0f);

    CollisionTools::resolve_object_collision(&this->rectangle_,
        other->shape(), Vector2F::DIRECTION_DOWN);

    this->set_move_state(PlayerMoveState::on_ground);
}
void Player::on_left_collision(const CollisionObject* other)
{
    this->set_velocity_x(0.0f);

    CollisionTools::resolve_object_collision(&this->rectangle_,
        other->shape(), Vector2F::DIRECTION_LEFT);
}
void Player::on_right_collision(const CollisionObject* other)
{
    this->set_velocity_x(0.0f);

    CollisionTools::resolve_object_collision(&this->rectangle_,
        other->shape(), Vector2F::DIRECTION_RIGHT);
}
void Player::on_top_left_collision(const CollisionObject* other)
{
    Vector2F amount = CollisionTools::calculate_object_collision_depth(
        this->shape(), other->shape(), Vector2F::DIRECTION_UP_LEFT);

    if (amount.abs_x_greater_than_y())
    {
        this->on_top_collision(other);
    }
    else
    {
        this->on_left_collision(other);
    }
}
void Player::on_top_right_collision(const CollisionObject* other)
{
	Vector2F amount = CollisionTools::calculate_object_collision_depth(
		this->shape(), other->shape(), Vector2F::DIRECTION_UP_RIGHT);

    if (amount.abs_x_greater_than_y())
    {
        this->on_top_collision(other);
    }
    else
    {
        this->on_right_collision(other);
    }
}
void Player::on_bottom_left_collision(const CollisionObject* other)
{
    Direction dir = this->velocity().direction();
    bool moving_up = dir == Direction::up || dir == Direction::up_left ||
		dir == Direction::up_right;

	Vector2F amount = CollisionTools::calculate_object_collision_depth(
		this->shape(), other->shape(), Vector2F::DIRECTION_DOWN_LEFT);

    if (amount.abs_x_greater_than_y() && !moving_up)
    {
        this->on_bottom_collision(other);
    }
    else
    {
        this->on_left_collision(other);
    }
}
void Player::on_bottom_right_collision(const CollisionObject* other)
{
    Direction dir = this->velocity().direction();
    bool moving_up = dir == Direction::up || dir == Direction::up_left ||
        dir == Direction::up_right;

	Vector2F amount = CollisionTools::calculate_object_collision_depth(
		this->shape(), other->shape(), Vector2F::DIRECTION_DOWN_RIGHT);

    if (amount.abs_x_greater_than_y() && !moving_up)
    {
        this->on_bottom_collision(other);
    }
    else
    {
        this->on_right_collision(other);
    }
}

void Player::update_weapon_position() const
{
    this->primary_->set_player_center(this->center());
}
void Player::on_projectile_collision(const CollisionObject* other)
{
    CollisionObjectType other_type = other->collision_object_type();

    if (other_type == CollisionObjectType::projectile_spray_team_a ||
        other_type == CollisionObjectType::projectile_spray_team_b)
    {
		this->health_ -= SPRAY_DAMAGE;
        //this->health_regen_timer_ = 0.0f;
	}
	else if (other_type == CollisionObjectType::projectile_jet_team_a ||
		other_type == CollisionObjectType::projectile_jet_team_b)
	{
		this->health_ -= JET_DAMAGE;
        //this->health_regen_timer_ = 0.0f;
	}
	else if (other_type == CollisionObjectType::projectile_rolling_team_a ||
		other_type == CollisionObjectType::projectile_rolling_team_b)
	{
		this->health_ -= ROLLING_DAMAGE;
        //this->health_regen_timer_ = 0.0f;
	}
	else if (other_type == CollisionObjectType::projectile_ball_team_a ||
		other_type == CollisionObjectType::projectile_ball_team_b)
	{
		this->health_ -= BALL_DAMAGE;
        //this->health_regen_timer_ = 0.0f;
	}
	else if (other_type == CollisionObjectType::projectile_mist_team_a ||
		other_type == CollisionObjectType::projectile_mist_team_b)
	{
		this->health_ -= MIST_DAMAGE;
        //this->health_regen_timer_ = 0.0f;
	}
	else
	{
		throw std::exception("Invalid projectile type.");
    }
    this->health_regen_timer_ = 0.0f;

    if (this->damage_sound_timer_ >= DAMAGE_SOUND_DELAY)
	{
		this->sound_bank_->play_wave(this->damage_sound_, DAMAGE_SOUND_VOLUME);
		this->damage_sound_timer_ = 0.0f;
	}
}

void Player::update(float dt)
{
    if (this->state_ == PlayerState::alive)
    {
        this->update_movement(dt);

        // Clip selection has to happen here, once per frame, not in draw().
        // draw() runs once per viewport, concurrently on every render worker,
        // so mutating the animation state there was both a data race on the
        // std::string sheet/element names and wrong on its own terms: a player
        // off-screen in every viewport never advanced its animation at all,
        // because Level guards the draw call with is_visible_in_viewport.
        this->update_animation_state();

        AnimationObject::update(dt);

        if (this->health_ <= 0.0f)
        {
			this->state_ = PlayerState::dead;
			this->sound_bank_->play_wave(this->death_sound_, DEATH_SOUND_VOLUME);
        }

        this->damage_sound_timer_ += dt;
    }
    else if (this->state_ == PlayerState::dead)
    {
        this->respawn_timer_ -= dt;

        if (this->respawn_timer_ <= 0.0f)
        {
            this->respawn();
            this->respawn_timer_ = RESPAWN_DELAY;
        }
    }
	else
	{
		throw std::exception("Invalid player state.");
	}
    if (this->input_.toggle_debug)
    {
        this->showing_debug_ = !this->showing_debug_;
    }

    //ammo regen
    if (this->health_regen_timer_ >= HEALTH_REGEN_DELAY)
    {
        this->health_ += HEALTH_REGEN_RATE * dt;
        if (this->health_ > 1.0f)
        {
            this->health_ = 1.0f;
        }
    }
    this->health_regen_timer_ += dt;
}
std::vector<std::unique_ptr<CollisionObject>>
    Player::update_weapon_and_get_projectiles(float dt) const
{
    return this->primary_->update_and_get_projectiles(
        this->input(),
        this->center(),
        MovingObject::velocity(),
        this->facing_right(),
        dt);
}
void Player::update_prev_rectangle()
{
	this->prev_rectangle_ = this->rectangle_;
}

void Player::update_movement(float dt)
{
    const PlayerInputData input = this->input_;
    const float x_input = input.x_movement;
    const PlayerMoveState move_state = this->move_state();
    const bool analog_stick_down = this->input_.left_analog_stick.y >
        DROP_DOWN_ANALOG_THRESHOLD;
    const bool jump_pressed = this->input_.jump_pressed;

    //If the player's velocity is zero and the user is requesting to move the player
    if (are_equal(MovingObject::velocity_x(), 0.0f) && x_input != 0.0f)
    {
        MovingObject::set_velocity_x(X_INITIAL_VELOCITY * x_input);
    }
    //If accelerating in the same direction as currently moving. right
    else if (MovingObject::velocity_x() > 0.0f && x_input > 0.0f)
    {
        MovingObject::alter_velocity_x(X_ACCELERATION * dt * x_input);
        if (MovingObject::velocity_x() > MAX_VELOCITY.x)
        {
            MovingObject::set_velocity_x(MAX_VELOCITY.x);
        }
    }
    //If accelerating in the same direction as currently moving. left
    else if (MovingObject::velocity_x() < 0.0f && x_input < 0.0f)
    {
        MovingObject::alter_velocity_x(X_ACCELERATION * dt * x_input);
        if (MovingObject::velocity_x() < -MAX_VELOCITY.x)
        {
            MovingObject::set_velocity_x(-MAX_VELOCITY.x);
        }
    }
    //If accelerating in the opposite direction as currently moving. right
    else if (MovingObject::velocity_x() > 0.0f && x_input < 0.0f)
    {
        MovingObject::alter_velocity_x(X_DEACCELERATION * dt * x_input);
        MovingObject::alter_velocity_x(X_ACCELERATION * dt * x_input);
    }
    //If accelerating in the opposite direction as currently moving. left
    else if (MovingObject::velocity_x() < 0.0f && x_input > 0.0f)
    {
        MovingObject::alter_velocity_x(X_DEACCELERATION * dt * x_input);
        MovingObject::alter_velocity_x(X_ACCELERATION * dt * x_input);
    }
    //No key pressed deaccelaration right
    else if (MovingObject::velocity_x() > 0.0f && are_equal(x_input, 0.0f))
    {
        MovingObject::alter_velocity_x(-X_DEACCELERATION * dt);
        if (MovingObject::velocity_x() < 0.0f)
        {
            MovingObject::set_velocity_x(0.0f);
        }
    }
    //No key pressed deaccelaration left
    else if (MovingObject::velocity_x() < 0.0f && are_equal(x_input, 0.0f))
    {
        MovingObject::alter_velocity_x(X_DEACCELERATION * dt);
        if (MovingObject::velocity_x() > 0.0f)
        {
            MovingObject::set_velocity_x(0.0f);
        }
    }
    //gravity
    if (this->is_on_ramp())
    {
		// increase gravity on ramp
		MovingObject::alter_velocity_y(GRAVITY * RAMP_GRAVITY_MULTIPLIER * dt);
	}
	else
	{
		MovingObject::alter_velocity_y(GRAVITY * dt);
    }
    if (MovingObject::velocity_y() > MAX_VELOCITY.y)
    {
        MovingObject::set_velocity_y(MAX_VELOCITY.y);
    }

    // DROPPING_DOWN state only lasts for one frame
    if (move_state == PlayerMoveState::dropping_down)
	{
		this->set_move_state(PlayerMoveState::in_air);
	}

    // check if dropping down from a platform
    if (move_state == PlayerMoveState::on_drop_down_ground && analog_stick_down &&
        !jump_pressed)
    {
        MovingObject::alter_velocity_y(DROP_DOWN_VELOCITY);
        this->set_move_state(PlayerMoveState::dropping_down);
    }
	else
	{
        this->do_jump(dt);
	}

    const Vector2F displacement = MovingObject::velocity() * dt;
    this->rectangle_.offset(displacement.x, displacement.y);

    if (x_input > 0.0f)
    {
        this->set_facing_right(true);
    }
    else if (x_input < 0.0f)
    {
        this->set_facing_right(false);
    }
}

void Player::do_jump(float dt)
{
    const PlayerMoveState move_state = this->move_state();
    const bool jump_pressed = this->input_.jump_pressed;
    const bool jump_held = this->input_.jump_held;
   
    if (this->is_on_ground() && jump_pressed)
    {
		MovingObject::set_velocity_y(JUMP_LAUNCH_VELOCITY);
		this->set_air_time(0.0f);
		this->set_move_state(PlayerMoveState::jumping);
		this->sound_bank_->play_wave(this->jump_sound_, JUMP_SOUND_VOLUME);
	}
	else if (move_state == PlayerMoveState::on_ceiling)
	{
		// do nothing
	}
	else if (move_state == PlayerMoveState::jumping)
	{
		if (jump_held)
		{
			if (this->air_time() <= JUMP_MAX_TIME)
			{
				MovingObject::alter_velocity_y(JUMP_ACCELERATION * dt);
			}
			else
			{
				this->set_move_state(PlayerMoveState::in_air);
			}
		}
		else // !jump held
		{
			this->set_move_state(PlayerMoveState::in_air);
		}
		this->alter_air_time(dt);
	}
	else if (move_state == PlayerMoveState::in_air)
	{
		// do nothing
	}

    if (this->velocity_y() < -MAX_VELOCITY.y)
    {
        this->set_velocity_y(-MAX_VELOCITY.y);
    }
}

RectangleF Player::bounds() const
{
    // Inflated to take in the weapon, which is drawn from Player::draw but
    // is not inside the player's own rectangle.
    RectangleF player_and_weapon_rect = this->rectangle_;
    player_and_weapon_rect.inflate(Vector2F(200.0f, 200.0f));
    return player_and_weapon_rect;
}
void Player::set_player_input(const PlayerInputData& input)
{
	this->input_ = input;
}

const RectangleF& Player::rectangle() const
{
	return this->rectangle_;
}
Vector2F Player::center() const
{
	return this->rectangle_.center();
}
const Vector2F& Player::velocity() const
{
    return MovingObject::velocity();
}
const Camera& Player::camera() const
{
	return this->camera_;
}
const Viewport& Player::viewport() const
{
	return this->viewport_;
}
float Player::input_x_movement() const
{
	return this->input_.x_movement;
}
const Vector2F& Player::input_shoot_direction() const
{
	return this->input_.shoot_direction;
}
bool Player::input_primary_shoot() const
{
	return this->input_.primary_shoot;
}
bool Player::input_jump_pressed() const
{
	return this->input_.jump_pressed;
}
bool Player::input_jump_held() const
{
	return this->input_.jump_held;
}
PlayerMoveState Player::move_state() const
{
	return this->move_state_;
}
bool Player::showing_debug() const
{
	return this->showing_debug_;
}
const PlayerAnimationInfo& Player::animation_info(PlayerAnimationState state)
{
    switch (state)
    {
    case PlayerAnimationState::idle:
        return ANIMATION_INFO_IDLE;
    case PlayerAnimationState::walking:
        return ANIMATION_INFO_WALK;
    case PlayerAnimationState::running:
        return ANIMATION_INFO_RUNNING;
    case PlayerAnimationState::jumping:
        return ANIMATION_INFO_JUMPING;
    case PlayerAnimationState::falling:
        return ANIMATION_INFO_FALLING;
    default:
        throw std::exception("Invalid player animation state.");
    }
}
void Player::respawn()
{
    this->rectangle_.set_position(this->respawn_position_);
    MovingObject::set_rotation(0.0f);
    MovingObject::set_velocity(Vector2F::ZERO);
	this->health_ = 1.0f;
	this->primary_->reset_ammo();
	this->state_ = PlayerState::alive;
    this->sound_bank_->play_wave(this->respawn_sound_, RESPAWN_SOUND_VOLUME);
}
PlayerAnimationState Player::calculate_animation_state() const
{
    PlayerMoveState move_state = this->move_state();
    if (this->is_on_ground())
    {
        float velocity_x = this->velocity_x();
        if (velocity_x == 0.0f)
        {
			return PlayerAnimationState::idle;
		}
        else if (std::fabs(velocity_x) > 600.0f)
        {
			return PlayerAnimationState::running;
		}
        else
        {
			return PlayerAnimationState::walking;
		}
	}
    else if (move_state == PlayerMoveState::on_ceiling)
    {
		return PlayerAnimationState::idle;
	}
    else if (move_state == PlayerMoveState::jumping)
    {
		return PlayerAnimationState::jumping;
	}
    else if (move_state == PlayerMoveState::in_air ||
		move_state == PlayerMoveState::dropping_down)
    {
        if (this->velocity_y() > 0.0f)
        {
			return PlayerAnimationState::falling;
		}
        else
        {
			return PlayerAnimationState::jumping;
		}
	}
    else
    {
		throw std::exception("Invalid player move state.");
	}
}
void Player::stop_sounds() const
{
    this->primary_->stop_sounds();
}
void Player::set_move_state(PlayerMoveState state)
{
	this->move_state_ = state;
}

void Player::set_air_time(float time)
{
	this->air_time_ = time;
}

void Player::alter_air_time(float time)
{
	this->air_time_ += time;
}

void Player::set_facing_right(bool facing_right)
{
	this->facing_right_ = facing_right;
}

bool Player::facing_right() const
{
	return this->facing_right_;
}

int Player::player_num() const
{
	return this->player_num_;
}

float Player::health() const
{
	return this->health_;
}

void Player::set_health(float health)
{
	this->health_ = health;
}

void Player::alter_health(float change)
{
	this->health_ += change;
}

void Player::set_respawn_timer(float respawn_timer)
{
	this->respawn_timer_ = respawn_timer;
}

void Player::alter_respawn_timer(float change)
{
	this->respawn_timer_ += change;
}

void Player::set_score(int score)
{
	this->score_ = score;
}

void Player::alter_score(int change)
{
	this->score_ += change;
}

int Player::score() const
{
	return this->score_;
}

void Player::set_team(PlayerTeam team)
{
	this->team_ = team;
}

void Player::set_team_colour(const Colour& team_colour)
{
	this->team_colour_ = team_colour;
}

void Player::set_viewport(const Viewport& viewport)
{
	this->viewport_ = viewport;
}

void Player::set_input(const PlayerInputData& input)
{
	this->input_ = input;
}

WeaponType Player::primary() const
{
	return this->primary_type_;
}

void Player::set_primary(WeaponType primary)
{
	this->primary_type_ = primary;
}

WeaponType Player::secondary() const
{
	return this->secondary_type_;
}

void Player::set_secondary(WeaponType secondary)
{
	this->secondary_type_ = secondary;
}

void Player::set_showing_debug(bool showing_debug)
{
	this->showing_debug_ = showing_debug;
}

void Player::set_player_num(int player_num)
{
	this->player_num_ = player_num;
}

void Player::set_camera(const Camera& camera)
{
	this->camera_ = camera;
}

float Player::weapon_ammo() const
{
	return this->primary_->ammo();
}

const Colour& Player::team_colour() const
{
	return this->team_colour_;
}

PlayerState Player::state() const
{
	return this->state_;
}
void Player::set_state(PlayerState state)
{
	this->state_ = state;
}

PlayerTeam Player::team() const
{
	return this->team_;
}

float Player::respawn_timer() const
{
	return this->respawn_timer_;
}

float Player::air_time() const
{
	return this->air_time_;
}

const PlayerInputData& Player::input() const
{
	return this->input_;
}

std::string Player::player_move_state_string() const
{
    switch (this->move_state_)
    {
    case PlayerMoveState::on_ground:
        return "on_ground";
    case PlayerMoveState::on_drop_down_ground:
        return "on_drop_down_ground";
    case PlayerMoveState::on_ceiling:
        return "on_ceiling";
	case PlayerMoveState::on_ramp_left:
		return "on_ramp_left";
	case PlayerMoveState::on_ramp_right:
		return "on_ramp_right";
    case PlayerMoveState::in_air:
        return "in_air";
    case PlayerMoveState::jumping:
        return "jumping";
    };
	return "invalid";
}

bool Player::is_on_ground() const
{
	return this->move_state_ == PlayerMoveState::on_ground ||
		this->move_state_ == PlayerMoveState::on_drop_down_ground ||
		this->move_state_ == PlayerMoveState::on_ramp_left ||
		this->move_state_ == PlayerMoveState::on_ramp_right;
}

bool Player::is_on_ramp() const
{
	return this->move_state_ == PlayerMoveState::on_ramp_left ||
		this->move_state_ == PlayerMoveState::on_ramp_right;
}

void Player::on_no_collision()
{
	PlayerMoveState move_state = this->move_state();

	if (move_state == PlayerMoveState::on_ground ||
		move_state == PlayerMoveState::on_drop_down_ground ||
		move_state == PlayerMoveState::on_ramp_left ||
		move_state == PlayerMoveState::on_ramp_right)
	{
		this->set_move_state(PlayerMoveState::in_air);
	}
}