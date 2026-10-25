#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_menu_model.h"
#include "game/debug/debug_tokenizer.h"
#include "game/debug/json_spans.h"
#include "game/debug/tune_document.h"
#include "game/debug/tune_scope.h"
#include "game/objects/crude_drawing.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/render/sprite_font_file.h"
#include "engine/ui/widget.h"
#include "engine/math/scalar.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <process.h>
#include <cstdlib>
#include <memory>

// Live tuning in the menus: tune completion and the debug menu's tuning tree.
namespace debug_console_acceptance
{
	namespace
	{
// CPU capture-chain evidence only: this test-derived public UiTexture is NOT
// MainMenuStageSelect and does not prove that a retained/reopened page adopted.
class StagePreviewCaptureProbe final : public labrador::UiTexture
{
public:
    using labrador::UiTexture::UiTexture;
    const labrador::Colour& captured_tint() const { return this->colour(); }
    const labrador::SpriteSheet* captured_sheet() const { return this->sprite_sheet(); }
    labrador::SpriteSheet::frame_handle captured_frame() const { return this->frame(); }
};

void check_stage_preview_background_colour_tunes(const std::filesystem::path& game_directory)
{
    const std::string path = "presentation.hud.player_identity.background_colour";
    const std::string local = "hud.player_identity.background_colour";
    {
        const std::string escaped = "{\r\n\"hud\":{\"player_identity\":{\"background_colour\":\"\\u003121827EE\",\"font\":\"retained\"}},\"outside\":1\r\n}\r\n";
        TuneDocument spans("presentation", escaped);
        const auto* leaf = spans.find(local);
        require(leaf != nullptr && spans.current().token(*leaf) == "\"\\u003121827EE\"" &&
            spans.find("hud.player_identity.font") == nullptr, "The exact backing leaf preserves its nested escaped token while font remains unavailable");
        spans.commit(spans.edited(*leaf, "\"2060C080\""), local);
        require(spans.edits().size() == 1 && spans.edits().at(local).baseline_token == "\"\\u003121827EE\"" &&
            spans.edits().at(local).current_token == "\"2060C080\"" && spans.reverted(local) == escaped,
            "Backing span edits retain both exact ledger tokens and reset to escaped CRLF baseline bytes");
        const std::string outside = "{\r\n\"hud\":{\"player_identity\":{\"background_colour\":\"\\u003121827EE\",\"font\":\"retained\"}},\"outside\":25\r\n}\r\n";
        const std::string wanted = "{\r\n\"hud\":{\"player_identity\":{\"background_colour\":\"2060C080\",\"font\":\"retained\"}},\"outside\":25\r\n}\r\n";
        const TuneSplice merged = spans.splice(outside, false);
        require(merged.conflicts.empty() && merged.written == 1 && merged.text == wanted,
            "A literal backing-only nested splice preserves unrelated external bytes and captured font text");
    }
    ScratchContent scratch(game_directory, L"stage-preview-backing-capture");
    GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
    Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
    TestDirector test(tuning, presentation); auto& impl = test.impl;
    impl.set_content_directories(scratch.source, scratch.staged);
    const auto* const document = impl.tune_document("presentation");
    const std::string baseline = read_text(scratch.source / L"presentation.json");
    const std::string tuning_source = read_text(scratch.source / L"tuning.json");
    const std::string tuning_staged = read_text(scratch.staged / L"tuning.json");
    const HudPresentation* const hud = &presentation.hud();
    const auto initial = colour_values(presentation); auto expected = initial;
    const auto held_materials = std::array<labrador::Colour, 8>{ hud->paper.ink, hud->paper.paper, hud->paper.shade,
        hud->closing_clock.ink, hud->closing_clock.paper, hud->closing_clock.shade, hud->closing_clock.accent,
        presentation.main_menu().stage_select.unselected };
    labrador::RenderResources render; labrador::AudioResources audio;
    for (const char* name : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
        { render.add_sprite_sheet(name, labrador::read_sprite_sheet((game_directory / "textures" / (std::string(name) + ".json")).string().c_str(), labrador::TextureHandle{})); }
    const std::array<const char*, 7> font_names{ "bowlby_one_144", "lato_black_24", "lato_black_36",
        "lato_black_48", "lato_black_72", "lato_black_144", "courier_prime_bold_16" };
    for (const char* name : font_names)
    {
        const auto file = labrador::read_sprite_font_file((game_directory / "fonts" / (std::string(name) + ".spritefont")).string());
        auto font = std::make_unique<labrador::Font>(labrador::TextureHandle{}, file.glyphs, file.line_spacing);
        if (font->contains(U'?')) { font->set_stand_in(U'?'); }
        render.add_font(name, std::move(font));
    }
    audio.add_sound_bank("paint_exe", labrador::SoundBank::silent());
    // TestDirector has no Application: commands exercise the actual full reader,
    // while resource validation below is a separate, direct public operation.
    presentation.validate_resources(render, audio);
    const std::string sheet_name = presentation.menu().sprite_sheet, frame_name = presentation.menu().fill_frame;
    const auto* const sheet = render.sprite_sheet(sheet_name);
    const auto frame = sheet->resolve_sprite_frame(frame_name);
    const auto frame_source = sheet->sprite_frame(frame).source_rectangle();
    const auto frame_origin = sheet->sprite_frame(frame).origin();
    const auto font_handles = [&]
    {
        std::vector<labrador::FontHandle> result;
        for (const char* name : font_names) { result.push_back(render.resolve_sprite_font(name)); }
        return result;
    };
    const auto held_fonts = font_handles();
    const mattmath::RectangleF rectangle(mattmath::Vector2F(150.0f, 330.0f), mattmath::Vector2F(1010.0f, 500.0f));
    require(document != nullptr && impl.tune_token(path) == "\"121827EE\"" &&
        hud->player_identity_background_colour == labrador::Colour(18, 24, 39, 238) &&
        presentation.main_menu().stage_select.preview_position == rectangle.position() &&
        presentation.main_menu().stage_select.preview_size == rectangle.size() &&
        tune_scope_of(path) == TuneScope::next_screen,
        "Stage preview backing retains authored121827EE, its existing rectangle and exact next-screen scope");
    const labrador::Colour default_tint((18.0f / 255.0f) * (238.0f / 255.0f), (24.0f / 255.0f) * (238.0f / 255.0f),
        (39.0f / 255.0f) * (238.0f / 255.0f), 238.0f / 255.0f);
    const auto capture_widget_chain = [&]
    {
        return std::make_unique<StagePreviewCaptureProbe>("preview_background", sheet_name, frame_name, rectangle,
            &render, CrudeDrawing::premultiplied(presentation.hud().player_identity_background_colour));
    };
    const auto retained = capture_widget_chain();
    require(retained->captured_tint() == default_tint && default_tint != hud->player_identity_background_colour &&
        default_tint != CrudeDrawing::premultiplied(default_tint),
        "Literal default premultiplied channels distinguish the previous raw tint and a double multiplication; old default pixel parity is not claimed");
    const auto metadata = [&](const StagePreviewCaptureProbe& widget)
    {
        require(widget.name() == "preview_background" && widget.rectangle() == rectangle && widget.bounds() == rectangle &&
            !widget.hidden() && widget.captured_sheet() == sheet && widget.captured_frame() == frame &&
            sheet->sprite_frame(frame).source_rectangle() == frame_source && sheet->sprite_frame(frame).origin() == frame_origin,
            "Capture-chain tint edits retain widget name, bounds, visibility, sheet/frame and source metadata");
    };
    const auto unchanged = [&]
    {
        require(&presentation.hud() == hud && colour_values(presentation) == expected && font_handles() == held_fonts &&
            presentation.menu().sprite_sheet == sheet_name && presentation.menu().fill_frame == frame_name &&
            presentation.main_menu().stage_select.preview_position == rectangle.position() &&
            presentation.main_menu().stage_select.preview_size == rectangle.size() &&
            std::array<labrador::Colour, 8>{ hud->paper.ink, hud->paper.paper, hud->paper.shade,
                hud->closing_clock.ink, hud->closing_clock.paper, hud->closing_clock.shade, hud->closing_clock.accent,
                presentation.main_menu().stage_select.unselected } == held_materials &&
            read_text(scratch.source / L"tuning.json") == tuning_source && read_text(scratch.staged / L"tuning.json") == tuning_staged &&
            !impl.in_match() && !impl.can_apply() && impl.request().empty(),
            "Backing commands preserve all root badge/normal HUD colours, other materials, resources, geometry, tuning and match ownership");
        retained->update(0.0f); retained->update(1.0f / 60.0f); retained->update(1.0f);
        require(retained->captured_tint() == default_tint, "Retained public UiTexture updates do not recapture the edited Presentation tint");
        metadata(*retained); presentation.validate_resources(render, audio);
    };
    const auto resource_path = scratch.root / L"missing-preview-resource.json";
    const JsonSpanDocument reference(baseline, "stage-preview-resource-reference");
    write_text(resource_path, reference.replaced(*reference.find("menu_style.fill_frame"), "\"__missing_preview_fill__\""));
    const Presentation missing_resource(resource_path.string().c_str());
    bool resource_refused = false;
    try { missing_resource.validate_resources(render, audio); }
    catch (const std::exception& error) { resource_refused = std::string(error.what()).find("menu_style.fill_frame") != std::string::npos; }
    require(resource_refused && document->current().text() == baseline && document->baseline().text() == baseline &&
        document->edits().empty() && read_text(scratch.source / L"presentation.json") == baseline &&
        read_text(scratch.staged / L"presentation.json") == baseline,
        "Separate direct resource validation rejects a reader-valid missing frame without touching the live command documents or content copies"); unchanged();
    std::srand(0x50525657); const int next_random = std::rand(); std::srand(0x50525657);
    struct CaptureCase { const char* word; labrador::Colour raw, tint; };
    const std::array<CaptureCase, 4> cases{ {
        { "RED", { 255, 0, 0 }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { "a1B2c3", { 161, 178, 195 }, { 161.0f / 255.0f, 178.0f / 255.0f, 195.0f / 255.0f, 1.0f } },
        { "2060C080", { 32, 96, 192, 128 }, { (32.0f / 255.0f) * (128.0f / 255.0f), (96.0f / 255.0f) * (128.0f / 255.0f),
            (192.0f / 255.0f) * (128.0f / 255.0f), 128.0f / 255.0f } },
        { "A532D600", { 165, 50, 214, 0 }, { 0.0f, 0.0f, 0.0f, 0.0f } } } };
    for (const auto& value : cases)
    {
        expected[stage_preview_background_colour_index] = value.raw; impl.log().clear();
        require(impl.execute("tune " + path + ' ' + value.word).ok && impl.execute("tune " + path).ok &&
            impl.execute("tune.diff presentation").ok && log_contains(impl.log(), path + " = \"" + value.word + '"', DebugSeverity::info) &&
            log_contains(impl.log(), path + "  \"121827EE\" -> \"" + value.word + "\"  [next screen]", DebugSeverity::info),
            "Named/RGB/partial/zero backing edits remain valid, inspectable and explicitly next-screen in the actual command diff");
        const auto fresh = capture_widget_chain(); metadata(*fresh);
        require(fresh->captured_tint() == value.tint, "A fresh public widget-chain factory captures current straight RGBA with one conversion");
        if (value.raw.a < 1.0f) { require(value.tint != value.raw, "Partial/zero nonblack input has a distinct raw-tint negative control"); }
        if (value.raw.a > 0.0f && value.raw.a < 1.0f)
            { require(value.tint != CrudeDrawing::premultiplied(value.tint), "Nonblack partial input distinguishes double multiplication"); }
        require(read_text(scratch.source / L"presentation.json") == baseline && read_text(scratch.staged / L"presentation.json") == baseline,
            "Unsaved backing commands leave both content copies untouched"); unchanged();
    }
    expected = initial;
    require(impl.execute("tune.reset " + path).ok && document->edits().empty() && document->current().text() == baseline,
        "Selected reset returns the exact original backing token without writing content"); unchanged();
    expected[stage_preview_background_colour_index] = labrador::Colour(32, 96, 192, 128);
    require(impl.execute("tune " + path + " 2060C080").ok, "A field-valid backing edit is pending for exact save/rebase");
    const std::string external = "\r\n" + baseline;
    const JsonSpanDocument save_reference(external, "stage-preview-backing-save-reference");
    const std::string saved = save_reference.replaced(*save_reference.find(local), "\"2060C080\"");
    write_text(scratch.source / L"presentation.json", external);
    require(impl.execute("tune.save presentation").ok && document->current().text() == saved && document->baseline().text() == saved &&
        document->edits().empty() && read_text(scratch.source / L"presentation.json") == saved && read_text(scratch.staged / L"presentation.json") == saved &&
        colour_values(Presentation((scratch.source / L"presentation.json").string().c_str())) == expected,
        "Backing Save changes exactly one literal token, preserves external bytes, reads back and rebases source/staging"); unchanged();
    expected[stage_preview_background_colour_index] = labrador::Colour(165, 50, 214, 0);
    require(impl.execute("tune " + path + " A532D600").ok, "The saved backing can be edited independently");
    expected[3] = labrador::Colour(120, 154, 188);
    require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok && document->edits().size() == 2,
        "Backing and unrelated overlay retain independent edit entries"); unchanged();
    expected[stage_preview_background_colour_index] = labrador::Colour(32, 96, 192, 128);
    require(impl.execute("tune.reset " + path).ok && document->edits().size() == 1 && !document->edits().contains(local) &&
        document->edits().contains("debug_overlay.colour"), "Selected backing reset preserves the overlay and its saved partial-alpha baseline"); unchanged();
    expected[stage_preview_background_colour_index] = labrador::Colour(165, 50, 214, 0);
    require(impl.execute("tune " + path + " A532D600").ok && document->edits().size() == 2, "Backing edit reapplies beside the overlay");
    const auto ledger = [&]
    {
        std::vector<std::string> result;
        for (const auto& [key, edit] : document->edits()) { result.push_back(key + '=' + edit.baseline_token + "->" + edit.current_token); }
        return result;
    };
    const auto held_ledger = ledger(); const std::string held_current = document->current().text();
    const JsonSpanDocument invalid_reference(saved, "stage-preview-backing-invalid-merge-reference");
    const std::string invalid = invalid_reference.replaced(*invalid_reference.find("hud.paper.keyline_width"), "4.1");
    write_text(scratch.source / L"presentation.json", invalid);
    for (const char* command : { "tune.save presentation", "tune.save presentation force" })
    {
        require(!impl.execute(command).ok && document->current().text() == held_current && document->baseline().text() == saved &&
            ledger() == held_ledger && document->edits().size() == 2 && read_text(scratch.source / L"presentation.json") == invalid &&
            read_text(scratch.staged / L"presentation.json") == saved && !std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
            !std::filesystem::exists(scratch.source / L"presentation.json.bak") && !std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
            "Ordinary/forced invalid merged HUD geometry saves preserve exact files, live values and both ledger token pairs"); unchanged();
    }
    expected = initial; expected[stage_preview_background_colour_index] = labrador::Colour(32, 96, 192, 128);
    require(impl.execute("tune.reset presentation").ok && document->edits().empty() && document->current().text() == saved &&
        document->baseline().text() == saved && read_text(scratch.source / L"presentation.json") == invalid &&
        read_text(scratch.staged / L"presentation.json") == saved, "Final reset restores the saved backing while preserving invalid external source"); unchanged();
    require(std::rand() == next_random, "Backing tuning/capture metadata checks consumed gameplay RNG");
}

	std::size_t paged_row_index(DebugMenuModel& model, const std::string& label)
	{
		const std::vector<std::string> first_page = labels(model.rows());
		do
		{
			const std::vector<DebugMenuRow> rows = model.rows();
			const auto found = std::find_if(rows.begin(), rows.end(), [&label](const DebugMenuRow& row)
				{ return row.label == label; });
			if (found != rows.end())
			{
				return static_cast<std::size_t>(found - rows.begin());
			}
			const auto more = std::find_if(rows.begin(), rows.end(), [](const DebugMenuRow& row)
				{ return row.kind == DebugMenuRow::Kind::more; });
			if (more == rows.end()) { break; }
			model.confirm(static_cast<std::size_t>(more - rows.begin()));
		} while (labels(model.rows()) != first_page);
		throw std::runtime_error("The debug menu has no row '" + label + "' on any page");
	}
	}

