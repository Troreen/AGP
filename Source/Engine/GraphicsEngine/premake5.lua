include "../../../Premake/common.lua"

-------------------------------------------------------------
project "GraphicsEngine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	
    vsprops {
        DisableFastUpToDateCheck = "true",
        ParallelCompilation = "true"
    }
    
	pchheader "GraphicsEngine.pch.h"
	pchsource "GraphicsEngine.pch.cpp"
	
	targetdir ("$(SolutionDir)Lib\\$(Configuration)")
	targetname("$(ProjectName)")
	objdir ("!$(SolutionDir)Intermediate\\$(ProjectName)\\$(Configuration)")

	includedirs {
		".",
		dirs.engine,
		dirs.source,
        dirs.utilities,
        dirs.utilities .. "CommonUtilities",
        dirs.dependencies .. "**" .. "include",
	}

	files {
		"**.h",
		"**.cpp",
		"**.hpp",
		"Shaders/**.hlsl",
		"Shaders/**.hlsli",
	}

	removefiles { "Content/**", "TemporaryShaders/**", "Intermediate/**" }

	libdirs {
        dirs.lib .. "$(Configuration)",
        dirs.dependencies .. "**" .. "lib",
    }
    
    multiprocessorcompile "On"
    conformancemode "On"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		libdirs { dirs.dependencies .. "**" .. "lib\\%{cfg.buildcfg}" }

    filter "not configurations:Debug"
        intrinsics "On"
		runtime "Release"
		optimize "Speed"
		libdirs { dirs.dependencies .. "**" .. "lib\\release" }
        
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

    -- GraphicsEngine compiles HLSL at runtime from deployed Content/Shaders.
    filter "files:Shaders/**.hlsl"
        buildaction "None"

	filter "files:**/DDSTextureLoader11.cpp"
		enablepch "Off"
    
    filter {}
