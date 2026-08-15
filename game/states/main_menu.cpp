#include "game/pch.h"
#include "game/states/main_menu.h"
#include "game/drawable_text.h"

using namespace DirectX;
using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

MainMenuPage::MainMenuPage(MenuContext* context) :
	MenuPage(context)
{
	// The bank and the three shared waves are MenuPage's now. What is left
	// here is what only the main menu makes: a refusal, a ready chime and the
	// track that plays across every page of it.
	const MainMenuPresentation& look = this->look();

	this->error_sound_ = this->resolve_wave(look.error_sound);
	this->ready_sound_ = this->resolve_wave(look.ready_sound);
	this->music_ = this->resolve_effect(look.music.name);
}

const MainMenuPresentation& MainMenuPage::look() const
{
	return this->presentation().main_menu();
}

int MainMenuPage::player_count() const
{ 
	return this->level_settings()->
		player_count();
}

std::vector<int> MainMenuPage::connected_slots() const
{
	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	std::vector<int> result;
	for (int slot = 0; slot < static_cast<int>(inputs.size()); slot++)
	{
		if (inputs[slot].connected)
		{
			result.push_back(slot);
		}
	}
	return result;
}

std::vector<ProcessedMenuInput> MainMenuPage::connected_inputs() const
{
	std::vector<ProcessedMenuInput> result;
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		if (input.connected)
		{
			result.push_back(input);
		}
	}
	return result;
}

#pragma region MainMenuTitle

MainMenuTitle::MainMenuTitle(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuTitle::update(float /*dt*/)
{
	// A pad can arrive at any moment, and asking for one is the whole purpose
	// of the line, so the answer is refreshed rather than remembered - but
	// only while this window has the foreground.
	//
	// The shell suspends the pad reader whenever it does not, and a suspended
	// reader answers "disconnected" for every slot. Refreshing from that would
	// put "Connect a controller" under Start every time the player clicked on
	// another window, which is the one thing this line must never say wrongly:
	// its whole job is to distinguish a game waiting for a pad from a game
	// that has hung. Unasked, the line keeps whatever it last showed, which is
	// the right answer in both directions.
	if (this->data()->window_has_foreground())
	{
		this->no_pad_->set_hidden(!this->connected_slots().empty());
	}

	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	for (int i = 0; i < inputs.size(); i++)
	{
		if (inputs[i].action == MenuInputAction::proceed)
		{
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(std::make_unique<MainMenuHome>(
				this->menu_context()));
			return;
		}
	}
}
void MainMenuTitle::init()
{
	this->set_prompts({
		{ L"A", L"Start" } });

	this->background_ = std::make_unique<UiTexture>(
		"background",
		"sprite_sheet_1",
		"square_white_4",
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().title_background_colour);

	this->title_ = std::make_unique<UiTextDropShadow>(
		"title",
		L"Colour Wars",
		this->look().title_font,
		Vector2F(200.0f, 300.0f),
		this->render_resources(),
		this->look().title_text_colour,
		this->menu_style().shadow_colour,
		this->menu_style().title_shadow_offset);

	this->start_ = std::make_unique<UiTextDropShadow>(
		"start",
		L"Start",
		this->menu_style().item_font,
		Vector2F(250.0f, 700.0f),
		this->render_resources(),
		this->look().title_start_text_colour,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	// Every input this game has is a gamepad button. With nothing plugged in
	// the title screen answers no key and there is no other screen to get to,
	// so a correct build with no pad on the desk looks exactly like a hung one
	// - which is the first thing a reader of this repository sees.
	//
	// Hidden here and revealed by update() rather than the other way round, so
	// the ordinary case never flashes it.
	this->no_pad_ = std::make_unique<UiTextDropShadow>(
		"no_pad",
		L"Connect a controller",
		this->menu_style().item_font,
		Vector2F(250.0f, 800.0f),
		this->render_resources(),
		this->look().title_no_pad_text_colour,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset,
		true);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->title_.get());
	this->text_container_->add_child(this->start_.get());
	this->text_container_->add_child(this->no_pad_.get());

	this->play_effect(this->music_, true, this->look().music.volume);
}

#pragma endregion MainMenuTitle

#pragma region MainMenuHome

MainMenuHome::MainMenuHome(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuHome::update(float /*dt*/)
{
	// The focus is read inside the loop rather than snapshotted above it. The
	// old form took the highlighted widget's *name* once and compared all four
	// pad slots against that string, so pad 0 moving the cursor and pad 1
	// pressing A in the same frame activated whatever pad 0 had just left -
	// and two pads pressing down together advanced one step while playing the
	// click twice. Two pads on a couch-multiplayer menu is the design centre.
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		if (input.action == MenuInputAction::back)
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuTitle>(this->menu_context()));
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			// One activation per frame. Every action on this page leaves the
			// page, so a second pad's press against a page that is already
			// going somewhere is not a second decision to honour.
			if (this->focus_.activate(0) == Activation::ran)
			{
				return;
			}
		}
		else if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
		}
	}
}
void MainMenuHome::init()
{
	this->set_prompts({
		{ L"A", L"Select" },
		{ L"B", L"Back" } });

	this->focus_.set_style({ this->menu_style().highlight, this->menu_style().unhighlight });

	this->background_ = std::make_unique<UiTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().home_background_colour);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Main Menu",
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset);

	this->play_ = std::make_unique<UiTextDropShadow>(
		"play",
		L"Play",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		this->menu_style().highlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->options_ = std::make_unique<UiTextDropShadow>(
		"options",
		L"Options",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->exit_ = std::make_unique<UiTextDropShadow>(
		"exit",
		L"Exit",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->play_.get());
	this->text_container_->add_child(this->options_.get());
	this->text_container_->add_child(this->exit_.get());

	// Navigation walks bounds(), in the design space the widgets were built
	// in. Declaration order does not decide adjacency - position does - so the
	// four-way if-chains this replaces are not written anywhere any more.
	this->focus_.add(this->play_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuModeSelect>(this->menu_context()));
		});
	this->focus_.add(this->options_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuOptions>(this->menu_context()));
		});
	this->focus_.add(this->exit_.get(), [this]
		{
			this->data()->application()->quit();
		});

	this->play_effect(this->music_, true, this->look().music.volume);
}

