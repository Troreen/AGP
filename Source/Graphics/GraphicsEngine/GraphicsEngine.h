#pragma once

#include <string>
#include <unordered_map>
#include <wrl.h>

#include "RHI/RenderHardwareInterface.h"
#include "Objects/Texture.h"


class GraphicsEngine
{
public:

	static GraphicsEngine& Get();

	bool Initialize(HWND aWindowHandle);
	void Render();

private:

	GraphicsEngine();
	~GraphicsEngine();

	RenderHardwareInterface myRHI;
	Texture myBackBuffer;

};
