#define WIN32_LEAN_AND_MEAN
#include <cstdio>
#include <exception>

#include "Application.h"
#include "ModelViewer.h"
#include "Windows.h"

#include "StringHelpers.h"

LRESULT CALLBACK WinProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

int GuardedMain()
{
#ifdef _DEBUG
    // Request a console window.
    AllocConsole();

    // Redirect stdout and stderr to the console. (std::cout and std::cerr)
    FILE* consoleOut;
	FILE* consoleErr;
    freopen_s(&consoleOut, "CONOUT$", "w", stdout);  // NOLINT(cert-err33-c)
    setvbuf(consoleOut, nullptr, _IONBF, 1024);  // NOLINT(cert-err33-c)
	freopen_s(&consoleErr, "CONOUT$", "w", stderr);  // NOLINT(cert-err33-c)
    setvbuf(consoleErr, nullptr, _IONBF, 1024);  // NOLINT(cert-err33-c)

    // Resize the console window to 1280 x 720 and force update.
	const HWND consoleWindow = GetConsoleWindow();
    RECT consoleSize;
    GetWindowRect(consoleWindow, &consoleSize);
    MoveWindow(consoleWindow, consoleSize.left, consoleSize.top, 1280, 720, true);

    // Read below about strings to understand this.
	SetConsoleOutputCP(CP_UTF8);
#endif

    /*
     * A note about strings:
     * Strings in C++ are terrible things. They come in a variety of formats which causes issues.
     * The reason for this is that, as a programmer, you would want one string format that can
     * handle ALL languages of the world (and more).
     * There have been an ongoing attempt since 1980 that has tried to unify how strings are
     * treated but this has over the years resulted in five major formats; ANSI, UTF-8, UTF-16,
     * UTF-16BE and UTF-16LE. None of these are compatible with the others 1=1.
     * The most modern type is UTF-8 and this is also what many modern libraries expect (ImGui,
     * FMOD, FBX SDK, etc). Windows defaults to UTF-16LE because Microsoft was one of the first
     * to adopt the unified standard well before UTF-8 was invented, and it's difficult to change
     * a whole operating systems string handling. To make matters worse there are also different
     * Code Pages for the oldest string encoding, ANSI, to allow for different languages. Microsoft
     * really did their best in a world where people could not agree on a unified String representation.
     *
     * To avoid weird problems with compatibility, mangled characters and other problems it is highly
     * recommended to store everything as UTF-8 encoded strings whenever possible. This means that
     * when we store i.e. the name of a Model, Level, Sound File, etc we do so in a UTF-8 formatted
     * std::string, and when we need to communicate with the Windows (or DirectX) API we need to use
     * std::wstring which represents a UTF-16LE string.
     *
     * I have provided functions available for conversion between these formats in the str namespace in
     * StringHelpers.h. The library can convert both from and to ACP, Wide (UTF-16LE) and UTF-8
     *
     * acp_to_utf8      - Converts Application Code Page strings to UTF-8. ACP strings come from
     *                    Windows A-apis like GetWindowTextA() or reading text from Windows Console.
     *                    LPSTR type in Windows.
     *               
     * wide_to_utf8     - Converts Wide-string strings to UTF-8. Wide-string (wstring) come from
     *                    more modern Windows APIs like GetWindowTextW(). LPWSTR type in Windows and
     *                    std::wstring / wchar_t in STL.
     *
     * The provided Logging library expects UTF-8 format strings which should provide minimal headaches
     * for any involved situation. For anything non-unicode (like non swedish signs, accents, etc) you
     * can usually use normal strings as you would anywhere since UTF-8 is assumed.
     *
     * SetConsoleOutputCP(CP_UTF8) tells the Windows Console that we'll output UTF-8. This DOES NOT
     * affect file output in any way, that's a whole other can of worms. But if you always write and
     * read your strings in the same format, and always treat them as byte blocks, you'll be fine.
     */

    MVLOG(Log, "ModelViewer starting...");

    constexpr SIZE windowSize = { 1920, 1080 };
    constexpr LPCWSTR windowTitle = L"AGP Modelviewer"; // L"" denotes "I want this to be a wide-string".

    ModelViewer MV;
    MV.Initialize(windowSize, WinProc, windowTitle);
    return MV.Run();
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    try
	{
		return GuardedMain();
	}
    catch(const std::exception& e)
    {
        std::string message = e.what();
        if(!str::is_valid_utf8(message))
        {
	        message = str::acp_to_utf8(message);
        }
        MVLOG(Error, "Exception caught!\n{}", message);
        return -1;
    }

    return 0;
}

LRESULT CALLBACK WinProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    if (uMsg == WM_DESTROY || uMsg == WM_CLOSE)
    {
        PostQuitMessage(0);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}