#include "game/pch.h"
#include "game/states/main_menu.h"
#include "game/objects/crude_drawing.h"
#include "game/carried/ui_text_drop_shadow.h"

using namespace DirectX;
using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

#pragma region MainMenuWeaponSelect

MainMenuWeaponSelect::MainMenuWeaponSelect(MenuContext* context,
	std::optional<Session> between_rounds) :
	MainMenuPage(context, std::move(between_rounds))
{

}

void MainMenuWeaponSelect::update_page(float /*dt*/)
{
	std::vector<ProcessedMenuInput> inputs =
		this->input()->player_inputs(*this->level_settings());
	if (this->wait_for_players(inputs))
	{
		return;
	}
	this->route_host_setup_input(inputs, this->host_row_);
	if (this->host_cursor_)
	{
		this->place_host_cursor(*this->host_cursor_, *this->player_widgets_[this->host_row_]->player);
	}
	const int player_count = this->player_count();
	const int num_inputs = static_cast<int>(inputs.size());
	// Each player may cancel independently, but leaving is a separate press
	// after the page has shown that every player is unconfirmed.
	const bool can_leave = this->all_players_unconfirmed();

	for (int i = 0; i < num_inputs && i < player_count; i++)
	{
		if (inputs[i].action == MenuInputAction::back)
		{
			if (can_leave)
			{
				this->play_wave(this->cancel_sound_);
				if (this->between_rounds())
				{
					this->return_to_round_menu();
				}
				else if (this->player_count() == 1)
				{
					this->context()->transition_to(
						std::make_unique<MainMenuModeSelect>(
							this->menu_context()));
				}
				else
				{
					this->context()->transition_to(
						std::make_unique<MainMenuTeamSelect>(
							this->menu_context()));
				}
				return;
			}
			if (this->select_states_[i].state ==
				ConfirmationState::confirmed)
			{
				this->play_wave(this->unready_sound_, this->look().unready_sound.volume);
				this->select_states_[i].state =
					ConfirmationState::unconfirmed;
				this->player_widgets_[i]->motion.press();
			}
			// Continue so other players can act in the same frame.
		}
		else if (inputs[i].action == MenuInputAction::proceed)
		{
			this->play_wave(this->confirm_sound_);
			if (this->select_states_[i].state != ConfirmationState::confirmed)
			{
				this->player_widgets_[i]->motion.press();
			}
			this->select_states_[i].state = ConfirmationState::confirmed;
		}
		else if (inputs[i].action == MenuInputAction::shuffle)
		{
			// Weapon Select has no shuffle; the same button changes the handicap.
			if (this->select_states_[i].state == ConfirmationState::unconfirmed)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_handicap(i);
			}
		}
		else if (this->select_states_[i].state ==
			ConfirmationState::unconfirmed)
		{
			if (inputs[i].direction == labrador::Direction::left)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_weapons(labrador::Direction::left, i);
			}
			else if (inputs[i].direction == labrador::Direction::right)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_weapons(labrador::Direction::right, i);
			}
		}
	}
	if (this->all_players_confirmed())
	{
		for (int i = 0; i < this->player_count(); i++)
		{
			if (this->select_states_[i].type == WeaponType::random_primary)
			{
				this->select_states_[i].type = random_weapon();
			}
		}
		this->set_level_settings();
		this->context()->transition_to(
			std::make_unique<MainMenuStageSelect>(this->menu_context(), this->between_rounds()));
		return;
	}
	this->update_weapon_select_visuals();
}
void MainMenuWeaponSelect::set_level_settings() const
{
	for (int i = 0; i < this->player_count(); i++)
	{
		this->level_settings()->
			set_player_weapon(i, this->select_states_[i].type);
		this->level_settings()->
			set_player_handicap(i, this->select_states_[i].handicap);
	}
}
WeaponType MainMenuWeaponSelect::random_weapon()
{
	int random = rand() % static_cast<int>(WeaponType::max_prim_wep);
	return static_cast<WeaponType>(random);
}
void MainMenuWeaponSelect::cycle_weapons(
	labrador::Direction direction, int player_index)
{
	int enum_max = static_cast<int>(WeaponType::max_prim_wep);
	const WeaponType previous = this->select_states_[player_index].type;
	int enum_pos =
		static_cast<int>(previous);
	if (direction == labrador::Direction::left)
	{
		if (enum_pos == 0)
		{
			enum_pos = enum_max;
		}
		else
		{
			enum_pos--;
		}
	}
	else if (direction == labrador::Direction::right)
	{
		if (enum_pos == enum_max)
		{
			enum_pos = 0;
		}
		else
		{
			enum_pos++;
		}
	}
	this->select_states_[player_index].type =
		static_cast<WeaponType>(enum_pos);
	if (this->select_states_[player_index].type != previous)
	{
		// The icon is reused for every tool, so its enum is the selection identity.
		this->player_widgets_[player_index]->motion.selection_changed();
	}
}
void MainMenuWeaponSelect::cycle_handicap(int player_index)
{
	SelectState& row = this->select_states_[static_cast<size_t>(player_index)];
	row.handicap = (row.handicap + 1) % handicaps::count;
	this->player_widgets_[static_cast<size_t>(player_index)]->handicap->set_text(
		this->text().handicap_label(row.handicap));
}
bool MainMenuWeaponSelect::all_players_confirmed() const
{
	for (int i = 0; i < this->player_count(); i++)
	{
		if (this->select_states_[i].state != ConfirmationState::confirmed)
		{
			return false;
		}
	}
	return true;
}
bool MainMenuWeaponSelect::all_players_unconfirmed() const
{
	for (int i = 0; i < this->player_count(); i++)
	{
		if (this->select_states_[i].state == ConfirmationState::confirmed)
		{
			return false;
		}
	}
	return true;
}
void MainMenuWeaponSelect::update_weapon_select_visuals()
{
	this->unconfirm_all_widgets();
	// The frames, name and description follow only the row's weapon, so they
	// are rewritten when it changes. A new row starts at none, which no row
	// can select, so its first write always happens. Colours follow readiness
	// and the live presentation, so they are still set every tick.
	this->displayed_weapons_.resize(static_cast<size_t>(this->player_count()),
		WeaponType::none);

	for (int i = 0; i < this->player_count(); i++)
	{
		const WeaponType type = this->select_states_[i].type;
		WeaponType& displayed = this->displayed_weapons_[static_cast<size_t>(i)];
		if (displayed != type)
		{
			const WeaponChoicePresentation& choice =
				this->weapon_select_art_.choice(type);
			this->player_widgets_[i]->weapon_icon->set_sprite_frame(
				choice.frame, choice.paint_frame, choice.supply_strip);
			this->player_widgets_[i]->weapon_name->set_text(choice.name);
			this->player_widgets_[i]->weapon_description->set_text(
				this->weapon_description(type));
			displayed = type;
		}

		if (this->select_states_[i].state == ConfirmationState::confirmed)
		{
			this->player_widgets_[i]->weapon_icon->
				set_colour(this->look().weapon_select.selected);
			this->player_widgets_[i]->weapon_name->
				set_colour(this->paper_art_.focus_ink);
		}
	}
	this->check_paper_geometry();
}
void MainMenuWeaponSelect::unconfirm_all_widgets()
{
	for (int i = 0; i < this->player_count(); i++)
	{
		this->player_widgets_[i]->weapon_icon->set_colour(
			this->look().weapon_select.unselected);
		this->player_widgets_[i]->weapon_name->set_colour(
			this->paper_art_.ink);
	}
}
void MainMenuWeaponSelect::init()
{
	this->weapon_select_art_ = this->look().weapon_select_art;
	this->weapon_descriptions_ = this->look().weapon_descriptions;
	const MainMenuPresentation& look = this->look();
	const MenuStyle& style = this->menu_style();
	this->paper_art_ = look.art;
	this->paper_art_.focus_padding = look.setup_focus_padding;
	this->paper_art_.shadow_offset = look.setup_shadow_offset;
	this->icon_art_ = this->paper_art_;
	this->icon_art_.keyline_width = style.rule_height;
	const Colour clear_shadow(0.0f, 0.0f, 0.0f, 0.0f);

	this->set_player_prompts(this->level_settings()->bot_count() > 0 ? MenuPrompt::bot_weapon : MenuPrompt::weapon);

	this->set_widget_spacing(this->look().weapon_select.spacing);
	// A random palette is not chosen until the level is built. Keep that
	// preview neutral instead of consuming a different random draw here.
	const TeamColourPair palette = this->save()->team_palette();
	const TeamColour preview_colours = palette == TeamColourPair::random
		? TeamColour(Colour::white, Colour::white)
		: TeamColourTools::team_colours(palette, this->presentation().team_palette());

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	
	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");

	this->background_ = std::make_unique<UiTexture>(
		"background",
		style.sprite_sheet,
		style.fill_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		CrudeDrawing::premultiplied(look.home_background_colour));
	this->paper_ = std::make_unique<MenuPaperArtwork>(
		RectangleF((Vector2F(DESIGN_RESOLUTION.x, style.footer_y) - look.weapon_box_size) / 2.0f,
			look.weapon_box_size),
		this->render_resources(), &style, &this->paper_art_, &this->focus_);

	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());

	this->heading_ = std::make_unique<colourwars::UiTextDropShadow>(
		"heading",
		this->text().heading(MenuHeading::weapons),
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->paper_art_.ink,
		clear_shadow,
		this->menu_style().heading_shadow_offset);

	this->text_container_->add_child(this->heading_.get());

	for (int i = 0; i < this->player_count(); i++)
	{
		SelectState state;
		state.state = ConfirmationState::unconfirmed;
		state.type = this->level_settings()
			->player_weapon(i);
		state.handicap = this->level_settings()->player_handicap(i);
		const WeaponChoicePresentation& choice =
			this->weapon_select_art_.choice(state.type);
		this->select_states_.push_back(state);

		auto widgets = std::make_unique<Widgets>(this->menu_style().motion);

		std::string name = "player_" + std::to_string(i + 1);
		std::wstring label_text = this->level_settings()->player_label(i, this->text());

		widgets->player = std::make_unique<colourwars::UiTextDropShadow>(
			name,
			label_text,
			this->menu_style().detail_font,
			this->calculate_widget_position(0, i + 2),
			this->render_resources(),
			this->paper_art_.ink,
			clear_shadow);
		widgets->handicap = std::make_unique<colourwars::UiTextDropShadow>(
			name + "_handicap",
			this->text().handicap_label(state.handicap),
			this->look().weapon_description_font,
			Vector2F(this->calculate_widget_position(0, i + 2).x, widgets->player->bounds().bottom()),
			this->render_resources(),
			this->paper_art_.ink,
			clear_shadow);
		widgets->badge = std::make_unique<UiPlayerBadge>(name + "_badge",
			UiPlayerBadge::before(widgets->player->bounds(), this->presentation().player_identity()),
			i, &this->presentation().player_identity(), this->render_resources());

		widgets->weapon_icon = std::make_unique<MenuIcon>(
			name + "_wep_icon",
			choice.frame,
			RectangleF(this->calculate_widget_position(2, i + 2), this->look().weapon_select.size),
			this->render_resources(),
			this->look().weapon_select.unselected, &this->menu_style(),
			this->weapon_select_art_.sheet, this->weapon_select_art_.fill_frame,
			&widgets->motion, &this->save()->play_settings(), &this->icon_art_);
		widgets->weapon_icon->set_sprite_frame(
			choice.frame, choice.paint_frame, choice.supply_strip);
		widgets->weapon_icon->set_paint_colour(preview_colours.team_colour(
			this->level_settings()->player_team(i)));
		widgets->motion.observe(widgets->weapon_icon.get());

		widgets->weapon_name = std::make_unique<colourwars::UiTextDropShadow>(
			name + "_wep_name",
			choice.name,
			this->look().weapon_description_font,
			widgets->weapon_icon->rectangle().position() +
				Vector2F(0.0f, this->look().weapon_select.size.y +
					this->menu_style().icon_padding.y + this->menu_style().rule_height),
			this->render_resources(),
			this->paper_art_.ink,
			clear_shadow,
			this->menu_style().detail_shadow_offset);

		widgets->weapon_description = std::make_unique<WrappedTextDropShadow>(
			name + "_wep_desc",
			this->weapon_description(state.type),
			this->look().weapon_description_font,
			widgets->weapon_icon->rectangle().position() +
				Vector2F(this->look().weapon_description_x_offset, 0.0f),
			this->render_resources(),
			this->look().weapon_description_max_width,
			this->look().weapon_description_colour,
			clear_shadow,
			this->look().weapon_description_shadow_offset);

		this->texture_container_->add_child(widgets->weapon_icon.get());
		this->texture_container_->add_child(widgets->badge.get());
		this->text_container_->add_child(widgets->weapon_name.get());
		this->text_container_->add_child(widgets->player.get());
		this->text_container_->add_child(widgets->handicap.get());
		this->text_container_->add_child(widgets->weapon_description.get());

		this->register_selection_motion(&widgets->motion);
		this->player_widgets_.push_back(std::move(widgets));
	}
	if (this->level_settings()->bot_count() > 0)
	{
		this->host_cursor_ = this->make_host_cursor();
		this->place_host_cursor(*this->host_cursor_, *this->player_widgets_[this->host_row_]->player);
		this->texture_container_->add_child(this->host_cursor_.get());
	}
	this->update_weapon_select_visuals();
}
const std::wstring& MainMenuWeaponSelect::weapon_description(WeaponType type) const
{
	const size_t index = static_cast<size_t>(type);
	return this->weapon_descriptions_.at(index);
}

