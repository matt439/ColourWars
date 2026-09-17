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

void MainMenuPage::set_player_prompts(
	const std::vector<PromptBar::Prompt>& prompts)
{
	this->player_prompts_ = prompts;
	this->set_prompts(prompts);
}

bool MainMenuPage::wait_for_players(
	const std::vector<ProcessedMenuInput>& inputs)
{
	// Focus loss can temporarily hide every pad. It does not change the roster.
	if (!this->context()->active())
	{
		return true;
	}
	const bool missing = std::any_of(inputs.begin(), inputs.end(),
		[](const ProcessedMenuInput& input) { return !input.connected; });
	if (missing != this->waiting_for_players_)
	{
		this->waiting_for_players_ = missing;
		if (missing)
		{
			this->set_prompts({
				{ L"", L"Reconnect controller" },
				{ L"B", L"Change players" } });
		}
		else
		{
			this->set_prompts(this->player_prompts_);
		}
	}
	if (missing)
	{
		// Any available device may recover a setup whose owner disconnected.
		for (const ProcessedMenuInput& input : this->menu_inputs())
		{
			if (input.action == MenuInputAction::back)
			{
				this->play_wave(this->cancel_sound_);
				this->context()->transition_to(
					std::make_unique<MainMenuPlayerCount>(this->menu_context()));
				break;
			}
		}
	}
	return missing;
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
	//
	// A query and not the on_activated/on_deactivated pair this page also has,
	// because the gate is on a refresh that happens every frame rather than on
	// anything that happens at the moment of the change. The callbacks could
	// only set a bool for this to read, which is the hand-written copy of the
	// engine's edge that the pin bump deleted from GameLevel.
	if (this->context()->active())
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

		// Left and right on the rows that carry a value. The focus group
		// deliberately has no opinion here: which rows hold a value and what
		// cycling one means is this game's, not the engine's. The walk found
		// nothing to move to, so the press falls through to the page.
		//
		// It finds nothing because every entry on this page overlaps every
		// other on the horizontal axis by more than the walk's tolerance -
		// they all start at the same left edge. That used to be put as "every
		// row shares a centre x", which stopped being true when the two value
		// rows became containers spanning their label and their value; the
		// answer did not change, because overlap is what the walk measures.
		this->adjust_focused_value(input.direction);
	}
}

// Asked of the focused row rather than worked out from it. This used to be one
// arm per value row, comparing the focused pointer against each label in turn
// and carrying that row's meaning in the arm - so a third setting meant a third
// comparison here, and the label had to be the thing registered because the
// value beside it was a separate widget nothing pointed at.
//
// A row is one focusable object that owns its value and what changing it means,
// so this asks only whether the cursor is on such a thing at all. Apply and
// Back are not, and answer null.
void MainMenuOptions::adjust_focused_value(labrador::Direction direction)
{
	if (direction != labrador::Direction::left &&
		direction != labrador::Direction::right)
	{
		return;
	}

	const OptionRow* row =
		dynamic_cast<const OptionRow*>(this->focus_.focused(0));
	if (row == nullptr)
	{
		return;
	}

	// A disabled row does not answer left and right either. move() will not
	// walk onto one, but it can be switched off under a cursor already sitting
	// there - Apply turning fullscreen on does exactly that - and a greyed row
	// that still cycled would be the one thing on the page saying two things
	// at once.
	if (!this->focus_.enabled(row))
	{
		return;
	}

	this->play_wave(this->direction_sound_);
	row->adjust(direction);
}

