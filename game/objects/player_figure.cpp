#include "game/pch.h"
#include "game/objects/player.h"

using namespace mattmath;
using namespace labrador;

// Player's figure: animation clip and action selection, drawing, and the
// visual bounds. The tick, contacts and movement are in player.cpp.

namespace
{
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
        const RectangleF canvas = this->figure_canvas(this->posed_rectangle());
        this->attachments_.draw_arms(draw_list, canvas, this->facing_right(), *this->primary_);
        this->attachments_.draw_sleeves(draw_list, canvas, this->facing_right(),
            this->grey_.value_or(this->TextureObject::colour()));
        this->primary_->draw(draw_list, this->showing_debug_);
        this->attachments_.draw_hands(draw_list, canvas, *this->primary_);
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
        this->pose_.tool && !this->pose_.whole && !this->preferences_->reduce_effects ?
        this->quiet_character_.rectangle(posed) : posed;
}

RectangleF Player::figure_canvas(const RectangleF& body) const
{
    return this->figure_ != nullptr ? this->figure_->canvas_rectangle(body) : body;
}

bool Player::drawn_whole() const
{
    return this->state_ == PlayerState::dead && this->pose_.whole;
}

bool Player::aim_marker_allowed() const
{
    return this->state_ == PlayerState::alive && this->health_ > 0.0f &&
        this->pose_.tool && !this->pose_.whole && !this->grey_.has_value();
}

void Player::set_pose(const FigurePose& pose)
{
    this->pose_ = pose;
    if (this->drawn_whole())
    {
        this->stop_redraw_sound();
    }
}

void Player::draw_figure(DrawList& draw_list, const RectangleF& body, bool idle) const
{
    const RectangleF rectangle = this->figure_canvas(body);
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
        if (!this->preferences_->reduce_effects && this->quiet_character_.state().blinking &&
            this->state_ == PlayerState::alive &&
            this->pose_.tool && !this->pose_.whole)
        {
            this->attachments_.draw_blink(draw_list, rectangle, this->facing_right());
        }
    }
    if (!this->pose_.tool || this->state_ == PlayerState::dead)
    {
        this->attachments_.draw_rest_arms(draw_list, rectangle, this->facing_right(),
            this->grey_.value_or(this->TextureObject::colour()));
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
    this->respawn_redraw_.draw(draw_list, this->figure_canvas(this->spawn_rectangle()),
        this->facing_right(), progress);
}

RectangleF Player::bounds() const
{
    const bool held = this->primary_ != nullptr && this->state_ == PlayerState::alive &&
        this->pose_.tool && !this->drawn_whole();
    RectangleF visual = this->attachments_.bounds(this->figure_canvas(this->posed_rectangle()),
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
            this->attachments_.bounds(this->figure_canvas(this->spawn_rectangle()),
                this->facing_right(), nullptr));
    }
    return this->bounds();
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
