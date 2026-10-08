#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_menu_page.h"
#include "engine/core/state_context.h"
#include "engine/render/text_encoding.h"
#include "game/debug/debug_director_impl.h"

using namespace labrador;
using namespace mattmath;

// Developer-only layout in menu design units (1920x1080); presentation.json
// ships, this page does not. The panel starts below the band where every
// pane's HUD names its player (P1's box ends near y 160 at 1080p).
namespace debug_menu_consts
{
	constexpr float left = 150.0f;
	constexpr float value_x = 700.0f;
	constexpr float title_y = 188.0f;
	// The queued line sits beside the measured DEBUG heading, this far to its
	// right, so it can never touch it; the rows start this far under it.
	constexpr float queued_gap = 40.0f;
	constexpr float rows_gap = 14.0f;
	constexpr float max_row_pitch = 46.0f;
	constexpr float footer_y = 846.0f;
	constexpr float footer_pitch = 30.0f;
	constexpr std::size_t footer_columns = 90;

	// A backing panel, so the rows stay readable over a busy match.
	const mattmath::RectangleF panel(110.0f, 176.0f, 1060.0f, 774.0f);
	const labrador::Colour panel_colour(0.02f, 0.03f, 0.05f, 0.82f);
}

using namespace debug_menu_consts;

DebugMenuPage::DebugMenuPage(MenuContext* context, DebugDirectorImpl* director) :
	MenuPage(context), director_(director), model_(*director)
{
	this->director_->set_debug_menu(this);
}

DebugMenuPage::~DebugMenuPage()
{
	this->director_->set_debug_menu(nullptr);
}

void DebugMenuPage::init()
{
	this->input()->reset_actions();
	const MenuStyle& style = this->menu_style();
	const std::string& small_font = this->presentation().controls_settings().help_font;
	this->focus_.set_style({ style.highlight, style.unhighlight, style.disabled });
	this->set_prompts({ { L"Up/Down", L"Move" }, { L"Left/Right", L"Change" },
		{ L"A", L"Select" }, { L"B", L"Back" }, { L"F10/Back+Start", L"Close" } });

	this->title_ = std::make_unique<UiTextDropShadow>("debug_title", L"DEBUG",
		style.heading_font, Vector2F(left, title_y), this->render_resources(),
		style.heading_colour, style.shadow_colour, style.heading_shadow_offset);
	// Measured, so no heading font runs into the queued line or the rows.
	RenderResources* resources = this->render_resources();
	const Vector2F heading = resources->measure_text(
		resources->resolve_sprite_font(style.heading_font), L"DEBUG");
	this->small_font_ = resources->resolve_sprite_font(small_font);
	const float small_height = resources->measure_text(this->small_font_, L"Queued").y;
	const Vector2F queued_at(left + heading.x + queued_gap, title_y + heading.y - small_height);
	this->queued_room_ = panel.right() - 30.0f - queued_at.x;
	this->first_row_y_ = title_y + heading.y + rows_gap;
	this->row_pitch_ = std::min(max_row_pitch, (footer_y - 10.0f - this->first_row_y_) /
		static_cast<float>(DebugMenuModel::max_rows));
	this->queued_ = std::make_unique<UiTextDropShadow>("debug_queued", L"",
		small_font, queued_at, resources,
		style.warning, style.shadow_colour, style.detail_shadow_offset);
	for (std::size_t i = 0; i < this->footer_.size(); ++i)
	{
		this->footer_[i] = std::make_unique<UiTextDropShadow>(
			"debug_footer_" + std::to_string(i), L"", small_font,
			Vector2F(left, footer_y + footer_pitch * static_cast<float>(i)),
			this->render_resources(), style.unhighlight, style.shadow_colour,
			style.detail_shadow_offset);
	}
	this->panel_ = std::make_unique<UiTexture>("debug_panel", "sprite_sheet_1", "pixel",
		panel, this->render_resources(), panel_colour);
	this->texture_container_ = std::make_unique<UiContainer>("debug_textures");
	this->texture_container_->add_child(this->panel_.get());

	this->director_->log().info("debug menu: open");
	this->refresh();
}

void DebugMenuPage::request_close()
{
	this->close();
}

void DebugMenuPage::close()
{
	if (this->closing_)
	{
		return;
	}
	this->closing_ = true;
	this->play_wave(this->cancel_sound_);
	this->director_->log().info("debug menu: closed");
	// The page beneath arms its own presses afresh.
	this->input()->reset_actions();
	this->context()->pop();
}

std::size_t DebugMenuPage::focused_row() const
{
	const UiWidget* focused = this->focus_.focused(0);
	for (std::size_t i = 0; i < this->rows_.size(); ++i)
	{
		if (this->rows_[i].get() == focused)
		{
			return i;
		}
	}
	return 0;
}

