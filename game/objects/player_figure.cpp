#include "game/pch.h"
#include "game/objects/player.h"

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
    // Setting off mid-shot hands the legs straight back to locomotion.
    if (this->action_ == PlayerAnimationState::firing && this->state_ == PlayerState::alive &&
        this->calculate_animation_state() != PlayerAnimationState::idle)
    {
        this->action_.reset();
        this->action_age_ = 0.0f;
    }
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

    // The clips were resolved at construction; frame_time stays live tuning.
    const PlayerAnimationTuning& info = this->animation_info(new_animation_state);
    const FigureClip& clip = this->figure_clips_[static_cast<size_t>(new_animation_state)];
    TextureObject::set_frame(clip.uniform);

    set_animation_strip_and_reset(clip.body);
    this->clothing_.select(clip.paint, info.frame_time);
    colourwars::AnimationObject::play();
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
    // The firing face only takes a standing body: walking, running and air
    // keep their own legs, which the tool's motion already answers.
    if (state == PlayerAnimationState::firing &&
        this->calculate_animation_state() != PlayerAnimationState::idle)
    {
        return;
    }
    if (this->action_.has_value())
    {
        if (state == PlayerAnimationState::firing && this->action_ == state)
        {
            // Sustained fire holds the recovery frame rather than flicking
            // back to the brace on every shot.
            const ActionClip clip = this->action_clip(state);
            this->action_age_ = std::min(this->action_age_,
                clip.frame_time * static_cast<float>(clip.frames - 1));
            this->update_action_frame();
            return;
        }
        // New contact/jump/hurt events can interrupt lower poses.
        if (action_priority(state) < action_priority(*this->action_))
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
    colourwars::AnimationObject::set_frame_index(frame);
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
    return this->can_act() &&
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
    return this->can_act() &&
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
        const SpriteSheet* sheet = colourwars::AnimationObject::render_resources()->sprite_sheet(this->idle_sheet_);
        sheet->draw(draw_list, this->idle_paint_, rectangle,
            this->grey_.value_or(this->TextureObject::colour()),
            colourwars::AnimationObject::draw_rotation(), colourwars::AnimationObject::origin(), flip,
            colourwars::AnimationObject::layer_depth());
        this->draw_body(draw_list, this->idle_body_, rectangle, this->face_band(true), flip);
    }
    else
    {
        this->clothing_.draw(draw_list, rectangle,
            this->grey_.value_or(this->TextureObject::colour()), flip);
        this->draw_body(draw_list, colourwars::AnimationObject::source_rectangle(), rectangle,
            this->face_band(false), flip);
        if (!this->preferences_->reduce_effects && this->quiet_character_.state().blinking &&
            this->state_ == PlayerState::alive &&
            this->pose_.tool && !this->pose_.whole)
        {
            if (this->person_.slipped() && this->figure_ != nullptr && this->respawn_look_ != nullptr)
            {
                this->attachments_.draw_blink(draw_list, rectangle, this->facing_right(),
                    this->person_.blink_eyes(*this->figure_, *this->respawn_look_));
            }
            else
            {
                this->attachments_.draw_blink(draw_list, rectangle, this->facing_right());
            }
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
    if (this->person_.slipped())
    {
        this->respawn_redraw_.draw(draw_list, this->figure_canvas(this->spawn_rectangle()),
            this->facing_right(), progress, this->drawn_strokes());
        return;
    }
    this->respawn_redraw_.draw(draw_list, this->figure_canvas(this->spawn_rectangle()),
        this->facing_right(), progress);
}

const FigurePerson& Player::person() const { return this->person_; }

std::vector<RespawnStroke> Player::drawn_strokes() const
{
    if (this->respawn_look_ == nullptr)
    {
        return {};
    }
    if (!this->person_.slipped() || this->figure_ == nullptr)
    {
        return this->respawn_look_->strokes;
    }
    return this->person_.strokes(*this->respawn_look_, this->figure_->slips);
}

const RectangleI* Player::face_band(bool idle) const
{
    const auto state = idle ? PlayerAnimationState::idle : this->animation_state_;
    const std::vector<RectangleI>& bands = this->face_bands_[static_cast<size_t>(state)];
    if (bands.empty())
    {
        return nullptr;
    }
    const int frame = idle || !this->action_.has_value() ? 0 : this->action_frame();
    return &bands[std::min(static_cast<size_t>(std::max(frame, 0)), bands.size() - 1)];
}

void Player::draw_body(DrawList& draw_list, const RectangleI& source, const RectangleF& canvas,
    const RectangleI* band, SpriteFlip flip) const
{
    const SpriteSheet* sheet = colourwars::AnimationObject::sprite_sheet();
    const Colour colour = this->colourwars::AnimationObject::colour();
    const float rotation = colourwars::AnimationObject::draw_rotation();
    const Vector2F origin = colourwars::AnimationObject::origin();
    const float depth = colourwars::AnimationObject::layer_depth();
    if (band == nullptr)
    {
        sheet->draw(draw_list, source, canvas, colour, rotation, origin, flip, depth);
        return;
    }
    // The person's face replaces the frame's own between the band's rows: the
    // frame above and below it, then the band. The figure is drawn unrotated
    // from its corner, and the band spans the same texels facing either way,
    // so the three pieces meet exactly where the one frame would have been.
    const RectangleF& texels = this->face_band_texels_;
    const float scale_x = canvas.width / static_cast<float>(source.width);
    const float scale_y = canvas.height / static_cast<float>(source.height);
    const int top = static_cast<int>(texels.y);
    const int bottom = static_cast<int>(texels.bottom());
    sheet->draw(draw_list, RectangleI(source.x, source.y, source.width, top),
        RectangleF(canvas.x, canvas.y, canvas.width, top * scale_y),
        colour, rotation, origin, flip, depth);
    sheet->draw(draw_list, RectangleI(source.x, source.y + bottom, source.width, source.height - bottom),
        RectangleF(canvas.x, canvas.y + bottom * scale_y, canvas.width, (source.height - bottom) * scale_y),
        colour, rotation, origin, flip, depth);
    sheet->draw(draw_list, *band,
        RectangleF(canvas.x + texels.x * scale_x, canvas.y + texels.y * scale_y,
            texels.width * scale_x, texels.height * scale_y),
        colour, rotation, origin, flip, depth);
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
    if (this->grounded())
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
		throw std::logic_error("Player has no animation for move state " +
			std::to_string(static_cast<int>(move_state)));
	}
}
