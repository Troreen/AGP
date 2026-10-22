#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

using Microsoft::WRL::ComPtr;

namespace
{
	using Vector = std::array<float, 4>;

	void Check(HRESULT aResult)
	{
		if (FAILED(aResult))
		{
			throw std::runtime_error("D3D11 operation failed: " + std::to_string(aResult));
		}
	}

	std::string ReadFile(const std::filesystem::path& aPath)
	{
		std::ifstream input(aPath, std::ios::binary);
		if (!input)
		{
			throw std::runtime_error("Cannot open " + aPath.string());
		}
		return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
	}

	// Resolve the authored material and engine helpers without copying or rewriting either.
	class ShaderIncludes final : public ID3DInclude
	{
	public:
		ShaderIncludes(std::filesystem::path aMaterial, std::filesystem::path anEngineDirectory)
		    : myMaterial(std::move(aMaterial)), myEngineDirectory(std::move(anEngineDirectory))
		{
		}

		HRESULT __stdcall Open(D3D_INCLUDE_TYPE, LPCSTR aName, LPCVOID, LPCVOID* someData, UINT* aSize) override
		{
			try
			{
				const std::filesystem::path path = std::string(aName) == "TestMaterial.hlsli" ? myMaterial : myEngineDirectory / aName;
				const std::string source = ReadFile(path);
				char* data = new char[source.size()];
				std::copy(source.begin(), source.end(), data);
				*someData = data;
				*aSize = static_cast<UINT>(source.size());
				return S_OK;
			}
			catch (...)
			{
				return E_FAIL;
			}
		}

		HRESULT __stdcall Close(LPCVOID someData) override
		{
			delete[] static_cast<const char*>(someData);
			return S_OK;
		}

	private:
		std::filesystem::path myMaterial;
		std::filesystem::path myEngineDirectory;
	};

	ComPtr<ID3DBlob> Compile(const std::string& aSource, const char* anEntry, const char* aTarget, ShaderIncludes& someIncludes)
	{
		ComPtr<ID3DBlob> bytecode;
		ComPtr<ID3DBlob> errors;
		const HRESULT result = D3DCompile(aSource.data(), aSource.size(), "TerrainMaterialReadback", nullptr, &someIncludes, anEntry,
		                                  aTarget, D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0, &bytecode, &errors);
		if (FAILED(result))
		{
			throw std::runtime_error(errors ? static_cast<const char*>(errors->GetBufferPointer()) : "Shader compilation failed");
		}
		return bytecode;
	}

	struct MaterialConstants
	{
		Vector MaskU = {0, 0, 0, 0.5f};
		Vector MaskV = {0, 0, 0, 0.5f};
		Vector SurfaceMapping = {1.0f / 800, 1.0f / 800, 3.5f, 3.5f};
		Vector NoiseControls = {0, 800, 0, 0};
		Vector Debug = {0, 0, 0, 0};
	};

	struct PixelInputs
	{
		Vector WorldPosition = {0, 0, 0, 1};
		Vector Normal = {0, 1, 0, 0};
		Vector Tangent = {1, 0, 0, 0};
		Vector Bitangent = {0, 0, 1, 0};
		Vector MeshUV = {0, 0, 0, 0};
	};

	class MaterialReadback
	{
	public:
		MaterialReadback(const std::filesystem::path& aMaterial, const std::filesystem::path& anEngineDirectory)
		{
			Check(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &myDevice, nullptr,
			                        &myContext));
			ShaderIncludes includes(aMaterial, anEngineDirectory);
			const std::string source = R"(
#include "TestMaterial.hlsli"
cbuffer TestInputs : register(b0)
{
    float4 TestWorld;
    float4 TestNormal;
    float4 TestTangent;
    float4 TestBitangent;
    float4 TestUV;
};
float4 Vertex(uint id : SV_VertexID) : SV_Position
{
    return float4(id == 1 ? 3 : -1, id == 2 ? 3 : -1, 0, 1);
}
struct Outputs
{
    float4 Color : SV_Target0;
    float4 Surface : SV_Target1;
    float4 Normal : SV_Target2;
};
Outputs Pixel()
{
    MaterialPixelParameters material = (MaterialPixelParameters)0;
    material.PixelColor = 1;
    material.VertexColor = 1;
    material.WorldPosition = TestWorld;
    material.VertexNormal = TestNormal.xyz;
    material.Normal = TestNormal.xyz;
    material.Tangent = TestTangent.xyz;
    material.BiNormal = TestBitangent.xyz;
    material.UV0 = TestUV.xy;
    Material_Pixel(material);
    Outputs result;
    result.Color = material.PixelColor;
    result.Surface = material.SurfaceValues;
    result.Normal = float4(material.Normal, 1);
    return result;
}
)";
			const auto vertexCode = Compile(source, "Vertex", "vs_5_0", includes);
			const auto pixelCode = Compile(source, "Pixel", "ps_5_0", includes);
			Check(myDevice->CreateVertexShader(vertexCode->GetBufferPointer(), vertexCode->GetBufferSize(), nullptr, &myVertexShader));
			Check(myDevice->CreatePixelShader(pixelCode->GetBufferPointer(), pixelCode->GetBufferSize(), nullptr, &myPixelShader));
			myMaterialBuffer = CreateBuffer(sizeof(MaterialConstants));
			myInputBuffer = CreateBuffer(sizeof(PixelInputs));

