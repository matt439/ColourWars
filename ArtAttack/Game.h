#ifndef  GAME_H
#define GAME_H

#include "DeviceResources.h"
#include "StepTimer.h"
#include "GameData.h"
#include "ResourceManager.h"
#include "StateContext.h"
#include "ThreadPool.h"
#include <Audio.h>

constexpr int TARGET_FPS = 60;
constexpr int NUM_THREADS_MIN = 1;
constexpr int NUM_THREADS_MAX = 16;

class Game final : public DX::IDeviceNotify, public StateContext
{
public:
    Game() noexcept(false);
    ~Game() override;

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void initialize(GameData* game_data);

    // Basic game loop
    void tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void on_activated() const;
    void on_deactivated() const;
    void on_suspending() const;
	void on_resuming();
    void on_window_moved() const;
    void on_display_change() const;
    void on_window_size_changed(int width, int height) const;

    void set_game_data(GameData* game_data);

private:

    void update(DX::StepTimer const& timer);
    void render();

    void clear() const;

    // Services that must survive device loss. Created once in initialize().
    void create_services();

    // GPU objects only. Re-run on every device restore.
    void create_device_dependent_resources();
    void create_window_size_dependent_resources() const;

    std::unique_ptr<DX::DeviceResources> _device_resources = nullptr;
    DX::StepTimer _timer = DX::StepTimer();

    // DECLARATION ORDER IS LOAD-BEARING BELOW THIS LINE.
    //
    // Members destruct in reverse declaration order. DirectXTK requires the
    // AudioEngine to outlive every WaveBank and SoundEffectInstance - their
    // destructors unregister themselves from it - and ResourceManager owns the
    // SoundBanks that own those. So _audio_engine is declared FIRST, and dies
    // LAST. It used to be declared near the end and was destroyed before the
    // banks it owns, on every normal exit.
    std::unique_ptr<DirectX::AudioEngine> _audio_engine = nullptr;

    std::unique_ptr<ResourceManager> _resource_manager = nullptr;
    std::unique_ptr<ResourceLoader> _resource_loader = nullptr;

    std::unique_ptr<DirectX::CommonStates> _states = nullptr;
    std::unique_ptr<float> _dt = nullptr;
    std::unique_ptr<DirectX::GamePad> _gamepad = nullptr;
    std::unique_ptr<ViewportManager> _viewport_manager = nullptr;
    GameData* _data = nullptr;
    std::unique_ptr<ThreadPool> _thread_pool = nullptr;
    std::vector<std::unique_ptr<DirectX::SpriteBatch>> _sprite_batches;
    std::vector<DirectX::SpriteBatch*> _sprite_batches_ptrs;
	std::unique_ptr<Partitioner> _partitioner = nullptr;

    // The state tree borrows every service above, so it must die first -
    // i.e. be declared last.
    std::unique_ptr<State> _state = nullptr;

    bool _resources_loaded = false;
};

#endif // ! GAME_H
