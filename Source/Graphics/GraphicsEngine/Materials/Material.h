#pragma once

#include "MaterialInterface.h"
#include "MaterialHelpers.h"
#include "GraphicsEngine/RHI/PipelineStateObject.h"
#include "Ensure.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

struct MaterialDescription
{
    std::string Name;
    MaterialDomain Domain = MaterialDomain::None;
    ShadingModel ShadingModel = ShadingModel::None;
    BlendMode BlendMode = BlendMode::None;
    std::filesystem::path MaterialShaderCode;
    std::filesystem::path AlbedoTexture;
    std::filesystem::path NormalTexture;
    std::filesystem::path MaterialTexture;
};

bool LoadMaterialDescription(const std::filesystem::path& aPath, MaterialDescription& outDescription);

class Material : public MaterialInterface
{
    friend class GraphicsEngine;

public:

    std::string_view GetName() const override { return myName; }
    MaterialDomain GetDomain() const override { return myDescription.Domain; }
    ShadingModel GetShadingModel() const override { return myDescription.ShadingModel; }
    BlendMode GetBlendMode() const override { return myDescription.BlendMode; }

    const PipelineStateObject& GetPSO() const override { return myPSO; }

    const uint8_t* GetParameterDataBlock() const override { return myData; }
    bool HasParameters() const override { return !myParameters.empty(); }
    const std::vector<MaterialParameterInfo>& GetParameters() const override { return myParameters;  }

    bool IsMaterialDataDirty() const override;
    void RefreshMaterialData() const override;
	const MaterialParameterInfo* GetParameterByIndex(unsigned aIndex) const override;
	const MaterialParameterInfo* GetParameterByName(const std::string& aName) const override;


	unsigned GetTextureSlotByName(const std::string& aName) const override;

	bool SetTexture(const std::string& aName, const std::shared_ptr<Texture>& aTexture) override;
	bool SetTexture(unsigned aSlot, const std::shared_ptr<Texture>& aTexture) override;

    std::shared_ptr<Texture> GetTexture(const std::string& aName) const override;
	std::shared_ptr<Texture> GetTexture(unsigned aSlot) const override; 

private:

	alignas(16) uint8_t myData[MATERIAL_BUFFER_SIZE] = {};
    std::vector<MaterialParameterInfo> myParameters;
	std::unordered_map<std::string, unsigned> myParameterNameToIndex;
    
    std::shared_ptr<Texture> myTextures[MAX_MATERIAL_TEXTURE_COUNT] = {};
	std::unordered_map<std::string, unsigned> myTextureSlotNameToIndex;

    MaterialDescription myDescription;
    PipelineStateObject myPSO;
    std::string myName;

};

class MaterialInstance : public MaterialInterface
{
public:

    static std::shared_ptr<MaterialInstance> Create(std::string_view aName, const std::shared_ptr<MaterialInterface>& aMaterialInterface);
   
    std::string_view GetName() const override { return myName; }
    MaterialDomain GetDomain() const override { return myParentMaterial->GetDomain(); }
    ShadingModel GetShadingModel() const override { return myParentMaterial->GetShadingModel(); }
    BlendMode GetBlendMode() const override { return myParentMaterial->GetBlendMode(); }

    const PipelineStateObject& GetPSO() const override { return myParentMaterial->GetPSO(); }

    const uint8_t* GetParameterDataBlock() const override { return myData; }
    bool HasParameters() const override { return myParentMaterial->HasParameters(); }
    const std::vector<MaterialParameterInfo>& GetParameters() const override { return myParentMaterial->GetParameters(); }

    bool IsMaterialDataDirty() const override;
    void RefreshMaterialData() const override;
	const MaterialParameterInfo* GetParameterByIndex(unsigned aIndex) const override { return myParentMaterial->GetParameterByIndex(aIndex); }
	const MaterialParameterInfo* GetParameterByName(const std::string& aName) const override { return myParentMaterial->GetParameterByName(aName); }

	template<typename T>
    bool SetValue(const std::string& aLabel, const T& aValue)
    {
        const MaterialParameterInfo* param = GetParameterByName(aLabel);
        if (!param)
            return false;

        ensure(MaterialHelpers::MaterialParameterTraits<T>::IsA(*param));

        return SetRawParameterValue(*param, &aValue, sizeof(T));
    }

    unsigned GetTextureSlotByName(const std::string& aName) const override;

    bool SetTexture(const std::string& aName, const std::shared_ptr<Texture>& aTexture) override;
    bool SetTexture(unsigned aSlot, const std::shared_ptr<Texture>& aTexture) override;

    std::shared_ptr<Texture> GetTexture(const std::string& aName) const override;
    std::shared_ptr<Texture> GetTexture(unsigned aSlot) const override;


private:

    bool SetRawParameterValue(const MaterialParameterInfo& aParamInfo, const void* aPtr, size_t aPtrSize);

    alignas(16) mutable uint8_t myData[MATERIAL_BUFFER_SIZE] = {};
	std::optional<std::shared_ptr<Texture>> myTextures[MAX_MATERIAL_TEXTURE_COUNT] = {};

    std::string myName;

	std::shared_ptr<const MaterialInterface> myParentMaterial;

	mutable bool myIsParameterDataDirty = false;

    std::vector<bool> myOverridenParameters;
};
