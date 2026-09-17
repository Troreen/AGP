#pragma once

#include "Logger/Logger.h"

#if _DEBUG
DECLARE_LOG_CATEGORY_WITH_NAME(LogGameFramework, GameFramework, Verbose);
#else
DECLARE_LOG_CATEGORY_WITH_NAME(LogGameFramework, GameFramework, Warning);
#endif

#define GFLOG(Verbosity, Message, ...) LOG(LogGameFramework, Verbosity, Message, ##__VA_ARGS__)
