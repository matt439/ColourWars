//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;
//using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);

    //m_deviceResources = std::make_unique<DX::DeviceResources>();
    // TODO: Provide parameters for swapchain format, depth/stencil format, and backbuffer count.
    //   Add DX::DeviceResources::c_AllowTearing to opt-in to variable rate displays.
    //   Add DX::DeviceResources::c_EnableHDR for HDR10 display.
    m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
    if (m_audio_engine)
    {
        m_audio_engine->Suspend();
    }
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(GameData* game_data)
{
    this->_data = game_data;

    this->_data->set_device_resources(m_deviceResources.get());

    MattMath::Vector2I window_size = this->_data->get_resolution_manager()->get_resolution_ivec();
    m_deviceResources->SetWindow(this->_data->get_window(),
        window_size.x, window_size.y);

    m_deviceResources->SetWindow(this->_data->get_window(),
        window_size.x, window_size.y);

    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags |= AudioEngine_Debug;
#endif
    m_audio_engine = std::make_unique<AudioEngine>(eflags);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    this->_gamepad = std::make_unique<GamePad>();
    this->_data->set_gamepad(this->_gamepad.get());

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
        {
            Update(m_timer);
        });

    Render();

}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());
    *this->_dt = elapsedTime;
    //this->_state->update();
    this->update();
    // TODO: Add your game logic here.
    if (!m_audio_engine->Update())
    {
        // more about this below...
    }

    elapsedTime;
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");

    //this->_state->draw();
    this->draw();

    m_deviceResources->PIXEndEvent();

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();

    context->ClearRenderTargetView(renderTarget, Colors::Black);
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    // Set the viewport.
    auto const viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.

    if (this->_gamepad)
        this->_gamepad->Resume();
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
    this->_gamepad->Suspend();
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
    this->_gamepad->Suspend();
    m_audio_engine->Suspend();
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();
    this->_gamepad->Resume();
    m_audio_engine->Resume();
    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}

// Properties
//void Game::GetDefaultSize(int& width, int& height) const noexcept
//{
//    // TODO: Change to desired default window size (note minimum size is 320x200).
//    width = 1920;
//    height = 1080;
//}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // TODO: Initialize device dependent objects here (independent of window size).
    auto context = m_deviceResources->GetD3DDeviceContext();
    m_spriteBatch = std::make_unique<SpriteBatch>(context);
    this->_data->set_sprite_batch(m_spriteBatch.get());

    this->_resource_manager = std::make_unique<ResourceManager>();
    this->_data->set_resource_manager(this->_resource_manager.get());
    this->_resource_loader = std::make_unique<ResourceLoader>(
        this->_resource_manager.get(), device, this->m_audio_engine.get());
    this->_data->set_resource_loader(this->_resource_loader.get());
    this->_dt = std::make_unique<float>(0.f);
    this->_data->set_dt(this->_dt.get());

    this->_viewport_manager = std::make_unique<ViewportManager>(
        this->_data->get_resolution_manager(), this->_data->get_sprite_batch(),
        this->_data->get_device_resources());
    this->_data->set_viewport_manager(this->_viewport_manager.get());

    m_states = std::make_unique<CommonStates>(device);
    this->_data->set_common_states(m_states.get());

    this->_resource_loader->load_all_resources();

    device;
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_spriteBatch.reset();
    m_states.reset();
    this->_resource_manager->reset_all_textures();
    this->_resource_manager->reset_all_sprite_fonts();
    this->_resource_manager->reset_all_sounds();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}

#pragma endregion
