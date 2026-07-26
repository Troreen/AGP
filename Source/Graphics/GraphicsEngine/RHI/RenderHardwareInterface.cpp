#include "GraphicsEngine.pch.h"

#if defined(AGP_RENDERER_HOST_TEST_FAULTS)
#include "GraphicsEngine/RendererHostFaultInjection.h"
#endif
#include "RenderHardwareInterface.h"

#include <d3dcompiler.h>

#include "StringHelpers.h"
#include "GraphicsEngine/Objects/Texture.h"
#include "GraphicsEngine/Objects/Buffer.h"
#include "GraphicsEngine/Objects/Shader.h"
#include "GraphicsEngine/Objects/Vertex.h"
#include "GraphicsCommandList.h"
#include "PipelineStateObject.h"
#include "Ensure.h"
#include "GraphicsEngine/InterOp/DDSTextureLoader11.h"
#include "FrameBenchmark.h"

using namespace Microsoft::WRL;

DECLARE_LOG_CATEGORY_WITH_NAME(RhiLog, RHI, Verbose);

DEFINE_LOG_CATEGORY(RhiLog);

RenderHardwareInterface::RenderHardwareInterface() = default;

RenderHardwareInterface::~RenderHardwareInterface() = default;

bool RenderHardwareInterface::Initialize(HWND aWindowHandle, bool aEnableDebug, Texture& outBackBuffer, Texture& outDepthStencil)
{
	myWindowHandle = aWindowHandle;
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
	myFrameBenchmark = FrameBenchmarkSession::CreateFromEnvironment();
	if (myFrameBenchmark)
	{
		RECT clientRect = {};
		GetClientRect(aWindowHandle, &clientRect);
		myFrameBenchmark->SetRuntimeInfo({
			.Adapter = adapterName,
			.Width = static_cast<unsigned>((std::max)(LONG{ 0 }, clientRect.right - clientRect.left)),
			.Height = static_cast<unsigned>((std::max)(LONG{ 0 }, clientRect.bottom - clientRect.top))
		});
		LOG(RhiLog, Log, "Frame benchmark enabled.");
	}


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

	if (FAILED(result) && aEnableDebug)
	{
		LOG(RhiLog, Warning, "Failed to create D3D11 debug device. Retrying without the debug layer.");
		result = D3D11CreateDevice(
			selectedAdapter.Get(),
			D3D_DRIVER_TYPE_UNKNOWN,
			NULL,
			0,
			NULL,
			0,
			D3D11_SDK_VERSION,
			&myDevice,
			NULL,
			&myContext
		);
	}

	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create D3D11 Device!");
		return false;
	}

	ComPtr<ID3D11Debug> deviceDebug;
	myDevice.As(&deviceDebug);
	if (deviceDebug)
	{
		ComPtr<ID3D11InfoQueue> infoQueue;
		deviceDebug->QueryInterface(IID_PPV_ARGS(&infoQueue));

		D3D11_MESSAGE_ID mask[] =
		{
			D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS
		};

		D3D11_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(mask);
		filter.DenyList.pIDList = mask;
		infoQueue->AddStorageFilterEntries(&filter);
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

	CommonUtilities::Vector2u clientSize = GetClientSize();
	Viewport viewport = { 0, 0, static_cast<float>(clientSize.x), static_cast<float>(clientSize.y), 0, 1 };
	outBackBuffer.myViewport = viewport;

	D3D11_TEXTURE2D_DESC depthDesc = {};
	depthDesc.Width = clientSize.x;
	depthDesc.Height = clientSize.y;
	depthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthDesc.CPUAccessFlags = 0;
	depthDesc.MiscFlags = 0;
	depthDesc.ArraySize = 1;
	depthDesc.MipLevels = 1;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;

	ComPtr<ID3D11Texture2D> depthTexture;
	result = myDevice->CreateTexture2D(&depthDesc, nullptr, &depthTexture);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create Depth Stencil!");
		return false;
	}

	SetObjectName(depthTexture, "DepthStencil_T2D");

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	result = myDevice->CreateDepthStencilView(depthTexture.Get(), &dsvDesc, &outDepthStencil.myDSV);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create Depth Stencil View!");
		return false;
	}

	SetObjectName(outDepthStencil.myDSV, "DepthStencil_DSV");

	outDepthStencil.myViewport = viewport;

	LOG(RhiLog, Log, "RHI Started!");
	return true;
}

