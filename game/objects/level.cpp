#include "game/pch.h"
#include "game/objects/level.h"
#include "game/input_slots.h"
#include "game/objects/structure.h"
#include "game/objects/translating_structure_paintable.h"
#include "game/simulation.h"
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace mattmath;
using namespace DirectX;
using namespace labrador;


namespace
{
    struct HorizontalBox
    {
        double left, top, right, bottom;
    };

    double horizontal_finite(double value)
    {
        if (!std::isfinite(value) || std::abs(value) >
            static_cast<double>((std::numeric_limits<float>::max)()))
        {
            throw std::invalid_argument("Horizontal motion requires finite representable arithmetic and edges.");
        }
        return value;
    }

    double horizontal_up(double value)
    {
        horizontal_finite(value);
        if (value < 0.0)
        { throw std::invalid_argument("Horizontal motion requires nonnegative displacement budgets."); }
        float result = static_cast<float>(value);
        if (static_cast<double>(result) < value)
        { result = std::nextafter(result, std::numeric_limits<float>::infinity()); }
        return horizontal_finite(static_cast<double>(result));
    }

    // A full upward float spacing covers nearest-rounding error on either sign.
    // Refuse extreme coordinates instead of deriving an infinite/vacuous bound.
    double horizontal_ulp(double magnitude)
    {
        const float value = static_cast<float>(horizontal_up(std::abs(magnitude)));
        const float next = std::nextafter(value, std::numeric_limits<float>::infinity());
        horizontal_finite(static_cast<double>(next));
        return static_cast<double>(next) - static_cast<double>(value);
    }

    HorizontalBox horizontal_box(const RectangleF& value)
    {
        const HorizontalBox box{ value.left(), value.top(), value.right(), value.bottom() };
        for (double edge : { box.left, box.top, box.right, box.bottom })
        { horizontal_finite(edge); }
        if (!std::isfinite(value.width) || !std::isfinite(value.height) ||
            !(value.width > 0.0f) || !(value.height > 0.0f) ||
            !(box.left < box.right) || !(box.top < box.bottom))
        { throw std::invalid_argument("Horizontal motion requires finite positive represented body dimensions."); }
        return box;
    }

    RectangleF horizontal_pose(const HorizontalMotionSpec& spec, float offset)
    {
        RectangleF result = spec.anchor;
        if (offset != 0.0f) { result.x += offset; }
        horizontal_box(result);
        return result;
    }

    HorizontalBox horizontal_path(const TranslatingStructurePaintable& mover)
    {
        const HorizontalMotionSpec spec = mover.motion_state().spec;
        HorizontalMotion::validate_spec(spec);
        const HorizontalBox first = horizontal_box(spec.anchor);
        const HorizontalBox last = horizontal_box(horizontal_pose(spec, spec.travel_x));
        return { std::min(first.left, last.left), first.top,
            std::max(first.right, last.right), first.bottom };
    }

    bool horizontal_contains(const HorizontalBox& outer, const HorizontalBox& inner)
    {
        return inner.left >= outer.left && inner.top >= outer.top &&
            inner.right <= outer.right && inner.bottom <= outer.bottom;
    }

    bool horizontal_intersects(const HorizontalBox& a, const HorizontalBox& b)
    {
        // Positive interiors only. A coplanar dock's top touches the rider's
        // feet boundary; it is not a raised lip/ceiling exemption.
        return a.left < b.right && a.right > b.left &&
            a.top < b.bottom && a.bottom > b.top;
    }

    struct HorizontalPlayerSpeed { double x, y; };

