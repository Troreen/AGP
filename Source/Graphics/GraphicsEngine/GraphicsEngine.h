#pragma once

typedef struct HWND__ *HWND;

class GraphicsEngine
{
public:

	static GraphicsEngine& Get();

	bool Initialize(HWND aWindowHandle);
	void Render();

private:

	GraphicsEngine();
	~GraphicsEngine();

};