#include "GraphicsEngine.pch.h"
#include "GraphicsCommandList.h"

#include "Ensure.h"
#include "Objects/Texture.h"
#include "StringHelpers.h"
#include "Objects/Sampler.h"

DECLARE_LOG_CATEGORY_WITH_NAME(CmdLog, CommandList, Verbose);

DEFINE_LOG_CATEGORY(CmdLog);

GraphicsCommandList::GraphicsCommandList() = default;
GraphicsCommandList::~GraphicsCommandList() = default;

GraphicsCommandList::GraphicsCommandList(GraphicsCommandList&& aOther) noexcept
{
	*this = std::move(aOther);
}

GraphicsCommandList& GraphicsCommandList::operator=(GraphicsCommandList&& aOther) noexcept
{
	if (this != &aOther)
	{
		myContext.Reset();
		myCommandList.Reset();
		myUDA.Reset();

		aOther.myContext.Swap(myContext);
		aOther.myCommandList.Swap(myCommandList);
		aOther.myUDA.Swap(myUDA);
		myName = std::move(aOther.myName);
	}

	return *this;
}

bool GraphicsCommandList::FinishCommandList()
{
	ensure(!IsReadyForExecution());

	const HRESULT result = myContext->FinishCommandList(false, &myCommandList);
	if (FAILED(result))
	{
		myCommandList.Reset();
		LOG(CmdLog, Error, "Command list {} could not be recorded.", myName);
		return false;
	}
	const std::string name = std::format("{}_CMD", myName);
	myCommandList->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<unsigned>(sizeof(char) * name.length()), name.data());
	return true;
}

void GraphicsCommandList::ResetCommandList()
{
	myCommandList.Reset();
}

void GraphicsCommandList::ClearRenderTarget(const Texture& aTarget) const
{
	ensure(!IsReadyForExecution());

	float clearColor[4] = { 0, 0, 0, 0 };
	myContext->ClearRenderTargetView(aTarget.myRTV.Get(), clearColor);
}

