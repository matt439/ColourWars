#include "game/pch.h"
#include "game/objects/player.h"
#include "game/objects/paint_audio.h"
#include "game/objects/structure_paintable.h"

#include "engine/collision/resolve.h"

#include <limits>

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

namespace
{
    // The authored body is neutral; its matching clothing clip carries team colour.
    const Colour player_tint = Colour::white;

    int action_priority(PlayerAnimationState state)
    {
        switch (state)
        {
        case PlayerAnimationState::firing: return 0;
        case PlayerAnimationState::landing:
        case PlayerAnimationState::taking_off: return 1;
        case PlayerAnimationState::hurt: return 2;
        case PlayerAnimationState::elimination: return 3;
        default: throw std::runtime_error("Locomotion is not a player action.");
        }
    }
}

Player::Player(const RectangleF& rectangle,
    const GameTuning* tuning,
    const MovementFeedbackPresentation* movement_feedback,
    const CombatFeedbackPresentation* combat_feedback,
    const RespawnRedrawPresentation* respawn_redraw,
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
    float layer_depth,
    int sound_index,
    const FigurePresentation* figure,
    int teammate_index) :
    MovingObject(velocity),
    AnimationObject(tuning->player().idle.sheet,
                    tuning->player().idle.strip,
                    render_resources, player_tint, rotation, origin, flip, layer_depth),
    TextureObject(tuning->player().idle.sheet,
                  tuning->player().idle.uniform_texture,
                  render_resources, team_colour, rotation, origin, flip, layer_depth),
    tuning_(tuning),
    clothing_(tuning->player().idle, render_resources, team_colour,
        rotation, origin, flip, layer_depth),
    attachments_(movement_feedback, figure, render_resources,
        teammate_index >= 0 ? teammate_index :
            (sound_index >= 0 ? sound_index : std::max(player_num, 0) % 4)),
    quiet_character_(figure, sound_index >= 0 ? sound_index : std::max(player_num, 0)),
    movement_feedback_(movement_feedback, render_resources),
    combat_feedback_(combat_feedback, render_resources),
    movement_audio_(&tuning->player().movement_sounds,
        audio_resources->sound_bank(tuning->player().sound_bank), preferences,
        sound_index >= 0 ? sound_index : player_num),
    respawn_redraw_(respawn_redraw, render_resources),
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
    this->movement_feedback_look_ = movement_feedback;
    const PlayerTuning& player = tuning->player();
    this->idle_sheet_ = render_resources->resolve_sprite_sheet(player.idle.sheet);
    const SpriteSheet* idle_sheet = render_resources->sprite_sheet(this->idle_sheet_);
    this->idle_body_ = idle_sheet->animation_strip(
        idle_sheet->resolve_animation_strip(player.idle.strip)).frame_rect(0);
    this->idle_paint_ = idle_sheet->animation_strip(
        idle_sheet->resolve_animation_strip(player.idle.paint_strip)).frame_rect(0);
    for (const PlayerAnimationState state : { PlayerAnimationState::taking_off,
        PlayerAnimationState::landing, PlayerAnimationState::firing,
        PlayerAnimationState::hurt, PlayerAnimationState::elimination })
    {
        const PlayerAnimationTuning& clip = this->animation_info(state);
        const SpriteSheet* sheet = render_resources->sprite_sheet(clip.sheet);
        const AnimationStrip& strip = sheet->animation_strip(
            sheet->resolve_animation_strip(clip.strip));
        this->action_clips_[static_cast<size_t>(state) -
            static_cast<size_t>(PlayerAnimationState::taking_off)] =
            { strip.frame_count(), strip.frame_time() };
    }
    if (player.idle.frame_time.has_value())
    {
        AnimationObject::set_frame_time(*player.idle.frame_time);
    }

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
    const int index = sound_index >= 0 ? sound_index : std::max(player_num, 0) % 4;
    if (index >= static_cast<int>(player.redraw_sounds.size()))
    {
        throw std::runtime_error("Player redraw sound requires a roster index from 0 to 3");
    }
    this->redraw_index_ = static_cast<size_t>(index);
    this->redraw_sound_ = this->sound_bank_->resolve_effect(
        player.redraw_sounds[this->redraw_index_].name);
}

Player::~Player()
{
    this->stop_redraw_sound();
}

void Player::update_animation_state()
{
    this->set_animation_state(this->action_.value_or(
        this->state_ == PlayerState::dead ? PlayerAnimationState::idle :
        this->calculate_animation_state()));
}

void Player::set_animation_state(PlayerAnimationState new_animation_state, bool restart)
{
    if (this->animation_state_ == new_animation_state && !restart)
    {
        return;
    }

    this->animation_state_ = new_animation_state;

    const PlayerAnimationTuning& info = this->animation_info(new_animation_state);
    TextureObject::set_frame(info.sheet, info.uniform_texture);

    set_animation_strip_and_reset(info.sheet, info.strip);
    this->clothing_.select(info);
    AnimationObject::play();
    if (info.frame_time.has_value())
    {
        set_frame_time(*info.frame_time);
    }
    else
    {
        set_frame_time_to_default();
    }
}

