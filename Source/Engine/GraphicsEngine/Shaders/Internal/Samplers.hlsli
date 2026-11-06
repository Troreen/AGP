#ifndef SAMPLERS_HLSLI
#define SAMPLERS_HLSLI

SamplerComparisonState ShadowCmpSampler : register(s0);
SamplerState LinearClamp : register(s1);
SamplerState TrilinearWrap : register(s2);
SamplerState TrilinearClamp : register(s3);

#endif