CommonUtilities::Vector2u RenderHardwareInterface::GetClientSize() const
{
	RECT clientRect = {};
	GetClientRect(myWindowHandle, &clientRect);
	const unsigned width = clientRect.right - clientRect.left;
	const unsigned height = clientRect.bottom - clientRect.top;

	return { width, height };
}

RenderHardwareInterface::ResizeBackBufferResult RenderHardwareInterface::ResizeBackBuffer(
	unsigned aWidth,
	unsigned aHeight,
	Texture& outBackBuffer,
	Texture& outDepthStencil)
{
	if (!myDevice || !myContext || !mySwapChain || aWidth == 0 || aHeight == 0)
	{
		return ResizeBackBufferResult::FailedTargetsPreserved;
	}

	// Depth resources do not depend on the swapchain. Build them before releasing
	// the live backbuffer so an allocation failure leaves the current frame targets
	// completely intact.
	Texture candidateDepthStencil;
	D3D11_TEXTURE2D_DESC depthDesc = {};
	depthDesc.Width = aWidth;
	depthDesc.Height = aHeight;
	depthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthDesc.ArraySize = 1;
	depthDesc.MipLevels = 1;
	depthDesc.SampleDesc.Count = 1;
	ComPtr<ID3D11Texture2D> depthTexture;
	HRESULT result = myDevice->CreateTexture2D(&depthDesc, nullptr, &depthTexture);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create a candidate resized depth texture.");
		return ResizeBackBufferResult::FailedTargetsPreserved;
	}
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	result = myDevice->CreateDepthStencilView(depthTexture.Get(), &dsvDesc, &candidateDepthStencil.myDSV);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create a candidate resized depth-stencil view.");
		return ResizeBackBufferResult::FailedTargetsPreserved;
	}
	SetObjectName(depthTexture, "DepthStencil_T2D");
	SetObjectName(candidateDepthStencil.myDSV, "DepthStencil_DSV");

	myContext->OMSetRenderTargets(0, nullptr, nullptr);
	outBackBuffer.myRTV.Reset();
	outBackBuffer.myResource.Reset();
	outBackBuffer.mySRV.Reset();
	myContext->Flush();

	result = mySwapChain->ResizeBuffers(0, aWidth, aHeight, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to resize swapchain buffers to {}x{}.", aWidth, aHeight);
		ComPtr<ID3D11Texture2D> restoredBackBufferTexture;
		if (SUCCEEDED(mySwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &restoredBackBufferTexture))
			&& SUCCEEDED(myDevice->CreateRenderTargetView(restoredBackBufferTexture.Get(), nullptr, &outBackBuffer.myRTV)))
		{
			SetObjectName(restoredBackBufferTexture, "BackBuffer_T2D");
			SetObjectName(outBackBuffer.myRTV, "BackBufferRTV");
			return ResizeBackBufferResult::FailedTargetsPreserved;
		}
		return ResizeBackBufferResult::FailedTargetsUnavailable;
	}

#if defined(AGP_RENDERER_HOST_TEST_FAULTS)
	if (AGP::Testing::ConsumeRendererHostFault(AGP::Testing::RendererHostFault::AfterSwapchainResize))
	{
		LOG(RhiLog, Warning, "Injected renderer-host resize failure after swapchain resize.");
		return ResizeBackBufferResult::FailedTargetsUnavailable;
	}
#endif

	ComPtr<ID3D11Texture2D> backBufferTexture;
	result = mySwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBufferTexture);
	Texture candidateBackBuffer;
	if (FAILED(result)
		|| FAILED(myDevice->CreateRenderTargetView(backBufferTexture.Get(), nullptr, &candidateBackBuffer.myRTV)))
	{
		LOG(RhiLog, Error, "Failed to recreate the resized backbuffer view.");
		return ResizeBackBufferResult::FailedTargetsUnavailable;
	}
	SetObjectName(backBufferTexture, "BackBuffer_T2D");
	SetObjectName(candidateBackBuffer.myRTV, "BackBufferRTV");

	const Viewport viewport = { 0, 0, static_cast<float>(aWidth), static_cast<float>(aHeight), 0, 1 };
	candidateBackBuffer.myViewport = viewport;
	candidateDepthStencil.myViewport = viewport;
	outBackBuffer = candidateBackBuffer;
	outDepthStencil = candidateDepthStencil;
	return ResizeBackBufferResult::Completed;
}

