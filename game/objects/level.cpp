#include "game/pch.h"
#include "game/objects/level.h"

using namespace mattmath;
using namespace DirectX;
using namespace artattack;

Level::Level(std::unique_ptr<Scene> scene,
	std::vector<Player*> players,
	std::vector<std::unique_ptr<GameObject>> viewport_dividers,
	const TeamColour& team_colours,
	const RectangleF& camera_bounds,
	const RectangleF& zoom_out_start_bounds,
	const RectangleF& zoom_out_finish_bounds,
	const GameTuning* tuning,
	const Presentation* presentation,
	const std::string& sound_bank_name,
	const std::string& music_name,
	float music_volume,
	const ResolutionManager* resolution_manager,
	const ViewportManager* viewport_manager,
	RenderResources* render_resources,
	const AudioResources* audio_resources) :
	scene_(std::move(scene)),
	players_(std::move(players)),
	viewport_dividers_(std::move(viewport_dividers)),
	music_volume_(music_volume),
	resolution_manager_(resolution_manager),
	viewport_manager_(viewport_manager),
	team_colours_(team_colours),
	tuning_(tuning),
	camera_bounds_(camera_bounds)
{
	const MatchTuning& match = tuning->match();

	this->timer_ = match.duration;
	this->start_timer_ = match.countdown;
	this->zoom_out_timer_ = match.zoom_out;
	this->overview_timer_ = match.overview;

	// The whole back buffer, which is what the overview is drawn into - the
	// pull-back is the one shot with no split.
	const Viewport screen(
		RectangleF(Vector2F::ZERO, resolution_manager->resolution_vec()));

	this->zoom_out_start_camera_ = Camera::frame(zoom_out_start_bounds, screen);
	this->zoom_out_finish_camera_ =
		Camera::frame(zoom_out_finish_bounds, screen);

	this->debug_text_ = std::make_unique<DebugText>(
		render_resources, resolution_manager,
		&presentation->debug_overlay());
	this->interface_gameplay_ = std::make_unique<InterfaceGameplay>(
		render_resources, &presentation->hud());
	this->sound_bank_ = audio_resources->sound_bank(sound_bank_name);

	this->music_ = this->sound_bank_->resolve_effect(music_name);
	this->zoom_out_sound_ =
		this->sound_bank_->resolve_effect(match.zoom_out_sound.name);
	this->countdown_sound_ =
		this->sound_bank_->resolve_wave(match.countdown_sound.name);

	this->build_opening_frame(render_resources);
}

Level::~Level()
{
	this->sound_bank_->stop_effect(this->music_, true);
	this->sound_bank_->stop_effect(this->zoom_out_sound_, true);
	this->stop_player_sounds();
}

void Level::build_opening_frame(RenderResources* render_resources)
{
	// The builder added every object to the scene and every add is pending
	// until a tick ends, so the scene has a tick to end before it has had one.
	// The frame drawn before the first update() is a real frame.
	this->scene_->end_tick();
	this->projectile_count_ = this->count_projectiles();

	for (size_t i = 0; i < this->players_.size(); ++i)
	{
		Player* player = this->players_[i];
		Camera camera = this->camera_tools_.calculate_camera(
			player->center(),
			this->viewport_manager_->player_viewport(
				static_cast<int>(i)).size(),
			player->camera(),
			this->camera_bounds_);
		player->set_camera(camera);
	}

	// The first beat, off the same number the timer starts at: the opening
	// text used to be a separate constant, so a countdown retuned to five
	// seconds would have opened on a "3".
	const MatchTuning& match = this->tuning_->match();
	this->countdown_text_ = std::make_unique<TextDropShadow>(
		std::to_wstring(static_cast<int>(match.countdown)),
		match.countdown_font,
		Vector2F::ZERO,
		render_resources,
		match.countdown_colour,
		match.countdown_shadow_colour,
		match.countdown_shadow_offset,
		match.countdown_scale,
		match.countdown_scale);
	this->centre_countdown_text();

	this->refresh_views();
}

void Level::set_countdown_text(const std::wstring& text)
{
	this->countdown_text_->set_text(text);

	// Every beat is a different width, so the centring is redone with the
	// text. "3" and "GO!" are 212 and 752 pixels wide in this font at this
	// scale, and both used to be placed by subtracting half of a 400x600 box
	// nothing had measured: the digits sat 94px left of centre and the last
	// beat jumped 270px right, every match, at every resolution.
	this->centre_countdown_text();
}

void Level::centre_countdown_text()
{
	const Vector2F resolution = this->resolution_manager_->resolution_vec();
	const Vector2F size = this->countdown_text_->text_bounds().size();

	this->countdown_text_->set_position((resolution - size) / 2.0f);
}

