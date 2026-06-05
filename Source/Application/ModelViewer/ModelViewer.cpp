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

    {
        // Temporary Mesh init

        std::vector<Vertex> meshVertices(24);
        // front face
        meshVertices[0].Position = { -0.5f, -0.5f, -0.5f, 1 };
        meshVertices[1].Position = { 0.5f, -0.5f, -0.5f, 1 };
        meshVertices[2].Position = { 0.5f, 0.5f, -0.5f, 1 };
        meshVertices[3].Position = { -0.5f, 0.5f, -0.5f, 1 };
        // back face
        meshVertices[4].Position = { -0.5f, -0.5f, 0.5f, 1 };
        meshVertices[5].Position = { 0.5f, -0.5f, 0.5f, 1 };
        meshVertices[6].Position = { 0.5f, 0.5f, 0.5f, 1 };
        meshVertices[7].Position = { -0.5f, 0.5f, 0.5f, 1 };
        // left face
        meshVertices[8].Position = { -0.5f, -0.5f, 0.5f, 1 };
        meshVertices[9].Position = { -0.5f, -0.5f, -0.5f, 1 };
        meshVertices[10].Position = { -0.5f, 0.5f, -0.5f, 1 };
        meshVertices[11].Position = { -0.5f, 0.5f, 0.5f, 1 };
        // right face
        meshVertices[12].Position = { 0.5f, -0.5f, -0.5f, 1 };
        meshVertices[13].Position = { 0.5f, -0.5f, 0.5f, 1 };
        meshVertices[14].Position = { 0.5f, 0.5f, 0.5f, 1 };
        meshVertices[15].Position = { 0.5f, 0.5f, -0.5f, 1 };
        // top face
        meshVertices[16].Position = { -0.5f, 0.5f, -0.5f, 1 };
        meshVertices[17].Position = { 0.5f, 0.5f, -0.5f, 1 };
        meshVertices[18].Position = { 0.5f, 0.5f, 0.5f, 1 };
        meshVertices[19].Position = { -0.5f, 0.5f, 0.5f, 1 };
        // bottom face
        meshVertices[20].Position = { -0.5f, -0.5f, 0.5f, 1 };
        meshVertices[21].Position = { 0.5f, -0.5f, 0.5f, 1 };
        meshVertices[22].Position = { 0.5f, -0.5f, -0.5f, 1 };
        meshVertices[23].Position = { -0.5f, -0.5f, -0.5f, 1 };

        // indices of a cube
        std::vector<unsigned> 
        meshIndices = {
            // front face
            0, 2, 1,
            0, 3, 2,
            
            // right face
            1, 2, 6,
            1, 6, 5,
            
            // back face
            4, 5, 6,
            4, 6, 7,
            
            // left face
            0, 4, 7,
            0, 7, 3,
            
            // top face
            0, 1, 5,
            0, 5, 4,
            
            // bottom face
            3, 7, 6,
            3, 6, 2
        };

        Mesh::Element cubeElement;
        cubeElement.NumVertices = static_cast<unsigned>(meshVertices.size());
        cubeElement.NumIndices = static_cast<unsigned>(meshIndices.size());

        myMesh.Initialize("Cube", { cubeElement }, std::move(meshVertices), std::move(meshIndices));
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
        GraphicsEngine::Get().Render(myMesh);
    }

    return 0;
}