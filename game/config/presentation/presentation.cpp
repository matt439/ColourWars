#include "game/pch.h"
#include "game/config/presentation/presentation.h"
#include "game/config/presentation/presentation_read.h"
#include "game/config/content_value.h"

#include "engine/assets/json.h"

#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>

using namespace labrador;
using namespace content_value;
using namespace presentation_read;

namespace
{
	bool is_hex_colour(const std::string& text)
	{
		if (text.length() != 6 && text.length() != 8)
		{
			return false;
		}
		return text.find_first_not_of("0123456789abcdefABCDEF") ==
			std::string::npos;
	}
}

namespace presentation_read
{
	Colour colour_from_text(const JsonValue& where, const std::string& text);

	// Accept named colours or RRGGBB/RRGGBBAA. Validate hex strings before
	// construction so a typo cannot silently become opaque black.
	Colour read_colour(const JsonValue& parent, const char* key)
	{
		return colour_from_text(parent, parent.string(key));
	}

	Colour colour_from_text(const JsonValue& parent, const std::string& text)
	{

		const std::optional<Colour> named = colour_from_name(text);
		if (named.has_value())
		{
			return *named;
		}

		if (is_hex_colour(text))
		{
			return Colour(text);
		}

		throw std::runtime_error(parent.where() + ": '" + text +
			"' is not a colour this engine knows by name, and is not six or "
			"eight hexadecimal digits.");
	}
}

const TeamPaletteEntry& TeamPalettePresentation::fixed_pair(TeamColourPair pair) const
{
	const char* key = TeamColourTools::palette_key(pair);
	for (const TeamPaletteEntry& entry : this->pairs)
	{
		if (entry.key == key) { return entry; }
	}
	throw std::invalid_argument("Missing fixed team palette");
}

const TeamPaletteEntry& TeamPalettePresentation::random_pair(size_t draw) const
{
	if (this->pairs.empty()) { throw std::invalid_argument("No team palette pairs to draw"); }
	return this->pairs[draw % this->pairs.size()];
}

const TeamPalettePresentation& Presentation::team_palette() const
{
	return this->team_palette_;
}

const WeaponChoicePresentation& WeaponSelectArtworkPresentation::choice(WeaponType type) const
{
	const size_t index = static_cast<size_t>(type);
	if (index >= this->choices.size())
	{
		throw std::out_of_range("Unsupported weapon selection type: " +
			std::to_string(static_cast<int>(type)));
	}
	return this->choices[index];
}

Presentation::Presentation(const char* path) :
	Presentation(read_json_file(path))
{
}

Presentation::Presentation(const JsonDocument& document)
{
	this->source_path_ = document.source_path();
	const JsonValue root = document.root();

	this->team_palette_ = read_team_palette(root);
	this->menu_ = read_menu_style(root);
	this->menu_text_ = read_menu_text(root);
	this->music_mix_ = read_music_mix(root);
	this->main_menu_ = read_main_menu(root, this->menu_.footer_y);
	this->controls_settings_ = read_controls_settings(root, this->menu_.footer_y);
	this->pause_menu_ = read_pause_menu(root);
	this->end_menu_ = read_end_menu(root);
	this->results_menu_ = read_results_menu(root);
	this->prompt_bar_ = read_prompt_bar(root);
	this->hud_ = read_hud(root);
	this->hud_.labels = this->menu_text_.hud;
	this->player_identity_ = read_player_identity(root);
	this->camera_ = read_camera(root);
	this->movement_feedback_ = read_movement_feedback(root);
	this->combat_feedback_ = read_combat_feedback(root);
	this->gate_permission_ = read_gate_permission(root);
	this->ferry_direction_ = read_ferry_direction(root);
	this->weapon_feedback_ = read_weapon_feedback(root);
	this->blocked_shot_ = read_blocked_shot(root);
	this->idle_drips_ = read_idle_drips(root);
	this->figure_ = read_figure(root);
	this->tool_motion_ = read_tool_motion(root);
	this->respawn_redraw_ = read_respawn_redraw(root);
	if (this->respawn_redraw_.canvas_size != this->figure_.canvas_size)
	{
		// The pen draws over the same overhanging canvas as the figure.
		throw std::runtime_error(root.object("respawn_redraw").where() +
			": canvas_size must equal figure.canvas_size.");
	}
	// A slip's stroke edits name respawn strokes by index, so they are checked
	// against that list; a person's two slips may not both change one stroke.
	const FigureSlipsPresentation& slips = this->figure_.slips;
	const std::string slips_where = root.object("figure").object("slips").where();
	const Vector2F last_nib = this->respawn_redraw_.canvas_size - Vector2F(this->respawn_redraw_.pen_width);
	for (const FigureSlip& slip : slips.list)
	{
		for (const FigureSlipStroke& edit : slip.strokes)
		{
			const bool on_canvas = std::all_of(edit.points.begin(), edit.points.end(),
				[&last_nib](const Vector2F& point)
				{
					return point.x >= 0.0f && point.y >= 0.0f && point.x <= last_nib.x && point.y <= last_nib.y;
				});
			if (edit.stroke >= this->respawn_redraw_.strokes.size() || !on_canvas)
			{
				throw std::runtime_error(slips_where + ": slip '" + slip.id +
					"' must edit an existing respawn_redraw stroke and keep the pen on the canvas.");
			}
		}
	}
	for (size_t first = 0; first < slips.list.size(); ++first)
	{
		for (size_t second = first; second < slips.list.size(); ++second)
		{
			if (first != second && slips.is_exclusive(first, second))
			{
				continue;
			}
			// A slip alone (first == second), or a pair a person may draw.
			std::vector<size_t> person{ first };
			if (second != first)
			{
				person.push_back(second);
			}
			std::vector<size_t> changed;
			for (const size_t index : person)
			{
				for (const FigureSlipStroke& edit : slips.list[index].strokes)
				{
					if (edit.edit == FigureSlipStroke::Edit::insert_after)
					{
						continue;
					}
					if (std::find(changed.begin(), changed.end(), edit.stroke) != changed.end())
					{
						throw std::runtime_error(slips_where + ": '" + slips.list[first].id + "' and '" +
							slips.list[second].id + "' both change respawn stroke " +
							std::to_string(edit.stroke) + "; make them exclusive.");
					}
					changed.push_back(edit.stroke);
				}
			}
		}
	}
	this->opening_ = read_opening(root);
	this->match_ = read_match(root);
	this->debug_overlay_ = read_debug_overlay(root);
}

