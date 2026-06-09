#pragma once
#include <memory>
#include <string>
#include <vector>

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


#include "GameFramework/World.h"
#include "GraphicsEngine/GraphicsEngine.h"
#include "GraphicsEngine/RHI/GraphicsCommandList.h"
#include "FreeFlyCameraController.h"
#include "MeshLibrary.h"

class Mesh;

class ModelViewer
{
	public:
	ModelViewer();
	
	bool Initialize(SIZE aWindowSize, WNDPROC aWindowProcess, LPCWSTR aWindowTitle);
	int Run();
	void LoadScene();

private:
	struct SpinningActor
	{
		Actor* Instance = nullptr;
		CommonUtilities::Vector3<float> RotationDegrees;
		CommonUtilities::Vector3<float> RotationSpeedDegrees;
	};

	std::shared_ptr<Mesh> GetRegisteredMesh(const std::string& aName) const;
	void UpdateScene(float aDeltaTime);

	bool myIsRunning = false;

	HWND myMainWindowHandle = nullptr;

	MeshLibrary myMeshLibrary;
	std::vector<SpinningActor> mySpinningActors;
	World myWorld;
	Actor* myCameraActor = nullptr;

	CommonUtilities::InputHandler myInputHandler;
	FreeFlyCameraController myCameraController;

	GraphicsCommandList myCommandList;
};
