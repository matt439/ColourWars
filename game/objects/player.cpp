#include "game/pch.h"
#include "game/objects/player.h"

#include "engine/collision/resolve.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

namespace
{
    // The sprite tint the animated half of a player is drawn with; the uniform
    // half is drawn in the team colour. Not tuning: it is one colour, read
    // once, and no balance pass ever wants it.
    const Colour player_tint = Colour::burlywood;
}

Player::Player(const RectangleF& rectangle,
    const GameTuning* tuning,
    RenderResources* render_resources,
    const AudioResources* audio_resources,
    const PlaySettings* preferences,
    int player_num,
    PlayerTeam team,
    WeaponType primary,
    std::unique_ptr<Weapon> primary_weapon,
    WeaponType /*secondary*/,
    const Colour& team_colour,
    const Viewport& view_port,
    const mattmath::Vector2F& respawn_position,
    const Vector2F& velocity,
    float rotation,
    const Vector2F& origin,
    SpriteFlip flip,
    float layer_depth) :
    MovingObject(velocity),
    AnimationObject(tuning->player().idle.sheet,
                    tuning->player().idle.strip,
                    render_resources, player_tint, rotation, origin, flip, layer_depth),
    TextureObject(tuning->player().idle.sheet,
                  tuning->player().idle.uniform_texture,
                  render_resources, team_colour, rotation, origin, flip, layer_depth),
    tuning_(tuning),
    primary_(std::move(primary_weapon)),
    player_num_(player_num),
    team_(team),
    primary_type_(primary),
    team_colour_(team_colour),
    viewport_(view_port),
    rectangle_(rectangle),
    prev_rectangle_(rectangle),
    respawn_position_(respawn_position)
{
    this->preferences_ = preferences;
    const PlayerTuning& player = tuning->player();

    this->health_ = player.starting_health;
    this->respawn_timer_ = player.respawn_delay;

    this->sound_bank_ = audio_resources->sound_bank(player.sound_bank);

    this->damage_sound_ =
        this->sound_bank_->resolve_wave(player.damage_sound.name);
    this->death_sound_ =
        this->sound_bank_->resolve_wave(player.death_sound.name);
    this->jump_sound_ =
        this->sound_bank_->resolve_wave(player.jump_sound.name);
    this->respawn_sound_ =
        this->sound_bank_->resolve_wave(player.respawn_sound.name);
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

    const PlayerAnimationTuning& info = this->animation_info(new_animation_state);
    TextureObject::set_frame(info.sheet, info.uniform_texture);

    set_animation_strip_and_reset(info.sheet, info.strip);
    if (info.frame_time.has_value())
    {
        set_frame_time(*info.frame_time);
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
CollisionObjectType Player::collision_type() const
{
    switch (this->team_)
    {
    case PlayerTeam::a:
        return CollisionObjectType::player_team_a;
    case PlayerTeam::b:
        return CollisionObjectType::player_team_b;
    default:
        throw std::runtime_error("Invalid player team");
    }
}
CollisionLayer Player::layer() const
{
    return collision_layer(this->collision_type());
}
CollisionMask Player::mask() const
{
    // Dead players collide with nothing, and this is now the only place that
    // says so. It used to be said twice and disagree: Player::is_colliding
    // returned false when dead, and separately the type accessor returned a
    // *_dead enumerator that every hand-written type list quietly omitted. The
    // enumerators are gone; an empty mask is the same fact, in the one place
    // that changes.
    if (this->state_ != PlayerState::alive)
    {
        return 0;
    }
    return collision_mask(this->collision_type());
}
CollisionTag Player::tag() const
{
    return to_collision_tag(this->collision_type());
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

void Player::on_contact(const CollisionObject& other, const Vector2F& normal,
    float penetration)
{
    const CollisionObjectType other_type = to_collision_type(other.tag());

    if (is_projectile(other_type))
    {
        this->on_projectile_contact(other_type);
        return;
    }

    if (!is_structure(other_type))
    {
        // Unreachable through the mask, and loud rather than silent if the
        // mask and this ever come apart.
        throw std::runtime_error(
            "Player contacted an object its mask should have excluded");
    }

    // Any level geometry counts as ground contact for end_contacts(),
    // including a drop-through platform the player is rising past. That is
    // how it has always behaved: the level set its bool from the type, before
    // any response decided whether to do anything about it.
    this->touched_structure_ = true;

    if (other_type == CollisionObjectType::structure_jump_through)
    {
        this->on_jump_through_contact(other);
        return;
    }
    if (is_structure_ramp(other_type))
    {
        this->on_ramp_contact(other_type, normal, penetration);
        return;
    }
    this->on_structure_contact(normal, penetration);
}
void Player::end_contacts()
{
    const PlayerMoveState move_state = this->move_state();

    // on_ceiling belongs on this list with the four ground states. Nothing else
    // clears it, so a head bonk used to leave it set for the whole descent -
    // harmless to the physics, since do_jump treats it exactly as in_air, but
    // calculate_animation_state answers it with the idle clip instead of the
    // falling one.
    if (!this->touched_structure_ &&
        (move_state == PlayerMoveState::on_ground ||
        move_state == PlayerMoveState::on_drop_down_ground ||
        move_state == PlayerMoveState::on_ramp_left ||
        move_state == PlayerMoveState::on_ramp_right ||
        move_state == PlayerMoveState::on_ceiling))
    {
        this->set_move_state(PlayerMoveState::in_air);
    }

    this->touched_structure_ = false;
}
void Player::on_structure_contact(const Vector2F& normal, float penetration)
{
    this->rectangle_.offset(separation(normal, penetration));

    // Structures are axis-aligned, so the normal is exactly one of four. The
    // comparison rather than an equality test is what keeps this honest if a
    // rotated one ever arrives.
    if (std::abs(normal.y) >= std::abs(normal.x))
    {
        MovingObject::set_velocity_y(0.0f);
        this->set_move_state(normal.y > 0.0f ?
            PlayerMoveState::on_ground : PlayerMoveState::on_ceiling);
    }
    else
    {
        this->set_velocity_x(0.0f);
    }
}
void Player::on_ramp_contact(CollisionObjectType other_type,
    const Vector2F& normal, float penetration)
{
    // A ramp's two right-angled faces are a wall and a flat underside, and
    // both behave exactly like the flat structure they are part of. Only the
    // hypotenuse is a ramp, and the normal is how it is recognised. The 130
    // lines this replaces recognised it by snapping the centre-to-centre
    // direction to one of eight compass points and then enumerating, per ramp
    // handedness, which of the eight each face was allowed to produce - and
    // threw on the ones nobody had listed.
    if (normal.y <= 0.0f)
    {
        this->on_structure_contact(normal, penetration);
        return;
    }

    // Standing on the slope. Separate straight up rather than along the
    // slope's own normal: resolving a ground contact along its normal slides
    // the player back down the hill, so they could never walk up one. The
    // axis is the vertical one and its sign does not matter - the distance
    // comes out of the normal.
    this->rectangle_.offset(
        separation_along(normal, penetration, Vector2F::DIRECTION_DOWN));

    this->set_move_state(other_type == CollisionObjectType::structure_ramp_left ?
        PlayerMoveState::on_ramp_left : PlayerMoveState::on_ramp_right);

    MovingObject::set_velocity_y(0.0f);
}
void Player::on_jump_through_contact(const CollisionObject& other)
{
    // Only lands if the player is moving down and was above the platform on
    // the previous tick.
    // Screen space, so down is +y. This used to ask Vector2F::direction()
    // for one of nine compass values and test the three with "down" in the
    // name, which is the sign of y and nothing else.
    const bool moving_down = this->velocity().y > 0.0f;

    const RectangleF other_rect = other.shape()->bounding_box();
    const bool was_above = this->prev_rectangle_.bottom() <= other_rect.top();

    if (moving_down && was_above &&
        this->move_state() != PlayerMoveState::dropping_down)
    {
        MovingObject::set_velocity_y(0.0f);
        this->rectangle_.set_position_y_from_bottom(other_rect.top());
        this->set_move_state(PlayerMoveState::on_drop_down_ground);
    }
}

void Player::update_weapon_position() const
{
    this->primary_->set_player_center(this->center());
}
void Player::on_projectile_contact(CollisionObjectType other_type)
{
    // One lookup where a ten-branch if-chain used to be, ending in a throw for
    // a case its own caller had already excluded. The five numbers it dispatched
    // to lived in player_consts.h - a second, disagreeing copy of the
    // player_damage already on ProjectileDetails, which nothing read. There is
    // one copy now and it is in ./tuning.json, on the projectile.
    this->health_ -= this->tuning_->projectile(
        to_projectile_type(other_type)).details.player_damage;

    this->health_regen_timer_ = 0.0f;

    if (this->damage_sound_timer_ >= this->player_tuning().damage_sound_delay)
	{
		this->sound_bank_->play_wave(this->damage_sound_, this->player_tuning().damage_sound.volume * this->preferences_->sfx_gain());
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
			this->sound_bank_->play_wave(this->death_sound_, this->player_tuning().death_sound.volume * this->preferences_->sfx_gain());
        }

        this->damage_sound_timer_ += dt;
    }
    else if (this->state_ == PlayerState::dead)
    {
        this->respawn_timer_ -= dt;

        if (this->respawn_timer_ <= 0.0f)
        {
            this->respawn();
            this->respawn_timer_ = this->player_tuning().respawn_delay;
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

    // Health regen. The ceiling is the health a player starts with, which is
    // what the two hardcoded 1.0f here and in respawn() were assuming.
    const PlayerTuning& tuning = this->player_tuning();
    if (this->health_regen_timer_ >= tuning.health_regen_delay)
    {
        this->health_ += tuning.health_regen_rate * dt;
        if (this->health_ > tuning.starting_health)
        {
            this->health_ = tuning.starting_health;
        }
    }
    this->health_regen_timer_ += dt;
}
std::vector<std::unique_ptr<CollisionObject>>
    Player::update_weapon_and_get_projectiles(float dt,
    std::span<CollisionObject* const> objects) const
{
    // A dead player fires nothing. The pad is still read into input_ while the
    // respawn timer runs, so forwarding it here had a corpse - invisible,
    // uncollidable and frozen at the death spot - painting in its team colour,
    // and scoring for it, for the whole respawn delay.
    //
    // The loop has to be stopped rather than merely left alone: the weapon ends
    // its own loop from inside the update being skipped here, so a player who
    // died mid-fire would otherwise spray audibly until they respawned.
    // Stopping it every dead frame is the same idempotent call the weapon makes
    // every frame the trigger is not held.
    if (this->state_ != PlayerState::alive)
    {
        this->stop_sounds();
        return {};
    }

    auto projectiles = this->primary_->update_and_get_projectiles(
        this->input(),
        this->center(),
        MovingObject::velocity(),
        this->facing_right(),
        dt);
    for (const auto& projectile : projectiles)
    {
        // Weapons produce paint projectiles; reject a builder that breaks
        // that contract before it can bypass the launch collision check.
        auto& paint = dynamic_cast<Projectile&>(*projectile);
        paint.resolve_launch_obstruction(this->center(), objects);
    }
    std::erase_if(projectiles, [](const auto& projectile)
        { return projectile->for_deletion(); });
    return projectiles;
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
    const bool drop_through = this->input_.drop_through;
    const bool jump_pressed = this->input_.jump_pressed;

    //If the player's velocity is zero and the user is requesting to move the player
    if (are_equal(MovingObject::velocity_x(), 0.0f) && x_input != 0.0f)
    {
        MovingObject::set_velocity_x(this->player_tuning().x_initial_velocity * x_input);
    }
    //If accelerating in the same direction as currently moving. right
    else if (MovingObject::velocity_x() > 0.0f && x_input > 0.0f)
    {
        MovingObject::alter_velocity_x(this->player_tuning().x_acceleration * dt * x_input);
        if (MovingObject::velocity_x() > this->player_tuning().max_velocity.x)
        {
            MovingObject::set_velocity_x(this->player_tuning().max_velocity.x);
        }
    }
    //If accelerating in the same direction as currently moving. left
    else if (MovingObject::velocity_x() < 0.0f && x_input < 0.0f)
    {
        MovingObject::alter_velocity_x(this->player_tuning().x_acceleration * dt * x_input);
        if (MovingObject::velocity_x() < -this->player_tuning().max_velocity.x)
        {
            MovingObject::set_velocity_x(-this->player_tuning().max_velocity.x);
        }
    }
    //If accelerating in the opposite direction as currently moving. right
    else if (MovingObject::velocity_x() > 0.0f && x_input < 0.0f)
    {
        MovingObject::alter_velocity_x(this->player_tuning().x_deacceleration * dt * x_input);
        MovingObject::alter_velocity_x(this->player_tuning().x_acceleration * dt * x_input);
    }
    //If accelerating in the opposite direction as currently moving. left
    else if (MovingObject::velocity_x() < 0.0f && x_input > 0.0f)
    {
        MovingObject::alter_velocity_x(this->player_tuning().x_deacceleration * dt * x_input);
        MovingObject::alter_velocity_x(this->player_tuning().x_acceleration * dt * x_input);
    }
    //No key pressed deaccelaration right
    else if (MovingObject::velocity_x() > 0.0f && are_equal(x_input, 0.0f))
    {
        MovingObject::alter_velocity_x(-this->player_tuning().x_deacceleration * dt);
        if (MovingObject::velocity_x() < 0.0f)
        {
            MovingObject::set_velocity_x(0.0f);
        }
    }
    //No key pressed deaccelaration left
    else if (MovingObject::velocity_x() < 0.0f && are_equal(x_input, 0.0f))
    {
        MovingObject::alter_velocity_x(this->player_tuning().x_deacceleration * dt);
        if (MovingObject::velocity_x() > 0.0f)
        {
            MovingObject::set_velocity_x(0.0f);
        }
    }
    //gravity
    if (this->is_on_ramp())
    {
		// increase gravity on ramp
		MovingObject::alter_velocity_y(this->player_tuning().gravity * this->player_tuning().ramp_gravity_multiplier * dt);
	}
	else
	{
		MovingObject::alter_velocity_y(this->player_tuning().gravity * dt);
    }
    if (MovingObject::velocity_y() > this->player_tuning().max_velocity.y)
    {
        MovingObject::set_velocity_y(this->player_tuning().max_velocity.y);
    }

    // DROPPING_DOWN state only lasts for one frame
    if (move_state == PlayerMoveState::dropping_down)
	{
		this->set_move_state(PlayerMoveState::in_air);
	}

    // check if dropping down from a platform
    if (move_state == PlayerMoveState::on_drop_down_ground && drop_through &&
        !jump_pressed)
    {
        MovingObject::alter_velocity_y(this->player_tuning().drop_down_velocity);
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
		MovingObject::set_velocity_y(this->player_tuning().jump_launch_velocity);
		this->set_air_time(0.0f);
		this->set_move_state(PlayerMoveState::jumping);
		this->sound_bank_->play_wave(this->jump_sound_, this->player_tuning().jump_sound.volume * this->preferences_->sfx_gain());
	}
	else if (move_state == PlayerMoveState::on_ceiling)
	{
		// do nothing
	}
	else if (move_state == PlayerMoveState::jumping)
	{
		if (jump_held)
		{
			if (this->air_time() <= this->player_tuning().jump_max_time)
			{
				MovingObject::alter_velocity_y(this->player_tuning().jump_acceleration * dt);
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

    if (this->velocity_y() < -this->player_tuning().max_velocity.y)
    {
        this->set_velocity_y(-this->player_tuning().max_velocity.y);
    }
}

RectangleF Player::bounds() const
{
    // The union of what is actually drawn, rather than a margin large enough
    // to be sure of covering it.
    //
    // The weapon is drawn from Player::draw and reaches outside the player's
    // own rectangle, which is the real requirement here. Meeting it by
    // inflating 200 units in every direction turned a 52 x 120 collider into
    // 452 x 520 - thirty-eight times the area - on the object that is queried
    // more than any other. bounds() is the coarse extent a broad phase
    // indexes and the scene culls against (engine/core/game_object.h), so
    // every unit of slack here is paid for on every query, and a bound this
    // loose would hide any index that ever tried to use it.
    if (this->primary_ == nullptr)
    {
        return this->rectangle_;
    }

    return RectangleF::union_of(this->rectangle_, this->primary_->draw_bounds());
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
const PlayerTuning& Player::player_tuning() const
{
    return this->tuning_->player();
}

const PlayerAnimationTuning& Player::animation_info(
    PlayerAnimationState state) const
{
    switch (state)
    {
    case PlayerAnimationState::idle:
        return this->player_tuning().idle;
    case PlayerAnimationState::walking:
        return this->player_tuning().walk;
    case PlayerAnimationState::running:
        return this->player_tuning().running;
    case PlayerAnimationState::jumping:
        return this->player_tuning().jumping;
    case PlayerAnimationState::falling:
        return this->player_tuning().falling;
    }

    throw std::runtime_error("Player animation state " +
        std::to_string(static_cast<int>(state)) + " has no clip.");
}
void Player::respawn()
{
    this->rectangle_.set_position(this->respawn_position_);
    MovingObject::set_rotation(0.0f);
    MovingObject::set_velocity(Vector2F::ZERO);
	this->health_ = this->player_tuning().starting_health;
	this->primary_->reset_ammo();
	this->state_ = PlayerState::alive;
    this->sound_bank_->play_wave(this->respawn_sound_, this->player_tuning().respawn_sound.volume * this->preferences_->sfx_gain());
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
void Player::pause_sounds() const
{
    this->primary_->pause_sounds();
}
void Player::resume_sounds() const
{
    this->primary_->resume_sounds();
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
    case PlayerMoveState::dropping_down:
        return "dropping_down";
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
