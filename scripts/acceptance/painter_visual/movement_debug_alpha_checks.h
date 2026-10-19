// Both presets. Included after painter_visual's ordinary fixture helpers.
// Direct consumer alpha proof: no developer helpers, commands, saves, input
// injection, Player/Level/StateContext updates or gameplay qualification.
enum class MovementDebugAlphaMode { correct, straight, doubled_fade };

Colour movement_debug_alpha_tint(const Colour& base, float fade, MovementDebugAlphaMode mode)
{
    if (mode == MovementDebugAlphaMode::straight)
    { return Colour(base.r * fade, base.g * fade, base.b * fade, base.a * fade); }
    const float rgb_fade = mode == MovementDebugAlphaMode::doubled_fade ? fade * fade : fade;
    return Colour(base.r * base.a * rgb_fade, base.g * base.a * rgb_fade,
        base.b * base.a * rgb_fade, base.a * fade);
}

class MovementAlphaFixture
{
public:
    MovementAlphaFixture(Application& app, const MovementFeedbackPresentation& look) :
        look_(look), geometry_(look), sheet_(app.render_resources()->sprite_sheet(look.sprite_sheet)),
        frame_(this->sheet_->sprite_frame(this->sheet_->resolve_sprite_frame(look.fill_frame)))
    {
        require(look.enabled && this->frame_.origin() == Vector2F::ZERO,
            "Movement alpha fixture requires enabled cues and the reviewed zero-origin fill");
        for (size_t index = 0; index < this->cues_.size(); ++index)
        {
            auto cue = std::make_unique<MovementFeedback>(&look, app.render_resources());
            const size_t role = index % 10;
            std::optional<Colour> paint;
            if (role == 0) { cue->takeoff(cell(index)); }
            else if (role == 1) { cue->land(cell(index), look.full_impact_speed); }
            else
            {
                if (role >= 4 && role <= 5) { paint = Colour(44, 168, 206); }
                else if (role >= 6 && role <= 7) { paint = Colour(44, 168, 206, 128); }
                else if (role >= 8) { paint = Colour(165, 50, 214, 0); }
                cue->contact(role % 2 == 0 ? MovementFeedbackKind::step : MovementFeedbackKind::skid,
                    cell(index), role % 2 == 0 ? 1.0f : -1.0f, paint);
            }
            if (index >= 10) { cue->update(this->style(index).duration * 0.5f); }
            require(cue->effects().size() == 1, "Movement alpha fixture did not emit exactly one cue");
            const auto& emitted = cue->effects()[0];
            const MovementFeedbackKind expected_kind = role == 0 ? MovementFeedbackKind::takeoff :
                (role == 1 ? MovementFeedbackKind::landing : (role % 2 == 0 ? MovementFeedbackKind::step : MovementFeedbackKind::skid));
            require(emitted.kind == expected_kind && emitted.position == cell(index) && emitted.strength == 1.0f &&
                emitted.direction == (role >= 2 && role % 2 != 0 ? -1.0f : 1.0f) && emitted.paint_colour == paint &&
                emitted.age == (index < 10 ? 0.0f : this->style(index).duration * 0.5f),
                "Movement fixture did not capture its independently supplied kind/age/direction/paint literals");
            this->effects_[index] = cue->effects()[0]; this->cues_[index] = std::move(cue);
            this->bounds_[index] = this->cues_[index]->bounds(RectangleF(cell(index), { 1, 1 }));
        }
        this->signature_ = this->signature();
    }

