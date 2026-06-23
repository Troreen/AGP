Texture2D AlbedoTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D MaterialTexture : register(t2);

TextureCube EnvCubeTexture : register(t98);
Texture2D BRDF_LUT_Texture : register(t99);

Texture2D DirectionalShadowMaps[4] : register(t100);
Texture2D SpotLightShadowMaps[4] : register(t104);
TextureCube PointLightShadowMaps[4] : register(t108);