void GraphicsCommandList::ClearDepthStencilView(const Texture& aTarget) const
{
	ensure(!IsReadyForExecution());

	myContext->ClearDepthStencilView(aTarget.myDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void GraphicsCommandList::SetRenderTarget(const Texture* aTarget, const Texture* aDepthStencil) const
{
	SetRenderTarget(&aTarget, 1, aDepthStencil);
}

void GraphicsCommandList::SetRenderTarget(const Texture** aTargetList, unsigned aNumTargets, const Texture* aDepthStencil) const
{
	ensure(!IsReadyForExecution());

	if (aNumTargets == 0 && !aDepthStencil)
	{
		ID3D11RenderTargetView* nullRTV = nullptr;
		myContext->OMSetRenderTargets(1, &nullRTV, nullptr);
		return;
	}

	ID3D11RenderTargetView* rtvs[8] = {};
	ID3D11DepthStencilView* dsv = nullptr;
	D3D11_VIEWPORT viewport = { 0, 0, 0, 0, 0, 1 };

	if (aNumTargets > 0)
	{
		unsigned numTargets = (std::min)(aNumTargets, 8u);
		memcpy_s(&viewport, sizeof(D3D11_VIEWPORT), &aTargetList[0]->myViewport, sizeof(Viewport));

		for (unsigned t = 0; t < numTargets; ++t)
		{
			rtvs[t] = aTargetList[t]->myRTV.Get();
		}
	}

	if (aDepthStencil != nullptr)
	{
		dsv = aDepthStencil->myDSV.Get();

		if (aNumTargets == 0)
		{
			memcpy_s(&viewport, sizeof(D3D11_VIEWPORT), &aDepthStencil->myViewport, sizeof(Viewport));
		}
	}

	myContext->OMSetRenderTargets(8u, rtvs, dsv);
	myContext->RSSetViewports(1, &viewport);
}

bool GraphicsCommandList::UpdateConstantBuffer(const Buffer& aConstantBuffer, const void* aBufferData, size_t aBufferDataSize) const
{
	ensure(!IsReadyForExecution());

	if (!aConstantBuffer.IsValid() || aConstantBuffer.myType != BufferType::ConstantBuffer)
	{
		LOG(CmdLog, Error, "Failed to update constant buffer! Buffer is either null or invalid type!");
		return false;
	}

	if (aBufferDataSize > aConstantBuffer.mySize)
	{
		LOG(CmdLog, Error, "Failed to update constant buffer {}! Data provided is larger than the buffer capacity!", aConstantBuffer.myName);
		return false;
	}

	D3D11_MAPPED_SUBRESOURCE resource = {};

	const HRESULT result = myContext->Map(aConstantBuffer.myBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);

	if (FAILED(result))
	{
		LOG(CmdLog, Error, "Failed to update constant buffer {}! Failed to map buffer!", aConstantBuffer.myName);
		return false;
	}

	memcpy_s(resource.pData, aConstantBuffer.mySize, aBufferData, aBufferDataSize);
	myContext->Unmap(aConstantBuffer.myBuffer.Get(), 0);

	return true;
}

bool GraphicsCommandList::UpdateDynamicVertexBuffer(const Buffer& aVertexBuffer, const Vertex* aVertices, size_t aVertexCount) const
{
	if (!aVertexBuffer.IsValid())
	{
		LOG(CmdLog, Error, "Failed to update Vertex buffer! Buffer is either null!");
		return false;
	}

	if (aVertexCount == 0)
	{
		LOG(CmdLog, Error, "Failed to update Vertex buffer {}! Data provided is larger than buffer capacity!", aVertexBuffer.myName);
		return false;
	}

	if (aVertexBuffer.myType != BufferType::VertexBuffer)
	{
		LOG(CmdLog, Error, "Failed to update Vertex buffer! Buffer  invalid type!");
		return false;
	}

	const size_t dataSize = sizeof(Vertex) * aVertexCount;
	if (dataSize > aVertexBuffer.mySize)
	{
		return false;
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource = {};
	const HRESULT result = myContext->Map(aVertexBuffer.myBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	memcpy(mappedResource.pData, aVertices, dataSize);
	myContext->Unmap(aVertexBuffer.myBuffer.Get(), 0);

	return true;
}

void GraphicsCommandList::SetVertexBuffer(const Buffer* aBuffer) const
{
	ensure(!IsReadyForExecution());

	constexpr unsigned offset = 0;
	if (aBuffer)
	{
		myContext->IASetVertexBuffers(0, 1, aBuffer->myBuffer.GetAddressOf(), &aBuffer->myStride, &offset);
	}
	else
	{
		const unsigned stride = 0;
		ID3D11Buffer* buffer = nullptr;
		myContext->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
	}
}

void GraphicsCommandList::SetIndexBuffer(const Buffer* aBuffer) const
{
	ensure(!IsReadyForExecution());

	if (aBuffer)
	{
		myContext->IASetIndexBuffer(aBuffer->myBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	}
	else
	{
		ID3D11Buffer* buffer = nullptr;
		myContext->IASetIndexBuffer(buffer, DXGI_FORMAT_UNKNOWN, 0);
	}
}

void GraphicsCommandList::SetConstantBuffer(const Buffer* aBuffer, unsigned aSlot, PipelineStages aStages) const
{
	ensure(!IsReadyForExecution());

	ID3D11Buffer* buffer = nullptr;
	if (aBuffer)
	{
		buffer = aBuffer->myBuffer.Get();
	}
	if (aStages & PipelineStage_VertexShader)
	{
		myContext->VSSetConstantBuffers(aSlot, 1, &buffer);
	}
	if (aStages & PipelineStage_GeometryShader)
	{
		myContext->GSSetConstantBuffers(aSlot, 1, &buffer);
	}
	if (aStages & PipelineStage_PixelShader)
	{
		myContext->PSSetConstantBuffers(aSlot, 1, &buffer);
	}
}

void GraphicsCommandList::SetPipelineState(const PipelineStateObject& aPSO) const
{
	ensure(!IsReadyForExecution());

	myContext->IASetPrimitiveTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(aPSO.myTopology));
	myContext->IASetInputLayout(aPSO.myInputLayout.Get());

	if (IsOverrideActive(PipelineStage_VertexShader))
	{
		myContext->VSSetShader(myOverridePipelineState.myVertexShader.Get(), nullptr, 0);
	}
	else
	{
		myContext->VSSetShader(aPSO.myVertexShader.Get(), nullptr, 0);
	}

	if (IsOverrideActive(PipelineStage_GeometryShader))
	{
		myContext->GSSetShader(myOverridePipelineState.myGeometryShader.Get(), nullptr, 0);
	}
	else
	{
		myContext->GSSetShader(aPSO.myGeometryShader.Get(), nullptr, 0);
	}

	if (IsOverrideActive(PipelineStage_Rasterizer))
	{
		myContext->RSSetState(myOverridePipelineState.myRasterizerState.Get());
	}
	else
	{
		myContext->RSSetState(aPSO.myRasterizerState.Get());
	}

	if (IsOverrideActive(PipelineStage_PixelShader))
	{
		myContext->PSSetShader(myOverridePipelineState.myPixelShader.Get(), nullptr, 0);
	}
	else
	{
		myContext->PSSetShader(aPSO.myPixelShader.Get(), nullptr, 0);
	}

	constexpr float blendFactor[4] = { };
	if (IsOverrideActive(PipelineStage_OutputMerger))
	{
		myContext->OMSetBlendState(myOverridePipelineState.myBlendState.Get(), blendFactor, 0xffffffff);
	}
	else
	{
		myContext->OMSetBlendState(aPSO.myBlendState.Get(), blendFactor, 0xffffffff);
	}

	const std::string message = std::format("Change Pipeline State {}", aPSO.myName);
	SetMarker(message);
}

void GraphicsCommandList::SetShaderResource(const Texture* aResource, unsigned aSlot, PipelineStages aStages) const
{
	SetShaderResources(&aResource, 1, aSlot, aStages);
}

void GraphicsCommandList::SetShaderResources(const Texture* const* aResourcesList, size_t aNumResources, unsigned aStartSlot, PipelineStages aStages) const
{
	std::vector<ID3D11ShaderResourceView*> srvs(aNumResources);

	for (size_t t = 0; t < aNumResources; ++t)
	{
		if (const Texture* currentTexture = aResourcesList[t])
		{
			srvs[t] = currentTexture->mySRV.Get();
		}
	}

	if (aStages & PipelineStage_VertexShader)
	{
		myContext->VSSetShaderResources(aStartSlot, static_cast<unsigned>(srvs.size()), srvs.data());
	}

	if (aStages & PipelineStage_GeometryShader)
	{
		myContext->GSSetShaderResources(aStartSlot, static_cast<unsigned>(srvs.size()), srvs.data());
	}

	if (aStages & PipelineStage_PixelShader)
	{
		myContext->PSSetShaderResources(aStartSlot, static_cast<unsigned>(srvs.size()), srvs.data());
	}
}

void GraphicsCommandList::SetShaderSamplers(const Sampler* const* aSamplerList, size_t aNumSamplers, unsigned aStartSlot, PipelineStages aStages) const
{
	std::vector<ID3D11SamplerState*> samplers(aNumSamplers);

	for (size_t s = 0; s < aNumSamplers; ++s)
	{
		if (const Sampler* currentSampler = aSamplerList[s])
		{
			samplers[s] = currentSampler->mySampler.Get();
		}
	}

	if (aStages & PipelineStage_VertexShader)
	{
		myContext->VSSetSamplers(aStartSlot, static_cast<unsigned>(samplers.size()), samplers.data());
	}

	if (aStages & PipelineStage_GeometryShader)
	{
		myContext->GSSetSamplers(aStartSlot, static_cast<unsigned>(samplers.size()), samplers.data());
	}

	if (aStages & PipelineStage_PixelShader)
	{
		myContext->PSSetSamplers(aStartSlot, static_cast<unsigned>(samplers.size()), samplers.data());
	}
}

void GraphicsCommandList::Draw(unsigned aNumVertices) const
{
	ensure(!IsReadyForExecution());

	myContext->Draw(aNumVertices, 0);
}

void GraphicsCommandList::DrawIndexed(unsigned aIndexCount, unsigned aIndexOffset) const
{
	ensure(!IsReadyForExecution());

	myContext->DrawIndexed(aIndexCount, aIndexOffset, 0);
}

void GraphicsCommandList::SetMarker(std::string_view aMarker) const
{
	ensure(!IsReadyForExecution());

	const std::wstring wideMarker = str::acp_to_wide_ascii(aMarker.data());
	myUDA->SetMarker(wideMarker.c_str());
}

void GraphicsCommandList::BeginEvent(std::string_view aEvent) const
{
	ensure(!IsReadyForExecution());

	const std::wstring wideEvent = str::acp_to_wide_ascii(aEvent.data());
	myUDA->BeginEvent(wideEvent.c_str());
}

void GraphicsCommandList::EndEvent() const
{
	ensure(!IsReadyForExecution());

	myUDA->EndEvent();
}

void GraphicsCommandList::SetOverridePipelineState(const PipelineStateObject& aOverridePSO, PipelineStages aOverrideStages)
{
	myOverridePipelineState = aOverridePSO;
	myCurrentOverrides = aOverrideStages;
}

void GraphicsCommandList::ChangeActiveOverrides(PipelineStages aOverrideFlags)
{
	myCurrentOverrides = aOverrideFlags;
}

bool GraphicsCommandList::IsOverrideActive(PipelineStages aOverride) const
{
	return (myCurrentOverrides & aOverride);
}

void GraphicsCommandList::ClearOverridePipelineState()
{
	myOverridePipelineState = {};
	myCurrentOverrides = PipelineStage_None;
}
