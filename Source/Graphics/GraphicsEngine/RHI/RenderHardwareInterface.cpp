#include "GraphicsEngine.pch.h"
#include "RenderHardwareInterface.h"

#include "StringHelpers.h"
#include "GraphicsEngine/Objects/Texture.h"

using namespace Microsoft::WRL;

DECLARE_LOG_CATEGORY_WITH_NAME(RhiLog, RHI, Verbose);

DEFINE_LOG_CATEGORY(RhiLog);

RenderHardwareInterface::RenderHardwareInterface() = default;

RenderHardwareInterface::~RenderHardwareInterface() = default;

bool RenderHardwareInterface::Initialize(HWND aWindowHandle, bool aEnableDebug, Texture& outBackBuffer)
{
	HRESULT result = E_FAIL;

	ComPtr<IDXGIFactory> dxFactory;
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), &dxFactory);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create DXGI Factory!");
		return false;
	}

	LOG(RhiLog, Log, "Initializing RHI...");

	ComPtr<IDXGIAdapter> tempAdapter;
	std::vector<ComPtr<IDXGIAdapter>> adapters;
	while (dxFactory->EnumAdapters(static_cast<unsigned>(adapters.size()), &tempAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		adapters.emplace_back(tempAdapter);
	}

	ComPtr<IDXGIAdapter> selectedAdapter;
	DXGI_ADAPTER_DESC selectedAdapterDesc = {};
	for (const auto& adapter : adapters)
	{
		DXGI_ADAPTER_DESC desc = {};
		adapter->GetDesc(&desc);
		if (selectedAdapterDesc.DedicatedVideoMemory < desc.DedicatedVideoMemory)
		{
			selectedAdapter = adapter;
			selectedAdapterDesc = desc;
		}
	}

	const wchar_t* wideAdapterName = selectedAdapterDesc.Description;
	const std::string adapterName = str::wide_to_utf8(wideAdapterName);
	LOG(RhiLog, Log, "Selected adapter: {}", adapterName);


	result = D3D11CreateDevice(
		selectedAdapter.Get(),
		D3D_DRIVER_TYPE_UNKNOWN,
		NULL,
		aEnableDebug ? D3D11_CREATE_DEVICE_DEBUG : 0,
		NULL, 
		0,
		D3D11_SDK_VERSION,
		&myDevice,
		NULL,
		&myContext
	);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create D3D11 Device!");
		return false;
	}


	const std::string dxAdapterName = "Adapter";
	myDevice->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<unsigned>(dxAdapterName.size() * sizeof(char)), dxAdapterName.data());

	const std::string contextName = "Context";
	myContext->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<unsigned>(contextName.size() * sizeof(char)), contextName.data());

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.OutputWindow = aWindowHandle;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = true;

	result = dxFactory->CreateSwapChain(myDevice.Get(), &swapChainDesc, &mySwapChain);

	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create Swap Chain!");
		return false;
	}
	
	const std::string swapChainName = "SwapChain";
	mySwapChain->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<unsigned>(swapChainName.size() * sizeof(char)), swapChainName.data());

	ComPtr<ID3D11Texture2D> backBufferTexture;
	result = mySwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBufferTexture);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to fetch Back Buffer!");
		return false;
	}

	SetObjectName(backBufferTexture, "BackBuffer_T2D");

	result = myDevice->CreateRenderTargetView(backBufferTexture.Get(), nullptr, &outBackBuffer.myRTV);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create Render Target View!");
		return false;
	}

	SetObjectName(outBackBuffer.myRTV, "BackBufferRTV");

	LOG(RhiLog, Log, "RHI Started!");
	return true;
}

void RenderHardwareInterface::Present() const
{
	mySwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
}

void RenderHardwareInterface::ClearRenderTarget(const Texture& aTarget) const
{
	float clearColor[4] = { 0, 1, 0, 0 };	 
	myContext->ClearRenderTargetView(aTarget.myRTV.Get(), clearColor);
}

void RenderHardwareInterface::SetObjectName(const Microsoft::WRL::ComPtr<ID3D11DeviceChild>& aObject, std::string_view aName) const
{
	if (aObject)
	{
		aObject->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<unsigned>(aName.length() * sizeof(char)), aName.data());
	}
}