void Level::suspend() const
{
	this->sound_bank_->pause_effect(this->music_);
	this->sound_bank_->pause_effect(this->zoom_out_sound_);
	this->pause_player_sounds();
}

void Level::resume() const
{
	this->sound_bank_->resume_effect(this->music_);
	this->sound_bank_->resume_effect(this->zoom_out_sound_);
	this->resume_player_sounds();
}

void Level::update(const std::vector<PlayerInputData>& player_inputs, float dt)
{
	this->frame_dt_ = dt;
	this->update_state(player_inputs, dt);

	// Last, and on every path: the views describe the state the tick left
	// behind, and draw() picks its overlay off the same state_ this saw.
	this->refresh_views();
}

void Level::update_state(const std::vector<PlayerInputData>& player_inputs,
	float dt)
{
	if (this->state_ == LevelState::start_countdown ||
		this->start_timer_ > -1.0f)
	{
		// The first update, and now the only thing left in it: the cameras and
		// the countdown text are built with the level, because the frame drawn
		// before this one is a real frame and used to be skipped rather than
		// made correct.
		if (this->start_timer_ >= this->tuning_->match().countdown)
		{
			this->sound_bank_->play_effect(this->music_, true, this->music_volume_);
			this->sound_bank_->play_wave(this->countdown_sound_,
				this->tuning_->match().countdown_sound.volume);
		}
		this->start_timer_ -= dt;
		if (this->start_timer_ > 0.0f)
		{
			this->set_countdown_text(std::to_wstring(
				static_cast<int>(this->start_timer_) + 1));
		}
		else
		{
			this->set_countdown_text(L"GO!");
		}

		if (this->start_timer_ <= 0.0f)
		{
			this->state_ = LevelState::active;
		}
		else
		{
			return;
		}
	}
	if (this->state_ == LevelState::active)
	{
		this->update_level_logic(player_inputs, dt);

		this->timer_ -= dt;
		if (this->timer_ <= 0.0f)
		{
			this->state_ = LevelState::zoom_out;

			// The pull-back starts on this tick, and the arm below that works
			// the camera out is an `else if` - so it does not run on the tick
			// the state is entered, while refresh_views() at the end of
			// update() does. Seeding it here is what stops the first zoom-out
			// frame being drawn through DEFAULT_CAMERA, which is world (0,0)
			// at 1:1 and empty background on every stage. It is exactly what
			// that arm would produce: the ratio is zero until the timer has
			// been decremented once.
			this->zoom_out_camera_ = this->zoom_out_start_camera_;

			this->sound_bank_->stop_effect(this->music_, true);
			this->stop_player_sounds();
			this->sound_bank_->play_effect(this->zoom_out_sound_, false,
				this->tuning_->match().zoom_out_sound.volume);
		}
	}
	else if (this->state_ == LevelState::zoom_out)
	{
		this->zoom_out_timer_ -= dt;

		if (this->zoom_out_timer_ <= 0.0f)
		{
			this->state_ = LevelState::overview;
			this->zoom_out_timer_ = 0.0f;
			this->sound_bank_->stop_effect(this->zoom_out_sound_, true);
		}

		// Both endpoints are fixed for the whole match - the bounds come out
		// of the level file and the resolution does not change mid-zoom - so
		// they are worked out when the level is built, not per frame. Only
		// the ratio between them varies.
		this->zoom_out_camera_ = Camera::calculate_intermediate_camera(
			this->zoom_out_start_camera_, this->zoom_out_finish_camera_,
			this->zoom_out_camera_ratio());
	}
	else if (this->state_ == LevelState::overview)
	{
		this->overview_timer_ -= dt;
		if (this->overview_timer_ <= 0.0f)
		{
			this->state_ = LevelState::finished;
		}
	}
	// No arm for LevelState::finished. state_ only becomes it in the arm
	// directly above, in the same chain, so it could never be read on the tick
	// it was written - and GameLevel::update sees finished and goes to the
	// results menu instead of calling update() again. The teardown it used to
	// hold (stopping the music) has already happened on the way into zoom_out,
	// and ~Level does it again.

}

float Level::zoom_out_camera_ratio() const
{
	// A pull-back tuned to zero is 0/0: the arm above zeroes zoom_out_timer_
	// on the last step before asking for the ratio, and the NaN that comes
	// back is lerped into zoom_out_camera_, installed as the only view for the
	// rest of the match and never recomputed. A zoom with no duration is a
	// zoom that is already over, so it answers 1 and lands on the finish
	// camera. Camera::frame guards its own division the same way. The proper
	// home for this is a range check where the tuning is read.
	const float duration = this->tuning_->match().zoom_out;
	if (duration <= 0.0f)
	{
		return 1.0f;
	}

	return 1.0f - (this->zoom_out_timer_ / duration);
}

