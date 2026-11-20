// Hold the same aim and transform across paired draws. Differences from the
// folded weapon expose the visible canopy alone, including at diagonal aim.
void check_umbrella_art(Application& app, const Canvas& canvas, const GameTuning& tuning,
    const Presentation& presentation, PlaySettings& preferences, const std::filesystem::path& output)
{
    const auto& look = tuning.weapon(WeaponType::umbrella);
    const TeamColour colours = TeamColourTools::team_colours(TeamColourPair::blue_orange, presentation.team_palette());
    for (const int count : { 2, 4 })
    {
        app.viewport_manager()->set_layout(count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player);
        const float scale = gameplay_scale(presentation.camera(),
            static_cast<float>(app.viewport_manager()->player_viewport(0).pixel_rect().width), 720.0f);
        std::array<Camera, 4> cameras;
        std::array<PlayerInputData, 4> inputs;
        std::vector<std::unique_ptr<WeaponUmbrella>> weapons;
        for (int pane = 0; pane < count; ++pane)
        {
            const PlayerTeam team = pane % 2 == 0 ? PlayerTeam::a : PlayerTeam::b;
            const auto index = static_cast<std::size_t>(pane);
            cameras[index] = centred(Vector2F::zero, app.viewport_manager()->player_viewport(pane), scale);
            auto& weapon = weapons.emplace_back(std::make_unique<WeaponUmbrella>(&tuning, team, pane, pane,
                colours.team_colour(team), WeaponType::umbrella, Vector2F::zero, app.render_resources(),
                app.audio_resources(), &preferences));
            auto& input = inputs[index];
            input.connected = true;
            input.shoot_direction_requested = true;
            input.shoot_angle = team == PlayerTeam::a ? -pi / 6 : pi * 5 / 6;
            input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
            weapon->prepare_canopy(input, Vector2F::zero, true, dt);
        }
        const auto draw = [&]()
        {
            return matrix(app, canvas, cameras, [&](int view, DrawList& list)
            { weapons[static_cast<std::size_t>(view)]->draw(list); }, count);
        };
        const Frame folded = draw();
        for (int pane = 0; pane < count; ++pane)
        {
            const auto i = static_cast<std::size_t>(pane);
            inputs[i].primary_shoot = true;
            weapons[i]->prepare_canopy(inputs[i], Vector2F::zero, true, dt);
            const auto canopy = weapons[i]->canopy();
            require(canopy && weapons[i]->draw_bounds().contains(canopy->bounds), "Canopy escaped weapon culling");
        }
        const Frame intact = draw();
        for (auto& weapon : weapons) { weapon->take_canopy_hit(); }
        const Frame cracked = draw();
        preferences.reduce_effects = true;
        require(draw().pixels == cracked.pixels, "Reduce effects hid or changed the cracked canopy");
        preferences.reduce_effects = false;
        for (auto& weapon : weapons) { weapon->set_paint_override(Colour::gray); }
        const Frame grey = draw();
        for (auto& weapon : weapons)
        {
            weapon->set_paint_override(std::nullopt);
            for (int hit = 1; hit < look.canopy.hits; ++hit) { weapon->take_canopy_hit(); }
            require(!weapon->canopy() && weapon->canopy_state().recovery_remaining > 0, "Expected broken canopy");
        }
        const Frame broken = draw();
        for (int pane = 0; pane < count; ++pane)
        {
            const auto index = static_cast<std::size_t>(pane);
            const Camera& camera = cameras[index];
            const Viewport& vp = app.viewport_manager()->player_viewport(pane);
            const RectangleF shield = weapons[index]->canopy_bounds();
            const auto full = check_bounds(intact, folded, shield, camera, vp);
            check_bounds(cracked, intact, shield, camera, vp);
            const auto remnant = check_bounds(broken, folded, shield, camera, vp);
            require(remnant < full / 2, "Broken canopy still presented a full-height shield");
            // The top quarter lies above this diagonal weapon body and its
            // paint mask. Only the canopy can recolour those pixels.
            const RectangleF tip = camera.calculate_view_rectangle(
                RectangleF(shield.x, shield.y, shield.width, shield.height / 4.0f));
            const RectangleI roi(vp.pixel_rect().x + static_cast<int>(std::ceil(tip.x)),
                vp.pixel_rect().y + static_cast<int>(std::ceil(tip.y)), static_cast<int>(tip.width) - 1,
                static_cast<int>(tip.height) - 1);
            require(crop(grey, roi).pixels != crop(cracked, roi).pixels, "Canopy ignored the opening paint override");
        }
        std::vector<Shots> fans(static_cast<std::size_t>(count));
        for (int pane = 0; pane < count; ++pane)
        {
            const auto i = static_cast<std::size_t>(pane);
            for (int step = 0; step < 180 && fans[i].empty(); ++step)
            {
                fans[i] = weapons[i]->update_and_get_projectiles(inputs[i], Vector2F::zero, Vector2F::zero, true, dt);
            }
            require(fans[i].size() == static_cast<std::size_t>(look.pellets), "Umbrella did not produce a full pellet fan");
            for (auto& pellet : fans[i]) { pellet->update(0.075f); }
            cameras[i] = centred(shot_bounds(fans[i]).center(), app.viewport_manager()->player_viewport(pane), scale);
        }
        const Frame blank = matrix(app, canvas, cameras, [](int, DrawList&) {}, count);
        for (int pellet = 0; pellet < look.pellets; ++pellet)
        {
            const auto index = static_cast<std::size_t>(pellet);
            const Frame shot = matrix(app, canvas, cameras, [&](int view, DrawList& list)
            { fans[static_cast<std::size_t>(view)][index]->draw(list); }, count);
            for (int pane = 0; pane < count; ++pane)
            {
                const auto i = static_cast<std::size_t>(pane);
                check_bounds(shot, blank, fans[i][index]->cull_bounds(1.0f / scale),
                    cameras[i], app.viewport_manager()->player_viewport(pane));
            }
        }
        if (count == 4)
        {
            // Use one pane from each frozen state at its playing size.
            const std::array states{ &folded, &intact, &cracked, &broken };
            const std::array labels{ "folded", "intact", "cracked", "broken" };
            for (int pane = 0; pane < count; ++pane)
            {
                const RectangleI area = app.viewport_manager()->player_viewport(pane).pixel_rect();
                const RectangleI centre(area.x + area.width / 2 - 70, area.y + area.height / 2 - 55, 140, 110);
                const auto index = static_cast<std::size_t>(pane);
                save(crop(*states[index], centre), output / (std::string("umbrella-") + labels[index] + "-crop.ppm"));
            }
            // Keep the shield out of these comparisons; the grip's own body
            // and paint layers must show all four poses through the normal
            // firing cadence. The two springs settle into the held lift.
            WeaponUmbrella& grip = *weapons.front();
            grip.reset_canopy();
            grip.set_motion_presentation(&presentation.tool_motion());
            cameras[0] = centred(Vector2F::zero, app.viewport_manager()->player_viewport(0), scale);
            std::array<std::optional<Frame>, 4> poses;
            for (int tick = 0; tick < 120; ++tick)
            {
                const auto pose = static_cast<std::size_t>(grip.motion_state().pose);
                if (!poses[pose])
                {
                    poses[pose] = matrix(app, canvas, cameras, [&](int, DrawList& list)
                    { grip.Weapon::draw(list); }, 1);
                }
                grip.update_and_get_projectiles(inputs[0], Vector2F::zero, Vector2F::zero, true, dt);
            }
            for (std::size_t i = 0; i < poses.size(); ++i)
            {
                require(poses[i].has_value(), "An equipped umbrella pose was never selected");
                for (std::size_t j = 0; j < i; ++j)
                { require(poses[i]->pixels != poses[j]->pixels, "Two equipped umbrella poses drew identical pixels"); }
            }
        }
    }
    std::cout << "PASS umbrella native 2P/4P: mirrored upright canopies, intact/cracked/broken silhouettes, "
        "reduced effects, grey opening override, cull bounds, all four equipped poses and every fired pellet visible\n";
}