    HorizontalPlayerSpeed horizontal_player_speed(const Player& actual,
        const PlayerTuning& values)
    {
        const Vector2F velocity = actual.velocity();
        // Check operands before max(): a NaN in a later argument could otherwise
        // be ignored by std::max rather than poison the derived budget.
        for (float value : { values.max_velocity.x, values.max_velocity.y,
            values.x_initial_velocity, values.x_acceleration, values.x_deacceleration,
            values.gravity, values.ramp_gravity_multiplier, values.jump_launch_velocity,
            values.drop_down_velocity, values.jump_acceleration,
            values.paint_underfoot.own_speed_multiplier, values.paint_underfoot.enemy_speed_multiplier })
        { horizontal_finite(static_cast<double>(value)); }
        horizontal_finite(velocity.x); horizontal_finite(velocity.y);
        if (!(values.max_velocity.x > 0.0f) || !(values.max_velocity.y > 0.0f) ||
            !(values.x_initial_velocity > 0.0f) || !(values.x_acceleration >= 0.0f) ||
            !(values.x_deacceleration > 0.0f) || !(values.ramp_gravity_multiplier >= 0.0f) ||
            !(values.paint_underfoot.own_speed_multiplier >= 1.0f) ||
            !(values.paint_underfoot.enemy_speed_multiplier > 0.0f &&
                values.paint_underfoot.enemy_speed_multiplier <= 1.0f))
        { throw std::invalid_argument("Horizontal motion requires supported player rate magnitudes."); }
        const double factor = values.paint_underfoot.enabled ?
            std::max(1.0, static_cast<double>(values.paint_underfoot.own_speed_multiplier)) : 1.0;
        const double initial = horizontal_up(std::abs(static_cast<double>(values.x_initial_velocity)) * factor);
        const double cap = horizontal_up(std::abs(static_cast<double>(values.max_velocity.x)) * factor);
        const double acceleration = horizontal_up(std::abs(static_cast<double>(values.x_acceleration)) * factor);
        const double braking = horizontal_up(std::abs(static_cast<double>(values.x_deacceleration)) * factor);
        // These float intermediates exist before any clamp/stop-time branch.
        horizontal_up(acceleration + braking);
        const double step = static_cast<double>(SECONDS_PER_STEP);
        double reachable_x = std::max({ std::abs(static_cast<double>(velocity.x)), initial, cap });
        double acceleration_step = 0.0;
        double least_braking = (std::numeric_limits<double>::max)();
        const std::array<float, 3> factors{ 1.0f,
            values.paint_underfoot.own_speed_multiplier,
            values.paint_underfoot.enemy_speed_multiplier };
        const std::size_t factor_count = values.paint_underfoot.enabled ? factors.size() : 1;
        const auto effective_rate = [](float base, float multiplier, bool positive)
        {
            // Mirror Player's ==1 branch and actual float scaling, after a
            // conservative double product check. In particular the denominator
            // below must not use a rounded-UP estimate of a tiny braking rate.
            horizontal_up(static_cast<double>(base) * static_cast<double>(multiplier));
            const float result = multiplier == 1.0f ? base : base * multiplier;
            if (!std::isfinite(result) || !(positive ? result > 0.0f : result >= 0.0f))
            { throw std::invalid_argument("Horizontal motion effective horizontal rates are non-finite or unusable."); }
            return result;
        };
        for (std::size_t index = 0; index < factor_count; ++index)
        {
            const float multiplier = factors[index];
            const float effective_initial = effective_rate(values.x_initial_velocity, multiplier, true);
            const float effective_cap = effective_rate(values.max_velocity.x, multiplier, true);
            const float effective_acceleration = effective_rate(values.x_acceleration, multiplier, false);
            const float effective_braking = effective_rate(values.x_deacceleration, multiplier, true);
            reachable_x = std::max({ reachable_x, static_cast<double>(effective_initial),
                static_cast<double>(effective_cap) });
            // |input|<=1 cannot increase either product. Check the opposing
            // sum, then acceleration*dt (before its multiply by input).
            horizontal_up(static_cast<double>(effective_braking) + effective_acceleration);
            acceleration_step = std::max(acceleration_step,
                horizontal_up(static_cast<double>(effective_acceleration) * step));
            least_braking = std::min(least_braking, static_cast<double>(effective_braking));
        }
        const double preclamp = reachable_x + acceleration_step;
        if (!std::isfinite(preclamp) || preclamp > static_cast<double>((std::numeric_limits<float>::max)()))
        { throw std::invalid_argument("Horizontal motion horizontal pre-clamp addition is unsafe."); }
        horizontal_up(preclamp);
        // braking_rate + acceleration*|input| is at least this ACTUAL positive
        // scaled rate for every enabled class. Retained/reachable vx can enter
        // that opposing-input division before any cap recovers its magnitude.
        const double stop_time = reachable_x / least_braking;
        if (!std::isfinite(stop_time) || stop_time > static_cast<double>((std::numeric_limits<float>::max)()))
        { throw std::invalid_argument("Horizontal motion horizontal stop-time division is unsafe."); }
        horizontal_up(stop_time);
        const double gravity = std::abs(static_cast<double>(values.gravity));
        const double ramp_gravity = horizontal_up(gravity *
            std::abs(static_cast<double>(values.ramp_gravity_multiplier)));
        double vertical = std::max({ std::abs(static_cast<double>(velocity.y)),
            std::abs(static_cast<double>(values.max_velocity.y)),
            std::abs(static_cast<double>(values.jump_launch_velocity)) });
        vertical = horizontal_up(vertical + horizontal_up(std::max(gravity, ramp_gravity) * step));
        vertical = horizontal_up(vertical + std::abs(static_cast<double>(values.drop_down_velocity)));
        vertical = horizontal_up(vertical + horizontal_up(
            std::abs(static_cast<double>(values.jump_acceleration)) * step));
        return { reachable_x, vertical };
    }
}

