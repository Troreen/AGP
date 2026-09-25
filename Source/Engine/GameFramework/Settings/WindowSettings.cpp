#include "WindowSettings.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace
{
	constexpr std::array<WindowResolution, 4> CommonWindowedResolutions{{
		{1280, 720}, {1920, 1080}, {2560, 1440}, {3840, 2160}
	}};
}

WindowSettings::Monitor WindowSettings::GetMonitor(HWND aWindow)
{
	const HMONITOR monitor = aWindow
		? MonitorFromWindow(aWindow, MONITOR_DEFAULTTOPRIMARY)
		: MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO monitorInfo{sizeof(MONITORINFO)};
	if (!GetMonitorInfoW(monitor, &monitorInfo))
	{
		throw std::runtime_error("Could not query the display monitor");
	}
	const RECT bounds = monitorInfo.rcMonitor;
	return {monitor, bounds, static_cast<unsigned>(bounds.right - bounds.left),
		static_cast<unsigned>(bounds.bottom - bounds.top)};
}

std::vector<WindowResolution> WindowSettings::GetAvailableResolutions(const Monitor& aMonitor)
{
	std::vector<WindowResolution> resolutions;
	for (const WindowResolution resolution : CommonWindowedResolutions)
	{
		if (resolution.Width <= aMonitor.Width && resolution.Height <= aMonitor.Height)
		{
			resolutions.push_back(resolution);
		}
	}
	if (resolutions.empty())
	{
		resolutions.push_back({aMonitor.Width, aMonitor.Height});
	}
	return resolutions;
}

ApplicationSettings WindowSettings::CapResolution(ApplicationSettings aSettings, const Monitor& aMonitor)
{
	if (aSettings.WindowedWidth > aMonitor.Width || aSettings.WindowedHeight > aMonitor.Height)
	{
		const WindowResolution resolution = GetAvailableResolutions(aMonitor).back();
		aSettings.WindowedWidth = resolution.Width;
		aSettings.WindowedHeight = resolution.Height;
	}
	return aSettings;
}

DWORD WindowSettings::GetWindowStyle(WindowMode aMode)
{
	return aMode == WindowMode::Borderless ? WS_POPUP : (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX);
}

void WindowSettings::RememberMonitor(HWND aWindow)
{
	const Monitor monitor = GetMonitor(aWindow);
	myCurrentMonitor = monitor.Handle;
	myCurrentMonitorBounds = monitor.Bounds;
}

bool WindowSettings::MonitorChanged(HWND aWindow) const
{
	const Monitor monitor = GetMonitor(aWindow);
	return monitor.Handle != myCurrentMonitor || monitor.Bounds.left != myCurrentMonitorBounds.left ||
		monitor.Bounds.top != myCurrentMonitorBounds.top || monitor.Bounds.right != myCurrentMonitorBounds.right ||
		monitor.Bounds.bottom != myCurrentMonitorBounds.bottom;
}

ApplicationSettings WindowSettings::Apply(HWND aWindow, const ApplicationSettings& aRequestedSettings, WindowMode aPreviousMode)
{
	const Monitor monitor = GetMonitor(aWindow);
	const ApplicationSettings effective = CapResolution(aRequestedSettings, monitor);
	if (!aWindow)
	{
		return effective;
	}

	const bool borderless = effective.Mode == WindowMode::Borderless;
	const DWORD style = GetWindowStyle(effective.Mode);
	RECT bounds{0, 0, static_cast<LONG>(effective.WindowedWidth), static_cast<LONG>(effective.WindowedHeight)};
	if (borderless)
	{
		bounds = monitor.Bounds;
	}
	else if (!AdjustWindowRectEx(&bounds, style, FALSE, 0))
	{
		throw std::runtime_error("Could not calculate the window size");
	}

	RECT previousBounds{};
	if (!GetWindowRect(aWindow, &previousBounds))
	{
		throw std::runtime_error("Could not query the window position");
	}
	if (aPreviousMode == WindowMode::Windowed && borderless)
	{
		myLastWindowedPosition = {previousBounds.left, previousBounds.top};
		myHasLastWindowedPosition = true;
	}
	const LONG requestedX = aPreviousMode == WindowMode::Borderless && myHasLastWindowedPosition
		? myLastWindowedPosition.x : previousBounds.left;
	const LONG requestedY = aPreviousMode == WindowMode::Borderless && myHasLastWindowedPosition
		? myLastWindowedPosition.y : previousBounds.top;
	const LONG x = borderless ? monitor.Bounds.left
		: (std::max)(monitor.Bounds.left, (std::min)(requestedX, monitor.Bounds.right - 100));
	const LONG y = borderless ? monitor.Bounds.top
		: (std::max)(monitor.Bounds.top, (std::min)(requestedY, monitor.Bounds.bottom - 40));

	SetLastError(0);
	if (!SetWindowLongPtrW(aWindow, GWL_STYLE, static_cast<LONG_PTR>(style)) && GetLastError() != 0)
	{
		throw std::runtime_error("Could not set the window mode");
	}
	if (!SetWindowPos(aWindow, nullptr, x, y, bounds.right - bounds.left, bounds.bottom - bounds.top,
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED))
	{
		throw std::runtime_error("Could not change the window resolution or mode");
	}
	RememberMonitor(aWindow);
	if (!borderless)
	{
		myLastWindowedPosition = {x, y};
		myHasLastWindowedPosition = true;
	}
	return effective;
}