void DebugMenuPage::rebuild(const std::vector<DebugMenuRow>& rows, std::size_t focus)
{
	// Rows in the detail font: twelve fit between the title and the log.
	MenuStyle style = this->menu_style();
	style.item_font = style.detail_font;
	style.item_shadow_offset = style.detail_shadow_offset;

	// The container holds loans to the old rows: replace it first.
	this->focus_.clear();
	this->text_container_ = std::make_unique<UiContainer>("debug_text");
	std::vector<std::unique_ptr<OptionRow>> built;
	this->shown_labels_.clear();
	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		const float y = this->first_row_y_ + this->row_pitch_ * static_cast<float>(i);
		built.push_back(std::make_unique<OptionRow>("debug_row_" + std::to_string(i),
			widen(rows[i].label), Vector2F(left, y), Vector2F(value_x, y), style,
			this->presentation().main_menu().options_value_colour, this->render_resources(),
			nullptr));
		built.back()->set_value(widen(rows[i].value));
		this->shown_labels_.push_back(rows[i].label);
	}
	this->rows_ = std::move(built);

	this->text_container_->add_child(this->title_.get());
	this->text_container_->add_child(this->queued_.get());
	for (const std::unique_ptr<OptionRow>& row : this->rows_)
	{
		this->text_container_->add_child(row.get());
		this->focus_.add(row.get());
	}
	for (const std::unique_ptr<UiTextDropShadow>& line : this->footer_)
	{
		this->text_container_->add_child(line.get());
	}
	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		this->focus_.set_enabled(this->rows_[i].get(), rows[i].enabled);
	}
	if (!this->rows_.empty())
	{
		this->focus_.set_focused(0, this->rows_[std::min(focus, this->rows_.size() - 1)].get());
	}
}

void DebugMenuPage::refresh()
{
	const std::vector<DebugMenuRow> rows = this->model_.rows();
	std::vector<std::string> labels;
	for (const DebugMenuRow& row : rows)
	{
		labels.push_back(row.label);
	}
	if (this->model_.level() != this->shown_level_)
	{
		this->shown_level_ = this->model_.level();
		this->rebuild(rows, this->model_.focus_hint());
	}
	else if (labels != this->shown_labels_)
	{
		// Same level, different rows (Apply appeared, a label grew): keep the
		// cursor on the same index.
		this->rebuild(rows, this->focused_row());
	}
	else
	{
		for (std::size_t i = 0; i < rows.size(); ++i)
		{
			this->rows_[i]->set_value(widen(rows[i].value));
			this->focus_.set_enabled(this->rows_[i].get(), rows[i].enabled);
		}
	}

	this->title_->set_text(widen(this->model_.title()));
	// Beside the heading there is one line's room: a long queue is cut short.
	std::wstring queued = widen(this->model_.queued());
	while (queued.size() > 4 &&
		this->render_resources()->measure_text(this->small_font_, queued).x > this->queued_room_)
	{
		queued = queued.substr(0, queued.size() - 4) + L"...";
	}
	this->queued_->set_text(queued);

	// The last lines the console would show, so a refused goto or an
	// unavailable restart says why without opening it.
	const std::deque<DebugLogLine>& lines = this->director_->log().lines();
	const MenuStyle& style = this->menu_style();
	for (std::size_t i = 0; i < this->footer_.size(); ++i)
	{
		const std::size_t back = this->footer_.size() - i;
		if (lines.size() < back)
		{
			this->footer_[i]->set_text(L"");
			continue;
		}
		const DebugLogLine& line = lines[lines.size() - back];
		std::string text = (line.severity == DebugSeverity::error ? "error: " :
			line.severity == DebugSeverity::warning ? "warning: " : "") + line.text;
		if (text.size() > footer_columns)
		{
			text = text.substr(0, footer_columns - 3) + "...";
		}
		this->footer_[i]->set_text(widen(text));
		this->footer_[i]->set_colour(line.severity == DebugSeverity::error ||
			line.severity == DebugSeverity::warning ? style.warning : style.unhighlight);
	}
}

void DebugMenuPage::update_page(float /*dt*/)
{
	if (this->closing_ || !this->context()->active())
	{
		return;
	}
	this->refresh();

	// Any device navigates, like the main menu; the first that did anything
	// this frame wins.
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		DebugMenuModel::Result result = DebugMenuModel::Result::stay;
		if (input.action == MenuInputAction::back)
		{
			this->play_wave(this->cancel_sound_);
			result = this->model_.back();
		}
		else if (input.action == MenuInputAction::proceed)
		{
			this->play_wave(this->confirm_sound_);
			result = this->model_.confirm(this->focused_row());
		}
		else if (input.direction == Direction::left || input.direction == Direction::right)
		{
			this->play_wave(this->direction_sound_);
			result = this->model_.step(this->focused_row(),
				input.direction == Direction::right ? 1 : -1);
		}
		else if (input.direction == Direction::up || input.direction == Direction::down)
		{
			if (this->focus_.move(0, input.direction))
			{
				this->play_wave(this->direction_sound_);
			}
		}
		else
		{
			continue;
		}

		if (result == DebugMenuModel::Result::close)
		{
			this->close();
			return;
		}
		this->refresh();
		return;
	}
}
#endif
