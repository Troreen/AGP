include "../../../Premake/common.lua"

-------------------------------------------------------------
project "GraphicsEngine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	
    vsprops {
        DisableFastUpToDateCheck = "true",
        ParallelCompilation = "true",
        CustomBuildAfterTargets = "Build"
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
		"**.hlsl",
		"**.hlsli",
	}

	libdirs {
        dirs.lib .. "$(Configuration)",
        dirs.dependencies .. "**" .. "lib",
    }
    
    buildoutputs { "*.hlsl*" }
    multiprocessorcompile "On"
    conformancemode "On"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		libdirs { dirs.dependencies .. "**" .. "lib\\%{cfg.buildcfg}" }

        buildmessage "Copying Shaders to Content Dir"
        buildcommands {
            'set "CONTENTROOT=$(SolutionDir)Content"',
            'set "SHADERSRC=$(ProjectDir)Shaders"',
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
        
        buildmessage "Copying Shaders to $(SolutionDir)Bin\\$(Configuration)\\Shaders..."
        buildcommands {
            'IF EXIST "$(ProjectDir)Shaders"  (',
            'xcopy /E /I /R /Y "$(ProjectDir)Shaders" "$(SolutionDir)Bin\\$(Configuration)\\Shaders")'
        }

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

    shadermodel "5.0"
    shaderentry "main"
    shaderobjectfileoutput ""

    filter "configurations:Debug"
        shaderheaderfileoutput "$(IntDir)PrecompiledShaders\\%%(Filename).h"
        shadervariablename "INTERNAL_%%(Filename)_ByteCode"

    filter "not configurations:Debug"
        shaderheaderfileoutput "$(ProjectDir)TemporaryShaders\\%%(Filename).h"
        shadervariablename "TEMP_%%(Filename)_ByteCode"

    filter "files:**_VS.hlsl"
        shadertype "Vertex"

    filter "files:**_PS.hlsl"
        shadertype "Pixel"

    filter "files:**_GS.hlsl"
        shadertype "Geometry"

	filter "files:**/DDSTextureLoader11.cpp"
		enablepch "Off"
    
    filter {}