void MainMenuOptions::apply_settings()
{
	this->play_wave(this->confirm_sound_);

	const bool fs = this->full_screen_selection_;

	// THE COERCION THIS USED TO DO IS GONE, and the engine is why.
	//
	// Fullscreen here is a window restyle - WS_POPUP and SW_SHOWMAXIMIZED -
	// and not a display-mode change, so the back buffer comes out
	// desktop-sized however small a resolution this page picked. That used to
	// leave ViewportManager and ui_camera() sizing from a number the screen
	// did not have, with the game drawn into the top-left corner of its own
	// window. This page worked around it by refusing to apply a resolution
	// while fullscreen was on.
	//
	// The engine closed it in 0702776: every route into a resize arrives at
	// on_window_size_changed, which re-points ResolutionManager at the real
	// client size before the renderer resizes. So the layout follows the
	// window now whatever this page does, and there is nothing left to protect
	// it from.
	//
	// It also remembers the way back. Application::set_resolution records the
	// preset as the size the game asked for, and set_fullscreen(false) returns
	// to that rather than to the monitor - which is the other half this page
	// used to hand-hold.
	//
	// THE PICK IS ALWAYS SAVED AND ONLY SOMETIMES APPLIED, which is two
	// decisions and they are not the same one.
	//
	// Saving it always is what makes the choice survive: the row is disabled
	// while fullscreen, so the only place a resolution can be picked is here,
	// windowed, possibly in the same press that turns fullscreen on. That pick
	// is the size the window comes back to, and it has to outlive a restart -
	// main.cpp seeds ApplicationOptions from the save.
	//
	// Applying it only when the game will end up WINDOWED is about not
	// resizing a window twice in one call. set_resolution resizes and
	// set_fullscreen(true) resizes again immediately, and each of those is a
	// synchronous WM_SIZE into the renderer's swap chain - from inside a
	// window procedure, since this runs under the tick. That sequence killed
	// the process once during testing with STATUS_FATAL_USER_CALLBACK_EXCEPTION
	// (0xc000041d), an exception escaping the window proc, and it did not
	// reproduce on the next run. Whatever the race is, this page has no reason
	// to be in it: a window about to become fullscreen does not need to be
	// resized to something else first.
	//
	// Nothing is lost by skipping it. Application::set_fullscreen(false) sizes
	// the window from options_.resolution, and the way out of fullscreen goes
	// through the branch below - which calls set_resolution first, and so sets
	// that very field to the pick before it is read.
	this->save()->set_resolution_and_save(this->resolution_selection_);

	if (!fs)
	{
		// Not routed through the resolution manager first: Application does
		// that itself, and the second call was this page saying the same thing
		// twice.
		this->data()->application()->set_resolution(this->resolution_selection_);
	}

	this->save()->set_full_screen_and_save(fs);
	this->apply_fullscreen_setting(fs);
	this->save()->set_team_palette_and_save(this->team_palette_selection_);

	// After applying the display settings: this reads what is now in effect,
	// rather than what was asked for.
	this->refresh_resolution_row();

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
	this->set_widget_spacing(this->look().options_spacing);
	this->set_prompts({
		{ L"Left/Right", L"Change" },
		{ L"A", L"Select" },
		{ L"B", L"Back" } });

	// Three, because the Resolution row switches off while fullscreen owns the
	// size. OptionRow paints only its label with this, so a disabled row greys
	// its name and leaves the value legible - which is the point, since the
	// value is the thing the player is being shown.
	this->focus_.set_style({ this->menu_style().highlight,
		this->menu_style().unhighlight,
		this->menu_style().disabled });
	Vector2F widget_size = this->widget_size();

	// FROM THE SAVE, NOT FROM THE RESOLUTION MANAGER, and the difference only
	// shows up in fullscreen - where it is a bug that writes itself into the
	// save file and stays.
	//
	// The manager's resolution() is the preset last asked for, and going
	// fullscreen asks for the monitor. On a 2560x1440 screen that is exactly a
	// preset, so the label really does become 2560x1440 - seeding from it made
	// this row believe the player had chosen 1440p, and turning fullscreen off
	// then applied it as the windowed size and saved it. A player who had
	// picked 1080p got a 1440p window and a save file that agreed.
	//
	// The save is the windowed choice, which is the question this row is
	// asking, and it is the same value Application keeps to return to.
	this->resolution_selection_ = this->save()->resolution();
	this->full_screen_selection_ = this->save()->fullscreen();
	this->team_palette_selection_ = this->save()->team_palette();

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

	this->resolution_row_ = std::make_unique<OptionRow>(
		"resolution",
		L"Resolution",
		this->calculate_widget_position(0, 2),
		this->calculate_widget_position(2, 2),
		this->menu_style(),
		this->look().options_value_colour,
		this->render_resources(),
		[this](Direction direction) { this->cycle_resolution(direction); });

	this->update_resolution_selection_text();

	this->full_screen_row_ = std::make_unique<OptionRow>(
		"full_screen",
		L"Fullscreen",
		this->calculate_widget_position(0, 3),
		this->calculate_widget_position(2, 3),
		this->menu_style(),
		this->look().options_value_colour,
		this->render_resources(),
		[this](Direction /*direction*/)
		{
			// Two states, so either direction is the same toggle.
			this->full_screen_selection_ = !this->full_screen_selection_;
			this->update_full_screen_selection_text();
		});

	this->update_full_screen_selection_text();

	this->team_palette_row_ = std::make_unique<OptionRow>(
		"team_palette",
		L"Team palette",
		this->calculate_widget_position(0, 4),
		this->calculate_widget_position(2, 4),
		this->menu_style(),
		this->look().options_value_colour,
		this->render_resources(),
		[this](Direction direction) { this->cycle_team_palette(direction); });
	this->update_team_palette_selection_text();

	this->apply_ = std::make_unique<UiTextDropShadow>(
		"apply",
		L"Apply",
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

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->resolution_row_.get());
	this->text_container_->add_child(this->full_screen_row_.get());
	this->text_container_->add_child(this->team_palette_row_.get());
	this->text_container_->add_child(this->apply_.get());
	this->text_container_->add_child(this->back_.get());

	// The value rows are focusable and do nothing on A; left and right are
	// what they answer to. A button with no action is a normal thing to have.
	//
	// A container in a container, which is the nesting engine/ui/widget.h says
	// works by composition: text_container_ draws each row, each row draws its
	// label and its value.
	this->focus_.add(this->resolution_row_.get());
	this->focus_.add(this->full_screen_row_.get());
	this->focus_.add(this->team_palette_row_.get());
	this->focus_.add(this->apply_.get(), [this] { this->apply_settings(); });
	this->focus_.add(this->back_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuHome>(this->menu_context()));
		});

	// After registration, because it switches a registered row off. A page
	// opened while the game is already fullscreen has to say so on its first
	// frame rather than on its first Apply.
	this->refresh_resolution_row();

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
	this->resolution_row_->set_value(widen(res_text));
}