    void unchanged() const
    {
        require(this->signature() == this->signature_ && this->look_.sprite_sheet == this->geometry_.sprite_sheet &&
            this->look_.fill_frame == this->geometry_.fill_frame, "Movement alpha edit changed numeric geometry or captured assets");
        for (size_t index = 0; index < this->cues_.size(); ++index)
        {
            require(this->cues_[index]->effects().size() == 1, "Movement alpha draw replaced its captured cue");
            const auto& actual = this->cues_[index]->effects()[0]; const auto& before = this->effects_[index];
            require(actual.kind == before.kind && actual.position == before.position && actual.age == before.age &&
                actual.strength == before.strength && actual.direction == before.direction && actual.paint_colour == before.paint_colour &&
                this->cues_[index]->bounds(RectangleF(cell(index), { 1, 1 })) == this->bounds_[index],
                "Movement alpha drawing changed age, strength, direction, optional captured paint or bounds");
        }
    }

    void draw(DrawList& list) const
    { this->unchanged(); for (const auto& cue : this->cues_) { cue->draw(list); } this->unchanged(); }

    void reference(DrawList& list, const Colour& body, const Colour& highlight, const Colour& keyline,
        MovementDebugAlphaMode mode = MovementDebugAlphaMode::correct) const
    {
        std::array<size_t, 3> puff_pixels{};
        for (size_t index = 0; index < this->cues_.size(); ++index)
        {
            const auto& effect = this->effects_[index];
            const float progress = index < 10 ? 0.0f : 0.5f, fade = 1.0f - progress;
            const auto& burst = this->style(index); const float pixel = this->geometry_.pixel_size;
            const auto snap = [pixel](float value) { return std::round(value / pixel) * pixel; };
            const auto raw = [&](const RectangleF& rectangle, const Colour& colour)
            {
                list.draw_sprite(this->sheet_->texture(), this->frame_.source_rectangle(), rectangle,
                    movement_debug_alpha_tint(colour, fade, mode), 0.0f, Vector2F::ZERO, SpriteFlip::none, 0.0f);
            };
            if (index % 10 >= 2)
            {
                // Independent literal contact recipe, including deliberate
                // main/corner overdraw and border-before-body-before-glint order.
                constexpr std::array<std::array<float, 4>, 3> marks{{
                    { 0.5f, 0.35f, 1.0f, 0.55f }, { 1.2f, 0.7f, 0.8f, 1.0f }, { 1.8f, 1.0f, 0.6f, 0.7f }
                }};
                const size_t count = index % 2 == 0 ? 2 : 3;
                for (size_t mark = 0; mark < count; ++mark)
                {
                    const auto& part = marks[mark];
                    const float size = std::max(pixel, snap(burst.particle_size * part[2] * (1.0f - progress * 0.35f)));
                    const float x = snap(cell(index).x - effect.direction * (burst.particle_size * part[0] + burst.spread * part[1] * progress));
                    const float y = snap(cell(index).y - pixel - size * 0.25f - burst.lift * part[3] * 4.0f * progress * (1.0f - progress));
                    const RectangleF main(x - size * 0.5f, y - size * 0.75f, size, size * 0.75f);
                    const RectangleF corner(x, y - size * 0.45f, size * 0.65f, size * 0.45f);
                    const float edge = this->geometry_.keyline_width;
                    raw({ main.x - edge, main.y - edge, main.width + edge * 2, main.height + edge * 2 }, keyline);
                    raw({ corner.x - edge, corner.y - edge, corner.width + edge * 2, corner.height + edge * 2 }, keyline);
                    raw(main, effect.paint_colour.value_or(body)); raw(corner, effect.paint_colour.value_or(body));
                    raw({ main.x, main.y, std::min(pixel, size * 0.5f), std::min(pixel, main.height) }, highlight);
                }
                continue;
            }

            const float spread = burst.spread * progress, lift = burst.lift * progress * (2.0f - progress);
            const float size = std::max(pixel, snap(burst.particle_size * (1.0f - progress * 0.5f)));
            std::array<RectangleF, 8> lobes, outline; std::array<RectangleF, 2> glints;
            for (size_t side = 0; side < 2; ++side)
            {
                const float direction = side == 0 ? -1.0f : 1.0f;
                const float x = snap(cell(index).x + direction * (size + spread)), y = snap(cell(index).y - lift - size);
                const auto lobe = [&](float along, float down, float width, float height)
                { return RectangleF(x + direction * along * size - width * size * 0.5f, y + down * size, width * size, height * size); };
                const bool landing = index % 10 == 1;
                const std::array<RectangleF, 3> side_lobes = landing ?
                    std::array{ lobe(0, 0.35f, 1.25f, 0.65f), lobe(-0.15f, 0.1f, 0.65f, 0.55f), lobe(0.55f, 0.6f, 0.65f, 0.4f) } :
                    std::array{ lobe(0, 0.2f, 0.9f, 0.8f), lobe(-0.15f, -0.15f, 0.55f, 0.65f), lobe(0.35f, 0.55f, 0.65f, 0.45f) };
                for (size_t part = 0; part < 3; ++part) { lobes[side * 4 + part] = side_lobes[part]; }
                const auto& crown = side_lobes[1];
                glints[side] = { crown.x + crown.width * 0.2f, crown.y + pixel * 0.25f,
                    std::min(pixel, crown.width * 0.5f), std::min(pixel, crown.height * 0.5f) };
                const float chip = std::max(pixel, snap(size * 0.2f));
                lobes[side * 4 + 3] = { snap(x + direction * (size + chip) - chip * 0.5f),
                    snap(y + (landing ? size * 0.45f : -pixel)), chip, chip };
            }
            outline = lobes;
            for (auto& rectangle : outline) { rectangle.inflate(this->geometry_.keyline_width); }
            // Quantization is monotone: truncating positive rectangle edges
            // commutes with union/intersection and the production ink priority.
            // Classify integer pixels directly; do not copy its cut-cell or
            // vertically merged span partitioner. Each covered puff pixel gets
            // exactly one tint, including at half life and lobe joins.
            const auto covers = [](const RectangleF& rectangle, int x, int y)
            {
                return x >= static_cast<int>(rectangle.left()) && x < static_cast<int>(rectangle.right()) &&
                    y >= static_cast<int>(rectangle.top()) && y < static_cast<int>(rectangle.bottom());
            };
            const auto ink_at = [&](int x, int y)
            {
                const bool in_body = std::any_of(lobes.begin(), lobes.end(), [&](const auto& r) { return covers(r, x, y); });
                const bool in_glint = std::any_of(glints.begin(), glints.end(), [&](const auto& r) { return covers(r, x, y); });
                const bool in_outline = std::any_of(outline.begin(), outline.end(), [&](const auto& r) { return covers(r, x, y); });
                return in_body ? (in_glint ? 2 : 1) : (in_outline ? 0 : -1);
            };
            const std::array<Colour, 3> inks{ keyline, body, highlight };
            const int left = static_cast<int>(cell(index).x) - 64, top = static_cast<int>(cell(index).y) - 80;
            for (int y = top; y < top + 96; ++y)
            {
                int start = left, ink = ink_at(left, y);
                for (int x = left; x <= left + 128; ++x)
                {
                    const int next = x == left + 128 ? -1 : ink_at(x, y);
                    if (next == ink) { continue; }
                    if (ink >= 0)
                    {
                        raw({ static_cast<float>(start), static_cast<float>(y), static_cast<float>(x - start), 1.0f }, inks[static_cast<size_t>(ink)]);
                        puff_pixels[static_cast<size_t>(ink)] += static_cast<size_t>(x - start);
                    }
                    start = x; ink = next;
                }
            }
        }
        require(std::all_of(puff_pixels.begin(), puff_pixels.end(), [](size_t pixels) { return pixels > 0; }),
            "Independent puff masks did not observe outline, body and highlight pixels");
    }

