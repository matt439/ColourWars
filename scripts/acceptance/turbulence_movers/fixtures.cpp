#include "game/pch.h"
#include "scripts/acceptance/turbulence_movers/fixtures.h"
#include "engine/math/triangle.h"
#include "engine/collision/narrow_phase.h"
#include "game/objects/fixed_slab_geometry.h"

namespace turbulence_movers_acceptance
{
    using namespace labrador;
    using namespace mattmath;

    std::string read_text(const fs::path& path)
    {
        std::ifstream input_file(path, std::ios::binary);
        require(input_file.good(), "Cannot read mover trial input: " + path.string());
        std::string text{ std::istreambuf_iterator<char>(input_file), {} };
        require(!input_file.bad(), "Cannot finish reading mover trial input: " + path.string());
        return text;
    }

    MenuLevelSettings settings(int count, PlayerTeam first_team)
    {
        MenuLevelSettings setup;
        require(count == 1 || count == 2 || count == 4, "Unsupported mover trial roster count");
        setup.set_stage("turbulence_mover_trial"); setup.set_player_count(count);
        setup.set_screen_layout(count == 1 ? ScreenLayout::one_player :
            (count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player));
        constexpr std::array<int, 4> slots{ 3, 1, 2, 0 };
        setup.begin_input_claims(count == 1 ? input_slots::keyboard_and_mouse : slots[0]);
        for (int view = 1; view < count; ++view)
        { require(setup.claim_input_slot(slots[static_cast<size_t>(view)]), "Trial sparse roster claim failed"); }
        for (int view = 0; view < count; ++view)
        {
            setup.set_player_team(view, view == 0 ? first_team : (count == 2 ?
                (first_team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a) :
                (view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b)));
            setup.set_player_weapon(view, WeaponType::sprayer);
        }
        return setup;
    }

    RectangleF named_rectangle(const fs::path& path, const std::string& name)
    {
        const JsonDocument source = read_json_file(path_to_utf8(path).c_str());
        const auto objects = source.root().array("collision_objects");
        for (size_t index = 0; index < objects.size(); ++index)
        {
            const auto object = objects.at(index);
            if (object.has("name") && object.string("name") == name)
            {
                const auto rectangle = object.object("rectangle");
                return { rectangle.number("x"), rectangle.number("y"), rectangle.number("width"), rectangle.number("height") };
            }
        }
        throw std::runtime_error("Missing named mover-trial rectangle: " + name);
    }

    fs::path station_variant(Context& context, const std::string& label, float phase, bool side_b, bool on_mover)
    {
        using namespace colourwars_editor;
        LevelDocument document(context.content / "tuning.json");
        document.load(context.stage);
        Entry selected;
        bool found = false;
        for (const Entry& entry : document.entries())
        {
            if (entry.kind != EntryKind::collision) { continue; }
            if (entry.label == "mover_a" || entry.label == "mover_b")
            {
                float own_phase = entry.label == "mover_a" ? phase : phase + 0.5f;
                if (own_phase >= 1.0f) { own_phase -= 1.0f; }
                document.set_property(entry, "motion/phase", std::to_string(own_phase));
                if (entry.label == (side_b ? "mover_b" : "mover_a")) { selected = entry; found = true; }
            }
        }
        require(found, "Station variant did not find its authored mover");
        float centre = mirrored(side_b, 1060.0f), feet = 880.0f;
        if (on_mover)
        {
            const auto guide = document.motion_guide(selected);
            require(guide.has_value(), "Station variant lost the mover's authored guide");
            centre = static_cast<float>(guide->initial_pose.x + guide->initial_pose.width / 2.0);
            feet = static_cast<float>(guide->initial_pose.y);
        }
        const auto body = context.world.tuning.player().size;
        document.set_rectangle({ side_b ? EntryKind::spawn_b : EntryKind::spawn_a, 0, "" },
            { centre - body.x / 2.0f, feet - body.y, body.x, body.y });
        const auto output = context.evidence / (label + ".json");
        require(!fs::exists(output), "Refusing to overwrite an earlier owned mover station: " + output.string());
        document.save(output);
        LevelDocument reopened(context.content / "tuning.json"); reopened.load(output);
        require(reopened.serialize() == document.serialize(), "Mover station changed during editor save/reopen");
        return output;
    }

    PlayerInputData input(float movement, bool pressed, bool held, bool drop, bool fire)
    {
        auto action = moving_structure_acceptance::aimed_input({ 0.0f, 1.0f }, 1.5707963267948966f, fire, movement);
        action.jump_pressed = pressed; action.jump_held = held; action.drop_through = drop;
        return action;
    }

    bool supported(const Player& player)
    {
        const auto state = player.move_state();
        return state == PlayerMoveState::on_ground || state == PlayerMoveState::on_drop_down_ground ||
            state == PlayerMoveState::on_ramp_left || state == PlayerMoveState::on_ramp_right;
    }

    float mirrored(bool side_b, float x) { return side_b ? 4000.0f - x : x; }

