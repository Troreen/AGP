#pragma once
#include "Vector.hpp"

#include <cstdint>
#include <string>
#include <wrl.h>

struct ID3D11SamplerState;

enum class SamplerAddressMode : uint8_t
{
	Wrap = 1,
	Mirror = 2,
	Clamp = 3,
	Border = 4,
};

enum class SamplerFilterMode : uint8_t
{
	Point = 0,
	Linear = 20,
	Trilinear = 21,
	Anisotropic = 85,
	ComparisonLinearPoint = 148,
};

enum class SamplerComparisonFunc : uint8_t
{
	Never = 1,
	Less = 2,
	Equal = 3,
	LessEqual = 4,
	Greater = 5,
	NotEqual = 6,
	GreaterEqual = 7,
	Always = 8,
};

struct SamplerDescription
{
	std::string Name;
	SamplerFilterMode FilterMode = SamplerFilterMode::Trilinear;
	SamplerAddressMode AddressMode = SamplerAddressMode::Wrap;
	SamplerComparisonFunc ComparisonFunction = SamplerComparisonFunc::Never;
	CU::Vector4f BorderColor = CU::Vector4f::One;
};

class Sampler
{
	friend class RenderHardwareInterface;
	friend class GraphicsCommandList;
public:
	Sampler();
	~Sampler();

private:

	Microsoft::WRL::ComPtr<ID3D11SamplerState> mySampler;
	SamplerDescription myDescription;

};