#pragma endregion MainMenuHome

#pragma region MainMenuOptions

MainMenuOptions::MainMenuOptions(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuOptions::update(float /*dt*/)
{
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		if (input.action == MenuInputAction::back)
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(std::make_unique<MainMenuHome>(
				this->menu_context()));
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			if (this->focus_.activate(0) == Activation::ran)
			{
				return;
			}
			continue;
		}
		if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
			continue;
		}

		// Left and right on the two rows that carry a value. The focus group
		// deliberately has no opinion here: which rows hold a value and what
		// cycling one means is this game's, not the engine's. The walk found
		// nothing to move to - every row shares a centre x - so the press
		// falls through to the page.
		this->adjust_focused_value(input.direction);
	}
}

void MainMenuOptions::adjust_focused_value(labrador::Direction direction)
{
	if (direction != labrador::Direction::left &&
		direction != labrador::Direction::right)
	{
		return;
	}

	const UiWidget* focused = this->focus_.focused(0);
	if (focused == this->resolution_element_.get())
	{
		this->play_wave(this->direction_sound_);
		this->cycle_resolution(direction);
	}
	else if (focused == this->full_screen_.get())
	{
		this->play_wave(this->direction_sound_);
		this->full_screen_selection_ = !this->full_screen_selection_;
		this->update_full_screen_selection_text();
	}
}

void MainMenuOptions::apply_settings()
{
	this->play_wave(this->confirm_sound_);

	bool fs = this->full_screen_selection_;

	// Fullscreen is a window restyle - WS_POPUP and SW_SHOWMAXIMIZED - and not
	// a display-mode change, so the back buffer comes out desktop-sized however
	// small a resolution this page picked. Applying both together left
	// ViewportManager and ui_camera() sizing from a number the screen does not
	// have and the game drawing into the top-left corner of it - saved, and
	// re-applied on every launch after.
	//
	// Nothing on ResolutionManager or Application reports the desktop size, so
	// the coercion is the other way round: while fullscreen is on the
	// resolution is left alone, and the row goes back to saying what is
	// actually in effect. Turning fullscreen off applies it as it always did,
	// and first, because set_fullscreen(false) sizes the window from it.
	if (fs)
	{
		this->resolution_selection_ = this->resolution_manager()->resolution();
		this->update_resolution_selection_text();
	}
	else
	{
		this->resolution_manager()->set_resolution(this->resolution_selection_);
		this->save()->set_resolution_and_save(this->resolution_selection_);

		this->data()->application()->set_resolution(this->resolution_selection_);
	}

	this->save()->set_full_screen_and_save(fs);
	this->apply_fullscreen_setting(fs);

	// The widgets are not touched. This is the page that changes the
	// resolution while its own widgets are on screen, so it is the page that
	// used to have to rescale them from the old size to the new one by hand -
	// the only site of the twenty-six that was not a one-off at construction.
	// ui_camera() reads the resolution every frame, so the next draw is
	// already at the new size, and navigation reads bounds() at the moment of
	// the press, in design space, where nothing moved at all.
}

void MainMenuOptions::apply_fullscreen_setting(bool fullscreen)
{
	this->data()->application()->set_fullscreen(fullscreen);
}

