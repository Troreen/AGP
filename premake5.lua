include "Premake/extensions.lua"

-- Name of the VS Solution.
workspace "Game"
    -- What configurations we will have.
    configurations { "Debug", "Release", "Retail" }
    platforms { "x64" }
    toolset "v145"
    startproject "Game"

filter { "platforms:x64" }
    system "Windows"
    architecture "x86_64"
    buildoptions "/sdl"

-- For Debug configuration, we want to set the _DEBUG #define flag and turn on symbol generation.
filter "configurations:Debug"
    warnings "Extra"
    defines { "_DEBUG" }
    symbols "On"

-- Generate symbols in release mode so we can debug the release build if needed.
filter "configurations:Release"
    warnings "Extra"
    defines { "_RELEASE" }
    optimize "On"
    symbols "On"

-- Settings to use for retail build used for shipping the game.
filter "configurations:Retail"
    warnings "Extra"
    defines { "_RETAIL" }
    optimize "On"

filter "not configurations:Debug"
    defines {"NDEBUG"}

filter {} 

group "Engine"
    include "Source/Engine/GraphicsEngine/premake5.lua"
    include "Source/Engine/GameFramework/premake5.lua"

group "External"
    include "Source/Utilities/Logger/premake5.lua"
    include "Source/Utilities/CommonUtilities/premake5.lua"

group "Game"
    include "Source/Application/Game/premake5.lua"
