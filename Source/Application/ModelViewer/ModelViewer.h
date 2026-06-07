#pragma once
#include <chrono>
#include <memory>
#include <vector>

#include "GraphicsEngine/GraphicsEngine.h"
#include "GraphicsEngine/Objects/Mesh.h"

#pragma region WindowsIncludes
#define	WIN32_LEAN_AND_MEAN

#define NOGDICAPMASKS     
//#define NOVIRTUALKEYCODES 
//#define NOWINMESSAGES     
//#define NOWINSTYLES       
//#define NOSYSMETRICS      
#define NOMENUS           
#define NOICONS           
#define NOKEYSTATES       
#define NOSYSCOMMANDS     
#define NORASTEROPS       
//#define NOSHOWWINDOW      
#define OEMRESOURCE       
#define NOATOM            
#define NOCLIPBOARD       
#define NOCOLOR           
#define NOCTLMGR          
#define NODRAWTEXT        
#define NOGDI             
//#define NOKERNEL          
//#define NOUSER            
//#define NONLS
#define NOMB              
#define NOMEMMGR          
#define NOMETAFILE        
#define NOMINMAX          
//#define NOMSG             
#define NOOPENFILE        
#define NOSCROLL          
#define NOSERVICE         
#define NOSOUND           
#define NOTEXTMETRIC      
#define NOWH              
#define NOWINOFFSETS      
#define NOCOMM            
#define NOKANJI           
#define NOHELP            
#define NOPROFILER        
#define NODEFERWINDOWPOS  
#define NOMCX
#include <Windows.h>
#pragma endregion

#include "FreeFlyCameraController.h"

class ModelViewer
{
public:
	ModelViewer();

	bool Initialize(SIZE aWindowSize, WNDPROC aWindowProcess, LPCWSTR aWindowTitle);
	int Run();

private:

	bool myIsRunning = false;

	HWND myMainWindowHandle = nullptr;

	Mesh myMesh;
	std::vector<CU::Transform> myMeshTransforms;

	CU::Camera3D myCamera;
	CommonUtilities::InputHandler myInputHandler;
	FreeFlyCameraController myCameraController;
	std::chrono::high_resolution_clock::time_point myPreviousFrameTime{};
};
