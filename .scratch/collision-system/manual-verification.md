# Pending local visual verification

The existing standalone demo artifact predates the collider-owned debug drawing
extension. Rebuild it with the optional demo enabled before checking the new
per-collider toggles/colors; restore the source switch to false afterward.

A separate demo-enabled executable is prepared at `Bin/Debug/GameCollisionDemo.exe`; the main `Bin/Debug/Game.exe` and source default remain demo-disabled. Run the demo executable from its current folder so it shares the existing runtime dependencies/settings. No source edit is needed for this local artifact.

Use the existing selected scene (`lvl_verify_anim/Lvl_Verify_Anim_Level.json` at this run's start). Keep the initial camera still and watch the roughly 20-second timeline described in [CollisionDetection.md](../../Docs/CollisionDetection.md#run-the-optional-demonstration):

1. Cyan moving sphere enters/leaves yellow sphere/box; console reports Enter, first Stay, and Exit.
2. At 5 seconds it turns gray and exits; at 6 seconds it becomes cyan and enters again.
3. It crosses the magenta filtered sphere without an event involving that filtered ID.
4. At 16 seconds the overlapping box disappears and reports Exit.
5. Press F4 during an overlap on another run. Confirm one EndPlay listener-removal message, a fresh run, and no accumulated callbacks or teardown Exit events from the cleared World.

Report whether these observations pass, or describe what differs. Computer-use capture could not obtain a frame, so this check remains required before resolving ticket 04. The local executable is a build artifact, not a staged/committed project file.
