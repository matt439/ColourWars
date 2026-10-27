#include "game/pch.h"
#include "game/objects/blocked_shot_feedback.h"
#include "game/objects/player.h"
#include "game/objects/projectile.h"
#include "game/objects/paint_audio.h"
#include "game/objects/structure_paintable.h"
#include "game/objects/fixed_slab_paintable.h"

#include "engine/collision/resolve.h"

#include <limits>

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

namespace
{
    // The authored body is neutral; its matching clothing clip carries team colour.
    const Colour player_tint = Colour::white;

    bool coordinate_at(float actual, float expected, float extent)
    {
        const float tolerance = 4.0f * std::numeric_limits<float>::epsilon() *
            std::max({ 1.0f, std::abs(actual), std::abs(expected), extent });
        return std::abs(actual - expected) <= tolerance;
    }

    bool feet_at(const RectangleF& body, float height)
    {
        return coordinate_at(body.bottom(), height, body.height);
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
    const ViewShakePresentation* view_shake) :
    MovingObject(velocity),
    AnimationObject(tuning->player().idle.sheet,
                    tuning->player().idle.strip,
                    render_resources, player_tint, rotation, origin, flip, layer_depth),
    TextureObject(tuning->player().idle.sheet,
                  tuning->player().idle.uniform_texture,
                  render_resources, team_colour, rotation, origin, flip, layer_depth),
    tuning_(tuning),
    figure_(figure),
    clothing_(tuning->player().idle, render_resources, team_colour,
        rotation, origin, flip, layer_depth),
    attachments_(movement_feedback, figure, render_resources),
    quiet_character_(figure, sound_index >= 0 ? sound_index : std::max(player_num, 0)),
    movement_feedback_(movement_feedback, render_resources),
    combat_feedback_(combat_feedback, render_resources),
    view_shake_(view_shake),
    movement_audio_(&tuning->player().movement_sounds,
        audio_resources->sound_bank(tuning->player().sound_bank),
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
    for (size_t index = 0; index < this->figure_clips_.size(); ++index)
    {
        const PlayerAnimationTuning& clip =
            this->animation_info(static_cast<PlayerAnimationState>(index));
        this->figure_clips_[index] = {
            render_resources->resolve_sheet_frame(clip.sheet, clip.uniform_texture),
            render_resources->resolve_sheet_strip(clip.sheet, clip.strip),
            render_resources->resolve_sheet_strip(clip.sheet, clip.paint_strip) };
    }
    for (const PlayerAnimationState state : { PlayerAnimationState::taking_off,
        PlayerAnimationState::landing, PlayerAnimationState::firing,
        PlayerAnimationState::hurt, PlayerAnimationState::elimination })
    {
        const RenderResources::SheetStrip& body =
            this->figure_clips_[static_cast<size_t>(state)].body;
        const AnimationStrip& strip =
            render_resources->sprite_sheet(body.sheet())->animation_strip(body.strip());
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
        this->on_projectile_contact(other_type, other);
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
    if (other_type == CollisionObjectType::structure_fixed_slab)
    {
        this->on_fixed_slab_contact(dynamic_cast<const FixedSlabPaintable&>(other), normal, penetration);
        return;
    }
    if (is_structure_ramp(other_type))
    {
        this->on_ramp_contact(other_type, other, normal, penetration);
        return;
    }
    this->on_structure_contact(other, normal, penetration);
}
void Player::end_contacts(std::span<const Contact> contacts,
    std::span<const HorizontalSupportRef> supports)
{
    // The new solid slab keeps its own support line; its normal extrusion's
    // AABB is not the line the feet walk on. Legacy ramps retain their path.
    if (this->projected_fixed_slab_support_ && !this->supported_this_tick_ &&
        !this->hit_ceiling_this_tick_ && this->is_on_ramp())
    {
        if (const auto support = this->fixed_slab_support_at(this->rectangle_);
            support && feet_at(this->rectangle_, support->y))
        {
            this->supported_this_tick_ = true;
            this->support_position_ = *support;
        }
    }
    const auto slab_position = this->fixed_slab_support_at(this->rectangle_);
    if (this->state_ != PlayerState::alive || this->health_ <= 0.0f ||
        !this->supported_this_tick_ || !this->is_on_ramp() ||
        !slab_position || !feet_at(this->rectangle_, slab_position->y))
    {
        this->fixed_slab_support_.reset();
    }
    this->projected_fixed_slab_support_ = false;

    // A projected slope can touch without penetrating, so dispatch may have
    // no manifold to resolve. Retain that support only if later wall/ceiling
    // responses left the body on the same surface. Real contacts take priority.
    if (this->projected_ramp_support_ && !this->supported_this_tick_ &&
        !this->hit_ceiling_this_tick_ && this->is_on_ramp())
    {
        if (const auto support = this->ramp_support_at(this->rectangle_);
            support && feet_at(this->rectangle_, support->y))
        {
            this->supported_this_tick_ = true;
            this->support_position_ = *support;
        }
    }
    const auto ramp_position = this->ramp_support_at(this->rectangle_);
    if (this->state_ != PlayerState::alive || this->health_ <= 0.0f ||
        !this->supported_this_tick_ || !this->is_on_ramp() ||
        !ramp_position || !feet_at(this->rectangle_, ramp_position->y))
    {
        this->ramp_support_.reset();
    }
    this->projected_ramp_support_ = false;

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
        // At either endpoint the initial manifold can be horizontal before
        // adjacent ground removes downward movement: entering uphill at the
        // toe, or downhill at the high crest. Recognize that exact endpoint
        // from the final grounded pose, regardless of pair dispatch order.
        if (other != nullptr && this->supported_this_tick_ &&
            this->move_state() == PlayerMoveState::on_ground &&
            !this->hit_ceiling_this_tick_ && this->state_ == PlayerState::alive && this->health_ > 0.0f)
        {
            const CollisionObjectType type = to_collision_type(other->tag());
            if (is_structure_ramp(type))
            {
                const RectangleF ramp = other->shape()->bounding_box();
                const bool rises_right = type == CollisionObjectType::structure_ramp_right;
                const float toe = rises_right ? ramp.left() : ramp.right();
                const float corner = rises_right ? this->rectangle_.right() : this->rectangle_.left();
                const bool uphill = rises_right ? this->movement_requested_x_ > 0.0f :
                    this->movement_requested_x_ < 0.0f;
                const bool at_toe = uphill && feet_at(this->rectangle_, ramp.bottom()) &&
                    coordinate_at(corner, toe, this->rectangle_.width);
                const float crest = rises_right ? ramp.right() : ramp.left();
                const float crest_corner = rises_right ? this->rectangle_.left() :
                    this->rectangle_.right();
                const bool downhill = rises_right ? this->movement_requested_x_ < 0.0f :
                    this->movement_requested_x_ > 0.0f;
                const bool at_crest = downhill && feet_at(this->rectangle_, ramp.top()) &&
                    coordinate_at(crest_corner, crest, this->rectangle_.width);
                if (at_toe || at_crest)
                {
                    this->ramp_support_ = RampSupport{ ramp, type };
                    this->set_move_state(rises_right ? PlayerMoveState::on_ramp_right :
                        PlayerMoveState::on_ramp_left);
                    this->support_position_ = at_toe ? Vector2F(toe, ramp.bottom()) :
                        Vector2F(crest, ramp.top());
                }
            }
            else if (type == CollisionObjectType::structure_fixed_slab)
            {
                const auto& slab = dynamic_cast<const FixedSlabPaintable&>(*other);
                const Vector2F start = slab.geometry().top_start();
                const Vector2F end = slab.geometry().top_end();
                const bool rises_right = end.y < start.y;
                const Vector2F toe = rises_right ? start : end;
                const Vector2F crest = rises_right ? end : start;
                const float toe_corner = rises_right ? this->rectangle_.right() : this->rectangle_.left();
                const bool uphill = rises_right ? this->movement_requested_x_ > 0.0f :
                    this->movement_requested_x_ < 0.0f;
                const bool downhill = rises_right ? this->movement_requested_x_ < 0.0f :
                    this->movement_requested_x_ > 0.0f;
                const bool at_toe = uphill && feet_at(this->rectangle_, toe.y) &&
                    coordinate_at(toe_corner, toe.x, this->rectangle_.width);
                // The adjoining flat may resolve first while the body
                // straddles the crest. Keep that real vertex support until
                // the trailing corner enters the slope, rather than requiring
                // a discrete step to land exactly on the crest's X.
                const bool at_crest = downhill && feet_at(this->rectangle_, crest.y) &&
                    this->rectangle_.left() <= crest.x && this->rectangle_.right() >= crest.x;
                if (at_toe || at_crest)
                {
                    this->remember_fixed_slab_support(slab);
                    this->set_move_state(rises_right ? PlayerMoveState::on_ramp_right :
                        PlayerMoveState::on_ramp_left);
                    this->support_position_ = at_toe ? toe : crest;
                }
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
                if (this->preferences_->view_shake_enabled())
                {
                    this->view_shake_.land(this->support_impact_speed_);
                }
                if (this->support_impact_speed_ >= this->movement_feedback_look_->landing_min_speed)
                {
                    this->pending_rumble_.hard_landing = true;
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
    // Sample after every paint/separation callback and buffered takeoff. The
    // prior step's frozen class has already served movement and ammunition.
    if (!this->player_tuning().paint_underfoot.enabled || !can_act)
    {
        this->underfoot_support_team_ = this->underfoot_step_team_ = PlayerTeam::none;
    }
    else if (this->movement_audio_dt_ > 0.0f)
    {
        this->underfoot_support_team_ = this->gameplay_support_team();
    }
    if (!supports.empty()) { this->choose_carrier(contacts, supports); }
    // Static movement retains its exact original subtraction. Involuntary
    // carry changes world pose, never personal footsteps or tool locomotion.
    const float resolved_x = this->movement_carried_x_ == 0.0f ?
        this->rectangle_.x - this->movement_start_x_ :
        (this->rectangle_.x - this->movement_start_x_) - this->movement_carried_x_;
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

    this->quiet_character_.update(can_act && !this->preferences_->reduce_effects,
        this->supported_this_tick_ && this->is_on_ground(),
        std::abs(this->velocity().x) < 0.01f && std::abs(this->input_.x_movement) < 0.01f &&
            !this->input_.primary_shoot,
        this->action_.has_value(), this->movement_previous_velocity_, this->velocity().x,
        this->weapon_contact_dt_);

    this->supported_this_tick_ = false;
    this->hit_ceiling_this_tick_ = false;
    this->support_impact_speed_ = 0.0f;
    this->paint_supports_.clear();
    this->resolved_flat_supports_.clear();
}

void Player::remember_flat_support(const CollisionObject& other)
{
    if (const auto* support = dynamic_cast<const Structure*>(&other))
    { this->resolved_flat_supports_.push_back(support); }
}

void Player::clear_carrier() noexcept
{
    this->carrier_id_ = 0;
    this->prepared_carrier_.reset();
    this->resolved_flat_supports_.clear();
    // Do not erase this tick's applied carry before feedback subtracts it.
}

HorizontalMotionId Player::carrier_id() const { return this->carrier_id_; }

void Player::prepare_carrier_step(std::optional<HorizontalCarryStep> step) noexcept
{
    if (!step || step->id == 0 || step->id != this->carrier_id_)
    {
        this->clear_carrier();
        return;
    }
    this->prepared_carrier_ = *step;
}

void Player::apply_carrier(float dt)
{
    if (!(dt > 0.0f) || !this->prepared_carrier_) { return; }
    const HorizontalCarryStep step = *this->prepared_carrier_;
    this->prepared_carrier_.reset();
    const bool flat = this->move_state() == PlayerMoveState::on_ground ||
        this->move_state() == PlayerMoveState::on_drop_down_ground;
    const float overlap = std::min(this->rectangle_.right(), step.previous_top.right()) -
        std::max(this->rectangle_.left(), step.previous_top.left());
    if (this->state_ != PlayerState::alive || this->health_ <= 0.0f || !flat ||
        step.id != this->carrier_id_ || !(overlap > 0.0f) ||
        !feet_at(this->rectangle_, step.previous_top.top()))
    {
        this->clear_carrier();
        return;
    }
    const float before = this->rectangle_.x;
    this->rectangle_.offset(step.displacement_x, 0.0f);
    this->movement_carried_x_ = this->rectangle_.x - before;
}

void Player::choose_carrier(std::span<const Contact> contacts,
    std::span<const HorizontalSupportRef> supports)
{
    const bool flat = this->move_state() == PlayerMoveState::on_ground ||
        this->move_state() == PlayerMoveState::on_drop_down_ground;
    if (this->state_ != PlayerState::alive || this->health_ <= 0.0f ||
        !flat || !this->supported_this_tick_)
    {
        this->clear_carrier();
        return;
    }
    const HorizontalSupportRef* best = nullptr;
    float best_overlap = -1.0f;
    for (const HorizontalSupportRef& candidate : supports)
    {
        if (candidate.surface == nullptr) { continue; }
        const CollisionObjectType type = to_collision_type(candidate.surface->tag());
        if (type != CollisionObjectType::structure &&
            type != CollisionObjectType::structure_paintable &&
            type != CollisionObjectType::structure_jump_through) { continue; }
        bool measured_top = false;
        for (const Contact& contact : contacts)
        {
            Vector2F normal;
            const CollisionObject* other = nullptr;
            if (contact.a == this) { other = contact.b; normal = contact.normal; }
            else if (contact.b == this) { other = contact.a; normal = -contact.normal; }
            if (other == candidate.surface && normal.y > 0.0f &&
                std::abs(normal.y) >= std::abs(normal.x)) { measured_top = true; break; }
        }
        const bool resolved_top = std::find(this->resolved_flat_supports_.begin(),
            this->resolved_flat_supports_.end(), candidate.surface) != this->resolved_flat_supports_.end();
        if (!measured_top && !resolved_top) { continue; }
        const RectangleF surface = candidate.surface->shape()->bounding_box();
        const float overlap = std::min(this->rectangle_.right(), surface.right()) -
            std::max(this->rectangle_.left(), surface.left());
        if (!(overlap > 0.0f) || !feet_at(this->rectangle_, surface.top())) { continue; }
        if (candidate.motion != 0 && candidate.motion == this->carrier_id_)
        {
            this->prepared_carrier_.reset();
            return; // Still valid established carrier wins before overlap ties.
        }
        const bool static_first = best != nullptr && candidate.motion == 0 && best->motion != 0;
        const bool same_kind = best != nullptr && (candidate.motion == 0) == (best->motion == 0);
        if (best == nullptr || overlap > best_overlap ||
            (overlap == best_overlap && (static_first || (same_kind && candidate.order < best->order))))
        {
            best = &candidate;
            best_overlap = overlap;
        }
    }
    this->carrier_id_ = best == nullptr ? 0 : best->motion;
    this->prepared_carrier_.reset();
}

PlayerTeam Player::gameplay_support_team() const
{
    if (this->fixed_slab_support_ && this->is_on_ramp() &&
        this->state_ == PlayerState::alive && this->health_ > 0.0f && this->supported_this_tick_)
    {
        if (const auto point = this->fixed_slab_support_at(this->rectangle_);
            point && feet_at(this->rectangle_, point->y))
        {
            return this->fixed_slab_support_->surface->top_paint_team(point->x);
        }
    }
    const bool flat_ground = this->move_state() == PlayerMoveState::on_ground ||
        this->move_state() == PlayerMoveState::on_drop_down_ground;
    if (this->state_ != PlayerState::alive || this->health_ <= 0.0f ||
        !this->supported_this_tick_ || !flat_ground) { return PlayerTeam::none; }
    const float foot_x = this->rectangle_.center().x;
    PlayerTeam answer = PlayerTeam::none;
    for (std::size_t index = 0; index < this->paint_supports_.size(); ++index)
    {
        const StructurePaintable* support = this->paint_supports_[index];
        bool duplicate = false;
        for (std::size_t previous = 0; previous < index; ++previous)
        { duplicate |= this->paint_supports_[previous] == support; }
        if (duplicate) { continue; }
        const RectangleF surface = support->shape()->bounding_box();
        if (!(foot_x >= surface.left() && foot_x < surface.right()) ||
            !feet_at(this->rectangle_, surface.top())) { continue; }
        const PlayerTeam team = support->top_paint_team(foot_x);
        if (team == PlayerTeam::none) { continue; }
        if (answer != PlayerTeam::none && answer != team) { return PlayerTeam::none; }
        answer = team;
    }
    return answer;
}

PlayerPaintUnderfootState Player::paint_underfoot_state() const
{
    if (!this->player_tuning().paint_underfoot.enabled) { return {}; }
    return { true, this->underfoot_support_team_, this->underfoot_step_team_ };
}

float Player::underfoot_speed_multiplier() const
{
    const PaintUnderfootTuning& effect = this->player_tuning().paint_underfoot;
    if (!effect.enabled || this->underfoot_step_team_ == PlayerTeam::none ||
        this->state_ != PlayerState::alive || this->health_ <= 0.0f) { return 1.0f; }
    return this->underfoot_step_team_ == this->team_ ? effect.own_speed_multiplier : effect.enemy_speed_multiplier;
}

float Player::underfoot_ammo_multiplier() const
{
    const PaintUnderfootTuning& effect = this->player_tuning().paint_underfoot;
    if (!effect.enabled || this->underfoot_step_team_ == PlayerTeam::none ||
        this->state_ != PlayerState::alive || this->health_ <= 0.0f) { return 1.0f; }
    return this->underfoot_step_team_ == this->team_ ? effect.own_ammo_regen_multiplier : effect.enemy_ammo_regen_multiplier;
}
float Player::ammo_regen_multiplier() const
{
    return this->underfoot_ammo_multiplier() *
        this->tuning_->handicap(this->handicap_).paint_refill;
}

int Player::handicap() const
{
    return this->handicap_;
}

void Player::set_handicap(int level)
{
    if (!handicaps::valid(level))
    {
        throw std::invalid_argument("Handicap level must be none, +1 or +2");
    }
    this->handicap_ = level;
}

std::optional<Player::PaintedSupport> Player::painted_support() const
{
    if (this->fixed_slab_support_ && this->is_on_ramp() && this->supported_this_tick_)
    {
        if (const auto point = this->fixed_slab_support_at(this->rectangle_);
            point && feet_at(this->rectangle_, point->y))
        {
            if (const auto colour = this->fixed_slab_support_->surface->top_paint_colour(point->x))
            {
                return PaintedSupport{ *colour, *point };
            }
        }
    }
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
    if (this->projected_fixed_slab_support_ && normal.y < 0.0f &&
        std::abs(normal.y) >= std::abs(normal.x))
    {
        if (const auto support = this->fixed_slab_support_at(this->prev_rectangle_);
            support && feet_at(this->prev_rectangle_, support->y))
        {
            const bool rises_right = this->fixed_slab_support_->end.y < this->fixed_slab_support_->start.y;
            this->rectangle_ = this->prev_rectangle_;
            MovingObject::set_velocity_x(0.0f);
            MovingObject::set_velocity_y(0.0f);
            this->support_position_ = *support;
            this->supported_this_tick_ = true;
            this->hit_ceiling_this_tick_ = true;
            this->set_move_state(rises_right ? PlayerMoveState::on_ramp_right : PlayerMoveState::on_ramp_left);
            this->projected_fixed_slab_support_ = false;
            // The restored pose still has its real fixed-lifetime support.
            // Retain the line so held uphill input rejects the same blocked
            // step next tick instead of falling back into the slab/ceiling.
            return;
        }
    }
    // A grounded slope step rises with the ramp. If that rise meets a
    // ceiling, separating downward would put the feet inside the same ramp.
    // Reject the attempted step at its previous supported pose instead. A
    // jump has already cleared projection, so its ordinary head bonk remains.
    if (this->projected_ramp_support_ && normal.y < 0.0f &&
        std::abs(normal.y) >= std::abs(normal.x))
    {
        if (const auto support = this->ramp_support_at(this->prev_rectangle_);
            support && feet_at(this->prev_rectangle_, support->y))
        {
            this->rectangle_ = this->prev_rectangle_;
            MovingObject::set_velocity_x(0.0f);
            MovingObject::set_velocity_y(0.0f);
            this->support_position_ = *support;
            this->supported_this_tick_ = true;
            this->hit_ceiling_this_tick_ = true;
            this->set_move_state(this->ramp_support_->type ==
                CollisionObjectType::structure_ramp_right ?
                PlayerMoveState::on_ramp_right : PlayerMoveState::on_ramp_left);
            return;
        }
    }
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
        if (normal.y > 0.0f) { this->remember_flat_support(other); }
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

    this->fixed_slab_support_.reset();
    this->projected_fixed_slab_support_ = false;
    this->set_move_state(other_type == CollisionObjectType::structure_ramp_left ?
        PlayerMoveState::on_ramp_left : PlayerMoveState::on_ramp_right);
    this->supported_this_tick_ = true;

    this->support_impact_speed_ = std::max(this->support_impact_speed_,
        this->velocity().y);
    // On a slope the lower supporting corner touches the surface; the centre
    // of the player's bottom edge floats above it.
    const RectangleF surface = other.shape()->bounding_box();
    this->ramp_support_ = RampSupport{ surface, other_type };
    const float support_x = other_type == CollisionObjectType::structure_ramp_right ?
        this->rectangle_.right() : this->rectangle_.left();
    this->support_position_ = { std::clamp(support_x, surface.left(), surface.right()),
        this->rectangle_.bottom() };

    MovingObject::set_velocity_y(0.0f);
}

void Player::remember_fixed_slab_support(const FixedSlabPaintable& slab)
{
    this->ramp_support_.reset();
    this->projected_ramp_support_ = false;
    this->fixed_slab_support_ = FixedSlabSupport{
        slab.geometry().top_start(), slab.geometry().top_end(), &slab };
    this->clear_carrier();
}

void Player::on_fixed_slab_contact(const FixedSlabPaintable& slab,
    const Vector2F& normal, float penetration)
{
    const Vector2F inward = slab.geometry().inward_normal();
    const float alignment = normal.x * inward.x + normal.y * inward.y;
    // SAT may select world Y at a top endpoint. Both that normal and the
    // slab's top normal qualify; either end cap and the underside do not.
    const bool top_contact = normal.y > 0.0f &&
        std::abs(normal.y) >= std::abs(normal.x) &&
        alignment >= inward.y - 4.0f * std::numeric_limits<float>::epsilon();
    const Vector2F start = slab.geometry().top_start();
    const Vector2F end = slab.geometry().top_end();
    if (!top_contact || this->rectangle_.right() < start.x || this->rectangle_.left() > end.x)
    {
        this->on_structure_contact(slab, normal, penetration);
        return;
    }

    this->rectangle_.offset(separation_along(normal, penetration, Vector2F::DIRECTION_DOWN));
    this->remember_fixed_slab_support(slab);
    const bool rises_right = end.y < start.y;
    this->set_move_state(rises_right ? PlayerMoveState::on_ramp_right : PlayerMoveState::on_ramp_left);
    this->supported_this_tick_ = true;
    this->support_impact_speed_ = std::max(this->support_impact_speed_, this->velocity().y);
    if (const auto point = this->fixed_slab_support_at(this->rectangle_))
    {
        this->support_position_ = *point;
    }
    MovingObject::set_velocity_y(0.0f);
}

std::optional<Vector2F> Player::fixed_slab_support_at(const RectangleF& body) const
{
    if (!this->fixed_slab_support_) { return std::nullopt; }
    const FixedSlabSupport& slab = *this->fixed_slab_support_;
    if (body.right() < slab.start.x || body.left() > slab.end.x) { return std::nullopt; }
    const bool rises_right = slab.end.y < slab.start.y;
    const float x = std::clamp(rises_right ? body.right() : body.left(), slab.start.x, slab.end.x);
    const float along = (x - slab.start.x) / (slab.end.x - slab.start.x);
    return Vector2F(x, slab.start.y + along * (slab.end.y - slab.start.y));
}

std::optional<Vector2F> Player::ramp_support_at(const RectangleF& body) const
{
    if (!this->ramp_support_) { return std::nullopt; }
    const RampSupport& ramp = *this->ramp_support_;
    if (body.right() < ramp.bounds.left() || body.left() > ramp.bounds.right())
    {
        return std::nullopt;
    }
    const bool rises_right = ramp.type == CollisionObjectType::structure_ramp_right;
    const float x = std::clamp(rises_right ? body.right() : body.left(),
        ramp.bounds.left(), ramp.bounds.right());
    const float along = (x - ramp.bounds.left()) / ramp.bounds.width;
    return Vector2F(x, rises_right ? ramp.bounds.bottom() - along * ramp.bounds.height :
        ramp.bounds.top() + along * ramp.bounds.height);
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
        this->remember_flat_support(other);
    }
}

void Player::update_weapon_position() const
{
    this->primary_->set_player_center(this->center());
}
void Player::on_projectile_contact(CollisionObjectType other_type,
    const CollisionObject& source)
{
    // The projectile side is unchanged: paint still lands.
    if (this->damage_immune_)
    {
        return;
    }

    // Damage belongs to the projectile's tuning record.
    const float damage = this->tuning_->projectile(
        to_projectile_type(other_type)).details.player_damage *
        this->tuning_->handicap(this->handicap_).damage_taken;
    const bool vulnerable = this->state_ == PlayerState::alive && this->health_ > 0.0f;
    this->health_ -= damage;
    if (this->health_ <= 0.0f)
    {
        this->underfoot_support_team_ = this->underfoot_step_team_ = PlayerTeam::none;
        this->clear_carrier();
    }
    if (vulnerable && this->health_ <= 0.0f)
    {
        // Capture the fatal contact now: the dead state arrives next update.
        // Later hits cannot replace credit. This metadata changes no damage,
        // contact, feedback, timer, or random-number decision.
        const auto* projectile = dynamic_cast<const Projectile*>(&source);
        const CollisionLayer enemy_layer = this->team_ == PlayerTeam::a ?
            collision_layers::PROJECTILE_TEAM_B : this->team_ == PlayerTeam::b ?
            collision_layers::PROJECTILE_TEAM_A : CollisionLayer{ 0 };
        const int owner = projectile != nullptr ? projectile->player_num() : -1;
        if (projectile != nullptr && enemy_layer != 0 && source.layer() == enemy_layer &&
            owner >= 0 && owner < paint_owners::count)
        {
            ++this->lethal_projectiles_.enemy_by_owner[static_cast<size_t>(owner)];
        }
        else { ++this->lethal_projectiles_.uncredited; }
    }

    if (damage > 0.0f && this->health_ > 0.0f && this->state_ == PlayerState::alive)
    {
        this->pending_rumble_.hit = true;
        this->start_action(PlayerAnimationState::hurt);
        this->combat_feedback_.hurt();
        if (this->preferences_->view_shake_enabled()) { this->view_shake_.hurt(); }
    }

    this->health_regen_timer_ = 0.0f;

    if (this->damage_sound_timer_ >= this->player_tuning().damage_sound_delay)
	{
		this->sound_bank_->play_wave(this->damage_sound_, this->player_tuning().damage_sound.volume);
		this->damage_sound_timer_ = 0.0f;
	}
}

void Player::update(float dt)
{
    // One completed contact snapshot feeds movement and the later weapon call.
    // dt0 does not refresh terrain context; the disabled path always clears it.
    if (!this->player_tuning().paint_underfoot.enabled ||
        this->state_ != PlayerState::alive || this->health_ <= 0.0f)
    {
        this->underfoot_support_team_ = this->underfoot_step_team_ = PlayerTeam::none;
    }
    else if (dt > 0.0f)
    {
        const bool flat_ground = this->move_state() == PlayerMoveState::on_ground ||
            this->move_state() == PlayerMoveState::on_drop_down_ground;
        const bool fixed_slab_ground = this->is_on_ramp() && this->fixed_slab_support_.has_value();
        this->underfoot_step_team_ = flat_ground || fixed_slab_ground ?
            this->underfoot_support_team_ : PlayerTeam::none;
        this->underfoot_support_team_ = PlayerTeam::none;
    }
    this->projected_ramp_support_ = false;
    this->projected_fixed_slab_support_ = false;
    this->paint_supports_.clear();
    this->resolved_flat_supports_.clear();
    this->movement_audio_dt_ = dt;
    this->movement_start_x_ = this->rectangle_.x;
    this->movement_carried_x_ = 0.0f;
    this->movement_previous_velocity_ = this->velocity().x;
    this->movement_requested_x_ = 0.0f;
    // Age old cues before movement can emit new ones; every new event is drawn
    // on its actual tick at age zero, irrespective of the number of views.
    this->movement_feedback_.update(dt);
    this->combat_feedback_.update(dt);
    this->view_shake_.update(dt, this->preferences_->view_shake_enabled());
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
        this->ramp_support_.reset();
        this->fixed_slab_support_.reset();
        this->reset_jump_input();
        this->set_move_state(PlayerMoveState::in_air);
        this->start_action(PlayerAnimationState::elimination);
        this->pending_rumble_.elimination = true;
        this->combat_feedback_.eliminate(this->rectangle_);
        if (this->preferences_->view_shake_enabled()) { this->view_shake_.eliminate(); }
        this->sound_bank_->play_wave(this->death_sound_, this->player_tuning().death_sound.volume);
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
        dt, this->ammo_regen_multiplier());
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
        if (const auto contact = projectile->take_blocked_contact();
            contact && this->blocked_shot_feedback_ != nullptr)
        {
            this->blocked_shot_feedback_->emit(*contact);
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
void Player::set_blocked_shot_feedback(BlockedShotFeedback* feedback)
{
    this->blocked_shot_feedback_ = feedback;
}
void Player::update_prev_rectangle()
{
	this->prev_rectangle_ = this->rectangle_;
}

void Player::update_movement(float dt)
{
    const PlayerTuning& base = this->player_tuning();
    const float multiplier = this->underfoot_speed_multiplier();
    // Neutral/off copies the old operands without an extra multiply by one.
    const float initial = multiplier == 1.0f ? base.x_initial_velocity : base.x_initial_velocity * multiplier;
    const float acceleration = multiplier == 1.0f ? base.x_acceleration : base.x_acceleration * multiplier;
    const float braking_rate = multiplier == 1.0f ? base.x_deacceleration : base.x_deacceleration * multiplier;
    const float cap = multiplier == 1.0f ? base.max_velocity.x : base.max_velocity.x * multiplier;
    const auto previous_ramp_position = this->ramp_support_at(this->rectangle_);
    const bool on_previous_ramp = this->is_on_ramp() && previous_ramp_position &&
        feet_at(this->rectangle_, previous_ramp_position->y);
    const auto previous_slab_position = this->fixed_slab_support_at(this->rectangle_);
    const bool on_previous_slab = this->is_on_ramp() && previous_slab_position &&
        feet_at(this->rectangle_, previous_slab_position->y);
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
        MovingObject::set_velocity_x(initial * x_input);
    }
    else if (MovingObject::velocity_x() > 0.0f && x_input > 0.0f)
    {
        MovingObject::alter_velocity_x(acceleration * dt * x_input);
        if (MovingObject::velocity_x() > cap)
        {
            MovingObject::set_velocity_x(cap);
        }
    }
    else if (MovingObject::velocity_x() < 0.0f && x_input < 0.0f)
    {
        MovingObject::alter_velocity_x(acceleration * dt * x_input);
        if (MovingObject::velocity_x() < -cap)
        {
            MovingObject::set_velocity_x(-cap);
        }
    }
    else if ((MovingObject::velocity_x() > 0.0f && x_input < 0.0f) ||
        (MovingObject::velocity_x() < 0.0f && x_input > 0.0f))
    {
        // Opposing input always includes release braking. Small corrections
        // must not make stopping weaker than letting go of the stick.

        const float braking = braking_rate +
            acceleration * std::fabs(x_input);
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
                acceleration * x_input * (dt - time_to_stop),
                -cap, cap));
        }
    }
    else if (MovingObject::velocity_x() > 0.0f && are_equal(x_input, 0.0f))
    {
        MovingObject::alter_velocity_x(-braking_rate * dt);
        if (MovingObject::velocity_x() < 0.0f)
        {
            MovingObject::set_velocity_x(0.0f);
        }
    }
    else if (MovingObject::velocity_x() < 0.0f && are_equal(x_input, 0.0f))
    {
        MovingObject::alter_velocity_x(braking_rate * dt);
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

    // Accepted jump/drop has already released support. A rider receives the
    // platform's world displacement before its own unchanged integration.
    this->apply_carrier(dt);
    const Vector2F displacement = MovingObject::velocity() * dt;
    this->movement_requested_x_ = displacement.x;
    this->rectangle_.offset(displacement.x, displacement.y);

    // Gravity keeps ordinary contacts supplied with downward movement, but
    // must not sink a supported slope walker below an adjoining flat crest.
    // Follow only the ramp already under the body, including its endpoint
    // while the footprint still overlaps it. This cannot climb a new step.
    // do_jump and the one-way drop transition have already changed the move
    // state when support is intentionally left. Down alone does not leave a
    // solid ramp, so it must not disable its grounded surface constraint.
    if (dt > 0.0f && on_previous_ramp && this->is_on_ramp())
    {
        if (const auto support = this->ramp_support_at(this->rectangle_))
        {
            this->rectangle_.set_position_y_from_bottom(support->y);
            MovingObject::set_velocity_y(0.0f);
            this->projected_ramp_support_ = true;
        }
    }
    if (dt > 0.0f && on_previous_slab && this->is_on_ramp())
    {
        if (const auto support = this->fixed_slab_support_at(this->rectangle_))
        {
            this->rectangle_.set_position_y_from_bottom(support->y);
            MovingObject::set_velocity_y(0.0f);
            this->projected_fixed_slab_support_ = true;
        }
    }

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
    this->clear_carrier();
    this->ramp_support_.reset();
    this->fixed_slab_support_.reset();
    this->projected_ramp_support_ = false;
    this->projected_fixed_slab_support_ = false;
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
    this->sound_bank_->play_wave(this->jump_sound_, this->player_tuning().jump_sound.volume);
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

const ViewShake& Player::view_shake() const { return this->view_shake_; }

void Player::clear_view_shake() { this->view_shake_.clear(); }

void Player::reset_jump_input()
{
    this->clear_carrier();
    this->jump_buffer_remaining_ = 0.0f;
    this->ledge_grace_remaining_ = 0.0f;
    this->jump_waiting_for_release_ = true;
}

void Player::observe_jump_release(bool held)
{
    if (!held) { this->jump_waiting_for_release_ = false; }
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

void Player::place_at(const Vector2F& top_left)
{
    this->clear_rumble_events();
    this->clear_carrier();
    this->underfoot_support_team_ = this->underfoot_step_team_ = PlayerTeam::none;
    this->view_shake_.clear();
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
    this->ramp_support_.reset();
    this->fixed_slab_support_.reset();
    this->projected_ramp_support_ = false;
    this->projected_fixed_slab_support_ = false;
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

RumbleEvents Player::take_rumble_events() noexcept
{
    RumbleEvents result = this->pending_rumble_;
    result.merge(this->primary_->take_rumble_events());
    this->pending_rumble_ = {};
    return result;
}

void Player::clear_rumble_events() noexcept
{
    this->pending_rumble_ = {};
    this->primary_->clear_rumble_events();
}

void Player::force_respawn()
{
    this->respawn();
    this->respawn_timer_ = this->player_tuning().respawn_delay;
}

void Player::kill()
{
    this->clear_carrier();
    this->fixed_slab_support_.reset();
    this->projected_fixed_slab_support_ = false;
    this->health_ = 0.0f;
    this->underfoot_support_team_ = this->underfoot_step_team_ = PlayerTeam::none;
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
    this->sound_bank_->play_wave(this->respawn_sound_, this->player_tuning().respawn_sound.volume);
}
void Player::stop_sounds() const
{
    this->primary_->stop_sounds();
    this->stop_redraw_sound();
}
void Player::reset_tool_motion()
{
    this->view_shake_.clear();
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

float Player::redraw_volume() const
{
    return this->player_tuning().redraw_sounds[this->redraw_index_].volume;
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
        this->sound_bank_->play_effect(this->redraw_sound_, true, this->redraw_volume());
        this->redraw_playing_ = true;
    }
    else
    {
        this->sound_bank_->set_effect_volume(this->redraw_sound_, this->redraw_volume());
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

const PlayerLethalProjectiles& Player::lethal_projectiles() const
{
    return this->lethal_projectiles_;
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
