#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
    // Renders only 2D, so no need for a depth buffer.
    _device_resources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);

    //m_deviceResources = std::make_unique<DX::DeviceResources>();
    // TODO: Provide parameters for swapchain format, depth/stencil format, and backbuffer count.
    //   Add DX::DeviceResources::c_AllowTearing to opt-in to variable rate displays.
    //   Add DX::DeviceResources::c_EnableHDR for HDR10 display.
    _device_resources->RegisterDeviceNotify(this);
}

Game::~Game()
{
    if (_audio_engine)
    {
        _audio_engine->Suspend();
    }
}

// Initialize the Direct3D resources required to run.
void Game::initialize(GameData* game_data)
{
    this->_data = game_data;

    this->_data->set_device_resources(_device_resources.get());

    MattMath::Vector2I window_size = this->_data->get_resolution_manager()->get_resolution_ivec();
    _device_resources->SetWindow(this->_data->get_window(),
        window_size.x, window_size.y);


    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags |= AudioEngine_Debug;
#endif
    _audio_engine = std::make_unique<AudioEngine>(eflags);

    _device_resources->CreateDeviceResources();
    create_device_dependent_resources();

    _device_resources->CreateWindowSizeDependentResources();
    create_window_size_dependent_resources();

    this->_gamepad = std::make_unique<GamePad>();
    this->_data->set_gamepad(this->_gamepad.get());

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    _timer.SetFixedTimeStep(true);
    _timer.SetTargetElapsedSeconds(1.0 / 60);
}

void Game::set_game_data(GameData* game_data)
{
	this->_data = game_data;
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::tick()
{
    _timer.Tick([&]()
        {
            update(_timer);
        });

    render();

}

// Updates the world.
void Game::update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());
    *this->_dt = elapsedTime;
    StateContext::update();

    if (!_audio_engine->Update())
    {
        // more about this below...
    }

}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::render()
{
    // Don't try to render anything before the first Update.
    if (_timer.GetFrameCount() == 0)
    {
        return;
    }

    clear();

    _device_resources->PIXBeginEvent(L"Render");

    this->draw();

    _device_resources->PIXEndEvent();

    // Show the new frame.
    _device_resources->Present();
}

// Helper method to clear the back buffers.
void Game::clear() const
{
    _device_resources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = _device_resources->GetD3DDeviceContext();
    auto renderTarget = _device_resources->GetRenderTargetView();

    context->ClearRenderTargetView(renderTarget, Colors::Black);
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    // Set the viewport.
    auto const viewport = _device_resources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    _device_resources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::on_activated() const
{
    // TODO: Game is becoming active window.

    if (this->_gamepad)
        this->_gamepad->Resume();
}

void Game::on_deactivated() const
{
    // TODO: Game is becoming background window.
    this->_gamepad->Suspend();
}

void Game::on_suspending() const
{
    // TODO: Game is being power-suspended (or minimized).
    this->_gamepad->Suspend();
    _audio_engine->Suspend();
}

void Game::on_resuming()
{
    _timer.ResetElapsedTime();
    this->_gamepad->Resume();
    _audio_engine->Resume();
    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::on_window_moved() const
{
    auto const r = _device_resources->GetOutputSize();
    _device_resources->WindowSizeChanged(r.right, r.bottom);
}

void Game::on_display_change() const
{
    _device_resources->UpdateColorSpace();
}

void Game::on_window_size_changed(int width, int height) const
{
    if (!_device_resources->WindowSizeChanged(width, height))
        return;

    create_window_size_dependent_resources();

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
void Game::create_device_dependent_resources()
{
    auto device = _device_resources->GetD3DDevice();

    // TODO: Initialize device dependent objects here (independent of window size).
    auto context = _device_resources->GetD3DDeviceContext();
    _sprite_batch = std::make_unique<SpriteBatch>(context);
    this->_data->set_sprite_batch(_sprite_batch.get());

    this->_resource_manager = std::make_unique<ResourceManager>();
    this->_data->set_resource_manager(this->_resource_manager.get());
    this->_resource_loader = std::make_unique<ResourceLoader>(
        this->_resource_manager.get(), device, this->_audio_engine.get());
    this->_data->set_resource_loader(this->_resource_loader.get());
    this->_dt = std::make_unique<float>(0.f);
    this->_data->set_dt(this->_dt.get());

    this->_viewport_manager = std::make_unique<ViewportManager>(
        this->_data->get_resolution_manager(), this->_data->get_sprite_batch(),
        this->_data->get_device_resources());
    this->_data->set_viewport_manager(this->_viewport_manager.get());

    _states = std::make_unique<CommonStates>(device);
    this->_data->set_common_states(_states.get());

    this->_resource_loader->load_all_resources();

    device;
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::create_window_size_dependent_resources() const
{
    auto size = _device_resources->GetOutputSize();
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    _sprite_batch.reset();
    _states.reset();
    this->_resource_manager->reset_all_textures();
    this->_resource_manager->reset_all_sprite_fonts();
    this->_resource_manager->reset_all_sounds();
}

void Game::OnDeviceRestored()
{
    create_device_dependent_resources();

    create_window_size_dependent_resources();
}

#pragma endregion
