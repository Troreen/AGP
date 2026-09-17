#pragma once
#include "Logger/Logger.h"

#if _DEBUG
DECLARE_LOG_CATEGORY_WITH_NAME(LogGame, Game, Verbose);
#else
DECLARE_LOG_CATEGORY_WITH_NAME(LogGame, Game, Warning);
#endif

#define GAMELOG(Verbosity, Message, ...) LOG(LogGame, Verbosity, Message, ##__VA_ARGS__)