// What the Resolution row says, and whether it can be picked at all.
//
// TWO DIFFERENT QUESTIONS, AND THE ENGINE ANSWERS THEM WITH TWO DIFFERENT
// CALLS. resolution() is a label - "the preset last asked for", in
// resolution_manager.h's own words - and resolution_ivec() is the live window.
// While fullscreen those two disagree, because the window is the monitor and
// the monitor is usually not one of the four presets. This row used to show
// the label and claim it was what was in effect: on a 2560x1440 monitor it sat
// there reading 1920x1080 while the game ran at 1440p.
//
// So the live size is what it shows while fullscreen is on, formatted from the
// vector rather than named, because a size that is not a preset has no name in
// the enum and inventing one is the coercion the engine spent a commit
// removing.
//
// AND THE ROW IS DISABLED WHILE IT SAYS THAT, which is the honest reading of a
// value the player cannot change: fullscreen takes the monitor, and nothing
// this row offers can alter that. It is not a fifth thing to remember either -
// resolution_selection_ is left alone underneath, so it is still the preset
// the window goes back to, and turning fullscreen off re-enables the row
// showing it.
void MainMenuOptions::refresh_resolution_row()
{
	const bool fullscreen = this->save()->fullscreen();

	if (fullscreen)
	{
		const Vector2I size = this->resolution_manager()->resolution_ivec();
		this->resolution_row_->set_value(
			widen(std::to_string(size.x) + "x" + std::to_string(size.y)));
	}
	else
	{
		this->update_resolution_selection_text();
	}

	this->focus_.set_enabled(this->resolution_row_.get(), !fullscreen);

	// AND THE CURSOR IS MOVED OFF IT, which the engine deliberately does not do
	// for us: FocusGroup's header says disabling the focused row leaves the
	// cursor where it is, because yanking a player's hand elsewhere mid-press
	// is worse than sitting on a row that answers `refused`.
	//
	// That is the right default and the wrong one here, and the difference is
	// who was holding the cursor. This page disables the row on the frame it
	// opens - Resolution is registered first, so it starts focused - and a page
	// whose first frame shows a greyed row and no highlight anywhere reads as
	// broken rather than as unavailable. Nobody's hand is on it yet.
	if (fullscreen &&
		this->focus_.focused(0) == this->resolution_row_.get())
	{
		this->focus_.set_focused(0, this->full_screen_row_.get());
	}
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
	this->full_screen_row_->set_value(fs_text);
}

