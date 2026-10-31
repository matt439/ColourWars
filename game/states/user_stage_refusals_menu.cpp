#include "game/pch.h"
#include "game/states/user_stage_refusals_menu.h"
#include "game/stage_scan_diagnostics.h"
#include "game/objects/crude_drawing.h"
#include "engine/core/state_context.h"
#include <algorithm>
#include <utility>

using namespace labrador;
using namespace mattmath;

UserStageRefusalsMenu::UserStageRefusalsMenu(MenuContext* context, std::vector<std::string> refusals) :
	MenuPage(context), refusals_(std::move(refusals))
{
}

void UserStageRefusalsMenu::init()
{
	this->input()->reset_actions();
	const MainMenuPresentation& main = this->presentation().main_menu();
	const StageScanPresentation& look = main.stage_scan;
	const StageScanMenuText& copy = this->text().stage_scan;
	const MenuStyle& style = this->menu_style();
	this->font_ = this->render_resources()->resolve_sprite_font(look.font);
	const auto layout = stage_scan_diagnostics::layout(look, main.art, style.footer_y,
		*this->render_resources()->font(this->font_), copy);
	this->body_bounds_ = layout.body;
	this->progress_bounds_ = layout.progress;
	this->background_ = std::make_unique<UiTexture>("background", style.sprite_sheet, style.fill_frame,
		menu_geometry::design_area(), this->render_resources(),
		CrudeDrawing::premultiplied(main.home_background_colour));
	this->paper_ = std::make_unique<MenuPaperArtwork>(layout.paper, this->render_resources(),
		&style, &main.art, &this->focus_);
	this->texture_container_ = std::make_unique<UiContainer>("textures");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());
	this->text_container_ = std::make_unique<UiContainer>("text");
	const auto make_text = [&](const char* name, const std::wstring& value,
		const RectangleF& bounds, float scale)
	{
		auto widget = std::make_unique<UiText>(name, value, look.font, bounds.position(),
			this->render_resources(), main.art.ink);
		widget->set_scale(scale);
		this->text_container_->add_child(widget.get());
		return widget;
	};
	this->heading_ = make_text("heading", copy.heading, layout.heading, look.heading_scale);
	this->unicode_hint_ = make_text("unicode_hint", copy.unicode_hint, layout.unicode_hint, look.text_scale);
	this->body_ = make_text("diagnostic", L"", layout.body, look.text_scale);
	this->progress_ = make_text("progress", L"", layout.progress, look.text_scale);
	PromptBarPresentation prompt = this->presentation().prompt_bar();
	prompt.font = look.font;
	prompt.position = look.prompt_position + Vector2F(0.0f, style.footer_y);
	prompt.shadow_offset = look.prompt_shadow_offset;
	this->set_prompts(copy.details_prompt, prompt, look.text_scale);
	this->refresh_file();
}

void UserStageRefusalsMenu::refresh_file()
{
	const StageScanPresentation& look = this->presentation().main_menu().stage_scan;
	this->page_index_ = 0;
	if (this->refusals_.empty())
	{
		this->pages_ = { this->text().stage_scan.no_refusals };
	}
	else
	{
		this->pages_ = stage_scan_diagnostics::pages(*this->render_resources()->font(this->font_),
			this->refusals_[this->file_index_], this->body_bounds_.width / look.text_scale,
			this->body_bounds_.height / look.text_scale);
	}
	this->refresh_text();
}

void UserStageRefusalsMenu::refresh_text()
{
	this->body_->set_text(this->pages_.at(this->page_index_));
	this->progress_->set_hidden(this->refusals_.empty());
	if (this->refusals_.empty()) { return; }
	this->progress_->set_text(this->text().stage_scan.progress(this->file_index_ + 1,
		this->refusals_.size(), this->page_index_ + 1, this->pages_.size()));
	const float scale = this->presentation().main_menu().stage_scan.text_scale;
	this->progress_->set_scale(scale);
	const float width = this->progress_->bounds().width;
	if (width > this->progress_bounds_.width)
	{
		this->progress_->set_scale(scale * this->progress_bounds_.width / width);
	}
}

void UserStageRefusalsMenu::close()
{
	this->play_wave(this->cancel_sound_);
	this->input()->reset_actions();
	this->context()->pop();
}

bool UserStageRefusalsMenu::debug_dismiss(bool /*leaving_match*/)
{
	this->close();
	return true;
}

void UserStageRefusalsMenu::update_page(float /*dt*/)
{
	if (!this->context()->active()) { return; }
	const auto inputs = this->menu_inputs();
	for (const ProcessedMenuInput& input : inputs)
	{
		if (input.action == MenuInputAction::proceed || input.action == MenuInputAction::back ||
			input.action == MenuInputAction::pause)
		{
			this->close();
			return;
		}
	}
	for (const ProcessedMenuInput& input : inputs)
	{
		if (this->refusals_.empty()) { return; }
		if (input.direction == Direction::left || input.direction == Direction::right)
		{
			if (this->refusals_.size() > 1)
			{
				this->file_index_ = input.direction == Direction::left ?
					(this->file_index_ == 0 ? this->refusals_.size() - 1 : this->file_index_ - 1) :
					(this->file_index_ + 1 == this->refusals_.size() ? 0 : this->file_index_ + 1);
				this->refresh_file();
				this->play_wave(this->direction_sound_);
			}
			return;
		}
		const std::size_t before = this->page_index_;
		if (input.direction == Direction::up && this->page_index_ > 0) { --this->page_index_; }
		else if (input.direction == Direction::down && this->page_index_ + 1 < this->pages_.size())
		{ ++this->page_index_; }
		if (before != this->page_index_)
		{
			this->refresh_text();
			this->play_wave(this->direction_sound_);
			return;
		}
	}
}
