#include "InputFixture.h"
#include "CameraControlsComponent.h"
#include "SpinComponent.h"
#include "GameFramework/World/World.h"
#include "GameFramework/Components/SceneComponent.h"
#include "EnumKeyCode.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
	using CommonUtilities::Vector3f;
	constexpr float DegreesToRadians = 0.017453292519943295f;
	constexpr float LookSensitivity = 0.0025f;

	void Expect(bool condition, const std::string& message)
	{
		if (!condition)
		{
			throw std::runtime_error(message);
		}
	}

	void ExpectVector(const Vector3f& actual, const Vector3f& expected, const std::string& message)
	{
		if ((actual - expected).Length() > 0.004f || !std::isfinite(actual.LengthSqr()))
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
	Vector3f AuthoredRotation(float yawDegrees, float pitchDegrees)
	{
		return {yawDegrees, pitchDegrees, 0};
	}

	class CameraFixture
	{
	public:
		InputFixture Native;
		std::unique_ptr<World> Session = std::make_unique<World>();
		Actor* Camera = nullptr;

		explicit CameraFixture(const Vector3f& authored = {})
		{
			Camera = Session->SpawnActor("Controlled camera");
			Camera->GetTransform().SetLocalRotationDegrees(authored);
			Camera->AddComponent<CameraControlsComponent>();
			Session->BeginPlay();
		}

		void Look(float yawDeltaDegrees, float pitchDeltaDegrees, bool active = true)
		{
			Native.Key(EKeyCode::MOUSERBUTTON, active);
			Native.Move(static_cast<int>(std::lround(yawDeltaDegrees * DegreesToRadians / LookSensitivity)), static_cast<int>(std::lround(pitchDeltaDegrees * DegreesToRadians / LookSensitivity)));
			Native.Input.Update();
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

	void FrameTimeSpin()
	{
		InputFixture native;
		auto& input = native.Input;
		auto world = std::make_unique<World>();
		auto* chest = world->SpawnActor("Chest");
		chest->AddComponent<SpinComponent>();
		auto* child = chest->AddComponent<SceneComponent>("Child");
		child->GetTransform().SetLocalPosition({10.0f, 0.0f, 0.0f});
		auto* childSpin = chest->AddComponent<SpinComponent>("ChildSpin");
		childSpin->SetTargetComponentName("Child");
		const CommonUtilities::Vector3f initialChildWorldPosition = child->GetWorldPosition();
		world->BeginPlay();
		world->Update(.002f);
		ExpectVector(chest->GetTransform().GetLocalForward(), ExpectedForward(.05f, 0), "Spin advances on a short frame");
		ExpectVector(child->GetTransform().GetLocalForward(), ExpectedForward(.05f, 0), "Child advances its own local spin");
		Expect((child->GetWorldPosition() - initialChildWorldPosition).LengthSqr() > 0.0f,
		       "Parent spin did not move the offset child around its orbit");
		native.Key(EKeyCode::R, true);
		input.Update();
		world->Update(.1f);
		ExpectVector(chest->GetTransform().GetLocalForward(), ExpectedForward(2.55f, 0), "R does not pause automatic spin");
		ExpectVector(child->GetTransform().GetLocalForward(), ExpectedForward(2.55f, 0), "Child spin continues without input controls");

	}
}

int RunCameraControlsTests()
{
	try
	{
		CardinalYawPitch();
		MixedInputAndClamp();
		StartupAim();
		FrameTimeSpin();
		std::cout << "PASS: camera controls and automatic frame-time chest spin\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
