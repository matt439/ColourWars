#pragma once

#include "game/presentation_match.h"
#include "game/presentation_menu.h"
#include "game/presentation_menu_text.h"
#include "game/sound_reference.h"

#include <array>
#include <string>
#include <vector>

namespace labrador
{
	class AudioResources;
	class JsonDocument;
	class RenderResources;
}

// Fonts, colours and layout loaded from presentation.json for menus and HUDs.
// Keep visual choices separate from gameplay balance in tuning.json. Widget
// structure and behavior remain in C++. The records are in three headers:
// presentation_match.h (in-match), presentation_menu.h (menu layout) and
// presentation_menu_text.h (menu copy). Code that needs only in-match records
// includes presentation_match.h, so the menu records stay out of gameplay.
class Presentation
{
public:
	// Loads and validates the file, reporting invalid content with source context.
	explicit Presentation(const char* path);
	// From a document already read or parsed (labrador::parse_json); every
	// diagnostic names document.source_path().
	explicit Presentation(const labrador::JsonDocument& document);

	void validate_resources(const labrador::RenderResources& render,
		const labrador::AudioResources& audio) const;
	// Every menu, results and screen-music sound validate_resources checks,
	// with the group game/sound_groups.cpp puts it in.
	std::vector<SoundReference> sound_references() const;
	// Validate composed Stage text against the actual loaded catalog, without
	// assuming a maximum stage count or a particular widest numeric glyph.
	void validate_stage_counts(const labrador::RenderResources& render,
		size_t total, size_t eligible) const;

	const TeamPalettePresentation& team_palette() const;
	const MenuStyle& menu() const;
	const MenuText& menu_text() const;
	const MusicMixPresentation& music_mix() const;
	const MainMenuPresentation& main_menu() const;
	const ControlsSettingsPresentation& controls_settings() const;
	const PauseMenuPresentation& pause_menu() const;
	const EndMenuPresentation& end_menu() const;
	const ResultsMenuPresentation& results_menu() const;
	const PromptBarPresentation& prompt_bar() const;
	const HudPresentation& hud() const;
	const PlayerIdentityPresentation& player_identity() const;
	const MatchPresentation& match() const;
	const GatePermissionPresentation& gate_permission() const;
	const FerryDirectionPresentation& ferry_direction() const;
	const CameraPresentation& camera() const;
	const MovementFeedbackPresentation& movement_feedback() const;
	const CombatFeedbackPresentation& combat_feedback() const;
	const WeaponFeedbackPresentation& weapon_feedback() const;
	const BlockedShotPresentation& blocked_shot() const;
	const IdleDripPresentation& idle_drip(WeaponType type = WeaponType::sprayer) const;
	const FigurePresentation& figure() const;
	const ToolMotionPresentation& tool_motion() const;
	const RespawnRedrawPresentation& respawn_redraw() const;
	const OpeningPresentation& opening() const;
	const DebugOverlayPresentation& debug_overlay() const;

	// The developer console's live feedback toggle. The only mutator: only
	// the Debug director holds a non-const Presentation. MovementFeedback reads
	// the flag every frame, so the change applies immediately.
	void set_movement_feedback_enabled(bool enabled);

private:
	struct Validation;
	void validate_stage_scan_resources(const labrador::RenderResources& render) const;

	std::string source_path_;
	TeamPalettePresentation team_palette_;
	MenuStyle menu_;
	MenuText menu_text_;
	MusicMixPresentation music_mix_;
	MainMenuPresentation main_menu_;
	ControlsSettingsPresentation controls_settings_;
	PauseMenuPresentation pause_menu_;
	EndMenuPresentation end_menu_;
	ResultsMenuPresentation results_menu_;
	PromptBarPresentation prompt_bar_;
	HudPresentation hud_;
	PlayerIdentityPresentation player_identity_;
	MatchPresentation match_;
	GatePermissionPresentation gate_permission_;
	FerryDirectionPresentation ferry_direction_;
	CameraPresentation camera_;
	MovementFeedbackPresentation movement_feedback_;
	CombatFeedbackPresentation combat_feedback_;
	WeaponFeedbackPresentation weapon_feedback_;
	BlockedShotPresentation blocked_shot_;
	std::array<IdleDripPresentation, 5> idle_drips_;
	FigurePresentation figure_;
	ToolMotionPresentation tool_motion_;
	RespawnRedrawPresentation respawn_redraw_;
	OpeningPresentation opening_;
	DebugOverlayPresentation debug_overlay_;
};
