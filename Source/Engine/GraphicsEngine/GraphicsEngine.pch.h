// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "GraphicsEngine.h"
#include "Logger/Logger.h"
#include "d3d11_1.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "Windows.h"

#if _DEBUG
DECLARE_LOG_CATEGORY_WITH_NAME(LogGraphicsEngine, GraphicsEngine, Verbose);
#else
DECLARE_LOG_CATEGORY_WITH_NAME(LogGraphicsEngine, GraphicsEngine, Warning);
#endif

#define GELOG(Verbosity, Message, ...) LOG(LogGraphicsEngine, Verbosity, Message, ##__VA_ARGS__)

#endif //PCH_H