Player::ActionClip Player::action_clip(PlayerAnimationState state) const
{
    ActionClip clip = this->action_clips_.at(static_cast<size_t>(state) -
        static_cast<size_t>(PlayerAnimationState::taking_off));
    clip.frame_time = this->animation_info(state).frame_time.value_or(clip.frame_time);
    return clip;
}

void Player::start_action(PlayerAnimationState state)
{
    if (this->action_.has_value())
    {
        // Automatic fire must reach its recovery frame instead of resetting on
        // every shot. New contact/jump/hurt events can interrupt lower poses.
        if ((state == PlayerAnimationState::firing && this->action_ == state) ||
            action_priority(state) < action_priority(*this->action_))
        {
            return;
        }
    }
    this->action_ = state;
    this->action_age_ = 0.0f;
    this->set_animation_state(state, true);
    this->update_action_frame();
}

void Player::age_action(float dt)
{
    if (!this->action_.has_value()) { return; }
    this->action_age_ += std::max(dt, 0.0f);
    const ActionClip clip = this->action_clip(*this->action_);
    if (this->action_age_ >= clip.frame_time * static_cast<float>(clip.frames))
    {
        this->action_.reset();
        this->action_age_ = 0.0f;
    }
}

void Player::update_action_frame()
{
    if (!this->action_.has_value()) { return; }
    const int frame = this->action_frame();
    AnimationObject::set_frame_index(frame);
    this->clothing_.select_frame(frame);
}

PlayerAnimationState Player::animation_state() const { return this->animation_state_; }
float Player::action_age() const { return this->action_age_; }
int Player::action_frame() const
{
    if (!this->action_.has_value()) { return 0; }
    const ActionClip clip = this->action_clip(*this->action_);
    return static_cast<int>(std::min(this->action_age_ / clip.frame_time,
        static_cast<float>(clip.frames - 1)));
}

void Player::draw(DrawList& draw_list) const
{
    if (this->state_ == PlayerState::dead && !this->drawn_whole())
    {
        if (this->action_ == PlayerAnimationState::elimination)
        {
            this->draw_figure(draw_list, this->posed_rectangle());
        }
        this->draw_respawn(draw_list);
        this->combat_feedback_.draw(draw_list, this->posed_rectangle(),
            this->grey_.value_or(this->team_colour_));
        return;
    }

    this->draw_figure(draw_list, this->posed_rectangle(), this->pose_.whole);
    // A figure drawn whole after dying has its tool where it fell.
    if (this->pose_.tool && !this->drawn_whole())
    {
        this->attachments_.draw_arms(draw_list, this->posed_rectangle(),
            this->facing_right(), *this->primary_);
        this->primary_->draw(draw_list, this->showing_debug_);
        this->attachments_.draw_hands(draw_list, this->posed_rectangle(), *this->primary_);
    }
    this->movement_feedback_.draw(draw_list);
    if (!this->drawn_whole())
    {
        this->combat_feedback_.draw(draw_list, this->posed_rectangle(),
            this->grey_.value_or(this->team_colour_));
    }
}

RectangleF Player::posed_rectangle() const
{
    const RectangleF base = this->drawn_whole() ? this->spawn_rectangle() : this->rectangle_;
    const float squash = base.height * this->pose_.squash;
    const RectangleF posed(base.x + this->pose_.offset.x, base.y + this->pose_.offset.y + squash,
        base.width, base.height - squash);
    return this->state_ == PlayerState::alive && this->health_ > 0.0f &&
        this->pose_.tool && !this->pose_.whole ? this->quiet_character_.rectangle(posed) : posed;
}

bool Player::drawn_whole() const
{
    return this->state_ == PlayerState::dead && this->pose_.whole;
}

void Player::set_pose(const FigurePose& pose)
{
    this->pose_ = pose;
    if (this->drawn_whole())
    {
        this->stop_redraw_sound();
    }
}

void Player::draw_figure(DrawList& draw_list, const RectangleF& rectangle, bool idle) const
{
    // Use the clip selected by update_animation_state; draw must not mutate shared state.
    const SpriteFlip flip = this->facing_right()
        ? SpriteFlip::none
        : SpriteFlip::horizontal;

    if (idle)
    {
        // Respawn's pen and the story whole figure always use the traced idle,
        // even while the eliminated figure still acts at its death position.
        const SpriteSheet* sheet = AnimationObject::render_resources()->sprite_sheet(this->idle_sheet_);
        sheet->draw(draw_list, this->idle_paint_, rectangle,
            this->grey_.value_or(this->TextureObject::colour()),
            AnimationObject::draw_rotation(), AnimationObject::origin(), flip,
            AnimationObject::layer_depth());
        sheet->draw(draw_list, this->idle_body_, rectangle,
            this->AnimationObject::colour(), AnimationObject::draw_rotation(),
            AnimationObject::origin(), flip, AnimationObject::layer_depth());
    }
    else
    {
        this->clothing_.draw(draw_list, rectangle,
            this->grey_.value_or(this->TextureObject::colour()), flip);
        AnimationObject::draw_with(draw_list, rectangle,
            this->AnimationObject::colour(), flip);
        if (this->quiet_character_.state().blinking && this->state_ == PlayerState::alive &&
            this->pose_.tool && !this->pose_.whole)
        {
            this->attachments_.draw_blink(draw_list, rectangle, this->facing_right());
        }
    }
    this->attachments_.draw_accent(draw_list, rectangle, this->facing_right());
    if (!this->pose_.tool || this->state_ == PlayerState::dead)
    {
        this->attachments_.draw_rest_arms(draw_list, rectangle, this->facing_right());
    }
}

