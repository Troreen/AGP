#pragma once
#include <Windows.h>

namespace StartupOptions
{
	// Read once during initialization, before worker threads are started.
	inline bool Disabled(const wchar_t* aName)
	{
		wchar_t value[2] = {};
		return GetEnvironmentVariableW(aName, value, 2) == 1 && value[0] == L'1';
	}
}