    Trial::Trial(Context& with, const fs::path& stage, const MenuLevelSettings& setup, unsigned int seed) : context(with)
    {
        auto info = std::make_unique<LevelLoadedInfo>(path_to_utf8(stage).c_str());
        info->validate();
        const LevelObjectBuilder object_builder(&with.world.render, &with.world.audio, &with.world.preferences, &with.world.tuning.paint());
        info->validate_content(object_builder, with.world.render, with.world.audio);
        bounds_ = info->out_of_bounds_rectangle();
        const auto entries = info->collision_objects_json();
        for (size_t index = 0; index < entries.size(); ++index)
        {
            const auto object = entries.at(index);
            if (object.has("motion") || object.string("collision_type") == "STRUCTURE_JUMP_THROUGH") { continue; }
            std::unique_ptr<Shape> shape;
            if (object.string("type") == "StructureRamp")
            {
                const auto t = object.object("triangle");
                shape = std::make_unique<TriangleRightAxisAligned>(t.number("x1"), t.number("y1"),
                    t.number("x2"), t.number("y2"), t.number("x3"), t.number("y3"));
            }
            else if (object.string("type") == "FixedSlabPaintable")
            {
                const auto slab = object.object("slab");
                const auto rise = slab.string("rise");
                require(rise == "left" || rise == "right", "Trial slab has an unsupported fixed rise");
                const FixedSlabGeometry geometry({ { slab.number("x"), slab.number("y") },
                    rise == "right" ? FixedSlabRise::right : FixedSlabRise::left });
                shape = std::make_unique<RectangleRotated>(geometry.shape());
            }
            else
            {
                const auto r = object.object("rectangle");
                shape = std::make_unique<RectangleF>(r.number("x"), r.number("y"), r.number("width"), r.number("height"));
            }
            solids_.emplace_back(object.string("name"), std::move(shape));
        }
        infos.add(setup.stage(), std::move(info));
        viewports.set_layout(setup.screen_layout());
        LevelBuilder builder(&viewports, &with.world.render, &with.world.audio, &with.world.preferences, &infos,
            &with.world.tuning, &with.world.presentation, &resolution, nullptr, &partitioner, &mouse);
        std::srand(seed);
        level = builder.build_level(setup, TeamColourPair::blue_orange, OpeningMode::off);
        check();
    }

    void Trial::tick(const PlayerInputData& action, SeatInputFilter* next, float seconds)
    {
        tick_inputs({ action }, next, seconds);
    }

    void Trial::tick_inputs(const std::vector<PlayerInputData>& actions, SeatInputFilter* next, float seconds)
    {
        require(actions.size() <= static_cast<size_t>(level->player_count()), "Too many mover-trial seat inputs");
        class Inputs final : public SeatInputFilter
        {
        public:
            Inputs(const std::vector<PlayerInputData>& values, SeatInputFilter* inner) : values_(values), inner_(inner) {}
            void filter_seats(std::vector<PlayerInputData>& seats, const Level& source) override
            {
                for (auto& seat : seats) { seat = input(); }
                for (size_t index = 0; index < values_.size(); ++index) { seats[index] = values_[index]; }
                if (inner_ != nullptr) { inner_->filter_seats(seats, source); }
            }
        private:
            const std::vector<PlayerInputData>& values_;
            SeatInputFilter* inner_;
        } filter(actions, next);
        level->update({}, seconds, &filter);
        if (seconds > 0.0f) { ++updates; }
        check();
    }

    int Trial::count_in()
    {
        int steps = 0;
        while (!level->match_started() && steps < 240) { tick(); ++steps; }
        require(level->match_started(), "Source-tuned mover trial did not finish its bounded count-in");
        return steps;
    }

    void Trial::start()
    {
        require(count_in() == 181, "Mover trial changed the source-tuned 181-update count-in");
        for (int step = 0; step < 3; ++step) { tick(); }
    }

    void Trial::check() const
    {
        for (int view = 0; view < level->player_count(); ++view)
        {
            const Player& actual = level->player(view);
            const auto& body = actual.rectangle();
            require(actual.state() == PlayerState::alive && actual.health() > (allow_damage ? 0.0f : 0.99f),
                "Mover route lost health or respawned at update " + std::to_string(updates));
            require(body.left() >= bounds_.left() - 0.05f && body.right() <= bounds_.right() + 0.05f &&
                body.top() >= bounds_.top() - 0.05f && body.bottom() <= bounds_.bottom() + 0.05f,
                "Mover route left whole-body Scene bounds at update " + std::to_string(updates));
            for (const auto& [name, shape] : solids_)
            {
                const auto hit = narrow_phase(*actual.shape(), *shape);
                require(!hit || hit->penetration <= 0.05f,
                    "Mover route overlaps " + name + " at update " + std::to_string(updates));
            }
        }
    }

    HorizontalStructureState Trial::mover(bool side_b) const
    {
        const auto states = level->horizontal_structure_states();
        require(states.size() == 2, "Trial must retain exactly two admitted movers");
        return states[side_b ? 1u : 0u];
    }

    float steering(const Trial& trial, float target)
    {
        const float error = target - trial.player().center().x;
        const float velocity = trial.player().velocity().x;
        const float stopping = velocity * velocity / (2.0f * trial.context.world.tuning.player().x_deacceleration) +
            std::abs(velocity) * dt + 4.0f;
        if (std::abs(error) < 5.0f || (velocity != 0.0f &&
            (velocity > 0.0f) == (error > 0.0f) && std::abs(error) <= stopping)) { return 0.0f; }
        return error > 0.0f ? 1.0f : -1.0f;
    }
}