Level::Level(std::unique_ptr<Scene> scene,
	std::vector<Player*> players,
	std::vector<std::unique_ptr<GameObject>> viewport_dividers,
	const TeamColour& team_colours,
	const RectangleF& camera_bounds,
	const RectangleF& zoom_out_start_bounds,
	const RectangleF& zoom_out_finish_bounds,
	const GameTuning* tuning,
	const Presentation* presentation,
	const std::wstring& stage_name,
	const std::string& sound_bank_name,
	const std::string& music_name,
	float music_volume,
	const std::string& end_sting_name,
	float end_sting_volume,
	const ResolutionManager* resolution_manager,
	const ViewportManager* viewport_manager,
	RenderResources* render_resources,
	const AudioResources* audio_resources,
	const PlaySettings* preferences,
	// Disambiguate from DirectX::Mouse.
	const labrador::Mouse* mouse,
	std::vector<LevelStation> stations,
	OpeningMode opening, std::optional<float> duration, const InputBindings* input_bindings) :
	scene_(std::move(scene)),
	players_(std::move(players)),
	stations_(std::move(stations)),
	viewport_dividers_(std::move(viewport_dividers)),
	music_volume_(music_volume),
	end_sting_volume_(end_sting_volume),
	resolution_manager_(resolution_manager),
	viewport_manager_(viewport_manager),
	mouse_(mouse),
	team_colours_(team_colours),
	tuning_(tuning),
	camera_bounds_(camera_bounds),
	zoom_out_start_bounds_(zoom_out_start_bounds),
	zoom_out_finish_bounds_(zoom_out_finish_bounds)
{
	this->preferences_ = preferences;
	this->input_bindings_ = input_bindings;
	this->presentation_ = presentation;
	this->render_resources_ = render_resources;
	const MenuStyle& menu = presentation->menu();
	const CountdownArtPresentation& paper = presentation->match().countdown_art;
	this->stage_entry_ = std::make_unique<TextDropShadow>(stage_name,
		menu.heading_font, Vector2F::ZERO, render_resources,
		paper.ink, paper.paper, menu.heading_shadow_offset);
	this->player_badge_ = std::make_unique<PlayerBadge>(
		&presentation->player_identity(), render_resources);
	this->player_identity_remaining_.assign(this->players_.size(),
		presentation->player_identity().emphasis_duration);
	this->paint_audio_ = std::make_unique<PaintAudio>(&tuning->paint(), audio_resources);
	this->blocked_shot_feedback_ = std::make_unique<BlockedShotFeedback>(
		&presentation->blocked_shot(), render_resources);
	for (Player* player : this->players_)
	{
		player->set_paint_audio(this->paint_audio_.get());
		player->set_blocked_shot_feedback(this->blocked_shot_feedback_.get());
		IdleDrip* drip = nullptr;
		const WeaponType type = player->primary_weapon().type();
		if (type == WeaponType::sprayer || type == WeaponType::sniper || type == WeaponType::bucket)
		{
			drip = this->scene_->add(std::make_unique<IdleDrip>(
				&presentation->idle_drip(type), render_resources, player->team_colour(),
				static_cast<uint32_t>(this->idle_drips_.size()), preferences));
		}
		this->idle_drips_.push_back(drip);
	}
	this->overlay_sheet_ = render_resources->resolve_sprite_sheet(
		presentation->hud().sprite_sheet);
	this->overlay_fill_ = render_resources->sprite_sheet(this->overlay_sheet_)
		->resolve_sprite_frame(presentation->hud().fill_frame);
	const MatchTuning& match = tuning->match();

	this->timer_ = duration.value_or(match.duration);
	this->start_timer_ = match.countdown;
	this->zoom_out_timer_ = match.zoom_out;
	this->overview_timer_ = match.overview;

	this->debug_text_ = std::make_unique<DebugText>(
		render_resources, resolution_manager,
		&presentation->debug_overlay());
	this->interface_gameplay_ = std::make_unique<InterfaceGameplay>(
		render_resources, &presentation->hud(), &presentation->player_identity());
	this->refresh_identity_texts();
	this->sound_bank_ = audio_resources->sound_bank(sound_bank_name);

	// A silent stage leaves music_ unresolved; every music call checks it.
	if (!music_name.empty())
	{
		this->music_ = this->sound_bank_->resolve_effect(music_name);
	}
	if (!end_sting_name.empty())
	{
		this->end_sting_ = this->sound_bank_->resolve_effect(end_sting_name);
	}
	this->zoom_out_sound_ =
		this->sound_bank_->resolve_effect(match.zoom_out_sound.name);
	this->countdown_sound_ =
		this->sound_bank_->resolve_effect(match.countdown_sound.name);
	this->go_sound_ = this->sound_bank_->resolve_effect(match.go_sound.name);
	this->time_up_sound_ = this->sound_bank_->resolve_effect(match.time_up_sound.name);
	this->last_ten_sound_ = this->sound_bank_->resolve_effect(match.last_ten_sound.name);
	this->room_tone_sound_ = this->sound_bank_->resolve_effect(match.room_tone_sound.name);
	this->sigh_sound_ = this->sound_bank_->resolve_effect(match.sigh_sound.name);
	this->chair_sound_ = this->sound_bank_->resolve_effect(match.chair_sound.name);
	this->glass_in_sound_ = this->sound_bank_->resolve_effect(match.glass_in_sound.name);
	this->celebrate_sound_ = this->sound_bank_->resolve_effect(match.celebrate_sound.name);
	this->slump_sound_ = this->sound_bank_->resolve_effect(match.slump_sound.name);
	this->colour_in_sound_ = this->sound_bank_->resolve_effect(match.colour_in_sound.name);
	this->skip_sound_ = this->sound_bank_->resolve_effect(match.skip_sound.name);
	this->undo_sound_ = this->sound_bank_->resolve_effect(match.undo_sound.name);

	if (opening != OpeningMode::off)
	{
		const OpeningPresentation& look = presentation->opening();
		this->monitor_ = std::make_unique<Monitor>(&look, render_resources,
			MonitorPersona::roll(look));
		this->silhouette_ = std::make_unique<Silhouette>(&look, render_resources);
		this->cursor_ = std::make_unique<ScreenCursor>(&look, render_resources);
		this->acting_ = std::make_unique<FigureActing>(&look, &presentation->respawn_redraw(),
			render_resources);
		this->reveal_text_ = presentation->menu_text().results;
		this->reveal_art_ = std::make_unique<RevealArt>(&look, render_resources);
		this->hover_sound_ = this->sound_bank_->resolve_wave(match.hover_sound.name);
		this->click_sound_ = this->sound_bank_->resolve_wave(match.click_sound.name);
		this->close_sound_ = this->sound_bank_->resolve_wave(match.close_sound.name);
		this->state_ = LevelState::opening;
		// A quick rematch starts with the colours being tried.
		this->opening_time_ = opening == OpeningMode::quick ? match.opening.settle : 0.0f;
		this->colour_players(false, false);
	}

	this->build_opening_frame(render_resources);
}

void Level::update(const std::vector<PlayerInputData>& player_inputs, float dt,
    SeatInputFilter* seats)
{
    // All dynamic validation/allocation precedes layout, clocks and input/RNG.
    std::optional<HorizontalFrame> horizontal;
    if (this->has_horizontal_motion())
    {
        HorizontalMotion::validate_dt(dt);
        if (dt > 0.0f) { horizontal.emplace(this->prepare_horizontal_frame(dt)); }
        this->publish_horizontal_zero();
    }
	this->refresh_layout();
	this->simulation_dt_ = dt;
	if (dt > 0.0f)
	{
		this->update_state(player_inputs, dt, seats, horizontal ? &*horizontal : nullptr);
		this->update_acting(dt);
	}

	// Refresh on every path so views and overlays reflect the completed tick.
	this->refresh_views();
}

std::vector<SlotRumbleEvents> Level::take_rumble_events()
{
	std::vector<SlotRumbleEvents> result;
	// Complete allocation before consuming any player's pending flags.
	result.reserve(this->players_.size());
	for (Player* player : this->players_)
	{
		result.push_back({ player->player_num(), player->take_rumble_events() });
	}
	return result;
}

void Level::clear_rumble_events() noexcept
{
	for (Player* player : this->players_) { player->clear_rumble_events(); }
}

