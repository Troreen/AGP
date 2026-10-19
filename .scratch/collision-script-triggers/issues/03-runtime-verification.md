# 03: Demonstrate and verify collision-driven scripted playback

Status: in-progress
Blocked by: 01, 02

## Spec

../spec.md; AC9 and integrated verification.

## What to build

Add an opt-in, code-authored runtime demo that uses the real trigger, visible collider outlines, and a visible sequence result. Document the scene, actors, positions, controls, and F4 reload behavior. Keep its switch off by default and do not change normal Player setup.

## Acceptance criteria

- [ ] Demo uses one Actor for collider, trigger, coordinator, and sequence actions; it adds Player collider geometry only while enabled.
- [ ] The visible action demonstrates entry activation and successful one-shot collider disablement.
- [ ] F4 reload clears the old listener and creates one fresh trigger.
- [ ] Existing coordinator manual activation remains available.
- [ ] Debug Game scenes visualize every active scene collider, and successful trigger activation logs the trigger and entrant Actor names.
- [ ] Debug solution, ScriptedEventsTests, CollisionTests, CustomComponentTests, and runtime procedure are run or exact limits are documented.
- [ ] Independent spec and standards reviews use the pinned original base and selected tracked plus untracked scope; confirmed in-scope defects are corrected.

## Verification

The opt-in block is in Game::ConfigureWorld and runs only when AGP_ENABLE_COLLISION_SCRIPT_TRIGGER_DEMO is set to a nonempty value. Debug Game scenes draw every collider attached to an active Actor regardless of that switch, including gray outlines for disabled colliders. Successful trigger activation logs the sequence and entering Actor names. After this follow-up, serial Debug x64 Game.sln build and ScriptedEventsTests build/run passed; the `CollisionOnly` public-header isolation check passed. Output is captured in `../test-followup.stdout.txt` and `../test-followup.stderr.txt`. Visual GUI confirmation of all outlines, successful activation, and F4 reload remains pending.

## Comments

The demo is separate from the pre-existing collision shape demonstration. Do not imply imported collider geometry is supported.
