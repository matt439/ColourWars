#pragma once

#include "game/tuning.h"
#include "game/objects/weapon_type.h"
#include "game/objects/player_team.h"
#include "game/objects/player_input_data.h"
#include "game/objects/weapon.h"
#include "engine/math/matt_math.h"
#include "engine/render/animation_object.h"
#include "engine/collision/collision_object.h"
#include "game/objects/collision_object_type.h"
#include "engine/core/moving_object.h"
#include "engine/audio/sound_bank.h"
#include "engine/audio/audio_resources.h"
#include "engine/render/colour.h"
#include "engine/render/camera.h"
#include "engine/render/viewport.h"

enum class PlayerState
{
    alive,
    dead
};

enum class PlayerMoveState
{
	on_ground,
    on_drop_down_ground,
    on_ramp_left,
	on_ramp_right,
    dropping_down,
	on_ceiling,
    jumping,
    in_air,
};

enum class PlayerAnimationState
{
	idle,
	walking,
	jumping,
	falling,
    running,
};

//enum class player_collision_type
//{
//    top_edge,
//    bottom_edge,
//    left_edge,
//    right_edge,
//    top_and_bottom_edges,
//    left_and_right_edges,
//    top_and_left_edges,
//    top_and_right_edges,
//    bottom_and_left_edges,
//    bottom_and_right_edges,
//    top_and_bottom_and_left_edges,
//    top_and_bottom_and_right_edges,
//    left_and_right_and_top_edges,
//    left_and_right_and_bottom_edges,
//    contained_inside_other,
//    contains_other,
//};