void Player::draw_respawn(DrawList& draw_list) const
{
    const float progress = this->respawn_progress();
    if (this->respawn_redraw_.filled(progress))
    {
        this->draw_figure(draw_list, this->spawn_rectangle(), true);
        return;
    }
    this->respawn_redraw_.draw(draw_list, this->spawn_rectangle(),
        this->facing_right(), progress);
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
    // An empty mask excludes dead players from every contact.
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
        // The collision mask must exclude all other types.
        throw std::runtime_error(
            "Player contacted an object its mask should have excluded");
    }

    if (other_type == CollisionObjectType::structure_jump_through)
    {
        this->on_jump_through_contact(other);
        return;
    }
    if (is_structure_ramp(other_type))
    {
        this->on_ramp_contact(other_type, other, normal, penetration);
        return;
    }
    this->on_structure_contact(other, normal, penetration);
}
void Player::end_contacts(std::span<const Contact> contacts)
{
    // Dispatch remeasures each pair and can skip an adjacent floor once the
    // first has separated the feet. Retain the measured top candidates too;
    // painted_support checks that they still touch the final footprint.
    for (const Contact& contact : contacts)
    {
        const CollisionObject* other = nullptr;
        Vector2F normal;
        if (contact.a == this)
        {
            other = contact.b;
            normal = contact.normal;
        }
        else if (contact.b == this)
        {
            other = contact.a;
            normal = -contact.normal;
        }
        if (other != nullptr && normal.y > 0.0f && std::abs(normal.y) >= std::abs(normal.x))
        {
            if (const auto* paintable = dynamic_cast<const StructurePaintable*>(other))
            {
                this->paint_supports_.push_back(paintable);
            }
        }
    }
    const PlayerMoveState move_state = this->move_state();
    const bool established_support = this->landing_feedback_ready_;

    if (this->state_ != PlayerState::alive || this->health_ <= 0.0f)
    {
        // Contact order must not leave a landing puff from a fatal impact.
        this->movement_feedback_.clear();
        this->had_ground_support_ = false;
        this->landing_feedback_ready_ = false;
    }
    else
    {
        if (this->supported_this_tick_)
        {
            // Suppress the first settling contact after spawn. Thereafter only
            // a real unsupported-to-supported transition can land, including a
            // drop or a walked-off ledge. Ground/ramp seams remain quiet.
            if (!this->had_ground_support_ && this->landing_feedback_ready_)
            {
                this->movement_feedback_.land(this->support_position_,
                    this->support_impact_speed_);
                if (this->support_impact_speed_ >= this->movement_feedback_look_->landing_min_speed)
                {
                    this->start_action(PlayerAnimationState::landing);
                }
            }
            this->landing_feedback_ready_ = true;
        }
        this->had_ground_support_ = this->supported_this_tick_;
    }

    // Only a resolving ground contact maintains support. Track ceiling contact
    // separately so a head bonk does not leave the idle animation active during descent.
    if ((this->is_on_ground() && !this->supported_this_tick_) ||
        (move_state == PlayerMoveState::on_ceiling &&
            !this->hit_ceiling_this_tick_))
    {
        this->set_move_state(PlayerMoveState::in_air);
    }

    if (this->state_ != PlayerState::alive || this->health_ <= 0.0f)
    {
        this->reset_jump_input();
    }
    else if (this->hit_ceiling_this_tick_)
    {
        // A ceiling is not support for a grace jump. Wait for a later landing
        // before consuming a buffer, including simultaneous floor/ceiling hits.
        this->ledge_grace_remaining_ = 0.0f;
    }
    else if (this->supported_this_tick_)
    {
        this->ledge_grace_remaining_ = this->player_tuning().ledge_grace_time;
        if (this->move_state() == PlayerMoveState::on_drop_down_ground &&
            this->input_.drop_through && !this->input_.jump_pressed)
        {
            // A held drop will leave this platform on the next update. An old
            // airborne buffer must not turn that intent into a bounce instead.
            this->reset_jump_input();
        }
        else if (this->jump_buffer_remaining_ > 0.0f &&
            !this->jump_waiting_for_release_)
        {
            // Contacts run after movement. Launch here so a buffered landing
            // responds on the contact tick, after every separation is complete.
            this->start_jump();
            this->update_animation_state();
        }
    }

    // Ground-aware tool feedback sees the completed contact phase, including a
    // buffered jump. Fatal contacts and spawn settling silently reset history.
    const bool can_act = this->state_ == PlayerState::alive && this->health_ > 0.0f;
    const float resolved_x = this->rectangle_.x - this->movement_start_x_;
    // Separation from a wall or an overlapping spawn is not walked distance.
    const float distance = resolved_x * this->movement_requested_x_ > 0.0f ?
        std::min(std::abs(resolved_x), std::abs(this->movement_requested_x_)) : 0.0f;
    const std::optional<PaintedSupport> paint = this->painted_support();
    const MovementContactCues cues = this->movement_audio_.end_contacts(can_act, this->supported_this_tick_,
        this->is_on_ground(), this->support_impact_speed_, distance, this->movement_previous_velocity_,
        this->input_.x_movement, this->movement_audio_dt_, paint.has_value());
    // Share the stride/reversal event with audio, including while muted. A
    // skid replaces that tick's step visually to keep the contact uncluttered.
    if (cues.skid || cues.step)
    {
        this->movement_feedback_.contact(cues.skid ? MovementFeedbackKind::skid :
            MovementFeedbackKind::step, paint ? paint->position : this->support_position_,
            cues.skid ? this->movement_previous_velocity_ : resolved_x,
            paint ? std::optional<Colour>(paint->colour) : std::nullopt);
    }
    this->movement_audio_dt_ = 0.0f;
    this->primary_->update_ground_contact(
        can_act && this->supported_this_tick_ && this->is_on_ground(),
        can_act && established_support, this->weapon_contact_dt_);
    if (can_act)
    {
        this->primary_->update_resolved_motion(
            this->supported_this_tick_ && this->is_on_ground(), established_support,
            std::copysign(distance, resolved_x), this->weapon_contact_dt_);
    }
    else
    {
        this->primary_->reset_motion();
    }

    this->quiet_character_.update(can_act,
        this->supported_this_tick_ && this->is_on_ground(),
        std::abs(this->velocity().x) < 0.01f && std::abs(this->input_.x_movement) < 0.01f &&
            !this->input_.primary_shoot,
        this->action_.has_value(), this->movement_previous_velocity_, this->velocity().x,
        this->weapon_contact_dt_);

    this->supported_this_tick_ = false;
    this->hit_ceiling_this_tick_ = false;
    this->support_impact_speed_ = 0.0f;
    this->paint_supports_.clear();
}

