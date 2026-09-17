#include "Runtime/Internal/RenderAccess.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "Scenes/SceneBuilder.h"
#include "GameFramework/Registration/SceneReader.h"
#include "GameFramework/Registration/References.h"
#include "Runtime/Internal/RegistryAccess.h"
#include "Runtime/Internal/WorldAccess.h"
#include "GameFramework/Integration/SceneData.h"
#include "GameFramework/Integration/AssetBindings.h"
#include "GraphicsEngine/Objects/Mesh.h"
#include "GraphicsEngine/Objects/Vertex.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

void Check(bool condition,const char* message);
void Near(float actual,float expected,const char* message);
namespace
{
using GameFrameworkInternal::WorldAccess;
using GameFrameworkInternal::RegistryAccess;
struct AuthoredProbe final : Component
{
    inline static int Alive=0,Begins=0,Ends=0,Resolved=0;
    inline static ComponentRef<AuthoredProbe> LastConfigured;
    float Speed=25,ExpectedPeerSpeed=0;
    bool ThrowAllocationInResolve=false;
    ActorRef Target;
    ComponentRef<AuthoredProbe> Peer;
    AuthoredProbe(){++Alive;}
    ~AuthoredProbe() override {--Alive;}
    void ResolveReferences(References& references) override
    {
        ++Resolved;
        if(ThrowAllocationInResolve) throw std::bad_alloc{};
        if(ExpectedPeerSpeed!=0 && (!Peer.Get() || Peer.Get()->Speed!=ExpectedPeerSpeed))
            references.Error("peer","Peer was not completely configured before resolution");
    }
    void BeginPlay() override {++Begins;}
    void EndPlay() override {++Ends;}
};
void ReadProbe(AuthoredProbe& probe,SceneReader& data)
{
    probe.Speed=data.OptionalFloat("speed",25);
    probe.ExpectedPeerSpeed=data.OptionalFloat("expected-peer-speed",0);
    data.BindActor("target",probe.Target,ReferenceRequirement::Optional);
    data.BindComponent<AuthoredProbe>("peer",probe.Peer,ReferenceRequirement::Optional);
    AuthoredProbe::LastConfigured=probe.GetRef<AuthoredProbe>();
}
ComponentRegistry MakeRegistry()
{
    ComponentRegistry registry;GameFrameworkInternal::RegisterBuiltInComponents(registry);
    registry.Register<AuthoredProbe>("test.Probe",ReadProbe);
    registry.Register<AuthoredProbe>("test.Required",[](AuthoredProbe& probe,SceneReader& data)
    {data.BindActor("target",probe.Target,ReferenceRequirement::Required);});
    registry.Register<AuthoredProbe>("test.ThrowReader",[](AuthoredProbe&,SceneReader&)
    {throw std::runtime_error("Expected property reader failure");});
    bool duplicate=false,empty=false,frozen=false;
    try {registry.Register<AuthoredProbe>("agp.Camera");} catch(const std::invalid_argument&){duplicate=true;}
    try {registry.Register<AuthoredProbe>("");} catch(const std::invalid_argument&){empty=true;}
    RegistryAccess::Freeze(registry);
    try {registry.Register<AuthoredProbe>("Too late");} catch(const std::logic_error&){frozen=true;}
    Check(duplicate && empty && frozen,"Registry setup accepted duplicate, empty or late registration");return registry;
}
ComponentRecord Record(std::string id,std::string name,std::string type)
{
    ComponentRecord result;result.Id=std::move(id);result.Name=std::move(name);result.Type=std::move(type);
    result.Source={"cpu-fixture.scene","source-actor","source-component",""};return result;
}
SceneData Fixture()
{
    SceneData scene;scene.Source.File="cpu-fixture.scene";scene.Version="test-1";
    ActorRecord first;first.Id="first-id";first.Name="Repeated";first.Source={"cpu-fixture.scene","source-first","",""};
    first.Components.push_back(Record("logic","Logic","test.Probe"));
    ActorRecord second;second.Id="second-id";second.Name="Repeated";second.Source={"cpu-fixture.scene","source-second","",""};
    second.Components.push_back(Record("logic","Logic","test.Probe"));
    scene.Actors={first,second};return scene;
}
void ResetCounters()
{
    Check(AuthoredProbe::Alive==0,"Previous candidate leaked allocations");
    AuthoredProbe::Begins=AuthoredProbe::Ends=AuthoredProbe::Resolved=0;AuthoredProbe::LastConfigured={};
}
void ExpectRejected(const SceneData& scene,const ComponentRegistry& registry,const char* code=nullptr,const char* field=nullptr)
{
    ResetCounters();auto result=SceneBuilder::Build(scene,registry);
    Check(!result && !result.Candidate && !result.Diagnostics.empty(),"Malformed scene produced a candidate or lost diagnostics");
    Check(AuthoredProbe::Alive==0 && AuthoredProbe::Begins==0 && AuthoredProbe::Ends==0 && !AuthoredProbe::LastConfigured.Get(),"Rejected data began gameplay or retained candidate references");
    if(code) Check(std::any_of(result.Diagnostics.begin(),result.Diagnostics.end(),[&](const auto& d){return d.Code==code && (!field || d.Property==field);}),"Expected structured property/reference diagnostic missing");
    Check(std::all_of(result.Diagnostics.begin(),result.Diagnostics.end(),[](const auto& d){return !d.Code.empty() && !d.File.empty();}),"Content diagnostics lost error code or source file");
}
void ConstructionAndReferences(const ComponentRegistry& registry)
{
    ResetCounters();
    {
        SceneData empty;auto built=SceneBuilder::Build(empty,registry);Check(bool(built),"Empty CPU scene required a camera");
        WorldAccess::Activate(*built.Candidate);
    }
    {
        auto scene=Fixture();
        auto& first=scene.Actors[0].Components[0];auto& second=scene.Actors[1].Components[0];
        first.Properties["speed"]=int64_t{12};first.Properties["expected-peer-speed"]=23.0;
        first.Properties["target"]=ObjectAddress{"second-id",""};first.Properties["peer"]=ObjectAddress{"second-id","logic"};
        second.Properties["speed"]=23.0;second.Properties["expected-peer-speed"]=12.0;
        second.Properties["target"]=ObjectAddress{"first-id",""};second.Properties["peer"]=ObjectAddress{"first-id","logic"};
        auto built=SceneBuilder::Build(scene,registry);Check(bool(built),"Forward/cyclic ID references rejected");
        Check(AuthoredProbe::Begins==0 && AuthoredProbe::Resolved==2 && WorldAccess::GetState(*built.Candidate)==WorldAccess::State::Prepared,"Builder began gameplay or skipped resolution");
        auto actors=built.Candidate->FindActors("Repeated");Check(actors.size()==2 && !built.Candidate->FindActor("Repeated"),"Authored duplicate labels confused identity");
        auto* a=actors[0]->GetComponent<AuthoredProbe>();auto* b=actors[1]->GetComponent<AuthoredProbe>();
        Check(a->Target.Get()==actors[1] && b->Target.Get()==actors[0] && a->Peer.Get()==b && b->Peer.Get()==a,"ID-based reference fixups used display names");
        Near(a->Speed,12,"Integer numeric property was not accepted as float");
        WorldAccess::Activate(*built.Candidate);Check(AuthoredProbe::Begins==2,"Valid data did not begin exactly once");
        auto oldActor=actors[1]->GetRef();auto oldComponent=b->GetRef<AuthoredProbe>();actors[1]->Destroy();
        Check(!a->Target.Get() && !a->Peer.Get() && !oldActor.Get() && !oldComponent.Get(),"Removing target left authored references live");
        SceneDiagnostics diagnostics;Check(WorldAccess::Flush(*built.Candidate,diagnostics),"Removal boundary failed");
    }
    Check(AuthoredProbe::Alive==0 && AuthoredProbe::Ends==2,"Constructed world teardown did not pair started components");
    ResetCounters();
    {
        auto built=SceneBuilder::Build(Fixture(),registry);Check(bool(built),"Optional absent properties rejected");
        Near(built.Candidate->FindActors("Repeated")[0]->GetComponent<AuthoredProbe>()->Speed,25,"Absent optional field did not use default");
    }
}
void MalformedData(const ComponentRegistry& registry)
{
    auto scene=Fixture();scene.Actors[0].Components[0].Properties["speed"]=std::string("fast");ExpectRejected(scene,registry,"wrong-type","speed");
    scene=Fixture();scene.Actors[0].Components[0].Properties["speed"]=std::numeric_limits<double>::infinity();ExpectRejected(scene,registry,"invalid-value","speed");
    scene=Fixture();scene.Actors[0].Components[0].Properties["typo"]=true;ExpectRejected(scene,registry,"unknown-property","typo");
    scene=Fixture();scene.Actors[0].Components[0].Properties["target"]=ObjectAddress{"missing",""};ExpectRejected(scene,registry,"missing-reference","target");
    scene=Fixture();scene.Actors[0].Components[0].Properties["target"]=ObjectAddress{"second-id","logic"};ExpectRejected(scene,registry,"invalid-reference","target");
    scene=Fixture();scene.Actors[0].Components[0].Properties["peer"]=ObjectAddress{"second-id",""};ExpectRejected(scene,registry,"invalid-reference","peer");
    scene=Fixture();scene.Actors[1].Components.push_back(Record("spatial","Spatial","agp.Scene"));scene.Actors[0].Components[0].Properties["peer"]=ObjectAddress{"second-id","spatial"};ExpectRejected(scene,registry,"wrong-reference-type","peer");
    scene=Fixture();scene.Actors[0].Components[0].Type="test.Required";ExpectRejected(scene,registry,"missing-property","target");
    scene=Fixture();scene.Actors[0].Components[0].Type="Unknown";ExpectRejected(scene,registry);
    scene=Fixture();scene.Actors[1].Id=scene.Actors[0].Id;ExpectRejected(scene,registry);
    scene=Fixture();scene.Actors[0].Components.push_back(scene.Actors[0].Components[0]);ExpectRejected(scene,registry);
    scene=Fixture();auto duplicateName=Record("another-id","Logic","test.Probe");scene.Actors[0].Components.push_back(duplicateName);ExpectRejected(scene,registry);
    scene=Fixture();scene.Actors[0].Pose.Position.x=std::numeric_limits<float>::quiet_NaN();ExpectRejected(scene,registry);
    scene=Fixture();scene.Actors[0].Components[0].Pose=LocalPose{};ExpectRejected(scene,registry);
    scene=Fixture();scene.Actors[0].Parent="second-id";scene.Actors[1].Parent="first-id";ExpectRejected(scene,registry);
    scene=Fixture();scene.Actors[0].Components[0].Type="test.ThrowReader";ExpectRejected(scene,registry);
    scene=Fixture();scene.RequireCamera=true;ExpectRejected(scene,registry);
    scene=Fixture();scene.ActiveCamera=ObjectAddress{"first-id","logic"};ExpectRejected(scene,registry);
    scene=Fixture();scene.Actors[0].Components.push_back(Record("mesh","Visual","agp.StaticMesh"));scene.Actors[0].Components.back().Properties["mesh"]=AssetId{"missing.mesh"};ExpectRejected(scene,registry);
    // Independent bad fields must report deterministically rather than stop at the first reader failure.
    scene=Fixture();scene.Actors[0].Components[0].Properties["speed"]=true;scene.Actors[1].Components[0].Properties["unexpected"]=false;
    auto first=SceneBuilder::Build(scene,registry);auto second=SceneBuilder::Build(scene,registry);
    Check(!first && !second && first.Diagnostics.size()>=2 && first.Diagnostics.size()==second.Diagnostics.size(),"Independent data errors were not aggregated");
    for(size_t i=0;i<first.Diagnostics.size();++i)
        Check(first.Diagnostics[i].Code==second.Diagnostics[i].Code && first.Diagnostics[i].Property==second.Diagnostics[i].Property && first.Diagnostics[i].Actor==second.Diagnostics[i].Actor,"Diagnostic order changed between identical builds");
}
void CameraPolicy(const ComponentRegistry& registry)
{
    auto scene=Fixture();scene.Actors[0].Components.push_back(Record("camera","View","agp.Camera"));
    scene.ActiveCamera=ObjectAddress{"first-id","camera"};scene.RequireCamera=true;
    {auto built=SceneBuilder::Build(scene,registry);Check(bool(built) && built.Camera.Get(),"Explicit valid camera rejected");}
    scene.Actors[0].Components.back().Enabled=false;ExpectRejected(scene,registry);
    scene.Actors[0].Components.back().Enabled=true;scene.Actors[0].Active=false;ExpectRejected(scene,registry);
    scene.Actors[0].Active=true;scene.Actors[0].Components.back().Properties["fov"]=180.0;ExpectRejected(scene,registry,"invalid-value","fov");
    scene.Actors[0].Components.back().Properties.clear();scene.Actors[0].Components.back().Properties["near"]=0.0;ExpectRejected(scene,registry,"invalid-value","near");
    scene.Actors[0].Components.back().Properties.clear();scene.Actors[0].Components.back().Properties["near"]=5.0;scene.Actors[0].Components.back().Properties["far"]=2.0;ExpectRejected(scene,registry,"invalid-value","far");
    scene.Actors[0].Components.back().Properties.clear();
    SceneBuildServices services;services.ClientSize={0,720};auto invalidSize=SceneBuilder::Build(scene,registry,services);
    Check(!invalidSize && std::any_of(invalidSize.Diagnostics.begin(),invalidSize.Diagnostics.end(),[](const auto& d){return d.Property=="resolution";}),"Invalid camera resolution reached backend projection");
    scene=Fixture();auto light=Record("light","Light","agp.PointLight");light.Properties["radius"]=0.0;scene.Actors[0].Components.push_back(light);ExpectRejected(scene,registry,"invalid-value","radius");
    scene=Fixture();scene.Actors[0].Components.push_back(Record("camera","View","agp.Camera"));
    auto& projection=scene.Actors[0].Components.back().Properties;
    projection["near"]=1e30;projection["far"]=2e30;ExpectRejected(scene,registry,"invalid-value","projection");
    projection.clear();projection["fov"]=1e-38;ExpectRejected(scene,registry,"invalid-value","projection");
    auto worldStorage=GameFrameworkInternal::WorldAccess::Create(); World& world=*worldStorage;auto* camera=world.SpawnActor("Camera")->AddComponent<CameraComponent>();
    Check(camera->SetPerspective(90,1,5000,{1280,720}),"Valid direct projection rejected");
    const auto before=GameFrameworkInternal::RenderAccess::Camera(*camera).GetProjectionMatrix();
    Check(!camera->SetPerspective(90,1e30f,2e30f,{1280,720}) && !camera->SetPerspective(1e-38f,1,5000,{1280,720}),"Finite inputs yielding nonfinite projection accepted");
    const auto after=GameFrameworkInternal::RenderAccess::Camera(*camera).GetProjectionMatrix();
    for(int r=1;r<=4;++r)for(int c=1;c<=4;++c)Near(after(r,c),before(r,c),"Rejected projection changed camera");
}
void MaterialSlotFailures(const ComponentRegistry& registry)
{
    // Existing mesh metadata can be prepared without creating any D3D device.
    auto mesh=std::make_shared<Mesh>();
    mesh->Initialize("one material slot",std::vector<Mesh::Element>{{}},std::vector<Vertex>{},std::vector<unsigned>{});
    GameFrameworkIntegration::AssetBindings assets;assets.BindMesh(AssetId{"fixture.mesh"},mesh);
    SceneBuildServices services;services.Assets=&assets;
    SceneData scene;scene.Source.File="asset-fixture.scene";ActorRecord actor;actor.Id="prop";actor.Name="Prop";
    auto visual=Record("visual","Visual","agp.StaticMesh");visual.Properties["mesh"]=AssetId{"fixture.mesh"};actor.Components.push_back(visual);scene.Actors.push_back(actor);
    auto slots=SceneBuilder::Build(scene,registry,services);
    Check(!slots && std::any_of(slots.Diagnostics.begin(),slots.Diagnostics.end(),[](const auto& d){return d.Code=="invalid-value" && d.Property=="materials";}),"Material count mismatch reached component binding");
    scene.Actors[0].Components[0].Properties["materials"]=std::vector<AssetId>{AssetId{"missing.material"}};
    auto missing=SceneBuilder::Build(scene,registry,services);
    Check(!missing && std::any_of(missing.Diagnostics.begin(),missing.Diagnostics.end(),[](const auto& d){return d.Code=="invalid-value" && d.Property=="materials[0]";}),"Missing material slot asset accepted");
}
void ConstructionCannotMutateLiveWorld()
{
    for(int phase=0;phase<3;++phase)
    {
        auto liveStorage=GameFrameworkInternal::WorldAccess::Create(); World& live=*liveStorage;auto* existing=live.SpawnActor("Live");auto reference=existing->GetRef();
        auto* capturedSkin=existing->AddComponent<SkeletalMeshComponent>();
        ComponentRegistry registry;
        if(phase==0)
            registry.RegisterFactory<AuthoredProbe>("test.CapturedMutation",[&]
            {
                try {existing->Destroy();}catch(const std::logic_error&){}
                return std::make_unique<AuthoredProbe>();
            });
        else
            registry.Register<AuthoredProbe>("test.CapturedMutation",[&](AuthoredProbe&,SceneReader&)
            {
                try {if(phase==1)existing->GetTransform().SetLocalPosition({10,20,30});else capturedSkin->Update(.5f);}catch(const std::logic_error&){}
            });
        RegistryAccess::Freeze(registry);
        auto scene=Fixture();scene.Actors.resize(1);scene.Actors[0].Components[0].Type="test.CapturedMutation";
        auto built=SceneBuilder::Build(scene,registry);
        Check(!built && !built.Diagnostics.empty() && reference.Get()==existing && existing->IsActive(),"Captured factory/reader mutation damaged live world");
        Near(existing->GetTransform().GetLocalPosition().LengthSqr(),0,"Property reader modified external pose");
        Check(AuthoredProbe::Alive==0,"Rejected construction retained candidate allocation");
    }
    ComponentRegistry nullFactory;nullFactory.RegisterFactory<AuthoredProbe>("test.Null",[]{return std::unique_ptr<AuthoredProbe>{};});RegistryAccess::Freeze(nullFactory);
    auto scene=Fixture();scene.Actors.resize(1);scene.Actors[0].Components[0].Type="test.Null";
    auto failed=SceneBuilder::Build(scene,nullFactory);Check(!failed && !failed.Diagnostics.empty(),"Null component factory accepted");
    for(int phase=0;phase<3;++phase)
    {
        ComponentRegistry allocationFailure;
        if(phase==0) allocationFailure.RegisterFactory<AuthoredProbe>("test.AllocationFailure",[]()->std::unique_ptr<AuthoredProbe>{throw std::bad_alloc{};});
        else if(phase==1) allocationFailure.Register<AuthoredProbe>("test.AllocationFailure",[](AuthoredProbe&,SceneReader&){throw std::bad_alloc{};});
        else allocationFailure.Register<AuthoredProbe>("test.AllocationFailure",[](AuthoredProbe& probe,SceneReader&){probe.ThrowAllocationInResolve=true;});
        RegistryAccess::Freeze(allocationFailure);scene.Actors[0].Components[0].Type="test.AllocationFailure";
        bool propagated=false;try{SceneBuilder::Build(scene,allocationFailure);}catch(const std::bad_alloc&){propagated=true;}
        Check(propagated && AuthoredProbe::Alive==0,"Allocation failure became recoverable data error or leaked component");
    }
}
}
void SceneConstructionTests()
{
    auto registry=MakeRegistry();ConstructionAndReferences(registry);MalformedData(registry);CameraPolicy(registry);MaterialSlotFailures(registry);ConstructionCannotMutateLiveWorld();
    Check(AuthoredProbe::Alive==0,"Scene construction tests leaked components");
}
