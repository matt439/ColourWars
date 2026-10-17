#include "game/pch.h"
#include "game/states/main_menu.h"
#include "game/drawable_text.h"
#include "game/save.h"
#include "game/objects/level_end_info.h"

using namespace DirectX;
using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

#pragma region MainMenuStageSelect

MainMenuStageSelect::MainMenuStageSelect(MenuContext* context) :
	MainMenuPage(context),
	stage_motion_(context->data()->presentation()->menu().motion)
{

}

void MainMenuStageSelect::update_page(float /*dt*/)
{
	const std::vector<ProcessedMenuInput> inputs =
		this->input()->player_inputs(*this->level_settings());
	if (this->wait_for_players(inputs))
	{
		return;
	}
	const int num_inputs = static_cast<int>(inputs.size());
	const int player_count = this->player_count();

	for (int i = 0; i < num_inputs && i < player_count; i++)
	{
		if (inputs[i].action == MenuInputAction::back)
		{
			if (this->select_state_.state == ConfirmationState::unconfirmed)
			{
				this->play_wave(this->cancel_sound_);
				this->context()->transition_to(
					std::make_unique<MainMenuWeaponSelect>(
						this->menu_context()));
				return;
			}
			if (this->select_state_.state == ConfirmationState::confirmed)
			{
				this->play_wave(this->unready_sound_, this->look().unready_sound.volume);
				this->select_state_.state =
					ConfirmationState::unconfirmed;
				this->stage_motion_.press();
			}
			// This selection is shared. Show its new state before another
			// action can leave the page, even if two players pressed together.
			break;
		}
		else if (inputs[i].action == MenuInputAction::proceed)
		{
			if (this->select_state_.state == ConfirmationState::confirmed)
			{
				this->play_wave(this->confirm_sound_);
				if (this->select_state_.slot == this->random_slot())
				{
					this->select_state_.slot = this->pick_random_stage();
				}
				this->set_level_settings();
				// The outgoing page is latched during its music fade. Settle an
				// unfinished lock response without refreshing Random's visuals.
				this->stage_motion_.reset();

				// Settings are already in GameMenu's context. Pop once and return so
				// simultaneous inputs cannot also pop the state underneath.
				this->menu_context()->finish_with_music([context = this->context()]
				{
					context->pop();
				});
				return;
			}
			else
			{
				this->play_wave(this->ready_sound_);
				this->select_state_.state = ConfirmationState::confirmed;
				this->stage_motion_.press();
			}
			// READY must be visible before a later press starts the match.
			break;
		}
		else if (this->select_state_.state == ConfirmationState::unconfirmed)
		{
			if (inputs[i].direction == labrador::Direction::left)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_stages(labrador::Direction::left);
			}
			else if (inputs[i].direction == labrador::Direction::right)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_stages(labrador::Direction::right);
			}
		}
	}
	this->update_stage_select_visuals();
}
void MainMenuStageSelect::set_level_settings() const
{
	this->level_settings()->set_stage(
		this->data()->stages()->at(
			static_cast<size_t>(this->select_state_.slot)).name);
}
void MainMenuStageSelect::update_stage_select_visuals()
{
	const int slot = this->select_state_.slot;
	const ConfirmationState state = this->select_state_.state;
	if (slot == this->displayed_slot_ && state == this->displayed_state_)
	{
		return;
	}
	const bool random = slot == this->random_slot();
	const bool confirmed = state == ConfirmationState::confirmed;
	const StageSelectPresentation& look = this->look().stage_select;
	this->stage_preview_->set_stage(random ? std::string() :
		this->data()->stages()->at(static_cast<size_t>(slot)).name);
	this->stage_name_->set_text(this->slot_display_name(slot));
	// User-authored stage names share the preview's width with the briefing.
	const RectangleF name_bounds = RectangleF::union_of(
		this->stage_name_->text_bounds_at(Vector2F::ZERO, 1.0f),
		this->stage_name_->text_bounds_at(this->menu_style().item_shadow_offset, 1.0f));
	const float name_scale = std::min(1.0f, look.preview_size.x / name_bounds.width);
	this->stage_name_->set_scale(name_scale);
	this->stage_name_->set_shadow_scale(name_scale);
	this->stage_name_->set_shadow_offset(this->menu_style().item_shadow_offset * name_scale);
	this->stage_name_->set_colour(confirmed ? look.selected_ink : look.unselected);
	this->random_mark_->set_hidden(!random);
	this->navigation_->set_text(random ? this->text().stage.random_navigation :
		L"<   ARENA " + std::to_wstring(slot + 1) + L" / " +
		std::to_wstring(this->random_slot()) + L"   >");
	// The record shares the navigation row, right-aligned to the preview. A
	// long played count shrinks it rather than reaching the arena counter.
	const std::wstring record = random ? std::wstring() : this->record_line(slot);
	this->stage_record_->set_text(record);
	this->stage_record_->set_hidden(record.empty());
	if (!record.empty())
	{
		const float preview_right = look.preview_position.x + look.preview_size.x;
		const float room = preview_right - this->navigation_->bounds().right() -
			this->menu_style().focus_padding.x;
		const RectangleF record_bounds = RectangleF::union_of(
			this->stage_record_->text_bounds_at(Vector2F::ZERO, 1.0f),
			this->stage_record_->text_bounds_at(this->menu_style().detail_shadow_offset, 1.0f));
		const float record_scale = std::min(1.0f, room / record_bounds.width);
		this->stage_record_->set_scale(record_scale);
		this->stage_record_->set_shadow_scale(record_scale);
		this->stage_record_->set_shadow_offset(this->menu_style().detail_shadow_offset * record_scale);
		this->stage_record_->set_position(Vector2F(preview_right -
			record_bounds.width * record_scale, look.record_y));
	}
	this->preview_edge_->set_colour(confirmed ? look.selected :
		this->menu_style().panel_edge_colour);
	this->selection_status_->set_text(confirmed ?
		this->text().stage.locked_status : this->text().stage.browse_status);
	this->selection_status_->set_colour(confirmed ? look.selected_ink : look.unselected);
	this->selection_hint_->set_text(confirmed ?
		this->text().stage.locked_hint : random ?
		L"Random: " + std::to_wstring(random_stage_indices(*this->data()->stages()).size()) +
		L" available arenas.\nConfirm to lock your choice." :
		this->text().stage.browse_hint);
	const RectangleF hint_bounds = RectangleF::union_of(
		this->selection_hint_->text_bounds_at(Vector2F::ZERO, 1.0f),
		this->selection_hint_->text_bounds_at(this->menu_style().detail_shadow_offset, 1.0f));
	const float hint_scale = std::min(1.0f,
		(look.briefing.size.x - 2.0f * look.briefing_padding.x) / hint_bounds.width);
	this->selection_hint_->set_scale(hint_scale);
	this->selection_hint_->set_shadow_scale(hint_scale);
	this->selection_hint_->set_shadow_offset(this->menu_style().detail_shadow_offset * hint_scale);
	this->set_player_prompts(confirmed ? MenuPrompt::arena_locked : MenuPrompt::arena_browse);
	this->refresh_briefing_paper();
	this->check_paper_geometry();
	this->displayed_slot_ = slot;
	this->displayed_state_ = state;
}
std::wstring MainMenuStageSelect::record_line(int slot) const
{
	const StageList* stages = this->data()->stages();
	if (slot < 0 || slot >= static_cast<int>(stages->size()))
	{
		return std::wstring();
	}
	const StageRecord* record = this->save()->records().find(
		stages->at(static_cast<size_t>(slot)).name);
	if (record == nullptr)
	{
		return std::wstring();
	}
	// Solo practice shows its coverage; a versus roster the winning share.
	const MenuRecordText& text = this->text().records;
	const bool solo = this->player_count() == 1;
	const int played = solo ? record->practice_played : record->versus_played;
	const std::optional<float>& best = solo ? record->best_coverage_percentage :
		record->best_win_percentage;
	if (played <= 0)
	{
		return std::wstring();
	}
	const std::wstring count = std::to_wstring(played) + L" " + text.played;
	if (!best)
	{
		return count;
	}
	return (solo ? text.best_coverage : text.best_win) + L" " +
		percentage_text(*best) + L" / " + count;
}
int MainMenuStageSelect::slot_of(const std::string& stage_name) const
{
	const StageList* stages = this->data()->stages();
	for (size_t i = 0; i < stages->size(); i++)
	{
		if (stages->at(i).name == stage_name)
		{
			return static_cast<int>(i);
		}
	}
	return 0;
}
int MainMenuStageSelect::random_slot() const
{
	return static_cast<int>(this->data()->stages()->size());
}
std::wstring MainMenuStageSelect::slot_display_name(int slot) const
{
	if (slot == this->random_slot())
	{
		return this->text().values.random;
	}
	const StageList* stages = this->data()->stages();
	if (slot >= 0 && slot < static_cast<int>(stages->size()))
	{
		// Startup validates stage names with this same text conversion.
		return drawable_text::widen_drawable(
			stages->at(static_cast<size_t>(slot)).display_name,
			"the stage list");
	}
	return L"ERROR";
}
int MainMenuStageSelect::pick_random_stage() const
{
	return static_cast<int>(random_stage_index(*this->data()->stages(),
		static_cast<size_t>(rand())));
}
void MainMenuStageSelect::init()
{
	this->select_state_.slot = this->slot_of(this->level_settings()->stage());
	const MenuStyle& style = this->menu_style();
	const MainMenuPresentation& main = this->look();
	const StageSelectPresentation& look = this->look().stage_select;
	RenderResources* resources = this->render_resources();
	this->paper_art_ = main.art;
	this->paper_art_.focus_padding = main.setup_focus_padding;
	this->paper_art_.shadow_offset = main.setup_shadow_offset;
	this->briefing_art_ = this->paper_art_;
	this->briefing_art_.paper = this->paper_art_.focus_fill;
	const Colour clear_shadow(0.0f, 0.0f, 0.0f, 0.0f);

	this->background_ = std::make_unique<UiTexture>("background",
		style.sprite_sheet, style.fill_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION), resources,
		main.home_background_colour);
	this->paper_ = std::make_unique<MenuPaperArtwork>(
		RectangleF((Vector2F(DESIGN_RESOLUTION.x, style.footer_y) - main.stage_box_size) / 2.0f,
			main.stage_box_size), resources, &style, &this->paper_art_, &this->focus_);
	this->heading_ = std::make_unique<UiTextDropShadow>("heading",
		this->text().heading(MenuHeading::arena), style.heading_font,
		this->calculate_widget_position(0, 0), resources,
		this->paper_art_.ink, clear_shadow, style.heading_shadow_offset);

	const RectangleF preview(look.preview_position, look.preview_size);
	RectangleF edge = preview;
	edge.inflate(style.rule_height);
	this->preview_edge_ = std::make_unique<UiTexture>("preview_edge",
		style.sprite_sheet, style.fill_frame, edge, resources, style.panel_edge_colour);
	this->preview_background_ = std::make_unique<UiTexture>("preview_background",
		style.sprite_sheet, style.fill_frame, preview, resources,
		this->presentation().hud().player_identity_background_colour);
	this->stage_preview_ = std::make_unique<StagePreview>("stage_preview",
		preview, this->data());
	this->stage_name_ = std::make_unique<UiTextDropShadow>("stage_name",
		L"", style.item_font, look.name_position, resources,
		look.unselected, clear_shadow, style.item_shadow_offset);
	this->navigation_ = std::make_unique<UiTextDropShadow>("navigation",
		L"", style.detail_font, look.navigation_position, resources,
		this->paper_art_.ink, clear_shadow, style.detail_shadow_offset);
	this->stage_record_ = std::make_unique<UiTextDropShadow>("stage_record",
		L"", look.record_font, Vector2F(0.0f, look.record_y), resources,
		this->paper_art_.ink, clear_shadow, style.detail_shadow_offset, true);
	this->random_mark_ = std::make_unique<UiTextDropShadow>("random_mark",
		L"?", this->look().announcement_font, Vector2F::ZERO, resources,
		style.highlight, style.shadow_colour, style.title_shadow_offset);
	this->random_mark_->set_position(preview.center() -
		this->random_mark_->bounds().center());

	const RectangleF briefing(look.briefing.position, look.briefing.size);
	this->briefing_layout_bounds_ = briefing;
	this->briefing_accent_motion_height_ = std::max(style.accent_width, look.briefing_padding.y);
	this->briefing_background_ = std::make_unique<MenuPaperArtwork>(briefing,
		resources, &style, &this->briefing_art_, &this->focus_);
	this->briefing_accent_ = std::make_unique<MenuStageAccent>("briefing_accent",
		RectangleF(briefing.position(), Vector2F(briefing.width, style.accent_width)),
		resources, &style, &this->stage_motion_, &this->save()->play_settings(),
		std::max(0.0f, look.briefing_padding.y - style.accent_width));
	this->briefing_accent_->set_colour(this->paper_art_.ink);
	this->stage_motion_.observe(this->briefing_accent_.get());
	this->register_selection_motion(&this->stage_motion_);
	const Vector2F info = briefing.position() + look.briefing_padding;
	const Vector2F row = look.briefing.spacing;

	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	for (UiObject* widget : std::initializer_list<UiObject*>{ this->background_.get(),
		this->paper_.get(),
		this->preview_edge_.get(), this->preview_background_.get(),
		this->stage_preview_.get(), this->briefing_background_.get(),
		this->briefing_accent_.get() })
	{
		this->texture_container_->add_child(widget);
	}
	this->text_container_ = std::make_unique<UiContainer>("text_container");
	for (UiObject* widget : std::initializer_list<UiObject*>{ this->heading_.get(),
		this->stage_name_.get(), this->navigation_.get(), this->stage_record_.get(),
		this->random_mark_.get() })
	{
		this->text_container_->add_child(widget);
	}

	const auto add_briefing = [this, resources, &style, &info, &row, &clear_shadow](
		const std::string& name, const std::wstring& text,
		const std::string& font, float line, const Colour& colour)
	{
		auto label = std::make_unique<UiTextDropShadow>(name, text, font,
			info + row * line, resources, colour, clear_shadow,
			style.detail_shadow_offset);
		this->text_container_->add_child(label.get());
		this->briefing_text_.push_back(std::move(label));
	};
	const bool solo = this->player_count() == 1;
	const int best_of = this->level_settings()->best_of();
	add_briefing("briefing_label", this->text().stage.briefing_label, style.brand_font, 0.0f,
		this->paper_art_.focus_ink);
	add_briefing("mode", solo ? this->text().stage.solo_mode : best_of == 1 ?
		this->text().stage.single_match_mode : L"BEST OF " + std::to_wstring(best_of),
		style.detail_font, 1.0f, this->paper_art_.ink);
	add_briefing("roster", std::to_wstring(this->player_count()) +
		(solo ? L" PLAYER" : L" PLAYERS / 2 TEAMS"),
		style.brand_font, 2.0f, this->paper_art_.ink);
	add_briefing("goal", this->text().stage.goal, style.brand_font, 3.0f, this->paper_art_.focus_ink);
	add_briefing("objective", solo ?
		this->text().stage.solo_objective : this->text().stage.versus_objective,
		style.brand_font, 4.0f, this->paper_art_.ink);

	this->selection_status_ = std::make_unique<UiTextDropShadow>("selection_status",
		L"", style.detail_font, info + row * 6.0f, resources,
		look.unselected, clear_shadow, style.detail_shadow_offset);
	this->selection_hint_ = std::make_unique<UiTextDropShadow>("selection_hint",
		L"", style.brand_font, info + row * 7.0f, resources,
		this->paper_art_.ink, clear_shadow, style.detail_shadow_offset);
	this->text_container_->add_child(this->selection_status_.get());
	this->text_container_->add_child(this->selection_hint_.get());
	this->update_stage_select_visuals();
}
void MainMenuStageSelect::refresh_briefing_paper()
{
	// The authored rectangle remains the text layout and live hint-fit budget.
	// Extend only its artwork to enclose complete unscaled briefing copy.
	RectangleF paper = this->briefing_layout_bounds_;
	const float inset = this->briefing_art_.keyline_width * 3.0f +
		this->briefing_art_.shadow_offset;
	const auto include = [&](const UiWidget& widget)
	{
		RectangleF bounds = widget.bounds();
		bounds.inflate(inset);
		paper = RectangleF::union_of(paper, bounds);
	};
	for (const auto& text : this->briefing_text_) { include(*text); }
	include(*this->selection_status_);
	include(*this->selection_hint_);
	this->briefing_background_->set_position(paper.position());
	this->briefing_background_->set_size(paper.size());
}
void MainMenuStageSelect::check_paper_geometry() const
{
	const auto inside_paper = [](const RectangleF& paper, const MenuPaperArtPresentation& art)
	{
		const float edge = art.keyline_width * 3.0f;
		return RectangleF(paper.position() + Vector2F(edge),
			paper.size() - Vector2F(edge * 2.0f + art.shadow_offset));
	};
	const RectangleF inside = inside_paper(this->paper_->rectangle(), this->paper_art_);
	const auto check = [](const RectangleF& bounds, const RectangleF& container,
		const char* role, const char* key)
	{
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			!std::isfinite(bounds.right()) || !std::isfinite(bounds.bottom()) ||
			bounds.width <= 0.0f || bounds.height <= 0.0f || !container.contains(bounds))
		{
			throw std::runtime_error(std::string(key) + ": Stage Select " + role +
				" must fit its paper including the retained text, shadow and artwork bounds.");
		}
	};
	const auto check_outer = [&](const RectangleF& bounds, const char* role)
	{
		check(bounds, inside, role, "main_menu.stage_box_size");
	};
	check_outer(this->heading_->bounds(), "heading");
	check_outer(this->stage_name_->bounds(), "arena name");
	check_outer(this->navigation_->bounds(), "navigation");
	if (!this->stage_record_->hidden())
	{
		const RectangleF record = this->stage_record_->bounds();
		check_outer(record, "stage record");
		const RectangleF navigation = this->navigation_->bounds();
		if (record.x <= navigation.right() || record.y < this->preview_edge_->bounds().bottom())
		{
			throw std::runtime_error("main_menu.stage_select.record_y: Stage Select record must sit below the preview edge, right of the arena counter.");
		}
	}
	check_outer(this->preview_edge_->bounds(), "preview edge");
	check_outer(this->preview_background_->bounds(), "preview background");
	check_outer(this->stage_preview_->bounds(), "arena preview");
	const RectangleF briefing = this->briefing_background_->rectangle();
	check_outer(briefing, "briefing card");
	const RectangleF preview = this->preview_edge_->bounds();
	if (briefing.x < preview.right() && briefing.right() > preview.x &&
		briefing.y < preview.bottom() && briefing.bottom() > preview.y)
	{
		throw std::runtime_error("main_menu.stage_select.briefing: Stage Select paper must remain separate from the preview edge.");
	}
	if (!this->random_mark_->hidden())
	{
		check(this->random_mark_->bounds(), this->preview_background_->rectangle(),
			"Random mark", "main_menu.stage_select.preview_size");
	}
	const RectangleF briefing_inside = inside_paper(briefing, this->briefing_art_);
	const auto check_briefing = [&](const UiWidget& widget, const char* role)
	{
		check_outer(widget.bounds(), role);
		check(widget.bounds(), briefing_inside, role, "main_menu.stage_select.briefing");
	};
	for (const auto& text : this->briefing_text_) { check_briefing(*text, "briefing copy"); }
	check_briefing(*this->selection_status_, "selection status");
	check_briefing(*this->selection_hint_, "selection hint");
	// The moving stripe keeps the original authored gutter even when the
	// surrounding paper grows to fit a longer objective or a live fitted hint.
	RectangleF accent = this->briefing_accent_->bounds();
	accent.height = this->briefing_accent_motion_height_;
	check_outer(accent, "briefing accent motion");
	if (!briefing.contains(accent))
	{
		throw std::runtime_error("main_menu.stage_select.briefing: Stage Select accent motion must stay inside its paper card.");
	}
}
void MainMenuStageSelect::cycle_stages(labrador::Direction direction)
{
	// Slots run 0..n-1 for the stages and n for Random, and the cursor wraps.
	const int last = random_stage_indices(*this->data()->stages()).empty() ?
		this->random_slot() - 1 : this->random_slot();
	int slot = this->select_state_.slot;
	if (direction == labrador::Direction::left)
	{
		slot = (slot == 0) ? last : slot - 1;
	}
	else if (direction == labrador::Direction::right)
	{
		slot = (slot == last) ? 0 : slot + 1;
	}
	if (slot != this->select_state_.slot)
	{
		this->select_state_.slot = slot;
		this->stage_motion_.selection_changed();
	}
}

#pragma endregion MainMenuStageSelect
