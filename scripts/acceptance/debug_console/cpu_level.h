#pragma once

// The debug_console case's real Level on the CPU, shared by replay_checks.cpp
// and level_checks.cpp: the world LevelBuilder needs, the match the director
// drives over it, and the keyboard seat's scripted input.
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/objects/level.h"
#include "game/objects/level_builder.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/seat_commands.h"
#include "game/stage_catalog.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/audio/sound_bank.h"
#include "engine/collision/partitioner.h"
#include "engine/core/registry.h"
#include "engine/input/mouse.h"
#include "engine/render/render_resources.h"
#include "engine/render/resolution_manager.h"
#include "engine/render/sprite_font_file.h"
#include "engine/render/viewport_manager.h"
#include <memory>

namespace debug_console_acceptance
{
	// ------------------------------------------- a real Level on the CPU

	// What LevelBuilder needs, with no window or device: sprite sheets and
	// fonts whose atlases are null handles, a silent bank, the levels, and
	// the shell services by value.
	struct CpuWorld
	{
		labrador::RenderResources render;
		labrador::AudioResources audio;
		PlaySettings preferences;
		StageCatalog catalog;
		const labrador::Registry<LevelLoadedInfo>& levels = catalog.level_infos();
		const StageList& stages = catalog.stages();
		labrador::ResolutionManager resolution;
		labrador::ViewportManager viewports{ &resolution };
		labrador::Mouse mouse;
		labrador::Partitioner partitioner;
		std::unique_ptr<LevelBuilder> builder;
		SeatCommandBuilder seat_commands{ &mouse, &viewports };

		CpuWorld(const std::filesystem::path& content, const GameTuning& tuning,
			const Presentation& presentation)
		{
			for (const char* sheet : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
			{
				this->render.add_sprite_sheet(sheet, labrador::read_sprite_sheet(
					(content / "textures" / (std::string(sheet) + ".json")).string().c_str(),
					labrador::TextureHandle{}));
			}
			for (const char* font : { "bowlby_one_144", "lato_black_24",
				"lato_black_36", "lato_black_48", "lato_black_72",
				"lato_black_144", "courier_prime_bold_16" })
			{
				const labrador::SpriteFontFile file = labrador::read_sprite_font_file(
					(content / "fonts" / (std::string(font) + ".spritefont")).string());
				auto loaded = std::make_unique<labrador::Font>(labrador::TextureHandle{}, file.glyphs,
					file.line_spacing);
				if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
				this->render.add_font(font, std::move(loaded));
			}
			this->audio.add_sound_bank("paint_exe", labrador::SoundBank::silent());
			const Stage listed[] = { { "movement_course", "Movement Course", "icon", false },
				{ "close_quarters", "Close Quarters", "icon", true } };
			for (const Stage& stage : listed)
			{
				auto info = std::make_unique<LevelLoadedInfo>(
					(content / "levels" / (stage.name + ".json")).string().c_str());
				info->validate();
				this->catalog.add(StageDefinition{ stage, std::move(info) });
			}
			this->builder = std::make_unique<LevelBuilder>(&this->viewports, &this->render, &this->audio,
				&this->preferences, &this->levels, &tuning, &presentation, &this->resolution, nullptr,
				&this->partitioner);
		}
	};

	// GameLevel's debug face over a real Level: one build path through
	// before_level_build and after_level_build, as GameLevel has.
	class CpuMatch final : public DebugMatchTarget
	{
	public:
		CpuMatch(CpuWorld& world, DebugDirector& director) : world_(world), director_(director) {}

		MatchSetup settings;
		std::unique_ptr<Level> level;
		int builds = 0;
		// What GameLevel would choose; the director may override it.
		OpeningMode opening = OpeningMode::off;

		void build()
		{
			this->level.reset();
			this->director_.before_level_build(*this);
			this->world_.viewports.set_layout(this->settings.screen_layout());
			this->level = this->world_.builder->build_level(this->settings, TeamColourPair::blue_orange,
				this->director_.opening_mode(this->opening));
			++this->builds;
			this->director_.after_level_build(*this);
		}

		// What GameLevel::on_suspend does when a state covers the match.
		void suspend()
		{
			this->level->suspend();
			this->director_.match_suspended(*this);
		}

		const MatchSetup& debug_settings() const override { return this->settings; }
		void debug_rebuild(const MatchSetup& plan, bool) override
		{
			this->settings = plan;
			this->build();
		}
		void debug_adopt(const MatchSetup& plan, bool) override { this->settings = plan; }
		void debug_leave() override {}
		// GameLevel::step_match's place, without its recorder and rumble.
		void step(const std::vector<SlotInput>& inputs, float dt,
			SeatInputFilter* seats)
		{
			this->level->refresh_layout();
			const SeatCommands commands = this->world_.seat_commands.build(*this->level, inputs);
			if (commands.skip_intro) { this->level->request_intro_skip(); }
			this->level->update(commands.seats, dt, seats);
		}
		bool debug_level_finished() const override { return this->level->state() == LevelState::finished; }
		const Level* debug_level() const override { return this->level.get(); }
		// GameLevel's, uncovered and in focus: only the debug clock pauses it,
		// which pauses the voices and leaves the players alone.
		void debug_sync_audio() override
		{
			if (this->director_.match_audio_paused())
			{
				this->level->pause_audio();
			}
			else
			{
				this->level->resume();
			}
		}
		std::vector<std::string> debug_status() const override { return {}; }
		void debug_set_overlays(const LevelOverlays& overlays) override { this->level->set_overlays(overlays); }
		void debug_set_player_text(bool showing) override { this->level->set_players_showing_debug(showing); }
		void debug_set_cheats(bool immune, bool infinite) override
		{
			this->god = immune;
			this->level->set_player_cheats(immune, infinite);
		}
		bool god = false;
		std::vector<LevelStation> debug_stations() const override
		{
			return this->world_.levels.get(this->settings.stage())->stations();
		}
		bool debug_place(int seat, const mattmath::Vector2F& top_left) override
		{
			return this->level->place_player(seat, top_left);
		}
		bool debug_respawn(int seat) override { return this->level->respawn_player(seat); }
		bool debug_kill(int seat) override { return this->level->kill_player(seat); }
		MatchPaint debug_paint() const override
		{
			const LevelEndInfo info = this->level->level_end_info();
			MatchPaint paint;
			paint.team_a = info.team_a_area;
			paint.team_b = info.team_b_area;
			paint.total = info.total_area;
			return paint;
		}
		bool debug_countdown_over() const override
		{
			return this->level->match_started();
		}
		void debug_set_seat_labels(const std::vector<std::wstring>& labels) override
		{
			this->seat_labels = labels;
			this->level->set_seat_labels(labels);
		}
		std::vector<mattmath::RectangleF> debug_screen_obstacles() const override
		{
			return this->level->opening_cards();
		}
		std::vector<std::wstring> seat_labels;

	private:
		CpuWorld& world_;
		DebugDirector& director_;
	};

	// The keyboard seat's own scripted input (the bots drive the rest).
	inline std::vector<SlotInput> keyboard_step(int step)
	{
		std::vector<SlotInput> inputs(input_slots::count);
		SlotInput& keys = inputs[static_cast<std::size_t>(input_slots::keyboard_and_mouse)];
		keys.connected = true;
		keys.x_movement = (step / 90) % 3 == 0 ? 1.0f : (step / 90) % 3 == 1 ? -1.0f : 0.0f;
		keys.jump_held = step % 75 < 12;
		keys.jump_pressed = step % 75 == 0;
		keys.primary_shoot = step % 40 < 25;
		return inputs;
	}
}
#endif