bool RenderHardwareInterface::CreateVertexBuffer(std::string_view aName, const std::vector<Vertex> &aVertexList, Buffer &outBuffer) const
{
	if(aVertexList.empty())
	{
		LOG(RhiLog, Error, "Failed to create vertex buffer for {}! Vertex list is empty.", aName);
		return false;
	}

	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.ByteWidth = static_cast<unsigned>(sizeof(Vertex) * aVertexList.size());

	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = aVertexList.data();

	const HRESULT result = myDevice->CreateBuffer(&desc, &data, &outBuffer.myBuffer);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create vertex buffer for {}! Failed to create Buffer.", aName);
		return false;
	}

	std::string bufferName = std::format("{}_VX", aName);
	SetObjectName(outBuffer.myBuffer, bufferName);

	outBuffer.myName = aName;
	outBuffer.mySize = desc.ByteWidth;
	outBuffer.myStride = sizeof(Vertex);
	outBuffer.myType = BufferType::VertexBuffer;

	return true;
}

bool RenderHardwareInterface::CreateIndexBuffer(std::string_view aName, const std::vector<unsigned> &aIndexList, Buffer &outBuffer) const
{
    if(aIndexList.empty())
	{
		LOG(RhiLog, Error, "Failed to create index buffer for {}! Index list is empty.", aName);
		return false;
	}

	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.ByteWidth = static_cast<unsigned>(sizeof(unsigned) * aIndexList.size());

	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = aIndexList.data();

	const HRESULT result = myDevice->CreateBuffer(&desc, &data, &outBuffer.myBuffer);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create index buffer for {}! Failed to create Buffer.", aName);
		return false;
	}

	std::string bufferName = std::format("{}_IX", aName);
	SetObjectName(outBuffer.myBuffer, bufferName);

	outBuffer.myName = aName;
	outBuffer.mySize = desc.ByteWidth;
	outBuffer.myStride = sizeof(unsigned);
	outBuffer.myType = BufferType::IndexBuffer;

	return true;
}

bool RenderHardwareInterface::CreateConstantBuffer(std::string_view aName, size_t aSize, Buffer &outBuffer) const
{
	if (aSize > 65536)
	{
		LOG(RhiLog, Error, "Failed to create constant buffer {}! Size is greater than 64kB!", aName);
		return false;
	}

	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.ByteWidth = static_cast<unsigned>(aSize);

	const HRESULT result = myDevice->CreateBuffer(&desc, nullptr, &outBuffer.myBuffer);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create constant buffer {}!", aName);
		return false;
	}

	SetObjectName(outBuffer.myBuffer, aName);

	outBuffer.myName = aName;
	outBuffer.mySize = aSize;
	outBuffer.myStride = static_cast<unsigned>(outBuffer.mySize);
	outBuffer.myType = BufferType::ConstantBuffer;

	return true;
}

