#include "ModelViewerComponents.h"
#include "GameFramework/World.h"
#include "GameFramework/Components/LightComponent.h"
#include "Runtime/Internal/WorldAccess.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
	using CommonUtilities::Quaternion;
	using CommonUtilities::Vector3f;
	using GameFrameworkInternal::WorldAccess;
	constexpr float DegreesToRadians = 0.017453292519943295f;
	constexpr float LookSensitivity = 0.0025f;

	void ExpectVector(const Vector3f& actual, const Vector3f& expected, const std::string& message)
	{
		if ((actual - expected).Length() > 0.0001f || !std::isfinite(actual.LengthSqr()))
		{
			throw std::runtime_error(message + ": actual (" + std::to_string(actual.x) + ", " + std::to_string(actual.y) + ", " +
			                         std::to_string(actual.z) + "), expected (" + std::to_string(expected.x) + ", " +
			                         std::to_string(expected.y) + ", " + std::to_string(expected.z) + ")");
		}
	}

	Vector3f ExpectedForward(float yawDegrees, float pitchDegrees)
	{
		const float yaw = yawDegrees * DegreesToRadians;
		const float pitch = pitchDegrees * DegreesToRadians;
		return {std::sin(yaw) * std::cos(pitch), -std::sin(pitch), std::cos(yaw) * std::cos(pitch)};
	}

	Vector3f ExpectedRight(float yawDegrees)
	{
		const float yaw = yawDegrees * DegreesToRadians;
		return {std::cos(yaw), 0, -std::sin(yaw)};
	}

	// An authored, roll-free orientation independent of the camera-controls composition.
	Quaternion<float> AuthoredRotation(float yawDegrees, float pitchDegrees)
	{
		const float yaw = yawDegrees * DegreesToRadians * 0.5f;
		const float pitch = pitchDegrees * DegreesToRadians * 0.5f;
		return {std::cos(yaw) * std::cos(pitch), std::cos(yaw) * std::sin(pitch), std::sin(yaw) * std::cos(pitch),
		        -std::sin(yaw) * std::sin(pitch)};
	}

	class CameraFixture
	{
	public:
		GameInput Input;
		std::unique_ptr<World> Session = WorldAccess::Create(&Input);
		Actor* Camera = nullptr;

		explicit CameraFixture(const Quaternion<float>& authored = {}, float parentYaw = 0, float parentRoll = 0)
		{
			auto* parent = Session->SpawnActor("Camera parent");
			parent->GetTransform().SetLocalRotationRadians(parentYaw * DegreesToRadians, 0, parentRoll * DegreesToRadians);
			Camera = Session->SpawnActor("Controlled camera");
			Camera->GetTransform().SetLocalRotation(authored);
			if (!Camera->SetParent(parent, ReparentMode::KeepLocal))
			{
				throw std::runtime_error("Camera fixture parenting failed");
			}
			Camera->AddComponent<CameraControlsComponent>();
			SceneDiagnostics diagnostics;
			if (!WorldAccess::Prepare(*Session, diagnostics))
			{
				throw std::runtime_error("Camera fixture preparation failed");
			}
			WorldAccess::Activate(*Session);
		}

		void Look(float yawDeltaDegrees, float pitchDeltaDegrees, bool active = true)
		{
			Input = {};
			Input.MouseLookActive = active;
			Input.MouseDeltaX = yawDeltaDegrees * DegreesToRadians / LookSensitivity;
			Input.MouseDeltaY = pitchDeltaDegrees * DegreesToRadians / LookSensitivity;
			WorldAccess::Update(*Session, 1.0f / 60.0f);
		}

		void ExpectLocal(float yawDegrees, float pitchDegrees, const std::string& label) const
		{
			const auto& transform = Camera->GetTransform();
			ExpectVector(transform.GetLocalForward(), ExpectedForward(yawDegrees, pitchDegrees), label + " forward");
			ExpectVector(transform.GetLocalRight(), ExpectedRight(yawDegrees), label + " right (pitch must not introduce roll)");
		}
	};

	void CardinalYawPitch()
	{
		for (float yaw : {0.0f, 90.0f, 180.0f, -90.0f})
		{
			CameraFixture fixture;
			fixture.Look(yaw, 0);
			fixture.Look(0, 30);
			fixture.ExpectLocal(yaw, 30, "Yaw " + std::to_string(yaw) + " then pitch 30");
		}
	}

	void MixedInputAndClamp()
	{
		CameraFixture fixture;
		fixture.Look(35, 20);
		fixture.Look(-15, 10);
		fixture.Look(5, -7);
		fixture.ExpectLocal(25, 23, "Mixed mouse input");
		fixture.Look(50, 50, false);
		fixture.ExpectLocal(25, 23, "Inactive mouse look");
		fixture.Look(0, 250);
		fixture.ExpectLocal(25, 89, "Positive pitch clamp");
		fixture.Look(0, -500);
		fixture.ExpectLocal(25, -89, "Negative pitch clamp");
	}

	void StartupAimAndParent()
	{
		CameraFixture authored(AuthoredRotation(40, -25));
		authored.ExpectLocal(40, -25, "Authored aim after BeginPlay");
		authored.Look(0, 0, false);
		authored.ExpectLocal(40, -25, "Authored aim without input");

		CameraFixture parented({}, 60);
		parented.Look(90, 30);
		parented.ExpectLocal(90, 30, "Parented local controls");
		const auto world = parented.Camera->GetTransform().GetWorldMatrix();
		ExpectVector({world(3, 1), world(3, 2), world(3, 3)}, ExpectedForward(150, 30), "Rotated-parent world forward");
		ExpectVector({world(1, 1), world(1, 2), world(1, 3)}, ExpectedRight(150), "Rotated-parent world right");

		CameraFixture tilted({}, 0, 90);
		tilted.Look(90, 30);
		tilted.ExpectLocal(90, 30, "Tilted-parent local controls");
		const auto tiltedWorld = tilted.Camera->GetTransform().GetWorldMatrix();
		ExpectVector({tiltedWorld(3, 1), tiltedWorld(3, 2), tiltedWorld(3, 3)}, {0.5f, 0.8660254f, 0}, "Tilted-parent world forward");
		ExpectVector({tiltedWorld(1, 1), tiltedWorld(1, 2), tiltedWorld(1, 3)}, {0, 0, -1}, "Tilted-parent world right");
	}

	void LightAimShortcuts()
	{
		GameInput input;
		auto world = WorldAccess::Create(&input);
		auto* camera = world->SpawnActor("Camera");
		camera->GetTransform().SetLocalPosition({120, 230, -340});
		camera->GetTransform().SetLocalRotation(AuthoredRotation(70, -35));
		auto* directional = world->SpawnActor("Directional")->AddComponent<DirectionalLightComponent>();
		auto* point = world->SpawnActor("Point")->AddComponent<PointLightComponent>();
		auto* spot = world->SpawnActor("Spot")->AddComponent<SpotLightComponent>();
		auto* controls = world->SpawnActor("Controls")->AddComponent<LightControlsComponent>();
		controls->Camera = camera->GetRef();
		controls->Directional = directional->GetRef<DirectionalLightComponent>();
		controls->Point = point->GetRef<PointLightComponent>();
		controls->Spot = spot->GetRef<SpotLightComponent>();
		SceneDiagnostics diagnostics;
		if (!WorldAccess::Prepare(*world, diagnostics))
		{
			throw std::runtime_error("Light shortcut fixture preparation failed");
		}
		WorldAccess::Activate(*world);
		input.KeysDown[static_cast<size_t>(Keys::SHIFT)] = true;
		input.KeysPressed[static_cast<size_t>('7')] = true;
		WorldAccess::Update(*world, 1.0f / 60.0f);
		ExpectVector(directional->GetWorldDirection(), ExpectedForward(70, -35), "Shift+7 camera-aligned directional light");
		input.KeysPressed.fill(false);
		input.KeysPressed[static_cast<size_t>('9')] = true;
		WorldAccess::Update(*world, 1.0f / 60.0f);
		ExpectVector(spot->GetWorldDirection(), ExpectedForward(70, -35), "Shift+9 camera-aligned spotlight");
		ExpectVector(spot->GetWorldPosition(), camera->GetTransform().GetWorldPosition(), "Shift+9 spotlight placement");
	}
}

int RunCameraControlsTests()
{
	try
	{
		CardinalYawPitch();
		MixedInputAndClamp();
		StartupAimAndParent();
		LightAimShortcuts();
		std::cout
		    << "PASS: actual camera controls preserve yaw-local pitch, startup aim, clamp, rotated-parent composition and light aim shortcuts\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