    static Frame detail(const Frame& actual)
    {
        Frame result{ 1280, 192, std::vector<unsigned char>(1280 * 192 * 4) };
        for (size_t index = 0; index < 20; ++index)
        {
            const Frame crop = foreground_crop(actual, { static_cast<int>(cell(index).x) - 64,
                static_cast<int>(cell(index).y) - 80, 128, 96 }, 0);
            for (int y = 0; y < 96; ++y)
            {
                const size_t source = static_cast<size_t>(y) * 128 * 4;
                const size_t target = (static_cast<size_t>(y + (index < 10 ? 0 : 96)) * 1280 + index % 10 * 128) * 4;
                std::copy_n(crop.pixels.begin() + source, 128 * 4, result.pixels.begin() + target);
            }
        }
        return result;
    }

private:
    static Vector2F cell(size_t index) { return { 80.0f + static_cast<float>(index % 10) * 120.0f, index < 10 ? 160.0f : 340.0f }; }
    const MovementBurstPresentation& style(size_t index) const
    {
        const size_t role = index % 10;
        if (role == 0) { return this->geometry_.takeoff; }
        if (role == 1) { return this->geometry_.landing; }
        return role % 2 == 0 ? this->geometry_.step : this->geometry_.skid;
    }
    std::vector<float> signature() const
    {
        const auto& look = this->look_;
        std::vector<float> result{ look.enabled ? 1.0f : 0.0f, look.pixel_size, look.landing_min_speed,
            look.full_impact_speed, look.minimum_strength, look.keyline_width };
        for (const auto& burst : { look.takeoff, look.landing, look.step, look.skid })
        { result.insert(result.end(), { burst.duration, burst.spread, burst.lift, burst.particle_size }); }
        return result;
    }
    const MovementFeedbackPresentation& look_;
    MovementFeedbackPresentation geometry_;
    const SpriteSheet* sheet_;
    const SpriteFrame& frame_;
    std::array<std::unique_ptr<MovementFeedback>, 20> cues_;
    std::array<MovementFeedback::Effect, 20> effects_;
    std::array<RectangleF, 20> bounds_;
    std::vector<float> signature_;
};

