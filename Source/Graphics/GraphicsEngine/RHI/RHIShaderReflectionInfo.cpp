#include "GraphicsEngine.pch.h"
#include "RHIShaderReflectionInfo.h"
#include "RHIStructs.h"
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <Ensure.h>

using namespace Microsoft::WRL;

template<>
struct std::hash<RHIShaderReflectionInfo::ResourceBinding>
{
	size_t operator()(const RHIShaderReflectionInfo::ResourceBinding& x) const noexcept
	{
		return std::hash<std::string>{}(x.Name);
	}
};

inline bool operator==(const RHIShaderReflectionInfo::ResourceBinding& a, const RHIShaderReflectionInfo::ResourceBinding& b) noexcept
{
	return a.Name == b.Name && a.Type == b.Type && a.BindPoint == b.BindPoint && a.Count == b.Count;
}

namespace
{
	std::string DeriveHLSLType(const D3D11_SHADER_TYPE_DESC& aTypeDesc)
	{
		std::string typeString;
		typeString.reserve(32);
		switch (aTypeDesc.Type)
		{
		case D3D_SVT_FLOAT:
			typeString += "float";
			break;
		case D3D_SVT_UINT:
			// If it's a single uint it's shown as dword, otherwise uint
			if (aTypeDesc.Columns > 1 || aTypeDesc.Rows > 1)
			{
				typeString += "uint";
			}
			else
			{
				typeString += "dword";
			}
			break;
		case D3D_SVT_INT:
			typeString += "int";
			break;
		case D3D_SVT_BOOL:
			typeString += "bool";
			break;
		case D3D_SVT_DOUBLE:
			typeString += "double";
			break;
		default:
			ensure(false);
		}

		switch (aTypeDesc.Class)
		{
		case D3D_SVC_MATRIX_COLUMNS:
			typeString += std::format("{}x{}", aTypeDesc.Columns, aTypeDesc.Rows);
			break;
		case D3D_SVC_MATRIX_ROWS:
			typeString += std::format("{}x{}", aTypeDesc.Rows, aTypeDesc.Columns);
			break;
		case D3D_SVC_VECTOR:
			typeString += std::format("{}", aTypeDesc.Columns);
			break;
		case D3D_SVC_SCALAR:
			// Scalar is just a single so no change.
			break;
		default:
			ensure(false);
		}

		return typeString;
	}

	void ReflectVariable(std::string_view aDomain, ID3D11ShaderReflectionType* aVarType, const D3D11_SHADER_VARIABLE_DESC* aVarDesc, const D3D11_SHADER_TYPE_DESC& aVarTypeDesc, RHIShaderReflectionInfo::ConstantBufferInfo& inoutBufferInfo, size_t& inoutOffset)
	{
		if(aVarTypeDesc.Members > 0)
		{
			// This is a struct or class or similar. It has child members.
			for(unsigned m = 0; m < aVarTypeDesc.Members; ++m)
			{
				ID3D11ShaderReflectionType* memberType = aVarType->GetMemberTypeByIndex(m);

				D3D11_SHADER_TYPE_DESC memberTypeDesc = {};
				memberType->GetDesc(&memberTypeDesc);

				const std::string memberName = aVarType->GetMemberTypeName(m);
				const std::string memberDomain = aDomain.empty() ? memberName : std::string(aDomain) + "." + memberName;

				std::string typeString = DeriveHLSLType(memberTypeDesc);
				memberTypeDesc.Name = typeString.c_str();

				// To avoid weird things happening with structs fill in the name in the type desc.
				if (memberTypeDesc.Name)
				{
					ensure(typeString == memberTypeDesc.Name);
				}
				// Passing nullptr for Desc here since struct members are not allowed to have default values in HLSL.
				ReflectVariable(memberDomain, memberType, nullptr, memberTypeDesc, inoutBufferInfo, inoutOffset);
			}
		}
		else
		{
			RHIShaderReflectionInfo::ConstantBufferInfo::MemberInfo memInfo;
			if (aDomain.starts_with("__"))
				return;
			memInfo.Name = aDomain;
			std::string typeString = DeriveHLSLType(aVarTypeDesc);
			memInfo.Type = typeString.c_str();

			// To avoid weird things happening with structs fill in the name in the type desc.
			if (aVarTypeDesc.Name)
			{
				ensure(typeString == aVarTypeDesc.Name);
			}
			memInfo.Size = static_cast<size_t>(aVarTypeDesc.Rows * aVarTypeDesc.Columns) * sizeof(float);
			if(aVarDesc && aVarDesc->DefaultValue != nullptr)
			{
				memcpy_s(memInfo.Default, 64, aVarDesc->DefaultValue, aVarDesc->Size);
			}
			memInfo.Offset = inoutOffset;
			inoutOffset += memInfo.Size;

			inoutBufferInfo.MemberNameToIndex.emplace(memInfo.Name, inoutBufferInfo.Members.size());
			inoutBufferInfo.Members.emplace_back(memInfo);
		}
	}