void Level::update_state(const std::vector<PlayerInputData>& player_inputs,
	float dt, SeatInputFilter* seats, const HorizontalFrame* horizontal)
{
	if (this->state_ == LevelState::opening)
	{
		this->update_opening(player_inputs, dt);
		return;
	}
	if (this->intro_countdown_ && this->skip_requested(player_inputs))
	{
		this->skip_intro();
	}
	if (this->state_ == LevelState::start_countdown ||
		(this->state_ == LevelState::active && this->start_timer_ > -1.0f))
	{
		const bool starting = !this->countdown_started_;
		if (starting)
		{
			this->countdown_started_ = true;
			if (this->music_.valid())
			{
				this->music_fade_.fade_to(1.0f, this->presentation_->music_mix().fade_in);
				this->sound_bank_->play_effect(this->music_, true, 0.0f);
			}
		}
		const float before = this->start_timer_;
		this->start_timer_ -= dt;
		// Only the opening countdown pushes through the monitor. A long tick
		// crossing its start still owns one transition cue, never a backlog.
		if (this->intro_countdown_ && !this->glass_in_started_ &&
			this->start_timer_ <= this->tuning_->match().opening.push)
		{
			this->glass_in_started_ = true;
			this->set_room_tone(false);
			this->sound_bank_->play_effect(this->glass_in_sound_, false,
				this->tuning_->match().glass_in_sound.volume);
		}
		// One cue for the displayed number, including the initial beat. A long
		// tick never queues the missed beats over GO or live play.
		if (this->start_timer_ > 0.0f &&
			(starting ||
				std::ceil(before) != std::ceil(this->start_timer_)))
		{
			this->sound_bank_->play_effect(this->countdown_sound_, false,
				this->tuning_->match().countdown_sound.volume);
		}
		if (this->start_timer_ > 0.0f)
		{
			this->set_countdown_text(std::to_wstring(
				static_cast<int>(std::ceil(this->start_timer_))));
		}
		else
		{
			this->set_countdown_text(L"GO!");
		}

		if (this->start_timer_ <= 0.0f)
		{
			if (this->state_ == LevelState::start_countdown)
			{
				this->sound_bank_->stop_effect(this->countdown_sound_, true);
				this->sound_bank_->play_effect(this->go_sound_, false,
					this->tuning_->match().go_sound.volume);
			}
			this->state_ = LevelState::active;
			if (before > 0.0f || starting) { this->emphasise_players(); }
			this->intro_countdown_ = false;
		}
		else
		{
			return;
		}
	}
	if (this->state_ == LevelState::active)
	{
		this->update_level_logic(player_inputs, dt, seats, horizontal);

		this->timer_ -= dt;
		// Sound only the current displayed second, never missed seconds after a
		// long update. Short matches also get their first positive clock cue.
		const float second = std::ceil(this->timer_);
		if (this->timer_ > 0.0f && second <= 10.0f && second != this->last_ten_second_)
		{
			this->last_ten_second_ = second;
			this->sound_bank_->play_effect(this->last_ten_sound_, false,
				this->tuning_->match().last_ten_sound.volume);
		}
		if (this->timer_ <= 0.0f)
		{
			this->state_ = LevelState::zoom_out;
			this->blocked_shot_feedback_->clear();

			// Seed the camera now: the zoom_out arm runs next tick, but refresh_views
			// already renders the newly entered state this frame.
			this->zoom_out_camera_ = this->zoom_out_start_camera_;

			if (this->music_.valid())
			{
				this->music_ending_ = true;
				this->music_fade_.fade_to(0.0f, this->presentation_->music_mix().fade_out);
			}
			if (this->end_sting_.valid())
			{
				this->sound_bank_->play_effect(this->end_sting_, false,
					this->end_sting_volume_);
			}
			this->stop_player_sounds();
			for (Player* player : this->players_)
			{
				player->reset_tool_motion();
			}
			for (IdleDrip* drip : this->idle_drips_)
			{
				if (drip != nullptr) { drip->clear(); }
			}
			this->sound_bank_->stop_effect(this->countdown_sound_, true);
			this->sound_bank_->stop_effect(this->go_sound_, true);
			this->sound_bank_->stop_effect(this->last_ten_sound_, true);
			this->sound_bank_->play_effect(this->time_up_sound_, false,
				this->tuning_->match().time_up_sound.volume);
			this->sound_bank_->play_effect(this->zoom_out_sound_, false,
				this->tuning_->match().zoom_out_sound.volume);
			this->start_reveal();
		}
	}
	else if (this->state_ == LevelState::zoom_out)
	{
		this->zoom_out_timer_ -= dt;

		if (this->zoom_out_timer_ <= 0.0f)
		{
			this->state_ = LevelState::overview;
			this->zoom_out_timer_ = 0.0f;
			// Even a shortened glass transition leaves the returning person silent.
			if (this->music_.valid()) { this->sound_bank_->stop_effect(this->music_, true); }
			if (this->end_sting_.valid()) { this->sound_bank_->stop_effect(this->end_sting_, true); }
			// Let the glass cue's tail finish naturally; destruction still owns it.
		}

		// refresh_layout refits the endpoints when the window changes size.
		this->zoom_out_camera_ = Camera::calculate_intermediate_camera(
			this->zoom_out_start_camera_, this->zoom_out_finish_camera_,
			this->zoom_out_camera_ratio());
	}
	else if (this->state_ == LevelState::overview && this->monitor_ != nullptr)
	{
		this->update_reveal(dt);
	}
	else if (this->state_ == LevelState::overview)
	{
		this->overview_timer_ -= dt;
		if (this->overview_timer_ <= 0.0f)
		{
			this->state_ = LevelState::finished;
		}
	}
	// GameLevel opens results when it sees finished, so this state needs no update arm.
	// Arena music fades through glass-out; overview and destruction stop it.

}

float Level::zoom_out_camera_ratio() const
{
	// Defensively treat a non-positive duration as a completed zoom, avoiding 0/0.
	// The tuning loader requires a positive duration.
	const float duration = this->tuning_->match().zoom_out;
	if (duration <= 0.0f)
	{
		return 1.0f;
	}

	return 1.0f - (this->zoom_out_timer_ / duration);
}