class Player final : public artattack::MovingObject,
                     public artattack::CollisionObject,
                     public artattack::AnimationObject, public artattack::TextureObject
{
public:
    // `tuning` is borrowed and outlives every player: it is the GameTuning
    // main() owns. The starting rectangle is the builder's, because a spawn
    // point is the level's business, but its size and every number the
    // movement uses come off this.
    //
    // `secondary` is accepted and ignored: there is no second weapon, the
    // builder passes a fixed type, and nothing ever read the one this used to
    // store. Dropping it is a change to that call site rather than to here.
    Player(const mattmath::RectangleF& rectangle,
        const GameTuning* tuning,
        artattack::RenderResources* render_resources,
        const artattack::AudioResources* audio_resources,
        int player_num,
        PlayerTeam team,
        WeaponType primary,
        std::unique_ptr<Weapon> primary_weapon,
        WeaponType secondary,
        const artattack::Colour& team_colour,
        const artattack::Viewport& view_port,
        const mattmath::Vector2F& respawn_position,
        const mattmath::Vector2F& velocity = mattmath::Vector2F::ZERO,
        float rotation = 0.0f,
        const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
        artattack::SpriteFlip flip = artattack::SpriteFlip::none,
        float layer_depth = 0.0f);

    void update(float dt) override;
    std::vector<std::unique_ptr<artattack::CollisionObject>>
        update_weapon_and_get_projectiles(float dt) const;
    void draw(artattack::DrawList& draw_list) const override;

    mattmath::RectangleF bounds() const override;

    const mattmath::Shape* shape() const override;
    artattack::CollisionLayer layer() const override;
    artattack::CollisionMask mask() const override;
    artattack::CollisionTag tag() const override;
    void on_contact(const artattack::CollisionObject& other,
        const mattmath::Vector2F& normal, float penetration) override;
    // Always false, and Level rests on it: a player is in the same scene
    // object list as the projectiles it fires, and Level holds bare Player*
    // for the input, the cameras and the HUD. Returning true here would have
    // the scene retire the object those pointers name. A dead player respawns;
    // it does not leave the world.
    bool for_deletion() const override;

    // Closes the frame's contact phase for this player.
    //
    // "I touched no ground this tick" is not something a contact can say, so
    // it is the absence of one - and the absence is only known once they have
    // all been dispatched. The level used to work this out itself with a bool
    // hoisted above the inner loop, which is a property of the player kept by
    // whoever happened to be iterating.
    void end_contacts();

	void set_player_input(const PlayerInputData& input);
    void set_camera(const artattack::Camera& camera);

	const mattmath::RectangleF& rectangle() const;
	mattmath::Vector2F center() const;
    // Republishes MovingObject's protected accessor. Not an override - it
    // never overrode anything meaningfully, and MovingObject's accessors are
    // no longer virtual.
    const mattmath::Vector2F& velocity() const;
	const artattack::Camera& camera() const;
	const artattack::Viewport& viewport() const;
	float input_x_movement() const;
	const mattmath::Vector2F& input_shoot_direction() const;
	bool input_primary_shoot() const;
	bool input_jump_pressed() const;
	bool input_jump_held() const;
	PlayerMoveState move_state() const;
    int player_num() const;

	bool showing_debug() const;

    float health() const;

    float weapon_ammo() const;
    const artattack::Colour& team_colour() const;

    const PlayerInputData& input() const;

    PlayerState state() const;

    PlayerTeam team() const;

    float respawn_timer() const;

    void update_weapon_position() const;
    void update_prev_rectangle();
    void stop_sounds() const;

    // Everything this player is making a noise with, quieted while a menu is
    // over the match and brought back when it goes.
    void pause_sounds() const;
    void resume_sounds() const;

	std::string player_move_state_string() const;

private:
    artattack::Camera camera_ = artattack::Camera::DEFAULT_CAMERA;

    // Borrowed, never null. Read by update_movement every frame, which is a
    // pointer hop rather than the 20 constants it replaces - and those were
    // burned into the object code of every translation unit that included the
    // header, which is exactly why a balance change was a rebuild.
    //
    // The whole table rather than just the player's half, because a hit reads
    // the damage off the projectile's record: that number belongs to what was
    // fired, not to who was standing there.
    const GameTuning* tuning_ = nullptr;

    const PlayerTuning& player_tuning() const;

    std::unique_ptr<Weapon> primary_ = nullptr;

    // Both set from `tuning` in the constructor body: a member initialiser
    // cannot reach a member being initialised beside it.
    float health_ = 0.0f;
    float damage_sound_timer_ = 0.0f;
    float air_time_ = 0.0f;
    bool facing_right_ = true;
    bool showing_debug_ = false;
    PlayerState state_ = PlayerState::alive;
    float respawn_timer_ = 0.0f;
    PlayerInputData input_ = PlayerInputData();
    PlayerMoveState move_state_ = PlayerMoveState::in_air;
    PlayerAnimationState animation_state_ = PlayerAnimationState::idle;

    // Overwritten by the constructor, every one of them. They are here so the
    // class has no indeterminate members, not because any of them is a
    // meaningful default - which is why they are literals rather than the
    // DEFAULT_* constants they used to be. A default player number, team,
    // weapon pick and colour are not tuning: nothing reads them.
    int player_num_ = 0;
    PlayerTeam team_ = PlayerTeam::none;
    WeaponType primary_type_ = WeaponType::none;
    artattack::Colour team_colour_ = artattack::Colour::gray;
    artattack::Viewport viewport_ = artattack::Viewport();

    mattmath::RectangleF rectangle_ = mattmath::RectangleF::ZERO;
    mattmath::RectangleF prev_rectangle_ = mattmath::RectangleF::ZERO;

    mattmath::Vector2F respawn_position_ = mattmath::Vector2F::ZERO;

    artattack::SoundBank* sound_bank_ = nullptr;

    // Resolved once at construction. These fire from update(), so the map
    // descent they used to cost was on the frame loop (T7, T8).
    artattack::SoundBank::WaveHandle damage_sound_;
    artattack::SoundBank::WaveHandle death_sound_;
    artattack::SoundBank::WaveHandle jump_sound_;
    artattack::SoundBank::WaveHandle respawn_sound_;


    float health_regen_timer_ = 0.0f;

	const mattmath::RectangleF* collision_rectangle() const;

	CollisionObjectType collision_type() const;

	// Set by any level-geometry contact and cleared by end_contacts().
	bool touched_structure_ = false;

	void on_projectile_contact(CollisionObjectType other_type);

	// One response for all four faces of a flat structure.
	//
	// The eight-way classification this replaces - top, bottom, left, right
	// and four corners, the corners re-measuring the depth along two axes to
	// pick between two of the others - was approximating the axis of least
	// penetration, having chosen its candidate axes by a different and worse
	// rule. The normal is that axis, exactly, and it arrives measured.
	void on_structure_contact(const mattmath::Vector2F& normal,
		float penetration);

	void on_ramp_contact(CollisionObjectType other_type,
		const mattmath::Vector2F& normal, float penetration);

	void on_jump_through_contact(const artattack::CollisionObject& other);

	void update_movement(float dt);
	void do_jump(float dt);

    PlayerAnimationState calculate_animation_state() const;

    // Selects the animation clip for this frame. Called from update(), never
    // from draw() - draw runs concurrently on every render worker.
    void update_animation_state();

    const PlayerAnimationTuning& animation_info(PlayerAnimationState state) const;

    void respawn();

    bool facing_right() const;
    void set_facing_right(bool facing_right);

    float air_time() const;
    void set_air_time(float air_time);
    void alter_air_time(float change);

    void set_move_state(PlayerMoveState move_state);

	bool is_on_ground() const;
    bool is_on_ramp() const;
};