	bool ReflectShader(ComPtr<ID3D11ShaderReflection>& aShaderRefl, RHIShaderReflectionInfo& outReflectionInfo)
	{
		D3D11_SHADER_DESC shaderDesc = {};
		aShaderRefl->GetDesc(&shaderDesc);

		outReflectionInfo.Type = static_cast<ShaderType>(D3D11_SHVER_GET_TYPE(shaderDesc.Version));
		outReflectionInfo.ConstantBuffers.reserve(shaderDesc.ConstantBuffers);
		outReflectionInfo.Bindings.reserve(shaderDesc.BoundResources);

		for (unsigned b = 0; b < shaderDesc.BoundResources; ++b)
		{
			D3D11_SHADER_INPUT_BIND_DESC bindDesc = {};
			aShaderRefl->GetResourceBindingDesc(b, &bindDesc);

			RHIShaderReflectionInfo::ResourceBinding binding;
			binding.Name = bindDesc.Name;
			binding.Type = bindDesc.Type;
			binding.BindPoint = bindDesc.BindPoint;
			binding.Count = bindDesc.BindCount;

			outReflectionInfo.Bindings.emplace_back(binding);
			//std::string lowerName = binding.Name;
			//std::ranges::transform(lowerName, lowerName.begin(), tolower);
			outReflectionInfo.BindingNameToIndex.emplace(binding.Name, b);
		}

		for (unsigned c = 0; c < shaderDesc.ConstantBuffers; ++c)
		{
			RHIShaderReflectionInfo::ConstantBufferInfo bufferInfo;
			ID3D11ShaderReflectionConstantBuffer* bufferRefl = aShaderRefl->GetConstantBufferByIndex(c);

			D3D11_SHADER_BUFFER_DESC cbufferDesc = {};
			D3D11_SHADER_INPUT_BIND_DESC cbufferBindDesc = {};

			bufferRefl->GetDesc(&cbufferDesc);
			aShaderRefl->GetResourceBindingDescByName(cbufferDesc.Name, &cbufferBindDesc);

			bufferInfo.Size = cbufferDesc.Size;
			bufferInfo.Name = cbufferDesc.Name;
			bufferInfo.Slot = cbufferBindDesc.BindPoint;

			bufferInfo.Members.reserve(cbufferDesc.Variables);
			bufferInfo.MemberNameToIndex.reserve(cbufferDesc.Variables);

			size_t offset = 0;
			for (unsigned v = 0; v < cbufferDesc.Variables; ++v)
			{
				ID3D11ShaderReflectionVariable* var = bufferRefl->GetVariableByIndex(v);

				D3D11_SHADER_VARIABLE_DESC varDesc = {};
				var->GetDesc(&varDesc);

				ID3D11ShaderReflectionType* varType = var->GetType();

				D3D11_SHADER_TYPE_DESC varTypeDesc = {};
				varType->GetDesc(&varTypeDesc);
				ReflectVariable(varDesc.Name, varType, &varDesc, varTypeDesc, bufferInfo, offset);
			}

			//std::string lowerName = bufferInfo.Name;
			//std::ranges::transform(lowerName, lowerName.begin(), tolower);
			outReflectionInfo.ConstantBufferNameToIndex.emplace(bufferInfo.Name, c);
			outReflectionInfo.ConstantBuffers.emplace_back(bufferInfo);
		}

		return true;
	}
}

bool RHIShaderReflector::Reflect(const uint8_t* aShaderData, size_t aShaderDataSize, RHIShaderReflectionInfo& outReflectionInfo)
{
	ComPtr<ID3D11ShaderReflection> shaderRefl;
	HRESULT reflectResult = D3DReflect(aShaderData, aShaderDataSize, IID_ID3D11ShaderReflection, reinterpret_cast<void**>(shaderRefl.GetAddressOf()));
	if (SUCCEEDED(reflectResult))
	{
		return ReflectShader(shaderRefl, outReflectionInfo);
	}

	return false;
}