void Level::update_level_logic(const std::vector<PlayerInputData>& player_inputs,
	float dt, SeatInputFilter* seats, const HorizontalFrame* horizontal)
{
	std::array<bool, 4> was_dead{};
	for (size_t view = 0; view < this->players_.size(); ++view)
	{
		was_dead[view] = this->players_[view]->state() == PlayerState::dead;
	}
	this->update_player_identity(dt);
	// Run the scene phases in order, with game-specific work between them.
	this->paint_audio_->update(dt);
	this->blocked_shot_feedback_->update(dt);
	this->apply_player_inputs(player_inputs, seats);
    if (horizontal != nullptr) { this->commit_horizontal_frame(*horizontal); }

	this->scene_->update(dt);
	this->resolve_projectile_movement();

	this->update_players(dt);

	this->scene_->resolve();
	for (CollisionObject* object : this->scene_->collision_objects())
	{
		if (is_projectile(to_collision_type(object->tag())))
		{
			Projectile& projectile = dynamic_cast<Projectile&>(*object);
			projectile.flush_paint_audio(*this->paint_audio_);
			if (const auto contact = projectile.take_blocked_contact())
			{
				this->blocked_shot_feedback_->emit(*contact);
			}
		}
	}

	// Loss of ground support is known only after all contacts have been dispatched.
	for (Player* player : this->players_)
	{
        if (this->has_horizontal_motion())
        { player->end_contacts(this->scene_->contacts(), this->horizontal_supports_); }
        else { player->end_contacts(this->scene_->contacts()); }
	}

	this->update_players_after_resolve();
	for (size_t view = 0; view < this->players_.size(); ++view)
	{
		if (was_dead[view] && this->players_[view]->state() == PlayerState::alive)
		{
			this->player_identity_remaining_[view] =
				this->presentation_->player_identity().emphasis_duration;
		}
	}
	this->update_idle_drips(dt);

	// Retire deleted/out-of-bounds objects and admit this tick's projectiles.
	// Deferred admission keeps the scene stable during iteration.
	this->scene_->end_tick();

	// Count once per tick for all debug overlays.
	this->projectile_count_ = this->count_projectiles();

	// Players must remain in the arena; unlike projectiles, they cannot be retired.
	for (const Player* player : this->players_)
	{
		if (!this->scene_->in_bounds(*player))
		{
			throw std::runtime_error("Player out of bounds");
		}
	}
}

void Level::resolve_mouse_aim(Player* player, size_t view_index,
	PlayerInputData& input) const
{
	if (!this->mouse_->focused())
	{
		// Without a valid cursor, leave aim unrequested rather than reuse a stale direction.
		return;
	}

	const labrador::Viewport viewport =
		this->viewport_manager_->player_viewport(static_cast<int>(view_index));

	const Vector2F cursor(static_cast<float>(this->mouse_->state().position.x),
		static_cast<float>(this->mouse_->state().position.y));
	const Vector2F aim = PlayerCamera::mouse_direction(
		player->center(), cursor, viewport, player->camera());

	// A cursor exactly on the player requests no direction; do not normalize zero.
	if (aim == Vector2F::ZERO)
	{
		return;
	}

	input.shoot_direction_requested = true;
	input.shoot_direction = aim;
	input.shoot_angle = input.shoot_direction.angle();
}

void Level::apply_player_inputs(
	const std::vector<PlayerInputData>& player_inputs, SeatInputFilter* seats)
{
	// One input per view; an idle dummy (or an unknown slot) gets neutral input.
	std::vector<PlayerInputData> by_view(this->players_.size());
	for (size_t i = 0; i < this->players_.size(); ++i)
	{
		Player* player = this->players_[i];
		const int slot = player->player_num();

		if (slot >= 0 && static_cast<size_t>(slot) < player_inputs.size())
		{
			by_view[i] = player_inputs[slot];

			// Copy the slot input to fill in this player's camera-dependent mouse aim.
			if (input_slots::is_keyboard_and_mouse(slot))
			{
				this->resolve_mouse_aim(player, i, by_view[i]);
			}
		}
	}

	if (seats != nullptr)
	{
		seats->filter_seats(by_view, *this);
	}

	for (size_t i = 0; i < this->players_.size(); ++i)
	{
		this->players_[i]->set_player_input(by_view[i]);
	}
}

const IdleDrip* Level::idle_drip(int view) const
{
	return this->idle_drips_.at(static_cast<size_t>(view));
}

void Level::update_idle_drips(float dt)
{
	for (size_t index = 0; index < this->players_.size(); ++index)
	{
		if (IdleDrip* drip = this->idle_drips_[index])
		{
			const Player& player = *this->players_[index];
			const Weapon& weapon = player.primary_weapon();
			const PlayerMoveState move = player.move_state();
			const bool supported = move == PlayerMoveState::on_ground ||
				move == PlayerMoveState::on_drop_down_ground ||
				move == PlayerMoveState::on_ramp_left || move == PlayerMoveState::on_ramp_right;
			const bool settled = player.state() == PlayerState::alive && player.health() > 0.0f &&
				supported && player.velocity().x == 0.0f && player.velocity().y == 0.0f &&
				player.input().x_movement == 0.0f && !player.input().primary_shoot &&
				!player.input().jump_held && !player.input().jump_pressed &&
				player.animation_state() == PlayerAnimationState::idle;
			drip->advance(settled, weapon.nozzle_pos(), weapon.rotation(), dt,
				this->projectile_targets_);
		}
	}
}

void Level::update_players(float dt)
{
	this->refresh_player_cameras();
	for (Player* player : this->players_)
	{
		for (std::unique_ptr<Projectile>& projectile :
			player->update_weapon_and_get_projectiles(
				dt, this->projectile_targets_))
		{
			// The game keeps the projectile interface until Scene takes ownership
			// through its engine collision interface.
			this->scene_->add(std::move(projectile));
		}
	}
}

void Level::resolve_projectile_movement()
{
	// Resolve the first hit along each projectile's travel before endpoint contacts
	// can consume it in scene order or miss a thin paintable face.
	for (CollisionObject* object : this->scene_->collision_objects())
	{
		if (is_projectile(to_collision_type(object->tag())))
		{
			dynamic_cast<Projectile&>(*object).resolve_movement_contacts(
				this->projectile_targets_);
		}
	}
}