void MainMenuOptions::init()
{
	this->set_prompts({
		{ L"Left/Right", L"Change" },
		{ L"A", L"Select" },
		{ L"B", L"Back" } });

	this->focus_.set_style({ this->menu_style().highlight, this->menu_style().unhighlight });
	Vector2F widget_size = this->widget_size();

	this->resolution_selection_ = this->resolution_manager()->resolution();
	this->full_screen_selection_ = this->save()->fullscreen();

	this->background_ = std::make_unique<UiTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().options_background_colour);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Options",
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset);

	this->resolution_element_ = std::make_unique<UiTextDropShadow>(
		"resolution_element",
		L"Resolution",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		this->menu_style().highlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->resolution_value_ = std::make_unique<UiTextDropShadow>(
		"resolution_value",
		L"null",
		this->menu_style().item_font,
		this->calculate_widget_position(2, 2),
		this->render_resources(),
		this->look().options_value_colour,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->update_resolution_selection_text();

	this->full_screen_ = std::make_unique<UiTextDropShadow>(
		"full_screen",
		L"Fullscreen",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->full_screen_value_ = std::make_unique<UiTextDropShadow>(
		"full_screen_value",
		L"null",
		this->menu_style().item_font,
		this->calculate_widget_position(2, 3),
		this->render_resources(),
		this->look().options_value_colour,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->update_full_screen_selection_text();

	this->apply_ = std::make_unique<UiTextDropShadow>(
		"apply",
		L"Apply",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->back_ = std::make_unique<UiTextDropShadow>(
		"back",
		L"Back",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 5),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->resolution_element_.get());
	this->text_container_->add_child(this->resolution_value_.get());
	this->text_container_->add_child(this->full_screen_.get());
	this->text_container_->add_child(this->full_screen_value_.get());
	this->text_container_->add_child(this->apply_.get());
	this->text_container_->add_child(this->back_.get());

	// The two value rows are focusable and do nothing on A; left and right are
	// what they answer to. A button with no action is a normal thing to have.
	this->focus_.add(this->resolution_element_.get());
	this->focus_.add(this->full_screen_.get());
	this->focus_.add(this->apply_.get(), [this] { this->apply_settings(); });
	this->focus_.add(this->back_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuHome>(this->menu_context()));
		});

	this->play_effect(this->music_, true, this->look().music.volume);
}

void MainMenuOptions::cycle_resolution(
	labrador::Direction direction)
{
	int enum_max = static_cast<int>(ScreenResolution::max);
	int enum_pos =
		static_cast<int>(this->resolution_selection_);
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
	this->resolution_selection_ =
		static_cast<ScreenResolution>(enum_pos);
	this->update_resolution_selection_text();
}

void MainMenuOptions::update_resolution_selection_text()
{
	// The resolution string round-trips through the save file, so it stays
	// narrow at its source and comes across here.
	const std::string res_text = this->resolution_manager()->
		convert_resolution_to_string(this->resolution_selection_);
	this->resolution_value_->set_text(widen(res_text));
}

void MainMenuOptions::update_full_screen_selection_text() const
{
	bool fs = this->full_screen_selection_; //this->context()->data()->save()->full_screen();
	std::wstring fs_text = L"";
	if (fs)
	{
		fs_text = L"On";
	}
	else
	{
		fs_text = L"Off";
	}
	this->full_screen_value_->set_text(fs_text);
}

#pragma endregion MainMenuOptions

#pragma region MainMenuModeSelect

MainMenuModeSelect::MainMenuModeSelect(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuModeSelect::update(float /*dt*/)
{
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		if (input.action == MenuInputAction::back)
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(std::make_unique<MainMenuHome>(
				this->menu_context()));
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			if (this->focus_.activate(0) == Activation::ran)
			{
				return;
			}
		}
		else if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
		}
	}
}
void MainMenuModeSelect::init()
{
	this->set_prompts({
		{ L"A", L"Select" },
		{ L"B", L"Back" } });

	this->focus_.set_style({ this->menu_style().highlight, this->menu_style().unhighlight });

	this->set_widget_spacing(this->look().mode_select_spacing);

	this->background_ = std::make_unique<UiTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().play_background_colour);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Mode Select",
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset);

	this->standard_ = std::make_unique<UiTextDropShadow>(
		"standard",
		L"Standard",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		this->menu_style().highlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	// The three that are not built. They take menu_style().disabled at
	// construction and keep it, because they are never handed to focus_ - see
	// the note where the other three rows are registered.
	this->tdm_ = std::make_unique<UiTextDropShadow>(
		"tdm",
		L"Team Deathmatch",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		this->menu_style().disabled,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->dm_ = std::make_unique<UiTextDropShadow>(
		"dm",
		L"Deathmatch",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		this->menu_style().disabled,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->practice_ = std::make_unique<UiTextDropShadow>(
		"practice",
		L"Practice",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 5),
		this->render_resources(),
		this->menu_style().disabled,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->back_ = std::make_unique<UiTextDropShadow>(
		"back",
		L"Back",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 6),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->standard_.get());
	this->text_container_->add_child(this->tdm_.get());
	this->text_container_->add_child(this->dm_.get());
	this->text_container_->add_child(this->practice_.get());
	this->text_container_->add_child(this->back_.get());

	this->focus_.add(this->standard_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuPlayerCount>(this->menu_context()));
		});
	// Three modes that are not built yet, and so are not added here at all.
	//
	// They used to be registered with an action that beeped, which made them
	// look exactly like every working row - the same unhighlight colour as
	// Back, one screen after the title - and answer a stranger's second
	// interaction with the game with a buzz and no explanation. They cannot
	// instead be registered and recoloured: FocusGroup derives the paint,
	// repainting every widget it holds from the focus of every slot on every
	// add, move and set_style, so a disabled colour on a registered widget
	// survives until the next cursor movement. Its header names the case and
	// leaves the page to reapply the colour afterwards, which is a repaint
	// fighting a repaint.
	//
	// So the cursor is never on them. focus_.move walks by bounds() rather
	// than by registration order, so Standard and Back remain neighbours with
	// three greyed labels drawn between them, and text_container_ still draws
	// all six.
	//
	// The cost is the beep: pressing A while on Standard cannot report that
	// Deathmatch is unavailable, because there is no way to be on Deathmatch.
	// That is the better trade while the rows are unbuildable, and the whole
	// question goes away when they are built.
	this->focus_.add(this->back_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuHome>(this->menu_context()));
		});

	this->play_effect(this->music_, true, this->look().music.volume);
}

#pragma endregion MainMenuModeSelect

#pragma region MainMenuPlayerCount

