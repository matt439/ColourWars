RectangleF stage_paper_surface_reference(DrawList& list, RenderResources& resources,
    const Presentation& captured, const Presentation& fitted, const MenuLevelSettings& settings,
    bool random, bool locked, size_t eligible_count, float accent_offset);

// Shared text is selected by a fixed role. The independent literals below pin
// the shipped wording/spacing while actual page matrices cover their routing.
void stage_random_text_checks(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output);
void stage_briefing_text_checks(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output);
void stage_cue_text_checks(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output);

void menu_text_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    const std::wstring brand = L"C O L O U R W A R S   /   LOCAL PAINT BATTLES";
    constexpr std::array<const wchar_t*, 11> headings{ L"Make your mark.", L"Options", L"Credits",
        L"Controls / audio", L"Pad sticks", L"Number of Players", L"Join Players", L"Mode Select",
        L"Team Select", L"Weapon Select", L"Choose your arena" };
    constexpr std::array<const wchar_t*, 21> prompts{
        L"A/Enter Start", L"A/Enter Select    B/Esc Back",
        L"Left/Right Change    A/Enter Select    B/Esc Back",
        L"Left/Right Change    A/Enter Select    B/Esc Back (discard unapplied changes)",
        L"D-pad/arrows Navigate / change    A/Enter Select    B/Esc Back (discard unapplied changes)",
        L"A/Enter Select    Left/Right Change    B/Esc Back",
        L"A/Enter Join / P1 continue    B/Esc Leave / back    X/R Profile    P1: Up/Down Row, Left/Right Human / Bot",
        L"Left/Right Pick a team    A/Enter Confirm    B/Esc Cancel",
        L"Left/Right Pick a team    A/Enter Confirm    B/Esc Cancel    X/R Random teams",
        L"Left/Right Change weapon    X/R Boost    A/Enter Confirm    B/Esc Cancel",
        L"Left/Right Browse arenas    A/Enter Lock arena    B/Esc Back",
        L"A/Enter Start match    B/Esc Unlock arena", L" Reconnect controller    B/Esc Change players",
        L"A/Enter Select    B/Start/Esc Resume", L" Reconnect, then Start    A/Enter Select",
        L"A/Enter Select", L"A/Enter Back    B/Esc Back",
        L"P1 Up/Down Row    Left/Right Team    A/Enter Ready    B/Esc Cancel    X/R Random teams",
        L"P1 Up/Down Row    Left/Right Tool    X/R Boost    A/Enter Ready    B/Esc Cancel",
        L"D-pad/arrows Navigate / change    A/Enter Select    B/Esc Back",
        L"Up/Down Letter    Left/Right Move    A/Enter Done    B/Esc Cancel"
    };
    static_assert(headings.size() == static_cast<size_t>(MenuHeading::count));
    static_assert(prompts.size() == static_cast<size_t>(MenuPrompt::count));
    class MenuTextProbe final : public MenuPage
    {
    public:
        MenuTextProbe(MenuContext* context, MenuHeading heading) : MenuPage(context), heading_(heading) {}
        void init() override
        {
            this->texture_container_ = std::make_unique<UiContainer>("textures");
            this->text_container_ = std::make_unique<UiContainer>("text");
            const MenuStyle& style = this->menu_style();
            this->heading_text_ = std::make_unique<UiTextDropShadow>("heading",
                this->text().heading(this->heading_), style.heading_font, style.layout.position,
                this->render_resources(), style.heading_colour, style.shadow_colour,
                style.heading_shadow_offset);
            this->text_container_->add_child(this->heading_text_.get());
            this->set_prompts(MenuPrompt::select_back);
        }
        void update_page(float /*dt*/) override {}
        void rebuild(MenuPrompt prompt) { this->set_prompts(prompt); }
    private:
        MenuHeading heading_;
        std::unique_ptr<UiTextDropShadow> heading_text_;
    };

    const Presentation original = presentation;
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels);
    data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct RestoreMenuText
    {
        StateContext& stack;
        Presentation& presentation;
        Presentation original;
        ~RestoreMenuText() { this->stack.clear(); this->presentation = this->original; }
    } restore{ stack, presentation, original };
    const auto tick = [&] { app.keyboard()->poll(); app.mouse()->poll(); stack.update(dt); };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        // Static Stage text/chrome comparisons observe the settled accent.
        app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f);
    };
    const auto frame = [&]
    {
        app.renderer()->begin_frame(); stack.draw(*app.renderer()); return read(*app.renderer());
    };
    const auto prepare = [&]
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true);
        app.mouse()->set_focused(true);
        for (const Key key : { Key::enter, Key::escape, Key::left, Key::right })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        settings = MenuLevelSettings(); settings.set_player_count(1);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_player_team(0, PlayerTeam::a); settings.set_stage("movement_course");
        input.reset_actions(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
    };
    const auto open_probe = [&](MenuHeading heading)
    {
        prepare(); auto probe = std::make_unique<MenuTextProbe>(&context, heading);
        MenuTextProbe* result = probe.get(); stack.push(std::move(probe)); tick(); tick(); return result;
    };
    const auto open_stage = [&]
    {
        prepare(); stack.push(std::make_unique<MainMenuStageSelect>(&context)); tick(); tick();
    };
    struct TextReference
    {
        Frame frame;
        std::array<RectangleF, 3> bounds;
    };
    const auto reference = [&](const Presentation& snapshot, const std::wstring& expected_brand,
        const std::wstring& heading, const std::wstring& prompt, bool stage, bool locked)
    {
        const MenuStyle& style = snapshot.menu(); const auto& footer = snapshot.prompt_bar();
        const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        const UiText brand_text("reference-brand", expected_brand, style.brand_font, style.brand_position,
            app.render_resources(), style.unhighlight);
        const UiTextDropShadow heading_text("reference-heading", heading, style.heading_font,
            style.layout.position, app.render_resources(), stage ? snapshot.main_menu().art.ink : style.heading_colour,
            stage ? Colour(0.0f, 0.0f, 0.0f, 0.0f) : style.shadow_colour,
            style.heading_shadow_offset);
        const UiTextDropShadow prompt_text("reference-prompt", prompt, footer.font, footer.position,
            app.render_resources(), footer.colour, footer.shadow_colour, footer.shadow_offset);
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(viewport);
        list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec()));
        list.set_camera(camera);
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
        const auto fill = sheet->resolve_sprite_frame(style.fill_frame);
        sheet->draw(list, fill, camera.visible_rectangle(viewport), style.scrim_colour);
        if (stage)
        {
            stage_paper_surface_reference(list, *app.render_resources(), snapshot, snapshot, settings,
                false, locked, 1, 0.0f);
        }
        sheet->draw(list, fill, RectangleF(0.0f, style.footer_y, menu_consts::DESIGN_RESOLUTION.x,
            menu_consts::DESIGN_RESOLUTION.y - style.footer_y), style.scrim_colour);
        sheet->draw(list, fill, RectangleF(style.brand_position.x, style.footer_y,
            menu_consts::DESIGN_RESOLUTION.x - style.brand_position.x * 2.0f, style.rule_height),
            style.panel_edge_colour);
        list.set_filter(TextureFilter::linear);
        if (!stage) { brand_text.draw(list); }
        heading_text.draw(list);
        if (stage)
        {
            // The measured heading box reaches into the next row. Its crop
            // includes the beginning of the real stage name, drawn after it.
            const auto& look = snapshot.main_menu().stage_select;
            UiTextDropShadow stage_name("reference-stage-name", L"Movement Course", style.item_font,
                look.name_position, app.render_resources(), locked ? presentation.main_menu().stage_select.selected_ink :
                    presentation.main_menu().stage_select.unselected,
                Colour(0.0f, 0.0f, 0.0f, 0.0f), style.item_shadow_offset);
            const RectangleF bounds = RectangleF::union_of(stage_name.text_bounds_at(Vector2F::ZERO, 1.0f),
                stage_name.text_bounds_at(style.item_shadow_offset, 1.0f));
            const float scale = std::min(1.0f, look.preview_size.x / bounds.width);
            stage_name.set_scale(scale); stage_name.set_shadow_scale(scale);
            stage_name.set_shadow_offset(style.item_shadow_offset * scale); stage_name.draw(list);
        }
        prompt_text.draw(list);
        return TextReference{ read(renderer), { brand_text.bounds(), heading_text.bounds(), prompt_text.bounds() } };
    };
    std::ofstream records(output / "menu-text.csv");
    records << "case,width,height,full_frame,brand_heading_prompt_match\n";
    const auto compare = [&](const std::string& name, const Presentation& snapshot,
        const std::wstring& expected_brand, const std::wstring& heading, const std::wstring& prompt,
        bool stage = false, bool locked = false)
    {
        const Frame actual = frame();
        const TextReference expected = reference(snapshot, expected_brand, heading, prompt, stage, locked);
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        for (size_t index = 0; index < expected.bounds.size(); ++index)
        {
            RectangleF bounds = expected.bounds[index]; bounds.inflate(Vector2F(3.0f));
            camera.calculate_view_rectangle(bounds);
            const RectangleI pixels = countdown_crop_bounds(bounds);
            const Frame actual_crop = foreground_crop(actual, pixels, 0);
            const Frame expected_crop = foreground_crop(expected.frame, pixels, 0);
            if (actual_crop.pixels != expected_crop.pixels)
            {
                const char* component = std::array<const char*, 3>{ "brand", "heading", "prompt" }[index];
                const std::string stem = "menu-text-" + name + '-' + component + '-' +
                    std::to_string(actual.width) + 'x' + std::to_string(actual.height) + "-failure";
                save(actual_crop, output / (stem + "-actual.ppm"));
                save(expected_crop, output / (stem + "-expected.ppm"));
                const RectangleI delta = foreground_delta_bounds(actual_crop, expected_crop);
                std::cerr << "Menu text case=" << name << " component=" << component << " index=" << index <<
                    " stage=" << stage << " locked=" << locked << " crop=(" << pixels.x << ',' << pixels.y << ',' <<
                    pixels.width << ',' << pixels.height << ") local_delta=(" << delta.x << ',' << delta.y << ',' <<
                    delta.width << ',' << delta.height << ")\n";
                throw std::runtime_error("Actual menu brand, heading or formatted prompt differs from the independent text fixture");
            }
            save(foreground_crop(actual, pixels, 0), output / ("menu-text-" + name + '-' +
                std::array<const char*, 3>{ "brand", "heading", "prompt" }[index] + '-' +
                std::to_string(actual.width) + 'x' + std::to_string(actual.height) + "-detail.ppm"));
        }
        require(stage || actual.pixels == expected.frame.pixels,
            "Shared menu text probe differs outside its independent reference text/chrome");
        std::srand(5093); const int next_random = std::rand(); std::srand(5093);
        require(actual.pixels == frame().pixels && std::rand() == next_random,
            "Frozen menu text drawing changed pixels or consumed random numbers");
        records << name << ',' << actual.width << ',' << actual.height << ',' << (!stage ? "true" : "false") << ",true\n";
        return actual;
    };

    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (size_t index = 0; index < headings.size(); ++index)
        {
            open_probe(static_cast<MenuHeading>(index));
            compare("default-heading-" + std::to_string(index), original, brand, headings[index], prompts[1]);
        }
        MenuTextProbe* probe = open_probe(MenuHeading::arena);
        for (size_t index = 0; index < prompts.size(); ++index)
        {
            probe->rebuild(static_cast<MenuPrompt>(index));
            compare("default-prompt-" + std::to_string(index), original, brand, headings[10], prompts[index]);
        }
        open_stage(); const Frame browse = compare("stage-browse", original, brand, headings[10], prompts[10], true);
        tap(Key::enter); compare("stage-locked", original, brand, headings[10], prompts[11], true, true);
        tap(Key::escape); require(frame().pixels == browse.pixels,
            "Stage unlock did not restore the exact authored browse text and page");
    }

    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    const auto place_fixture_options_caption = [&](Presentation& fixture)
    {
        auto& main = const_cast<MainMenuPresentation&>(fixture.main_menu());
        auto& words = const_cast<MenuText&>(fixture.menu_text());
        const auto& style = fixture.menu();
        // These custom headings also replace Options. Keep its unscaled
        // caption beside the longer title without changing the Stage fixture.
        const UiText heading("fixture-options-heading", words.heading(MenuHeading::options),
            style.heading_font, main.options_heading_position, app.render_resources(), main.art.ink);
        main.options_value_x = std::max(main.options_value_x,
            heading.bounds().right() + main.setup_focus_padding.x);
        words.options_palette_timing = L"Next round.";
        const UiText caption("fixture-options-caption", words.options_palette_timing, style.detail_font,
            Vector2F(main.options_value_x, main.options_heading_position.y), app.render_resources(), main.art.ink);
        const RectangleF paper((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) -
            main.options_box_size) / 2.0f, main.options_box_size);
        require(paper.contains(caption.bounds()) && caption.bounds().x >= heading.bounds().right() &&
            caption.bounds().bottom() <= main.options_position.y - main.setup_focus_padding.y,
            "Custom heading fixture lost unscaled Options caption clearance inside its paper and above row zero");
    };
    Presentation old = original;
    auto& old_text = const_cast<MenuText&>(old.menu_text());
    old_text.brand = L"OLD~ BRAND";
    for (auto& heading : old_text.headings) { heading = L"OLD~ HEADING"; }
    for (auto& prompt : old_text.prompts) { prompt = L"OLD~ PROMPT"; }
    old_text.prompts[static_cast<size_t>(MenuPrompt::arena_browse)] = L"OLD~ BROWSE";
    old_text.prompts[static_cast<size_t>(MenuPrompt::arena_locked)] = L"OLD~ LOCKED";
    auto& old_footer = const_cast<PromptBarPresentation&>(old.prompt_bar());
    old_footer.position += Vector2F(36.0f, 8.0f); old_footer.colour = Colour::cyan;
    place_fixture_options_caption(old);
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    presentation = old; open_stage();
    const Frame old_browse = compare("custom-open", old, L"OLD~ BRAND", L"OLD~ HEADING", L"OLD~ BROWSE", true);

    // Borrow the loaded atlas through the public font seam, retaining its
    // original glyphs everywhere except '~'. No existing font or device changes.
    RenderResources& resources = *app.render_resources();
    const FontHandle old_font_handle = resources.resolve_sprite_font(old_footer.font);
    const Font* source = resources.font(old_font_handle);
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character <= U'~'; ++character)
    {
        if (character == U'~') { continue; }
        const Glyph* glyph = source->find(character);
        require(glyph != nullptr, "Menu text font fixture lacks a baseline printable glyph");
        glyphs.push_back(*glyph);
    }
    const std::string narrow_name = "acceptance_menu_text_without_tilde";
    resources.add_font(narrow_name, std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    const FontHandle narrow = resources.resolve_sprite_font(narrow_name);
    require(resources.can_render(old_font_handle, L"OLD~ PROMPT") && !resources.can_render(narrow, L"~"),
        "Menu text font-rebase fixture did not exclude the old text's glyph");
    Presentation next = original;
    auto& next_text = const_cast<MenuText&>(next.menu_text());
    next_text.brand = L"NEW BRAND";
    for (auto& heading : next_text.headings) { heading = L"NEW HEADING"; }
    for (auto& prompt : next_text.prompts) { prompt = L"NEW PROMPT"; }
    next_text.prompts[static_cast<size_t>(MenuPrompt::arena_browse)] = L"NEW BROWSE";
    next_text.prompts[static_cast<size_t>(MenuPrompt::arena_locked)] = L"NEW LOCKED";
    auto& next_footer = const_cast<PromptBarPresentation&>(next.prompt_bar());
    next_footer.font = narrow_name; next_footer.position += Vector2F(60.0f, 12.0f);
    next_footer.colour = Colour::yellow;
    const_cast<MenuStyle&>(next.menu()).brand_font = narrow_name;
    place_fixture_options_caption(next);
    next.validate_resources(resources, *app.audio_resources());
    presentation = next; tick();
    require(frame().pixels == old_browse.pixels, "Presentation assignment rebased an already-open page's text or font");
    for (int repeat = 0; repeat < 2; ++repeat)
    {
        tap(Key::enter); compare("old-page-rebuild-locked-" + std::to_string(repeat), old,
            L"OLD~ BRAND", L"OLD~ HEADING", L"OLD~ LOCKED", true, true);
        tap(Key::escape); compare("old-page-rebuild-browse-" + std::to_string(repeat), old,
            L"OLD~ BRAND", L"OLD~ HEADING", L"OLD~ BROWSE", true);
        require(frame().pixels == old_browse.pixels,
            "Old page prompt rebuild used the replacement font, layout or text snapshot");
    }
    open_stage(); const Frame new_browse = compare("reopened-new-font", next,
        L"NEW BRAND", L"NEW HEADING", L"NEW BROWSE", true);
    require(new_browse.pixels != old_browse.pixels, "Reopened page did not adopt new authored text/font/layout");
    presentation = original; tick();
    require(frame().pixels == new_browse.pixels, "Presentation reversal changed the reopened page snapshot");
    tap(Key::enter); compare("new-page-after-reversal", next,
        L"NEW BRAND", L"NEW HEADING", L"NEW LOCKED", true, true);
    tap(Key::escape); require(frame().pixels == new_browse.pixels,
        "Prompt rebuild after reversal changed the new page's cached font or text");
    open_stage(); compare("restored-default", original, brand, headings[10], prompts[10], true);
    require(settings.stage() == "movement_course" && settings.player_count() == 1 &&
        settings.player_settings()[0].num == input_slots::keyboard_and_mouse &&
        settings.player_weapon(0) == WeaponType::sprayer && settings.player_team(0) == PlayerTeam::a,
        "Text/browse/lock/unlock fixture changed the match plan");
    menu_item_page_checks(app, canvas, tuning, presentation, output);
    menu_setup_text_checks(app, canvas, tuning, presentation, output);
    title_home_text_checks(app, canvas, tuning, presentation, output, true);
    stage_random_text_checks(app, levels, canvas, tuning, presentation, output);
    stage_briefing_text_checks(app, levels, canvas, tuning, presentation, output);
    stage_cue_text_checks(app, levels, canvas, tuning, presentation, output);
    require(records.good(), "Menu text evidence write failed");
    std::cout << "PASS shared menu text: independent brand, 11 headings and 17 formatted legends at 720p/1080p/QHD; "
        "actual arena lock/unlock; custom text/font/layout reopen and old-page snapshots through assignments, "
        "repeated prompt rebuilds and reversal with an old glyph absent from the new font; frozen draw RNG\n";
}