std::optional<Player::PaintedSupport> Player::painted_support() const
{
    for (const StructurePaintable* support : this->paint_supports_)
    {
        const RectangleF surface = support->shape()->bounding_box();
        const float left = std::max(this->rectangle_.left(), surface.left());
        const float right = std::min(this->rectangle_.right(), surface.right());
        // Later contacts may have separated the feet from this surface. Use
        // a few representable float steps, rather than a world-sized margin.
        const float tolerance = 4.0f * std::numeric_limits<float>::epsilon() *
            std::max({ 1.0f, std::abs(surface.top()), std::abs(this->rectangle_.y),
                this->rectangle_.height, std::abs(this->rectangle_.bottom()) });
        if (left < right && std::abs(this->rectangle_.bottom() - surface.top()) <= tolerance)
        {
            if (const std::optional<Colour> colour =
                support->top_paint_colour(left + (right - left) / 2.0f))
            {
                // A seam's resolving floor can be dry while this adjacent
                // measured support is wet. Keep tint and origin together.
                return PaintedSupport{ *colour,
                    { left + (right - left) / 2.0f, surface.top() } };
            }
        }
    }
    return std::nullopt;
}

void Player::on_structure_contact(const CollisionObject& other,
    const Vector2F& normal, float penetration)
{
    this->rectangle_.offset(separation(normal, penetration));

    // Classify the contact by its dominant normal component.
    if (std::abs(normal.y) >= std::abs(normal.x))
    {
        if (normal.y > 0.0f)
        {
            this->support_impact_speed_ = std::max(this->support_impact_speed_,
                this->velocity().y);
            const RectangleF surface = other.shape()->bounding_box();
            this->support_position_ = { (std::max(this->rectangle_.left(), surface.left()) +
                std::min(this->rectangle_.right(), surface.right())) / 2.0f,
                this->rectangle_.bottom() };
        }
        MovingObject::set_velocity_y(0.0f);
        if (normal.y > 0.0f)
        {
            this->supported_this_tick_ = true;
            if (const auto* paintable = dynamic_cast<const StructurePaintable*>(&other))
            {
                this->paint_supports_.push_back(paintable);
            }
        }
        else
        {
            this->hit_ceiling_this_tick_ = true;
        }
        this->set_move_state(normal.y > 0.0f ?
            PlayerMoveState::on_ground : PlayerMoveState::on_ceiling);
    }
    else
    {
        this->set_velocity_x(0.0f);
    }
}
void Player::on_ramp_contact(CollisionObjectType other_type,
    const CollisionObject& other, const Vector2F& normal, float penetration)
{
    // A ramp's vertical face and flat underside resolve as walls; only its
    // hypotenuse needs slope handling.
    if (normal.y <= 0.0f)
    {
        this->on_structure_contact(other, normal, penetration);
        return;
    }

    // Separate vertically on slopes; separation along the normal would slide
    // the player downhill and prevent climbing.
    this->rectangle_.offset(
        separation_along(normal, penetration, Vector2F::DIRECTION_DOWN));

    this->set_move_state(other_type == CollisionObjectType::structure_ramp_left ?
        PlayerMoveState::on_ramp_left : PlayerMoveState::on_ramp_right);
    this->supported_this_tick_ = true;

    this->support_impact_speed_ = std::max(this->support_impact_speed_,
        this->velocity().y);
    // On a slope the lower supporting corner touches the surface; the centre
    // of the player's bottom edge floats above it.
    const RectangleF surface = other.shape()->bounding_box();
    const float support_x = other_type == CollisionObjectType::structure_ramp_right ?
        this->rectangle_.right() : this->rectangle_.left();
    this->support_position_ = { std::clamp(support_x, surface.left(), surface.right()),
        this->rectangle_.bottom() };

    MovingObject::set_velocity_y(0.0f);
}
void Player::on_jump_through_contact(const CollisionObject& other)
{
    // Land only while moving down (+y) from above the platform on the previous tick.
    const bool moving_down = this->velocity().y > 0.0f;

    const RectangleF other_rect = other.shape()->bounding_box();
    const bool was_above = this->prev_rectangle_.bottom() <= other_rect.top();

    if (moving_down && was_above &&
        this->move_state() != PlayerMoveState::dropping_down)
    {
        this->support_impact_speed_ = std::max(this->support_impact_speed_,
            this->velocity().y);
        MovingObject::set_velocity_y(0.0f);
        this->rectangle_.set_position_y_from_bottom(other_rect.top());
        this->support_position_ = { (std::max(this->rectangle_.left(), other_rect.left()) +
            std::min(this->rectangle_.right(), other_rect.right())) / 2.0f, other_rect.top() };
        this->set_move_state(PlayerMoveState::on_drop_down_ground);
        this->supported_this_tick_ = true;
    }
}

