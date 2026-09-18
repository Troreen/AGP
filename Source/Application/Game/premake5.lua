include "../../../Premake/common.lua"

-------------------------------------------------------------
project "Game"
	kind "WindowedApp"
	language "C++"
	cppdialect "C++20"
	
    vsprops {
        DisableFastUpToDateCheck = "true",
        ParallelCompilation = "true"
    }
    
	targetdir ("$(SolutionDir)Bin\\$(Configuration)")
	targetname("$(ProjectName)")
	objdir ("!$(SolutionDir)Intermediate\\$(ProjectName)\\$(Configuration)")

	includedirs {
		".",
		dirs.engine,
		dirs.source,
        dirs.utilities,
        dirs.utilities .. "CommonUtilities",
        dirs.dependencies .. "**" .. "include",
        dirs.dependencies .. "**" .. "source",
	}

	files {
		"**.h",
		"**.cpp",
		"**.hpp",
		"**.ico",
		"**.rc",
		"**.hlsl",
		"**.hlsli",
		path.join(dirs.dependencies, "TGAFBXImporter", "source", "Importer.cpp"),
		path.join(dirs.dependencies, "TGAFBXImporter", "source", "Internals.cpp"),
		path.join(dirs.dependencies, "TGAFBXImporter", "source", "TgaFbxStructs.cpp"),
	}

	-- The executable enters through Main.cpp and the reusable GameFramework runtime.
	-- Keep historical standalone viewer sources out even if they appear in an import.
	removefiles {
		"Application.cpp",
		"Application.h",
		"FreeFlyCameraController.cpp",
		"FreeFlyCameraController.h",
		"ModelViewer.cpp",
		"ModelViewer.h",
		"ModelViewer.rc",
	}

	libdirs {
        dirs.lib .. "$(Configuration)",
        dirs.dependencies .. "**" .. "lib",
    }

	links {
		"GameFramework",
		"GraphicsEngine",
		"CommonUtilities",
		"Logger",
		"libfbxsdk.lib",
		"libxml2-md.lib",
		"zlib-md.lib",
		"d3d11.lib",
		"dxguid.lib",
		"dxgi.lib",
		"d3dcompiler.lib",
	}
    
    multiprocessorcompile "On"
    conformancemode "On"
	defines { "_WINDOWS", "FBXSDK_SHARED" }

	prebuildcommands { 'xcopy /s /y "$(SolutionDir)Dependencies\\.dlls\\*.dll" "$(OutDir)"' }

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		includedirs { dirs.engine .. "GraphicsEngine\\" .. "Shaders\\" .. "Material" }
		libdirs { dirs.dependencies .. "**" .. "lib\\%{cfg.buildcfg}" }
		links { "CommonUtilities-d.lib", "SoundEngine-FMod-d.lib"}

		vsprops {
        	CustomBuildAfterTargets = "Build"
		}
		
    filter "not configurations:Debug"
        intrinsics "On"
		runtime "Release"
		optimize "Speed"
		libdirs { dirs.dependencies .. "**" .. "lib\\release" }
		links { "CommonUtilities.lib", "SoundEngine-FMod.lib" }

	filter "system:windows"
		staticruntime "off"
		symbols "On"
		systemversion "latest"
		-- The distributed SoundEngine-FMod libraries reference PDBs that are not
		-- shipped. Suppress only that third-party linker diagnostic.
		linkoptions { "/IGNORE:4099" }
		
		defines {
			"_LIB"
		}
        
	filter { "system:windows", "not configurations:Retail" }
		buildoptions { "/Gm-" }
		buildoptions { "/Gy" }
		buildoptions { "/Gw" }

    filter {}