bool RenderHardwareInterface::CreateDepthStencil(std::string_view aName, unsigned aWidth, unsigned aHeight, Texture& outDepthStencil, bool aCubeMap) const
{
	ensure(!aName.empty());

	D3D11_TEXTURE2D_DESC depthDesc = {};
	depthDesc.Width = aWidth;
	depthDesc.Height = aHeight;
	depthDesc.Format = DXGI_FORMAT_R32_TYPELESS; // DXGI_FORMAT_R32G8X24_TYPLESS
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthDesc.CPUAccessFlags = 0;
	depthDesc.MiscFlags = aCubeMap ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = aCubeMap ? 6 : 1;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;

	HRESULT result = S_OK;

	ComPtr<ID3D11Texture2D> depthTexture;
	result = myDevice->CreateTexture2D(&depthDesc, nullptr, &depthTexture);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create depth stencil {}!", aName);
		return false;
	}

	const std::string textureName = std::format("{}_T2D", aName);
	SetObjectName(depthTexture, textureName);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // DXGI_FORMAT_D32_FLOAT_S8X24_UINT
	if (aCubeMap)
	{
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Texture2DArray.FirstArraySlice = 0;
		dsvDesc.Texture2DArray.ArraySize = 6;
	}
	else
	{
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	}

	result = myDevice->CreateDepthStencilView(depthTexture.Get(), &dsvDesc, &outDepthStencil.myDSV);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create depth stencil view for {}!", aName);
		return false;
	}

	const std::string dsvName = std::format("{}_DSV", aName);
	SetObjectName(outDepthStencil.myDSV, dsvName);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // DXGI_FORMAT_R32_FLOAT_X824_TYPELESS
	if (aCubeMap)
	{
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = depthDesc.MipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;
	}
	else
	{
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = depthDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
	}

	result = myDevice->CreateShaderResourceView(depthTexture.Get(), &srvDesc, &outDepthStencil.mySRV);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create shader resource view for depth stencil {}!", aName);
		return false;
	}

	const std::string srvName = std::format("{}_SRV", aName);
	SetObjectName(outDepthStencil.mySRV, srvName);

	outDepthStencil.myViewport = { 0, 0, static_cast<float>(aWidth), static_cast<float>(aHeight), 0, 1 };
	outDepthStencil.myName = aName;

	return true;
}

bool RenderHardwareInterface::CreatePipelineStateObject(const PipelineStateDescription& aDescription, PipelineStateObject &outPSO) const
{
    ensure(!aDescription.Name.empty());

	bool hasErrored = false;

	if (aDescription.VertexShader.ByteCode)
	{
		ComPtr<ID3D11VertexShader> shader;
		const HRESULT result = myDevice->CreateVertexShader(aDescription.VertexShader.ByteCode, aDescription.VertexShader.ByteCodeSize, nullptr, &shader);
		if (FAILED(result))
		{
			LOG(RhiLog, Error, "Failed to create vertex shader for the pipeline state object {}!", aDescription.Name);
			hasErrored = true;
		}
		else 
		{
			const std::string shaderName = std::format("{}_VS", aDescription.Name);
			SetObjectName(shader, shaderName);
			outPSO.myVertexShader = shader;
		}
	}

	if (aDescription.PixelShader.ByteCode)
	{
		ComPtr<ID3D11PixelShader> shader;
		const HRESULT result = myDevice->CreatePixelShader(aDescription.PixelShader.ByteCode, aDescription.PixelShader.ByteCodeSize, nullptr, &shader);
		if (FAILED(result))
		{
			LOG(RhiLog, Error, "Failed to create pixel shader for the pipeline state object {}!", aDescription.Name);
			hasErrored = true;
		}
		else 
		{
			const std::string shaderName = std::format("{}_PS", aDescription.Name);
			SetObjectName(shader, shaderName);
			outPSO.myPixelShader = shader;
		}
	}

	if (aDescription.GeometryShader.ByteCode)
	{
		ComPtr<ID3D11GeometryShader> shader;
		const HRESULT result = myDevice->CreateGeometryShader(aDescription.GeometryShader.ByteCode, aDescription.GeometryShader.ByteCodeSize, nullptr, &shader);
		if (FAILED(result))
		{
			LOG(RhiLog, Error, "Failed to create geometry shader for the pipeline state object {}!", aDescription.Name);
			hasErrored = true;
		}
		else
		{
			const std::string shaderName = std::format("{}_GS", aDescription.Name);
			SetObjectName(shader, shaderName);
			outPSO.myGeometryShader = shader;
		}
	}

	if (!aDescription.InputLayoutElements.empty())
	{
		std::vector<D3D11_INPUT_ELEMENT_DESC> elements;
		elements.reserve(aDescription.InputLayoutElements.size());

		for (const auto& desc : aDescription.InputLayoutElements)
		{
			D3D11_INPUT_ELEMENT_DESC element = {};
			element.SemanticName = desc.Semantic.c_str();
			element.SemanticIndex = desc.SemanticIndex;
			element.Format = static_cast<DXGI_FORMAT>(desc.Format);
			
			element.InputSlot = 0;
			element.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			element.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			element.InstanceDataStepRate = 0;

			elements.emplace_back(element);
		}

		ComPtr<ID3D11InputLayout> inputLayout;

		const HRESULT result = myDevice->CreateInputLayout(
			elements.data(), 
			static_cast<unsigned>(elements.size()),
			aDescription.VertexShader.ByteCode,
			aDescription.VertexShader.ByteCodeSize,
			&inputLayout 
		);
		if (FAILED(result))
		{
			LOG(RhiLog, Error, "Failed to create input layout for the pipeline state object {}!", aDescription.Name);
			hasErrored = true;
		}
		else
		{
			const std::string inputLayoutName = std::format("{}_IL", aDescription.Name);
			SetObjectName(inputLayout, inputLayoutName);
			outPSO.myInputLayout = inputLayout;
		}
	}

	outPSO.myName = aDescription.Name;
	outPSO.myTopology = aDescription.Topology;

	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = static_cast<D3D11_FILL_MODE>(aDescription.RasterizerState.FillMode);
	rasterizerDesc.CullMode = static_cast<D3D11_CULL_MODE>(aDescription.RasterizerState.CullMode);
	rasterizerDesc.FrontCounterClockwise = false;
	rasterizerDesc.DepthBias = aDescription.RasterizerState.DepthBias;
	rasterizerDesc.DepthBiasClamp = aDescription.RasterizerState.DepthBiasClamp;
	rasterizerDesc.SlopeScaledDepthBias = aDescription.RasterizerState.SlopeScaledDepthBias;
	rasterizerDesc.DepthClipEnable = aDescription.RasterizerState.DepthClipEnable;
	rasterizerDesc.ScissorEnable = false;
	rasterizerDesc.MultisampleEnable = false;
	rasterizerDesc.AntialiasedLineEnable = false;

	ComPtr<ID3D11RasterizerState> rasterizerState;
	const HRESULT rasterizerResult = myDevice->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
	if (FAILED(rasterizerResult))
	{
		LOG(RhiLog, Error, "Failed to create rasterizer state for the pipeline state object {}!", aDescription.Name);
		hasErrored = true;
	}
	else
	{
		const std::string rasterizerName = std::format("{}_RS", aDescription.Name);
		SetObjectName(rasterizerState, rasterizerName);
		outPSO.myRasterizerState = rasterizerState;
	}

	return !hasErrored;
}