void Player::update_weapon_position() const
{
    this->primary_->set_player_center(this->center());
}
void Player::on_projectile_contact(CollisionObjectType other_type)
{
    // The projectile side is unchanged: paint still lands.
    if (this->damage_immune_)
    {
        return;
    }

    // Damage belongs to the projectile's tuning record.
    const float damage = this->tuning_->projectile(
        to_projectile_type(other_type)).details.player_damage;
    this->health_ -= damage;

    if (damage > 0.0f && this->health_ > 0.0f && this->state_ == PlayerState::alive)
    {
        this->start_action(PlayerAnimationState::hurt);
        this->combat_feedback_.hurt();
    }

    this->health_regen_timer_ = 0.0f;

    if (this->damage_sound_timer_ >= this->player_tuning().damage_sound_delay)
	{
		this->sound_bank_->play_wave(this->damage_sound_, this->player_tuning().damage_sound.volume * this->preferences_->sfx_gain());
		this->damage_sound_timer_ = 0.0f;
	}
}

void Player::update(float dt)
{
    this->paint_supports_.clear();
    this->movement_audio_dt_ = dt;
    this->movement_start_x_ = this->rectangle_.x;
    this->movement_previous_velocity_ = this->velocity().x;
    this->movement_requested_x_ = 0.0f;
    // Age old cues before movement can emit new ones; every new event is drawn
    // on its actual tick at age zero, irrespective of the number of views.
    this->movement_feedback_.update(dt);
    this->combat_feedback_.update(dt);
    this->age_action(dt);
    if (this->state_ == PlayerState::alive && this->health_ <= 0.0f)
    {
        // Damage arrives during contacts, after update. Retire input before the
        // next movement tick so a fatal hit cannot produce a final jump.
        this->state_ = PlayerState::dead;
        this->primary_->reset_motion();
        this->movement_audio_.reset();
        this->movement_feedback_.clear();
        this->had_ground_support_ = false;
        this->landing_feedback_ready_ = false;
        this->reset_jump_input();
        this->set_move_state(PlayerMoveState::in_air);
        this->start_action(PlayerAnimationState::elimination);
        this->combat_feedback_.eliminate(this->rectangle_);
        this->sound_bank_->play_wave(this->death_sound_, this->player_tuning().death_sound.volume * this->preferences_->sfx_gain());
    }

    else if (this->state_ == PlayerState::alive)
    {
        this->update_movement(dt);

        // Select and advance animation during update: draw may be culled or run
        // concurrently for several views.
        this->update_animation_state();

        if (this->action_.has_value())
        {
            this->update_action_frame();
        }
        else
        {
            AnimationObject::update(dt);
            this->clothing_.advance(dt);
        }

        this->damage_sound_timer_ += dt;
    }
    else if (this->state_ == PlayerState::dead)
    {
        this->reset_jump_input();
        this->update_animation_state();
        this->update_action_frame();
        // Frozen/developer probes must not rewind the visible outline across
        // its audio boundary before a later pause/resume.
        this->respawn_timer_ -= std::max(dt, 0.0f);

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

    // Regenerate up to the configured starting health.
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
    this->update_redraw_sound(dt);
}
std::vector<std::unique_ptr<Projectile>>
    Player::update_weapon_and_get_projectiles(float dt,
    std::span<CollisionObject* const> objects)
{
    this->weapon_contact_dt_ = dt;
    // Moving projectile contacts can be fatal before this phase, while state_
    // still says alive. Neither those players nor dead players can act. Stop the
    // loop explicitly because skipping update also skips normal release handling.
    if (this->state_ != PlayerState::alive || this->health_ <= 0.0f)
    {
        this->primary_->stop_sounds();
        this->primary_->reset_motion();
        return {};
    }

    // Before, so the shot is affordable; after, so the HUD shows a full tank.
    if (this->infinite_ammo_)
    {
        this->primary_->reset_ammo(false);
    }
    auto projectiles = this->primary_->update_and_get_projectiles(
        this->input(),
        this->center(),
        MovingObject::velocity(),
        this->facing_right(),
        dt);
    // A paid shot is still a shot when its muzzle immediately contacts cover.
    // Check before launch obstruction retires it, never from held input or the
    // weapon's looping presentation flag (which also spans the cooldown).
    if (dt > 0.0f && !projectiles.empty())
    {
        this->start_action(PlayerAnimationState::firing);
    }
    if (this->infinite_ammo_)
    {
        this->primary_->reset_ammo(false);
    }
    for (const auto& projectile : projectiles)
    {
        projectile->resolve_launch_obstruction(this->center(), objects);
        if (this->paint_audio_ != nullptr)
        {
            projectile->flush_paint_audio(*this->paint_audio_);
        }
    }
    std::erase_if(projectiles, [](const auto& projectile)
        { return projectile->for_deletion(); });
    return projectiles;
}
void Player::set_paint_audio(PaintAudio* audio)
{
    this->paint_audio_ = audio;
}
void Player::update_prev_rectangle()
{
	this->prev_rectangle_ = this->rectangle_;
}

void Player::update_movement(float dt)
{
    this->jump_buffer_remaining_ = std::max(0.0f, this->jump_buffer_remaining_ - dt);
    this->ledge_grace_remaining_ = std::max(0.0f, this->ledge_grace_remaining_ - dt);
    if (this->jump_waiting_for_release_)
    {
        if (!this->input_.jump_held && !this->input_.jump_pressed)
        {
            this->jump_waiting_for_release_ = false;
        }
    }
    else if (this->input_.jump_pressed)
    {
        this->jump_buffer_remaining_ = this->player_tuning().jump_buffer_time;
    }

    const PlayerInputData input = this->input_;
    const float x_input = input.x_movement;
    const PlayerMoveState move_state = this->move_state();
    const bool drop_through = this->input_.drop_through;
    const bool jump_pressed = this->input_.jump_pressed &&
        !this->jump_waiting_for_release_;

    if (are_equal(MovingObject::velocity_x(), 0.0f) && x_input != 0.0f)
    {
        MovingObject::set_velocity_x(this->player_tuning().x_initial_velocity * x_input);
    }
    else if (MovingObject::velocity_x() > 0.0f && x_input > 0.0f)
    {
        MovingObject::alter_velocity_x(this->player_tuning().x_acceleration * dt * x_input);
        if (MovingObject::velocity_x() > this->player_tuning().max_velocity.x)
        {
            MovingObject::set_velocity_x(this->player_tuning().max_velocity.x);
        }
    }
    else if (MovingObject::velocity_x() < 0.0f && x_input < 0.0f)
    {
        MovingObject::alter_velocity_x(this->player_tuning().x_acceleration * dt * x_input);
        if (MovingObject::velocity_x() < -this->player_tuning().max_velocity.x)
        {
            MovingObject::set_velocity_x(-this->player_tuning().max_velocity.x);
        }
    }
    else if ((MovingObject::velocity_x() > 0.0f && x_input < 0.0f) ||
        (MovingObject::velocity_x() < 0.0f && x_input > 0.0f))
    {
        // Opposing input always includes release braking. Small corrections
        // must not make stopping weaker than letting go of the stick.
        const PlayerTuning& tuning = this->player_tuning();
        const float braking = tuning.x_deacceleration +
            tuning.x_acceleration * std::fabs(x_input);
        const float time_to_stop = std::fabs(MovingObject::velocity_x()) / braking;
        if (dt <= time_to_stop)
        {
            MovingObject::alter_velocity_x(std::copysign(braking * dt, x_input));
        }
        else
        {
            // Spend only the remainder accelerating in the new direction;
            // braking itself cannot propel the player past zero.
            MovingObject::set_velocity_x(std::clamp(
                tuning.x_acceleration * x_input * (dt - time_to_stop),
                -tuning.max_velocity.x, tuning.max_velocity.x));
        }
    }
    else if (MovingObject::velocity_x() > 0.0f && are_equal(x_input, 0.0f))
    {
        MovingObject::alter_velocity_x(-this->player_tuning().x_deacceleration * dt);
        if (MovingObject::velocity_x() < 0.0f)
        {
            MovingObject::set_velocity_x(0.0f);
        }
    }
    else if (MovingObject::velocity_x() < 0.0f && are_equal(x_input, 0.0f))
    {
        MovingObject::alter_velocity_x(this->player_tuning().x_deacceleration * dt);
        if (MovingObject::velocity_x() > 0.0f)
        {
            MovingObject::set_velocity_x(0.0f);
        }
    }
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

    // dropping_down lasts for one update.
    if (move_state == PlayerMoveState::dropping_down)
	{
		this->set_move_state(PlayerMoveState::in_air);
	}

    if (move_state == PlayerMoveState::on_drop_down_ground && drop_through &&
        !jump_pressed)
    {
        this->reset_jump_input();
        MovingObject::alter_velocity_y(this->player_tuning().drop_down_velocity);
        this->set_move_state(PlayerMoveState::dropping_down);
    }
	else
	{
        this->do_jump(dt);
	}

    const Vector2F displacement = MovingObject::velocity() * dt;
    this->movement_requested_x_ = displacement.x;
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
    const bool jump_requested = !this->jump_waiting_for_release_ &&
        (this->input_.jump_pressed || this->jump_buffer_remaining_ > 0.0f);
    // A context reset blocks new takeoffs, but pausing an existing jump must
    // preserve its held-button extension when simulation resumes.
    const bool jump_held = this->input_.jump_held;
   
    if ((this->is_on_ground() || this->ledge_grace_remaining_ > 0.0f) &&
        jump_requested)
    {
		this->start_jump();
	}
	else if (move_state == PlayerMoveState::on_ceiling)
	{
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
		else
		{
			this->set_move_state(PlayerMoveState::in_air);
		}
		this->alter_air_time(dt);
	}
	else if (move_state == PlayerMoveState::in_air)
	{
	}

    if (this->velocity_y() < -this->player_tuning().max_velocity.y)
    {
        this->set_velocity_y(-this->player_tuning().max_velocity.y);
    }
}

void Player::start_jump()
{
    this->start_action(PlayerAnimationState::taking_off);
    // A grace jump occurs at the current airborne feet; it does not invent a
    // ground contact at the ledge. Buffered jumps use the resolved support.
    this->movement_feedback_.takeoff(this->had_ground_support_ ?
        this->support_position_ : Vector2F(this->rectangle_.center().x,
            this->rectangle_.bottom()));
    this->had_ground_support_ = false;
    this->jump_buffer_remaining_ = 0.0f;
    this->ledge_grace_remaining_ = 0.0f;
    MovingObject::set_velocity_y(std::max(this->player_tuning().jump_launch_velocity,
        -this->player_tuning().max_velocity.y));
    this->set_air_time(0.0f);
    this->set_move_state(PlayerMoveState::jumping);
    this->sound_bank_->play_wave(this->jump_sound_, this->player_tuning().jump_sound.volume * this->preferences_->sfx_gain());
}

RectangleF Player::bounds() const
{
    const bool held = this->primary_ != nullptr && this->state_ == PlayerState::alive &&
        this->pose_.tool && !this->drawn_whole();
    RectangleF visual = this->attachments_.bounds(this->posed_rectangle(),
        this->facing_right(), held ? this->primary_.get() : nullptr);
    if (held)
    {
        visual = RectangleF::union_of(visual, this->primary_->draw_bounds());
    }
    return RectangleF::union_of(
        this->movement_feedback_.bounds(RectangleF::union_of(this->rectangle_, visual)),
        this->combat_feedback_.bounds(this->posed_rectangle()));
}

RectangleF Player::cull_bounds(float /*units_per_pixel*/) const
{
    if (this->state_ == PlayerState::dead)
    {
        return RectangleF::union_of(this->bounds(),
            this->attachments_.bounds(this->spawn_rectangle(), this->facing_right(), nullptr));
    }
    return this->bounds();
}

const MovementFeedback& Player::movement_feedback() const
{
    return this->movement_feedback_;
}

const CombatFeedback& Player::combat_feedback() const
{
    return this->combat_feedback_;
}

const QuietCharacter& Player::quiet_character() const
{
    return this->quiet_character_;
}

const Weapon& Player::primary_weapon() const
{
    if (this->primary_ == nullptr)
    {
        throw std::runtime_error("Player has no primary weapon");
    }
    return *this->primary_;
}

void Player::set_player_input(const PlayerInputData& input)
{
	this->input_ = input;
}

void Player::reset_jump_input()
{
    this->jump_buffer_remaining_ = 0.0f;
    this->ledge_grace_remaining_ = 0.0f;
    this->jump_waiting_for_release_ = true;
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
    case PlayerAnimationState::taking_off:
        return this->player_tuning().takeoff;
    case PlayerAnimationState::landing:
        return this->player_tuning().land;
    case PlayerAnimationState::firing:
        return this->player_tuning().fire;
    case PlayerAnimationState::hurt:
        return this->player_tuning().hurt;
    case PlayerAnimationState::elimination:
        return this->player_tuning().elimination;
    }

    throw std::runtime_error("Player animation state " +
        std::to_string(static_cast<int>(state)) + " has no clip.");
}
void Player::place_at(const Vector2F& top_left)
{
    this->quiet_character_.reset();
    this->combat_feedback_.clear();
    this->primary_->reset_motion();
    this->action_.reset();
    this->action_age_ = 0.0f;
    this->set_animation_state(PlayerAnimationState::idle);
    this->paint_supports_.clear();
    this->movement_audio_.reset();
    this->movement_audio_dt_ = 0.0f;
    this->movement_feedback_.clear();
    this->had_ground_support_ = false;
    this->landing_feedback_ready_ = false;
    this->support_impact_speed_ = 0.0f;
    this->rectangle_.set_position(top_left);
    this->prev_rectangle_ = this->rectangle_;
    this->reset_jump_input();
    this->set_move_state(PlayerMoveState::in_air);
    this->set_air_time(0.0f);
    this->supported_this_tick_ = false;
    this->hit_ceiling_this_tick_ = false;
    MovingObject::set_rotation(0.0f);
    MovingObject::set_velocity(Vector2F::ZERO);
}

