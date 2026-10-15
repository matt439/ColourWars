# Reading ColourWars as an OOP example

ColourWars demonstrates object-oriented design through a playable game. Its
objects own state, preserve rules, collaborate through interfaces, and specialize
behavior through inheritance. The hierarchies are deliberate. Labrador's
preference for a different style inside the engine does not govern this client.

Start with `Session` and `PaintTile` for small objects, then follow `Player`,
`Weapon` and `Projectile`, and finally the menu state stack. This guide describes
the current code; the documents under `review/` remain historical records.

## Where the concepts live

| Concept | Read | What to notice |
|---|---|---|
| Encapsulation and invariants | [Session](../game/session.h), [implementation](../game/session.cpp) | The win ledger is private. `finish_round` and `next_round` express allowed operations; repeating `finish_round` cannot award another win. |
| Abstraction | [PaintableObject](../game/objects/paintable_object.h), [Projectile](../game/objects/projectile.h) | A consumer asks for paint totals or projectile behavior without knowing the concrete representation. Pure virtual functions make required behavior explicit. |
| Single and multilevel inheritance | [WeaponBucket](../game/objects/weapon_bucket.h), [Weapon](../game/objects/weapon.h), [ProjectileMist](../game/objects/projectile_mist.h) | Bucket specializes `RelativeVelocityWeapon`, which specializes `Weapon`; mist specializes `DiffusingProjectile`, which specializes `Projectile`. Each intermediate class shares meaningful behavior. |
| Multiple inheritance | [StructurePaintable](../game/objects/structure_paintable.h), [Player](../game/objects/player.h) | A painted structure has both structure behavior and the scoring interface. A player combines movement, collision, animation and texture roles. |
| Runtime polymorphism | [Player::update_weapon_and_get_projectiles](../game/objects/player.cpp), [WeaponBuilder](../game/objects/weapon_builder.cpp) | Player owns a `unique_ptr<Weapon>`; the factory chooses its concrete type and virtual calls select the behavior. Player does not switch on weapon classes. |
| Overriding and shared algorithms | [WeaponRoller](../game/objects/weapon_roller.cpp), [WeaponBucket](../game/objects/weapon_bucket.cpp) | Roller changes aim and tint; bucket changes firing sound. Shared ammunition and firing work remains in the base workflow. |
| Composition | [PaintTile](../game/objects/paint_tile.h), [OptionRow](../game/states/option_row.h) | A tile owns its splash animation. A settings row owns its label and value, and accepts an adjustment callback. |
| Ownership and borrowing | [Level](../game/objects/level.h), [LevelBuilder](../game/objects/level_builder.cpp) | Scene owns its objects; Level owns Scene and borrows Player pointers. Moving a `unique_ptr` transfers ownership. |
| Dependency injection | [Player constructor](../game/objects/player.h), [GameData](../game/game_data.h), [main](../game/main.cpp) | Constructors receive their services and tuning. The application entry point owns and connects their lifetimes. |
| Polymorphic states | [MenuPage](../game/states/menu_page.h), [MainMenuPage](../game/states/main_menu.h), [GameMenu and GameLevel](../game/states/game_states.h) | The state stack calls common `init`, `update` and `draw` operations; individual pages supply their behavior. |
| Resource lifetime | [Level destructor](../game/objects/level.cpp), [GameLevel destructor](../game/states/game_states.cpp) | Destruction stops match audio and restores the previous screen layout as well as releasing owned objects. |

`Weapon` and `RelativeVelocityWeapon` are concrete classes. Sprayer and mister
are constructed directly as `RelativeVelocityWeapon` with different tuning.
`Projectile`, `DiffusingProjectile`, `PaintableObject`, `MenuPage` and
`MainMenuPage` are abstract. `override` checks that a declaration actually
overrides a base function; `final` closes a leaf class to further derivation.
An overload has a different parameter list and is a different function, even
when its name matches a virtual base function.

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
    Level owns Scene
      Scene owns Players, Structures and Projectiles
        Player owns Weapon
        StructurePaintable owns PaintTiles
          PaintTile owns PaintTileSplash