			D3D11_TEXTURE2D_DESC texture = {};
			texture.Width = texture.Height = texture.MipLevels = texture.ArraySize = texture.SampleDesc.Count = 1;
			texture.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			texture.BindFlags = D3D11_BIND_RENDER_TARGET;
			for (size_t index = 0; index < myTargets.size(); ++index)
			{
				Check(myDevice->CreateTexture2D(&texture, nullptr, &myTargets[index]));
				Check(myDevice->CreateRenderTargetView(myTargets[index].Get(), nullptr, &myViews[index]));
			}
			texture.BindFlags = 0;
			texture.Usage = D3D11_USAGE_STAGING;
			texture.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			Check(myDevice->CreateTexture2D(&texture, nullptr, &myStaging));

			D3D11_SAMPLER_DESC sampler = {};
			sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			sampler.MaxLOD = D3D11_FLOAT32_MAX;
			Check(myDevice->CreateSamplerState(&sampler, &myWrap));
			sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			Check(myDevice->CreateSamplerState(&sampler, &myClamp));
			D3D11_RASTERIZER_DESC rasterizer = {};
			rasterizer.FillMode = D3D11_FILL_SOLID;
			rasterizer.CullMode = D3D11_CULL_NONE;
			rasterizer.DepthClipEnable = true;
			Check(myDevice->CreateRasterizerState(&rasterizer, &myRasterizer));
		}

		void SetTexture(size_t anIndex, const Vector& aValue)
		{
			SetTexture(anIndex, {aValue, aValue});
		}

		void SetTexture(size_t anIndex, const std::array<Vector, 2>& someValues)
		{
			D3D11_TEXTURE2D_DESC texture = {};
			texture.Width = 2;
			texture.Height = texture.MipLevels = texture.ArraySize = texture.SampleDesc.Count = 1;
			texture.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			texture.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			const D3D11_SUBRESOURCE_DATA data = {someValues.data(), sizeof(someValues), 0};
			ComPtr<ID3D11Texture2D> resource;
			Check(myDevice->CreateTexture2D(&texture, &data, &resource));
			Check(myDevice->CreateShaderResourceView(resource.Get(), nullptr, &myTextures.at(anIndex)));
		}

		std::array<Vector, 3> Draw(const MaterialConstants& someConstants, const PixelInputs& someInputs = {})
		{
			myContext->UpdateSubresource(myMaterialBuffer.Get(), 0, nullptr, &someConstants, 0, 0);
			myContext->UpdateSubresource(myInputBuffer.Get(), 0, nullptr, &someInputs, 0, 0);
			ID3D11Buffer* materialBuffer = myMaterialBuffer.Get();
			ID3D11Buffer* inputBuffer = myInputBuffer.Get();
			myContext->PSSetConstantBuffers(4, 1, &materialBuffer);
			myContext->PSSetConstantBuffers(0, 1, &inputBuffer);
			std::array<ID3D11ShaderResourceView*, 11> textures;
			for (size_t index = 0; index < textures.size(); ++index)
			{
				textures[index] = myTextures[index].Get();
			}
			myContext->PSSetShaderResources(4, static_cast<UINT>(textures.size()), textures.data());
			ID3D11SamplerState* samplers[] = {myWrap.Get(), myClamp.Get()};
			myContext->PSSetSamplers(2, 2, samplers);
			ID3D11RenderTargetView* views[] = {myViews[0].Get(), myViews[1].Get(), myViews[2].Get()};
			myContext->OMSetRenderTargets(3, views, nullptr);
			const D3D11_VIEWPORT viewport = {0, 0, 1, 1, 0, 1};
			myContext->RSSetViewports(1, &viewport);
			myContext->RSSetState(myRasterizer.Get());
			myContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			myContext->VSSetShader(myVertexShader.Get(), nullptr, 0);
			myContext->PSSetShader(myPixelShader.Get(), nullptr, 0);
			myContext->Draw(3, 0);

			std::array<Vector, 3> result;
			for (size_t index = 0; index < result.size(); ++index)
			{
				myContext->CopyResource(myStaging.Get(), myTargets[index].Get());
				D3D11_MAPPED_SUBRESOURCE mapped = {};
				Check(myContext->Map(myStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped));
				std::copy_n(static_cast<const float*>(mapped.pData), 4, result[index].begin());
				myContext->Unmap(myStaging.Get(), 0);
			}
			return result;
		}

	private:
		ComPtr<ID3D11Buffer> CreateBuffer(UINT aSize)
		{
			D3D11_BUFFER_DESC description = {};
			description.ByteWidth = aSize;
			description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			ComPtr<ID3D11Buffer> buffer;
			Check(myDevice->CreateBuffer(&description, nullptr, &buffer));
			return buffer;
		}

		ComPtr<ID3D11Device> myDevice;
		ComPtr<ID3D11DeviceContext> myContext;
		ComPtr<ID3D11VertexShader> myVertexShader;
		ComPtr<ID3D11PixelShader> myPixelShader;
		ComPtr<ID3D11Buffer> myMaterialBuffer;
		ComPtr<ID3D11Buffer> myInputBuffer;
		std::array<ComPtr<ID3D11Texture2D>, 3> myTargets;
		std::array<ComPtr<ID3D11RenderTargetView>, 3> myViews;
		ComPtr<ID3D11Texture2D> myStaging;
		std::array<ComPtr<ID3D11ShaderResourceView>, 11> myTextures;
		ComPtr<ID3D11SamplerState> myWrap;
		ComPtr<ID3D11SamplerState> myClamp;
		ComPtr<ID3D11RasterizerState> myRasterizer;
	};

	void Expect(const char* aName, const Vector& anActual, const Vector& anExpected)
	{
		for (size_t component = 0; component < 3; ++component)
		{
			if (!std::isfinite(anActual[component]) || std::abs(anActual[component] - anExpected[component]) > 0.0001f)
			{
				throw std::runtime_error(std::string(aName) + " component " + std::to_string(component) + ": expected " +
				                         std::to_string(anExpected[component]) + ", got " + std::to_string(anActual[component]));
			}
		}
		std::cout << "PASS: " << aName << '\n';
	}
}

