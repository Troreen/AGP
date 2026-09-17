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

	-- These files implement the legacy standalone model viewer. The current
	-- executable enters through Main.cpp and the reusable GameFramework runtime.
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
		
    	buildoutputs { "*.hlsl*" }
        buildmessage "Copying materials to content dir"
        buildcommands {
            'set "CONTENTROOT=$(SolutionDir)Content"',
			'set "SHADERSRC=$(ProjectDir)Materials"',
			'set "SHADERDEST=%CONTENTROOT%\\Shaders"',
			'if not exist "%CONTENTROOT%" mkdir "%CONTENTROOT%"',
			'if not exist "%SHADERDEST%" mkdir "%SHADERDEST%"',
			'xcopy /E /I /R /Y "%SHADERSRC%" "%SHADERDEST%"'
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
		
		defines {
			"_LIB"
		}
        
	filter { "system:windows", "not configurations:Retail" }
		buildoptions { "/Gm-" }
		buildoptions { "/Gy" }
		buildoptions { "/Gw" }

    filter {}
