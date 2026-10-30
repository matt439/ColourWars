# Reading ColourWars as an OOP example

ColourWars demonstrates object-oriented design through a playable game. Its
objects own state, preserve rules, collaborate through interfaces, and specialize
behavior through inheritance. The hierarchies are deliberate. Labrador's
preference for a different style inside the engine does not govern this client.

Start with `Session` and `PaintTile` for small objects, then follow `Player`,
`Weapon` and `Projectile`, and finally the menu state stack. This guide describes
the current code; the documents under `review/` remain historical records.
[The architecture overview](architecture.md) covers the layout, the dependency
direction, the services and the state stack these classes sit in.

## Where the concepts live

| Concept | Read | What to notice |
|---|---|---|
| Encapsulation and invariants | [Session](../game/session.h), [implementation](../game/session.cpp) | The win ledger is private. `finish_round` and `next_round` express allowed operations; repeating `finish_round` cannot award another win. |
| Abstraction | [PaintableObject](../game/objects/paintable_object.h), [Projectile](../game/objects/projectile.h) | A consumer asks for paint totals or projectile behavior without knowing the concrete representation. Pure virtual functions make required behavior explicit. |
| Single and multilevel inheritance | [WeaponBucket](../game/objects/weapon_bucket.h), [Weapon](../game/objects/weapon.h), [ProjectileMist](../game/objects/projectile_mist.h) | Bucket specializes `RelativeVelocityWeapon`, which specializes `Weapon`; mist specializes `DiffusingProjectile`, which specializes `Projectile`. Each intermediate class shares meaningful behavior. |
| Multiple inheritance | [StructurePaintable](../game/objects/structure_paintable.h), [FixedSlabPaintable](../game/objects/fixed_slab_paintable.h), [Player](../game/objects/player.h) | A painted structure has both structure behavior and the scoring interface. `FixedSlabPaintable` takes the same two bases directly rather than deriving from `StructurePaintable`: its sloping face keeps paint in its own rigid frame, so it implements `paint_total` and `append_paint_surfaces` over its own tiles instead of inheriting the axis-aligned ones. A player combines movement, collision, animation and texture roles; its two drawing bases share an engine base, [explained below](#players-two-drawing-bases). |
| Specializing a concrete class | [TeamGateStructurePaintable](../game/objects/team_gate_structure_paintable.h), [TranslatingStructurePaintable](../game/objects/translating_structure_paintable.h) | `StructurePaintable` is concrete and built directly for ordinary painted walls; these two are `final` subclasses. The gate adds a team permission settled from its own held paint and overrides `mask`; the translating structure adds horizontal motion and overrides its geometry, contact and paint-surface functions. Both keep the base's tiles and scoring. |
| Implementation inheritance | [CrudeDrawing](../game/objects/crude_drawing.h), [PlayerBadge](../game/objects/player_badge.h), [Monitor](../game/objects/monitor.h) | A base over the engine's `TextureObject` whose constructor and drawing helpers (`fill`, `blob`, `ring`, `line`, ...) are protected. Nine `final` leaves reuse those helpers: `CountdownArt`, `FigureActing`, `FigureAttachments`, `Monitor`, `PlayerBadge`, `RevealArt`, `ScreenCursor`, `Silhouette` and `TeammateMarker`. They share how to draw, not an interface: owners hold and call each leaf by its own type. |
| Runtime polymorphism | [Player::update_weapon_and_get_projectiles](../game/objects/player.cpp), [WeaponBuilder](../game/objects/weapon_builder.cpp) | Player owns a `unique_ptr<Weapon>`; the factory chooses its concrete type and virtual calls select the behavior. Player does not switch on weapon classes. |
| Double dispatch and role queries | [Collider](../game/objects/collider.h), [Structure](../game/objects/structure.h), [Player::on_structure_contact](../game/objects/player.cpp) | The engine names the other party of a contact only as a `CollisionObject`. `Collider::on_contact` asks that party to `touch` this one, and its `touch` calls the handler for its own class: `on_structure_contact`, `on_player_contact` or `on_projectile_contact`. A structure then answers queries (`jump_through`, `ramp`, `slab_geometry`, `top_paint_team`, `cover_point`, `passable_for`) instead of being cast to its leaf. `Collider::of` is the one conversion from the engine's type, and no class reads another's tag to decide how to respond. |
| Factory over a closed family | [LevelObjectBuilder](../game/objects/level_object_builder.cpp), [TerrainMaterial](../game/objects/terrain_material.h), [implementation](../game/objects/terrain_material.cpp) | The level object builder reads a record's `type`, `gate` and `motion` fields and returns a `StructurePaintable`, `TeamGateStructurePaintable`, `TranslatingStructurePaintable` or `FixedSlabPaintable` as an owning collision object. `TerrainMaterial::make` maps a level's `material` name and collision type to `GroundMaterial`, `PencilMaterial` and their siblings, refusing a mismatch. Each leaf works out its drawing in its constructor ([terrain_material_drawings.cpp](../game/objects/terrain_material_drawings.cpp)); `OrientedTerrainMaterial` shares the long-axis layout between pencil, ruler, eraser and tin. A `Structure` owns its material and never asks which one it has. |
| Overriding and shared algorithms | [WeaponRoller](../game/objects/weapon_roller.cpp), [WeaponBucket](../game/objects/weapon_bucket.cpp), [ProjectileRolling](../game/objects/projectile_rolling.h) | Roller changes aim, tint, its sleeve's motion and pose, and answers `has_aim_marker` with false; bucket changes firing sound. The rolling projectile overrides its contact handlers, `resolve_travel`, `launch_hits_players` and `duds_on_bare_cover`. Shared ammunition, firing and sweep work remains in the base workflow, which never asks which tool or kind it is running. |
| Composition | [PaintTile](../game/objects/paint_tile.h), [OptionRow](../game/states/option_row.h) | A tile owns its splash animation. A settings row owns its label and value, and accepts an adjustment callback. |
| Ownership and borrowing | [Level](../game/objects/level.h), [LevelBuilder](../game/objects/level_builder.cpp) | Scene owns its objects; Level owns Scene and borrows Player pointers. Moving a `unique_ptr` transfers ownership. |
| Dependency injection | [Player constructor](../game/objects/player.h), [GameData](../game/game_data.h), [main](../game/main.cpp) | Constructors receive their services and tuning. The application entry point owns and connects their lifetimes. |
| Polymorphic states | [MenuPage](../game/states/menu_page.h), [MainMenuPage and RosterSetupPage](../game/states/main_menu.h), [SettingsPanelPage](../game/states/settings_panel_page.h), [PauseMenuPage](../game/states/pause_menu.h), [GameMenu and GameLevel](../game/states/game_states.h) | The state stack calls common `init`, `update` and `draw` operations; individual pages supply their behavior. Each intermediate page base is named for its role and carries only what that role shares: a main-menu frame page, the roster setup pages under it, the settings panels that can only pop, and the pause pages that `finish` with a `PauseMenuAction`. The two states also implement [MenuOwner](../game/states/menu_owner.h), through which the pages above them reach their music and stage folder. |
| Resource lifetime | [Level destructor](../game/objects/level_audio.cpp), [GameLevel destructor](../game/states/game_states.cpp) | Destruction stops match audio and restores the previous screen layout as well as releasing owned objects. |

`Weapon` and `RelativeVelocityWeapon` are concrete classes. Sprayer and mister
are constructed directly as `RelativeVelocityWeapon` with different tuning.
`Collider`, `Projectile`, `DiffusingProjectile`, `PaintableObject`, `MenuPage`
and its page bases (`MainMenuPage`, `RosterSetupPage`, `SettingsPanelPage` and
`PauseMenuPage`) are abstract. `CrudeDrawing` is not abstract, but only its
leaves can construct it. `override` checks that a declaration actually
overrides a base function; `final` closes a leaf class to further derivation.
An overload has a different parameter list and is a different function, even
when its name matches a virtual base function. Most `CrudeDrawing` leaves show
this: their `draw` functions take the leaf's own parameters, so they hide
`TextureObject`'s virtual `draw` rather than override it. `Monitor::draw`'s
parameters match the base's `(DrawList&, const RectangleF&) const` exactly, so
it does override it, though it is not marked `override`. Level owns a
`Monitor` and calls it by that type, so no call reaches it virtually.

An OO example does not need every class to be polymorphic. `Session` protects
behavior without a base class, while `PlayerInputData` and `PaintTotal` carry
values between objects. Their roles are different and useful.

## Inheritance and ownership answer different questions

`WeaponBucket` **is a** `Weapon`; a `Player` **has a** `Weapon`. Public inheritance
lets code use a bucket through the weapon interface. Composition gives a player
exclusive ownership of whichever weapon the builder supplied. The player does
not become a weapon, and inheritance does not decide who deletes an object.

The principal ownership chain is:

```text
Application owns the state stack
  GameLevel owns Level
    Level owns PaintAudio and Scene
      Scene owns Players, Structures and Projectiles
        Player owns Weapon and FigureAttachments
        StructurePaintable (and its gate/translating leaves) owns PaintTiles
        FixedSlabPaintable owns its own PaintTiles
          PaintTile owns PaintTileSplash
      Level owns InterfaceGameplay (which owns a PlayerBadge)
      Level owns the other CrudeDrawing leaves: CountdownArt, Monitor,
        Silhouette, ScreenCursor, FigureActing, RevealArt, PlayerBadge,
        TeammateMarker
```

Level also borrows pointers to the Scene's players, and, collected once when
the match's objects are admitted, to its team gates and translating
structures. Players respawn rather than being removed from Scene, and structures
stay for the match, so these pointers remain valid while Level exists. The
index in Level's player vector is a stable roster participant; `player_num()`
identifies a human input slot. Bots have no device or pane. Explicit mappings
connect human pane indices to participants, so a bot in P2 does not rename P3
when P3 owns the second pane. UI containers similarly reference
widgets owned by pages or compound widgets such as `OptionRow`.

Polymorphic ownership requires safe destruction through the base type.
`PaintableObject` declares a virtual destructor. Engine
[GameObject](../external/labrador/engine/core/game_object.h),
[DrawObject](../external/labrador/engine/render/draw_object.h) and
[State](../external/labrador/engine/core/state.h) provide virtual destruction
for their hierarchies, including Projectile, Weapon and MenuPage. Derived
destructors remain virtual even when not redeclared. Do not copy these objects
into base values: that would slice off their derived state and behavior.

Constructors establish usable objects. `Session` rejects unsupported series
lengths; builders supply complete object dependencies; tuning and resource
names are checked before gameplay. Borrowed services and tuning must remain
valid for every object using them. In [main.cpp](../game/main.cpp), Save, the
stage catalog, tuning, presentation and GameData are declared before
Application, so Application's states are destroyed first. `unique_ptr` members
clean up owned subobjects if later construction throws.

`GameData` is a borrowed service bag for the shell, not a global singleton.
Gameplay builders pass the narrower dependencies individual objects need.
Tuning describes balance and presentation describes layout; C++ implements
behavior. A numeric change normally belongs in data. A new behavioral rule may
belong in a subclass.

## Player's two drawing bases

`Player` derives from `colourwars::AnimationObject` and `labrador::TextureObject`,
and both derive, non-virtually, from the engine's `SpriteSheetObject` and
through it `DrawObject`. A player therefore holds two complete `DrawObject`
subobjects, each with its own sheet, frame, colour, rotation, origin, flip and
layer depth. The two are used for different parts of one figure:

- The `AnimationObject` subobject is built with the player's tint and plays the
  body strip of the current animation state; `draw_figure` draws it with
  `draw_with`.
- The `TextureObject` subobject is built with the team colour and is given each
  state's uniform frame with `set_frame`. `Player::draw` never draws it as a
  texture: its colour is the team colour that the clothing, sleeves, idle paint
  and resting arms are drawn in, replaced by the opening's grey until the
  figure is coloured.

Because the two bases are separate, an unqualified call to a `DrawObject`
member such as `colour()` or `render_resources()` would be ambiguous, and the
compiler refuses it. [player_figure.cpp](../game/objects/player_figure.cpp)
and [player.cpp](../game/objects/player.cpp) name the base every time:
`colourwars::AnimationObject::colour()` for the body and
`TextureObject::colour()` for the uniform, `TextureObject::set_frame` when the
animation state changes. A `Player*` also cannot convert to a single
`DrawObject*`; nothing in the game needs it to, because Scene holds players as
`CollisionObject`s and calls the one `GameObject::draw` that `Player`
overrides. Virtual inheritance would merge the two into one subobject with one
colour and one frame, so it is not used here.

## Follow one shot from input to score

1. [PlayerInput::player_inputs](../game/objects/player_input.cpp) converts engine
   device snapshots into a `SlotInput` per input slot: a seat's command
   (`PlayerInputData`) plus the pause and intro-skip signals only the shell reads.
   [GameLevel::update](../game/states/game_states.cpp) checks match-level input
   such as pause, then
   [SeatCommandBuilder](../game/objects/seat_commands.cpp) maps each slot to the
   participant playing from it and adds the camera-dependent aim direction for the
   keyboard/mouse player.
2. [Level::apply_player_inputs](../game/objects/level.cpp) hands each player its
   command, after the bots and any seat filter have had theirs. Player owns the
   resulting input for its update.
3. [Player::update_weapon_and_get_projectiles](../game/objects/player.cpp)
   suppresses firing while dead and calls the owned Weapon. The weapon's shared
   firing workflow updates aim, timers, ammunition and sound, then creates
   projectiles through its firing hooks.
4. [ProjectileBuilder](../game/objects/projectile_builder.cpp) selects the
   concrete projectile class and returns owned `Projectile` objects. Player
   checks their launch paths for obstructions. Level transfers the surviving
   objects into Scene through the engine's `CollisionObject` interface.
5. [Level::update_level_logic](../game/objects/level.cpp) coordinates Scene's
   update/contact phases and the game's swept projectile movement.
   [Projectile::resolve_movement_contacts](../game/objects/projectile_contacts.cpp)
   prevents a fast flying shot from passing through intervening cover. The
   engine calls each party's `on_contact`, and
   [Collider](../game/objects/collider.h) dispatches it a second time on the
   other party's class; the engine does not know what painting means.
6. [StructurePaintable::on_projectile_contact](../game/objects/structure_paintable.cpp)
   identifies the reached tiles and calls
   [PaintTile::paint](../game/objects/paint_tile.cpp). A capture changes team and
   player ownership; friendly repaint retains the original player's credit.
   The splash is a tile-owned animation, not a separate Scene object.
   The projectile accumulates cosmetic contact results; Level-owned
   [PaintAudio](../game/objects/paint_audio.h) plays the completed batch with
   limits per paint owner across every surface. Player flushes launch hits
   before discarding blocked shots, and Level flushes Scene contacts before
   retirement. This feedback does not alter paint credit or collision response.
7. [Level::level_end_info](../game/objects/level.cpp) requests `PaintTotal` from
   scored objects and builds [LevelEndInfo](../game/objects/level_end_info.h).
   [ResultsMenu](../game/states/results_menu.cpp) displays it, while
   [Session::finish_round](../game/session.cpp) records a decisive round's win.

The score is surface still held, not accumulated shots or paint events. A
paintable object's totals must include neutral area in `total`, credited team
area in `team_a`/`team_b`, and player area indexed by paint owner (device
slots, then device-less views; [paint_owners.h](../game/paint_owners.h)). Read the
`PaintableObject` contract and Level's enrollment rule together: the object
must be in Scene's collision collection and implement `PaintableObject`.
That scoring role is independent of its collision tag: a painted one-way
platform keeps `structure_jump_through` and contributes held area in full.

Rendering follows another contract: engine `GameObject::draw` is const because
several views may draw the same object concurrently. Change animation state
during update; compute per-view tint and offsets locally during draw. The
roller's `paint_colour` override, which reads state set during update,
demonstrates this separation.

## Add a weapon without losing the contracts

`Weapon::update_and_get_projectiles` is the shared workflow. It dispatches
virtual movement/aim and ammunition/sound hooks, then calls
`shoot(direction, player_velocity)` when a shot is due. The default `shoot`
calls `calculate_projectile_launch_velocity(direction, starting_speed,
player_velocity)` and delegates construction to ProjectileBuilder.
`RelativeVelocityWeapon` overrides that same launch-velocity signature and
reads its velocity mode and scale from tuning; it inherits the rest of the
workflow. Subclasses normally override the relevant hook and inherit this
sequence. The workflow calls hooks virtually; an explicit `Weapon::hook(...)`
call instead selects the base implementation and suppresses that dispatch.
All firing results are `std::vector<std::unique_ptr<Projectile>>`; ownership
is widened to the engine collision interface only when Level adds them to
Scene.

`ready_to_shoot()` reads the same cooldown and full-shot paint gate as that
workflow. Ammunition warnings and recovery cues belong to the shared update;
the optional sniper readiness cue follows a paid cooldown. After contacts,
Player calls the virtual `update_ground_contact` hook. Its default is inert;
Roller overrides it for lift/settle sounds, after buffered jumps and spawn
settling have been accounted for. Lifecycle `stop_sounds()` stays silent.

First decide whether the change is data or behavior. Sprayer and mister show
two tuned configurations of the same concrete class. Roller, sniper and bucket
show subclasses with different behavior. Reuse an existing concrete class when
it already expresses the new weapon; introduce a subclass when it has a new
rule to implement.

For a new selectable primary weapon, keep these registrations together:

1. Add its enumerator in [weapon_type.h](../game/objects/weapon_type.h) among
   concrete primaries, before `random_primary`. Selection cycles the contiguous
   range from the first primary through Random; random selection excludes
   Random itself. Update the concrete count in
   [GameTuning](../game/tuning.h) if adding after its current last primary:
   `WEAPON_COUNT` is currently `WeaponType::bucket + 1`. `max_prim_wep` aliases
   `random_primary`. Compile-time checks beside the enum enforce this contiguous
   order and the boundary after `bucket`; update them with the registrations.
   `grenade_std` is still passed as an ignored secondary placeholder, and `none`
   marks an unset weapon. Neither belongs in the primary range.
2. Add its record under `weapons` in [tuning.json](../game/content/tuning.json).
   Register it in both the constructor's `add_weapon` list in
   [tuning.cpp](../game/tuning.cpp) and `validate_resources`' weapon list in
   [tuning_validation.cpp](../game/tuning_validation.cpp). Supply the sprite,
   projectile kind, ammunition/timing values, sound names and relative-velocity
   settings required by its chosen implementation. A looping sound needs
   independent voices for each team and player position. Each weapon supplies
   `feedback_sounds.empty`, `recovered` and a positive `empty_repeat_delay`;
   `stop`, `ready`, `lift` and `settle` are optional mechanism cues. Their
   volumes are SFX multipliers in [0, 1], and startup checks their wave names.
3. Add a case to [WeaponBuilder](../game/objects/weapon_builder.cpp) that returns
   an owning `unique_ptr<Weapon>` of the appropriate type. For a new subclass,
   declare changed hooks with `override`; add its `.cpp` to
   [game/CMakeLists.txt](../game/CMakeLists.txt). Return Projectile objects from
   firing hooks, and preserve the base's ammunition, sound and lifetime rules.
4. Add its menu icon and displayed name in
   [MainMenuWeaponSelect::update_weapon_select_visuals](../game/states/main_menu_weapon_select.cpp).
   Add the description to `main_menu.weapon_descriptions` in
   [presentation.json](../game/content/presentation.json) and the corresponding
   `describe` registration in [presentation_read_menu.cpp](../game/presentation_read_menu.cpp).
   The description array in [presentation_menu.h](../game/presentation_menu.h) includes
   `random_primary`; keep that relationship if reorganizing the enum.
5. Supply any new frames/animations in the sprite-sheet definition and texture,
   and any new wave/effect names in the live
   [sound-bank definition](../game/content/sounds/paint_exe.json). New audio
   needs source permission, master acceptance and bank publication through the
   [audio workflow](../game/audio/paint_exe/README.md). Existing asset
   files are staged on build. New resource files also need the manifest and
   [runtime asset list](../cmake/runtime_assets.cmake) updated. Optional audio
   remains optional; the game must still work with its silent bank.
6. Build Debug and Release, cycle through the new menu choice and Random, and
   start a match. Check its distinctive hook, ammo exhaustion/regeneration,
   obstruction and paint behavior, death/respawn, pause/resume and restart.
   Compare with an existing weapon to catch shared-workflow regressions. This
   repository deliberately has no `ctest` suite; engine tests cover the engine.

If the weapon introduces a new projectile kind, add its enumerator to
[projectile_type.h](../game/objects/projectile_type.h) (`ProjectileType`, with
snake_case scoped enumerators) before the `ProjectileType::count` sentinel.
Every per-kind table is sized from that sentinel (`GameTuning`'s
`PROJECTILE_COUNT`, PaintTuning's `splash_animations` and `impact_sounds`,
PaintAudio, PaintTile and ProjectileBuilder's arrays), so no count is edited by
hand; what the sentinel cannot do is fill a slot. Register the kind by name in
[tuning.cpp](../game/tuning.cpp) (`read_projectile_type`, `add_projectile`,
and the `splash_animation` and `impact` lists) and in
[tuning_validation.cpp](../game/tuning_validation.cpp) (`splash`, `projectile`
and `impact`), and add its records to tuning.json's projectiles,
`paint.splash.animations` and `paint.impact_sounds`. Add its concrete class and
a case in [ProjectileBuilder](../game/objects/projectile_builder.cpp). Decide
whether it is swept: list it in `is_swept` beside the enum, or
[tunnelling_limits.cpp](../game/tunnelling_limits.cpp) refuses to compile
until the unswept kind has a tunnelling bound.
Follow the existing team-tag mappings in
[collision_object_type.h](../game/objects/collision_object_type.h) and
[collision_object_type.cpp](../game/objects/collision_object_type.cpp), which
choose its layer and mask. Its contact rules belong to its class: the base
[Projectile](../game/objects/projectile.h) dies on its first contact and sweeps
its travel, and a kind that behaves otherwise overrides `on_structure_contact`,
`on_player_contact`, `resolve_travel`, `launch_hits_players` or
`duds_on_bare_cover`, as [ProjectileRolling](../game/objects/projectile_rolling.h)
does. Players and structures receive it as a `Projectile` through
`on_projectile_contact` and need no change. A new shape or trajectory must still
satisfy the launch/sweep assumptions; changing the enum alone does not
establish that behavior.

The classes named `WeaponBuilder` and `ProjectileBuilder` are factories: one
call selects and constructs finished objects. `LevelBuilder` assembles a larger
object graph using the other construction helpers. These names describe their
job in the game; they do not imply the classical staged Builder pattern with
separate construction steps and interchangeable representations.

Keep the guide and contracts in sync when changing an extension point. This
example needs clear, working interfaces and observable specialization; it does
not need artificial inheritance or every design pattern to fill a checklist.

## Add a scored paint object

Scoring is a role, not a registration. Level keeps no list of scored objects:
[Level::level_end_info and Level::paint_surfaces](../game/objects/level.cpp)
walk Scene's collision objects and ask each for its `paintable()` role
([Collider](../game/objects/collider.h)). Whatever answers is scored, whatever
its collision tag. The only check runs the
other way: an object tagged `structure_paintable` that is not a
`PaintableObject` throws. The steps below are what the team gate and the fixed
scored slab needed.

1. Choose the base. If the paint lies in axis-aligned bands on a rectangle,
   derive from [StructurePaintable](../game/objects/structure_paintable.h) and
   override only what changes, as
   [TeamGateStructurePaintable](../game/objects/team_gate_structure_paintable.h)
   (permission, `mask`, drawing) and
   [TranslatingStructurePaintable](../game/objects/translating_structure_paintable.h)
   (pose, contact, paint surfaces) do. Both are `final` and keep the base's
   tiles, `paint_total` and capture credit. If the paint lives in a different
   frame, take `Structure` and `PaintableObject` directly, as
   [FixedSlabPaintable](../game/objects/fixed_slab_paintable.h) does, and own
   the scoring state: its own `PaintTile`s, `PaintCredit` and
   `PaintSplashClips`, and answer `paintable()` with itself. Tiles borrow the
   splash clips, so copying is deleted in both bases.
2. Keep the [PaintableObject](../game/objects/paintable_object.h) contract.
   `paint_total` reports area held now (neutral area in `total`, team areas,
   owner areas by paint owner) and the match's `PaintCredit`.
   `append_paint_surfaces` appends the same tiles in the same order under the
   given `source_index`, and changes nothing else.
3. Pick a collision tag in
   [collision_object_type.h](../game/objects/collision_object_type.h). Reuse
   `structure_paintable` or `structure_jump_through` when the object collides
   like one of them; the gate passes `structure_paintable` to the base
   constructor. A new tag, as `structure_fixed_slab` was, is appended after
   `none` so existing replay and debug tag values keep their numbers, and is
   added to `is_structure` in that header and to `collision_layer` and
   `collision_mask` in
   [collision_object_type.cpp](../game/objects/collision_object_type.cpp).
4. Build it in `LevelObjectBuilder::build_collision_object`
   ([level_object_builder.cpp](../game/objects/level_object_builder.cpp)),
   returning an owning `unique_ptr<CollisionObject>`. The record's `type`
   selects the slab; a `gate` or `motion` field on a `StructurePaintable`
   record selects the gate or the translating structure. If its shape changes
   how neighbouring faces clip their paint, add it to the `StageContext`
   pre-pass as well, as the slab does through `context.slabs`. Scene owns the
   result from there.
5. Give per-step behaviour a typed borrow through a role. Level collects its
   gates in `build_opening_frame` ([level_opening.cpp](../game/objects/level_opening.cpp))
   and its movers in `admit_horizontal_structures`
   ([level_horizontal_motion.cpp](../game/objects/level_horizontal_motion.cpp))
   by asking each [Structure](../game/objects/structure.h) for its `gate()` or
   `mover()`, which those two leaves answer with themselves, and calls them each
   step (`settle_team_gates`). The slab is static and needed none of this.
6. Teach contacts only what the shape needs, as answers to `Structure`'s
   queries rather than as a new case in its callers. The gate inherits the
   base's painting `on_projectile_contact` and answers `passable_for`; the
   translating structure overrides the handler only to shift the splash back
   into its rest frame before handing it to the base's `paint_from_projectile`.
   The slab answers `slab_geometry`, `top_paint_team`, `top_paint_colour` and
   `cover_point` for its sloped face, which `Player::on_fixed_slab_contact` in
   [player.cpp](../game/objects/player.cpp) and the projectile resolution in
   [projectile_contacts.cpp](../game/objects/projectile_contacts.cpp) read;
   the translating structure's relative sweep lives there too.
7. Make it authorable and refused in the same words by game and editor: the
   format's vocabulary and tables in the engine-free
   [level_format.h](../game/level_format.h), the
   [level schema](../game/level.schema.json) and [level format](level-format.md), the
   editor's [level document](../editor/level_document.cpp) and
   [level JSON](../editor/level_json.cpp), and startup validation. The gate's
   rules sit in the engine-free [team_gate_contract.h](../game/team_gate_contract.h),
   read by the builder, [content_validation.cpp](../game/content_validation.cpp)
   and the editor; the slab's paint polygon in
   [fixed_slab_paint_geometry.h](../game/fixed_slab_paint_geometry.h) is
   shared the same way.
8. Add its `.cpp` to [game/CMakeLists.txt](../game/CMakeLists.txt), build Debug
   and Release, and run its acceptance case (the existing ones are
   `fixed_slab`, `team_gate` and `moving_structure`). A match on a stage that
   uses it should move the results' totals by its painted area.

Do not reach for a registry, a variant or a flatter hierarchy to make this list
shorter; AGENTS.md keeps this game's object model as it is.
