#pragma once
#include <wrl.h>

#include "GraphicsEngine/RHI/RHIStructs.h"

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11ShaderResourceView;

class Texture
{
	friend class RenderHardwareInterface;
	friend class GraphicsCommandList;
	friend class GraphicsEngine;

public:
	Texture();
	~Texture();

	std::string_view GetName() const { return myName; }

	float GetWidth() const { return myViewport.Width; }
	float GetHeight() const { return myViewport.Height; }

private:

	std::string myName;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> myRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> myDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mySRV;
	Viewport myViewport;
};