void Level::refresh_player_cameras()
{
	for (size_t i = 0; i < this->players_.size(); ++i)
	{
		Player* player = this->players_[i];
		// Fatal contacts can leave state alive until the next Player update.
		// Dead players retain velocity while camera_focus holds then visits spawn.
		const float velocity = player->state() == PlayerState::alive && player->health() > 0.0f
			? player->velocity().x : 0.0f;
		const Camera camera = this->player_camera_.follow(
			player->camera_focus(),
			this->viewport_manager_->player_viewport(
				static_cast<int>(i)),
			this->resolution_manager_->resolution_vec().y,
			this->presentation_->camera(),
			player->camera(),
			this->camera_bounds_, velocity);
		player->set_camera(camera);
	}
}

void Level::set_debug_frame_time(double frame_ms)
{
	this->debug_frame_ms_ = frame_ms;
}

void Level::refresh_layout()
{
	this->refresh_station_labels();
	this->refresh_keyboard_controls();
	const Vector2F size = this->resolution_manager_->resolution_vec();
	if (size == this->layout_size_)
	{
		// Preferences can change while pause covers the same-sized match.
		this->refresh_views();
		return;
	}

	const Viewport screen(RectangleF(Vector2F::ZERO, size));
	this->zoom_out_start_camera_ = Camera::frame(this->zoom_out_start_bounds_, screen);
	this->zoom_out_finish_camera_ = Camera::frame(this->zoom_out_finish_bounds_, screen);
	this->zoom_out_camera_ = Camera::calculate_intermediate_camera(
		this->zoom_out_start_camera_, this->zoom_out_finish_camera_,
		this->zoom_out_camera_ratio());
	this->centre_countdown_text();
	this->centre_controls_text();
	this->refresh_player_cameras();
	this->refresh_views();
	this->layout_size_ = size;
}

void Level::update_players_after_resolve()
{
	for (Player* player : this->players_)
	{
		player->update_weapon_position();
		player->update_prev_rectangle();
	}
}

Camera Level::view_camera(int view) const
{
	const Player& player = *this->players_.at(static_cast<size_t>(view));
	if (this->state_ != LevelState::active || !this->preferences_->view_shake_enabled() ||
		player.view_shake().offset() == Vector2F::ZERO) { return player.camera(); }
	return PlayerCamera::displaced(player.camera(),
		this->viewport_manager_->player_viewport(view), player.view_shake().offset(),
		this->camera_bounds_);
}

void Level::refresh_views()
{
	// Match draw()'s paths, including the intro countdown and glass return.
	// A small active pane still uses ordinary terrain. All mode writes happen
	// before drawing; Scene's workers only read each shared structure.
	const bool miniature = this->intro_showing() || this->reveal_showing() ||
		(this->state_ != LevelState::start_countdown && this->state_ != LevelState::active);
	// These existing stable targets include every structure and player, but no
	// retiring projectile. This is mode configuration, not a draw-order mirror.
	for (CollisionObject* object : this->projectile_targets_)
	{
		if (auto* structure = dynamic_cast<Structure*>(object))
		{
			structure->set_miniature(miniature);
		}
	}
	this->scene_->clear_views();

	if (this->intro_showing() || this->reveal_showing())
	{
		this->add_intro_views();
		return;
	}

	if (this->state_ == LevelState::start_countdown ||
		this->state_ == LevelState::active)
	{
		// Pane order follows players_, not input slots, which may be sparse.
		for (size_t i = 0; i < this->players_.size(); ++i)
		{
			this->scene_->add_view(
				Viewport(this->viewport_manager_->player_viewport(
					static_cast<int>(i)).pixel_rect()),
				this->view_camera(static_cast<int>(i)));
		}
	}
	else
	{
		// Use one fullscreen view without changing the shared split-screen layout.
		this->scene_->add_view(this->viewport_manager_->fullscreen_viewport(),
			this->zoom_out_camera_);
	}
}

int Level::player_count() const
{
	return static_cast<int>(this->players_.size());
}

const Player& Level::player(int view) const
{
	return *this->players_.at(static_cast<size_t>(view));
}

LevelState Level::state() const
{
	return this->state_;
}

bool Level::match_started() const
{
	return this->state_ == LevelState::active || this->state_ == LevelState::zoom_out ||
		this->state_ == LevelState::overview || this->state_ == LevelState::finished;
}

int Level::count_projectiles() const
{
	int count = 0;
	for (const CollisionObject* object : this->scene_->collision_objects())
	{
		if (is_projectile(to_collision_type(object->tag())))
		{
			count++;
		}
	}
	return count;
}

LevelEndInfo Level::level_end_info() const
{
	auto result = LevelEndInfo();
	result.team_colours = this->team_colours_;

	// Derive team participation from the players actually in the match.
	result.team_a_played = false;
	result.team_b_played = false;
	for (const Player* player : this->players_)
	{
		switch (player->team())
		{
		case PlayerTeam::a:
			result.team_a_played = true;
			break;
		case PlayerTeam::b:
			result.team_b_played = true;
			break;
		default:
			break;
		}
	}

	for (CollisionObject* object : this->scene_->collision_objects())
	{
		// Scoring is a role: a paintable support can retain jump-through
		// collision without losing its held area or player contribution.
		PaintableObject* paintable_object = dynamic_cast<PaintableObject*>(object);
		if (paintable_object == nullptr)
		{
			if (to_collision_type(object->tag()) == CollisionObjectType::structure_paintable)
			{
				throw std::runtime_error(
					"Object tagged STRUCTURE_PAINTABLE is not an PaintableObject");
			}
			continue;
		}

		PaintTotal paint = paintable_object->paint_total();

		result.team_a_area += paint.team_a;
		result.team_b_area += paint.team_b;
		result.total_area += paint.total;
		for (size_t slot = 0; slot < result.player_areas.size(); ++slot)
		{
			result.player_areas[slot] += paint.player_areas[slot];
		}
	}

	return result;
}