void MainMenuOptions::cycle_team_palette(labrador::Direction direction)
{
	const int first = static_cast<int>(TeamColourPair::random);
	const int end = static_cast<int>(TeamColourPair::count);
	int value = static_cast<int>(this->team_palette_selection_) +
		(direction == Direction::left ? -1 : 1);
	if (value < first)
	{
		value = end - 1;
	}
	else if (value >= end)
	{
		value = first;
	}
	this->team_palette_selection_ = static_cast<TeamColourPair>(value);
	this->update_team_palette_selection_text();
}

void MainMenuOptions::update_team_palette_selection_text()
{
	// State which colour belongs to which team before starting a match.
	const wchar_t* label = L"Random";
	switch (this->team_palette_selection_)
	{
	case TeamColourPair::blue_orange: label = L"A: Blue / B: Orange"; break;
	case TeamColourPair::pink_green: label = L"A: Pink / B: Green"; break;
	case TeamColourPair::blue_yellow: label = L"A: Blue / B: Yellow"; break;
	case TeamColourPair::random: break;
	default: throw std::invalid_argument("Invalid team palette");
	}
	this->team_palette_row_->set_value(label);
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

	// Three colours, because three of the six rows on this page are modes that
	// are not built. The third is the group's to apply now, which is the whole
	// of what changed here.
	this->focus_.set_style({ this->menu_style().highlight,
		this->menu_style().unhighlight,
		this->menu_style().disabled });

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

	// The three that are not built. The colour here is the one they open on;
	// focus_ owns it from registration onward - see the note where they are
	// registered.
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
	// Three modes that are not built yet: registered, with no action, and
	// switched off.
	//
	// They used to not be registered at all, which is what the page could do
	// before FocusGroup had a third state. That worked and left the paint
	// hanging on a promise - the colour passed at construction survived only
	// because nothing ever registered them, and the moment anything did, the
	// next add() or move() would repaint them as ordinary rows. The colour is
	// derived from enablement now, so it is the group's answer rather than
	// this page's hope.
	//
	// THE BEEP DOES NOT COME BACK, which is what the report that asked for
	// this expected. move() takes its candidates from the enabled entries, so
	// the cursor still cannot land on Deathmatch, so there is still no press
	// to refuse. `refused` is reachable only where a row switches off under a
	// cursor already on it, which on this page never happens - these three are
	// decided once at init and never change. Player Count is the page that
	// gets that half.
	//
	// It is still the right shape: the rows are now what they always were, a
	// thing that is there and cannot be picked, said once rather than
	// arranged for.
	this->focus_.add(this->tdm_.get());
	this->focus_.add(this->dm_.get());
	this->focus_.add(this->practice_.get());
	this->focus_.set_enabled(this->tdm_.get(), false);
	this->focus_.set_enabled(this->dm_.get(), false);
	this->focus_.set_enabled(this->practice_.get(), false);

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
	if (!this->context()->active())
	{
		return;
	}
	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	for (int slot = 0; slot < static_cast<int>(inputs.size()); ++slot)
	{
		const ProcessedMenuInput& input = inputs[slot];
		if (input.action == MenuInputAction::back)
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuModeSelect>(this->menu_context()));
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			this->activating_slot_ = slot;
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
void MainMenuPlayerCount::init()
{
	this->set_prompts({
		{ L"A", L"Select" },
		{ L"B", L"Back" } });

	// Devices can be connected and claimed on the next page.
	this->focus_.set_style({ this->menu_style().highlight,
		this->menu_style().unhighlight,
		this->menu_style().disabled });

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

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->_1_player.get());
	this->text_container_->add_child(this->_2_players.get());
	this->text_container_->add_child(this->_3_players.get());
	this->text_container_->add_child(this->_4_players.get());
	this->text_container_->add_child(this->back_.get());

	this->focus_.add(this->_1_player.get(),
		[this] { this->start_join(1, ScreenLayout::one_player); });
	this->focus_.add(this->_2_players.get(),
		[this] { this->start_join(2, ScreenLayout::two_player); });
	this->focus_.add(this->_3_players.get(),
		[this] { this->start_join(3, ScreenLayout::three_player); });
	this->focus_.add(this->_4_players.get(),
		[this] { this->start_join(4, ScreenLayout::four_player); });
	this->focus_.add(this->back_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuModeSelect>(this->menu_context()));
		});

	this->play_effect(this->music_, true, this->look().music.volume);
}

