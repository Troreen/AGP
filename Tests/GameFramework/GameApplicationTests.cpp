#include "InputFixture.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <crtdbg.h>

#include "EnumKeyCode.h"
#include "GraphicsEngine/GraphicsEngine.h"

#include <iostream>
#include <stdexcept>
#include <string>

void Check(bool value, const char* message)
{
	if (!value)
	{
		throw std::runtime_error(message);
	}
}

int RunCameraControlsTests();

int RunRenderPassControlsTest()
{
	GraphicsEngine& graphics = GraphicsEngine::Get();
	Check(std::string(graphics.GetRenderPassName()) == "Lit", "Render-pass test did not start on Lit");

	InputFixture fixture;
	auto& input = fixture.Input;
	input.BindActionToInputCode("PreviousRenderPass", EKeyCode::F5);
	input.BindActionToInputCode("NextRenderPass", EKeyCode::F6);
	const unsigned previous = input.AddEventListener("PreviousRenderPass", [&graphics](const CommonUtilities::InputEvent& event)
	{
		if (event.inputData.isPressed) graphics.SelectPreviousRenderPass();
	});
	const unsigned next = input.AddEventListener("NextRenderPass", [&graphics](const CommonUtilities::InputEvent& event)
	{
		if (event.inputData.isPressed) graphics.SelectNextRenderPass();
	});
	fixture.Key(EKeyCode::F5, true); input.Update();
	Check(std::string(graphics.GetRenderPassName()) == "Shadows (Directional)", "F5 wrap failed");
	fixture.Key(EKeyCode::F5, false); fixture.Key(EKeyCode::F6, true); input.Update();
	Check(std::string(graphics.GetRenderPassName()) == "Lit", "F6 advance failed");
	fixture.Key(EKeyCode::F6, false); input.Update(); fixture.Key(EKeyCode::F6, true); input.Update();
	Check(std::string(graphics.GetRenderPassName()) == "Albedo (sRGB)", "F6 next failed");
	fixture.Key(EKeyCode::F6, false); fixture.Key(EKeyCode::F5, true); input.Update();
	Check(std::string(graphics.GetRenderPassName()) == "Lit", "F5 previous failed");
	input.RemoveEventListener(previous);
	input.RemoveEventListener(next);

	std::cout << "PASS: F5 previous and F6 next render-pass controls\n";
	return 0;
}

int main(int argc, char** argv)
{
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

	const std::string scenario = argc > 1 ? argv[1] : "camera-controls";
	try
	{
		if (scenario == "camera-controls")
		{
			return RunCameraControlsTests();
		}
		if (scenario == "render-pass-controls")
		{
			return RunRenderPassControlsTest();
		}
		throw std::runtime_error("Unknown runtime test scenario: " + scenario);
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