void MainMenuWeaponSelect::check_paper_geometry() const
{
	const RectangleF panel = this->paper_->rectangle();
	const float edge = this->paper_art_.keyline_width * 3.0f;
	const RectangleF inside(panel.position() + Vector2F(edge),
		panel.size() - Vector2F(edge * 2.0f + this->paper_art_.shadow_offset));
	const auto check = [&](const RectangleF& bounds, const char* role)
	{
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			bounds.width <= 0.0f || bounds.height <= 0.0f || !inside.contains(bounds))
		{
			throw std::runtime_error(std::string("main_menu.weapon_box_size: Weapon Select ") + role +
				" must fit the paper including its retained text, badge and tile bounds.");
		}
	};
	check(this->heading_->bounds(), "heading");
	float previous_bottom = this->heading_->bounds().bottom();
	for (const auto& widgets : this->player_widgets_)
	{
		const RectangleF tile = widgets->weapon_icon->artwork_bounds();
		check(widgets->player->bounds(), "player label");
		check(widgets->handicap->bounds(), "handicap");
		check(widgets->badge->bounds(), "player badge");
		check(tile, "weapon tile");
		check(widgets->weapon_name->bounds(), "weapon name");
		check(widgets->weapon_description->bounds(), "weapon description");
		const float inset_limit = std::max(0.0f,
			std::min(this->menu_style().icon_padding.x, this->menu_style().icon_padding.y));
		if (std::min(tile.width, tile.height) - inset_limit * 2.0f <=
			this->icon_art_.keyline_width * 6.0f + this->icon_art_.shadow_offset)
		{
			throw std::runtime_error("main_menu.weapon_select.size: Weapon Select tiles must leave positive paper faces within their motion envelope.");
		}
		RectangleF row = RectangleF::union_of(widgets->player->bounds(), widgets->badge->bounds());
		row = RectangleF::union_of(row, widgets->handicap->bounds());
		row = RectangleF::union_of(row, tile);
		row = RectangleF::union_of(row, widgets->weapon_name->bounds());
		row = RectangleF::union_of(row, widgets->weapon_description->bounds());
		if (row.y < previous_bottom)
		{
			throw std::runtime_error("main_menu.weapon_select.spacing: Weapon Select rows must follow the heading without overlap.");
		}
		if (widgets->player->bounds().right() > tile.x || widgets->handicap->bounds().right() > tile.x ||
			widgets->weapon_name->bounds().right() > widgets->weapon_description->bounds().x)
		{
			throw std::runtime_error("main_menu.weapon_select.spacing: Weapon Select labels, tiles and descriptions must retain separate columns.");
		}
		previous_bottom = row.bottom();
	}
}

#pragma endregion MainMenuWeaponSelect
