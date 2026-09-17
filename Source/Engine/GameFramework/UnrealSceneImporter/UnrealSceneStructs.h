#include <vector>
#include <string>
#include <variant>
#include <Vector.hpp>
#include <array>
#include <Matrix.hpp>

using MaterialParameterColorValue = CommonUtilities::Vector4f;

enum class UnrealComponentType : int64_t
{
    Custom = -1,
    SceneComponent,
    StaticMesh,
    SkeletalMesh,
    PointLight,
    SpotLight,
    DirectionalLight,
    Box,
    Sphere,
    Capsule,
    SpringArm

};

enum class MaterialType
{
    Scalar,
    Vector3f,
    Vector3d,
    Texture,
    TextureCollection,
    Font,
    RunTimeVirtualTexture,
    SparseVolumeTexture,
    StaticSwitch, //Bool
    ParameterCollection
};

struct BaseComponentData
{
    std::string name;
    UnrealComponentType typeID;
    std::string parent;
    std::vector<std::string> tags;
    CommonUtilities::Matrix4f transform;
};


struct TextureValue
{
    std::string name;
    std::string path;
};

using MaterialParameterValue = std::variant<float, CommonUtilities::Vector4f, TextureValue>;

struct MaterialParameterData
{
    std::string name;
    MaterialType type;
    MaterialParameterValue value;
};

struct MaterialData
{
    std::string name;
    std::string parent;
    std::vector<MaterialParameterData> parameters;
};

struct StaticMeshComponentData : public BaseComponentData
{
    std::string mesh;
    std::string contentPath;
    std::vector<MaterialData> materials;
};

struct CommonLightComponentData : public BaseComponentData
{
    CommonUtilities::Vector4f color;
    float intensity;
};

struct PointLightComponentData : public CommonLightComponentData
{
    float falloffExponent;
    float attenuationRadius;
};

struct SpotLightComponentData : public PointLightComponentData
{
    float innerConeAngle;
    float outerConeAngle;
};

using ImportedComponentData = std::variant<BaseComponentData, MaterialData, StaticMeshComponentData, CommonLightComponentData, PointLightComponentData, SpotLightComponentData>;

struct UnrealActorData
{
    std::string name;
    std::string archetype;
    std::vector<std::string> tags;
    CommonUtilities::Matrix4f transform;
    std::vector<ImportedComponentData> components;
};

struct UnrealSceneData
{
    std::vector<UnrealActorData> myActors;
};