void Level::update_level_logic(const std::vector<PlayerInputData>& player_inputs,
	float dt)
{
	// The whole of a tick, in the order the scene names its phases. What this
	// replaces was a hundred lines of loops over three object lists - two
	// update sweeps, a collidables rebuild, the collision dispatch, an
	// out-of-bounds pass and a swap-and-pop compaction - none of which was
	// about paint.
	this->apply_player_inputs(player_inputs);

	this->scene_->update(dt);

	this->update_players(dt);

	this->scene_->resolve();

	// "I touched no ground this tick" is the absence of a contact, so it is
	// only knowable once they have all been dispatched.
	for (Player* player : this->players_)
	{
		player->end_contacts();
	}

	this->update_players_after_resolve();

	// Retires what left the arena or died, and admits the projectiles fired
	// above - which is why a projectile's first sweep is the next tick's and
	// not this one's. It has to be: the players and their projectiles are one
	// list now, so a push from inside the loop walking it would invalidate the
	// walk.
	this->scene_->end_tick();

	// Once per tick, for the debug overlay. It used to be once per view, from
	// inside the draw.
	this->projectile_count_ = this->count_projectiles();

	// A player leaving the level is a genuine simulation failure, not a
	// gameplay event, so it still reports loudly rather than being retired with
	// the projectiles that missed.
	for (const Player* player : this->players_)
	{
		if (!this->scene_->in_bounds(*player))
		{
			throw std::runtime_error("Player out of bounds");
		}
	}
}

void Level::apply_player_inputs(
	const std::vector<PlayerInputData>& player_inputs)
{
	for (Player* player : this->players_)
	{
		const int slot = player->player_num();

		if (slot >= 0 && static_cast<size_t>(slot) < player_inputs.size())
		{
			player->set_player_input(player_inputs[slot]);
		}
		else
		{
			player->set_player_input(PlayerInputData());
		}
	}
}

void Level::update_players(float dt)
{
	for (size_t i = 0; i < this->players_.size(); ++i)
	{
		Player* player = this->players_[i];
		Camera camera = this->camera_tools_.calculate_camera(
			player->center(),
			this->viewport_manager_->player_viewport(
				static_cast<int>(i)).size(),
			player->camera(),
			this->camera_bounds_);
		player->set_camera(camera);

		for (std::unique_ptr<CollisionObject>& projectile :
			player->update_weapon_and_get_projectiles(dt))
		{
			this->scene_->add(std::move(projectile));
		}
	}
}

void Level::update_players_after_resolve()
{
	for (Player* player : this->players_)
	{
		player->update_weapon_position();
		player->update_prev_rectangle();
	}
}

void Level::refresh_views()
{
	this->scene_->clear_views();

	if (this->state_ == LevelState::start_countdown ||
		this->state_ == LevelState::active)
	{
		// One pane per player, in player order - which is the order the
		// overlay reads players_ back in. The pane index is that position,
		// never player_num(): the pad slot is a different number the moment
		// the pads driving a match are not 0..n-1.
		for (size_t i = 0; i < this->players_.size(); ++i)
		{
			const Player* player = this->players_[i];
			this->scene_->add_view(
				this->viewport_manager_->player_viewport(
					static_cast<int>(i)),
				player->camera());
		}
	}
	else
	{
		// One fullscreen view, and it is the whole of what "fullscreen" now
		// costs. Getting here used to mean calling
		// ViewportManager::set_layout(one_player) from inside a draw -
		// permanently clobbering shared presentation state, never restoring it,
		// so restarting a 2-4 player match rendered every camera into one pane
		// with no split-screen dividers. Saying it on the view list says it for
		// this frame and no further.
		this->scene_->add_view(this->viewport_manager_->fullscreen_viewport(),
			this->zoom_out_camera_);
	}
}

void Level::stop_player_sounds() const
{
	for (const Player* player : this->players_)
	{
		player->stop_sounds();
	}
}

void Level::pause_player_sounds() const
{
	for (const Player* player : this->players_)
	{
		player->pause_sounds();
	}
}

void Level::resume_player_sounds() const
{
	for (const Player* player : this->players_)
	{
		player->resume_sounds();
	}
}

