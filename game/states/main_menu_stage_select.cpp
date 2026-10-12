#include "game/pch.h"
#include "game/states/main_menu.h"
#include "game/drawable_text.h"

using namespace DirectX;
using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

#pragma region MainMenuStageSelect

MainMenuStageSelect::MainMenuStageSelect(MenuContext* context) :
	MainMenuPage(context)
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
				this->play_wave(this->cancel_sound_);
				this->select_state_.state =
					ConfirmationState::unconfirmed;
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
				this->stop_effect(this->music_, true);
				if (this->select_state_.slot == this->random_slot())
				{
					this->select_state_.slot = this->pick_random_stage();
				}
				this->set_level_settings();

				// Settings are already in GameMenu's context. Pop once and return so
				// simultaneous inputs cannot also pop the state underneath.
				this->context()->pop();
				return;
			}
			else
			{
				this->play_wave(this->ready_sound_);
				this->select_state_.state = ConfirmationState::confirmed;
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
			else if ((inputs[i].direction == labrador::Direction::up ||
				inputs[i].direction == labrador::Direction::down) &&
				this->select_state_.slot != this->random_slot() &&
				this->data()->stages()->at(static_cast<size_t>(
					this->select_state_.slot)).name == "movement_course")
			{
				const int change = inputs[i].direction == labrador::Direction::up ? 1 : 2;
				const int mode = static_cast<int>(this->level_settings()->workshop_visual_mode());
				this->level_settings()->set_workshop_visual_mode(
					static_cast<PlayerVisualMode>((mode + change) % 3));
				this->play_wave(this->direction_sound_);
				break;
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
	const PlayerVisualMode visual_mode = this->level_settings()->workshop_visual_mode();
	if (slot == this->displayed_slot_ && state == this->displayed_state_ &&
		visual_mode == this->displayed_visual_mode_)
	{
		return;
	}
	const bool random = slot == this->random_slot();
	const bool confirmed = state == ConfirmationState::confirmed;
	const bool course = !random && this->data()->stages()->at(
		static_cast<size_t>(slot)).name == "movement_course";
	const std::wstring art_label = visual_mode == PlayerVisualMode::original ? L"Original art" :
		visual_mode == PlayerVisualMode::continuous ? L"Workshop / rotating tool" : L"Workshop / posed tool";
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
	this->stage_name_->set_colour(confirmed ? look.selected : look.unselected);
	this->random_mark_->set_hidden(!random);
	this->navigation_->set_text(random ? L"<   SURPRISE ME   >" :
		L"<   ARENA " + std::to_wstring(slot + 1) + L" / " +
		std::to_wstring(this->random_slot()) + L"   >");
	this->preview_edge_->set_colour(confirmed ? look.selected :
		this->menu_style().panel_edge_colour);
	this->selection_status_->set_text(confirmed ? L"ARENA LOCKED" : L"FIND YOUR ARENA");
	this->selection_status_->set_colour(confirmed ? look.selected :
		this->menu_style().heading_colour);
	this->selection_hint_->set_text(confirmed && course ?
		art_label + L"\nConfirm again to start." : confirmed ?
		L"Confirm again to start.\nBack to change your choice." : random ?
		L"Random: " + std::to_wstring(random_stage_indices(*this->data()->stages()).size()) +
		L" available arenas.\nConfirm to lock your choice." : course ?
		L"Up/Down: " + art_label +
		L"\nConfirm to lock your choice." :
		L"Left/Right to browse.\nConfirm to lock your choice.");
	const RectangleF hint_bounds = RectangleF::union_of(
		this->selection_hint_->text_bounds_at(Vector2F::ZERO, 1.0f),
		this->selection_hint_->text_bounds_at(this->menu_style().detail_shadow_offset, 1.0f));
	const float hint_scale = std::min(1.0f,
		(look.briefing.size.x - 2.0f * look.briefing_padding.x) / hint_bounds.width);
	this->selection_hint_->set_scale(hint_scale);
	this->selection_hint_->set_shadow_scale(hint_scale);
	this->selection_hint_->set_shadow_offset(this->menu_style().detail_shadow_offset * hint_scale);
	this->set_player_prompts(confirmed ? std::vector<PromptBar::Prompt>{
		{ L"A", L"Start match" }, { L"B", L"Unlock arena" } } :
		course ? std::vector<PromptBar::Prompt>{ { L"Left/Right", L"Arena" },
			{ L"Up/Down", L"Art" }, { L"A", L"Lock" }, { L"B", L"Back" } } :
		std::vector<PromptBar::Prompt>{ { L"Left/Right", L"Browse arenas" },
			{ L"A", L"Lock arena" }, { L"B", L"Back" } });
	this->displayed_slot_ = slot;
	this->displayed_state_ = state;
	this->displayed_visual_mode_ = visual_mode;
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
		return L"Random";
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
	const StageSelectPresentation& look = this->look().stage_select;
	RenderResources* resources = this->render_resources();

	this->background_ = std::make_unique<UiTexture>("background",
		this->look().backdrop_sheet, this->look().backdrop_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION), resources,
		this->look().play_background_colour);
	this->heading_ = std::make_unique<UiTextDropShadow>("heading",
		L"Choose your arena", style.heading_font,
		this->calculate_widget_position(0, 0), resources,
		style.heading_colour, style.shadow_colour, style.heading_shadow_offset);

	const RectangleF preview(look.preview_position, look.preview_size);
	RectangleF edge = preview;
	edge.inflate(style.rule_height);
	this->preview_edge_ = std::make_unique<UiTexture>("preview_edge",
		"sprite_sheet_1", "pixel", edge, resources, style.panel_edge_colour);
	this->preview_background_ = std::make_unique<UiTexture>("preview_background",
		"sprite_sheet_1", "pixel", preview, resources,
		this->presentation().hud().player_identity_background_colour);
	this->stage_preview_ = std::make_unique<StagePreview>("stage_preview",
		preview, this->data());
	this->stage_name_ = std::make_unique<UiTextDropShadow>("stage_name",
		L"", style.item_font, look.name_position, resources,
		look.unselected, style.shadow_colour, style.item_shadow_offset);
	this->navigation_ = std::make_unique<UiTextDropShadow>("navigation",
		L"", style.detail_font, look.navigation_position, resources,
		style.highlight, style.shadow_colour, style.detail_shadow_offset);
	this->random_mark_ = std::make_unique<UiTextDropShadow>("random_mark",
		L"?", this->look().announcement_font, Vector2F::ZERO, resources,
		style.highlight, style.shadow_colour, style.title_shadow_offset);
	this->random_mark_->set_position(preview.center() -
		this->random_mark_->bounds().center());

	const RectangleF briefing(look.briefing.position, look.briefing.size);
	this->briefing_background_ = std::make_unique<UiTexture>("briefing_background",
		"sprite_sheet_1", "pixel", briefing, resources,
		this->presentation().hud().player_identity_background_colour);
	this->briefing_accent_ = std::make_unique<UiTexture>("briefing_accent",
		"sprite_sheet_1", "pixel", RectangleF(briefing.position(),
			Vector2F(briefing.width, style.accent_width)), resources, style.highlight);
	const Vector2F info = briefing.position() + look.briefing_padding;
	const Vector2F row = look.briefing.spacing;

	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	for (UiObject* widget : std::initializer_list<UiObject*>{ this->background_.get(),
		this->preview_edge_.get(), this->preview_background_.get(),
		this->stage_preview_.get(), this->briefing_background_.get(),
		this->briefing_accent_.get() })
	{
		this->texture_container_->add_child(widget);
	}
	this->text_container_ = std::make_unique<UiContainer>("text_container");
	for (UiObject* widget : std::initializer_list<UiObject*>{ this->heading_.get(),
		this->stage_name_.get(), this->navigation_.get(), this->random_mark_.get() })
	{
		this->text_container_->add_child(widget);
	}

	const auto add_briefing = [this, resources, &style, &info, &row](
		const std::string& name, const std::wstring& text,
		const std::string& font, float line, const Colour& colour)
	{
		auto label = std::make_unique<UiTextDropShadow>(name, text, font,
			info + row * line, resources, colour, style.shadow_colour,
			style.detail_shadow_offset);
		this->text_container_->add_child(label.get());
		this->briefing_text_.push_back(std::move(label));
	};
	const bool solo = this->player_count() == 1;
	const int best_of = this->level_settings()->best_of();
	add_briefing("briefing_label", L"MATCH SETUP", style.brand_font, 0.0f,
		style.highlight);
	add_briefing("mode", solo ? L"SOLO PRACTICE" : best_of == 1 ?
		L"SINGLE MATCH" : L"BEST OF " + std::to_wstring(best_of),
		style.detail_font, 1.0f, style.heading_colour);
	add_briefing("roster", std::to_wstring(this->player_count()) +
		(solo ? L" PLAYER" : L" PLAYERS / 2 TEAMS"),
		style.brand_font, 2.0f, style.unhighlight);
	add_briefing("goal", L"PAINT TO WIN", style.brand_font, 3.0f, style.highlight);
	add_briefing("objective", solo ?
		L"Make the arena your canvas.\nTry weapons. Cover more ground." :
		L"Cover the arena in your colour.\nMost painted surface wins.",
		style.brand_font, 4.0f, style.unhighlight);

	this->selection_status_ = std::make_unique<UiTextDropShadow>("selection_status",
		L"", style.detail_font, info + row * 6.0f, resources,
		style.heading_colour, style.shadow_colour, style.detail_shadow_offset);
	this->selection_hint_ = std::make_unique<UiTextDropShadow>("selection_hint",
		L"", style.brand_font, info + row * 7.0f, resources,
		style.unhighlight, style.shadow_colour, style.detail_shadow_offset);
	this->text_container_->add_child(this->selection_status_.get());
	this->text_container_->add_child(this->selection_hint_.get());
	this->update_stage_select_visuals();
	this->play_music(this->music_, this->look().music.volume);
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
	this->select_state_.slot = slot;
}

#pragma endregion MainMenuStageSelect
