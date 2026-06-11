#pragma once
#include <functional>
#include <wrl.h>
#include "d3dcommon.h"
#include <string>
#include <filesystem>

/**
 * Handles management of the HLSL Program include file for Materials.
 */
class MaterialShaderIncludeHandler : public ID3DInclude
{
public:
	MaterialShaderIncludeHandler(const std::filesystem::path& aShaderRoot, const std::filesystem::path& aShaderProgramPath, const std::filesystem::path& aMaterialProgramPath);
	virtual ~MaterialShaderIncludeHandler() = default;

	HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override;
	HRESULT Close(LPCVOID pData) override;

private:

	struct LiveFileInfo
	{
		std::string Data;
		std::filesystem::path Path;
	};

	std::unordered_map<const void*, LiveFileInfo> myOpenFiles;

	std::filesystem::path myShaderRoot;
	std::filesystem::path myShaderProgramPath;
	std::filesystem::path myMaterialProgramPath;
};
