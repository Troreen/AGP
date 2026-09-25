# Dear ImGui debug UI

AGP vendors Dear ImGui v1.92.4-docking in `Dependencies/ImGui` (see `VERSION.txt` and
`LICENSE.txt` there). No per-developer ImGui installation or runtime DLL is
needed. Both Git and Perforce must carry that directory with the application,
engine, Premake, and generated Visual Studio project changes. Perforce ignores
generated `.vcxproj` files, so run `GenerateProject.bat` after syncing; Git
tracks them, but regenerating is also supported.

Build `Game.sln` in Visual Studio, then run Game. `F9` toggles the panel. It
starts visible in Debug and hidden in Release/Retail. Check "Dear ImGui demo"
in the panel to see available widgets. Drag an ImGui window by its title bar out
of the game window to move it to another monitor. The layout is saved as
`imgui.ini` beside `Game.exe` in the active `Bin/<configuration>` directory.
The game can remain borderless on its own monitor. Pressing `F9` in either the
game window or a detached ImGui window toggles the debug UI.

Place game-specific windows in `Game::DrawDebugUI()` in
`Source/Application/Game/Game.cpp`. Include `imgui.h` and use normal ImGui
`Begin`/`End` calls there. The callback runs once per rendered frame on the
main thread, after the world update and before ImGui renders over the scene.
The engine owns the ImGui context and the Win32/DirectX 11 backends; do not
create a second context or call their frame methods from game code. Window
messages are forwarded to ImGui, and the game input handler yields mouse and
keyboard events while ImGui captures them.
The Win32 and DirectX 11 backends create and render detached platform windows;
the application pumps their messages together with the game window's messages.

For a Perforce changelist, include all files under `Dependencies/ImGui`,
`Docs/ImGui.md`, and the changed files in `Source/Application/Game`,
`Source/Engine/GraphicsEngine`, and `Source/Utilities/CommonUtilities`. Keep
the dependency source files and build changes together so the next sync builds.
