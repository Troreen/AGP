include "../../../Premake/common.lua"

-------------------------------------------------------------
project "GameFramework"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"

	vsprops {
		DisableFastUpToDateCheck = "true",
		ParallelCompilation = "true"
	}

	targetdir ("$(SolutionDir)Lib\\$(Configuration)")
	targetname("$(ProjectName)")
	objdir ("!$(SolutionDir)Intermediate\\$(ProjectName)\\$(Configuration)")

	includedirs {
		".",
		dirs.source,
		dirs.engine,
		dirs.utilities,
		dirs.utilities .. "CommonUtilities",
		dirs.dependencies .. "**" .. "include",
	}

	files {
		"**.h",
		"**.cpp",
		"**.hpp",
	}

	links {
		"GraphicsEngine",
		"CommonUtilities",
		"Logger",
	}

	libdirs {
		dirs.lib .. "$(Configuration)",
		dirs.dependencies .. "**" .. "lib",
	}

	multiprocessorcompile "On"
	conformancemode "On"
	usestandardpreprocessor "On"

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
		warnings "Extra"
		defines { "_LIB" }

	filter { "system:windows", "not configurations:Retail" }
		buildoptions { "/Gm-", "/Gy", "/Gw" }

	filter {}
