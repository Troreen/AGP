#include "GraphicsEngine.pch.h"
#include "GraphicsCommandList.h"

#include "GraphicsEngine/Objects/Buffer.h"
#include "GraphicsEngine/Objects/Texture.h"
#include "GraphicsEngine/RHI/PipelineStateObject.h"
#include "Ensure.h"
#include "StringHelpers.h"

#include <array>
#include <cstring>
#include <format>
#include <utility>

DECLARE_LOG_CATEGORY_WITH_NAME(CmdLog, CommandList, Verbose);

DEFINE_LOG_CATEGORY(CmdLog);


GraphicsCommandList::GraphicsCommandList() = default;

GraphicsCommandList::~GraphicsCommandList() = default;

GraphicsCommandList::GraphicsCommandList(GraphicsCommandList &&aOther) noexcept
{
    *this = std::move(aOther);
}

GraphicsCommandList &GraphicsCommandList::operator=(GraphicsCommandList &&aOther) noexcept
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

void GraphicsCommandList::FinishCommandList()
{
	ensure(!IsReadyForExecution());
    myContext->FinishCommandList(false, &myCommandList);
    const std::string name = std::format("{}_CMD", myName);
    myCommandList->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<unsigned>(sizeof(char) * name.length()), name.data());
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

void GraphicsCommandList::ClearDepthStencil(const Texture &aTarget) const
{
	ensure(!IsReadyForExecution());
	myContext->ClearDepthStencilView(aTarget.myDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void GraphicsCommandList::SetRenderTarget(const Texture* aTarget, const Texture* aDepthStencil) const
{
	ensure(!IsReadyForExecution());
	ID3D11RenderTargetView* rtv = nullptr;
	ID3D11DepthStencilView* dsv = nullptr;
	D3D11_VIEWPORT viewport = { 0, 0, 0, 0, 0, 1 };

	if (aTarget)
	{
		rtv = aTarget->myRTV.Get();
		memcpy_s(&viewport, sizeof(D3D11_VIEWPORT), &aTarget->myViewport, sizeof(Viewport));
	}

	if (aDepthStencil)
	{
		dsv = aDepthStencil->myDSV.Get();
		if(!aTarget)
		{
			memcpy_s(&viewport, sizeof(D3D11_VIEWPORT), &aDepthStencil->myViewport, sizeof(Viewport));
		}
	}

	myContext->OMSetRenderTargets(1, &rtv, dsv);
	myContext->RSSetViewports(1, &viewport);
}

bool GraphicsCommandList::UpdateConstantBuffer(const Buffer &aConstantBuffer, const void *aBufferData, size_t aBufferDataSize) const
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
	
	const HRESULT result = myContext->Map(aConstantBuffer.myBuffer.Get(), 0,  D3D11_MAP_WRITE_DISCARD, 0, &resource);
	if (FAILED(result))
	{
		LOG(CmdLog, Error, "Failed to update constant buffer {}! Failed to map buffer!", aConstantBuffer.myName);
		return false;
	}

	memcpy_s(resource.pData, aConstantBuffer.mySize, aBufferData, aBufferDataSize);
	myContext->Unmap(aConstantBuffer.myBuffer.Get(), 0);

	return true;
}

void GraphicsCommandList::SetVertexBuffer(const Buffer *aBuffer) const
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

void GraphicsCommandList::SetIndexBuffer(const Buffer *aBuffer) const
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

void GraphicsCommandList::SetConstantBuffer(const Buffer *aBuffer, unsigned aSlot, PipeLineStages aStages) const
{
	ensure(!IsReadyForExecution());
	ID3D11Buffer* buffer = nullptr;
	if (aBuffer)
	{
		buffer = aBuffer->myBuffer.Get();
	}

	if (aStages & PipeLineStage_VertexShader)
	{
		myContext->VSSetConstantBuffers(aSlot, 1, &buffer);
	}
	if (aStages & PipeLineStage_GeometryShader)
	{
		myContext->GSSetConstantBuffers(aSlot, 1, &buffer);
	}
	if (aStages & PipeLineStage_PixelShader)
	{
		myContext->PSSetConstantBuffers(aSlot, 1, &buffer);
	}
	
}

void GraphicsCommandList::SetPipelineState(const PipelineStateObject *aPSO)
{
	ensure(!IsReadyForExecution());
	const PipelineStateObject& pipelineState = *aPSO;
	myContext->IASetPrimitiveTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(aPSO->myTopology)); 
	myContext->IASetInputLayout(aPSO->myInputLayout.Get());

	myContext->VSSetShader(
		IsOverrideActive(PipeLineStage_VertexShader) ? myOverridePipelineState.myVertexShader.Get() : pipelineState.myVertexShader.Get(),
		nullptr,
		0);
	myContext->GSSetShader(
		IsOverrideActive(PipeLineStage_GeometryShader) ? myOverridePipelineState.myGeometryShader.Get() : pipelineState.myGeometryShader.Get(),
		nullptr,
		0);
	myContext->PSSetShader(
		IsOverrideActive(PipeLineStage_PixelShader) ? myOverridePipelineState.myPixelShader.Get() : pipelineState.myPixelShader.Get(),
		nullptr,
		0);
	myContext->RSSetState(
		IsOverrideActive(PipeLineStage_Rasterizer) ? myOverridePipelineState.myRasterizerState.Get() : pipelineState.myRasterizerState.Get());

	const std::string message = std::format("Change Pipeline State - {}", aPSO->myName);
	SetMarker(message);
}

