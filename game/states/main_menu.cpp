#include "game/pch.h"
#include "game/states/main_menu.h"

using namespace DirectX;
using namespace mattmath;
using namespace main_menu_consts;
using namespace menu_consts;
using namespace colour_consts;
using namespace artattack;

MainMenuPage::MainMenuPage(MenuContext* context) :
	MenuPage(context),
	SoundBankObject(main_menu_consts::SOUND_BANK,
		this->audio_resources())
{
	this->direction_sound_ = this->resolve_wave(DIRECTION_SOUND);
	this->confirm_sound_ = this->resolve_wave(CONFIRM_SOUND);
	this->cancel_sound_ = this->resolve_wave(CANCEL_SOUND);
	this->error_sound_ = this->resolve_wave(ERROR_SOUND);
	this->ready_sound_ = this->resolve_wave(READY_SOUND);
	this->music_ = this->resolve_effect(MUSIC);
}

int MainMenuPage::player_count() const
{ 
	return this->level_settings()->
		player_count();
}

#pragma region MainMenuTitle

MainMenuTitle::MainMenuTitle(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuTitle::draw(Renderer& renderer) const
{
	std::vector<UiLayer> layers;

	layers.push_back({ this->texture_container_.get(), TextureFilter::point });

	layers.push_back({ this->text_container_.get(), TextureFilter::linear });

	this->draw_ui_layers(renderer, layers);
}
void MainMenuTitle::update(float /*dt*/)
{
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
	this->background_ = std::make_unique<UiTexture>(
		"background",
		"sprite_sheet_1",
		"square_white_4",
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		TITLE_BACKGROUND_COLOUR);

	this->title_ = std::make_unique<UiTextDropShadow>(
		"title",
		L"Colour Wars",
		TITLE_FONT,
		Vector2F(200.0f, 300.0f),
		this->render_resources(),
		TITLE_TEXT_COLOUR,
		SHADOW_COLOUR,
		TITLE_SHADOW_OFFSET);

	this->start_ = std::make_unique<UiTextDropShadow>(
		"start",
		L"Start",
		ITEM_FONT,
		Vector2F(250.0f, 700.0f),
		this->render_resources(),
		TITLE_START_TEXT_COLOUR,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->title_.get());
	this->text_container_->add_child(this->start_.get());

	this->play_effect(this->music_, true, MUSIC_VOLUME);
}

#pragma endregion MainMenuTitle

#pragma region MainMenuHome

MainMenuHome::MainMenuHome(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuHome::draw(Renderer& renderer) const
{
	std::vector<UiLayer> layers;

	layers.push_back({ this->texture_container_.get(), TextureFilter::point });

	layers.push_back({ this->text_container_.get(), TextureFilter::linear });

	this->draw_ui_layers(renderer, layers);
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
			if (this->focus_.activate(0))
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
	this->focus_.set_style({ STANDARD_HIGHLIGHT, STANDARD_UNHIGHLIGHT });

	this->background_ = std::make_unique<UiTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		HOME_BACKGROUND_COLOUR);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Main Menu",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->play_ = std::make_unique<UiTextDropShadow>(
		"play",
		L"Play",
		ITEM_FONT,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		STANDARD_HIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->options_ = std::make_unique<UiTextDropShadow>(
		"options",
		L"Options",
		ITEM_FONT,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->exit_ = std::make_unique<UiTextDropShadow>(
		"exit",
		L"Exit",
		ITEM_FONT,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

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

	this->play_effect(this->music_, true, MUSIC_VOLUME);
}

#pragma endregion MainMenuHome

#pragma region MainMenuOptions

MainMenuOptions::MainMenuOptions(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuOptions::draw(Renderer& renderer) const
{
	std::vector<UiLayer> layers;

	layers.push_back({ this->texture_container_.get(), TextureFilter::point });

	layers.push_back({ this->text_container_.get(), TextureFilter::linear });

	this->draw_ui_layers(renderer, layers);
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
			if (this->focus_.activate(0))
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

void MainMenuOptions::adjust_focused_value(artattack::Direction direction)
{
	if (direction != artattack::Direction::left &&
		direction != artattack::Direction::right)
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

	this->resolution_manager()->set_resolution(this->resolution_selection_);
	this->save()->set_resolution_and_save(this->resolution_selection_);

	this->data()->application()->set_resolution(this->resolution_selection_);

	bool fs = this->full_screen_selection_;
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
	this->focus_.set_style({ STANDARD_HIGHLIGHT, STANDARD_UNHIGHLIGHT });
	Vector2F widget_size = this->widget_size();

	this->resolution_selection_ = this->resolution_manager()->resolution();
	this->full_screen_selection_ = this->save()->fullscreen();

	this->background_ = std::make_unique<UiTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		OPTIONS_BACKGROUND_COLOUR);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Options",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->resolution_element_ = std::make_unique<UiTextDropShadow>(
		"resolution_element",
		L"Resolution",
		ITEM_FONT,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		STANDARD_HIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->resolution_value_ = std::make_unique<UiTextDropShadow>(
		"resolution_value",
		L"null",
		ITEM_FONT,
		this->calculate_widget_position(2, 2),
		this->render_resources(),
		OPTIONS_VALUE_COLOUR,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->update_resolution_selection_text();

	this->full_screen_ = std::make_unique<UiTextDropShadow>(
		"full_screen",
		L"Fullscreen",
		ITEM_FONT,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->full_screen_value_ = std::make_unique<UiTextDropShadow>(
		"full_screen_value",
		L"null",
		ITEM_FONT,
		this->calculate_widget_position(2, 3),
		this->render_resources(),
		OPTIONS_VALUE_COLOUR,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->update_full_screen_selection_text();

	this->apply_ = std::make_unique<UiTextDropShadow>(
		"apply",
		L"Apply",
		ITEM_FONT,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->back_ = std::make_unique<UiTextDropShadow>(
		"back",
		L"Back",
		ITEM_FONT,
		this->calculate_widget_position(0, 5),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

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

	this->play_effect(this->music_, true, MUSIC_VOLUME);
}

void MainMenuOptions::cycle_resolution(
	artattack::Direction direction)
{
	int enum_max = static_cast<int>(ScreenResolution::max);
	int enum_pos =
		static_cast<int>(this->resolution_selection_);
	if (direction == artattack::Direction::left)
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
	else if (direction == artattack::Direction::right)
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

void MainMenuModeSelect::draw(Renderer& renderer) const
{
	std::vector<UiLayer> layers;

	layers.push_back({ this->texture_container_.get(), TextureFilter::point });

	layers.push_back({ this->text_container_.get(), TextureFilter::linear });

	this->draw_ui_layers(renderer, layers);
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
			if (this->focus_.activate(0))
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
	this->focus_.set_style({ STANDARD_HIGHLIGHT, STANDARD_UNHIGHLIGHT });

	//this->set_widget_position(MODE_SELECT_WIDGET_POSITION);
	this->set_widget_spacing(MODE_SELECT_WIDGET_SPACING);

	this->background_ = std::make_unique<UiTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		PLAY_BACKGROUND_COLOUR);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Mode Select",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->standard_ = std::make_unique<UiTextDropShadow>(
		"standard",
		L"Standard",
		ITEM_FONT,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		STANDARD_HIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->tdm_ = std::make_unique<UiTextDropShadow>(
		"tdm",
		L"Team Deathmatch",
		ITEM_FONT,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->dm_ = std::make_unique<UiTextDropShadow>(
		"dm",
		L"Deathmatch",
		ITEM_FONT,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->practice_ = std::make_unique<UiTextDropShadow>(
		"practice",
		L"Practice",
		ITEM_FONT,
		this->calculate_widget_position(0, 5),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->back_ = std::make_unique<UiTextDropShadow>(
		"back",
		L"Back",
		ITEM_FONT,
		this->calculate_widget_position(0, 6),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

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
	// Three modes that are not built yet. They answer with an error beep, and
	// they say so here rather than in an if-chain three screens away.
	this->focus_.add(this->tdm_.get(),
		[this] { this->play_wave(this->error_sound_); });
	this->focus_.add(this->dm_.get(),
		[this] { this->play_wave(this->error_sound_); });
	this->focus_.add(this->practice_.get(),
		[this] { this->play_wave(this->error_sound_); });
	this->focus_.add(this->back_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuHome>(this->menu_context()));
		});

	this->play_effect(this->music_, true, MUSIC_VOLUME);
}

#pragma endregion MainMenuModeSelect

#pragma region MainMenuPlayerCount

MainMenuPlayerCount::MainMenuPlayerCount(MenuContext* context) :
	MainMenuPage(context)
{

}
void MainMenuPlayerCount::draw(Renderer& renderer) const
{
	std::vector<UiLayer> layers;

	layers.push_back({ this->texture_container_.get(), TextureFilter::point });

	layers.push_back({ this->text_container_.get(), TextureFilter::linear });

	this->draw_ui_layers(renderer, layers);
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
			if (this->focus_.activate(0))
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
	this->focus_.set_style({ STANDARD_HIGHLIGHT, STANDARD_UNHIGHLIGHT });

	//this->set_widget_position(PLAYER_COUNT_WIDGET_POSITION);
	this->set_widget_spacing(PLAYER_COUNT_WIDGET_SPACING);

	this->background_ = std::make_unique<UiTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		PLAY_BACKGROUND_COLOUR);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Number of Players",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->_1_player = std::make_unique<UiTextDropShadow>(
		"1_player",
		L"1 Player",
		ITEM_FONT,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		STANDARD_HIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_2_players = std::make_unique<UiTextDropShadow>(
		"2_players",
		L"2 Players",
		ITEM_FONT,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_3_players = std::make_unique<UiTextDropShadow>(
		"3_players",
		L"3 Players",
		ITEM_FONT,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_4_players = std::make_unique<UiTextDropShadow>(
		"4_players",
		L"4 Players",
		ITEM_FONT,
		this->calculate_widget_position(0, 5),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->back_ = std::make_unique<UiTextDropShadow>(
		"back",
		L"Back",
		ITEM_FONT,
		this->calculate_widget_position(0, 6),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

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

	// One player skips team select: there is no other team to be on.
	this->focus_.add(this->_1_player.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			MenuLevelSettings* settings =
				this->level_settings();
			settings->set_player_count(1);
			settings->set_player_team(0, PlayerTeam::a);
			settings->set_screen_layout(ScreenLayout::one_player);
			settings->set_player_num(0, 0);
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

	this->play_effect(this->music_, true, MUSIC_VOLUME);
}

void MainMenuPlayerCount::start_team_select(int players, ScreenLayout layout)
{
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

void MainMenuTeamSelect::draw(Renderer& renderer) const
{
	std::vector<UiLayer> layers;

	layers.push_back({ this->texture_container_.get(), TextureFilter::point });

	layers.push_back({ this->text_container_.get(), TextureFilter::linear });

	this->draw_ui_layers(renderer, layers);
}
void MainMenuTeamSelect::update(float /*dt*/)
{
	std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	int num_inputs = static_cast<int>(inputs.size());
	int player_count = this->player_count();

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
			this->play_wave(this->confirm_sound_);
			this->select_states_[i].state = ConfirmationState::confirmed;
		}
		else if (this->select_states_[i].state ==
			ConfirmationState::unconfirmed)
		{
			if (inputs[i].direction == artattack::Direction::left)
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
			else if (inputs[i].direction == artattack::Direction::right)
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
void MainMenuTeamSelect::set_level_settings() const
{
	for (int i = 0; i < this->player_count(); i++)
	{
		this->level_settings()->
			set_player_team(i, this->select_states_[i].team);

		this->level_settings()->
			set_player_num(i, i);
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
				main_menu_consts::TEAM_SELECT_SELECTED_COLOUR);
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
			main_menu_consts::TEAM_SELECT_UNSELECTED_COLOUR);
		this->player_widgets_[i]->player_center->set_colour(
			main_menu_consts::TEAM_SELECT_UNSELECTED_COLOUR);
		this->player_widgets_[i]->player_b->set_colour(
			main_menu_consts::TEAM_SELECT_UNSELECTED_COLOUR);
	}
}
void MainMenuTeamSelect::init()
{
	this->set_widget_spacing(TEAM_SELECT_WIDGET_SPACING);

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
		PLAY_BACKGROUND_COLOUR);

	this->texture_container_->add_child(this->background_.get());

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Team Select",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->text_container_->add_child(this->heading_.get());

	
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
			ITEM_FONT,
			this->calculate_widget_position(0, i + 2),
			this->render_resources(),
			TEAM_SELECT_UNSELECTED_COLOUR,
			SHADOW_COLOUR,
			ITEM_SHADOW_OFFSET);

		widgets->player_a = std::make_unique<UiTexture>(
			name + "_a",
			"sprite_sheet_1",
			"team_select_a",
			RectangleF(this->calculate_widget_position(2, i + 2), TEAM_SELECT_TEAM_WIDGET_SIZE),
			this->render_resources());

		auto move = Vector2F(TEAM_SELECT_TEAM_WIDGET_SIZE.x, 0.0f);

		widgets->player_center = std::make_unique<UiTexture>(
			name + "_center",
			"sprite_sheet_1",
			"team_select_center",
			RectangleF(widgets->player_a->rectangle().position() + move,
				TEAM_SELECT_TEAM_WIDGET_SIZE),
			this->render_resources());

		widgets->player_b = std::make_unique<UiTexture>(
			name + "_b",
			"sprite_sheet_1",
			"team_select_b",
			RectangleF(widgets->player_center->rectangle().position() + move,
				TEAM_SELECT_TEAM_WIDGET_SIZE),
			this->render_resources());

		this->texture_container_->add_child(widgets->player_a.get());
		this->texture_container_->add_child(widgets->player_center.get());
		this->texture_container_->add_child(widgets->player_b.get());

		this->text_container_->add_child(widgets->player.get());

		this->player_widgets_.push_back(std::move(widgets));
	}

	this->play_effect(this->music_, true, MUSIC_VOLUME);
}

#pragma endregion MainMenuTeamSelect

#pragma region MainMenuWeaponSelect

MainMenuWeaponSelect::MainMenuWeaponSelect(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuWeaponSelect::draw(Renderer& renderer) const
{
	std::vector<UiLayer> layers;

	layers.push_back({ this->texture_container_.get(), TextureFilter::point });

	layers.push_back({ this->text_container_.get(), TextureFilter::linear });

	this->draw_ui_layers(renderer, layers);
}
void MainMenuWeaponSelect::update(float /*dt*/)
{
	std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	int player_count = this->player_count();
	int num_inputs = static_cast<int>(inputs.size());

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
			if (inputs[i].direction == artattack::Direction::left)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_weapons(artattack::Direction::left, i);
			}
			else if (inputs[i].direction == artattack::Direction::right)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_weapons(artattack::Direction::right, i);
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
	artattack::Direction direction, int player_index)
{
	int enum_max = static_cast<int>(WeaponType::max_prim_wep);
	int enum_pos =
		static_cast<int>(this->select_states_[player_index].type);
	if (direction == artattack::Direction::left)
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
	else if (direction == artattack::Direction::right)
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
				set_colour(WEAPON_SELECT_SELECTED_COLOUR);
			this->player_widgets_[i]->weapon_name->
				set_colour(WEAPON_SELECT_SELECTED_COLOUR);
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
			WEAPON_SELECT_UNSELECTED_COLOUR);
		this->player_widgets_[i]->weapon_name->set_colour(
			WEAPON_SELECT_UNSELECTED_COLOUR);
	}
}
void MainMenuWeaponSelect::init()
{
	this->set_widget_spacing(WEAPON_SELECT_WIDGET_SPACING);

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
		PLAY_BACKGROUND_COLOUR);

	this->texture_container_->add_child(this->background_.get());

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Weapon Select",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

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
			ITEM_FONT,
			this->calculate_widget_position(0, i + 2),
			this->render_resources(),
			WEAPON_SELECT_UNSELECTED_COLOUR);

		widgets->weapon_icon = std::make_unique<UiTexture>(
			name + "_wep_icon",
			"sprite_sheet_1",
			"sprayer",
			RectangleF(this->calculate_widget_position(2, i + 2), WEAPON_SELECT_WEAPON_WIDGET_SIZE),
			this->render_resources(),
			PLAY_BACKGROUND_COLOUR);

		widgets->weapon_name = std::make_unique<UiTextDropShadow>(
			name + "_wep_name",
			L"Sprayer",
			DETAIL_FONT,
			widgets->weapon_icon->rectangle().position() +
				Vector2F(0.0f, WEAPON_SELECT_WEAPON_WIDGET_SIZE.y),
			this->render_resources(),
			WEAPON_SELECT_UNSELECTED_COLOUR,
			SHADOW_COLOUR,
			DETAIL_SHADOW_OFFSET);

		widgets->weapon_description = std::make_unique<UiTextDropShadow>(
			name + "_wep_desc",
			this->weapon_description(WeaponType::sprayer),
			WEAPON_DESCRIPTION_FONT,
			widgets->weapon_icon->rectangle().position() +
				Vector2F(WEAPON_DESC_X_OFFSET, 0.0f),
			this->render_resources(),
			WEAPON_DESCRIPTION_FONT_COLOUR,
			SHADOW_COLOUR,
			WEAPON_DESCRIPTION_SHADOW_OFFSET);

		this->texture_container_->add_child(widgets->weapon_icon.get());
		this->text_container_->add_child(widgets->weapon_name.get());
		this->text_container_->add_child(widgets->player.get());
		this->text_container_->add_child(widgets->weapon_description.get());

		this->player_widgets_.push_back(std::move(widgets));
	}
	this->play_effect(this->music_, true, MUSIC_VOLUME);
}
std::wstring MainMenuWeaponSelect::weapon_description(WeaponType type)
{
	switch (type)
	{
	case WeaponType::sprayer:
		return SPRAYER_DESC;
	case WeaponType::sniper:
		return SNIPER_DESC;
	case WeaponType::roller:
		return ROLLER_DESC;
	case WeaponType::mister:
		return MISTER_DESC;
	case WeaponType::bucket:
		return BUCKET_DESC;
	case WeaponType::random_primary:
		return RANDOM_DESC;
	default:
		return L"ERROR";
	}
}

#pragma endregion MainMenuWeaponSelect

#pragma region MainMenuStageSelect

MainMenuStageSelect::MainMenuStageSelect(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuStageSelect::draw(Renderer& renderer) const
{
	std::vector<UiLayer> layers;

	layers.push_back({ this->texture_container_.get(), TextureFilter::point });

	layers.push_back({ this->text_container_.get(), TextureFilter::linear });

	this->draw_ui_layers(renderer, layers);
}
void MainMenuStageSelect::update(float /*dt*/)
{
	std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	int num_inputs = static_cast<int>(inputs.size());
	int player_count = this->player_count();

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
			//no break here as I want simultaneous input from players to be possible
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
				this->context()->pop();
			}
			else
			{
				this->play_wave(this->ready_sound_);
				this->select_state_.state = ConfirmationState::confirmed;
			}
		}
		else if (this->select_state_.state == ConfirmationState::unconfirmed)
		{
			if (inputs[i].direction == artattack::Direction::left)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_stages(artattack::Direction::left);
			}
			else if (inputs[i].direction == artattack::Direction::right)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_stages(artattack::Direction::right);
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
	const StageList* stages = this->data()->stages();
	const int slot = this->select_state_.slot;
	if (slot == this->random_slot())
	{
		this->stage_icon_->set_sprite_frame("random");
		this->stage_name_->set_text(L"Random");
	}
	else if (slot >= 0 && slot < static_cast<int>(stages->size()))
	{
		const Stage& stage = stages->at(static_cast<size_t>(slot));
		this->stage_icon_->set_sprite_frame(stage.icon_frame);
		this->stage_name_->set_text(widen(stage.display_name));
	}
	else
	{
		this->stage_icon_->set_sprite_frame("error");
		this->stage_name_->set_text(L"ERROR");
	}
	if (this->select_state_.state == ConfirmationState::confirmed)
	{
		this->ready_->set_hidden(false);
	}
}
void MainMenuStageSelect::unconfirm_all_widgets()
{
	this->stage_icon_->set_colour(
		main_menu_consts::STAGE_SELECT_UNSELECTED_COLOUR);
	this->stage_name_->set_colour(
		main_menu_consts::STAGE_SELECT_UNSELECTED_COLOUR);
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
int MainMenuStageSelect::pick_random_stage() const
{
	return rand() % this->random_slot();
}
void MainMenuStageSelect::init()
{
	this->select_state_.slot = this->slot_of(this->level_settings()->stage());

	this->set_widget_spacing(STAGE_SELECT_WIDGET_SPACING);

	this->background_ = std::make_unique<UiTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		PLAY_BACKGROUND_COLOUR);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Stage Select",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);


	this->stage_icon_ = std::make_unique<UiTexture>(
		"stage_icon",
		"sprite_sheet_1",
		"stage_test_1",
		RectangleF(this->calculate_widget_position(0, 1), STAGE_SELECT_ICON_SIZE),
		this->render_resources(),
		PLAY_BACKGROUND_COLOUR);

	this->stage_name_ = std::make_unique<UiTextDropShadow>(
		"stage_name",
		L"Test 1",
		ITEM_FONT,
		this->calculate_widget_position(0, 5),
		this->render_resources(),
		STAGE_SELECT_UNSELECTED_COLOUR,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->ready_ = std::make_unique<UiTextDropShadow>(
		"ready",
		L"READY?",
		ANNOUNCEMENT_FONT,
		this->background_->rectangle().center() - Vector2F(400.0f, 100.0f),
		this->render_resources(),
		STAGE_SELECT_SELECTED_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET,
		true);

	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->stage_icon_.get());

	this->text_container_ = std::make_unique<UiContainer>("text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->stage_name_.get());
	this->text_container_->add_child(this->ready_.get());

	this->play_effect(this->music_, true, MUSIC_VOLUME);
}
void MainMenuStageSelect::cycle_stages(artattack::Direction direction)
{
	// Slots run 0..n-1 for the stages and n for Random, and the cursor wraps.
	const int last = this->random_slot();
	int slot = this->select_state_.slot;
	if (direction == artattack::Direction::left)
	{
		slot = (slot == 0) ? last : slot - 1;
	}
	else if (direction == artattack::Direction::right)
	{
		slot = (slot == last) ? 0 : slot + 1;
	}
	this->select_state_.slot = slot;
}

#pragma endregion MainMenuStageSelect