void MainMenuPlayerCount::start_join(int players, ScreenLayout layout)
{
	this->play_wave(this->confirm_sound_);
	MenuLevelSettings* settings = this->level_settings();
	settings->set_player_count(players);
	settings->set_screen_layout(layout);
	settings->begin_input_claims(this->activating_slot_);
	if (players == 1)
	{
		settings->set_player_team(0, PlayerTeam::a);
	}
	this->context()->transition_to(
		std::make_unique<MainMenuJoin>(this->menu_context()));
}

#pragma endregion MainMenuPlayerCount

MainMenuJoin::MainMenuJoin(MenuContext* context) : MainMenuPage(context)
{
}

void MainMenuJoin::init()
{
	this->set_prompts({ { L"A", L"Join / P1 continue" },
		{ L"B", L"Leave / P1 back" } });
	this->set_widget_spacing(this->look().player_count_spacing);
	const MenuStyle& style = this->menu_style();
	this->background_ = std::make_unique<UiTexture>("background",
		"sprite_sheet_1", "pixel", RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(), this->look().play_background_colour);
	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->text_container_ = std::make_unique<UiContainer>("text_container");
	this->heading_ = std::make_unique<UiTextDropShadow>("heading", L"Join Players",
		style.heading_font, this->calculate_widget_position(0, 0),
		this->render_resources(), style.heading_colour, style.shadow_colour,
		style.heading_shadow_offset);
	this->text_container_->add_child(this->heading_.get());
	for (int player = 0; player < this->player_count(); ++player)
	{
		this->players_.push_back(std::make_unique<UiTextDropShadow>(
			"player_" + std::to_string(player), L"", style.item_font,
			this->calculate_widget_position(0, player + 2), this->render_resources(),
			style.unhighlight, style.shadow_colour, style.item_shadow_offset));
		this->text_container_->add_child(this->players_.back().get());
	}
	this->status_ = std::make_unique<UiTextDropShadow>("status", L"",
		style.detail_font, this->calculate_widget_position(0, 7),
		this->render_resources(), style.warning, style.shadow_colour,
		style.detail_shadow_offset);
	this->text_container_->add_child(this->status_.get());
	this->refresh_players(this->input()->player_inputs(*this->level_settings()));
	this->play_effect(this->music_, true, this->look().music.volume);
}