MainMenuPlayerCount::MainMenuPlayerCount(MenuContext* context) :
	MainMenuPage(context)
{

}
void MainMenuPlayerCount::update(float /*dt*/)
{
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		if (input.action == MenuInputAction::back)
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuModeSelect>(this->menu_context()));
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			if (this->focus_.activate(0) == Activation::ran)
			{
				return;
			}
		}
		else if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
			this->not_enough_pads_->set_hidden(true);
		}
	}
}
void MainMenuPlayerCount::init()
{
	this->set_prompts({
		{ L"A", L"Select" },
		{ L"B", L"Back" } });

	this->focus_.set_style({ this->menu_style().highlight, this->menu_style().unhighlight });

	this->set_widget_spacing(this->look().player_count_spacing);

	this->background_ = std::make_unique<UiTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().play_background_colour);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Number of Players",
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset);

	this->_1_player = std::make_unique<UiTextDropShadow>(
		"1_player",
		L"1 Player",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		this->menu_style().highlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->_2_players = std::make_unique<UiTextDropShadow>(
		"2_players",
		L"2 Players",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->_3_players = std::make_unique<UiTextDropShadow>(
		"3_players",
		L"3 Players",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->_4_players = std::make_unique<UiTextDropShadow>(
		"4_players",
		L"4 Players",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 5),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->back_ = std::make_unique<UiTextDropShadow>(
		"back",
		L"Back",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 6),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	// Row 7, not 8. This page spaces its rows 100 apart from a base of 150, so
	// row 8 is design y 950 - and the prompt bar sits at 990 in a face whose
	// line box reaches back up past it. The two overlapped by a few pixels,
	// descenders into capitals, at exactly the moment both are trying to tell
	// a confused player something. Row 7 is 850 and is otherwise empty: this
	// page's last focusable row, Back, is row 6.
	this->not_enough_pads_ = std::make_unique<UiTextDropShadow>(
		"not_enough_pads",
		L"Not enough controllers",
		this->menu_style().detail_font,
		this->calculate_widget_position(0, 7),
		this->render_resources(),
		this->menu_style().warning,
		this->menu_style().shadow_colour,
		this->menu_style().detail_shadow_offset,
		true);

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->_1_player.get());
	this->text_container_->add_child(this->_2_players.get());
	this->text_container_->add_child(this->_3_players.get());
	this->text_container_->add_child(this->_4_players.get());
	this->text_container_->add_child(this->back_.get());
	this->text_container_->add_child(this->not_enough_pads_.get());

	// One player skips team select: there is no other team to be on.
	this->focus_.add(this->_1_player.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			MenuLevelSettings* settings =
				this->level_settings();
			settings->set_player_count(1);
			settings->set_player_team(0, PlayerTeam::a);
			settings->set_screen_layout(ScreenLayout::one_player);
			// The pad that is actually there, which is not always slot 0. The
			// match reads a player's input by pad slot, so writing 0 here left
			// the one player on a lone pad in slot 1 unable to move.
			const std::vector<int> slots = this->connected_slots();
			settings->set_player_num(0, slots.empty() ? 0 : slots.front());
			this->context()->transition_to(
				std::make_unique<MainMenuWeaponSelect>(this->menu_context()));
		});
	this->focus_.add(this->_2_players.get(),
		[this] { this->start_team_select(2, ScreenLayout::two_player); });
	this->focus_.add(this->_3_players.get(),
		[this] { this->start_team_select(3, ScreenLayout::three_player); });
	this->focus_.add(this->_4_players.get(),
		[this] { this->start_team_select(4, ScreenLayout::four_player); });
	this->focus_.add(this->back_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuModeSelect>(this->menu_context()));
		});

	this->play_effect(this->music_, true, this->look().music.volume);
}

// Refused rather than started, when the count is more than the pads on the
// table.
//
// Team select drives one row per *connected pad* - `i < num_inputs && i <
// player_count` - while all_players_confirmed() asks about every player up to
// player_count. Four players on two pads therefore reaches a page that cannot
// be finished by any sequence of inputs: players 2 and 3 have no pad to confirm
// with and the transition waits on them forever. The way out is to work out
// that Back is gated on all_players_unconfirmed() and that both live pads must
// therefore unconfirm first, which nothing on the screen says.
//
// Past that page it is no better: set_level_settings falls back to `slots[i] :
// i`, welding two of the four characters to slots with no pad in them, and the
// match starts with two players who cannot move.
//
// Refused here rather than gated inside team select's update(), which runs
// every frame and would beep sixty times a second. The rows are not greyed the
// way Mode Select's unbuilt ones are: a pad can be plugged in at any moment,
// and the disabled treatment there works precisely because it is decided once
// and never changes.
void MainMenuPlayerCount::start_team_select(int players, ScreenLayout layout)
{
	if (players > static_cast<int>(this->connected_slots().size()))
	{
		this->play_wave(this->error_sound_);
		this->not_enough_pads_->set_hidden(false);
		return;
	}

	this->play_wave(this->confirm_sound_);
	MenuLevelSettings* settings = this->level_settings();
	settings->set_player_count(players);
	settings->set_screen_layout(layout);
	this->context()->transition_to(
		std::make_unique<MainMenuTeamSelect>(this->menu_context()));
}

#pragma endregion MainMenuPlayerCount

#pragma region MainMenuTeamSelect