const MatchPresentation& Presentation::match() const
{
	return this->match_;
}

const CameraPresentation& Presentation::camera() const
{
	return this->camera_;
}

const GatePermissionPresentation& Presentation::gate_permission() const
{
	return this->gate_permission_;
}

const FerryDirectionPresentation& Presentation::ferry_direction() const
{
	return this->ferry_direction_;
}

const MovementFeedbackPresentation& Presentation::movement_feedback() const
{
	return this->movement_feedback_;
}

const CombatFeedbackPresentation& Presentation::combat_feedback() const
{
	return this->combat_feedback_;
}

const WeaponFeedbackPresentation& Presentation::weapon_feedback() const
{
	return this->weapon_feedback_;
}

const BlockedShotPresentation& Presentation::blocked_shot() const
{
	return this->blocked_shot_;
}

const IdleDripPresentation& Presentation::idle_drip(WeaponType type) const
{
	return this->idle_drips_.at(static_cast<size_t>(type));
}

const RespawnRedrawPresentation& Presentation::respawn_redraw() const
{
	return this->respawn_redraw_;
}

const FigurePresentation& Presentation::figure() const
{
	return this->figure_;
}

const ToolMotionPresentation& Presentation::tool_motion() const
{
	return this->tool_motion_;
}

const OpeningPresentation& Presentation::opening() const
{
	return this->opening_;
}

const MenuStyle& Presentation::menu() const
{
	return this->menu_;
}

const MenuText& Presentation::menu_text() const
{
	return this->menu_text_;
}

const MusicMixPresentation& Presentation::music_mix() const
{
	return this->music_mix_;
}

const MainMenuPresentation& Presentation::main_menu() const
{
	return this->main_menu_;
}

const ControlsSettingsPresentation& Presentation::controls_settings() const
{
	return this->controls_settings_;
}

const PauseMenuPresentation& Presentation::pause_menu() const
{
	return this->pause_menu_;
}

const EndMenuPresentation& Presentation::end_menu() const
{
	return this->end_menu_;
}

const ResultsMenuPresentation& Presentation::results_menu() const
{
	return this->results_menu_;
}

const PromptBarPresentation& Presentation::prompt_bar() const
{
	return this->prompt_bar_;
}

const PlayerIdentityPresentation& Presentation::player_identity() const
{
	return this->player_identity_;
}

const HudPresentation& Presentation::hud() const
{
	return this->hud_;
}

const DebugOverlayPresentation& Presentation::debug_overlay() const
{
	return this->debug_overlay_;
}

void Presentation::set_movement_feedback_enabled(bool enabled)
{
	this->movement_feedback_.enabled = enabled;
}
