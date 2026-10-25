// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class TGAUnrealPlugin : ModuleRules
{
	string EnginePath {
		get { return Path.GetFullPath(Target.RelativeEnginePath); }
	}

	string EngineThirdPartyPath {
		get { return Path.GetFullPath(Path.Combine(EnginePath, "Source/ThirdParty/")); }
	}

	string FBXModuleIncludePath {
		get { return Path.Combine(EngineThirdPartyPath, "FBX/2018.1.1/include/"); }
	}

	string FBXModuleLibraryPath {
		get { return Path.Combine(EngineThirdPartyPath, "FBX/2018.1.1/lib/vs2015/x64/release/"); }   
	}

	string FBXStubLibraryName {
		get { return "libfbxsdk.lib"; }
	}

	string FBXRuntimeLibraryName {
		get { return "libfbxsdk.dll"; }
	}

	string FBXSharedLibrarySymbol {
		get { return "FBXSDK_SHARED"; }
	}
	
	public TGAUnrealPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
				Path.Combine(EngineDirectory,"Source/ThirdParty/"),
				Path.Combine(EngineDirectory,"Source/Runtime/Navmesh"),
				Path.Combine(EngineDirectory,"Source/Developer/DesktopPlatform"),
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
				Path.Combine(EngineDirectory,"Source/ThirdParty/")
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"FBX",
				"Navmesh",
				"NavigationSystem"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Json",
				"JsonUtilities",
				"JsonBlueprintUtilities",
				"DeveloperSettings",
				"Landscape",
				"LandscapeEditor",
				"LandscapeEditorUtilities",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
		
	}
}
