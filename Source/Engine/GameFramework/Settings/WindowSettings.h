#pragma once

#include "EngineSettings.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <vector>

// Monitor queries and changes to an already created game window.
class WindowSettings final
{
public:
	struct Monitor
	{
		HMONITOR Handle;
		RECT Bounds;
		unsigned Width;
		unsigned Height;
	};

	static Monitor GetMonitor(HWND aWindow);
	static std::vector<WindowResolution> GetAvailableResolutions(const Monitor& aMonitor);
	static ApplicationSettings CapResolution(ApplicationSettings aSettings, const Monitor& aMonitor);
	static DWORD GetWindowStyle(WindowMode aMode);

	void RememberMonitor(HWND aWindow);
	bool MonitorChanged(HWND aWindow) const;
	ApplicationSettings Apply(HWND aWindow, const ApplicationSettings& aRequestedSettings, WindowMode aPreviousMode);

private:
	HMONITOR myCurrentMonitor = nullptr;
	RECT myCurrentMonitorBounds{};
	POINT myLastWindowedPosition{};
	bool myHasLastWindowedPosition = false;
};
