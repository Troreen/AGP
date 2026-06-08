#include "GraphicsEngine.pch.h"
#include "RenderHardwareInterface.h"

#include "StringHelpers.h"
#include "GraphicsEngine/Objects/Texture.h"
#include "GraphicsEngine/Objects/Buffer.h"
#include "GraphicsEngine/Objects/Vertex.h"
#include "GraphicsCommandList.h"
#include "PipelineStateObject.h"
#include "Ensure.h"

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
	if(aVertexList.empty())
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
	if(aIndexList.empty())
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

void RenderHardwareInterface::ExecuteCommandList(const GraphicsCommandList &aCommandList) const
{
	ensure(aCommandList.IsReadyForExecution());
	myContext->ExecuteCommandList(aCommandList.myCommandList.Get(), false);
}

void RenderHardwareInterface::Present() const
{
	mySwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
}

void RenderHardwareInterface::SetObjectName(const Microsoft::WRL::ComPtr<ID3D11DeviceChild>& aObject, std::string_view aName) const
{
	if (aObject)
	{
		aObject->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<unsigned>(aName.length() * sizeof(char)), aName.data());
	}
}