int main(int anArgumentCount, char** someArguments)
{
	try
	{
		if (anArgumentCount != 3)
		{
			throw std::runtime_error("Usage: TerrainMaterialTests.exe <material.hlsli> <engine material include directory>");
		}
		MaterialReadback renderer(someArguments[1], someArguments[2]);
		for (size_t layer = 0; layer < 3; ++layer)
		{
			Vector color = {0, 0, 0, 1};
			color[layer] = 1;
			renderer.SetTexture(layer * 3, color);
			renderer.SetTexture(layer * 3 + 1, color);
			renderer.SetTexture(layer * 3 + 2, Vector{0.5f, 0.5f, 1, 1});
		}
		renderer.SetTexture(9, Vector{0.3f, 0.5f, 0.25f, 1});
		renderer.SetTexture(10, Vector{0.5f, 0.5f, 0.5f, 1});
		MaterialConstants constants;
		const auto overlap = renderer.Draw(constants);
		Expect("ordered Rough/Cliff overlap base color", overlap[0], {0.375f, 0.375f, 0.25f, 1});
		Expect("ordered Rough/Cliff overlap ORM", overlap[1], {0.375f, 0.375f, 0.25f, 1});
		Expect("flat detail preserves horizontal normal", overlap[2], {0, 1, 0, 1});

		constants.NoiseControls[0] = 1;
		renderer.SetTexture(10, Vector{1, 1, 1, 1});
		renderer.SetTexture(9, Vector{0, 0, 0, 1});
		Expect("noise does not add absent layers", renderer.Draw(constants)[0], {1, 0, 0, 1});
		renderer.SetTexture(10, Vector{0, 0, 0, 1});
		renderer.SetTexture(9, Vector{0, 1, 0, 1});
		Expect("noise preserves fully painted Rough", renderer.Draw(constants)[0], {0, 1, 0, 1});
		renderer.SetTexture(9, Vector{0, 1, 1, 1});
		Expect("noise preserves fully painted Cliff", renderer.Draw(constants)[0], {0, 0, 1, 1});

		PixelInputs slope;
		slope.Normal = {0, 0.8f, 0.6f, 0};
		Expect("flat detail preserves geometric slope", renderer.Draw(constants, slope)[2], {0, 0.8f, 0.6f, 1});

		// A known 3-4-5 tilt points along world U even if the floater's mesh axes turn.
		renderer.SetTexture(8, Vector{0.8f, 0.5f, 1, 1});
		PixelInputs floater;
		floater.Tangent = {0, 0, -1, 0};
		floater.Bitangent = {1, 0, 0, 0};
		floater.MeshUV = {0.73f, 0.19f, 0, 0};
		Expect("detail normal follows world axes", renderer.Draw(constants)[2], {0, 0.8f, 0.6f, 1});
		Expect("rotated floater keeps world normal detail", renderer.Draw(constants, floater)[2], {0, 0.8f, 0.6f, 1});

		constants.NoiseControls[0] = 0;
		renderer.SetTexture(9, Vector{0, 0, 0, 1});
		renderer.SetTexture(0, std::array<Vector, 2>{Vector{1, 0, 0, 1}, Vector{0, 1, 0, 1}});
		PixelInputs placement;
		placement.WorldPosition = {0, 0, 0, 1};
		Expect("surface uses native Landscape phase", renderer.Draw(constants, placement)[0], {0, 1, 0, 1});
		placement.WorldPosition[2] = 400;
		Expect("surface world U changes at half repeat", renderer.Draw(constants, placement)[0], {1, 0, 0, 1});
		placement.WorldPosition[2] = 800;
		placement.MeshUV = {0.25f, 0.75f, 0, 0};
		Expect("surface repeats after 800 world units independently of mesh UV", renderer.Draw(constants, placement)[0], {0, 1, 0, 1});

		// Two known noise texels make strength and patch-size controls observable.
		renderer.SetTexture(0, Vector{1, 0, 0, 1});
		renderer.SetTexture(9, Vector{0, 0.5f, 0, 1});
		renderer.SetTexture(10, std::array<Vector, 2>{Vector{0, 0, 0, 1}, Vector{1, 1, 1, 1}});
		placement.WorldPosition[2] = 600;
		constants.NoiseControls = {1, 800, 0, 0};
		Expect("noise texture reshapes a painted transition", renderer.Draw(constants, placement)[0], {0.25f, 0.75f, 0, 1});
		constants.NoiseControls[0] = 0.5f;
		Expect("noise strength controls transition displacement", renderer.Draw(constants, placement)[0], {0.375f, 0.625f, 0, 1});
		constants.NoiseControls = {1, 1600, 0, 0};
		Expect("noise patch size changes world sampling", renderer.Draw(constants, placement)[0], {0.75f, 0.25f, 0, 1});
		constants.NoiseControls[0] = 0;
		Expect("zero noise strength restores baseline", renderer.Draw(constants, placement)[0], {0.5f, 0.5f, 0, 1});

		renderer.SetTexture(9, std::array<Vector, 2>{Vector{0, 1, 0, 1}, Vector{0, 0, 1, 1}});
		constants.MaskU = {0.001f, 0, 0, 0};
		placement.WorldPosition = {900, 0, 250, 1};
		Expect("mask U maps AGP world Z", renderer.Draw(constants, placement)[0], {0, 1, 0, 1});
		placement.WorldPosition[2] = 750;
		Expect("mask remains separate from repeating surface transform", renderer.Draw(constants, placement)[0], {0, 0, 1, 1});

		// Opposite horizontal normals can cancel. The visible fallback must stay finite and upward.
		renderer.SetTexture(9, Vector{0, 0.5f, 0, 1});
		renderer.SetTexture(2, Vector{1, 0.5f, 0, 1});
		renderer.SetTexture(5, Vector{0, 0.5f, 0, 1});
		Expect("cancelled detail normals use geometric normal", renderer.Draw(constants)[2], {0, 1, 0, 1});

		std::cout << "GPU readback uses D3D11 WARP; authored scene appearance and hardware timing remain separate checks.\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