std::vector<PaintSurfaceSample> Level::paint_surfaces() const
{
    std::vector<PaintSurfaceSample> samples;
    std::size_t source_index = 0;
    for (const CollisionObject* object : this->scene_->collision_objects())
    {
        const PaintableObject* paintable = dynamic_cast<const PaintableObject*>(object);
        if (paintable == nullptr)
        {
            if (to_collision_type(object->tag()) == CollisionObjectType::structure_paintable)
            {
                throw std::runtime_error("Object tagged STRUCTURE_PAINTABLE is not an PaintableObject");
            }
            continue;
        }
        paintable->append_paint_surfaces(samples, source_index++);
    }
    return samples;
}

bool Level::has_horizontal_motion() const
{
    return !this->horizontal_structures_.empty();
}

void Level::admit_horizontal_structures()
{
    // Leave static Level arithmetic, allocation and construction order alone.
    bool moving = false;
    for (CollisionObject* object : this->scene_->collision_objects())
    { moving |= dynamic_cast<TranslatingStructurePaintable*>(object) != nullptr; }
    if (!moving) { return; }

    std::uint64_t support_order = 0;
    for (CollisionObject* object : this->scene_->collision_objects())
    {
        auto* surface = dynamic_cast<Structure*>(object);
        if (surface == nullptr) { continue; }
        if (support_order > (std::numeric_limits<std::uint32_t>::max)())
        { throw std::invalid_argument("Horizontal motion has too many canonical support identities."); }
        HorizontalMotionId id = 0;
        if (auto* mover = dynamic_cast<TranslatingStructurePaintable*>(surface))
        {
            if (mover->motion_id() != 0 || this->horizontal_structures_.size() >=
                static_cast<std::size_t>((std::numeric_limits<int>::max)()))
            { throw std::invalid_argument("Horizontal motion identities must be bound once within INT_MAX."); }
            this->horizontal_structures_.push_back(mover);
            id = static_cast<HorizontalMotionId>(this->horizontal_structures_.size());
        }
        this->horizontal_supports_.push_back({ surface,
            static_cast<std::uint32_t>(support_order++), id });
    }
    this->validate_horizontal_admission();
    this->validate_horizontal_budget(nullptr);
    // All counts/geometry were checked before the first binding operation.
    for (std::size_t index = 0; index < this->horizontal_structures_.size(); ++index)
    { this->horizontal_structures_[index]->bind_motion_id(static_cast<HorizontalMotionId>(index + 1)); }
}

void Level::validate_horizontal_admission() const
{
    if (!this->has_horizontal_motion()) { return; }
    if (this->players_.empty() || !this->scene_->bounds())
    { throw std::invalid_argument("Horizontal motion admission requires actual Players and finite Scene bounds."); }
    const HorizontalBox world = horizontal_box(*this->scene_->bounds());
    for (const TranslatingStructurePaintable* mover : this->horizontal_structures_)
    {
        const HorizontalBox path = horizontal_path(*mover);
        if (!horizontal_contains(world, path))
        { throw std::invalid_argument("Horizontal motion platform envelope leaves Scene bounds."); }
        for (const Player* player : this->players_)
        {
            if (player == nullptr)
            { throw std::invalid_argument("Horizontal motion admission received a null Player."); }
            const RectangleF& body = player->rectangle();
            horizontal_box(body);
            const double scale = std::max({ std::abs(path.left), std::abs(path.right),
                std::abs(path.top), static_cast<double>(body.width), static_cast<double>(body.height) });
            const double pad = 2.0 * horizontal_ulp(horizontal_finite(2.0 * scale));
            const HorizontalBox standing{ horizontal_finite(path.left - body.width - pad),
                horizontal_finite(path.top - body.height - pad),
                horizontal_finite(path.right + body.width + pad), path.top };
            if (!horizontal_contains(world, standing))
            { throw std::invalid_argument("Horizontal motion standing envelope leaves Scene bounds."); }
            for (const CollisionObject* object : this->scene_->collision_objects())
            {
                if (object == mover || !is_structure(to_collision_type(object->tag()))) { continue; }
                if (object->shape() == nullptr)
                { throw std::invalid_argument("Horizontal motion blocking geometry has no shape."); }
                const RectangleF actual = object->shape()->bounding_box();
                const HorizontalBox obstacle = dynamic_cast<const TranslatingStructurePaintable*>(object) != nullptr ?
                    horizontal_path(*static_cast<const TranslatingStructurePaintable*>(object)) : horizontal_box(actual);
                // Positive, finite sums must not turn the engine's raw float
                // extent addition into an infinite, automatically passing bound.
                horizontal_finite(static_cast<double>(body.width) + actual.width);
                horizontal_finite(static_cast<double>(body.height) + actual.height);
                if (horizontal_intersects(path, obstacle))
                { throw std::invalid_argument("Horizontal motion platform envelope intersects blocking geometry."); }
                if (horizontal_intersects(standing, obstacle))
                { throw std::invalid_argument("Horizontal motion standing envelope intersects blocking geometry."); }
            }
        }
    }
}

