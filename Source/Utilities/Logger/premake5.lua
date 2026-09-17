include "../../../Premake/common.lua"

-------------------------------------------------------------
project "Logger"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	
	targetdir ("$(SolutionDir)Lib\\$(Configuration)")
	targetname("$(ProjectName)")
	objdir ("!$(SolutionDir)Intermediate\\$(ProjectName)\\$(Configuration)")

	includedirs { 
		"%{prj.location}",
        "$(VC_IncludePath)",
        "$(WindowsSDK_IncludePath)"
    }

	files {
		"**.h",
		"**.cpp",
		"**.hpp",
	}

    fatalwarnings "All"
	multiprocessorcompile "On"
	 
	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		files {"tools/**"}
		includedirs {"tools/"}
		warnings "Extra"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
		files {"tools/**"}
		includedirs {"tools/"}
		warnings "Extra"
		
	filter "configurations:Retail"
		runtime "Release"
		optimize "on"

	filter "system:windows"
		staticruntime "off"
		systemversion "latest"
		
		defines {
			"_LIB"
		}
	
	filter { "system:windows", "not configurations:Retail" }
		editandcontinue "Off"
		buildoptions { "/Gm-" }
		buildoptions { "/Gy" }
		buildoptions { "/Gw" }

    filter {}