void MainMenuJoin::refresh_players(const std::vector<ProcessedMenuInput>& inputs)
{
	bool ready = this->level_settings()->input_roster_complete();
	const std::vector<MenuPlayerSettings> players = this->level_settings()->player_settings();
	for (int player = 0; player < this->player_count(); ++player)
	{
		const bool connected = inputs[player].connected;
		ready = ready && connected;
		std::wstring label = this->level_settings()->player_label(player);
		if (!connected && players[player].num >= 0)
		{
			label += L" - Reconnect";
		}
		this->players_[player]->set_text(label);
		this->players_[player]->set_colour(connected ?
			this->menu_style().highlight : this->menu_style().disabled);
	}
	this->status_->set_text(ready ? L"Everyone joined. Player 1 can continue." :
		L"Press A / Enter on each device to join. Reconnect reserved pads.");
}

void MainMenuJoin::update(float /*dt*/)
{
	if (!this->context()->active())
	{
		return;
	}
	MenuLevelSettings* settings = this->level_settings();
	const std::vector<ProcessedMenuInput> players = this->input()->player_inputs(*settings);
	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	// Evaluate readiness before accepting joins: the last join cannot also
	// dismiss the page in the same frame as Player 1's confirmation.
	const bool ready = settings->input_roster_complete() &&
		std::all_of(players.begin(), players.end(),
			[](const ProcessedMenuInput& input) { return input.connected; });
	bool continue_requested = false;
	bool roster_changed = false;
	for (int slot = 0; slot < static_cast<int>(inputs.size()); ++slot)
	{
		const int player = settings->player_index_for_slot(slot);
		if (inputs[slot].action == MenuInputAction::back)
		{
			if (player == 0 || !players[0].connected)
			{
				this->play_wave(this->cancel_sound_);
				this->context()->transition_to(
					std::make_unique<MainMenuPlayerCount>(this->menu_context()));
				return;
			}
			if (settings->release_input_slot(slot))
			{
				roster_changed = true;
				this->play_wave(this->cancel_sound_);
			}
		}
		else if (inputs[slot].action == MenuInputAction::proceed)
		{
			if (player == 0 && ready)
			{
				continue_requested = true;
			}
			if (settings->claim_input_slot(slot))
			{
				roster_changed = true;
				this->play_wave(this->ready_sound_);
			}
		}
	}
	if (continue_requested && !roster_changed)
	{
		this->play_wave(this->confirm_sound_);
		if (this->player_count() == 1)
		{
			this->context()->transition_to(
				std::make_unique<MainMenuWeaponSelect>(this->menu_context()));
		}
		else
		{
			this->context()->transition_to(
				std::make_unique<MainMenuTeamSelect>(this->menu_context()));
		}
		return;
	}
	this->refresh_players(this->input()->player_inputs(*settings));
}

#pragma region MainMenuTeamSelect

MainMenuTeamSelect::MainMenuTeamSelect(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuTeamSelect::update(float /*dt*/)
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
	for (int i = 0; i < this->player_count(); i++)
	{
		this->level_settings()->
			set_player_team(i, this->select_states_[i].team);
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
	this->set_player_prompts({
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
		std::wstring label_text = this->level_settings()->player_label(i);

		widgets->player = std::make_unique<UiTextDropShadow>(
			name,
			label_text,
			this->menu_style().detail_font,
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
	const std::vector<ProcessedMenuInput> inputs =
		this->input()->player_inputs(*this->level_settings());
	if (this->wait_for_players(inputs))
	{
		return;
	}
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
	this->set_player_prompts({
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
		std::wstring label_text = this->level_settings()->player_label(i);

		widgets->player = std::make_unique<UiTextDropShadow>(
			name,
			label_text,
			this->menu_style().detail_font,
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
	this->set_player_prompts({
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
