#pragma once

#include "game/play_settings.h"

#include "game/tuning.h"
#include "game/objects/weapon_type.h"
#include "game/objects/player_team.h"
#include "game/objects/player_input_data.h"
#include "game/objects/weapon.h"
#include "game/objects/movement_feedback.h"
#include "game/objects/player_visual.h"
#include "engine/math/rectanglef.h"
#include "engine/math/shape.h"
#include "engine/math/vector2f.h"
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


class Player final : public labrador::MovingObject,
                     public labrador::CollisionObject,
                     public labrador::AnimationObject, public labrador::TextureObject
{
public:
    // Borrowed tuning and movement presentation must outlive the player. The builder supplies the spawn
    // rectangle; tuning supplies movement and health parameters.
    // secondary is currently accepted but ignored; players carry one weapon.
    Player(const mattmath::RectangleF& rectangle,
        const GameTuning* tuning,
        const MovementFeedbackPresentation* movement_feedback,
        labrador::RenderResources* render_resources,
        const labrador::AudioResources* audio_resources,
        const PlaySettings* preferences,
        int player_num,
        PlayerTeam team,
        WeaponType primary,
        std::unique_ptr<Weapon> primary_weapon,
        WeaponType secondary,
        const labrador::Colour& team_colour,
        const labrador::Viewport& view_port,
        const mattmath::Vector2F& respawn_position,
        const mattmath::Vector2F& velocity = mattmath::Vector2F::ZERO,
        float rotation = 0.0f,
        const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
        labrador::SpriteFlip flip = labrador::SpriteFlip::none,
        float layer_depth = 0.0f);

    void update(float dt) override;
    std::vector<std::unique_ptr<Projectile>>
        update_weapon_and_get_projectiles(float dt,
        std::span<labrador::CollisionObject* const> objects);
    void draw(labrador::DrawList& draw_list) const override;

    mattmath::RectangleF bounds() const override;

    const mattmath::Shape* shape() const override;
    labrador::CollisionLayer layer() const override;
    labrador::CollisionMask mask() const override;
    labrador::CollisionTag tag() const override;
    void on_contact(const labrador::CollisionObject& other,
        const mattmath::Vector2F& normal, float penetration) override;
    // Always false: Level holds borrowed Player pointers for the entire match.
    // A dead player respawns rather than being retired by Scene.
    bool for_deletion() const override;

    // Finish the contact phase and detect loss of ground support after all
    // contacts have been dispatched.
    void end_contacts();
    const MovementFeedback& movement_feedback() const;

    // A Movement Course presentation comparison. Construction and simulation
    // remain identical; the builder enables this only for an explicit choice.
    void enable_workshop_visual(const WorkshopPlayerPresentation* presentation,
        labrador::RenderResources* render_resources, PlayerVisualMode mode,
        int view_index);
    const PlayerVisual* workshop_visual() const;
    // The workshop attachment's geometry input, for consumer acceptance.
    const Weapon& primary_weapon() const;
    // Movement facing, which mirrors the body, shoulders and badge.
    bool facing_right() const;

	void set_player_input(const PlayerInputData& input);
    // Context changes discard queued jumps and require a release before a new
    // press. Level calls this when a pause/reconnect menu covers the match.
    void reset_jump_input();
    void set_camera(const labrador::Camera& camera);

	const mattmath::RectangleF& rectangle() const;
	mattmath::Vector2F center() const;
    // Expose MovingObject's protected accessor.
    const mattmath::Vector2F& velocity() const;
	const labrador::Camera& camera() const;
	const labrador::Viewport& viewport() const;
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
    const labrador::Colour& team_colour() const;

    const PlayerInputData& input() const;

    PlayerState state() const;

    PlayerTeam team() const;

    float respawn_timer() const;

    void update_weapon_position() const;
    void update_prev_rectangle();
    void stop_sounds() const;

    // Pause/resume the player's audio while a menu covers the match.
    void pause_sounds() const;
    void resume_sounds() const;

	std::string player_move_state_string() const;

private:
    labrador::Camera camera_ = labrador::Camera::DEFAULT_CAMERA;

    // Borrowed, never null. Includes player movement/health and projectile damage tuning.
    const GameTuning* tuning_ = nullptr;

    MovementFeedback movement_feedback_;
    std::unique_ptr<PlayerVisual> workshop_visual_;
    bool had_ground_support_ = false;
    bool landing_feedback_ready_ = false;
    float support_impact_speed_ = 0.0f;
    mattmath::Vector2F support_position_;

    const PlayerTuning& player_tuning() const;

    std::unique_ptr<Weapon> primary_ = nullptr;

    // Health and respawn delay are initialized from tuning in the constructor.
    float health_ = 0.0f;
    float damage_sound_timer_ = 0.0f;
    float air_time_ = 0.0f;
    float jump_buffer_remaining_ = 0.0f;
    float ledge_grace_remaining_ = 0.0f;
    bool jump_waiting_for_release_ = false;
    bool facing_right_ = true;
    bool showing_debug_ = false;
    PlayerState state_ = PlayerState::alive;
    float respawn_timer_ = 0.0f;
    PlayerInputData input_ = PlayerInputData();
    PlayerMoveState move_state_ = PlayerMoveState::in_air;
    PlayerAnimationState animation_state_ = PlayerAnimationState::idle;

    int player_num_ = 0;
    PlayerTeam team_ = PlayerTeam::none;
    WeaponType primary_type_ = WeaponType::none;
    labrador::Colour team_colour_ = labrador::Colour::gray;
    labrador::Viewport viewport_ = labrador::Viewport();

    mattmath::RectangleF rectangle_ = mattmath::RectangleF::ZERO;
    mattmath::RectangleF prev_rectangle_ = mattmath::RectangleF::ZERO;

    mattmath::Vector2F respawn_position_ = mattmath::Vector2F::ZERO;

    labrador::SoundBank* sound_bank_ = nullptr;
    const PlaySettings* preferences_ = nullptr;

    // Sound handles resolved once at construction.
    labrador::SoundBank::WaveHandle damage_sound_;
    labrador::SoundBank::WaveHandle death_sound_;
    labrador::SoundBank::WaveHandle jump_sound_;
    labrador::SoundBank::WaveHandle respawn_sound_;


    float health_regen_timer_ = 0.0f;

	const mattmath::RectangleF* collision_rectangle() const;

	CollisionObjectType collision_type() const;

	// Set by the responses that resolve a vertical contact; cleared after
	// every contact phase. Sideways and ignored platform contacts set neither.
	bool supported_this_tick_ = false;
	bool hit_ceiling_this_tick_ = false;

	void on_projectile_contact(CollisionObjectType other_type);

	// Resolve flat surfaces using the contact normal and penetration depth.
	void on_structure_contact(const labrador::CollisionObject& other,
		const mattmath::Vector2F& normal,
		float penetration);

	void on_ramp_contact(CollisionObjectType other_type,
		const labrador::CollisionObject& other,
		const mattmath::Vector2F& normal, float penetration);

	void on_jump_through_contact(const labrador::CollisionObject& other);

	void update_movement(float dt);
	void do_jump(float dt);
    void start_jump();

    PlayerAnimationState calculate_animation_state() const;

    // Select the clip during update; rendering may run concurrently for several views.
    void update_animation_state();

    const PlayerAnimationTuning& animation_info(PlayerAnimationState state) const;

    void respawn();

    void set_facing_right(bool facing_right);

    float air_time() const;
    void set_air_time(float air_time);
    void alter_air_time(float change);

    void set_move_state(PlayerMoveState move_state);

	bool is_on_ground() const;
    bool is_on_ramp() const;
};
