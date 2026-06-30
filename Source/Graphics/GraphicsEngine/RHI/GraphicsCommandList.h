#pragma once
#include <cstddef>
#include <string>
#include <wrl.h>

#include "RHIStructs.h"
#include "PipelineStateObject.h"

class Buffer;
class RenderHardwareInterface;
class Sampler;
class Texture;

class GraphicsCommandList
{
    friend class RenderHardwareInterface;

public:
    GraphicsCommandList();
    ~GraphicsCommandList();

    GraphicsCommandList(const GraphicsCommandList&) = delete;
    GraphicsCommandList& operator=(const GraphicsCommandList&) = delete;

    GraphicsCommandList(GraphicsCommandList&& aOther) noexcept;
    GraphicsCommandList& operator=(GraphicsCommandList&& aOther) noexcept;

    void FinishCommandList();
    void ResetCommandList();

    bool IsReadyForExecution() const { return myCommandList; }

    void ClearRenderTarget(const Texture& aTarget) const;
	void ClearDepthStencil(const Texture& aTarget) const;
	void SetRenderTarget(const Texture* aTarget, const Texture* aDepthStencil) const;

    bool UpdateConstantBuffer(const Buffer& aConstantBuffer, const void* aBufferData, size_t aBufferDataSize) const;

	void SetVertexBuffer(const Buffer* aBuffer) const;
	void SetIndexBuffer(const Buffer* aBuffer) const;
	void SetConstantBuffer(const Buffer* aBuffer, unsigned aSlot, PipeLineStages aStages) const; 
	void SetPipelineState (const PipelineStateObject* aPSO);
	void SetOverridePipelineState(const PipelineStateObject& aOverridePSO, PipeLineStages aOverrideStages);
	bool IsOverrideActive(PipeLineStages aOverride) const;
	void ClearOverridePipelineState();

    void SetShaderResources(const Texture* const* aResourcesList, size_t aNumResources, unsigned aStartSlot, PipeLineStages aStages) const;
    void SetShaderSamplers(const Sampler* const* aSamplerList, size_t aNumSamplers, unsigned aStartSlot, PipeLineStages aStages) const;


	void Draw(unsigned aNumVertices) const;
	void DrawIndexed(unsigned aIndexCount, unsigned aIndexOffset) const;

    void SetMarker(std::string_view aMarker) const;
    void BeginEvent(std::string_view aEvent) const;
    void EndEvent() const;

private:
    std::string myName;

    Microsoft::WRL::ComPtr<struct ID3D11DeviceContext> myContext;
    Microsoft::WRL::ComPtr<struct ID3D11CommandList> myCommandList;
    Microsoft::WRL::ComPtr<struct ID3DUserDefinedAnnotation> myUDA;
    PipelineStateObject myOverridePipelineState;
    PipeLineStages myCurrentOverrides = PipeLineStage_None;

};

