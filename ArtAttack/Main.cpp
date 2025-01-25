#include "pch.h"
#include "Game.h"
#include "Save.h"
#include "ResolutionManager.h"
#include "GameData.h"
#include "GameStates.h"
#include "MattMath.h"

using namespace DirectX;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

namespace
{
    std::unique_ptr<Game> g_game = nullptr;
    std::unique_ptr<Save> g_save = nullptr;
    std::unique_ptr<ResolutionManager> g_resolution_manager = nullptr;
    std::unique_ptr<GameData> g_game_data = nullptr;
    std::unique_ptr<SimpleMath::Vector2> g_screen_resolution = nullptr;
}

LPCWSTR g_szAppName = L"Colour Wars";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ExitGame() noexcept;

// Indicates to hybrid graphics systems to prefer the discrete part by default
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

//// Retrieves all available graphics adapters
//std::vector<IDXGIAdapter1*> get_adapters()
//{
//    IDXGIFactory1* pFactory = NULL;
//    CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory);
//
//    std::vector<IDXGIAdapter1*> vAdapters;
//    IDXGIAdapter1* pAdapter = NULL;
//    for (UINT i = 0;
//        pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND;
//        ++i)
//    {
//        vAdapters.push_back(pAdapter);
//    }
//    if (pFactory)
//    {
//        pFactory->Release();
//    }
//    return vAdapters;
//}
//
//std::vector<IDXGIOutput*> get_outputs(IDXGIAdapter1* adapter)
//{
//    std::vector<IDXGIOutput*> outputs;
//    IDXGIOutput* p_output = NULL;
//    for (UINT i = 0;
//        adapter->EnumOutputs(i, &p_output) != DXGI_ERROR_NOT_FOUND;
//        ++i)
//    {
//        outputs.push_back(p_output);
//    }
//    return outputs;
//}

//RECT get_adapter0_output0_rect()
//{
//	std::vector<IDXGIAdapter1*> adapters = get_adapters();
//	std::vector<IDXGIOutput*> outputs = get_outputs(adapters[0]);
//	std::unique_ptr<DXGI_OUTPUT_DESC> output_desc = std::make_unique<DXGI_OUTPUT_DESC>();
//	HRESULT h_res = outputs[0]->GetDesc(output_desc.get());
//	RECT desktop_rect = output_desc->DesktopCoordinates;
//	return desktop_rect;
//}

// Entry point
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (!XMVerifyCPUSupport())
        return 1;

    HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
    if (FAILED(hr))
        return 1;

    g_save = std::make_unique<Save>();
    g_resolution_manager = std::make_unique<ResolutionManager>();

    g_save->load_save_file();
    g_resolution_manager->set_resolution(g_save->get_resolution());

    g_game_data = std::make_unique<GameData>();
    g_game_data->set_resolution_manager(g_resolution_manager.get());
    g_game_data->set_save(g_save.get());

    //g_screen_resolution = std::make_unique<SimpleMath::Vector2>(0.0f);
    //g_game_data->set_screen_resolution(g_screen_resolution.get());

    g_game = std::make_unique<Game>();
    g_game->set_game_data(g_game_data.get());

    // Register class and create window
    {
        // Register class
        WNDCLASSEXW wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = hInstance;
        wcex.hIcon = LoadIconW(hInstance, L"IDI_ICON");
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wcex.lpszClassName = L"ChromaClashWindowClass";
        wcex.hIconSm = LoadIconW(wcex.hInstance, L"IDI_ICON");
        if (!RegisterClassExW(&wcex))
            return 1;

        // Create window
        //int w, h;
        //g_game->GetDefaultSize(w, h);

        //RECT desktop_rect = get_adapter0_output0_rect();
        //SimpleMath::Vector2 desktop_vec =
        //    SimpleMath::Vector2(static_cast<float>(desktop_rect.right - desktop_rect.left),
        //        static_cast<float>(desktop_rect.bottom - desktop_rect.top));
        //DirectX::XMINT2 desktop_ivec = DirectX::XMINT2(static_cast<int>(desktop_vec.x), static_cast<int>(desktop_vec.y));

        //g_game_data->get_resolution()->set_resolution(desktop_ivec);



        //g_screen_resolution->x = static_cast<float>(desktop_rect.right - desktop_rect.left);
        //g_screen_resolution->y = static_cast<float>(desktop_rect.bottom - desktop_rect.top);


        MattMath::Vector2I res_ivec2 =
            g_resolution_manager->get_resolution_ivec();
        RECT rc = { 0, 0, static_cast<LONG>(res_ivec2.x), static_cast<LONG>(res_ivec2.y) };

        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

        HWND hwnd;

        if (g_save->get_fullscreen())
        {
            hwnd = CreateWindowExW(WS_EX_TOPMOST, L"ChromaClashWindowClass", g_szAppName, WS_POPUP,
                CW_USEDEFAULT, CW_USEDEFAULT, res_ivec2.x, res_ivec2.y,
                nullptr, nullptr, hInstance, g_game.get());
        }
        else //windowed
        {
            hwnd = CreateWindowExW(0, L"ChromaClashWindowClass", g_szAppName, WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, res_ivec2.x, res_ivec2.y,
                nullptr, nullptr, hInstance, g_game.get());
        }

        //HWND hwnd = CreateWindowExW(0, L"ChromaClashWindowClass", g_szAppName, WS_OVERLAPPEDWINDOW,
        //    CW_USEDEFAULT, CW_USEDEFAULT, res_ivec2.x, res_ivec2.y,
        //    nullptr, nullptr, hInstance, g_game.get());

        //HWND hwnd = CreateWindowExW(WS_EX_TOPMOST, L"ChromaClashWindowClass", g_szAppName, WS_POPUP,
        //    CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
        //    nullptr, nullptr, hInstance, g_game.get());

        //HWND hwnd = CreateWindowExW(WS_EX_TOPMOST, L"ChromaClashWindowClass", g_szAppName, WS_POPUP,
        //    CW_USEDEFAULT, CW_USEDEFAULT, desktop_ivec.x, desktop_ivec.y,
        //    nullptr, nullptr, hInstance, g_game.get());

        //SetWindowPos(hwnd, HWND_TOP, 0, 0, 800, 600, SWP_FRAMECHANGED);

        // TODO: Change to CreateWindowExW(WS_EX_TOPMOST, L"ChromaClashWindowClass", g_szAppName, WS_POPUP,
        // to default to fullscreen.

        if (!hwnd)
            return 1;

        /*ShowWindow(hwnd, nCmdShow);*/

        if (g_save->get_fullscreen())
        {
            ShowWindow(hwnd, SW_SHOWMAXIMIZED);
        }
        else //windowed
        {
            ShowWindow(hwnd, nCmdShow);
        }

        //ShowWindow(hwnd, SW_SHOWMAXIMIZED);

        // TODO: Change nCmdShow to SW_SHOWMAXIMIZED to default to fullscreen.

        GetClientRect(hwnd, &rc);

        g_game_data->set_window(hwnd);

        //g_game->Initialize(hwnd, rc.right - rc.left, rc.bottom - rc.top);
        g_game->initialize(g_game_data.get());
        g_game->transition_to(std::make_unique<GameMenu>(g_game_data.get()));
    }

    // Main message loop
    MSG msg = {};
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            g_game->tick();
        }
    }

    g_game.reset();

    CoUninitialize();

    return static_cast<int>(msg.wParam);
}

