#pragma once

namespace
{
    void check_runoff_surfaces(RenderResources& render, AudioResources& audio,
        const GameTuning& tuning, const TeamColour& colours)
    {
        PlaySettings preferences;
        const PaintTuning& look = tuning.paint();
        const auto& runoff = look.runoff;
        const RectangleF wall(0, 0, 100, 160);
        const PaintableFaces left{ true, false, false, false };
        const PaintableFaces right{ false, false, true, false };
        const auto surface = [&](const PaintableFaces& faces,
            std::span<const RectangleF> obstacles = {})
        {
            return std::make_unique<StructurePaintable>("sprite_sheet_1", "square_white_4",
                wall, std::make_unique<RectangleF>(wall), &render, &audio,
                &preferences, &look, CollisionObjectType::structure_paintable,
                colours, faces, obstacles);
        };
        const auto contact = [&](StructurePaintable& object, RectangleF area,
            PlayerTeam team = PlayerTeam::a, int owner = 0)
        {
            const Vector2F half = tuning.projectile(ProjectileType::spray).details.col_rect_size / 2;
            area.offset(half.x, half.y);
            const ProjectileSpray shot(area, Vector2F::ZERO, team, owner,
                colours.team_colour(team), &render, &tuning.projectile(ProjectileType::spray));
            object.on_contact(shot, Vector2F::ZERO, 0);
        };
        const auto prime = [&](StructurePaintable& object, float x, float bottom = 160.0f)
        {
            contact(object, {x, 0, look.thickness, bottom});
            object.update(10);
            require(object.runoff_visuals().empty(), "Priming runoff did not expire");
        };
        const auto totals_equal = [](const PaintTotal& before, const PaintTotal& after)
        {
            require(before.total == after.total && before.team_a == after.team_a &&
                before.team_b == after.team_b && before.player_areas == after.player_areas,
                "Runoff changed coverage, ownership or player attribution");
        };
        const auto contained = [](const PaintRunoffVisual& visual)
        {
            const auto inside = [&](const RectangleF& part)
            {
                require(part.width >= 0 && part.height >= 0,
                    "Clipped runoff has negative dimensions");
                if (part.width > 0 && part.height > 0)
                    require(visual.owned_band.contains(part),
                        "Runoff crossed a scored ownership rectangle or inward keyline");
            };
            inside(visual.trail);
            inside(visual.bead);
        };

        for (const bool is_left : { true, false })
        {
            auto object = surface(is_left ? left : right);
            require(object->runoff_visuals().empty(), "Construction emitted wall runoff");
            const float x = is_left ? 0.0f : wall.right() - look.thickness;
            prime(*object, x);
            const PaintTotal total = object->paint_total();
            contact(*object, {x, 16.1f, look.thickness, 0.2f}, PlayerTeam::a, 1);
            auto visuals = object->runoff_visuals();
            require(visuals.size() == 1 && visuals[0].team == PlayerTeam::a,
                "A projectile contact did not allocate one runoff on its wall strip");
            const float source = visuals[0].source_y;
            require(visuals[0].head_y == source && visuals[0].bead == RectangleF::ZERO &&
                visuals[0].trail == RectangleF::ZERO, "Runoff moved before its authored delay");
            const float black_edge = is_left ? x + look.thickness - look.edge_width : x + look.edge_width;
            require(is_left ? visuals[0].owned_band.right() <= black_edge :
                visuals[0].owned_band.left() >= black_edge, "Runoff covers the inward black edge");
            object->update(0);
            object->update(-1);
            visuals = object->runoff_visuals();
            require(visuals.size() == 1 && visuals[0].age == 0 && visuals[0].head_y == source,
                "Nonpositive update advanced wall runoff");
            object->update(runoff.delay);
            require(object->runoff_visuals()[0].head_y == source, "Runoff ignored its exact delay");
            object->update(runoff.duration * 0.25f);
            const auto quarter = object->runoff_visuals()[0];
            contained(quarter);
            object->update(runoff.duration * 0.25f);
            const auto half = object->runoff_visuals()[0];
            contained(half);
            require(quarter.head_y > source && half.head_y - quarter.head_y > quarter.head_y - source &&
                half.head_y - source <= runoff.distance,
                "Wall runoff does not accelerate downward within its travel bound");

            // Contacts from sustained fire cannot restart the moving slot and
            // hold it forever at its source. Friendly credit is also unchanged.
            for (int tick = 0; tick < 20; ++tick)
            {
                const auto before = object->runoff_visuals();
                contact(*object, {x, 16.1f, look.thickness, 0.2f}, PlayerTeam::a, 2);
                const auto after = object->runoff_visuals();
                require(after.size() == 1 && after[0].age == before[0].age &&
                    after[0].head_y == before[0].head_y,
                    "Repeated contact grows or restarts a live wall runoff");
                object->update(runoff.duration * 0.01f);
            }
            totals_equal(total, object->paint_total());
            preferences.reduce_effects = true;
            const float hidden_age = object->runoff_visuals()[0].age;
            object->update(runoff.duration * 0.05f);
            require(object->runoff_visuals()[0].age > hidden_age,
                "Reduce effects froze runoff instead of suppressing its drawing");
            preferences.reduce_effects = false;
            object->update(1000000);
            require(object->runoff_visuals().empty(), "Long update did not retire runoff");
            totals_equal(total, object->paint_total());
            contact(*object, {x, 16.1f, look.thickness, 0.2f}, PlayerTeam::a, 2);
            require(object->runoff_visuals().size() == 1 && object->runoff_visuals()[0].age == 0,
                "An expired strip cannot accept a later projectile runoff");
            contact(*object, {x, 16.1f, look.thickness, 0.2f}, PlayerTeam::b, 3);
            for (const auto& visual : object->runoff_visuals())
                require(visual.team == PlayerTeam::b, "Enemy source repaint retains old-team runoff");
        }

        // Horizontal paint receives the same contact and wet fill as before,
        // but never owns a downward runoff slot, including at a corner hit.
        auto horizontal = surface({ false, true, false, true });
        contact(*horizontal, wall);
        require(horizontal->runoff_visuals().empty(), "Horizontal faces emitted wall runoff");
        auto corner = surface({ true, true, false, false });
        contact(*corner, {0, 0, look.thickness, look.thickness});
        require(corner->runoff_visuals().size() == 1,
            "A corner hit emitted more than one runoff for its vertical strip");

        // Neutral and enemy paint below the contact both terminate the current
        // same-team run. Adjacent friendly tiles captured by different players
        // are a continuous run; attribution must not become a visual barrier.
        for (const bool enemy : { false, true })
        {
            auto object = surface(left);
            contact(*object, {0, 0, look.thickness, 79.9f});
            if (enemy) contact(*object, {0, 80.1f, look.thickness, 79.8f}, PlayerTeam::b, 3);
            object->update(10);
            contact(*object, {0, 68.1f, look.thickness, 0.2f}, PlayerTeam::a, 1);
            const PaintTotal total = object->paint_total();
            const auto initial = object->runoff_visuals();
            require(initial.size() == 1 && initial[0].owned_band.bottom() <= 80,
                "Runoff ownership run crossed neutral or enemy paint");
            bool stopped = false;
            for (int tick = 0; tick < 120; ++tick)
            {
                object->update((runoff.delay + runoff.duration) / 120);
                const auto visuals = object->runoff_visuals();
                if (visuals.empty()) { stopped = true; break; }
                require(visuals[0].owned_band.bottom() <= 80, "Runoff changed ownership barrier");
                contained(visuals[0]);
            }
            require(stopped, "Runoff never stopped at its ownership boundary");
            totals_equal(total, object->paint_total());
        }
        auto friendly = surface(left);
        contact(*friendly, {0, 0, look.thickness, 79.9f}, PlayerTeam::a, 0);
        contact(*friendly, {0, 80.1f, look.thickness, 79.8f}, PlayerTeam::a, 1);
        friendly->update(10);
        contact(*friendly, {0, 68.1f, look.thickness, 0.2f});
        const auto friendly_visual = friendly->runoff_visuals()[0];
        require(friendly_visual.owned_band.bottom() > 80 &&
            friendly_visual.owned_band.bottom() <= friendly_visual.source_y + runoff.distance,
            "Friendly player credit incorrectly splits contiguous owned wall paint");

        auto interrupted = surface(left);
        prime(*interrupted, 0);
        contact(*interrupted, {0, 16.1f, look.thickness, 0.2f});
        interrupted->update(runoff.delay + runoff.duration * 0.2f);
        contact(*interrupted, {0, 32.1f, look.thickness, 0.2f}, PlayerTeam::b, 3);
        const auto cut = interrupted->runoff_visuals();
        require(cut.empty() || (cut[0].team == PlayerTeam::a && cut[0].owned_band.bottom() <= 32),
            "Enemy repaint ahead did not truncate the existing runoff corridor");
        contact(*interrupted, {0, 32.1f, look.thickness, 0.2f}, PlayerTeam::a, 2);
        const auto restored = interrupted->runoff_visuals();
        if (!cut.empty())
            require(restored.size() == 1 && restored[0].owned_band.bottom() <= cut[0].owned_band.bottom(),
                "Reclaimed paint resuscitated runoff beyond its interrupted corridor");
        for (int tick = 0; tick < 120; ++tick)
        {
            interrupted->update((runoff.delay + runoff.duration) / 120);
            for (const auto& visual : interrupted->runoff_visuals())
                if (visual.team == PlayerTeam::a && visual.source_y < 32)
                    require(visual.owned_band.bottom() <= 32,
                        "An old runoff jumped a previously repainted ownership gap");
        }

        // Occlusion splits one wall into independent strips, preserving real
        // fractional edge tiles. A separate tiny exposed fragment must clip
        // safely even when a bead is taller than the whole fragment.
        const RectangleF obstacles[]{ {-10, 63.5f, 10, 31.25f}, {-10, 96, 10, 62.75f} };
        auto clipped = surface(left, obstacles);
        prime(*clipped, 0);
        contact(*clipped, {0, 48.1f, look.thickness, 0.2f});
        contact(*clipped, {0, 94.9f, look.thickness, 0.2f});
        contact(*clipped, {0, 159, look.thickness, 0.2f});
        require(clipped->runoff_visuals().size() == 3,
            "Disjoint exposed wall strips share a runoff slot");
        const PaintTotal clipped_total = clipped->paint_total();
        for (int tick = 0; tick < 120; ++tick)
        {
            clipped->update((runoff.delay + runoff.duration) / 120);
            for (const auto& visual : clipped->runoff_visuals())
            {
                contained(visual);
                require(visual.owned_band.bottom() <= 63.5f ||
                    (visual.owned_band.top() >= 94.75f && visual.owned_band.bottom() <= 96) ||
                    visual.owned_band.top() >= 158.75f,
                    "Runoff crossed covered wall geometry");
            }
        }
        totals_equal(clipped_total, clipped->paint_total());
        std::cout << "PASS runoff delay, accelerated travel, lifetime, repeated contacts, reduced effects, "
            "all wall bounds, ownership barriers, fractional occlusion and score invariance\n";
    }
}
