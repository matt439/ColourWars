#include "game/pch.h"
#include "game/objects/level.h"

using namespace mattmath;
using namespace labrador;

// The opening before the split (docs/backstory.md, "Opening"): the person's
// monitor, the person in front of it, the two colours tried on the two halves
// and undone, the person leaving, then the countdown in the picture and the
// push through the glass. None of it touches the simulation: the players are
// not updated until the countdown is over, as before.

namespace
{
	RectangleF lerp(const RectangleF& from, const RectangleF& to, float amount)
	{
		return RectangleF(from.x + (to.x - from.x) * amount,
			from.y + (to.y - from.y) * amount,
			from.width + (to.width - from.width) * amount,
			from.height + (to.height - from.height) * amount);
	}

	Vector2F lerp(const Vector2F& from, const Vector2F& to, float amount)
	{
		return from + (to - from) * amount;
	}

	float smooth(float amount)
	{
		const float t = std::clamp(amount, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	// How far into a span of `length` seconds starting at `start` the opening is.
	float through(float time, float start, float length)
	{
		return length > 0.0f ? std::clamp((time - start) / length, 0.0f, 1.0f) :
			(time >= start ? 1.0f : 0.0f);
	}

	// Of each preview: the cursor reaches the swatch, then the colour shows.
	constexpr float reach = 0.4f;
}

Level::OpeningBeat Level::opening_beat() const
{
	const OpeningTuning& timing = this->tuning_->match().opening;
	const float time = this->opening_time_;
	const float first = timing.settle;
	const float second = first + timing.preview;
	const float undo = second + timing.preview;
	const float leave = undo + timing.undo;

	OpeningBeat beat;
	const float a = through(time, first, timing.preview);
	const float b = through(time, second, timing.preview);
	beat.previewing_a = time >= first && time < second && a >= reach;
	beat.previewing_b = time >= second && time < undo && b >= reach;
	beat.picker = time >= first && time < undo;
	beat.cursor_at_a = smooth(a / reach);
	beat.cursor_at_b = smooth(b / reach);
	beat.cursor_back = smooth(through(time, undo, timing.undo));

	// The sigh, then out of the chair, then out of the room.
	const float leaving = through(time, leave, timing.leave);
	beat.pose.sigh = std::sin(std::clamp(leaving / 0.35f, 0.0f, 1.0f) * 3.14159265f);
	beat.pose.stand = smooth((leaving - 0.35f) / 0.25f);
	beat.pose.leave = smooth((leaving - 0.6f) / 0.4f);
	return beat;
}

float Level::opening_length() const
{
	const OpeningTuning& timing = this->tuning_->match().opening;
	return timing.settle + timing.preview * 2.0f + timing.undo + timing.leave;
}

bool Level::intro_showing() const
{
	return this->state_ == LevelState::opening ||
		(this->state_ == LevelState::start_countdown && this->intro_countdown_);
}

bool Level::intro_playing() const
{
	return this->intro_showing();
}

float Level::push_ratio() const
{
	if (this->state_ != LevelState::start_countdown || !this->intro_countdown_)
	{
		return 0.0f;
	}
	const float push = this->tuning_->match().opening.push;
	return smooth(1.0f - this->start_timer_ / push);
}

float Level::glass_push() const
{
	if (this->reveal_showing() && this->state_ == LevelState::zoom_out)
	{
		// Back out of the panes into the glass, the push in reverse.
		return 1.0f - smooth(this->zoom_out_camera_ratio());
	}
	return this->push_ratio();
}

RectangleF Level::intro_screen() const
{
	// The glass grows to fill the window as the view pushes through it.
	const RectangleF window(Vector2F::ZERO, this->resolution_manager_->resolution_vec());
	return this->monitor_->screen_for_glass(
		lerp(this->monitor_->glass(window), window, this->glass_push()));
}

bool Level::skip_requested(const std::vector<PlayerInputData>& player_inputs) const
{
	// Anyone in the match, by the device slot their input arrives on.
	for (const Player* player : this->players_)
	{
		const size_t slot = static_cast<size_t>(player->player_num());
		if (slot < player_inputs.size() && player_inputs[slot].skip_intro)
		{
			return true;
		}
	}
	return false;
}

void Level::colour_players(bool team_a, bool team_b)
{
	for (Player* player : this->players_)
	{
		player->set_coloured(player->team() == PlayerTeam::a ? team_a : team_b,
			this->presentation_->opening().figure_grey);
	}
}

void Level::skip_intro()
{
	// Straight to the split-screen countdown, from its start if it had not begun.
	this->state_ = LevelState::start_countdown;
	this->intro_countdown_ = false;
	this->colour_players(true, true);
}

void Level::update_opening(const std::vector<PlayerInputData>& player_inputs, float dt)
{
	if (this->skip_requested(player_inputs))
	{
		this->skip_intro();
		return;
	}
	this->opening_time_ += dt;
	if (this->opening_time_ >= this->opening_length())
	{
		// The figures come alive in their colours as the countdown starts.
		this->state_ = LevelState::start_countdown;
		this->intro_countdown_ = true;
		this->colour_players(true, true);
		return;
	}
	// Each team sees itself in colour only while its own half is previewed.
	const OpeningBeat beat = this->opening_beat();
	this->colour_players(beat.previewing_a, beat.previewing_b);
}

void Level::add_intro_views()
{
	const RectangleF screen = this->intro_screen();
	const RectangleF picture = this->monitor_->picture(screen);
	const float push = this->glass_push();
	if (push <= 0.0f)
	{
		const Viewport view(picture);
		this->scene_->add_view(view, Camera::frame(this->picture_bounds_, view));
		return;
	}
	// One view per player, all the whole picture at first, each flying out to
	// its own pane and its own camera.
	for (size_t i = 0; i < this->players_.size(); ++i)
	{
		const RectangleF pane = Viewport(this->viewport_manager_->player_viewport(
			static_cast<int>(i)).pixel_rect()).rectangle();
		const Viewport view(lerp(picture, pane, push));
		this->scene_->add_view(view, Camera::calculate_intermediate_camera(
			Camera::frame(this->picture_bounds_, view), this->players_[i]->camera(), push));
	}
}

void Level::draw_intro(Renderer& renderer) const
{
	const Viewport fullscreen = this->viewport_manager_->fullscreen_viewport();
	const RectangleF window(Vector2F::ZERO, this->resolution_manager_->resolution_vec());
	const RectangleF screen = this->intro_screen();

	// Behind the picture: the scene's first view continues this list.
	if (renderer.view_count() == 0)
	{
		renderer.set_view_count(1);
	}
	DrawList behind = renderer.view(renderer.view_count() - 1);
	behind.set_viewport(fullscreen);
	behind.set_camera(Camera::DEFAULT_CAMERA);
	behind.set_filter(TextureFilter::point);
	this->monitor_->draw(behind, screen);

	// Each view's figures act in that view's own camera.
	this->scene_->draw(renderer, [this](int view, DrawList& list) { this->draw_acting(list, view); });

	// In front: the last view executes after every other.
	DrawList front = renderer.view(renderer.view_count() - 1);
	if (this->reveal_showing())
	{
		this->draw_reveal(front, screen);
		return;
	}
	if (this->state_ != LevelState::opening)
	{
		front.set_viewport(fullscreen);
		front.set_camera(Camera::DEFAULT_CAMERA);
		this->draw_countdown_text(front);
		return;
	}

	const OpeningBeat beat = this->opening_beat();
	if (beat.previewing_a || beat.previewing_b)
	{
		// The preview over its team's half, in the picture's own space.
		const RectangleF picture = this->monitor_->picture(screen);
		const Viewport view(picture);
		const Camera camera = Camera::frame(this->picture_bounds_, view);
		const RectangleF visible = camera.visible_rectangle(view);
		const float middle = std::clamp(this->picture_middle_, visible.x, visible.right());
		const RectangleF half = beat.previewing_a ?
			RectangleF(visible.x, visible.y, middle - visible.x, visible.height) :
			RectangleF(middle, visible.y, visible.right() - middle, visible.height);
		front.set_viewport(view);
		front.set_camera(camera);
		this->monitor_->draw_preview(front,
			half, this->team_colours_.team_colour(
				beat.previewing_a ? PlayerTeam::a : PlayerTeam::b));
	}
	front.set_viewport(fullscreen);
	front.set_camera(Camera::DEFAULT_CAMERA);
	front.set_filter(TextureFilter::point);
	this->monitor_->draw_close_button(front, screen);
	if (beat.picker)
	{
		this->monitor_->draw_picker(front, screen,
			this->team_colours_.team_colour(PlayerTeam::a),
			this->team_colours_.team_colour(PlayerTeam::b));
	}

	// From the middle of the picture to each swatch, then back.
	const RectangleF picture = this->monitor_->picture(screen);
	const Vector2F rest = picture.position() + Vector2F(picture.width * 0.62f, picture.height * 0.4f);
	const Vector2F a = this->monitor_->swatch(screen, 0).center();
	const Vector2F b = this->monitor_->swatch(screen, 1).center();
	Vector2F cursor = lerp(rest, a, beat.cursor_at_a);
	cursor = lerp(cursor, b, beat.cursor_at_b);
	cursor = lerp(cursor, rest, beat.cursor_back);
	this->cursor_->draw(front, cursor, window.height);

	this->silhouette_->draw(front, window, this->monitor_->persona(), beat.pose);
}

// ---------------------------------------------------------------------------
// The reveal (docs/backstory.md, "The reveal"): the views pull back out of the
// panes into the same monitor, the same person comes back and sits down,
// hovers each colour over the whole picture, wavers, and clicks the winner;
// the slot fills and the coverage shows; the person leaves. With no winner
// (in practice, nothing painted) there is nothing to choose: the person
// looks, then closes the program without saving.

bool Level::revealed() const
{
	return this->monitor_ != nullptr;
}

bool Level::reveal_showing() const
{
	return this->monitor_ != nullptr && (this->state_ == LevelState::zoom_out ||
		this->state_ == LevelState::overview || this->state_ == LevelState::finished);
}

float Level::reveal_length() const
{
	const RevealTuning& timing = this->tuning_->match().reveal;
	// Once the program is closed nobody is left to celebrate.
	const float react = this->winner_ == PlayerTeam::none ? 0.0f : timing.react;
	return timing.walk_in + timing.hover * 2.0f + timing.decide + timing.show + timing.leave +
		react;
}

void Level::start_reveal()
{
	if (this->monitor_ == nullptr)
	{
		return;
	}
	// The coverage is counted once, now, and is what the click and the
	// results agree on.
	this->end_info_ = this->level_end_info();
	this->winner_ = this->end_info_.winning_team();
	this->reveal_time_ = 0.0f;
}

Level::RevealBeat Level::reveal_beat() const
{
	const RevealTuning& timing = this->tuning_->match().reveal;
	const float time = this->reveal_time_;
	const float first = timing.walk_in;
	const float second = first + timing.hover;
	const float decide = second + timing.hover;
	const float click = decide + timing.decide;
	const float leave = click + timing.show;

	RevealBeat beat;
	const float walking = through(time, 0.0f, timing.walk_in);
	beat.pose.leave = 1.0f - smooth(walking / 0.6f);
	beat.pose.stand = 1.0f - smooth((walking - 0.6f) / 0.4f);
	const float leaving = through(time, leave, timing.leave);
	if (leaving > 0.0f)
	{
		// Leans back out of the chair, then goes.
		beat.pose.stand = smooth(leaving / 0.4f);
		beat.pose.leave = smooth((leaving - 0.4f) / 0.6f);
	}

	const float a = through(time, first, timing.hover);
	const float b = through(time, second, timing.hover);
	beat.cursor_at_a = smooth(a / reach);
	beat.cursor_at_b = smooth(b / reach);
	beat.showing_a = time >= first && time < second && a >= reach;
	beat.showing_b = time >= second && time < decide && b >= reach;
	beat.picker = time >= first;
	// Back and forth between the two, faster as it goes, until the click.
	const float wavering = through(time, decide, timing.decide);
	beat.waver = time >= decide && time < click ?
		0.5f - 0.5f * std::cos(wavering * wavering * 3.14159265f * 5.0f) : 0.0f;
	beat.clicked = time >= click;
	return beat;
}

void Level::update_reveal(float dt)
{
	const RevealBeat before = this->reveal_beat();
	this->reveal_time_ += dt;
	const RevealBeat after = this->reveal_beat();
	const MatchTuning& match = this->tuning_->match();
	const float gain = this->preferences_->sfx_gain();
	if ((after.showing_a && !before.showing_a) || (after.showing_b && !before.showing_b))
	{
		this->sound_bank_->play_wave(this->hover_sound_, match.hover_sound.volume * gain);
	}
	if (after.clicked && !before.clicked)
	{
		this->sound_bank_->play_wave(this->click_sound_, match.click_sound.volume * gain);
	}
	if (this->reveal_time_ >= this->reveal_length())
	{
		this->state_ = LevelState::finished;
	}
}

void Level::draw_reveal(DrawList& front, const RectangleF& screen) const
{
	if (this->state_ == LevelState::zoom_out)
	{
		return;
	}
	const RectangleF window(Vector2F::ZERO, this->resolution_manager_->resolution_vec());
	const Viewport fullscreen = this->viewport_manager_->fullscreen_viewport();
	const RevealBeat beat = this->reveal_beat();
	const Colour colour_a = this->team_colours_.team_colour(PlayerTeam::a);
	const Colour colour_b = this->team_colours_.team_colour(PlayerTeam::b);
	const RectangleF picture = this->monitor_->picture(screen);

	const bool closing = this->winner_ == PlayerTeam::none;
	if (!closing && (beat.showing_a || beat.showing_b))
	{
		// The whole picture this time: the person is deciding, not trying.
		const Viewport view(picture);
		const Camera camera = Camera::frame(this->picture_bounds_, view);
		front.set_viewport(view);
		front.set_camera(camera);
		this->monitor_->draw_preview(front, camera.visible_rectangle(view),
			beat.showing_a ? colour_a : colour_b);
	}
	front.set_viewport(fullscreen);
	front.set_camera(Camera::DEFAULT_CAMERA);
	front.set_filter(TextureFilter::point);
	const Vector2F rest = picture.position() + Vector2F(picture.width * 0.62f, picture.height * 0.4f);
	if (closing)
	{
		// Stares at the empty picture, moves to the close button, hesitates.
		const Vector2F close = this->monitor_->close_button(screen).center();
		const Vector2F cursor = beat.clicked ? close :
			lerp(rest, close, beat.cursor_at_b);
		if (beat.clicked)
		{
			this->monitor_->draw_closed(front, screen);
		}
		else
		{
			this->monitor_->draw_close_button(front, screen);
		}
		this->cursor_->draw(front, cursor, window.height);
		this->silhouette_->draw(front, window, this->monitor_->persona(), beat.pose);
		return;
	}

	this->monitor_->draw_close_button(front, screen);
	if (beat.picker)
	{
		this->monitor_->draw_picker(front, screen, colour_a, colour_b);
	}
	if (beat.clicked)
	{
		this->monitor_->draw_saved(front, screen, this->team_colours_.team_colour(this->winner_));
	}

	const Vector2F a = this->monitor_->swatch(screen, 0).center();
	const Vector2F b = this->monitor_->swatch(screen, 1).center();
	Vector2F cursor = lerp(rest, a, beat.cursor_at_a);
	cursor = lerp(cursor, b, beat.cursor_at_b);
	cursor = lerp(cursor, a, beat.waver);
	if (beat.clicked)
	{
		cursor = this->winner_ == PlayerTeam::a ? a : b;
	}

	if (beat.clicked)
	{
		// The coverage, each half's number in its own colour.
		const OpeningPresentation& look = this->presentation_->opening();
		front.set_filter(TextureFilter::linear);
		const std::wstring texts[2] = { this->end_info_.team_a_percentage_string(),
			this->end_info_.team_b_percentage_string() };
		const Colour colours[2] = { colour_a, colour_b };
		for (int side = 0; side < 2; ++side)
		{
			const Vector2F size = this->render_resources_->measure_text(this->percent_font_, texts[side]);
			if (size.y <= 0.0f)
			{
				continue;
			}
			const float scale = look.percent_height * window.height / size.y;
			const Vector2F centre(picture.x + picture.width * (side == 0 ? 0.25f : 0.75f),
				picture.y + picture.height * 0.32f);
			const Vector2F at = centre - size * scale * 0.5f;
			const float drop = std::max(2.0f, scale * size.y * 0.05f);
			front.draw_text(this->percent_font_, texts[side], at + Vector2F(drop, drop),
				look.percent_shadow, scale, 0.0f, Vector2F::ZERO, 0.0f);
			front.draw_text(this->percent_font_, texts[side], at, colours[side],
				scale, 0.0f, Vector2F::ZERO, 0.0f);
		}
		front.set_filter(TextureFilter::point);
	}

	this->cursor_->draw(front, cursor, window.height);
	this->silhouette_->draw(front, window, this->monitor_->persona(), beat.pose);
}

// ---------------------------------------------------------------------------
// The figures' acting: as they come alive they look at their hands, then
// across at the other side; while the person is back they freeze, but cheat;
// alone again, the winners celebrate and the losers slump.

void Level::update_acting(float dt)
{
	if (this->acting_ == nullptr)
	{
		return;
	}
	FigureAct act = FigureAct::none;
	if (this->state_ == LevelState::start_countdown && this->intro_countdown_)
	{
		const float countdown = this->tuning_->match().countdown;
		const float alive = countdown > 0.0f ? 1.0f - this->start_timer_ / countdown : 1.0f;
		act = alive < 0.35f ? FigureAct::look_at_hands :
			alive < 0.7f ? FigureAct::look_across : FigureAct::none;
	}
	else if (this->state_ == LevelState::overview || this->state_ == LevelState::finished)
	{
		const RevealTuning& timing = this->tuning_->match().reveal;
		const bool alone = this->reveal_time_ >= this->reveal_length() - timing.react;
		// A draw is nobody's to celebrate.
		act = !alone ? FigureAct::cheat :
			this->winner_ != PlayerTeam::none ? FigureAct::celebrate : FigureAct::none;
	}
	if (this->state_ != LevelState::finished)
	{
		this->act_time_ += dt;
	}

	const OpeningPresentation& look = this->presentation_->opening();
	this->acts_.assign(this->players_.size(), FigureAct::none);
	for (size_t view = 0; view < this->players_.size(); ++view)
	{
		Player* player = this->players_[view];
		FigureAct mine = act;
		if (mine == FigureAct::celebrate && this->winner_ != PlayerTeam::none &&
			player->team() != this->winner_)
		{
			mine = FigureAct::slump;
		}
		this->acts_[view] = mine;

		// Each figure out of step with the others.
		const float time = this->act_time_ + static_cast<float>(view) * 0.37f;
		const float height = player->rectangle().height;
		FigurePose pose;
		pose.whole = this->reveal_showing();
		switch (mine)
		{
		case FigureAct::cheat:
			// Knees shaking: a jitter of the whole figure, a few times a second.
			pose.offset.x = (static_cast<int>(time * 18.0f) % 2 == 0 ? 1.0f : -1.0f) *
				look.shake * player->rectangle().width;
			break;
		case FigureAct::celebrate:
			// Landed once the results are up.
			pose.offset.y = this->state_ == LevelState::finished ? 0.0f :
				-std::abs(std::sin(time * 3.14159265f * 2.2f)) * look.hop * height;
			pose.tool = false;
			break;
		case FigureAct::slump:
			pose.squash = look.slump;
			break;
		default:
			break;
		}
		player->set_pose(pose);
	}
}

void Level::draw_acting(DrawList& list, int /*view*/) const
{
	if (this->acting_ == nullptr || this->acts_.size() != this->players_.size())
	{
		return;
	}
	for (size_t index = 0; index < this->players_.size(); ++index)
	{
		const Player* player = this->players_[index];
		const FigureAct act = this->acts_[index];
		// The eyes are the original art's; the workshop figures have their own.
		if (act == FigureAct::none ||
			(player->state() == PlayerState::dead && !player->drawn_whole()) ||
			player->workshop_visual() != nullptr)
		{
			continue;
		}
		const RectangleF figure = player->posed_rectangle();
		const bool right = player->facing_right();
		const float time = this->act_time_ + static_cast<float>(index) * 0.37f;
		const float across = player->team() == PlayerTeam::a ? 1.0f : -1.0f;
		switch (act)
		{
		case FigureAct::look_at_hands:
			this->acting_->draw_eyes(list, figure, right, Vector2F(right ? 0.6f : -0.6f, 1.0f));
			break;
		case FigureAct::look_across:
			this->acting_->draw_eyes(list, figure, right, Vector2F(across, 0.0f));
			break;
		case FigureAct::cheat:
			// Darting: one side, then the other, never at the person.
			this->acting_->draw_eyes(list, figure, right,
				Vector2F(std::sin(time * 4.3f) > 0.0f ? 1.0f : -1.0f, -0.2f));
			this->acting_->draw_sweat(list, figure, right, time);
			break;
		case FigureAct::slump:
			this->acting_->draw_eyes(list, figure, right, Vector2F(0.0f, 1.0f));
			break;
		default:
			break;
		}
	}
}