void movement_debug_alpha_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Presentation local = presentation;
    auto& movement = const_cast<MovementFeedbackPresentation&>(local.movement_feedback());
    auto& overlay = const_cast<DebugOverlayPresentation&>(local.debug_overlay());
    const DebugOverlayPresentation text_geometry = overlay;
    require(movement.colour == Colour(212, 215, 218) && movement.highlight == Colour(255, 255, 255) &&
        movement.keyline == Colour(0, 0, 0) && overlay.colour == Colour(211, 211, 211) && overlay.shadow_colour == Colour(0, 0, 0),
        "Movement/DebugText opaque parity fixture differs from reviewed defaults");
    MovementAlphaFixture cues(app, movement);
    PlaySettings preferences; preferences.music_volume = preferences.sfx_volume = 0;
    MenuLevelSettings settings; settings.set_stage("movement_course"); settings.set_player_count(1);
    settings.set_screen_layout(ScreenLayout::one_player); settings.begin_input_claims(input_slots::keyboard_and_mouse);
    settings.set_player_team(0, PlayerTeam::a); settings.set_player_weapon(0, WeaponType::sprayer);
    const PlayerBuilder player_builder(&tuning, &local);
    const auto paint = local.team_palette().fixed_pair(TeamColourPair::blue_orange).colours;
    auto players = player_builder.build_players(settings, levels.get("movement_course"), paint,
        app.render_resources(), app.audio_resources(), &preferences, app.viewport_manager());
    require(players.size() == 1, "DebugText alpha fixture did not build exactly one Player");
    const Player& player = *players[0];
    require(player.state() == PlayerState::alive && player.move_state() == PlayerMoveState::in_air &&
        !player.input_primary_shoot() && !player.input_jump_pressed() && !player.input_jump_held(),
        "DebugText fixture no longer has its fixed unadvanced input/state words");
    const DebugText text(app.render_resources(), app.resolution_manager(), &overlay);
    const FontHandle font = app.render_resources()->resolve_sprite_font(text_geometry.font);
    const auto numbers = [](std::initializer_list<float> values)
    {
        std::wstring result;
        for (const float value : values) { if (!result.empty()) { result += L", "; } result += std::to_wstring(value); }
        return result;
    };
    const RectangleF rect = player.rectangle(); const Vector2F center = player.center(), velocity = player.velocity();
    const Camera camera = player.camera(); const Viewport viewport = player.viewport(); const auto& input = player.input();
    const std::array<std::wstring, 19> literal_lines{{
        L"rect: " + numbers({ rect.x, rect.y, rect.width, rect.height }), L"center: " + numbers({ center.x, center.y }),
        L"vel: " + numbers({ velocity.x, velocity.y }), L"camera: " + numbers({ camera.translation.x, camera.translation.y, camera.scale }),
        L"resolution: 1280.000000, 720.000000", L"viewport: " + numbers({ viewport.x, viewport.y, viewport.width, viewport.height }),
        L"x-input: " + numbers({ player.input_x_movement() }),
        L"shoot-direction: " + numbers({ player.input_shoot_direction().x, player.input_shoot_direction().y }),
        L"shoot-angle: " + numbers({ input.shoot_angle }), L"left-stick: " + numbers({ input.left_analog_stick.x, input.left_analog_stick.y }),
        L"right-stick: " + numbers({ input.right_analog_stick.x, input.right_analog_stick.y }), L"primary-shooting: false",
        L"sim dt: 0.125000", L"render fps: n/a", L"proj_count: 7", L"jump_pressed: 0", L"jump_held: 0", L"move_state: in_air", L"state: alive"
    }};
    const auto player_snapshot = [&]
    {
        const auto rectangle = player.rectangle(); const auto view = player.viewport(); const auto current_camera = player.camera();
        const auto& weapon = player.primary_weapon(); const auto& feedback = weapon.feedback()->state(); const auto& current_input = player.input();
        return std::vector<float>{ rectangle.x, rectangle.y, rectangle.width, rectangle.height,
            player.center().x, player.center().y, player.velocity().x, player.velocity().y,
            current_camera.translation.x, current_camera.translation.y, current_camera.scale, view.x, view.y, view.width, view.height,
            player.health(), player.weapon_ammo(), player.respawn_timer(), static_cast<float>(player.state()), static_cast<float>(player.move_state()),
            static_cast<float>(player.animation_state()), player.action_age(), static_cast<float>(player.action_frame()),
            player.input_x_movement(), player.input_shoot_direction().x, player.input_shoot_direction().y, current_input.shoot_angle,
            current_input.left_analog_stick.x, current_input.left_analog_stick.y, current_input.right_analog_stick.x, current_input.right_analog_stick.y,
            player.input_primary_shoot() ? 1.0f : 0.0f, player.input_jump_pressed() ? 1.0f : 0.0f, player.input_jump_held() ? 1.0f : 0.0f,
            weapon.rotation(), weapon.nozzle_pos().x, weapon.nozzle_pos().y, feedback.muzzle_remaining, feedback.empty_remaining, feedback.recovered_remaining,
            static_cast<float>(player.movement_feedback().effects().size()) };
    };
    const auto player_before = player_snapshot(); const RectangleF player_bounds = player.bounds();
    const auto unchanged = [&]
    {
        cues.unchanged();
        require(player_snapshot() == player_before && player.bounds() == player_bounds && overlay.font == text_geometry.font &&
            overlay.font_scale == text_geometry.font_scale && overlay.line_spacing == text_geometry.line_spacing &&
            overlay.position == text_geometry.position && overlay.shadow_offset == text_geometry.shadow_offset &&
            overlay.shadow_scale == text_geometry.shadow_scale, "Alpha consumer drawing changed Player state/bounds or DebugText geometry/font");
    };
    size_t readbacks = 0, rows = 0, crops = 0, negatives = 0, legacy = 0;
    const auto render = [&](const Colour& background, const std::function<void(DrawList&)>& draw)
    {
        const Canvas canvas(app.render_resources(), background); Renderer& renderer = *app.renderer();
        renderer.begin_frame(); renderer.set_view_count(1); DrawList list = renderer.view(0);
        list.set_viewport(app.viewport_manager()->fullscreen_viewport()); list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point); canvas.draw(list, { 0, 0, 1280, 720 }); draw(list); ++readbacks;
        return read(renderer);
    };
    const auto frozen = [&](const std::function<Frame()>& draw)
    {
        unchanged(); std::srand(8451); const int expected_random = std::rand(); std::srand(8451);
        Frame first = draw(); require(first.pixels == draw().pixels, "Frozen alpha consumer changed pixels between draws");
        require(std::rand() == expected_random, "Alpha consumer drawing consumed match randomness"); unchanged(); return first;
    };
    const auto text_reference = [&](DrawList& list, const Colour& colour, const Colour& shadow, MovementDebugAlphaMode mode)
    {
        list.set_filter(TextureFilter::linear); Vector2F at = text_geometry.position;
        for (const auto& line : literal_lines)
        {
            list.draw_text(font, line, at + text_geometry.shadow_offset, movement_debug_alpha_tint(shadow, 1, mode),
                text_geometry.shadow_scale, 0, Vector2F::ZERO, 0);
            list.draw_text(font, line, at, movement_debug_alpha_tint(colour, 1, mode), text_geometry.font_scale, 0, Vector2F::ZERO, 0);
            at.y += text_geometry.line_spacing;
        }
        list.set_filter(TextureFilter::point);
    };
    struct Palette { const char* name; Colour body, highlight, keyline, text, shadow; };
    const std::array<Palette, 3> palettes{{
        { "opaque", Colour(212, 215, 218), Colour(255, 255, 255), Colour(0, 0, 0), Colour(211, 211, 211), Colour(0, 0, 0) },
        { "rgba128", Colour(32, 96, 192, 128), Colour(192, 96, 32, 128), Colour(53, 149, 209, 128), Colour(81, 199, 130, 128), Colour(145, 84, 210, 128) },
        { "alpha0", Colour(165, 50, 214, 0), Colour(42, 144, 64, 0), Colour(222, 129, 90, 0), Colour(51, 173, 214, 0), Colour(196, 72, 129, 0) }
    }};
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    std::ofstream records(output / "alpha-movement.csv");
    records << "consumer,palette,background,match,frozen_state_bounds_rng,legacy_parity,straight_alpha_rejected,double_fade_rejected,captured_paint,effect_count\n";
    for (size_t palette = 0; palette < palettes.size(); ++palette)
    {
        const auto& colours = palettes[palette]; movement.colour = colours.body; movement.highlight = colours.highlight;
        movement.keyline = colours.keyline; overlay.colour = colours.text; overlay.shadow_colour = colours.shadow;
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            const Frame movement_actual = frozen([&] { return render(backgrounds[background], [&](DrawList& list) { cues.draw(list); }); });
            const auto movement_reference = [&](MovementDebugAlphaMode mode)
            { return render(backgrounds[background], [&](DrawList& list) { cues.reference(list, colours.body, colours.highlight, colours.keyline, mode); }); };
            require(movement_actual.pixels == movement_reference(MovementDebugAlphaMode::correct).pixels,
                "Movement alpha differs from independent literal contact/quantized-union puff channels");
            const Frame debug_actual = frozen([&] { return render(backgrounds[background], [&](DrawList& list) { text.draw_debug_info(list, &player, 7, 0.125f, 0); }); });
            const auto debug_reference = [&](MovementDebugAlphaMode mode)
            { return render(backgrounds[background], [&](DrawList& list) { text_reference(list, colours.text, colours.shadow, mode); }); };
            require(debug_actual.pixels == debug_reference(MovementDebugAlphaMode::correct).pixels,
                "Actual DebugText differs from nineteen independent literal lines and explicit premultiplied channels");
            if (palette == 0)
            {
                const Frame old_movement = movement_reference(MovementDebugAlphaMode::straight);
                // First six columns contain only opaque fallback or opaque
                // captured paint. Partial/zero-alpha captured paint deliberately
                // changes under the fix and is outside this legacy parity box.
                const RectangleI opaque_region(0, 64, 740, 320);
                require(foreground_crop(movement_actual, opaque_region, 0).pixels == foreground_crop(old_movement, opaque_region, 0).pixels,
                    "Movement alpha fix changed opaque-default or opaque-paint legacy pixels"); ++legacy;
                require(debug_actual.pixels == debug_reference(MovementDebugAlphaMode::straight).pixels,
                    "DebugText alpha fix changed opaque-default legacy pixels"); ++legacy;
                require(movement_actual.pixels != movement_reference(MovementDebugAlphaMode::doubled_fade).pixels,
                    "Movement alpha fixture cannot detect premultiplying after event fade"); ++negatives;
            }
            else
            {
                require(movement_actual.pixels != movement_reference(MovementDebugAlphaMode::straight).pixels,
                    "Movement alpha fixture cannot detect straight-alpha RGB leakage"); ++negatives;
                require(debug_actual.pixels != debug_reference(MovementDebugAlphaMode::straight).pixels,
                    "DebugText alpha fixture cannot detect straight-alpha foreground/shadow leakage"); ++negatives;
                const std::string suffix = std::string(colours.name) + (background == 0 ? "-black.ppm" : "-nonblack.ppm");
                save(MovementAlphaFixture::detail(movement_actual), output / ("alpha-movement-" + suffix)); ++crops;
                save(foreground_crop(debug_actual, { 0, 198, 300, 108 }, 0), output / ("alpha-debug-text-" + suffix)); ++crops;
            }
            const char* parity = palette == 0 ? "true" : "not-sampled";
            const char* straight = palette == 0 ? "not-sampled" : "true";
            records << "movement," << colours.name << ',' << (background == 0 ? "black" : "nonblack")
                << ",true,true," << parity << ',' << straight << ',' << (palette == 0 ? "true" : "not-sampled")
                << ",opaque-rgba128-alpha0-retained,20\n"; ++rows;
            records << "debug-text," << colours.name << ',' << (background == 0 ? "black" : "nonblack")
                << ",true,true," << parity << ',' << straight << ",not-applicable,not-applicable,0\n"; ++rows;
            unchanged();
        }
    }
    require(records.good() && readbacks == 50 && rows == 12 && crops == 8 && negatives == 10 && legacy == 4,
        "Movement/DebugText alpha evidence exceeded its bounded counts");
    std::ofstream summary(output / "alpha-movement-summary.json");
    summary << "{\"scope\":\"movement-debug-text-consumer-alpha\",\"state_updates\":0,\"fixture_effect_updates\":10,"
        "\"readbacks\":50,\"csv_rows\":12,\"movement_cases\":6,\"debug_text_cases\":6,\"negative_controls\":10,"
        "\"legacy_parity_controls\":4,\"crops\":8,\"captured_paint_variants\":3,\"crop_pixels\":1112640}\n";
    require(summary.good(), "Cannot retain movement/DebugText alpha summary");
    std::cout << "PASS movement/DebugText alpha: fifty readbacks,twelve CSV rows,eight compact crops; independent contact/raw-puff masks "
        "and nineteen DebugText lines,RGBA128/zero over two backgrounds,opaque legacy parity,straight/double-fade controls,"
        "captured opaque/partial/zero paint and frozen state/bounds/RNG. Both-preset consumer-only scope; no saves,commands,"
        "Player/Level/StateContext updates,global input,physical-device,listening,human or performance acceptance.\n";
}