void Player::force_respawn()
{
    this->respawn();
    this->respawn_timer_ = this->player_tuning().respawn_delay;
}

void Player::kill()
{
    this->health_ = 0.0f;
    this->quiet_character_.reset();
    this->primary_->reset_motion();
}

void Player::set_coloured(bool coloured, const Colour& grey)
{
    this->grey_ = coloured ? std::nullopt : std::optional<Colour>(grey);
    this->primary_->set_paint_override(this->grey_);
}

void Player::set_showing_debug(bool showing)
{
    this->showing_debug_ = showing;
}

void Player::set_damage_immune(bool immune)
{
    this->damage_immune_ = immune;
}

void Player::set_infinite_ammo(bool infinite)
{
    this->infinite_ammo_ = infinite;
}

void Player::respawn()
{
    this->stop_redraw_sound();
    this->place_at(this->respawn_position_);
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
    this->stop_redraw_sound();
}
void Player::reset_tool_motion()
{
    this->primary_->reset_motion();
    this->quiet_character_.reset();
}
void Player::pause_sounds() const
{
    this->primary_->pause_sounds();
    if (this->redraw_playing_ && !this->redraw_paused_)
    {
        this->sound_bank_->pause_effect(this->redraw_sound_);
        this->redraw_paused_ = true;
    }
}
void Player::resume_sounds() const
{
    this->primary_->resume_sounds();
    if (this->redraw_playing_)
    {
        if (!this->redrawing())
        {
            this->stop_redraw_sound();
            return;
        }
        this->sound_bank_->set_effect_volume(this->redraw_sound_, this->redraw_gain());
        if (this->redraw_paused_)
        {
            this->sound_bank_->resume_effect(this->redraw_sound_);
            this->redraw_paused_ = false;
        }
    }
}

