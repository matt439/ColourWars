#pragma once

#include "game/play_settings.h"

#include "game/tuning.h"
#include "game/objects/weapon_type.h"
#include "game/objects/player_team.h"
#include "game/objects/player_input_data.h"
#include "game/objects/weapon.h"
#include "game/objects/movement_feedback.h"
#include "game/objects/combat_feedback.h"
#include "game/objects/movement_audio.h"
#include "game/objects/respawn_redraw.h"
#include "game/objects/figure_clothing.h"
#include "game/objects/figure_attachments.h"
#include "game/objects/quiet_character.h"
#include "engine/math/rectanglef.h"
#include "engine/math/shape.h"
#include "engine/math/vector2f.h"
#include "engine/render/animation_object.h"
#include "engine/collision/collision_object.h"
#include "engine/collision/contacts.h"
#include "game/objects/collision_object_type.h"
#include "engine/core/moving_object.h"
#include "engine/audio/sound_bank.h"
#include "engine/audio/audio_resources.h"
#include "engine/render/colour.h"
#include "engine/render/camera.h"
#include "engine/render/viewport.h"
#include <array>
#include <optional>

enum class PlayerState
{
    alive,
    dead
};

class StructurePaintable;

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
    taking_off,
    landing,
    firing,
    hurt,
    elimination,
};


// How the opening and the reveal move a figure without touching the
// simulation: drawn shifted, squashed from the feet up, without its tool.
struct FigurePose
{
    mattmath::Vector2F offset = mattmath::Vector2F::ZERO;
    float squash = 0.0f;
    bool tool = true;
    // A dead figure drawn finished at its spawn: in the reveal everyone is
    // in the picture, however far its redraw had got when the clock ran out.
    bool whole = false;
};

