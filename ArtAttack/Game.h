#ifndef  GAME_H
#define GAME_H

#include "DeviceResources.h"
#include "StepTimer.h"
#include "GameData.h"
#include "ResourceManager.h"
#include "StateContext.h"
#include <Audio.h>

class Game final : public DX::IDeviceNotify, public StateContext
{
private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    std::unique_ptr<DX::DeviceResources> m_deviceResources = nullptr;
    DX::StepTimer m_timer = DX::StepTimer();

    std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch = nullptr;

    std::unique_ptr<DirectX::CommonStates> m_states = nullptr;

    std::unique_ptr<ResourceLoader> _resource_loader = nullptr;
    std::unique_ptr<ResourceManager> _resource_manager = nullptr;
    std::unique_ptr<float> _dt = nullptr;
    std::unique_ptr<State> _state = nullptr;
    std::unique_ptr<DirectX::GamePad> _gamepad = nullptr;
    std::unique_ptr<ViewportManager> _viewport_manager = nullptr;
    GameData* _data = nullptr;

    std::unique_ptr<DirectX::AudioEngine> m_audio_engine = nullptr;

public:
    Game() noexcept(false);
    ~Game();

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void Initialize(GameData* game_data);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();    void OnResuming();
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    void set_game_data(GameData* game_data) { this->_data = game_data; }
};

#endif // ! GAME_H
