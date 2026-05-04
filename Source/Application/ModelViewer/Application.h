#pragma once
#include "Logger/Logger.h"

#if _DEBUG
DECLARE_LOG_CATEGORY_WITH_NAME(LogModelViewer, ModelViewer, Verbose);
#else
DECLARE_LOG_CATEGORY_WITH_NAME(LogModelViewer, ModelViewer, Warning);
#endif

#define MVLOG(Verbosity, Message, ...) LOG(LogModelViewer, Verbosity, Message, ##__VA_ARGS__)

struct ApplicationData
{
	// Application State data goes here for easy access.
};

inline ApplicationData Application;