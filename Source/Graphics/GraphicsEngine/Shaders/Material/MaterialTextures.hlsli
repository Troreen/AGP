// Material textures stay in t0-t15. Global PBL textures sit immediately before
// the shadow map block, which starts at t100.
static const uint MATERIAL_ALBEDO_TEXTURE_SLOT = 0;
static const uint MATERIAL_NORMAL_TEXTURE_SLOT = 1;
static const uint MATERIAL_ORM_TEXTURE_SLOT = 2;
static const uint GLOBAL_ENV_CUBE_TEXTURE_SLOT = 98;
static const uint GLOBAL_BRDF_LUT_TEXTURE_SLOT = 99;
static const uint SHADOW_TEXTURE_SLOT_START = 100;

Texture2D AlbedoTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D MaterialTexture : register(t2);

TextureCube EnvCubeTexture : register(t98);
Texture2D BRDF_LUT_Texture : register(t99);

Texture2D DirectionalShadowMaps[4] : register(t100);
Texture2D SpotLightShadowMaps[4] : register(t104);
TextureCube PointLightShadowMaps[4] : register(t108);