class Player final : public labrador::MovingObject,
                     public labrador::CollisionObject,
                     public labrador::AnimationObject, public labrador::TextureObject
{
public:
    // Borrowed tuning and presentation must outlive the player. The builder supplies the spawn
    // rectangle; tuning supplies movement and health parameters.
    // secondary is currently accepted but ignored; players carry one weapon.
    // The builder supplies sound_index as the roster/view index, independently
    // of sparse device slots and device-less debug players.
    // teammate_index picks a distinct head accent within that player's team.
    Player(const mattmath::RectangleF& rectangle,
        const GameTuning* tuning,
        const MovementFeedbackPresentation* movement_feedback,
        const CombatFeedbackPresentation* combat_feedback,
        const RespawnRedrawPresentation* respawn_redraw,
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
        float layer_depth = 0.0f,
        int sound_index = -1,
        const FigurePresentation* figure = nullptr,
        int teammate_index = -1);

    ~Player() override;

    void update(float dt) override;
    std::vector<std::unique_ptr<Projectile>>
        update_weapon_and_get_projectiles(float dt,
        std::span<labrador::CollisionObject* const> objects);
    // Borrowed from the owning Level; launch hits are flushed before retired
    // shots leave this function, rather than disappearing before Scene sees them.
    void set_paint_audio(PaintAudio* audio);
    void draw(labrador::DrawList& draw_list) const override;

    mattmath::RectangleF bounds() const override;
    // While dead, the spawn too: the redraw is there, wherever the player fell.
    mattmath::RectangleF cull_bounds(float units_per_pixel) const override;

    const mattmath::Shape* shape() const override;
    labrador::CollisionLayer layer() const override;
    labrador::CollisionMask mask() const override;
    labrador::CollisionTag tag() const override;
    void on_contact(const labrador::CollisionObject& other,
        const mattmath::Vector2F& normal, float penetration) override;
    // Always false: Level holds borrowed Player pointers for the entire match.
    // A dead player respawns rather than being retired by Scene.
    bool for_deletion() const override;

    // Finish the contact phase after dispatch. Measured contacts retain adjacent
    // support candidates whose callbacks an earlier separation made redundant.
    void end_contacts(std::span<const labrador::Contact> contacts = {});
    const MovementFeedback& movement_feedback() const;
    const CombatFeedback& combat_feedback() const;
    const QuietCharacter& quiet_character() const;

    // The equipped tool, including its current gameplay aim.
    const Weapon& primary_weapon() const;
    // Movement facing, which mirrors the figure.
    bool facing_right() const;
    // The selected presentation clip and its deterministic, simulation-aged
    // action phase. Locomotion has no active action and reports frame/age zero.
    PlayerAnimationState animation_state() const;
    float action_age() const;
    int action_frame() const;

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
    // How far the redraw at the spawn has come: 0 at death, 1 at respawn.
    float respawn_progress() const;
    // Where the player is drawn while dead, and appears again.
    mattmath::RectangleF spawn_rectangle() const;
    // What the camera follows: the player, or, a moment after death, the
    // spawn where the figure is being redrawn.
    mattmath::Vector2F camera_focus() const;

    void update_weapon_position() const;
    void update_prev_rectangle();
    void stop_sounds() const;
    // Lifecycle reset is explicit and mutable, separate from const audio stops.
    void reset_tool_motion();

    // Pause/resume the player's audio while a menu covers the match.
    void pause_sounds() const;
    void resume_sounds() const;

	std::string player_move_state_string() const;

    // Developer tools. Plain state in every build; only Debug builds set it
    // (the console's overlay.player, god, ammo, goto and respawn).
    void set_showing_debug(bool showing);
    void set_damage_immune(bool immune);
    void set_infinite_ammo(bool infinite);
    // At rest at `top_left` (the character's top-left), placed exactly as a
    // respawn places it, without the respawn's health, ammo, state or sound.
    void place_at(const mattmath::Vector2F& top_left);
    // Respawns now, at the spawn, as the respawn timer would.
    void force_respawn();
    // Health to zero, ignoring god: the next update dies as a fatal hit does.
    void kill();

    // In the opening the figures are grey until they see their colour
    // (docs/backstory.md): uniform and tool in `grey` until coloured.
    void set_coloured(bool coloured, const labrador::Colour& grey);
    // Set by Level between updates; drawing only.
    void set_pose(const FigurePose& pose);
    // Where the figure is drawn, with its pose: at its spawn if it is dead
    // and drawn whole.
    mattmath::RectangleF posed_rectangle() const;
    // Dead, but in the picture because the reveal draws everyone.
    bool drawn_whole() const;

private:
    PaintAudio* paint_audio_ = nullptr;
    labrador::Camera camera_ = labrador::Camera::DEFAULT_CAMERA;

    // Borrowed, never null. Includes player movement/health and projectile damage tuning.
    const GameTuning* tuning_ = nullptr;

    FigureClothing clothing_;
    FigureAttachments attachments_;
    QuietCharacter quiet_character_;
    MovementFeedback movement_feedback_;
    CombatFeedback combat_feedback_;
    MovementAudio movement_audio_;
    float movement_audio_dt_ = 0.0f;
    float movement_start_x_ = 0.0f;
    float movement_requested_x_ = 0.0f;
    float movement_previous_velocity_ = 0.0f;
    RespawnRedraw respawn_redraw_;
    std::optional<labrador::Colour> grey_;
    FigurePose pose_;
    bool had_ground_support_ = false;
    bool landing_feedback_ready_ = false;
    float weapon_contact_dt_ = 0.0f;
    float support_impact_speed_ = 0.0f;
    const MovementFeedbackPresentation* movement_feedback_look_ = nullptr;
    mattmath::Vector2F support_position_;
    // Borrowed only for this contact phase; query after every paint callback.
    std::vector<const StructurePaintable*> paint_supports_;
    struct PaintedSupport
    {
        labrador::Colour colour;
        mattmath::Vector2F position;
    };
    std::optional<PaintedSupport> painted_support() const;

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
    // Developer cheats: no paint damage; the tank refilled around every shot.
    bool damage_immune_ = false;
    bool infinite_ammo_ = false;
    PlayerState state_ = PlayerState::alive;
    float respawn_timer_ = 0.0f;
    PlayerInputData input_ = PlayerInputData();
    PlayerMoveState move_state_ = PlayerMoveState::in_air;
    PlayerAnimationState animation_state_ = PlayerAnimationState::idle;
    std::optional<PlayerAnimationState> action_;
    float action_age_ = 0.0f;
    struct ActionClip
    {
        int frames = 0;
        float frame_time = 0.0f;
    };
    std::array<ActionClip, 5> action_clips_;
    labrador::RenderResources::SpriteSheetHandle idle_sheet_;
    mattmath::RectangleI idle_body_;
    mattmath::RectangleI idle_paint_;

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
    labrador::SoundBank::EffectHandle redraw_sound_;
    size_t redraw_index_ = 0;
    mutable bool redraw_playing_ = false;
    mutable bool redraw_paused_ = false;


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
    void set_animation_state(PlayerAnimationState state, bool restart = false);
    void start_action(PlayerAnimationState state);
    void age_action(float dt);
    void update_action_frame();
    ActionClip action_clip(PlayerAnimationState state) const;
    void draw_figure(labrador::DrawList& draw_list,
        const mattmath::RectangleF& rectangle, bool idle = false) const;
    void draw_respawn(labrador::DrawList& draw_list) const;

    const PlayerAnimationTuning& animation_info(PlayerAnimationState state) const;

    void respawn();
    bool redrawing() const;
    float redraw_gain() const;
    void update_redraw_sound(float dt) const;
    void stop_redraw_sound() const;

    void set_facing_right(bool facing_right);

    float air_time() const;
    void set_air_time(float air_time);
    void alter_air_time(float change);

    void set_move_state(PlayerMoveState move_state);

	bool is_on_ground() const;
    bool is_on_ramp() const;
};