void Level::validate_horizontal_budget(const HorizontalFrame* prepared) const
{
    if (!this->has_horizontal_motion()) { return; }
    if (prepared != nullptr && prepared->motions.size() != this->horizontal_structures_.size())
    { throw std::invalid_argument("Horizontal motion preparation cardinality differs from admission."); }
    const double step = static_cast<double>(SECONDS_PER_STEP);
    double maximum_path = 0.0;
    for (const TranslatingStructurePaintable* mover : this->horizontal_structures_)
    {
        const HorizontalMotionSpec spec = mover->motion_state().spec;
        HorizontalMotion::validate_spec(spec);
        maximum_path = std::max(maximum_path, horizontal_up(
            2.0 * static_cast<double>(spec.travel_x) / static_cast<double>(spec.period_seconds) * step));
    }
    const HorizontalBox world = horizontal_box(*this->scene_->bounds());
    for (const Player* player : this->players_)
    {
        const RectangleF& body = player->rectangle();
        const HorizontalBox actual = horizontal_box(body);
        const HorizontalPlayerSpeed speed = horizontal_player_speed(*player, this->tuning_->player());
        const double personal_x = horizontal_up(speed.x * step);
        const double personal_y = horizontal_up(speed.y * step);
        const double x_scale = horizontal_finite(std::max({ std::abs(world.left), std::abs(world.right),
            std::abs(actual.left), std::abs(actual.right) }) + personal_x + maximum_path);
        const double y_scale = horizontal_finite(std::max({ std::abs(world.top), std::abs(world.bottom),
            std::abs(actual.top), std::abs(actual.bottom) }) + personal_y);
        const double x_ulp = horizontal_ulp(horizontal_finite(2.0 * x_scale));
        const double y_ulp = horizontal_ulp(horizontal_finite(2.0 * y_scale));
        const double personal = horizontal_up(personal_x + 4.0 * x_ulp);
        const double vertical = horizontal_up(personal_y + 4.0 * y_ulp);
        const double body_width = std::min(static_cast<double>(body.width), actual.right - actual.left) - 2.0 * x_ulp;
        const double body_height = std::min(static_cast<double>(body.height), actual.bottom - actual.top) - 2.0 * y_ulp;
        for (std::size_t index = 0; index < this->horizontal_structures_.size(); ++index)
        {
            const TranslatingStructurePaintable& mover = *this->horizontal_structures_[index];
            const HorizontalMotionSpec spec = mover.motion_state().spec;
            const HorizontalPreparedStep probe = prepared == nullptr ? mover.prepare_motion(SECONDS_PER_STEP) :
                prepared->motions[index];
            const HorizontalMotionStep& collision = probe.collision;
            horizontal_box(collision.before); horizontal_box(collision.after);
            if (collision.segment_count > collision.segments.size())
            { throw std::invalid_argument("Horizontal motion preparation has too many segments."); }
            double observed_path = 0.0;
            for (std::uint32_t segment = 0; segment < collision.segment_count; ++segment)
            {
                const HorizontalMotionSegment& part = collision.segments[segment];
                const RectangleF from = horizontal_pose(spec, part.from_x);
                const RectangleF to = horizontal_pose(spec, part.to_x);
                observed_path = horizontal_finite(observed_path + std::abs(static_cast<double>(to.x) - from.x));
            }
            const float carry = collision.after.x - collision.before.x;
            horizontal_finite(carry);
            const double ideal = horizontal_up(2.0 * static_cast<double>(spec.travel_x) /
                static_cast<double>(spec.period_seconds) * step);
            // Four segment endpoints each incur offset/pose rounding; reserve
            // full ulps also for a copied carry subtraction and body addition.
            // The actual prepared WORLD path is included if it is larger.
            const double movement = horizontal_up(std::max({ ideal, observed_path,
                std::abs(static_cast<double>(carry)) }) + 16.0 * x_ulp);
            const double mover_width = static_cast<double>(spec.anchor.width) - 2.0 * x_ulp;
            const double mover_height = static_cast<double>(spec.anchor.height) - 2.0 * y_ulp;
            if (!(personal + movement < std::min(body_width, mover_width)))
            { throw std::invalid_argument("Horizontal motion relative horizontal displacement is unsafe."); }
            const double height_bound = horizontal_finite(body_height + mover_height);
            if (!(body_height > 0.0) || !(mover_height > 0.0) || !(vertical < height_bound))
            { throw std::invalid_argument("Horizontal motion relative vertical displacement is unsafe."); }
        }
    }
}

Level::HorizontalFrame Level::prepare_horizontal_frame(float dt) const
{
    HorizontalMotion::validate_dt(dt);
    this->validate_horizontal_admission();
    HorizontalFrame frame;
    frame.motions.reserve(this->horizontal_structures_.size());
    for (const TranslatingStructurePaintable* mover : this->horizontal_structures_)
    { frame.motions.push_back(mover->prepare_motion(dt)); }
    frame.carriers.resize(this->players_.size());
    this->validate_horizontal_budget(&frame);
    for (std::size_t player = 0; player < this->players_.size(); ++player)
    {
        const HorizontalMotionId selected = this->players_[player]->carrier_id();
        if (selected == 0) { continue; }
        for (std::size_t index = 0; index < this->horizontal_structures_.size(); ++index)
        {
            if (this->horizontal_structures_[index]->motion_id() != selected) { continue; }
            const HorizontalMotionStep& step = frame.motions[index].collision;
            frame.carriers[player] = HorizontalCarryStep{ selected, step.before, step.after,
                step.after.x - step.before.x };
            break;
        }
    }
    return frame;
}

void Level::commit_horizontal_frame(const HorizontalFrame& prepared) noexcept
{
    for (std::size_t index = 0; index < this->horizontal_structures_.size(); ++index)
    { this->horizontal_structures_[index]->commit_motion(prepared.motions[index]); }
    for (std::size_t index = 0; index < this->players_.size(); ++index)
    { this->players_[index]->prepare_carrier_step(prepared.carriers[index]); }
}

void Level::publish_horizontal_zero() noexcept
{
    for (TranslatingStructurePaintable* mover : this->horizontal_structures_)
    { mover->publish_zero_step(); }
    for (Player* player : this->players_)
    {
        std::optional<HorizontalCarryStep> zero;
        for (const TranslatingStructurePaintable* mover : this->horizontal_structures_)
        {
            if (mover->motion_id() == player->carrier_id())
            {
                const RectangleF current = mover->bounds();
                zero = HorizontalCarryStep{ mover->motion_id(), current, current, 0.0f };
                break;
            }
        }
        player->prepare_carrier_step(zero);
    }
}

std::vector<HorizontalStructureState> Level::horizontal_structure_states() const
{
    std::vector<HorizontalStructureState> result;
    result.reserve(this->horizontal_structures_.size());
    for (const TranslatingStructurePaintable* mover : this->horizontal_structures_)
    {
        HorizontalStructureState state;
        state.id = mover->motion_id();
        state.motion = mover->motion_state();
        // History is persisted pose state, never the transient zero collision step.
        state.previous_bounds = horizontal_pose(state.motion.spec, state.motion.previous_offset.x);
        state.current_bounds = horizontal_pose(state.motion.spec, state.motion.current_offset.x);
        mover->append_anchor_paint_surfaces(state.local_paint);
        result.push_back(std::move(state));
    }
    return result;
}
