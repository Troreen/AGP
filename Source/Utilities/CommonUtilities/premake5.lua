include "../../../Premake/common.lua"

-------------------------------------------------------------
project "CommonUtilities"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	
	targetdir ("$(SolutionDir)Lib\\$(Configuration)")
	objdir ("!$(SolutionDir)Intermediate\\$(ProjectName)\\$(Configuration)")

	files {
		"**.h",
		"**.cpp",
		"**.hpp",
	}

	multiprocessorcompile "On"
    conformancemode "On"
	 
	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		targetname("$(ProjectName)-d")

	filter "not configurations:Debug"
		runtime "Release"
		optimize "on"
		buildoptions { "/GL" }
		targetname("$(ProjectName)")

		vsprops {
			WholeProgramOptimization = "true"
		}

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
