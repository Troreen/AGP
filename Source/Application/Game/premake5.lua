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
		dirs.utilities .. "ImGui",
        dirs.dependencies .. "nlohmann_json\\include",
        dirs.dependencies .. "**" .. "include",
		dirs.dependencies .. "TGAFBXImporter\\include",
	}

	files {
		"**.h",
		"**.cpp",
		"**.hpp",
		"**.ico",
		"**.rc",
		"**.hlsl",
		"**.hlsli",
	}

	removefiles {
		"Application.cpp",
		"Application.h",
		"FreeFlyCameraController.cpp",
		"FreeFlyCameraController.h",
		"ModelViewer.cpp",
		"ModelViewer.h",
		"Resources/ModelViewer.rc",
	}

	libdirs {
        dirs.lib .. "$(Configuration)",
        dirs.dependencies .. "**" .. "lib",
    }

	links {
		"GameFramework",
		"GraphicsEngine",
		"CommonUtilities",
		"AISystems",
		"Logger",
		"ImGui",
		"d3d11.lib",
		"dxguid.lib",
		"dxgi.lib",
		"d3dcompiler.lib",
		"TGAFbx.lib"
	}
    
    multiprocessorcompile "On"
    conformancemode "On"
	defines { "_WINDOWS" }

	prebuildcommands { 'xcopy /s /y "$(SolutionDir)Dependencies\\.dlls\\*.dll" "$(OutDir)"' }
	-- Combine custom and engine shaders beside the executable. Copy engine sources
	-- last so legacy engine files in Content cannot replace the current interfaces.
	postbuildcommands {
		'if not exist "$(SolutionDir)Content" (echo ERROR: Content stream is missing & exit /b 1)',
		'if not exist "$(SolutionDir)Source\\Engine\\GraphicsEngine\\Shaders" (echo ERROR: Engine shaders are missing & exit /b 1)',
		'if not exist "$(SolutionDir)Dependencies\\FMod\\Desktop\\Master.bank" (echo ERROR: FMOD Master.bank is missing & exit /b 1)',
		'if not exist "$(SolutionDir)Dependencies\\FMod\\Desktop\\Master.strings.bank" (echo ERROR: FMOD Master.strings.bank is missing & exit /b 1)',
		'robocopy "$(SolutionDir)Content\\Shaders" "$(OutDir)EngineShaders" *.hlsl *.hlsli /E /IS /IT /COPY:DT /DCOPY:DT /R:1 /W:1 /NFL /NDL /NJH /NJS',
		'if errorlevel 8 exit /b 1',
		'robocopy "$(SolutionDir)Source\\Engine\\GraphicsEngine\\Shaders" "$(OutDir)EngineShaders" /E /IS /IT /COPY:DT /DCOPY:DT /R:1 /W:1 /NFL /NDL /NJH /NJS',
		'if errorlevel 8 exit /b 1',
		'robocopy "$(SolutionDir)Dependencies\\FMod\\Desktop" "$(OutDir)Audio" *.bank /MIR /COPY:DT /DCOPY:DT /R:1 /W:1 /NFL /NDL /NJH /NJS',
		'if errorlevel 8 exit /b 1',
		'if not exist "$(SolutionDir)Bin\\Settings\\ApplicationSettings.json" (echo ERROR: Application settings are missing & exit /b 1)',
		'if not exist "$(SolutionDir)Bin\\Settings\\InputBindings.json" (echo ERROR: Input bindings are missing & exit /b 1)',
		'exit /b 0'
	}

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

	filter { "files:SimdJson/simdjson.cpp or SimdJson/simdjson.h" }
		disablewarnings { "4100", "4244", "4505", "26437", "26495", "26817" }
		
    filter {}
