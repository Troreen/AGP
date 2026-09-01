#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "GraphicsEngine/RHI/RHIStructs.h"

class Texture;
class PipelineStateObject;

/**
 * Which Domain this Material belongs to. Controls the Vertex type, Vertex Shader and Topology.
 */
enum class MaterialDomain : uint8_t
{
	None,
	Surface,
	// More to come
};

/**
 * The Shading Model of this Material. Controls the Pixel Shader.
 */
enum class ShadingModel : uint8_t
{
	None,
	Unlit,
	Lit,
};

/**
 * Enum of types used in HLSL during the AGP Course. To make matching easier.
 */
enum class MaterialParameterType : uint8_t
{
	Unknown,
	Float, Float2, Float3, Float4,
	Int, Int2, Int3, Int4,
	Uint, Uint2, Uint3, Uint4,
	Bool, Matrix4x4
};

/**
 * Information about a Material Parameter found in the HLSL MaterialParametersBuffer
 */
struct MaterialParameterInfo
{
	std::string Name;
	MaterialParameterType Type;
	size_t Offset;
	size_t Size;
	size_t Index;
};

/**
 * Generic Interface for Material-like objects.
 * Provides a concrete method of communication with both Material and MaterialInstance
 * which both look the same outwards but work differently internally.
 */
class MaterialInterface
{
public:

	// Maximum allowed size of the HLSL MaterialParametersBuffer constant buffer.
	// This is to make sure we don't underflow data storage on the C++ side.
	// This cannot be more than 65535 (64 kB)! Hard limit for CBuffers.
	static constexpr unsigned MATERIAL_BUFFER_SIZE = 128;

	// This cannot ever be more than 16.
	// VS and GS stages are limited to 16 textures while PS and CS can have 128.
	// However, to keep things simple we assume that >= 16 are system slots
	// which cannot be used by materials.
	static constexpr unsigned MAX_MATERIAL_TEXTURE_COUNT = 16;
	static constexpr unsigned ALBEDO_TEXTURE_SLOT = 0;
	static constexpr unsigned NORMAL_TEXTURE_SLOT = 1;
	static constexpr unsigned MATERIAL_TEXTURE_SLOT = 2;

	MaterialInterface() = default;
	virtual ~MaterialInterface() = default;

	// No copying Materials
	MaterialInterface(const MaterialInterface&) = delete;
	MaterialInterface& operator=(const MaterialInterface&) = delete;

	// Move is OK!
	MaterialInterface(MaterialInterface&&) noexcept = default;
	MaterialInterface& operator=(MaterialInterface&&) noexcept = default;

	virtual std::string_view GetName() const = 0;

	virtual MaterialDomain GetDomain() const = 0;
	virtual ShadingModel GetShadingModel() const = 0;
	virtual BlendMode GetBlendMode() const = 0;

	virtual const PipelineStateObject& GetPSO() const = 0;

	// /**
	//  * If True there has been changes to the C++ side MaterialParametersBuffer
	//  * and you need to call RefreshMaterialData(). Bubbles upwards along Child-Parent relationships
	//  * so that if a Parent MaterialParametersBuffer is Dirty we need to rebuild.
	//  * @return True if the MaterialParametersBuffer has been changed since last call to RefreshMaterialData.
	//  */
	virtual bool IsMaterialDataDirty() const = 0;

	// /**
	//  * Refreshes the MaterialParametersBuffer settings. Also asks our Parent, if there is one, for
	//  * updated MaterialParametersBuffer settings.
	//  * @return 
	//  */
	virtual void RefreshMaterialData() const = 0;

	// /**
	//  * Retrieves a pointer to the C++ MaterialParametersBuffer data.
	//  */
	virtual const uint8_t* GetParameterDataBlock() const = 0;

	// /**
	//  * If this Material has any Parameters or not.
	//  */
	virtual bool HasParameters() const = 0;

	// /**
	//  * Retrieve the list of Parameters for this Material.
	//  */
	virtual const std::vector<MaterialParameterInfo>& GetParameters() const = 0;	

	// /**
	//  * Retrieves a pointer to the MaterialParameterInfo based on the Parameter Index.
	//  * @param aIndex The Index of the Parameter to retrieve info about or nullptr if Index is invalid.
	//  */
	virtual const MaterialParameterInfo* GetParameterByIndex(unsigned aIndex) const = 0;

	// /**
	//  * Retrieves a pointer to the MaterialParameterInfo based on the Parameter Name.
	//  * @param aName The Name of the Parameter to retrieve info about or nullptr if Name is invalid.
	//  */
	virtual const MaterialParameterInfo* GetParameterByName(const std::string& aName) const = 0;

	virtual unsigned GetTextureSlotByName(const std::string& aName) const = 0;

	virtual bool SetTexture(const std::string& aName, const std::shared_ptr<Texture>& aTexture) = 0;
	virtual bool SetTexture(unsigned aSlot, const std::shared_ptr<Texture>& aTexture) = 0;
	virtual std::shared_ptr<Texture> GetTexture(const std::string& aName) const = 0;
	virtual std::shared_ptr<Texture> GetTexture(unsigned aSlot) const = 0;

};