```

Level also borrows pointers to the Scene's players. Players respawn rather than
being removed from Scene, so these pointers remain valid for the match. The
index in Level's player vector is a view index; `player_num()` identifies an
input slot. They are not interchangeable. UI containers similarly reference
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
level registry, stages, tuning, presentation and GameData are declared before
Application, so Application's states are destroyed first. `unique_ptr` members
clean up owned subobjects if later construction throws.

`GameData` is a borrowed service bag for the shell, not a global singleton.
Gameplay builders pass the narrower dependencies individual objects need.
Tuning describes balance and presentation describes layout; C++ implements
behavior. A numeric change normally belongs in data. A new behavioral rule may
belong in a subclass.

## Follow one shot from input to score

1. [PlayerInput::player_inputs](../game/objects/player_input.cpp) converts engine
   device snapshots into `PlayerInputData`, indexed by input slot.
   [GameLevel::update](../game/states/game_states.cpp) checks match-level input
   such as pause and passes the snapshots to Level.
2. [Level::apply_player_inputs](../game/objects/level.cpp) assigns each player's
   snapshot. `resolve_mouse_aim` adds the camera-dependent aim direction for the
   keyboard/mouse player. Player owns the resulting input for its update.
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
   [Projectile::resolve_movement_contacts](../game/objects/projectile.cpp)
   prevents a fast flying shot from passing through intervening cover. Collision
   responses operate through virtual `on_contact` methods; the engine does not
   know what painting means.
6. [StructurePaintable::on_contact](../game/objects/structure_paintable.cpp)
   identifies the reached tiles and calls
   [PaintTile::paint](../game/objects/paint_tile.cpp). A capture changes team and
   player ownership; friendly repaint retains the original player's credit.
   The splash is a tile-owned animation, not a separate Scene object.
7. [Level::level_end_info](../game/objects/level.cpp) requests `PaintTotal` from
   scored objects and builds [LevelEndInfo](../game/objects/level_end_info.h).
   [ResultsMenu](../game/states/results_menu.cpp) displays it, while
   [Session::finish_round](../game/session.cpp) records a decisive round's win.

The score is surface still held, not accumulated shots or paint events. A
paintable object's totals must include neutral area in `total`, credited team
area in `team_a`/`team_b`, and player area indexed by device slot. Read the
`PaintableObject` contract and Level's enrollment rule together: the object
must be in Scene's collision collection, tagged `structure_paintable`, and
implement `PaintableObject`. Implementing the interface alone does not enroll
an object in scoring.

Rendering follows another contract: engine `GameObject::draw` is const because
several views may draw the same object concurrently. Change animation state
during update; compute per-view tint and offsets locally during draw. The
roller's `draw_colour` demonstrates this separation.

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
   In [tuning.cpp](../game/tuning.cpp), register it in both the constructor's
   `add_weapon` list and `validate_resources`' weapon list. Supply the sprite,
   projectile kind, ammunition/timing values, sound names and relative-velocity
   settings required by its chosen implementation. A looping sound needs
   independent voices for each team and player position.
3. Add a case to [WeaponBuilder](../game/objects/weapon_builder.cpp) that returns
   an owning `unique_ptr<Weapon>` of the appropriate type. For a new subclass,
   declare changed hooks with `override`; add its `.cpp` to
   [game/CMakeLists.txt](../game/CMakeLists.txt). Return Projectile objects from
   firing hooks, and preserve the base's ammunition, sound and lifetime rules.
4. Add its menu icon and displayed name in
   [MainMenuWeaponSelect::update_weapon_select_visuals](../game/states/main_menu_weapon_select.cpp).
   Add the description to `main_menu.weapon_descriptions` in
   [presentation.json](../game/content/presentation.json) and the corresponding
   `describe` registration in [presentation.cpp](../game/presentation.cpp).
   The description array in [presentation.h](../game/presentation.h) includes
   `random_primary`; keep that relationship if reorganizing the enum.
5. Supply any new frames/animations in the sprite-sheet definition and texture,
   and any new wave/effect names in the live
   [sound-bank definition](../game/content/sounds/paint_exe.json). New audio
   needs source permission, master acceptance and bank publication through the
   [audio workflow](../game/audio/paint_exe/README.md). Existing asset
   files are staged on build. New resource files also need the manifest and
   [release asset list](../cmake/release_assets.cmake) updated. Optional audio
   remains optional; the game must still work with its silent bank.
6. Build Debug and Release, cycle through the new menu choice and Random, and
   start a match. Check its distinctive hook, ammo exhaustion/regeneration,
   obstruction and paint behavior, death/respawn, pause/resume and restart.
   Compare with an existing weapon to catch shared-workflow regressions. This
   repository deliberately has no `ctest` suite; engine tests cover the engine.

If the weapon introduces a new projectile kind, also update
[projectile_type.h](../game/objects/projectile_type.h) (`ProjectileType`, with
snake_case scoped enumerators), its concrete class and
[ProjectileBuilder](../game/objects/projectile_builder.cpp), the projectile
count/loading/validation in GameTuning (`PROJECTILE_COUNT` currently ends at
`ProjectileType::ball + 1`), and the projectile entry in tuning.json.
Follow the existing team-tag mappings and contact handling in
[collision_object_type.h](../game/objects/collision_object_type.h),
[collision_object_type.cpp](../game/objects/collision_object_type.cpp),
[Projectile](../game/objects/projectile.cpp),
[Player](../game/objects/player.cpp) and
[StructurePaintable](../game/objects/structure_paintable.cpp). A new shape or
trajectory must still satisfy the launch/sweep assumptions; changing the enum
alone does not establish that behavior.

The classes named `WeaponBuilder` and `ProjectileBuilder` are factories: one
call selects and constructs finished objects. `LevelBuilder` assembles a larger
object graph using the other construction helpers. These names describe their
job in the game; they do not imply the classical staged Builder pattern with
separate construction steps and interchangeable representations.

Keep the guide and contracts in sync when changing an extension point. This
example needs clear, working interfaces and observable specialization; it does
not need artificial inheritance or every design pattern to fill a checklist.