bool RenderHardwareInterface::CreateCommandList(std::string_view aName, GraphicsCommandList &outCommandList) const
{
    ensure(!outCommandList.myContext);

	const HRESULT result = myDevice->CreateDeferredContext(0, &outCommandList.myContext);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create command list!");
		return false;
	}

	const std::string name = std::format("{}_CTXT", aName);
	SetObjectName(outCommandList.myContext, name);
	outCommandList.myName = aName;
	outCommandList.myContext->QueryInterface(IID_PPV_ARGS(&outCommandList.myUDA));

	return true;
}

bool RenderHardwareInterface::CreateRenderTargetTexture(std::string_view aName, unsigned aWidth, unsigned aHeight, unsigned aFormat, Texture& outTexture) const
{
	ensure(!aName.empty());
	ensure(aWidth > 0);
	ensure(aHeight > 0);

	const DXGI_FORMAT format = static_cast<DXGI_FORMAT>(aFormat);

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = aWidth;
	textureDesc.Height = aHeight;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	ComPtr<ID3D11Texture2D> texture;
	HRESULT result = myDevice->CreateTexture2D(&textureDesc, nullptr, &texture);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create render target texture {}!", aName);
		return false;
	}

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	ComPtr<ID3D11RenderTargetView> rtv;
	result = myDevice->CreateRenderTargetView(texture.Get(), &rtvDesc, &rtv);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create render target view for texture {}!", aName);
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	ComPtr<ID3D11ShaderResourceView> srv;
	result = myDevice->CreateShaderResourceView(texture.Get(), &srvDesc, &srv);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create shader resource view for render target texture {}!", aName);
		return false;
	}

	SetObjectName(texture, aName);
	const std::string rtvName = std::format("{}_RTV", aName);
	SetObjectName(rtv, rtvName);
	const std::string srvName = std::format("{}_SRV", aName);
	SetObjectName(srv, srvName);

	outTexture.myResource = texture;
	outTexture.myRTV = rtv;
	outTexture.mySRV = srv;
	outTexture.myViewport = { 0, 0, static_cast<float>(aWidth), static_cast<float>(aHeight), 0, 1 };
	outTexture.myName = aName;

	return true;
}

