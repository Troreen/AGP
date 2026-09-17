#include "GameComponents.h"
#include "GameFramework/World/World.h"
#include "GameFramework/Components/LightComponent.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
	using CommonUtilities::Quaternion;
	using CommonUtilities::Vector3f;
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
		std::unique_ptr<World> Session = std::make_unique<World>(&Input);
		Actor* Camera = nullptr;

		explicit CameraFixture(const Quaternion<float>& authored = {})
		{
			Camera = Session->SpawnActor("Controlled camera");
			Camera->GetTransform().SetLocalRotation(authored);
			Camera->AddComponent<CameraControlsComponent>();
			Session->BeginPlay();
		}

		void Look(float yawDeltaDegrees, float pitchDeltaDegrees, bool active = true)
		{
			Input = {};
			Input.MouseLookActive = active;
			Input.MouseDeltaX = yawDeltaDegrees * DegreesToRadians / LookSensitivity;
			Input.MouseDeltaY = pitchDeltaDegrees * DegreesToRadians / LookSensitivity;
			Session->Update(1.0f / 60.0f);
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

	void StartupAim()
	{
		CameraFixture authored(AuthoredRotation(40, -25));
		authored.ExpectLocal(40, -25, "Authored aim after BeginPlay");
		authored.Look(0, 0, false);
		authored.ExpectLocal(40, -25, "Authored aim without input");
	}

	void LightAimShortcuts()
	{
		GameInput input;
		auto world = std::make_unique<World>(&input);
		auto* camera = world->SpawnActor("Camera");
		camera->GetTransform().SetLocalPosition({120, 230, -340});
		camera->GetTransform().SetLocalRotation(AuthoredRotation(70, -35));
		camera->AddComponent<CameraControlsComponent>();
		auto* directional = world->SpawnActor("Directional")->AddComponent<DirectionalLightComponent>();
		auto* point = world->SpawnActor("Point")->AddComponent<PointLightComponent>();
		auto* spot = world->SpawnActor("Spot")->AddComponent<SpotLightComponent>();
		auto* controls = world->SpawnActor("Controls")->AddComponent<LightControlsComponent>();
		controls->CameraName = "Camera";
		controls->DirectionalName = "Directional";
		controls->PointName = "Point";
		controls->SpotName = "Spot";
		world->BeginPlay();
		input.MouseLookActive = true;
		input.MouseDeltaX = 10 * DegreesToRadians / LookSensitivity;
		input.KeysDown[static_cast<size_t>(Keys::SHIFT)] = true;
		input.KeysPressed[static_cast<size_t>('7')] = true;
		world->Update(1.0f / 60.0f);
		ExpectVector(directional->GetWorldDirection(), ExpectedForward(80, -35), "Shift+7 uses camera movement from the same Update");
		input.MouseDeltaX = 0;
		input.KeysPressed.fill(false);
		input.KeysPressed[static_cast<size_t>('9')] = true;
		world->Update(1.0f / 60.0f);
		ExpectVector(spot->GetWorldDirection(), ExpectedForward(80, -35), "Shift+9 camera-aligned spotlight");
		ExpectVector(spot->GetWorldPosition(), camera->GetTransform().GetWorldPosition(), "Shift+9 spotlight placement");
	}

	void FrameTimeSpin()
	{
		GameInput input;
		auto world = std::make_unique<World>(&input);
		auto* chest = world->SpawnActor("Chest");
		chest->AddComponent<SpinComponent>();
		world->BeginPlay();
		world->Update(.002f);
		ExpectVector(chest->GetTransform().GetLocalForward(), ExpectedForward(.05f, 0), "Spin advances on a short frame");
		input.KeysPressed[static_cast<size_t>(Keys::R)] = true;
		world->Update(0);
		input.KeysPressed.fill(false);
		input.KeysDown[static_cast<size_t>(Keys::R)] = true;
		world->Update(.2f);
		ExpectVector(chest->GetTransform().GetLocalForward(), ExpectedForward(.05f, 0), "Pressed R pauses; held R does not toggle again");
		input.KeysPressed[static_cast<size_t>(Keys::R)] = true;
		world->Update(.1f);
		ExpectVector(chest->GetTransform().GetLocalForward(), ExpectedForward(2.55f, 0), "Second R press resumes frame-time spin");
	}
}

int RunCameraControlsTests()
{
	try
	{
		CardinalYawPitch();
		MixedInputAndClamp();
		StartupAim();
		LightAimShortcuts();
		FrameTimeSpin();
		std::cout << "PASS: camera controls, same-frame light aiming and single-update chest spin\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
