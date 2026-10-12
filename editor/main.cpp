#include "editor/editor.h"
#include "editor/win32_text.h"

#include <windows.h>
#include <shellapi.h>

#include <exception>
#include <filesystem>

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using colourwars_editor::Editor;
using colourwars_editor::wide;

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show)
{
    try
    {
        int count = 0; wchar_t** arguments = CommandLineToArgvW(GetCommandLineW(), &count);
        const std::filesystem::path path = arguments && count > 1 ? std::filesystem::path(arguments[1]) : std::filesystem::path{};
        if (arguments) LocalFree(arguments);
        Editor editor(instance);
        return editor.run(show, path);
    }
    catch (const std::exception& error)
    {
        MessageBoxW(nullptr, wide(error.what()).c_str(), L"ColourWars Level Editor", MB_OK | MB_ICONERROR);
        return 1;
    }
}
