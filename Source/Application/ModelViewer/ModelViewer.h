#pragma once
#include <array>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
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
#include "FrameScheduler.h"
#include "FreeFlyCameraController.h"
#include "MeshLibrary.h"

class Mesh;
class DirectionalLightComponent;
class MaterialInterface;
class PointLightComponent;
class SkeletalMeshComponent;
class SpotLightComponent;
class StaticMeshComponent;

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

	struct ModelViewerInputFrame
	{
		std::array<bool, 256> KeysDown = {};
		std::array<bool, 256> KeysPressed = {};
		FreeFlyCameraController::InputState CameraInput;

		void ClearPressed();
	};

	std::shared_ptr<Mesh> GetRegisteredMesh(const std::string& aName) const;
	std::shared_ptr<MaterialInterface> GetMaterial(const std::filesystem::path& aMaterialFile);
	StaticMeshComponent* CreateStaticMeshActor(
		const std::string& anActorName,
		const std::string& aComponentName,
		const std::string& aMeshName,
		const std::filesystem::path& aMaterialFile,
		const CommonUtilities::Vector3<float>& aPosition,
		const CommonUtilities::Vector3<float>& aRotationDegrees,
		const CommonUtilities::Vector3<float>& aScale);
	void HandleAnimationInput(const ModelViewerInputFrame& anInputFrame);
	void HandleLightInput(const ModelViewerInputFrame& anInputFrame);
	void UpdateScene(float aDeltaTime);
	void StartUpdateThread();
	void StopUpdateThread();
	void RunFixedUpdateStep(float aDeltaTime, ModelViewerInputFrame& inoutInputFrame);
	void BuildAndPublishRenderSnapshot();
	void LogRuntimeStats() const;
	ModelViewerInputFrame CaptureInputFrame();
	void SubmitInputFrame(const ModelViewerInputFrame& anInputFrame);
	bool ConsumePendingInputFrame(ModelViewerInputFrame& inoutInputFrame);

	static bool IsKeyDown(const ModelViewerInputFrame& anInputFrame, Keys aKey);
	static bool IsKeyPressed(const ModelViewerInputFrame& anInputFrame, Keys aKey);

	bool myIsRunning = false;

	HWND myMainWindowHandle = nullptr;

	MeshLibrary myMeshLibrary;
	std::vector<SpinningActor> mySpinningActors;
	World myWorld;
	Actor* myCameraActor = nullptr;
	SkeletalMeshComponent* myAnimatedMeshComponent = nullptr;
	DirectionalLightComponent* myDirectionalLightComponent = nullptr;
	std::vector<PointLightComponent*> myPointLightComponents;
	SpotLightComponent* mySpotLightComponent = nullptr;

	CommonUtilities::InputHandler myInputHandler;
	FreeFlyCameraController myCameraController;

	GraphicsCommandList myCommandList;
	static constexpr size_t RenderSnapshotBufferCount = 3;
	EngineScheduling::TripleBufferedSnapshotQueue<GraphicsEngine::RenderSceneSnapshot, RenderSnapshotBufferCount> myRenderSnapshots;

	EngineScheduling::FixedStepUpdateWorker<ModelViewerInputFrame> myUpdateWorker;
	std::mutex myInputMutex;
	std::condition_variable myInputCondition;
	ModelViewerInputFrame myPendingInputFrame;
	bool myHasPendingInputFrame = false;
	bool myHasMainThreadMouseLookAnchor = false;

	std::filesystem::path myContentRoot;
	std::unordered_map<std::string, std::shared_ptr<MaterialInterface>> myMaterialCache;
};