bool RenderHardwareInterface::CreateTexture(std::string_view aName, const uint8_t* aByteCode, size_t aByteCodeSize, Texture& outTexture) const
{
	ensure(!aName.empty());

	ComPtr<ID3D11Resource> resource;
	ComPtr<ID3D11ShaderResourceView> srv;
	const HRESULT result = DirectX::CreateDDSTextureFromMemory(
		myDevice.Get(),
		aByteCode,
		aByteCodeSize,
		&resource,
		&srv
	); 
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to load texture {}!", aName);
		return false;
	}

	SetObjectName(resource, aName);
	const std::string srvName = std::format("{}_SRV", aName);
	SetObjectName(srv, srvName);

	outTexture.myResource = resource;
	outTexture.mySRV = srv;
	outTexture.myName = aName;

	return true;

}

bool RenderHardwareInterface::CreateColorTexture(std::string_view aName, const std::array<uint8_t, 4>& aColor, Texture& outTexture) const
{
	ensure(!aName.empty());

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = 1;
	textureDesc.Height = 1;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA textureData = {};
	textureData.pSysMem = aColor.data();
	textureData.SysMemPitch = 4;

	ComPtr<ID3D11Texture2D> texture;
	HRESULT result = myDevice->CreateTexture2D(&textureDesc, &textureData, &texture);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create color texture {}!", aName);
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	ComPtr<ID3D11ShaderResourceView> srv;
	result = myDevice->CreateShaderResourceView(texture.Get(), &srvDesc, &srv);
	if (FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create shader resource view for color texture {}!", aName);
		return false;
	}

	SetObjectName(texture, aName);
	const std::string srvName = std::format("{}_SRV", aName);
	SetObjectName(srv, srvName);

	outTexture.myResource = texture;
	outTexture.mySRV = srv;
	outTexture.myName = aName;

	return true;
}

bool RenderHardwareInterface::CreateSampler(const SamplerDescription& aDescription, Sampler& outSampler) const
{
	ensure(!aDescription.Name.empty());

	D3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(D3D11_DEFAULT);
	samplerDesc.Filter = static_cast<D3D11_FILTER>(aDescription.FilterMode);
	samplerDesc.AddressU = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(aDescription.AddressMode);
	samplerDesc.AddressV = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(aDescription.AddressMode);
	samplerDesc.AddressW = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(aDescription.AddressMode);
	samplerDesc.ComparisonFunc = static_cast<D3D11_COMPARISON_FUNC>(aDescription.ComparisonFunction);
	samplerDesc.BorderColor[0] = aDescription.BorderColor.x;
	samplerDesc.BorderColor[1] = aDescription.BorderColor.y;
	samplerDesc.BorderColor[2] = aDescription.BorderColor.z;
	samplerDesc.BorderColor[3] = aDescription.BorderColor.w;

	const HRESULT result = myDevice->CreateSamplerState(&samplerDesc, &outSampler.mySampler);
	if(FAILED(result))
	{
		LOG(RhiLog, Error, "Failed to create sampler state {}!", aDescription.Name);
		return false;
	}

	SetObjectName(outSampler.mySampler, aDescription.Name);
	outSampler.myDescription = aDescription;
	return true;
}

void RenderHardwareInterface::ExecuteCommandList(const GraphicsCommandList &aCommandList) const
{
	ensure(aCommandList.IsReadyForExecution());
	myContext->ExecuteCommandList(aCommandList.myCommandList.Get(), false);
}