bool Player::redrawing() const
{
    return this->state_ == PlayerState::dead && !this->drawn_whole() &&
        this->respawn_redraw_.drawing(this->respawn_progress());
}

float Player::redraw_gain() const
{
    return this->player_tuning().redraw_sounds[this->redraw_index_].volume *
        this->preferences_->sfx_gain();
}

void Player::update_redraw_sound(float dt) const
{
    // Drawing can be culled or repeated for several views; only a progressing
    // simulation tick starts the pen, and a long tick never catches up a replay.
    if (dt <= 0.0f)
    {
        return;
    }
    if (!this->redrawing())
    {
        this->stop_redraw_sound();
        return;
    }
    if (!this->redraw_playing_)
    {
        this->sound_bank_->play_effect(this->redraw_sound_, true, this->redraw_gain());
        this->redraw_playing_ = true;
    }
    else
    {
        this->sound_bank_->set_effect_volume(this->redraw_sound_, this->redraw_gain());
    }
}

void Player::stop_redraw_sound() const
{
    if (this->redraw_playing_)
    {
        this->sound_bank_->stop_effect(this->redraw_sound_, true);
        this->redraw_playing_ = false;
        this->redraw_paused_ = false;
    }
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

float Player::respawn_progress() const
{
    // The delay is live tuning: a change mid-wait must not leave 0..1.
    const float delay = this->player_tuning().respawn_delay;
    return delay > 0.0f ?
        std::clamp(1.0f - this->respawn_timer_ / delay, 0.0f, 1.0f) : 1.0f;
}

RectangleF Player::spawn_rectangle() const
{
    return RectangleF(this->respawn_position_, this->rectangle_.size());
}

Vector2F Player::camera_focus() const
{
    if (this->state_ == PlayerState::dead &&
        this->player_tuning().respawn_delay - this->respawn_timer_ >=
            this->respawn_redraw_.camera_hold())
    {
        return this->spawn_rectangle().center();
    }
    return this->center();
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