void GraphicsCommandList::SetOverridePipelineState(const PipelineStateObject& aOverridePSO, PipeLineStages aOverrideStages)
{
	ensure(!IsReadyForExecution());
	myOverridePipelineState = aOverridePSO;
	myCurrentOverrides = aOverrideStages;
}

bool GraphicsCommandList::IsOverrideActive(PipeLineStages aOverride) const
{
	return (myCurrentOverrides & aOverride) != 0;
}

void GraphicsCommandList::ClearOverridePipelineState()
{
	ensure(!IsReadyForExecution());
	myOverridePipelineState = {};
	myCurrentOverrides = PipeLineStage_None;
}

void GraphicsCommandList::SetShaderResources(const Texture* const* aResourcesList, size_t aNumResources, unsigned aStartSlot, PipeLineStages aStages) const
{
	constexpr size_t maxResourceSlots = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
	const bool isValidRange =
		aNumResources <= maxResourceSlots
		&& aStartSlot <= maxResourceSlots
		&& aNumResources <= maxResourceSlots - aStartSlot;
	ensure(isValidRange);
	ensure(aResourcesList != nullptr || aNumResources == 0);
	if (!isValidRange || (aResourcesList == nullptr && aNumResources > 0) || aNumResources == 0)
	{
		return;
	}

	std::array<ID3D11ShaderResourceView*, maxResourceSlots> srvs = {};
	for (size_t t = 0; t < aNumResources; ++t)
	{
		if (const Texture* currentTexture = aResourcesList[t])
		{
			srvs[t] = currentTexture->mySRV.Get();
		}
	}

	const unsigned numResources = static_cast<unsigned>(aNumResources);
	if (aStages & PipeLineStage_VertexShader)
		myContext->VSSetShaderResources(aStartSlot, numResources, srvs.data());
	if (aStages & PipeLineStage_GeometryShader)
		myContext->GSSetShaderResources(aStartSlot, numResources, srvs.data());
	if (aStages & PipeLineStage_PixelShader)
		myContext->PSSetShaderResources(aStartSlot, numResources, srvs.data());
}

void GraphicsCommandList::SetShaderSamplers(const Sampler* const* aSamplerList, size_t aNumSamplers, unsigned aStartSlot, PipeLineStages aStages) const
{
	constexpr size_t maxSamplerSlots = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
	const bool isValidRange =
		aNumSamplers <= maxSamplerSlots
		&& aStartSlot <= maxSamplerSlots
		&& aNumSamplers <= maxSamplerSlots - aStartSlot;
	ensure(isValidRange);
	ensure(aSamplerList != nullptr || aNumSamplers == 0);
	if (!isValidRange || (aSamplerList == nullptr && aNumSamplers > 0) || aNumSamplers == 0)
	{
		return;
	}

	std::array<ID3D11SamplerState*, maxSamplerSlots> samplers = {};
	for (size_t s = 0; s < aNumSamplers; ++s)
	{
		if (const Sampler* currentSampler = aSamplerList[s])
		{
			samplers[s] = currentSampler->mySampler.Get();
		}
	}

	const unsigned numSamplers = static_cast<unsigned>(aNumSamplers);
	if (aStages & PipeLineStage_VertexShader)
		myContext->VSSetSamplers(aStartSlot, numSamplers, samplers.data());
	if (aStages & PipeLineStage_GeometryShader)
		myContext->GSSetSamplers(aStartSlot, numSamplers, samplers.data());
	if (aStages & PipeLineStage_PixelShader)
		myContext->PSSetSamplers(aStartSlot, numSamplers, samplers.data());
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

	const std::wstring wideMarker = str::acp_to_wide(std::string(aMarker));
	myUDA->SetMarker(wideMarker.c_str());
}

void GraphicsCommandList::BeginEvent(std::string_view aEvent) const
{
	ensure(!IsReadyForExecution());

	const std::wstring wideEvent = str::acp_to_wide(std::string(aEvent));
	myUDA->BeginEvent(wideEvent.c_str());
}

void GraphicsCommandList::EndEvent() const
{
	ensure(!IsReadyForExecution());
	
	myUDA->EndEvent();
}