	void check_tune_completion_and_menu(const std::filesystem::path& game_directory)
	{
		check_stage_preview_background_colour_tunes(game_directory);
		ScratchContent scratch(game_directory, L"menu");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const DebugRegistry& registry = impl.registry();

		require(registry.complete("tune tuning.pl").line == "tune tuning.player.",
			"A path completes one segment, with no space after it");
		require(registry.complete("tune tuning.player.jump_b").line ==
			"tune tuning.player.jump_buffer_time ", "A leaf completes with a space");
		const std::vector<std::string> groups = registry.complete("tune presentation.").candidates;
		require(std::find(groups.begin(), groups.end(), "presentation.hud.") != groups.end() &&
			std::find(groups.begin(), groups.end(), "presentation.camera.") != groups.end(),
			"presentation. lists its groups");
		require(registry.complete("tune tuning.player.gravity ").candidates ==
			std::vector<std::string>{ "next", "prev" } &&
			registry.complete("tune presentation.movement_feedback.enabled ").candidates ==
			std::vector<std::string>{ "off", "on", "toggle" }, "The value completes by kind");
		require(registry.complete("tune presentation.movement_feedback.co").line ==
			"tune presentation.movement_feedback.colour " &&
			registry.complete("tune presentation.debug_overlay.font ").candidates.empty(),
			"Colour leaves complete as paths while arbitrary font strings stay outside the tree");
		const auto colour_choices = registry.complete("tune presentation.movement_feedback.colour ").candidates;
		for (const char* path : { colour_paths[7], colour_paths[8], colour_paths[9], colour_paths[10],
			colour_paths[11], colour_paths[identity_ink_colour_index], colour_paths[identity_background_colour_index] })
		{
			require(registry.complete(std::string("tune ") + path).line == std::string("tune ") + path + " " &&
				registry.complete(std::string("tune ") + path + " ").candidates == colour_choices,
				std::string("The selection/feedback/identity colour completes as an exact leaf with literal colour choices: ") + path);
		}
		// The colour leaf shares its prefix with the existing numeric width.
		// Keep both path choices until a space explicitly selects the colour.
		const std::string feedback_keyline_prefix = colour_paths[12];
		const DebugCompletion feedback_keyline_matches = registry.complete("tune " + feedback_keyline_prefix);
		require(feedback_keyline_matches.line == "tune " + feedback_keyline_prefix &&
			feedback_keyline_matches.candidates == std::vector<std::string>{
				feedback_keyline_prefix, feedback_keyline_prefix + "_width" } &&
			registry.complete("tune " + feedback_keyline_prefix + " ").candidates == colour_choices,
			"Feedback keyline completion preserves the numeric sibling and explicit colour choices");
		require(colour_choices.size() >= 2 &&
			std::find(colour_choices.begin(), colour_choices.end(), "next") == colour_choices.end() &&
			std::find(colour_choices.begin(), colour_choices.end(), "prev") == colour_choices.end() &&
			std::find(colour_choices.begin(), colour_choices.end(), "toggle") == colour_choices.end(),
			"Colour value completion offers literal examples instead of numeric or boolean operations");
		for (const std::string& choice : colour_choices)
		{
			require(impl.execute("tune presentation.movement_feedback.colour " + choice).ok,
				"A completed colour example is accepted by the actual command: " + choice);
		}
		impl.execute("tune.reset");

		DebugMenuModel model(impl);
		const std::vector<std::string> roots = labels(model.rows());
		require(std::find(roots.begin(), roots.end(), "tuning") != roots.end() &&
			std::find(roots.begin(), roots.end(), "replay") != roots.end(),
			"The root lists the tuning and replay categories");
		model.confirm(row_index(model.rows(), "tuning"));
		require(labels(model.rows()) == std::vector<std::string>{ "tuning", "presentation", "tune.diff",
			"tune.reset", "tune.save" }, "Tuning opens on the two files and the save rows");
		model.confirm(row_index(model.rows(), "tuning"));
		require(model.title() == "DEBUG / tuning", "A file node's title");
		model.confirm(row_index(model.rows(), "player"));
		require(model.title() == "DEBUG / tuning / player", "The node's title is its path");
		std::vector<DebugMenuRow> rows = model.rows();
		require(rows.size() == DebugMenuModel::max_rows && rows.back().kind == DebugMenuRow::Kind::more &&
			rows.back().label.rfind("more (1/", 0) == 0, "A long node is paged with a more row");
		const std::size_t gravity = row_index(rows, "gravity");
		const std::string gravity_token = impl.tune_token("tuning.player.gravity");
		const float gravity_value = tuning.player().gravity;
		require(rows[gravity].kind == DebugMenuRow::Kind::variable && rows[gravity].value == gravity_token,
			"A leaf shows its token");
		require(rows[row_index(rows, "size")].kind == DebugMenuRow::Kind::category,
			"An inner node is a category row");
		model.step(gravity, 1);
		require(tuning.player().gravity == static_cast<float>(gravity_value +
			tune_step(gravity_value, tune_integer_token(gravity_token))) &&
			log_contains(impl.log(), "[menu] tune tuning.player.gravity next", DebugSeverity::echo),
			"Right on a leaf runs tune <path> next, echoed");
		model.step(gravity, -1);
		require(tuning.player().gravity == gravity_value, "Left runs prev");
		const std::string first_page = labels(model.rows()).front();
		model.confirm(model.rows().size() - 1);
		require(labels(model.rows()).front() != first_page &&
			model.rows().back().label.rfind("more (2/", 0) == 0, "more advances the page");
		model.back();
		require(model.title() == "DEBUG / tuning" &&
			model.rows()[model.focus_hint()].label == "player", "Back pops one segment onto the node it left");
		model.confirm(row_index(model.rows(), "player"));
		model.confirm(row_index(model.rows(), "size"));
		require(model.title() == "DEBUG / player / size" &&
			model.rows()[row_index(model.rows(), "y")].value.find("*rebuild") != std::string::npos,
			"A value that is not live is marked with its scope");
		model.back();
		model.back();
		model.back();
		require(model.title() == "DEBUG / tuning", "Back reaches the category");
		model.back();
		require(model.title() == "DEBUG", "and then the root");

		// The presentation boolean steps by toggle.
		model.confirm(row_index(model.rows(), "tuning"));
		model.confirm(row_index(model.rows(), "presentation"));
		// Presentation gains authored sections over time; follow its actual
		// pagination rather than assuming a particular section stays on page one.
		model.confirm(paged_row_index(model, "movement_feedback"));
		const bool enabled = presentation.movement_feedback().enabled;
		model.step(row_index(model.rows(), "enabled"), -1);
		require(presentation.movement_feedback().enabled != enabled,
			"Left or Right on a boolean toggles it");
		impl.execute("tune.reset");
		model.back();
		require(model.rows()[model.focus_hint()].label == "movement_feedback",
			"Back restores the presentation page and row it left");
		model.confirm(paged_row_index(model, "weapon_feedback"));
		const bool weapon_enabled = presentation.weapon_feedback().enabled;
		model.step(row_index(model.rows(), "enabled"), 1);
		require(presentation.weapon_feedback().enabled != weapon_enabled,
			"The weapon feedback group remains reachable and its boolean is tunable");
		impl.execute("tune.reset");
		model.back();
		model.confirm(paged_row_index(model, "movement_feedback"));
		const std::size_t colour_row = paged_row_index(model, "colour");
		const std::string colour_token = impl.tune_token("presentation.movement_feedback.colour");
		rows = model.rows();
		require(rows[colour_row].kind == DebugMenuRow::Kind::inspect && rows[colour_row].enabled &&
			rows[colour_row].value.find(colour_token) != std::string::npos &&
			rows[colour_row].value.find("(console)") != std::string::npos,
			"An audited colour has an enabled inspect row with its quoted token and console hint");
		const std::string colour_text = impl.tune_document("presentation")->current().text();
		const auto before_colours = colour_values(presentation);
		const std::string colour_title = model.title();
		impl.log().clear();
		model.step(colour_row, 1); model.step(colour_row, -1);
		require(impl.log().lines().empty() && colour_values(presentation) == before_colours &&
			impl.tune_document("presentation")->current().text() == colour_text,
			"Left/Right on a colour inspect row emits no step and changes no values or pending edits");
		require(model.confirm(colour_row) == DebugMenuModel::Result::stay && model.title() == colour_title &&
			log_contains(impl.log(), "[menu] tune presentation.movement_feedback.colour", DebugSeverity::echo) &&
			log_contains(impl.log(), "presentation.movement_feedback.colour = " + colour_token, DebugSeverity::info) &&
			colour_values(presentation) == before_colours,
			"Confirm inspects the colour through a read-only tune getter and stays on the same menu node");
		model.back();
		model.confirm(paged_row_index(model, "player_identity"));
		const std::size_t background_row = paged_row_index(model, "background_colour");
		const std::string background_path = colour_paths[identity_background_colour_index];
		const std::string background_token = impl.tune_token(background_path);
		rows = model.rows();
		require(rows[background_row].kind == DebugMenuRow::Kind::inspect && rows[background_row].enabled &&
			rows[background_row].value.find(background_token) != std::string::npos &&
			rows[background_row].value.find("(console)") != std::string::npos,
			"Identity background has an enabled inspect row with its exact quoted token and console hint");
		const auto identity_before = colour_values(presentation);
		const std::string identity_text = impl.tune_document("presentation")->current().text();
		const std::string identity_title = model.title();
		impl.log().clear();
		model.step(background_row, 1); model.step(background_row, -1);
		require(impl.log().lines().empty() && colour_values(presentation) == identity_before &&
			impl.tune_document("presentation")->current().text() == identity_text,
			"Left/Right on identity background cannot invent numeric stepping or alter either badge colour");
		require(model.confirm(background_row) == DebugMenuModel::Result::stay && model.title() == identity_title &&
			log_contains(impl.log(), "[menu] tune " + background_path, DebugSeverity::echo) &&
			log_contains(impl.log(), background_path + " = " + background_token, DebugSeverity::info) &&
			colour_values(presentation) == identity_before &&
			impl.tune_document("presentation")->current().text() == identity_text,
			"Confirm inspects the exact background leaf without changing its sibling, pending edits or menu node");
        const std::string preview_path = colour_paths[stage_preview_background_colour_index];
        require(registry.complete("tune " + preview_path).line == "tune " + preview_path + " " &&
            registry.complete("tune " + preview_path + " ").candidates == colour_choices,
            "The exact captured preview colour completes as one leaf with literal colour choices");
        DebugMenuModel preview_model(impl);
        preview_model.confirm(row_index(preview_model.rows(), "tuning"));
        preview_model.confirm(row_index(preview_model.rows(), "presentation"));
        preview_model.confirm(paged_row_index(preview_model, "hud"));
        preview_model.confirm(paged_row_index(preview_model, "player_identity"));
        const std::size_t preview_row = paged_row_index(preview_model, "background_colour");
        const std::string preview_token = impl.tune_token(preview_path);
        const auto preview_before = colour_values(presentation);
        const std::string preview_text = impl.tune_document("presentation")->current().text();
        const std::string preview_title = preview_model.title();
        rows = preview_model.rows();
        require(rows[preview_row].kind == DebugMenuRow::Kind::inspect && rows[preview_row].enabled &&
            rows[preview_row].value.find(preview_token) != std::string::npos &&
            rows[preview_row].value.find("*next screen") != std::string::npos &&
            rows[preview_row].value.find("(console)") != std::string::npos,
            "The captured preview backing has an enabled read-only row marked next screen");
        impl.log().clear();
        preview_model.step(preview_row, 1); preview_model.step(preview_row, -1);
        require(impl.log().lines().empty() && colour_values(presentation) == preview_before &&
            impl.tune_document("presentation")->current().text() == preview_text,
            "Preview inspect rows never invent numeric colour stepping");
        require(preview_model.confirm(preview_row) == DebugMenuModel::Result::stay &&
            preview_model.title() == preview_title &&
            log_contains(impl.log(), "[menu] tune " + preview_path, DebugSeverity::echo) &&
            log_contains(impl.log(), preview_path + " = " + preview_token, DebugSeverity::info) &&
            colour_values(presentation) == preview_before &&
            impl.tune_document("presentation")->current().text() == preview_text,
            "Confirm only inspects the next-screen preview token and preserves the menu, values and document");
	}
}
#endif
