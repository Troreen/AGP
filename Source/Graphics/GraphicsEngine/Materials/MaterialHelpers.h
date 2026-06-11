#pragma once
// Include your Common Utilities Types here as needed
#include "Matrix.hpp"
#include "Vector.hpp"
#include "MaterialInterface.h"

#include <string>
#include <unordered_map>

/**
 * TGA Material System.
 * Helper methods for material inner workings.
 */
namespace MaterialHelpers
{
	static MaterialParameterType HLSLTypeToMaterialParameterType(const std::string& aHLSLTypeName)
	{
		static std::unordered_map<std::string, MaterialParameterType> HLSLTypeToMaterialParameterType = {
			{ "float",		MaterialParameterType::Float },
			{ "bool",			MaterialParameterType::Int },		// This is because bool is 1 byte on CPU, and 4 bytes on GPU.
			{ "float2",		MaterialParameterType::Float2 },
			{ "float3",		MaterialParameterType::Float3 },
			{ "float4",		MaterialParameterType::Float4 },
			{ "dword",		MaterialParameterType::Uint }		// Single uint shows up as a DWORD definition.
		};

		auto it = HLSLTypeToMaterialParameterType.find(aHLSLTypeName);
		if (it == HLSLTypeToMaterialParameterType.end())
			return MaterialParameterType::Unknown;

		return it->second;
	}

	/**
	 * Default Template for Type Traits.
	 * Helps identify what the C++ type T equals in HLSL.
	 * By default, this Template indicates an unsupported Type.
	 * @tparam T The C++ type to store Traits for.
	 */
	template<typename T>
	struct MaterialParameterTraits
	{
		static constexpr bool Supported = false;
		static bool IsA(const MaterialParameterInfo&) = delete;
	};

	/**
	 * Helper Macro that makes defining MaterialParameterTraits easier.
	 * Generates copies of the MaterialParameterTraits template with the
	 * supplied values and type.
	 * @param CppType The C++ Type we're defining Traits for.
	 * @param EnumType The MaterialParameterType enum value that equals the C++ type.
	 */
	#define DECLARE_MATERIAL_PARAMETER_TRAIT(CppType, EnumType) \
	template<> struct MaterialParameterTraits<CppType> { \
		static constexpr bool Supported = true; \
		static bool IsA(const MaterialParameterInfo& aInfo) { \
			return aInfo.Type == (EnumType) && aInfo.Size == sizeof(CppType); \
		} \
	};

	// Register the types I want to use in the MaterialParametersBuffer in HLSL.
	DECLARE_MATERIAL_PARAMETER_TRAIT(float, MaterialParameterType::Float);
	DECLARE_MATERIAL_PARAMETER_TRAIT(CU::Vector2f, MaterialParameterType::Float2);
	DECLARE_MATERIAL_PARAMETER_TRAIT(CU::Vector3f, MaterialParameterType::Float3);
	DECLARE_MATERIAL_PARAMETER_TRAIT(CU::Vector4f, MaterialParameterType::Float4);
	DECLARE_MATERIAL_PARAMETER_TRAIT(int, MaterialParameterType::Int);
	DECLARE_MATERIAL_PARAMETER_TRAIT(CU::Vector2i, MaterialParameterType::Int2);
	DECLARE_MATERIAL_PARAMETER_TRAIT(CU::Vector3<int>, MaterialParameterType::Int3);
	DECLARE_MATERIAL_PARAMETER_TRAIT(CU::Vector4i, MaterialParameterType::Int4);
	DECLARE_MATERIAL_PARAMETER_TRAIT(unsigned, MaterialParameterType::Uint);
	DECLARE_MATERIAL_PARAMETER_TRAIT(CU::Vector2u, MaterialParameterType::Uint2);
	DECLARE_MATERIAL_PARAMETER_TRAIT(CU::Vector3<unsigned>, MaterialParameterType::Uint3);
	DECLARE_MATERIAL_PARAMETER_TRAIT(CU::Vector4u, MaterialParameterType::Uint4);
	DECLARE_MATERIAL_PARAMETER_TRAIT(bool, MaterialParameterType::Bool);
	DECLARE_MATERIAL_PARAMETER_TRAIT(CU::Matrix4f, MaterialParameterType::Matrix4x4);
}
