#pragma once

#include "AssetHandling/Asset.h"
#include "Logger/Logger.h"
#include "World/Actor.h"
#include "World/Component.h"

#include <CommonUtilities/CommonUtilities.h>

#include <filesystem>
#include <functional>
#include <vector>
#include <array>
#include <unordered_map>
#include <memory>
#include <string>
#include <cassert>

#if _DEBUG
DECLARE_LOG_CATEGORY_WITH_NAME(LogGameFramework, GameFramework, Verbose);
#else
DECLARE_LOG_CATEGORY_WITH_NAME(LogGameFramework, GameFramework, Warning);
#endif

#define GFLOG(Verbosity, Message, ...) LOG(LogGameFramework, Verbosity, Message, ##__VA_ARGS__)