// Windows procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool s_in_sizemove = false;
    static bool s_in_suspend = false;
    static bool s_minimized = false;
    static bool s_fullscreen = g_save->get_fullscreen();
    // TODO: Set s_fullscreen to true if defaulting to fullscreen.

    auto game = reinterpret_cast<Game*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
        if (lParam)
        {
            auto params = reinterpret_cast<LPCREATESTRUCTW>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(params->lpCreateParams));
        }
        break;

    case WM_PAINT:
        if (s_in_sizemove && game)
        {
            game->tick();
        }
        else
        {
            PAINTSTRUCT ps;
            std::ignore = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DISPLAYCHANGE:
        if (game)
        {
            game->on_display_change();
        }
        break;

    case WM_MOVE:
        if (game)
        {
            game->on_window_moved();
        }
        break;

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
        {
            if (!s_minimized)
            {
                s_minimized = true;
                if (!s_in_suspend && game)
                    game->on_suspending();
                s_in_suspend = true;
            }
        }
        else if (s_minimized)
        {
            s_minimized = false;
            if (s_in_suspend && game)
                game->on_resuming();
            s_in_suspend = false;
        }
        else if (!s_in_sizemove && game)
        {
            game->on_window_size_changed(LOWORD(lParam), HIWORD(lParam));
        }
        break;

    case WM_ENTERSIZEMOVE:
        s_in_sizemove = true;
        break;

    case WM_EXITSIZEMOVE:
        s_in_sizemove = false;
        if (game)
        {
            RECT rc;
            GetClientRect(hWnd, &rc);

            game->on_window_size_changed(rc.right - rc.left, rc.bottom - rc.top);
        }
        break;

    case WM_GETMINMAXINFO:
        if (lParam)
        {
            auto info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize.x = 320;
            info->ptMinTrackSize.y = 200;
        }
        break;

    case WM_ACTIVATEAPP:
        if (game)
        {
            if (wParam)
            {
                game->on_activated();
            }
            else
            {
                game->on_deactivated();
            }
        }
        break;

    case WM_POWERBROADCAST:
        switch (wParam)
        {
        case PBT_APMQUERYSUSPEND:
            if (!s_in_suspend && game)
                game->on_suspending();
            s_in_suspend = true;
            return TRUE;

        case PBT_APMRESUMESUSPEND:
            if (!s_minimized)
            {
                if (s_in_suspend && game)
                    game->on_resuming();
                s_in_suspend = false;
            }
            return TRUE;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_SYSKEYDOWN:
        //if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
        //{
        //    // Implements the classic ALT+ENTER fullscreen toggle
        //    if (s_fullscreen)
        //    {
        //        SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
        //        SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);

        //        //int width = 800;
        //        //int height = 600;
        //        XMINT2 res_i = g_resolution->get_resolution_ivec();
        //        //if (game)
        //        //    game->GetDefaultSize(width, height);

        //        ShowWindow(hWnd, SW_SHOWNORMAL);

        //        SetWindowPos(hWnd, HWND_TOP, 0, 0, res_i.x, res_i.y, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
        //    }
        //    else
        //    {
        //        SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP);
        //        SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

        //        SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        //        ShowWindow(hWnd, SW_SHOWMAXIMIZED);
        //    }

        //    s_fullscreen = !s_fullscreen;
        //}
        break;

    case WM_MENUCHAR:
        // A menu is active and the user presses a key that does not correspond
        // to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
        return MAKELRESULT(0, MNC_CLOSE);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Exit helper
void ExitGame() noexcept
{
    PostQuitMessage(0);
}