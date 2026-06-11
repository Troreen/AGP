#pragma once

#include "RHIStructs.h"

struct RHIShaderReflectionInfo
{
	struct FunctionInfo
	{
		struct ParamInfo
		{
			std::string Name;
			std::string Semantic;
			unsigned Columns = 0;
			unsigned Index = 0;
		};

		std::string Name;
		std::vector<ParamInfo> InParams;
		std::unordered_map<std::string, size_t> InParamToIndex;
		std::vector<ParamInfo> OutParams;
		std::unordered_map<std::string, size_t> OutParamToIndex;
		bool HasReturn = false;
	};

	std::vector<FunctionInfo> Functions;
	std::unordered_map<std::string, size_t> FunctionNameToIndex;

	struct ResourceBinding
	{
		std::string Name;
		unsigned Type = 0;
		unsigned BindPoint = 0;
		unsigned Count = 0;
	};

	std::vector<ResourceBinding> Bindings;
	std::unordered_map<std::string, size_t> BindingNameToIndex;

	struct ConstantBufferInfo
	{
		struct MemberInfo
		{
			std::string Name;
			std::string Type;
			size_t Size;
			size_t Offset;
			uint8_t Default[64]{};
		};

		std::string Name;
		size_t Size;
		unsigned Slot;
		std::vector<MemberInfo> Members;
		std::unordered_map<std::string, size_t> MemberNameToIndex;
	};

	std::vector<ConstantBufferInfo> ConstantBuffers;
	std::unordered_map<std::string, size_t> ConstantBufferNameToIndex;

	ShaderType Type = ShaderType::Unknown;
};

struct RHIShaderReflector
{
	static bool Reflect(const uint8_t* aShaderData, size_t aShaderDataSize, RHIShaderReflectionInfo& outReflectionInfo);
};