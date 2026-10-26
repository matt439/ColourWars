// Isolated leaf probe for four preference percentages. No broad Controls matrix,
// pad injection, gameplay state, foreground input or hardware qualification.
// Successful run: 132 explicit StateContext updates, 87 readbacks, 52 value CSV
// rows, four status comparisons and eight compact crops (four states x two sizes).
void percentage_text_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    struct PercentageProfile
    {
        std::wstring previous;
        explicit PercentageProfile(const std::filesystem::path& root)
        {
            wchar_t* raw = nullptr;
            size_t length = 0;
            require(_wdupenv_s(&raw, &length, L"LOCALAPPDATA") == 0,
                "Cannot read the percentage probe's runner profile");
            if (raw != nullptr) { this->previous = raw; std::free(raw); }
            std::filesystem::create_directories(root);
            require(_wputenv_s(L"LOCALAPPDATA", root.c_str()) == 0,
                "Cannot isolate the percentage probe's save");
        }
        ~PercentageProfile() { _wputenv_s(L"LOCALAPPDATA", this->previous.c_str()); }
    } profile(output / "percentage-profile");
    const std::filesystem::path save_path = output / "percentage-profile" / "ColourWars" / "save_data.json";
    const Presentation original = presentation;
    const float original_cutoff = tuning.input().movement_deadzone;
    StateContext& stack = app;
    struct RestorePercentageState
    {
        StateContext& stack;
        Presentation& presentation;
        Presentation original;
        GameTuning& tuning;
        float cutoff;
        ~RestorePercentageState()
        {
            this->stack.clear(); this->presentation = this->original;
            const_cast<InputTuning&>(this->tuning.input()).movement_deadzone = this->cutoff;
        }
    };
    Save preferences;
    const auto saved_bytes = [&]
    {
        std::ifstream stream(save_path, std::ios::binary);
        require(stream.good(), "Percentage fixture lost its private save");
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };
    const auto same_play = [](const PlaySettings& first, const PlaySettings& second)
    {
        if (first.aim_sensitivity != second.aim_sensitivity || first.music_volume != second.music_volume ||
            first.sfx_volume != second.sfx_volume || first.view_shake != second.view_shake ||
            first.aim_marker != second.aim_marker || first.reduce_effects != second.reduce_effects) { return false; }
        for (size_t slot = 0; slot < first.sticks.size(); ++slot)
        {
            if (first.sticks[slot].movement_deadzone != second.sticks[slot].movement_deadzone ||
                first.sticks[slot].aim_deadzone != second.sticks[slot].aim_deadzone) { return false; }
        }
        return true;
    };
    const auto seed = [&](int music, int sfx, int movement, int aim)
    {
        PlaySettings fresh;
        fresh.aim_sensitivity = 4; fresh.view_shake = false;
        fresh.music_volume = music; fresh.sfx_volume = sfx;
        for (StickSettings& slot : fresh.sticks)
            { slot.movement_deadzone = movement; slot.aim_deadzone = aim; }
        require(preferences.set_display_settings_and_save(ScreenResolution::s_1920_1080,
            false, TeamColourPair::blue_yellow) && preferences.set_play_settings_and_save(fresh),
            "Cannot seed the percentage fixture's isolated preferences");
        return fresh;
    };
    const auto check_preferences = [&](const PlaySettings& expected)
    {
        require(same_play(preferences.play_settings(), expected) &&
            preferences.resolution() == ScreenResolution::s_1920_1080 && !preferences.fullscreen() &&
            preferences.team_palette() == TeamColourPair::blue_yellow,
            "Percentage formatting changed active or unrelated preferences");
    };
    const auto check_reload = [&](const PlaySettings& expected)
    {
        Save reloaded; reloaded.load_save_file();
        require(same_play(reloaded.play_settings(), expected) &&
            reloaded.resolution() == ScreenResolution::s_1920_1080 && !reloaded.fullscreen() &&
            reloaded.team_palette() == TeamColourPair::blue_yellow,
            "Percentage fixture's persisted preferences differ from the expected private save");
    };
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning);
    data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    // Clear borrowed pages before MenuContext, GameData or Save is destroyed,
    // including failure exits; the caller's Presentation and tuning then return.
    RestorePercentageState restore{ stack, presentation, original, tuning, original_cutoff };
    const_cast<InputTuning&>(tuning.input()).movement_deadzone = 0.999f;
    require(static_cast<int>(std::lround(tuning.input().movement_deadzone * 100.0f)) == 100,
        "Percentage fixture did not exercise an inherited cutoff that rounds to 100");
    size_t updates = 0, readbacks = 0, csv_rows = 0, crops = 0, status_checks = 0;
    const auto tick = [&](float elapsed = 0.0f)
    {
        require(updates < 132, "Percentage leaf probe exceeded its explicit update budget");
        ++updates; app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); tick(0.51f);
    };
    const auto open_parent = [&]
    {
        stack.clear(); stack.notify_activation(true);
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::space, Key::escape })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        input.reset_actions(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<ControlsSettingsMenu>(&context)); tick(); tick();
        require(stack.depth() == 2, "Percentage probe did not open actual Controls/audio");
    };
    int selected_slot = 0;
    const auto open_child = [&]
    {
        selected_slot = 0;
        for (int slot = 0; slot < input_slots::pad_count; ++slot)
        {
            if (app.gamepads()->connected(slot)) { selected_slot = slot; break; }
        }
        tap(Key::enter);
        require(stack.depth() == 3, "Percentage probe did not open actual Pad Sticks through its parent");
    };
    const auto resize = [&](Vector2I size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    const auto actual_frame = [&] { ++readbacks; return main_menu_frame(app); };
    const auto camera = [&]
    {
        return Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
    };
    const auto project = [&](RectangleF bounds)
    {
        camera().calculate_view_rectangle(bounds);
        return countdown_crop_bounds(bounds);
    };
    enum class PercentageVariant { legacy, old_long, new_long };
    const auto literal_value = [](PercentageVariant variant, int value)
    {
        // Deliberately independent from MenuText::setting_percentage and its
        // token substitution. Historical default and two explicit string oracles.
        const std::wstring number = std::to_wstring(value);
        switch (variant)
        {
        case PercentageVariant::legacy: return number + L"%";
        case PercentageVariant::old_long:
            return L"Old~ shared percentage for all players: " + number + L" percent";
        case PercentageVariant::new_long:
            return L"Shared setting percent " + number + L" in this paint session";
        }
        throw std::runtime_error("Unknown percentage reference variant");
    };
    std::ofstream records(output / "percentage-values.csv");
    records << "case,page,width,height,row,value,raw_width,raw_height,scale,font,updates,readbacks,match\n";
    const auto compare = [&](const std::string& name, bool sticks, size_t focus_row,
        const Presentation& snapshot, PercentageVariant variant, int first, int second, bool save_crop)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.controls_settings(); const auto& art = look.art;
        const std::array<const wchar_t*, 10> control_labels{ L"Pad sticks", L"Keyboard Jump", L"Pad Jump", L"Music volume", L"SFX volume",
            L"View shake", L"Aim marker", L"Reduce effects", L"Apply", L"Back" };
        const std::array<const wchar_t*, 5> stick_labels{ L"Pad", L"Move deadzone", L"Aim deadzone", L"Apply", L"Back" };
        const std::array<std::wstring, 2> values{ literal_value(variant, first), literal_value(variant, second) };
        const float width = look.controls_position.x - art.focus_padding.x * 2.0f - art.shadow_offset -
            look.layout.position.x - look.layout.spacing.x;
        const float height = look.layout.spacing.y - art.focus_padding.y * 2.0f;
        require(width > 0.0f && height > 0.0f, "Percentage fixture has no positive value column or row");
        std::vector<std::unique_ptr<UiText>> text;
        std::array<Vector2F, 2> raw_values{};
        std::array<float, 2> scales{};
        const auto add = [&](const std::wstring& value, const Vector2F& position, const Colour& colour)
        {
            auto widget = std::make_unique<UiText>("reference-percentage", value, look.item_font,
                position, app.render_resources(), colour);
            UiText* result = widget.get(); text.push_back(std::move(widget)); return result;
        };
        const size_t row_count = sticks ? stick_labels.size() : control_labels.size();
        const size_t first_value_row = sticks ? 1 : 3;
        RectangleF focused_bounds;
        for (size_t row = 0; row < row_count; ++row)
        {
            const Vector2F position = look.layout.position + Vector2F(0.0f, look.layout.spacing.y * static_cast<float>(row));
            UiText* label = add(sticks ? stick_labels[row] : control_labels[row], position,
                row == focus_row ? art.focus_ink : art.ink);
            const bool adjustable = sticks ? row < 3 : (row >= 3 && row <= 7);
            const bool fitted_action = sticks ? row == 3 : (row == 1 || row == 2 || row == 8);
            if (adjustable || fitted_action)
            {
                const RectangleF raw = label->bounds();
                const float available_width = fitted_action ? look.controls_position.x - art.focus_padding.x * 2.0f -
                    art.shadow_offset - position.x : look.layout.spacing.x - art.focus_padding.x;
                label->set_scale(std::min({ 1.0f, available_width / raw.width, height / raw.height }));
            }
            RectangleF row_bounds = label->bounds();
            if (row == first_value_row || row == first_value_row + 1)
            {
                const size_t value_index = row - first_value_row;
                UiText* value = add(values[value_index], position + Vector2F(look.layout.spacing.x, 0.0f), art.ink);
                const RectangleF raw = value->bounds(); raw_values[value_index] = raw.size();
                float scale = 1.0f;
                if (raw.width > width) { scale = width / raw.width; }
                if (raw.height * scale > height) { scale = height / raw.height; }
                value->set_scale(scale); scales[value_index] = scale;
                require(scale > 0.0f && value->bounds().width <= width + 0.01f &&
                    value->bounds().height <= height + 0.01f &&
                    (variant == PercentageVariant::legacy || scale < 1.0f),
                    "Percentage reference did not preserve or exercise OptionRow value fitting");
                row_bounds = RectangleF::union_of(row_bounds, value->bounds());
            }
            else if (sticks && row == 0)
            {
                UiText* value = add(L"Pad " + std::to_wstring(selected_slot + 1),
                    position + Vector2F(look.layout.spacing.x, 0.0f), art.ink);
                const RectangleF raw = value->bounds();
                value->set_scale(std::min({ 1.0f, width / raw.width, height / raw.height }));
                row_bounds = RectangleF::union_of(row_bounds, value->bounds());
            }
            if (row == focus_row) { focused_bounds = row_bounds; }
        }
        const Frame actual = actual_frame();
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        const Camera view = camera(); list.set_camera(view); list.set_filter(TextureFilter::point);
        const Canvas background(app.render_resources(), snapshot.main_menu().home_background_colour);
        background.draw(list, view.visible_rectangle(app.viewport_manager()->fullscreen_viewport()));
        UiTexture focused("reference-percentage-focus", style.sprite_sheet, style.fill_frame,
            focused_bounds, app.render_resources());
        FocusGroup focus; focus.add(&focused);
        const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.box_size) / 2.0f,
            look.box_size);
        const MenuPaperArtwork artwork(panel, app.render_resources(), &style, &art, &focus);
        artwork.draw(list); list.set_filter(TextureFilter::linear);
        for (const auto& widget : text) { widget->draw(list); }
        ++readbacks; const Frame reference = read(renderer);
        const RectangleF columns(look.layout.position.x + look.layout.spacing.x - 2.0f,
            look.layout.position.y + look.layout.spacing.y * static_cast<float>(first_value_row) - 2.0f,
            width + 4.0f, look.layout.spacing.y + height + 4.0f);
        const RectangleI pixels = project(columns);
        const Frame observed = foreground_crop(actual, pixels, 0);
        const Frame expected = foreground_crop(reference, pixels, 0);
        require(observed.pixels == expected.pixels,
            "Actual preference percentages differ from independent literal values, captured font or fitting");
        if (save_crop)
        {
            require(crops < 8, "Percentage leaf probe exceeded its compact crop budget");
            save(observed, output / ("percentage-" + name + '-' + std::to_string(actual.width) + 'x' +
                std::to_string(actual.height) + ".ppm")); ++crops;
        }
        for (size_t row = 0; row < values.size(); ++row)
        {
            records << name << ',' << (sticks ? "sticks" : "controls") << ',' << actual.width << ',' <<
                actual.height << ',' << row + first_value_row << ',' << (row == 0 ? first : second) << ',' <<
                raw_values[row].x << ',' << raw_values[row].y << ',' << scales[row] << ',' << look.item_font << ',' <<
                updates << ',' << readbacks << ",true\n"; ++csv_rows;
        }
        std::srand(5137); const int expected_random = std::rand(); std::srand(5137);
        require(actual.pixels == actual_frame().pixels && std::rand() == expected_random,
            "Frozen percentage drawing changed pixels or consumed RNG");
        return actual;
    };
    const auto two_sizes = [&](const std::string& name, bool sticks, const Presentation& snapshot,
        PercentageVariant variant, int first, int second)
    {
        const size_t before = updates;
        resize({ 1280, 720 });
        const Frame baseline = compare(name, sticks, 0, snapshot, variant, first, second, true);
        resize({ 1920, 1080 });
        std::ignore = compare(name, sticks, 0, snapshot, variant, first, second, true);
        resize({ 1280, 720 });
        require(compare(name + "-resize-return", sticks, 0, snapshot, variant, first, second, false).pixels == baseline.pixels &&
            updates == before, "Percentage resize/readback changed state cadence, text, font or raster");
        return baseline;
    };
    const auto check_status = [&](const std::wstring& expected, bool warning, bool sticks)
    {
        const auto& look = original.controls_settings(); const auto& style = original.menu();
        const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.box_size) / 2.0f,
            look.box_size);
        const float available = panel.right() - look.art.shadow_offset - look.art.focus_padding.x - look.status_position.x;
        UiText text("reference-percentage-status", expected, look.help_font, look.status_position,
            app.render_resources(), warning ? original.main_menu().options_warning_colour : look.art.focus_ink);
        text.set_scale(std::min(1.0f, available / text.bounds().width));
        // This unchanged right-column copy shares the full-width status crop.
        // Retain its captured text and existing fit instead of cropping it away.
        const auto& help = original.menu_text().help;
        const Vector2F help_position = sticks ? look.stick_preview.association_position : look.controls_position;
        const Vector2F help_available = sticks ? look.controls_position + look.controls_size - help_position : look.controls_size;
        std::wstring controls_help = help.controls;
        if (!sticks)
        {
            require(preferences.input_bindings().keyboard_jump == Key::space,
                "Percentage fixture unexpectedly changed its effective Jump binding");
            const size_t token = controls_help.find(L"{jump}");
            require(token != std::wstring::npos && controls_help.find(L"{jump}", token + 6) == std::wstring::npos,
                "Independent percentage help reference lost its single authored Jump token");
            controls_help.replace(token, 6, L"Space");
        }
        UiText context_text("reference-percentage-status-context", sticks ? help.pad_association : controls_help,
            look.help_font, help_position, app.render_resources(), look.art.ink);
        const RectangleF context_bounds = context_text.bounds();
        context_text.set_scale(std::min({ 1.0f, help_available.x / context_bounds.width,
            help_available.y / context_bounds.height }));
        const Frame actual = actual_frame();
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        const Camera view = camera(); list.set_camera(view); list.set_filter(TextureFilter::point);
        // The status descenders reach the outer paper's bottom shade bands.
        // Retain that existing artwork behind the independent literal text.
        const Canvas background(app.render_resources(), original.main_menu().home_background_colour);
        background.draw(list, view.visible_rectangle(app.viewport_manager()->fullscreen_viewport()));
        FocusGroup empty_focus;
        const MenuPaperArtwork artwork(panel, app.render_resources(), &style, &look.art, &empty_focus);
        artwork.draw(list);
        list.set_filter(TextureFilter::linear); text.draw(list); context_text.draw(list);
        ++readbacks; const Frame reference = read(renderer);
        const RectangleI pixels = project(RectangleF(look.status_position - Vector2F(1.0f),
            Vector2F(available + 2.0f, text.bounds().height + 2.0f)));
        require(foreground_crop(actual, pixels, 0).pixels == foreground_crop(reference, pixels, 0).pixels,
            "Percentage edit/Apply/failure/parent return changed the fixed save status or warning colour");
        ++status_checks;
    };
    const auto same_preview = [&](const Frame& before, const Frame& after)
    {
        const auto& look = original.controls_settings();
        const RectangleI pixels = project(RectangleF(look.controls_position, look.controls_size));
        require(foreground_crop(before, pixels, 0).pixels == foreground_crop(after, pixels, 0).pixels,
            "Replacing percentage text/font changed the current stick response preview");
    };
    const PlaySettings inherited = seed(17, 0, -1, 90);
    const std::string inherited_bytes = saved_bytes();
    resize({ 1280, 720 }); presentation = original; open_parent();
    std::ignore = two_sizes("default-controls", false, original, PercentageVariant::legacy, 17, 0);
    open_child();
    std::ignore = two_sizes("inherited-child", true, original, PercentageVariant::legacy, 100, 90);
    check_preferences(inherited);
    require(saved_bytes() == inherited_bytes, "Opening percentage rows changed the private save");
    stack.clear();
    Presentation old = original, next = original;
    const_cast<MenuText&>(old.menu_text()).values.percentage = L"Old~ shared percentage for all players: {value} percent";
    const_cast<MenuText&>(next.menu_text()).values.percentage = L"Shared setting percent {value} in this paint session";
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    const Font* source = app.render_resources()->font(
        app.render_resources()->resolve_sprite_font(original.controls_settings().item_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* glyph = source->find(character);
        require(glyph != nullptr, "Percentage replacement font lacks a baseline printable glyph");
        Glyph copy = *glyph;
        if (character >= U'0' && character <= U'9') { copy.x_offset += 1.0f; }
        glyphs.push_back(copy);
    }
    const std::string font_name = "acceptance_percentage_values_without_tilde";
    app.render_resources()->add_font(font_name,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    const FontHandle replacement = app.render_resources()->resolve_sprite_font(font_name);
    require(replacement != app.render_resources()->resolve_sprite_font(original.controls_settings().item_font) &&
        !app.render_resources()->can_render(replacement, L"~") &&
        app.render_resources()->font(replacement)->drawn(U'2').x_offset == source->drawn(U'2').x_offset + 1.0f,
        "Percentage font negative control did not distinguish both old glyph retention and new numeric metrics");
    const_cast<ControlsSettingsPresentation&>(next.controls_settings()).item_font = font_name;
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    const PlaySettings seeded = seed(0, 100, 0, 90);
    const std::string seeded_bytes = saved_bytes();
    presentation = old; open_parent();
    const Frame old_parent = two_sizes("old-long-controls", false, old, PercentageVariant::old_long, 0, 100);
    presentation = next; tick();
    require(compare("old-parent-retained", false, 0, old, PercentageVariant::old_long, 0, 100, false).pixels == old_parent.pixels,
        "Replacing percentage copy/font changed an existing Controls page");
    tap(Key::down); tap(Key::down); tap(Key::down); tap(Key::right); tap(Key::down); tap(Key::left);
    tap(Key::up); tap(Key::up); tap(Key::up); tap(Key::up);
    const Frame pending_parent = compare("old-parent-pending", false, 0, old, PercentageVariant::old_long, 10, 90, false);
    check_preferences(seeded);
    require(saved_bytes() == seeded_bytes, "Pending percentage volume edit wrote the private save");
    open_child();
    const Frame new_child = two_sizes("new-long-child", true, next, PercentageVariant::new_long, 0, 90);
    presentation = old; tick();
    const Frame retained_child = compare("new-child-retained", true, 0, next, PercentageVariant::new_long, 0, 90, false);
    require(retained_child.pixels == new_child.pixels, "Percentage reversal changed an open child's captured copy/font");
    same_preview(new_child, retained_child);
    tap(Key::down); tap(Key::right); tap(Key::down); tap(Key::left);
    std::ignore = compare("new-child-pending", true, 2, next, PercentageVariant::new_long, 5, 85, false);
    check_status(L"Apply to save pad deadzones.", false, true);
    tap(Key::down); tap(Key::enter);
    PlaySettings applied = seeded;
    applied.sticks[static_cast<size_t>(selected_slot)].movement_deadzone = 5;
    applied.sticks[static_cast<size_t>(selected_slot)].aim_deadzone = 85;
    check_preferences(applied); check_reload(applied);
    const std::string applied_bytes = saved_bytes();
    require(applied_bytes != seeded_bytes, "Actual percentage Apply did not persist the selected deadzones");
    check_status(L"Pad deadzones applied and saved.", false, true);
    const Frame applied_child = compare("new-child-applied", true, 3, next, PercentageVariant::new_long, 5, 85, false);
    tap(Key::enter);
    require(saved_bytes() == applied_bytes && applied_child.pixels == actual_frame().pixels,
        "Repeated percentage Apply changed private bytes, values or captured font");
    check_preferences(applied);
    tap(Key::up); tap(Key::up); tap(Key::right); tap(Key::down); tap(Key::down);
    const std::filesystem::path blocked = save_path.wstring() + L".tmp";
    require(std::filesystem::create_directory(blocked), "Cannot block the private percentage temporary save");
    struct RemovePercentageBlocker
    {
        std::filesystem::path path;
        ~RemovePercentageBlocker() { std::error_code error; std::filesystem::remove(this->path, error); }
    } blocker{ blocked };
    tap(Key::enter);
    PlaySettings active = applied;
    active.sticks[static_cast<size_t>(selected_slot)].movement_deadzone = 10;
    check_preferences(active); check_reload(applied);
    require(saved_bytes() == applied_bytes, "Failed percentage save replaced previous private bytes");
    check_status(L"Active for this run. Could not save.", true, true);
    std::ignore = compare("new-child-save-failed", true, 3, next, PercentageVariant::new_long, 10, 85, false);
    require(std::filesystem::remove(blocked), "Cannot remove the owned private percentage write refusal");
    tap(Key::up); tap(Key::up); tap(Key::right);
    std::ignore = compare("new-child-discard-pending", true, 1, next, PercentageVariant::new_long, 15, 85, false);
    tap(Key::escape);
    require(stack.depth() == 2 && saved_bytes() == applied_bytes,
        "Percentage child Escape saved pending values or closed its parent");
    check_preferences(active);
    require(compare("old-parent-after-child", false, 0, old, PercentageVariant::old_long, 10, 90, false).pixels ==
        pending_parent.pixels, "Child Apply/failure/discard changed the parent's captured percentage words or pending volume");
    check_status(L"Apply to save changes for all players.", false, false);
    tap(Key::escape);
    require(stack.depth() == 1 && saved_bytes() == applied_bytes,
        "Percentage parent Escape persisted its pending volume");
    check_preferences(active);
    presentation = next; open_parent();
    std::ignore = compare("new-parent-reopened", false, 0, next, PercentageVariant::new_long, 0, 100, false);
    presentation = old; tick(); tap(Key::down); tap(Key::down); tap(Key::down); tap(Key::right);
    std::ignore = compare("new-parent-reversed-pending", false, 3, next, PercentageVariant::new_long, 10, 100, false);
    tap(Key::escape);
    require(stack.depth() == 1 && saved_bytes() == applied_bytes, "New percentage page persisted a discarded value");
    check_preferences(active);
    presentation = old; open_parent();
    std::ignore = compare("old-parent-reopened", false, 0, old, PercentageVariant::old_long, 0, 100, false);
    open_child();
    const Frame reopened_child = compare("old-child-reopened", true, 0, old, PercentageVariant::old_long, 10, 85, false);
    presentation = next; tick();
    const Frame reversed_child = compare("old-child-reversed", true, 0, old, PercentageVariant::old_long, 10, 85, false);
    require(reopened_child.pixels == reversed_child.pixels, "Reversal changed the reopened old percentage child snapshot");
    same_preview(reopened_child, reversed_child);
    tap(Key::down); tap(Key::right);
    std::ignore = compare("old-child-reversed-pending", true, 1, old, PercentageVariant::old_long, 15, 85, false);
    tap(Key::escape); tap(Key::escape);
    require(stack.depth() == 1 && saved_bytes() == applied_bytes,
        "Reopened percentage discard changed persisted preferences or stack ownership");
    check_preferences(active); check_reload(applied);
    require(updates == 132 && readbacks == 87 && csv_rows == 52 && status_checks == 4 && crops == 8 && records.good(),
        "Percentage leaf probe drifted from its frozen update/readback/CSV/crop budgets");
    std::ofstream summary(output / "percentage-summary.json");
    summary << "{\n  \"outcome\": \"passed\",\n  \"updates\": " << updates << ",\n  \"readbacks\": " << readbacks <<
        ",\n  \"value_csv_rows\": " << csv_rows << ",\n  \"status_comparisons\": " << status_checks <<
        ",\n  \"compact_crops\": " << crops << ",\n  \"sizes\": [\"1280x720\", \"1920x1080\"],\n" <<
        "  \"inputs\": \"normal production keyboard reader; actual Controls parent and Sticks child\",\n" <<
        "  \"limits\": \"No global input, foreground/hardware/physical-pad, gameplay, listening or human acceptance\"\n}\n";
    require(summary.good(), "Percentage summary evidence write failed");
    std::cout << "PASS preference percentage leaf: actual four rows, saved17/0/100 and inherited100/explicit0/90; "
        "independent literals, long value fitting, distinct font aliases, retained/reopened/reversed snapshots; "
        "child return, Apply/reload/repeat, active-versus-disk save failure and discard; stable preview/frozen RNG; "
        "updates=" << updates << " readbacks=" << readbacks << " csv_rows=" << csv_rows << " crops=" << crops << '\n';
}
