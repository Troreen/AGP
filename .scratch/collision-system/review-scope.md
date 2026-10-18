# Collision system independent review scope

Base: d627d712a1024b4cfb7f1b623692e51ef4714ae9
Branch: collision-system. HEAD remains the original base; no feature commits.

Tracked diff: `git diff d627d712a1024b4cfb7f1b623692e51ef4714ae9 -- Source Tests Docs README.md .scratch/collision-system`
Read selected new source/docs below; untracked file content is not included in git diff. Exclude the unrelated Bin/Settings/ApplicationSettings.json delta. Evidence logs may be consulted for verification but are not source changes.

- .scratch/collision-system/evidence/logger-shutdown.md
- .scratch/collision-system/issues/01-safe-eventbus-lifecycle.md
- .scratch/collision-system/issues/02-sphere-collider-events.md
- .scratch/collision-system/issues/03-box-collider-overlaps.md
- .scratch/collision-system/issues/04-demo-and-verification.md
- .scratch/collision-system/research.md
- .scratch/collision-system/spec.md
- .scratch/collision-system/workflow.md
- Docs/CollisionDetection.md
- README.md
- Source/Application/Game/Components/CollisionDemoComponent.cpp
- Source/Application/Game/Components/CollisionDemoComponent.h
- Source/Application/Game/Game.cpp
- Source/Application/Game/Game.vcxproj
- Source/Application/Game/Game.vcxproj.filters
- Source/Engine/GameFramework/Collision/CollisionOverlapEvent.h
- Source/Engine/GameFramework/Collision/CollisionSystem.cpp
- Source/Engine/GameFramework/Collision/CollisionSystem.h
- Source/Engine/GameFramework/Components/ColliderComponent.cpp
- Source/Engine/GameFramework/Components/ColliderComponent.h
- Source/Engine/GameFramework/GameFramework.vcxproj
- Source/Engine/GameFramework/GameFramework.vcxproj.filters
- Source/Engine/GameFramework/World/Actor.cpp
- Source/Engine/GameFramework/World/World.cpp
- Source/Engine/GameFramework/World/World.h
- Source/Engine/GraphicsEngine/GraphicsEngine.vcxproj
- Source/Engine/GraphicsEngine/GraphicsEngine.vcxproj.filters
- Source/Utilities/Logger/Logger.cpp
- Tests/GameFramework/CollisionTests.cpp
- Tests/GameFramework/CollisionTests.vcxproj
- Tests/GameFramework/CustomComponentTests.vcxproj
- Tests/GameFramework/GameApplicationTests.vcxproj
- Tests/GameFramework/RunPublicHeaderIsolation.ps1
- Tests/GameFramework/ScriptedEventsTests.vcxproj
- Tests/GameFramework/ScriptedEventsTests.cpp

EventBus ownership revision: EventBus source is unchanged from HEAD; superseded/eventbus/ is historical, outside active review/build scope.