void Level::draw(Renderer& renderer) const
{
	if (this->state_ == LevelState::start_countdown ||
		this->state_ == LevelState::active)
	{
		this->scene_->draw(renderer,
			[this](int view_index, DrawList& list)
			{
				this->draw_player_overlay(view_index, list);
			});
	}
	else
	{
		// The overview has no HUD, no dividers and no countdown: one fullscreen
		// view of the world, and nothing over it.
		this->scene_->draw(renderer);
	}
}

void Level::draw_player_overlay(int view_index, DrawList& list) const
{
	const Player* player = this->players_[static_cast<size_t>(view_index)];

	// The pane the scene has just drawn the world into, rather than a second
	// question to the layout for the rectangle it was handed one of.
	const Viewport& player_vp = this->scene_->view(view_index).viewport;

	// Everything from here down goes over the world, and this is the case
	// renderer.h describes for the camera being per draw-range rather than per
	// view: same viewport, same frame, three different mappings.
	//
	// The dividers are authored in screen coordinates, so they are drawn
	// through a camera that subtracts this pane's screen origin.
	list.set_camera(Camera(player_vp));

	for (const std::unique_ptr<GameObject>& divider : this->viewport_dividers_)
	{
		divider->draw(list);
	}

	// The HUD is already in pane-local coordinates - it is laid out from the
	// viewport's own size - so it wants the identity. Both of these used to be
	// a bare SpriteBatch::Begin() with no camera anywhere near them, which is
	// what the identity means.
	list.set_camera(Camera::DEFAULT_CAMERA);

	this->interface_gameplay_->draw_gameplay_interface(list,
		player_vp.size(),
		player->health(),
		player->weapon_ammo(),
		this->timer_,
		player->team_colour(),
		player->respawn_timer(),
		player->state() == PlayerState::dead);

	if (player->showing_debug())
	{
		this->debug_text_->draw_debug_info(list, player,
			this->projectile_count_, this->frame_dt_);
	}

	// The countdown is positioned in full-screen coordinates, and it used to be
	// drawn once per pane through that pane's camera so that the panes
	// reassembled one centred countdown between them. That only works while the
	// panes tile the whole back buffer, and the three-player layout does not:
	// it puts players 0, 1 and 2 in the top-left, bottom-right and bottom-left
	// quadrants and leaves the top-right one belonging to nobody, so nothing
	// ever rasterised it and the top-right corner of every "3", "2", "1" and
	// "GO!" was missing.
	//
	// Once, over the whole screen, from the last view - whose command list is
	// executed after every other pane's (renderer.h, submit), so it still lands
	// over all of them, which is where it was before. The viewport goes back so
	// this leaves the list as it found it.
	if (view_index == this->scene_->view_count() - 1 &&
		(this->state_ == LevelState::start_countdown ||
			this->start_timer_ > -1.0f))
	{
		list.set_viewport(this->viewport_manager_->fullscreen_viewport());
		list.set_camera(Camera::DEFAULT_CAMERA);
		this->draw_countdown_text(list);
		list.set_viewport(player_vp);
	}
}

LevelState Level::state() const
{
	return this->state_;
}

int Level::count_projectiles() const
{
	int count = 0;
	for (const CollisionObject* object : this->scene_->collision_objects())
	{
		// The ten-way comparison written out here is is_projectile(), which
		// already existed two files away.
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

	// Which teams were in the match, asked of the players rather than of the
	// settings, because this is the object that holds them and a team with no
	// player in it is exactly what the results screen must not report on.
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
		if (to_collision_type(object->tag()) ==
			CollisionObjectType::structure_paintable)
		{
			// dynamic_cast on a POINTER returns nullptr on failure; only the
			// reference form throws bad_cast. The catch here could never fire,
			// and the null was dereferenced unchecked one line above it.
			auto paintable_object = dynamic_cast<PaintableObject*>(object);

			if (paintable_object == nullptr)
			{
				throw std::runtime_error(
					"Object tagged STRUCTURE_PAINTABLE is not an PaintableObject");
			}

			PaintTotal paint = paintable_object->paint_total();

			result.team_a_area += paint.team_a;
			result.team_b_area += paint.team_b;
			result.total_area += paint.total;
		}
	}

	return result;
}

void Level::draw_countdown_text(DrawList& draw_list) const
{
	// Linear, because this is the one thing on screen that is not pixel art:
	// a 144pt face scaled by two. It was a bare SpriteBatch::Begin() before,
	// which is DirectXTK's LinearClamp by default - the only place in the
	// level that took it, and now the only place that asks for it. The list
	// goes back to point so the next range is not surprised by it.
	draw_list.set_filter(TextureFilter::linear);
	this->countdown_text_->draw(draw_list);
	draw_list.set_filter(TextureFilter::point);
}