bool RenderHardwareInterface::BeginBackBufferFrame(
	const Texture& aBackBuffer,
	const Texture& aDepthStencil,
	const std::array<float, 4>& aClearColor) const
{
	if (!myContext || !aBackBuffer.myRTV || !aDepthStencil.myDSV)
	{
		return false;
	}

	myContext->ClearRenderTargetView(aBackBuffer.myRTV.Get(), aClearColor.data());
	myContext->ClearDepthStencilView(aDepthStencil.myDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	ID3D11RenderTargetView* renderTarget = aBackBuffer.myRTV.Get();
	myContext->OMSetRenderTargets(1, &renderTarget, aDepthStencil.myDSV.Get());
	const D3D11_VIEWPORT viewport = {
		aBackBuffer.myViewport.TopLeftX,
		aBackBuffer.myViewport.TopLeftY,
		aBackBuffer.myViewport.Width,
		aBackBuffer.myViewport.Height,
		aBackBuffer.myViewport.MinDepth,
		aBackBuffer.myViewport.MaxDepth
	};
	myContext->RSSetViewports(1, &viewport);
	return true;
}

bool RenderHardwareInterface::Present() const
{
	if (!myFrameBenchmark)
	{
		return SUCCEEDED(mySwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
	}

	const FrameBenchmarkSession::Clock::time_point presentStart = FrameBenchmarkSession::Clock::now();
	const HRESULT presentResult = mySwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	const FrameBenchmarkSession::Clock::time_point presentEnd = FrameBenchmarkSession::Clock::now();

	if (myFrameBenchmark->RecordPresent(presentStart, presentEnd))
	{
		if (myFrameBenchmark->WasWrittenSuccessfully())
		{
			LOG(RhiLog, Log, "Benchmark complete: {:.3f} ms average frame time. Results: {}",
				myFrameBenchmark->GetAverageFrameMilliseconds(),
				myFrameBenchmark->GetResultDirectory().string());
		}
		else
		{
			LOG(RhiLog, Error, "Benchmark completed, but its result files could not be written.");
		}
		PostMessageW(myWindowHandle, WM_CLOSE, 0, 0);
	}
	return SUCCEEDED(presentResult);
}

bool RenderHardwareInterface::CompileShader(ShaderType aShaderType, const std::filesystem::path &aPath, 
	ID3DInclude *aIncludeHandler, bool aCompileDebug, Shader &outShader) const
{
    std::ifstream codeFile(aPath, std::ios::binary);
	std::ostringstream codeFileStream;
	codeFileStream << codeFile.rdbuf();
	codeFile.close();
	const std::string shaderSource = codeFileStream.str();

	if	(shaderSource.empty())
	{
		LOG(RhiLog, Error, "Failed to compile shader {}! Shader source is empty.", aPath.string());
		return false;
	}

	std::string shaderTarget(6, ' '); 
	switch (aShaderType)
	{
	case ShaderType::VertexShader:
		shaderTarget = "vs_5_0";
		break;
	case ShaderType::PixelShader:
		shaderTarget = "ps_5_0";
		break;
	case ShaderType::GeometryShader:
		shaderTarget = "gs_5_0";
		break;
	case ShaderType::ComputeShader:
		shaderTarget = "cs_5_0";
		break;
	default:
		LOG(RhiLog, Error, "Failed to compile shader {}! Invalid shader type specified.", aPath.string());
		return false;
	}

	std::vector<D3D_SHADER_MACRO> shaderMacros;
	unsigned shaderFlags = D3DCOMPILE_WARNINGS_ARE_ERRORS;
	if (aCompileDebug)
	{
		shaderMacros.emplace_back(D3D_SHADER_MACRO{ .Name = "_DEBUG", .Definition = "1" });
		shaderFlags |= D3DCOMPILE_DEBUG;
	}

	shaderMacros.emplace_back(D3D_SHADER_MACRO{ .Name = nullptr, .Definition = nullptr });

	ComPtr<ID3DBlob> compiledShader;
	ComPtr<ID3DBlob> errorMessages;

	const HRESULT result = D3DCompile(
		shaderSource.data(),
		shaderSource.size(),
		aPath.string().c_str(),
		shaderMacros.data(),
		aIncludeHandler,
		"main",
		shaderTarget.c_str(),
		shaderFlags,
		0,
		&compiledShader,
		&errorMessages
	);

	if (FAILED(result))
	{
		const std::string message = static_cast<const char*>(errorMessages->GetBufferPointer());
		LOG(RhiLog, Error, "Shader compilation failed!");
		LOG(RhiLog, Error, "Error {}", message);
		return false;
	}

	outShader.myBlob = compiledShader;
	outShader.myType = aShaderType;

	return true;
}


void RenderHardwareInterface::SetObjectName(const Microsoft::WRL::ComPtr<ID3D11DeviceChild>& aObject, std::string_view aName) const
{
	if (aObject)
	{
		aObject->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<unsigned>(aName.length() * sizeof(char)), aName.data());
	}
}
