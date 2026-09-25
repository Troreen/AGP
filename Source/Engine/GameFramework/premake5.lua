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
		dirs.dependencies .. "nlohmann_json\\include",
		dirs.dependencies .. "TGAFBXImporter\\include",
		dirs.dependencies .. "**" .. "include",
	}

	files {
		"**.h",
		"**.cpp",
		"**.hpp",
		dirs.dependencies .. "nlohmann_json/include/nlohmann/json.hpp",
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

	-- The vendored simdjson amalgamation emits these warnings both while it is
	-- compiled and from its inline implementation included by our two adapters.
	-- Keep the generated third-party source untouched and scope suppression to
	-- the translation units that compile it.
	filter { "files:SimdJson/simdjson.cpp or SimdJson/simdjson.h" }
		disablewarnings { "4100", "4244", "4505", "26437", "26495", "26817" }

	filter "files:AssetHandling/AssetRegistry.cpp"
		disablewarnings { "4100", "4244" }

	filter "files:UnrealSceneImporter/UnrealSceneImporter.cpp"
		disablewarnings { "4100", "4244" }

	filter {}
