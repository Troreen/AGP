#include "GameFramework/UnrealSceneImporter/UnrealSceneImporter.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AssetHandling/MaterialAsset.h"
#include "GameFramework/AssetHandling/TextureAsset.h"
#include "GameFramework/Scenes/SceneWorldBuilder.h"
#include "GameFramework/World/World.h"
#include "GraphicsEngine/GraphicsEngine.h"
#include "GraphicsEngine/RHI/GraphicsCommandList.h"
#include <Windows.h>
#include <Importer.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <d3d11.h>
#include <iostream>
#include <limits>
#include <cmath>

struct CapturedFrame
{
	unsigned Width = 0;
	unsigned Height = 0;
	std::vector<unsigned char> Pixels;
};

CapturedFrame CaptureRender(World& world, RenderPassType pass, const std::filesystem::path& output, bool gameSettings = false)
{
	auto& graphics = GraphicsEngine::Get();
	GraphicsCommandList commands;
	if (!graphics.CreateCommandList("Normal diagnostic", commands))
	{
		throw std::runtime_error("Cannot create diagnostic command list");
	}
	GraphicsRenderSettings settings;
	settings.RenderPass = pass;
	settings.RunPostProcesses = gameSettings;
	settings.TonemapAlgorithm = gameSettings ? TonemapType::Unreal : TonemapType::None;
	graphics.Render(commands, world.GenerateRenderSnapshot(), settings);
	if (!commands.FinishCommandList())
	{
		throw std::runtime_error("Cannot finish diagnostic frame");
	}
	graphics.ExecuteCommandList(commands);
	graphics.SetRenderTarget(graphics.GetBackBuffer(), nullptr);
	auto* context = graphics.GetNativeDeviceContext();
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> view;
	context->OMGetRenderTargets(1, view.GetAddressOf(), nullptr);
	if (!view)
	{
		throw std::runtime_error("Cannot read actual renderer output");
	}
	Microsoft::WRL::ComPtr<ID3D11Resource> resource;
	view->GetResource(resource.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
	resource.As(&texture);
	D3D11_TEXTURE2D_DESC description{};
	texture->GetDesc(&description);
	if (description.Format != DXGI_FORMAT_R8G8B8A8_UNORM && description.Format != DXGI_FORMAT_B8G8R8A8_UNORM)
	{
		throw std::runtime_error("Unexpected framebuffer format");
	}
	const bool bgra = description.Format == DXGI_FORMAT_B8G8R8A8_UNORM;
	description.Usage = D3D11_USAGE_STAGING;
	description.BindFlags = 0;
	description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	description.MiscFlags = 0;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;
	if (FAILED(graphics.GetNativeDevice()->CreateTexture2D(&description, nullptr, staging.GetAddressOf())))
	{
		throw std::runtime_error("Cannot create framebuffer readback");
	}
	context->CopyResource(staging.Get(), texture.Get());
	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
	{
		throw std::runtime_error("Cannot map rendered frame");
	}
	CapturedFrame frame{description.Width, description.Height, {}};
	frame.Pixels.resize(frame.Width * frame.Height * 3);
	for (unsigned y = 0; y < frame.Height; ++y)
	{
		const auto* row = static_cast<const unsigned char*>(mapped.pData) + y * mapped.RowPitch;
		for (unsigned x = 0; x < frame.Width; ++x)
		{
			for (unsigned channel = 0; channel < 3; ++channel)
			{
				frame.Pixels[(y * frame.Width + x) * 3 + channel] = row[x * 4 + (bgra ? 2 - channel : channel)];
			}
		}
	}
	context->Unmap(staging.Get(), 0);
	std::ofstream image(output, std::ios::binary);
	image << "P6\n" << frame.Width << ' ' << frame.Height << "\n255\n";
	image.write(reinterpret_cast<const char*>(frame.Pixels.data()), frame.Pixels.size());
	return frame;
}

size_t ChangedPixels(const CapturedFrame& before, const CapturedFrame& after)
{
	size_t count = 0;
	for (size_t pixel = 0; pixel < before.Pixels.size(); pixel += 3)
	{
		if (before.Pixels[pixel] != after.Pixels[pixel] || before.Pixels[pixel + 1] != after.Pixels[pixel + 1] ||
			before.Pixels[pixel + 2] != after.Pixels[pixel + 2])
		{
			++count;
		}
	}
	return count;
}

int wmain(int argc, wchar_t** argv)
{
	if (argc != 3 && argc != 4 && argc != 5)
	{
		return 2;
	}
	UnrealSceneImporter sceneImporter;
	const UnrealImportResult scene = sceneImporter.ImportScene(argv[1]);
	if (!scene)
	{
		for (const ImportDiagnostic& diagnostic : scene.Diagnostics)
		{
			std::cerr << diagnostic.Context << ": " << diagnostic.Message << '\n';
		}
		return 1;
	}
	std::cout << "Scene import passed: " << scene.Data->Actors.size() << " actors, "
		<< scene.Data->Materials.size() << " master materials\n";
	TGA::FBX::Importer::InitImporter();
	TGA::FBX::Mesh mesh;
	if (!TGA::FBX::Importer::LoadMesh(argv[2], mesh) || !mesh.IsValid())
	{
		std::cerr << TGA::FBX::Importer::GetLastSDKError() << '\n';
		TGA::FBX::Importer::UninitImporter();
		return 1;
	}
	std::array<float, 3> minimum;
	std::array<float, 3> maximum;
	minimum.fill(std::numeric_limits<float>::max());
	maximum.fill(std::numeric_limits<float>::lowest());
	size_t vertices = 0;
	for (const TGA::FBX::Mesh::Element& element : mesh.Elements)
	{
		vertices += element.Vertices.size();
		for (const TGA::FBX::Vertex& vertex : element.Vertices)
		{
			for (size_t axis = 0; axis < 3; ++axis)
			{
				minimum[axis] = std::min(minimum[axis], vertex.Position[axis]);
				maximum[axis] = std::max(maximum[axis], vertex.Position[axis]);
			}
		}
	}
	std::cout << "Runtime FBX import passed: " << vertices << " vertices, " << mesh.Elements.size() << " elements\n";
	std::array<double, 3> averageNormal{};
	for (const auto& element : mesh.Elements)
	{
		for (const auto& vertex : element.Vertices)
		{
			for (size_t axis = 0; axis < 3; ++axis)
			{
				averageNormal[axis] += vertex.Normal[axis];
			}
		}
	}
	std::cout << "Imported vertex mean normal: " << averageNormal[0] / vertices << ", "
		<< averageNormal[1] / vertices << ", " << averageNormal[2] / vertices << '\n';
	double alignmentSum = 0;
	double totalArea = 0;
	for (const auto& element : mesh.Elements)
	{
		for (size_t index = 0; index + 2 < element.Indices.size(); index += 3)
		{
			const auto& a = element.Vertices[element.Indices[index]];
			const auto& b = element.Vertices[element.Indices[index + 1]];
			const auto& c = element.Vertices[element.Indices[index + 2]];
			std::array<double, 3> ab{}, ac{}, normal{};
			for (size_t axis = 0; axis < 3; ++axis)
			{
				ab[axis] = b.Position[axis] - a.Position[axis];
				ac[axis] = c.Position[axis] - a.Position[axis];
				normal[axis] = a.Normal[axis] + b.Normal[axis] + c.Normal[axis];
			}
			const std::array<double, 3> face{ab[1] * ac[2] - ab[2] * ac[1], ab[2] * ac[0] - ab[0] * ac[2], ab[0] * ac[1] - ab[1] * ac[0]};
			double faceLength = 0, normalLength = 0, dot = 0;
			for (size_t axis = 0; axis < 3; ++axis)
			{
				faceLength += face[axis] * face[axis];
				normalLength += normal[axis] * normal[axis];
				dot += face[axis] * normal[axis];
			}
			faceLength = std::sqrt(faceLength);
			normalLength = std::sqrt(normalLength);
			if (faceLength > 0 && normalLength > 0)
			{
				alignmentSum += std::abs(dot) / normalLength;
				totalArea += faceLength;
			}
		}
	}
	std::cout << "Area-weighted face/vertex normal alignment: " << alignmentSum / totalArea << '\n';
	if (argc == 5 && std::wstring_view(argv[3]) == L"--compare-geometry")
	{
		TGA::FBX::Mesh candidate;
		if (!TGA::FBX::Importer::LoadMesh(argv[4], candidate) || candidate.Elements.size() != mesh.Elements.size())
		{
			return 1;
		}
		auto samples = [](const TGA::FBX::Mesh& imported)
		{
			std::vector<std::array<float, 7>> values;
			for (const auto& element : imported.Elements)
			{
				for (const auto& vertex : element.Vertices)
				{
					values.push_back({vertex.Position[0], vertex.Position[1], vertex.Position[2], vertex.UVs[0][0], vertex.UVs[0][1], vertex.UVs[1][0], vertex.UVs[1][1]});
				}
			}
			std::sort(values.begin(), values.end());
			return values;
		};
		const auto originalSamples = samples(mesh);
		const auto candidateSamples = samples(candidate);
		if (originalSamples.size() != candidateSamples.size())
		{
			return 1;
		}
		double maximumPositionDifference = 0;
		double maximumUvDifference = 0;
		for (size_t vertex = 0; vertex < originalSamples.size(); ++vertex)
		{
			for (size_t axis = 0; axis < 7; ++axis)
			{
				const double difference = std::abs(originalSamples[vertex][axis] - candidateSamples[vertex][axis]);
				if (axis < 3)
				{
					maximumPositionDifference = std::max(maximumPositionDifference, difference);
				}
				else
				{
					maximumUvDifference = std::max(maximumUvDifference, difference);
				}
			}
		}
		std::cout << "Geometry comparison: vertices=" << originalSamples.size() << ", max position difference="
			<< maximumPositionDifference << " cm, max UV0/UV1 difference=" << maximumUvDifference << '\n';
		TGA::FBX::Importer::UninitImporter();
		return maximumPositionDifference > 0.002 || maximumUvDifference > 0.000001 ? 1 : 0;
	}
	if (argc == 4 && std::wstring_view(argv[3]) == L"--mesh-normals")
	{
		TGA::FBX::Importer::UninitImporter();
		return alignmentSum / totalArea < 0.8 ? 1 : 0;
	}
	std::cout << "AGP XYZ bounds: min [" << minimum[0] << ", " << minimum[1] << ", " << minimum[2]
		<< "], max [" << maximum[0] << ", " << maximum[1] << ", " << maximum[2] << "]\n";
	const HWND window = CreateWindowExW(0, L"STATIC", L"Prepared terrain validation", WS_OVERLAPPEDWINDOW,
		0, 0, 128, 128, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
	try
	{
		if (!window || !GraphicsEngine::Get().Initialize(window, "Source/Engine/GraphicsEngine/Shaders"))
		{
			throw std::runtime_error("Cannot initialize DirectX verification device");
		}
		const std::filesystem::path contentRoot = std::filesystem::path(argv[2]).parent_path().parent_path()
			.parent_path().parent_path();
		AssetRegistry assets;
		assets.Initialize(contentRoot);
		SceneWorldBuilder builder;
		if (!builder.Initialize("Bin/Settings/MaterialNameConversions.json"))
		{
			throw std::runtime_error("Cannot load material name conversions");
		}
		SceneFallbackAssets fallbacks;
		fallbacks.MissingShader = "Source/Engine/GraphicsEngine/Shaders/Material/Material.hlsli";
		const auto atlas = assets.GetAsset<TextureAsset>("T_Lvl_Splatmap_Testing2_Weightmap_Combined");
		const auto white = assets.GetAsset<TextureAsset>("T_Lvl_Splatmap_Testing2_Debug_BaseColor");
		const bool surfaceMode = std::any_of(scene.Data->Materials.begin(), scene.Data->Materials.end(),
			[](const MasterMaterialRecord& material) { return material.Name == "MM_Landscape"; });
		std::vector<std::pair<std::string, std::shared_ptr<TextureAsset>>> surfaceTextures;
		if (surfaceMode)
		{
			for (const std::string layer : {"Dunes", "Rough", "Cliff"})
			{
				for (const std::string role : {"BaseColor", "Normal", "ORM"})
				{
					const std::string suffix = role == "ORM" ? "OcclusionRoughnessMetallic" : role;
					surfaceTextures.emplace_back(layer + role, assets.GetAsset<TextureAsset>("T_8M_Snow_" + layer + "_" + suffix));
				}
			}
			surfaceTextures.emplace_back("BlendNoise", assets.GetAsset<TextureAsset>("T_Splatmap_TransitionNoise"));
		}
		auto world = builder.BuildWorld(*scene.Data, assets, contentRoot / "Shaders", {128, 128}, fallbacks);
		const auto material = assets.GetAsset<MaterialAsset>(surfaceMode ? "MM_Landscape_Inst" : "M_PaintedLandscapeWeights_Inst");
		if (!material || !atlas || !white ||
			material->GetMaterial()->GetTexture("SplatMask") != atlas->GetTexture() ||
			material->GetMaterial()->GetTexture(0) != white->GetTexture() ||
			!material->GetMaterial()->GetParameterByName("MB_MaskU") ||
			!material->GetMaterial()->GetParameterByName("MB_MaskV"))
		{
			throw std::runtime_error("Prepared material mapping or DDS texture binding failed");
		}
		for (const auto& [role, texture] : surfaceTextures)
		{
			if (!texture || material->GetMaterial()->GetTexture(role) != texture->GetTexture())
			{
				throw std::runtime_error("Surface texture binding failed: " + role);
			}
		}
		if (surfaceMode)
		{
			const MaterialParameterInfo* mapping = material->GetMaterial()->GetParameterByName("MB_SurfaceMapping");
			std::array<float, 4> surfaceMapping{};
			if (!mapping || mapping->Size != sizeof(surfaceMapping))
			{
				throw std::runtime_error("Surface mapping parameter missing");
			}
			std::memcpy(surfaceMapping.data(), material->GetMaterial()->GetParameterDataBlock() + mapping->Offset, sizeof(surfaceMapping));
			std::cout << "Surface mapping: " << surfaceMapping[0] << ", " << surfaceMapping[1] << ", "
				<< surfaceMapping[2] << ", " << surfaceMapping[3] << '\n';
			const MaterialParameterInfo* debug = material->GetMaterial()->GetParameterByName("MB_DEBUG");
			float debugValue = -1;
			if (debug && debug->Size == sizeof(debugValue))
			{
				std::memcpy(&debugValue, material->GetMaterial()->GetParameterDataBlock() + debug->Offset, sizeof(debugValue));
			}
			std::cout << "Surface debug mode: " << debugValue << '\n';
			if (std::wstring_view(argv[1]).find(L"_Runtime_RGB_") != std::wstring_view::npos && debugValue != 2)
			{
				throw std::runtime_error("Lit RGB scene did not bind the raw-weight base-color debug mode");
			}
			if (std::abs(surfaceMapping[0] - 0.00125f) > 1.0e-7f || std::abs(surfaceMapping[1] - 0.00125f) > 1.0e-7f)
			{
				throw std::runtime_error("Surface maps do not use the required 8-metre repeat; zero mapping samples one texel across all terrain");
			}
			const MaterialParameterInfo* channels = material->GetMaterial()->GetParameterByName("MB_RoughChannels");
			std::array<float, 4> values{};
			if (!channels || channels->Size != sizeof(values))
			{
				throw std::runtime_error("Rough channel selector missing");
			}
			std::memcpy(values.data(), material->GetMaterial()->GetParameterDataBlock() + channels->Offset, sizeof(values));
			if (values != std::array<float, 4>{1, 0, 0, 0})
			{
				throw std::runtime_error("Rough channel selector did not bind the exported red channel");
			}
		}
		std::cout << "DirectX scene construction, mask mapping and " << surfaceTextures.size()
			<< " surface/noise texture bindings passed\n";
		if (argc == 4 && std::wstring_view(argv[3]) == L"--rgb-surface")
		{
			const std::filesystem::path output = ".scratch/splatmap-v2/rgb-surface-diagnosis";
			std::filesystem::create_directories(output);
			world->BeginPlay();
			world->Update(0.0f);
			const auto instance = std::dynamic_pointer_cast<MaterialInstance>(material->GetMaterial());
			if (!instance)
			{
				throw std::runtime_error("Expected a Landscape material instance");
			}
			for (const auto pass : {RenderPassType::Albedo, RenderPassType::NormalWorld, RenderPassType::Roughness,
				RenderPassType::Metalness, RenderPassType::AmbientOcclusionTexture})
			{
				instance->SetValue("MB_DEBUG", 0.0f);
				const auto before = CaptureRender(*world, pass, output / (std::to_string(static_cast<int>(pass)) + "-surface.ppm"));
				instance->SetValue("MB_DEBUG", 2.0f);
				const auto after = CaptureRender(*world, pass, output / (std::to_string(static_cast<int>(pass)) + "-rgb.ppm"));
				const size_t changed = ChangedPixels(before, after);
				std::cout << "RGB base-color comparison pass " << static_cast<int>(pass) << ": changed pixels=" << changed << '\n';
				if ((pass == RenderPassType::Albedo && changed < 100) || (pass != RenderPassType::Albedo && changed != 0))
				{
					throw std::runtime_error("RGB debug must change base color and retain identical normals/ORM");
				}
			}
		}
		else if (argc == 4 && std::wstring_view(argv[3]) == L"--terrain-light")
		{
			const std::filesystem::path output = ".scratch/splatmap-v2/terrain-light-diagnosis";
			std::filesystem::create_directories(output);
			world->BeginPlay();
			world->Update(0.0f);
			CU::Vector4f anchor{0, 0, 0, 1};
			float nearest = std::numeric_limits<float>::max();
			for (const auto& element : mesh.Elements)
			{
				for (const auto& vertex : element.Vertices)
				{
					const float distance = vertex.Position[0] * vertex.Position[0] + vertex.Position[2] * vertex.Position[2];
					if (distance < nearest)
					{
						nearest = distance;
						anchor = {vertex.Position[0], vertex.Position[1], vertex.Position[2], 1};
					}
				}
			}
			auto* camera = world->GetActiveCamera();
			auto& transform = camera->GetOwner()->GetTransform();
			transform.SetLocalPosition({0, 2000, -9000});
			double minimumLight = 1.0;
			double maximumLight = 0.0;
			double minimumAlbedo = 1.0;
			double maximumAlbedo = 0.0;
			std::ofstream measurements(output / "fixed-point.csv");
			measurements << "yaw,x,y,litR,litG,litB,albedoR,albedoG,albedoB,normalR,normalG,normalB\n";
			for (int yaw = -30; yaw <= 30; yaw += 5)
			{
				transform.SetLocalRotationDegrees(static_cast<float>(yaw), 12, 0);
				const auto lit = CaptureRender(*world, RenderPassType::Light, output / ("lit-" + std::to_string(yaw) + ".ppm"), true);
				const auto albedo = CaptureRender(*world, RenderPassType::Albedo, output / ("albedo-" + std::to_string(yaw) + ".ppm"), true);
				const auto normals = CaptureRender(*world, RenderPassType::NormalWorld, output / ("normal-" + std::to_string(yaw) + ".ppm"), true);
				const CU::Vector4f clip = anchor * camera->GetView() * camera->GetProjection();
				const int x = static_cast<int>((clip.x / clip.w * 0.5f + 0.5f) * lit.Width);
				const int y = static_cast<int>((0.5f - clip.y / clip.w * 0.5f) * lit.Height);
				if (x < 0 || y < 0 || x >= static_cast<int>(lit.Width) || y >= static_cast<int>(lit.Height))
				{
					throw std::runtime_error("Diagnostic anchor left the frame");
				}
				const size_t pixel = (y * lit.Width + x) * 3;
				const double lightValue = (lit.Pixels[pixel] + lit.Pixels[pixel + 1] + lit.Pixels[pixel + 2]) / 765.0;
				const double albedoValue = (albedo.Pixels[pixel] + albedo.Pixels[pixel + 1] + albedo.Pixels[pixel + 2]) / 765.0;
				minimumLight = std::min(minimumLight, lightValue);
				maximumLight = std::max(maximumLight, lightValue);
				minimumAlbedo = std::min(minimumAlbedo, albedoValue);
				maximumAlbedo = std::max(maximumAlbedo, albedoValue);
				measurements << yaw << ',' << x << ',' << y;
				for (const auto* frame : {&lit, &albedo, &normals})
				{
					for (int channel = 0; channel < 3; ++channel)
					{
						measurements << ',' << static_cast<int>(frame->Pixels[pixel + channel]);
					}
				}
				measurements << '\n';
				std::cout << "Fixed terrain point yaw " << yaw << ": lit=" << lightValue << ", albedo=" << albedoValue << '\n';
			}
			if (maximumLight > 0.05 && minimumLight < maximumLight * 0.35 && minimumAlbedo > maximumAlbedo * 0.6)
			{
				throw std::runtime_error("The same terrain point darkens when only the camera orientation changes");
			}
		}
		else if (argc == 4 && std::wstring_view(argv[3]).starts_with(L"--light-"))
		{
			const std::filesystem::path output = std::filesystem::path(".scratch/splatmap-v2/light-diagnosis") / argv[3];
			std::filesystem::create_directories(output);
			world->BeginPlay();
			world->Update(0.0f);
			auto& cameraTransform = world->GetActiveCamera()->GetOwner()->GetTransform();
			const bool gameSettings = std::wstring_view(argv[3]).ends_with(L"-full");
			if (std::wstring_view(argv[3]) != L"--light-debug")
			{
				cameraTransform.SetLocalPosition(std::wstring_view(argv[3]) == L"--light-inside-full"
					? CU::Vector3f{0, 2000, 0} : CU::Vector3f{0, 2000, -9000});
				cameraTransform.SetLocalRotationDegrees(0, 12, 0);
			}
			const auto original = cameraTransform.GetLocalMatrix();
			double minimumRatio = 1.0;
			double maximumRatio = 0.0;
			std::ofstream measurements(output / "camera-turn.csv");
			measurements << "angle,litEnergy,unshadowedEnergy,shadowRatio\n";
			for (int angle = 0; angle < 360; angle += 30)
			{
				auto rotated = original;
				const float radians = angle * 3.14159265358979323846f / 180.0f;
				for (int row = 1; row <= 3; ++row)
				{
					rotated(row, 1) = original(row, 1) * std::cos(radians) + original(row, 3) * std::sin(radians);
					rotated(row, 3) = -original(row, 1) * std::sin(radians) + original(row, 3) * std::cos(radians);
				}
				cameraTransform.SetMatrix(rotated);
				const auto lit = CaptureRender(*world, RenderPassType::Light, output / ("lit-" + std::to_string(angle) + ".ppm"), gameSettings);
				const auto lights = world->GenerateRenderSnapshot().Lights;
				std::vector<bool> shadowFlags;
				for (auto* light : lights)
				{
					shadowFlags.push_back(light->CastShadows());
					light->SetCastShadows(false);
				}
				const auto unshadowed = CaptureRender(*world, RenderPassType::Light, output / ("unshadowed-" + std::to_string(angle) + ".ppm"), gameSettings);
				for (size_t i = 0; i < lights.size(); ++i)
				{
					lights[i]->SetCastShadows(shadowFlags[i]);
				}
				double litEnergy = 0;
				double unshadowedEnergy = 0;
				for (size_t i = 0; i < lit.Pixels.size(); ++i)
				{
					litEnergy += lit.Pixels[i];
					unshadowedEnergy += unshadowed.Pixels[i];
				}
				const double ratio = unshadowedEnergy > 10000 ? litEnergy / unshadowedEnergy : 1.0;
				if (unshadowedEnergy > 10000)
				{
					minimumRatio = std::min(minimumRatio, ratio);
					maximumRatio = std::max(maximumRatio, ratio);
				}
				measurements << angle << ',' << litEnergy << ',' << unshadowedEnergy << ',' << ratio << '\n';
				std::cout << "Camera turn " << angle << ": lit=" << litEnergy << ", without shadows=" << unshadowedEnergy << ", ratio=" << ratio << '\n';
			}
			cameraTransform.SetMatrix(original);
			if (maximumRatio > 0 && minimumRatio < maximumRatio * 0.75)
			{
				throw std::runtime_error("Turning the camera in place causes large directional-shadow darkening");
			}
		}
		else if (argc == 4)
		{
			const std::filesystem::path output = ".scratch/splatmap-v2/normal-diagnosis";
			std::filesystem::create_directories(output);
			world->BeginPlay();
			world->Update(0.0f);
			const auto localBefore = CaptureRender(*world, RenderPassType::NormalLocal, output / "local-before.ppm");
			const auto worldBefore = CaptureRender(*world, RenderPassType::NormalWorld, output / "world-before.ppm");
			const auto normalPath = output / "tilted-normal.dds";
			std::ifstream whiteInput(std::filesystem::path(argv[1]).parent_path() / "T_Lvl_Splatmap_Testing2_Debug_BaseColor.dds", std::ios::binary);
			std::vector<char> normalBytes((std::istreambuf_iterator<char>(whiteInput)), {});
			if (normalBytes.size() != 152)
			{
				throw std::runtime_error("Expected 1x1 DX10 RGBA source for normal diagnostic");
			}
			normalBytes[148] = static_cast<char>(230);
			normalBytes[149] = static_cast<char>(128);
			normalBytes[150] = static_cast<char>(255);
			normalBytes[151] = static_cast<char>(255);
			std::ofstream normalOutput(normalPath, std::ios::binary);
			normalOutput.write(normalBytes.data(), normalBytes.size());
			normalOutput.close();
			auto normal = std::make_shared<Texture>();
			if (!GraphicsEngine::Get().LoadTexture(normalPath, *normal))
			{
				throw std::runtime_error("Cannot load diagnostic normal texture");
			}
			for (const std::string role : {"DunesNormal", "RoughNormal", "CliffNormal"})
			{
				material->GetMaterial()->SetTexture(role, normal);
			}
			const auto localAfter = CaptureRender(*world, RenderPassType::NormalLocal, output / "local-after.ppm");
			const auto worldAfter = CaptureRender(*world, RenderPassType::NormalWorld, output / "world-after.ppm");
			const size_t localChanged = ChangedPixels(localBefore, localAfter);
			const size_t worldChanged = ChangedPixels(worldBefore, worldAfter);
			std::cout << "Normal diagnostic: changed local pixels=" << localChanged << ", changed world pixels=" << worldChanged << '\n';
			material->GetMaterial()->SetTexture(1, normal);
			const auto baseSlotChanged = CaptureRender(*world, RenderPassType::NormalLocal, output / "local-base-slot.ppm");
			std::cout << "Changing only legacy slot 1 changed local pixels=" << ChangedPixels(localAfter, baseSlotChanged) << '\n';
			if (worldChanged < 100)
			{
				throw std::runtime_error("Diagnostic did not exercise the terrain normal maps");
			}
			if (localChanged < 100)
			{
				throw std::runtime_error("Local-normal view ignores the Landscape material's normal maps");
			}
		}
		TGA::FBX::Importer::UninitImporter();
		DestroyWindow(window);
	}
	catch (const std::exception& error)
	{
		TGA::FBX::Importer::UninitImporter();
		if (window)
		{
			DestroyWindow(window);
		}
		std::cerr << error.what() << '\n';
		return 1;
	}
	return 0;
}
