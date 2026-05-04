#include "ModelViewer.h"

#include <algorithm>
#include <filesystem>
#include <future>
#include <iostream>
#include <vector>

#include "Application.h"

ModelViewer::ModelViewer() = default;

bool ModelViewer::Initialize(SIZE aWindowSize, WNDPROC aWindowProcess, LPCWSTR aWindowTitle)
{
	constexpr LPCWSTR windowClassName = L"ModelViewerMainWindow";

    // First we create our Window Class
    WNDCLASS windowClass = {};
    windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = aWindowProcess;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = windowClassName;
    RegisterClass(&windowClass);

    // Put the window in the middle of the screen regardless of resolution.
    LONG posX = (GetSystemMetrics(SM_CXSCREEN) - aWindowSize.cx) / 2;
	posX = std::max<LONG>(posX, 0);

	LONG posY = (GetSystemMetrics(SM_CYSCREEN) - aWindowSize.cy) / 2;
	posY = std::max<LONG>(posY, 0);

	// Then we use the class to create our window
    myMainWindowHandle = CreateWindow(
        windowClassName,                                // Classname
        aWindowTitle,                                   // Window Title
        WS_OVERLAPPEDWINDOW | WS_POPUP,                 // Flags
        posX,
        posY,
        aWindowSize.cx,
        aWindowSize.cy,
        nullptr, nullptr, nullptr,
        nullptr
    );

	{   // Graphics Init
        MVLOG(Log, "Initializing Graphics Engine...");

	    if(!GraphicsEngine::Get().Initialize(myMainWindowHandle))
	        return false;
	}

    MVLOG(Log, "Ready!");

    // Show our program window and give it focus.
    ShowWindow(myMainWindowHandle, SW_SHOW);
    SetForegroundWindow(myMainWindowHandle);

    return true;
}

int ModelViewer::Run()
{
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));

    myIsRunning = true;

    while (myIsRunning)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                myIsRunning = false;
            }

            // TODO: CU Input Manager is updated here.
        }

        // TODO: Frame Update and Rendering goes here
    }

    return 0;
}