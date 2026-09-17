#include "GameFramework/Registration/SceneReader.h"
#include "GameFramework/Integration/SceneData.h"
#include "GameFramework/Actor.h"
#include <cmath>
#include <limits>
#include <set>

struct SceneReader::Impl
{
    const ComponentRecord& Record;
    SceneDiagnostic Source;
    SceneDiagnostics& Diagnostics;
    const AssetLookup* Assets;
    CommonUtilities::Vector2u ClientSize;
    std::function<Actor*(const std::string&)> ActorLookup;
    std::function<Component*(const ObjectAddress&)> ComponentLookup;
    std::set<std::string> Consumed;
    struct Fixup
    {
        std::string Field;
        ObjectAddress Address;
        bool ComponentTarget;
        std::function<bool(Actor*, Component*)> Assign;
    };
    std::vector<Fixup> Fixups;

    void Error(const std::string& field, std::string message, std::string code, std::string phase = "configuration")
    {
        auto error = Source;
        error.Property = Source.Property.empty() ? field : field.empty() ? Source.Property : Source.Property + "." + field;
        error.Message = std::move(message); error.Code = std::move(code); error.Phase = std::move(phase);
        Diagnostics.push_back(std::move(error));
    }
    const PropertyValue* Read(const std::string& field)
    {
        const auto it = Record.Properties.find(field);
        if (it == Record.Properties.end()) return nullptr;
        Consumed.insert(field);
        return &it->second;
    }
    template<class T> T Optional(const std::string& field, T fallback)
    {
        const auto* value = Read(field);
        if (!value) return fallback;
        if (const auto* typed = std::get_if<T>(value)) return *typed;
        Error(field, "Property has the wrong value type", "wrong-type");
        return fallback;
    }
};

SceneReader::SceneReader(const ComponentRecord& record, SceneDiagnostic source, SceneDiagnostics& diagnostics,
    const AssetLookup* assets, CommonUtilities::Vector2u clientSize,
    std::function<Actor*(const std::string&)> actorLookup,
    std::function<Component*(const ObjectAddress&)> componentLookup)
    : myImpl(std::make_unique<Impl>(Impl{record, std::move(source), diagnostics, assets, clientSize,
        std::move(actorLookup), std::move(componentLookup), {}, {}})) {}
SceneReader::~SceneReader() = default;

bool SceneReader::Has(const std::string& field) const { return myImpl->Record.Properties.contains(field); }
float SceneReader::OptionalFloat(const std::string& field, float fallback)
{
    const auto* value = myImpl->Read(field);
    if (!value) return fallback;
    double number;
    if (const auto* real = std::get_if<double>(value)) number = *real;
    else if (const auto* integer = std::get_if<int64_t>(value)) number = static_cast<double>(*integer);
    else { myImpl->Error(field, "Expected a numeric property", "wrong-type"); return fallback; }
    if (!std::isfinite(number) || std::abs(number) > std::numeric_limits<float>::max())
    { myImpl->Error(field, "Number must be finite and fit a float", "invalid-value"); return fallback; }
    return static_cast<float>(number);
}
bool SceneReader::OptionalBool(const std::string& field, bool fallback) { return myImpl->Optional(field, fallback); }
std::string SceneReader::OptionalString(const std::string& field, std::string fallback) { return myImpl->Optional(field, std::move(fallback)); }
CommonUtilities::Vector3f SceneReader::OptionalVector3(const std::string& field, CommonUtilities::Vector3f fallback)
{
    const auto value = myImpl->Optional(field, fallback);
    if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z))
    { myImpl->Error(field, "Vector must be finite", "invalid-value"); return fallback; }
    return value;
}
CommonUtilities::Quaternion<float> SceneReader::OptionalQuaternion(const std::string& field, CommonUtilities::Quaternion<float> fallback)
{
    const auto value = myImpl->Optional(field, fallback);
    if (!std::isfinite(value.w) || !std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z)
        || (value.w == 0 && value.x == 0 && value.y == 0 && value.z == 0))
    { myImpl->Error(field, "Quaternion must be finite and nonzero", "invalid-value"); return fallback; }
    return value;
}
CommonUtilities::Vector4f SceneReader::OptionalColor(const std::string& field, CommonUtilities::Vector4f fallback)
{
    const auto value = myImpl->Optional(field, fallback);
    if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z) || !std::isfinite(value.w))
    { myImpl->Error(field, "Color must be finite", "invalid-value"); return fallback; }
    return value;
}
AssetId SceneReader::OptionalAsset(const std::string& field, AssetId fallback) { return myImpl->Optional(field, std::move(fallback)); }
std::vector<AssetId> SceneReader::OptionalAssets(const std::string& field, std::vector<AssetId> fallback)
{ return myImpl->Optional(field, std::move(fallback)); }
AssetId SceneReader::RequiredAsset(const std::string& field)
{
    if (!Has(field)) { myImpl->Error(field, "Required asset property is missing", "missing-property"); return {}; }
    auto result = OptionalAsset(field);
    if (result.Value.empty()) myImpl->Error(field, "Asset identifier cannot be empty", "invalid-value");
    return result;
}
void SceneReader::Error(std::string field, std::string message) { myImpl->Error(field, std::move(message), "invalid-value"); }
const AssetLookup& SceneReader::GetAssets() const
{
    struct EmptyLookup final : AssetLookup
    {
        MeshAsset FindMesh(const AssetId&) const override { return {}; }
        MaterialAsset FindMaterial(const AssetId&) const override { return {}; }
    };
    static const EmptyLookup empty;
    return myImpl->Assets ? *myImpl->Assets : empty;
}
CommonUtilities::Vector2u SceneReader::GetClientSize() const { return myImpl->ClientSize; }
void SceneReader::BindActor(const std::string& field, ActorRef& target, ReferenceRequirement requirement)
{
    Bind(field, false, requirement, [&target](Actor* actor, Component*)
    { if (!actor) return false; target = actor->GetRef(); return true; });
}
void SceneReader::Bind(const std::string& field, bool component, ReferenceRequirement requirement,
    std::function<bool(Actor*, Component*)> assign)
{
    const auto* value = myImpl->Read(field);
    if (!value)
    {
        if (requirement == ReferenceRequirement::Required) myImpl->Error(field, "Required reference property is missing", "missing-property");
        return;
    }
    const auto* address = std::get_if<ObjectAddress>(value);
    if (!address) { myImpl->Error(field, "Expected an object address", "wrong-type"); return; }
    if (address->ActorId.empty() || (component ? address->ComponentId.empty() : !address->ComponentId.empty()))
    { myImpl->Error(field, "Object address does not match the reference kind", "invalid-reference"); return; }
    myImpl->Fixups.push_back({field, *address, component, std::move(assign)});
}
void SceneReader::Finish()
{
    for (const auto& [field, value] : myImpl->Record.Properties)
        if (!myImpl->Consumed.contains(field)) myImpl->Error(field, "Unknown or unconsumed property", "unknown-property");
}
void SceneReader::Resolve()
{
    for (const auto& fixup : myImpl->Fixups)
    {
        auto* actor = myImpl->ActorLookup(fixup.Address.ActorId);
        auto* component = fixup.ComponentTarget ? myImpl->ComponentLookup(fixup.Address) : nullptr;
        if (!actor || (fixup.ComponentTarget && !component))
            myImpl->Error(fixup.Field, "Referenced object does not exist", "missing-reference", "fixup");
        else if (!fixup.Assign(actor, component))
            myImpl->Error(fixup.Field, "Referenced component has the wrong type", "wrong-reference-type", "fixup");
    }
    myImpl->Fixups.clear();
}