MainMenuTeamSelect::MainMenuTeamSelect(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuTeamSelect::update(float /*dt*/)
{
	// The connected pads only, in slot order, so entry i is player i's. Reading
	// menu_inputs() straight indexes by pad slot instead, and then a couch whose
	// first pad is in slot 1 leaves player 0 on an empty slot - unable to pick a
	// team and, since back is read in this same loop, unable to leave.
	const std::vector<ProcessedMenuInput> inputs = this->connected_inputs();
	const int num_inputs = static_cast<int>(inputs.size());
	const int player_count = this->player_count();

	for (int i = 0; i < num_inputs && i < player_count; i++)
	{
		if (inputs[i].action == MenuInputAction::back)
		{
			if (this->all_players_unconfirmed())
			{
				this->play_wave(this->cancel_sound_);
				this->context()->transition_to(
					std::make_unique<MainMenuPlayerCount>(
						this->menu_context()));
				return;
			}
			if (this->select_states_[i].state == ConfirmationState::confirmed)
			{
				this->play_wave(this->cancel_sound_);
				this->select_states_[i].state =
					ConfirmationState::unconfirmed;
			}
			//no break here as I want simultaneous input from players to be possible
		}
		else if (inputs[i].action == MenuInputAction::proceed &&
			this->select_states_[i].team != PlayerTeam::none)
		{
			// The only gate on a confirm used to be that a team had been
			// chosen at all, and nothing anywhere asked which. Four players
			// could all pick team A, all confirm, and start a match that ran
			// 4v0 for four minutes against an opponent who did not exist -
			// every stage lists four spawns per team, so nothing downstream
			// tripped either.
			//
			// Refused on the press that would complete the roster, so a
			// player choosing a side early is never argued with and the one
			// who would have started the match is told why it did not.
			if (this->confirm_completes_roster(i) &&
				!this->roster_is_playable())
			{
				this->play_wave(this->error_sound_);
				this->both_teams_needed_->set_hidden(false);
			}
			else
			{
				this->play_wave(this->confirm_sound_);
				this->select_states_[i].state = ConfirmationState::confirmed;
			}
		}
		else if (this->select_states_[i].state ==
			ConfirmationState::unconfirmed)
		{
			if (inputs[i].direction == labrador::Direction::left)
			{
				this->play_wave(this->direction_sound_);
				if (this->select_states_[i].team == PlayerTeam::none)
				{
					this->select_states_[i].team = PlayerTeam::a;
				}
				else if (this->select_states_[i].team == PlayerTeam::b)
				{
					this->select_states_[i].team = PlayerTeam::none;
				}
			}
			else if (inputs[i].direction == labrador::Direction::right)
			{
				this->play_wave(this->direction_sound_);
				if (this->select_states_[i].team == PlayerTeam::none)
				{
					this->select_states_[i].team = PlayerTeam::b;
				}
				else if (this->select_states_[i].team == PlayerTeam::a)
				{
					this->select_states_[i].team = PlayerTeam::none;
				}
			}
		}
	}
	// Cleared as soon as somebody crosses to the other side, rather than on a
	// timer or on the next press: the message is about a state, and the state
	// is over. The same predicate as the refusal, so the two cannot disagree
	// about what is wrong.
	if (this->roster_is_playable())
	{
		this->both_teams_needed_->set_hidden(true);
	}

	// Reachable only through a confirm that passed the check above - a player
	// cannot change team while confirmed, and unconfirming means confirming
	// again - so this transition can no longer start a one-team match.
	if (this->all_players_confirmed())
	{
		this->set_level_settings();
		this->context()->transition_to(
			std::make_unique<MainMenuWeaponSelect>(
				this->menu_context()));
		return;
	}
	this->update_team_select_visuals();
}
bool MainMenuTeamSelect::confirm_completes_roster(int player) const
{
	for (int i = 0; i < this->player_count(); i++)
	{
		if (i != player &&
			this->select_states_[i].state != ConfirmationState::confirmed)
		{
			return false;
		}
	}
	return true;
}
bool MainMenuTeamSelect::roster_is_playable() const
{
	// One player has nobody to need. The one-player row skips this page
	// entirely, but the end menu's "Change Teams" comes back to it with a
	// count of one - and that is a route worth keeping, because it is the only
	// way a solo player can play as team B. Requiring both teams there refused
	// every press with no way forward, on the row the end menu highlights by
	// default.
	//
	// A solo roster is exactly what LevelEndInfo::one_team_only was written
	// for, so it is a match this game can now finish honestly.
	if (this->player_count() < 2)
	{
		return true;
	}

	return this->both_teams_chosen();
}
bool MainMenuTeamSelect::both_teams_chosen() const
{
	bool team_a = false;
	bool team_b = false;
	for (int i = 0; i < this->player_count(); i++)
	{
		switch (this->select_states_[i].team)
		{
		case PlayerTeam::a:
			team_a = true;
			break;
		case PlayerTeam::b:
			team_b = true;
			break;
		default:
			break;
		}
	}
	return team_a && team_b;
}
void MainMenuTeamSelect::set_level_settings() const
{
	// Player i's pad slot, which is not i: this page is driven by the connected
	// pads in slot order, and the match reads a player's input by pad slot, so
	// the two have to be the same mapping or the match starts on dead pads.
	const std::vector<int> slots = this->connected_slots();
	for (int i = 0; i < this->player_count(); i++)
	{
		this->level_settings()->
			set_player_team(i, this->select_states_[i].team);

		this->level_settings()->
			set_player_num(i,
				i < static_cast<int>(slots.size()) ? slots[i] : i);
	}
}
bool MainMenuTeamSelect::all_players_confirmed() const
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
bool MainMenuTeamSelect::all_players_unconfirmed() const
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
void MainMenuTeamSelect::update_team_select_visuals()
{
	this->deselect_and_unconfirm_all_widgets();

	for (int i = 0; i < this->player_count(); i++)
	{
		UiWidget* selected_widget = nullptr;
		switch (this->select_states_[i].team)
		{
		case PlayerTeam::a:
			this->player_widgets_[i]->player_a->set_sprite_frame(
				"team_select_a_selected");
			selected_widget = this->player_widgets_[i]->player_a.get();
			break;
		case PlayerTeam::none:
			this->player_widgets_[i]->player_center->set_sprite_frame(
				"team_select_center_selected");
			selected_widget = this->player_widgets_[i]->player_center.get();
			break;
		case PlayerTeam::b:
			this->player_widgets_[i]->player_b->set_sprite_frame(
				"team_select_b_selected");
			selected_widget = this->player_widgets_[i]->player_b.get();
			break;
		}
		if (this->select_states_[i].state ==
			ConfirmationState::confirmed)
		{
			selected_widget->set_colour(
				this->look().team_select.selected);
		}
	}
}
void MainMenuTeamSelect::deselect_and_unconfirm_all_widgets()
{
	for (int i = 0; i < this->player_count(); i++)
	{
		this->player_widgets_[i]->player_a->set_sprite_frame(
			"team_select_a");
		this->player_widgets_[i]->player_center->set_sprite_frame(
			"team_select_center");
		this->player_widgets_[i]->player_b->set_sprite_frame(
			"team_select_b");

		this->player_widgets_[i]->player_a->set_colour(
			this->look().team_select.unselected);
		this->player_widgets_[i]->player_center->set_colour(
			this->look().team_select.unselected);
		this->player_widgets_[i]->player_b->set_colour(
			this->look().team_select.unselected);
	}
}
void MainMenuTeamSelect::init()
{
	this->set_prompts({
		{ L"Left/Right", L"Pick a team" },
		{ L"A", L"Confirm" },
		{ L"B", L"Cancel" } });

	this->set_widget_spacing(this->look().team_select.spacing);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	
	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");

	this->background_ = std::make_unique<UiTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().play_background_colour);

	this->texture_container_->add_child(this->background_.get());

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Team Select",
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset);

	this->text_container_->add_child(this->heading_.get());

	// Row 6, under the four player rows, which occupy rows 2 to 5. Row 7 is
	// off the bottom: this page spaces its rows 125 apart from a base of 150,
	// so row 7 is y 1025 in a design space 1080 tall and the descenders of
	// "player" would have been cut off by the screen edge.
	this->both_teams_needed_ = std::make_unique<UiTextDropShadow>(
		"both_teams_needed",
		L"Both teams need a player",
		this->menu_style().detail_font,
		this->calculate_widget_position(0, 6),
		this->render_resources(),
		this->menu_style().warning,
		this->menu_style().shadow_colour,
		this->menu_style().detail_shadow_offset,
		true);

	this->text_container_->add_child(this->both_teams_needed_.get());


	for (int i = 0; i < this->player_count(); i++)
	{
		auto state = TeamSelectState();
		state.state = ConfirmationState::unconfirmed;
		state.team = this->level_settings()->
			player_team(i);
		this->select_states_.push_back(state);

		auto widgets = std::make_unique<PlayerWidgets>();

		std::string name = "player_" + std::to_string(i + 1);
		std::wstring label_text = L"Player " + std::to_wstring(i + 1);

		widgets->player = std::make_unique<UiTextDropShadow>(
			name,
			label_text,
			this->menu_style().item_font,
			this->calculate_widget_position(0, i + 2),
			this->render_resources(),
			this->look().team_select.unselected,
			this->menu_style().shadow_colour,
			this->menu_style().item_shadow_offset);

		widgets->player_a = std::make_unique<UiTexture>(
			name + "_a",
			"sprite_sheet_1",
			"team_select_a",
			RectangleF(this->calculate_widget_position(2, i + 2), this->look().team_select.size),
			this->render_resources());

		auto move = Vector2F(this->look().team_select.size.x, 0.0f);

		widgets->player_center = std::make_unique<UiTexture>(
			name + "_center",
			"sprite_sheet_1",
			"team_select_center",
			RectangleF(widgets->player_a->rectangle().position() + move,
				this->look().team_select.size),
			this->render_resources());

		widgets->player_b = std::make_unique<UiTexture>(
			name + "_b",
			"sprite_sheet_1",
			"team_select_b",
			RectangleF(widgets->player_center->rectangle().position() + move,
				this->look().team_select.size),
			this->render_resources());

		this->texture_container_->add_child(widgets->player_a.get());
		this->texture_container_->add_child(widgets->player_center.get());
		this->texture_container_->add_child(widgets->player_b.get());

		this->text_container_->add_child(widgets->player.get());

		this->player_widgets_.push_back(std::move(widgets));
	}

	this->play_effect(this->music_, true, this->look().music.volume);
}

