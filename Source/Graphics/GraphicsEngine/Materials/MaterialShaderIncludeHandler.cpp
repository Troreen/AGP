#include "GraphicsEngine.pch.h"
#include "MaterialShaderIncludeHandler.h"
#include "d3dcompiler.h"

using namespace Microsoft::WRL;

MaterialShaderIncludeHandler::MaterialShaderIncludeHandler(const std::filesystem::path& aShaderRoot, const std::filesystem::path& aShaderProgramPath, const std::filesystem::path& aMaterialProgramPath)
	: myShaderRoot(aShaderRoot)
	, myShaderProgramPath(aShaderProgramPath)
	, myMaterialProgramPath(aMaterialProgramPath)
{
		
}

HRESULT MaterialShaderIncludeHandler::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
	// Not used.
	IncludeType;

	const std::filesystem::path programPath = myShaderProgramPath.parent_path();

	std::filesystem::path sourcePath = pFileName;

	// Transform the file name to lowercase for comparison.
	std::string fileName = sourcePath.filename().string();
	std::ranges::transform(fileName, fileName.begin(), tolower);

	// Our generic include name is material.hlsli
	// so if the shader requests this file give it the myMaterialProgramPath instead.
	// This will ensure we load the custom program.
	if (fileName == "material.hlsli")
	{
		sourcePath = myMaterialProgramPath;
	}
	else
	{		
		// Build the include folder for the file name.
		sourcePath = programPath / pFileName;
	}

	// Make sure the file exists
	bool found = false;
	// Check if this is a relative include and, if so, update the path.
	if (pParentData)
	{
		const auto it = myOpenFiles.find(pParentData);
		if (it != myOpenFiles.end())
		{
			const std::filesystem::path& parentPath = it->second.Path.parent_path();
			sourcePath = std::filesystem::weakly_canonical(parentPath / pFileName);
			
			// If this file doesn't exist, check if it's relative to the shader path.
			if (!std::filesystem::exists(sourcePath))
			{
				sourcePath = std::filesystem::weakly_canonical(myShaderRoot / pFileName);
			}
		}
	}

	std::ostringstream outputStream;
	// Make sure the file exists.
	if (!std::filesystem::exists(sourcePath))
	{
		// Patch to try and show the correct file we were looking for.
		outputStream << "#line 1 \"" << sourcePath.generic_string() << "\"\n";
		outputStream << "#pragma message(\"" << "Material Include Handler could not locate " << sourcePath.generic_string() << "\")\n";
		outputStream << "#pragma message(\"" << "\t\t\tWhile compiling: " << myShaderProgramPath.generic_string() << "\")\n";
		outputStream << "#pragma message(\"" << "\t\t\tFor Material: " << myMaterialProgramPath.generic_string() << "\")\n";
		outputStream << "#error \"Material Include Handler reported error!\"\n";
	}
	else
	{
		std::ifstream file(sourcePath, std::ios::binary);
		outputStream << file.rdbuf();
		file.close();
	}

	LiveFileInfo info;
	info.Data = outputStream.str();
	info.Path = sourcePath;

	*ppData = info.Data.data();
	auto it = myOpenFiles.emplace(*ppData, std::move(info));
	*pBytes = static_cast<unsigned>(it.first->second.Data.size());

	return S_OK;
}

HRESULT MaterialShaderIncludeHandler::Close(LPCVOID pData)
{
	auto it = myOpenFiles.find(pData);
	if (it == myOpenFiles.end())
		return E_FAIL;

	myOpenFiles.erase(it);
	return S_OK;
}