#pragma endregion MainMenuTeamSelect

#pragma region MainMenuWeaponSelect

MainMenuWeaponSelect::MainMenuWeaponSelect(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuWeaponSelect::update(float /*dt*/)
{
	// The connected pads only, in slot order, so entry i is player i's - the
	// same mapping team select wrote into the settings. Indexed by pad slot, a
	// player whose pad is not in slot i can neither choose nor go back.
	const std::vector<ProcessedMenuInput> inputs = this->connected_inputs();
	const int player_count = this->player_count();
	const int num_inputs = static_cast<int>(inputs.size());

	for (int i = 0; i < num_inputs && i < player_count; i++)
	{
		if (inputs[i].action == MenuInputAction::back)
		{
			if (this->all_players_unconfirmed())
			{
				this->play_wave(this->cancel_sound_);
				if (this->player_count() == 1)
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
				this->play_wave(this->cancel_sound_);
				this->select_states_[i].state =
					ConfirmationState::unconfirmed;
			}
			//no break here as I want simultaneous input
			// from players to be possible
		}
		else if (inputs[i].action == MenuInputAction::proceed)
		{
			this->play_wave(this->confirm_sound_);
			this->select_states_[i].state = ConfirmationState::confirmed;
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
		//check if any players have selected random then pick a random weapon.
		for (int i = 0; i < this->player_count(); i++)
		{
			if (this->select_states_[i].type == WeaponType::random_primary)
			{
				this->select_states_[i].type = random_weapon();
			}
		}
		this->set_level_settings();
		this->context()->transition_to(
			std::make_unique<MainMenuStageSelect>(this->menu_context()));
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
	int enum_pos =
		static_cast<int>(this->select_states_[player_index].type);
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

	for (int i = 0; i < this->player_count(); i++)
	{
		switch (this->select_states_[i].type)
		{
		case WeaponType::sprayer:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("sprayer");
			this->player_widgets_[i]->weapon_name->set_text(L"Sprayer");
			break;
		case WeaponType::sniper:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("sniper");
			this->player_widgets_[i]->weapon_name->set_text(L"Sniper");
			break;
		case WeaponType::roller:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("roller");
			this->player_widgets_[i]->weapon_name->set_text(L"Roller");
			break;
		case WeaponType::mister:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("mister");
			this->player_widgets_[i]->weapon_name->set_text(L"Mister");
			break;
		case WeaponType::bucket:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("bucket");
			this->player_widgets_[i]->weapon_name->set_text(L"Bucket");
			break;
		case WeaponType::random_primary:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("random");
			this->player_widgets_[i]->weapon_name->set_text(L"Random");
			break;
		default:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("error");
			this->player_widgets_[i]->weapon_name->set_text(L"ERROR");
			break;
		}

		if (this->select_states_[i].state == ConfirmationState::confirmed)
		{
			this->player_widgets_[i]->weapon_icon->
				set_colour(this->look().weapon_select.selected);
			this->player_widgets_[i]->weapon_name->
				set_colour(this->look().weapon_select.selected);
		}

		this->player_widgets_[i]->weapon_description->set_text(
			this->weapon_description(this->select_states_[i].type));
	}
}
void MainMenuWeaponSelect::unconfirm_all_widgets()
{
	for (int i = 0; i < this->player_count(); i++)
	{
		this->player_widgets_[i]->weapon_icon->set_colour(
			this->look().weapon_select.unselected);
		this->player_widgets_[i]->weapon_name->set_colour(
			this->look().weapon_select.unselected);
	}
}
void MainMenuWeaponSelect::init()
{
	this->set_prompts({
		{ L"Left/Right", L"Change weapon" },
		{ L"A", L"Confirm" },
		{ L"B", L"Cancel" } });

	this->set_widget_spacing(this->look().weapon_select.spacing);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	
	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");

	this->background_ = std::make_unique<UiTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().play_background_colour);

	this->texture_container_->add_child(this->background_.get());

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Weapon Select",
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset);

	this->text_container_->add_child(this->heading_.get());

	for (int i = 0; i < this->player_count(); i++)
	{
		SelectState state;
		state.state = ConfirmationState::unconfirmed;
		state.type = this->level_settings()
			->player_weapon(i);
		this->select_states_.push_back(state);

		auto widgets = std::make_unique<Widgets>();

		std::string name = "player_" + std::to_string(i + 1);
		std::wstring label_text = L"Player " + std::to_wstring(i + 1);

		widgets->player = std::make_unique<UiTextDropShadow>(
			name,
			label_text,
			this->menu_style().item_font,
			this->calculate_widget_position(0, i + 2),
			this->render_resources(),
			this->look().weapon_select.unselected);

		widgets->weapon_icon = std::make_unique<UiTexture>(
			name + "_wep_icon",
			"sprite_sheet_1",
			"sprayer",
			RectangleF(this->calculate_widget_position(2, i + 2), this->look().weapon_select.size),
			this->render_resources(),
			this->look().play_background_colour);

		widgets->weapon_name = std::make_unique<UiTextDropShadow>(
			name + "_wep_name",
			L"Sprayer",
			this->menu_style().detail_font,
			widgets->weapon_icon->rectangle().position() +
				Vector2F(0.0f, this->look().weapon_select.size.y),
			this->render_resources(),
			this->look().weapon_select.unselected,
			this->menu_style().shadow_colour,
			this->menu_style().detail_shadow_offset);

		widgets->weapon_description = std::make_unique<UiTextDropShadow>(
			name + "_wep_desc",
			this->weapon_description(WeaponType::sprayer),
			this->look().weapon_description_font,
			widgets->weapon_icon->rectangle().position() +
				Vector2F(this->look().weapon_description_x_offset, 0.0f),
			this->render_resources(),
			this->look().weapon_description_colour,
			this->menu_style().shadow_colour,
			this->look().weapon_description_shadow_offset);

		this->texture_container_->add_child(widgets->weapon_icon.get());
		this->text_container_->add_child(widgets->weapon_name.get());
		this->text_container_->add_child(widgets->player.get());
		this->text_container_->add_child(widgets->weapon_description.get());

		this->player_widgets_.push_back(std::move(widgets));
	}
	this->play_effect(this->music_, true, this->look().music.volume);
}
std::wstring MainMenuWeaponSelect::weapon_description(WeaponType type) const
{
	// A lookup, where this was a six-branch switch ending in a default that
	// returned the literal L"ERROR" - a string a player could see on the
	// weapon-select screen if the enum ever grew a seventh pickable weapon.
	// An untuned weapon reads as empty instead.
	const size_t index = static_cast<size_t>(type);
	const auto& descriptions = this->look().weapon_descriptions;
	if (index >= descriptions.size())
	{
		return std::wstring();
	}
	return descriptions[index];
}

#pragma endregion MainMenuWeaponSelect

#pragma region MainMenuStageSelect

MainMenuStageSelect::MainMenuStageSelect(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuStageSelect::update(float /*dt*/)
{
	// The connected pads only, in slot order, so entry i is player i's - the
	// same mapping the two pages before this one used. Indexed by pad slot, a
	// player whose pad is not in slot i can neither cycle the stage nor go back.
	const std::vector<ProcessedMenuInput> inputs = this->connected_inputs();
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
			// No break here as I want simultaneous input from players to be
			// possible. That is about this arm only - unconfirming is a change
			// to the shared state that two players may make in the same frame.
			// The proceed arm below can leave the screen, and where it does it
			// returns instead.
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

				// The end of the main menu, and the whole of what it reports:
				// the settings it assembled are already in the MenuContext
				// GameMenu owns, so closing is the message. It used to write
				// true through a heap-allocated bool that GameMenu read on
				// every frame of every menu page to find out.
				//
				// One pop, whatever the other pads did this frame. Both pops
				// would queue and both would drain (state_context.h), so a
				// second A on the same tick took GameMenu off the stack too -
				// out from under this page, taking its result callback with it.
				this->context()->pop();
				return;
			}
			else
			{
				this->play_wave(this->ready_sound_);
				this->select_state_.state = ConfirmationState::confirmed;
			}
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
	this->unconfirm_all_widgets();

	// Every stage names and pictures itself, from its own level file, so this
	// reads the list rather than a switch that has to grow a case per stage.
	// init() builds the icon from the same two helpers, so the first frame
	// painted is the selected stage's and no name that is not in the sheet is
	// ever handed to the eager resolve.
	const int slot = this->select_state_.slot;
	this->stage_icon_->set_sprite_frame(this->slot_icon_frame(slot));
	this->stage_name_->set_text(this->slot_display_name(slot));
	if (this->select_state_.state == ConfirmationState::confirmed)
	{
		this->ready_->set_hidden(false);
	}
}
void MainMenuStageSelect::unconfirm_all_widgets()
{
	this->stage_icon_->set_colour(
		this->look().stage_select.unselected);
	this->stage_name_->set_colour(
		this->look().stage_select.unselected);
	this->ready_->set_hidden(true);
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
std::string MainMenuStageSelect::slot_icon_frame(int slot) const
{
	if (slot == this->random_slot())
	{
		return "random";
	}
	const StageList* stages = this->data()->stages();
	if (slot >= 0 && slot < static_cast<int>(stages->size()))
	{
		return stages->at(static_cast<size_t>(slot)).icon_frame;
	}
	return "error";
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
		// The same conversion the stage was checked with at load, so the two
		// cannot disagree about which characters are acceptable. It cannot
		// throw here: main.cpp ran it over this exact string before the stage
		// joined the list.
		return drawable_text::widen_drawable(
			stages->at(static_cast<size_t>(slot)).display_name,
			"the stage list");
	}
	return L"ERROR";
}
int MainMenuStageSelect::pick_random_stage() const
{
	return rand() % this->random_slot();
}
void MainMenuStageSelect::init()
{
	this->set_prompts({
		// A twice: the first press confirms the stage, the second starts the
		// match. One legend for both, because it is the same button and the
		// bar is built once in init().
		{ L"Left/Right", L"Change stage" },
		{ L"A", L"Confirm, then start" },
		{ L"B", L"Back" } });

	this->select_state_.slot = this->slot_of(this->level_settings()->stage());

	this->set_widget_spacing(this->look().stage_select.spacing);

	this->background_ = std::make_unique<UiTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().play_background_colour);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Stage Select",
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset);


	// Both are built from the slot the cursor starts on, not from a literal.
	// The frame is resolved eagerly, inside UiTexture's construction, so a
	// name that is not in the sheet throws out of here rather than showing
	// wrong - which is what "stage_test_1" did, a frame the sheet has not
	// carried for three stages.
	this->stage_icon_ = std::make_unique<UiTexture>(
		"stage_icon",
		"sprite_sheet_1",
		this->slot_icon_frame(this->select_state_.slot),
		RectangleF(this->calculate_widget_position(0, 1), this->look().stage_select.size),
		this->render_resources(),
		this->look().play_background_colour);

	this->stage_name_ = std::make_unique<UiTextDropShadow>(
		"stage_name",
		this->slot_display_name(this->select_state_.slot),
		this->menu_style().item_font,
		this->calculate_widget_position(0, 5),
		this->render_resources(),
		this->look().stage_select.unselected,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->ready_ = std::make_unique<UiTextDropShadow>(
		"ready",
		L"READY?",
		this->look().announcement_font,
		this->background_->rectangle().center() - Vector2F(400.0f, 100.0f),
		this->render_resources(),
		this->look().stage_select.selected,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset,
		true);

	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->stage_icon_.get());

	this->text_container_ = std::make_unique<UiContainer>("text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->stage_name_.get());
	this->text_container_->add_child(this->ready_.get());

	this->play_effect(this->music_, true, this->look().music.volume);
}
void MainMenuStageSelect::cycle_stages(labrador::Direction direction)
{
	// Slots run 0..n-1 for the stages and n for Random, and the cursor wraps.
	const int last = this->random_slot();
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