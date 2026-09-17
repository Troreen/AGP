#include "GameFramework/Scenes/SceneBuilder.h"
#include "GameFramework/Scenes/ConnectionContext.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Runtime/Internal/GameLoop.h"
#include "GameFramework/Runtime/Internal/WorldAccess.h"
#include "GameFramework/Runtime/Internal/RegistryAccess.h"
using GameFrameworkInternal::WorldAccess;
using GameFrameworkInternal::RegistryAccess;
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

void Check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
void Near(float actual, float expected, const char* message)
{
    Check(std::abs(actual-expected) < .001f,message);
}
void MatrixNear(const CommonUtilities::Matrix4f& a, const CommonUtilities::Matrix4f& b)
{
    for (int r=1;r<=4;++r) for (int c=1;c<=4;++c) Near(a(r,c),b(r,c),"World pose changed unexpectedly");
}
void Start(World& world)
{
    SceneDiagnostics diagnostics;
    Check(WorldAccess::Prepare(world,diagnostics),"Valid world failed preparation");
    WorldAccess::Activate(world);
}
void Flush(World& world)
{
    SceneDiagnostics diagnostics;
    Check(WorldAccess::Flush(world,diagnostics),"Valid mutation batch failed");
}
struct Probe : Component
{
    std::string* Trace = nullptr;
    std::function<void()> OnConnect, OnBegin, OnFixed, OnUpdate, OnLate, OnEnd, OnCleanup;
    void Record(const char* phase) { if (Trace) *Trace += GetName() + phase + " "; }
    void Connect(ConnectionContext&) override { Record("C"); if (OnConnect) OnConnect(); }
    void BeginPlay() override { Record("B"); if (OnBegin) OnBegin(); }
    void FixedUpdate(float) override { Record("F"); if (OnFixed) OnFixed(); }
    void Update(float) override { Record("U"); if (OnUpdate) OnUpdate(); }
    void LateUpdate(float) override { Record("L"); if (OnLate) OnLate(); }
    void EndPlay() override { Record("E"); if (OnEnd) OnEnd(); }
    void OnDestroy() override { Record("D"); if (OnCleanup) OnCleanup(); }
};
struct Consumer : Component
{
    std::string ActorName, ComponentName;
    bool IsOptional = false;
    ComponentHandle<Probe> Dependency;
    void Connect(ConnectionContext& context) override
    {
        if (ActorName.empty()) Dependency = IsOptional ? context.Optional<Probe>(ComponentName) : context.Require<Probe>(ComponentName);
        else Dependency = IsOptional ? context.Optional<Probe>(ActorName,ComponentName) : context.Require<Probe>(ActorName,ComponentName);
    }
};
struct Invalid : Component
{
    void Connect(ConnectionContext& context) override { context.Error("required","Deliberate invalid fixture"); }
};

void Lifecycle()
{
    std::string trace;
    World world;
    auto* first=world.CreateActor("First");
    auto* a=first->AddComponent<Probe>("A"); a->Trace=&trace;
    auto* b=first->AddComponent<Probe>("B"); b->Trace=&trace; b->SetEnabled(false);
    auto* second=world.CreateActor("Second"); second->SetActive(false);
    auto* c=second->AddComponent<Probe>("C"); c->Trace=&trace;
    WorldAccess::Update(world,.02f); Check(trace.empty(),"Construction ticked");
    Start(world); Check(trace=="AC BC CC AB BB CB ","Connect/Begin order or disabled initialization incorrect");
    trace.clear(); WorldAccess::FixedUpdate(world,.02f); WorldAccess::Update(world,.02f);
    Check(trace=="AF AU AL ","Disabled/inactive objects ticked");
    b->SetEnabled(true); second->SetActive(true); trace.clear();
    WorldAccess::FixedUpdate(world,.02f); WorldAccess::Update(world,.02f);
    Check(trace=="AF BF CF AU BU CU AL BL CL ","Phase order changed");
    trace.clear(); WorldAccess::Shutdown(world);
    Check(trace.find("CE BE AE ")==0,"EndPlay not reverse activation order");
    Check(!world.CreateActor("After shutdown"),"Shutdown allowed spawn");
}
void ClosingWorld()
{
    World world;
    auto* actor=world.SpawnActor("Retained for shutdown");
    auto* component=actor->AddComponent<Probe>("Existing");
    auto actorRef=actor->GetRef();
    auto componentRef=component->GetRef<Probe>();
    Start(world);
    WorldAccess::Close(world);
    Check(!world.SpawnActor("Rejected") && !actor->AddComponent<Probe>("Rejected"),"Closing world accepted construction");
    Check(actorRef.Get()==actor && componentRef.Get()==component,"Closing discarded shutdown borrows");
    WorldAccess::Shutdown(world);
    Check(!actorRef.Get() && !componentRef.Get(),"Shutdown retained live references");
}
void MutationsAndHandles()
{
    std::string trace;
    ActorHandle expired;
    ComponentHandle<Probe> expiredComponent;
    {
        World world;
        auto* actor=world.CreateActor("Actor"); expired=actor->GetHandle();
        auto* a=actor->AddComponent<Probe>("A"); a->Trace=&trace;
        auto* b=actor->AddComponent<Probe>("B"); b->Trace=&trace;
        expiredComponent=b->GetHandle<Probe>();
        Check(!ComponentHandle<SceneComponent>(expiredComponent).Get(),"Wrong-type handle resolved");
        a->OnUpdate=[&] { b->Destroy(); b->Destroy(); a->Destroy(); actor->AddComponent<Probe>("Next")->Trace=&trace; };
        Start(world); trace.clear(); WorldAccess::Update(world,.01f);
        Check(trace=="AU ","Destroyed later component or pending spawn ticked");
        Check(!expiredComponent.Get() && !actor->FindComponent("B"),"Destroy did not invalidate lookup immediately");
        trace.clear(); Flush(world); WorldAccess::Update(world,.01f);
        Check(trace.find("BE AE")!=std::string::npos && trace.find("NextB")!=std::string::npos && trace.find("NextU")!=std::string::npos,"Boundary did not end old and begin new components");
        auto* replacement=actor->AddComponent<Probe>("B");
        Check(!expiredComponent.Get() && replacement,"Slot reuse revived an old handle");
        auto* doomed=world.CreateActor("Never started");
        doomed->AddComponent<Probe>("Doomed")->Trace=&trace; doomed->Destroy(); trace.clear(); Flush(world);
        Check(trace.find("DoomedB")==std::string::npos && trace.find("DoomedE")==std::string::npos,"Destroyed pending object started");
        auto* invalid=actor->AddComponent<Invalid>("Invalid"); auto invalidHandle=invalid->GetHandle();
        SceneDiagnostics errors; Check(!WorldAccess::Flush(world,errors) && !errors.empty() && !invalidHandle.Get(),"Invalid runtime batch survived");
        Check(actor->GetHandle().Get()==actor,"Invalid batch destroyed existing actor");
    }
    Check(!expired.Get() && !expiredComponent.Get(),"World destruction left handles alive");
}
void FrozenBoundaries()
{
    for (int phase=0;phase<6;++phase)
    {
        World world;
        auto* actor=world.CreateActor("Actor"); auto* source=actor->AddComponent<Probe>("Source");
        ComponentHandle<Probe> spawned;
        auto spawn=[&] { if (!spawned.Get()) spawned=actor->AddComponent<Probe>("Spawned")->GetHandle<Probe>(); };
        if (phase==0) source->OnConnect=spawn;
        if (phase==1) source->OnBegin=spawn;
        if (phase==2) source->OnFixed=spawn;
        if (phase==3) source->OnUpdate=spawn;
        if (phase==4) source->OnLate=spawn;
        if (phase==5) source->OnCleanup=spawn;
        Start(world);
        WorldAccess::FixedUpdate(world,.01f); WorldAccess::Update(world,.01f);
        if (phase==5) { source->Destroy(); Flush(world); }
        Check(spawned.Get() && !spawned.Get()->HasBegunPlay(),"Lifecycle addition joined the same frozen batch");
        Flush(world); Check(spawned.Get()->HasBegunPlay(),"Lifecycle addition never activated");
        WorldAccess::Shutdown(world);
    }
    // One boundary before all catch-up fixed steps, not one boundary per step.
    World world; auto* a=world.CreateActor("A"); auto* p=a->AddComponent<Probe>("P");
    ComponentHandle<Probe> spawned; int ticks=0;
    p->OnFixed=[&] { if (!spawned.Get()) { auto* n=a->AddComponent<Probe>("N"); n->OnFixed=[&]{++ticks;}; spawned=n->GetHandle<Probe>(); } };
    Start(world); GameFrameworkInternal::GameLoop loop(.01f); GameInput input;
    Flush(world); loop.Advance(.035f,input,[&](float dt,const auto&){WorldAccess::FixedUpdate(world,dt);},[&](float dt,const auto&){WorldAccess::Update(world,dt);},[](float,const auto&){});
    Check(ticks==0,"Spawn ticked inside same catch-up frame"); Flush(world); WorldAccess::FixedUpdate(world,.01f); Check(ticks==1,"Spawn missed next frame");
    WorldAccess::Shutdown(world);
}
void Hierarchy()
{
    World world;
    auto* root=world.CreateActor("Root"); root->SetPosition({10,0,0}); root->SetRotation(90,0,0); root->SetScale({2,2,2});
    auto* child=world.CreateActor("Child"); child->SetPosition({0,0,5});
    Check(child->SetParent(root,ReparentMode::KeepLocal),"Valid actor attachment failed");
    Near(child->GetWorldMatrix()(4,1),20,"Rotated/scaled actor parent not inherited");
    auto* pivot=child->AddComponent<SceneComponent>("Pivot"); pivot->GetLocalTransform().SetPosition({0,3,0});
    auto* spatial=child->AddComponent<SceneComponent>("Spatial"); spatial->GetLocalTransform().SetPosition({0,0,2});
    Check(spatial->SetParent(pivot,ReparentMode::KeepLocal),"Valid spatial attachment failed");
    Near(spatial->GetWorldPosition().x,24,"Nested spatial placement wrong"); Near(spatial->GetWorldPosition().y,6,"Nested spatial height wrong");
    Check(!root->SetParent(child,ReparentMode::KeepLocal) && !pivot->SetParent(spatial,ReparentMode::KeepLocal),"Cycle accepted");
    World other; Check(!child->SetParent(other.CreateActor("Foreign"),ReparentMode::KeepLocal),"Cross-world parent accepted");
    Check(!spatial->SetParent(root->AddComponent<SceneComponent>("Other owner"),ReparentMode::KeepLocal),"Cross-actor component parent accepted");
    auto before=spatial->GetWorldMatrix(); Check(spatial->SetParent(nullptr,ReparentMode::KeepWorld),"KeepWorld failed"); MatrixNear(before,spatial->GetWorldMatrix());
    auto* singular=child->AddComponent<SceneComponent>("Singular"); singular->GetLocalTransform().SetScale({0,1,1});
    Check(!spatial->SetParent(singular,ReparentMode::KeepWorld),"Singular KeepWorld succeeded"); MatrixNear(before,spatial->GetWorldMatrix());
    CommonUtilities::Matrix4f shear; shear(1,2)=.5f;
    Check(!spatial->SetWorldMatrix(shear),"Shear silently decomposed"); MatrixNear(before,spatial->GetWorldMatrix());
    auto desired=before; desired(4,2)=16; Check(spatial->SetWorldMatrix(desired),"Representable world edit failed"); MatrixNear(desired,spatial->GetWorldMatrix());
    auto* light=child->AddComponent<SpotLightComponent>("Light"); light->SetRadius(300);
    Near(light->GetWorldDirection().x,1,"Light did not inherit world rotation"); Near(light->GetRadius(),300,"Scale changed light radius");
    auto* camera=child->AddComponent<CameraComponent>("Camera"); camera->SyncCameraToOwner();
    Near(camera->GetCamera().GetTransform().GetForward().x,1,"Camera did not inherit world rotation");
    Near(camera->GetCamera().GetTransform().GetScale().x,1,"Camera inherited view scale");
    Start(world); root->SetActive(false); Check(!child->IsActive() && child->IsLocallyActive(),"Activation overwrote child flag");
    root->SetActive(true); Check(child->IsActive(),"Activation did not restore child");
    Check(child->SetParent(nullptr,ReparentMode::KeepWorld),"Active detach not queued"); Check(child->GetParent()==root,"Active detach applied during frame"); Flush(world);
    Check(!child->GetParent(),"Queued detach never applied");
    auto handle=spatial->GetHandle<SceneComponent>();
    spatial->SetParent(pivot,ReparentMode::KeepLocal); Flush(world); pivot->Destroy(); Check(!handle.Get(),"Spatial subtree remained alive"); Flush(world);
    child->SetParent(root,ReparentMode::KeepLocal); Flush(world); auto childHandle=child->GetHandle(); root->Destroy();
    Check(!childHandle.Get(),"Actor descendants remained alive"); Flush(world); Check(WorldAccess::GetActors(world).empty(),"Subtree memory not collected");
}
ComponentRegistry Registry()
{
    ComponentRegistry registry; registry.Register<CameraComponent>("Camera"); registry.Register<Probe>("Probe"); registry.Register<Consumer>("Consumer"); registry.Register<SceneComponent>("Scene");
    bool duplicate=false; try { registry.Register<Probe>("Probe"); } catch (const std::invalid_argument&) { duplicate=true; }
    Check(duplicate,"Duplicate registration accepted"); RegistryAccess::Freeze(registry);
    bool frozen=false; try { registry.Register<Probe>("Another"); } catch (const std::logic_error&) { frozen=true; }
    Check(frozen,"Frozen registry changed"); return registry;
}
SceneDescription Description()
{
    SceneDescription scene; ActorDescription actor; actor.Id="Camera actor";
    ComponentDescription camera; camera.Name="Camera"; camera.Type="Camera"; actor.Components.push_back(camera);
    scene.Actors.push_back(actor); scene.CameraActor=actor.Id; scene.CameraComponent=camera.Name; return scene;
}
void BuilderAndDependencies()
{
    auto registry=Registry(); auto scene=Description();
    auto consumer=ComponentDescription::Make<Consumer>("Consumer","Consumer",[](auto& c){c.ActorName="Later actor";c.ComponentName="Target";});
    scene.Actors[0].Components.push_back(consumer);
    ActorDescription later; later.Id="Later actor";
    ComponentDescription target; target.Name="Target";target.Type="Probe";later.Components.push_back(target); scene.Actors.push_back(later);
    auto result=SceneBuilder::Build(scene,registry); Check(bool(result),"Forward-reference candidate rejected");
    Check(WorldAccess::GetState(*result.Candidate)==WorldAccess::State::Prepared && !result.Camera.Get()->HasBegunPlay(),"Builder activated candidate");
    auto old=std::move(result.Candidate); WorldAccess::Activate(*old); auto oldHandle=old->FindActor("Later actor")->GetHandle();
    auto invalid=scene; invalid.Actors[1].Components[0].Type="Unknown"; invalid.Actors.push_back(invalid.Actors[1]);
    auto failed=SceneBuilder::Build(invalid,registry); Check(!failed && failed.Diagnostics.size()>=2 && oldHandle.Get(),"Invalid candidate corrupted active scene or lost diagnostics");
    auto replacement=SceneBuilder::Build(scene,registry); Check(bool(replacement),"Replacement preparation failed");
    WorldAccess::Shutdown(*old); old=std::move(replacement.Candidate); WorldAccess::Activate(*old); Check(!oldHandle.Get(),"Old handle resolved after replacement");
    for (int mode=0;mode<5;++mode)
    {
        World world; auto* a=world.CreateActor("A"); auto* c=a->AddComponent<Consumer>("Consumer");
        c->IsOptional=(mode==0 || mode==1);
        if (mode==1) c->ComponentName="Provided missing target";
        if (mode>=2) { a->AddComponent<Probe>("One"); a->AddComponent<Probe>("Two"); }
        if (mode==3) c->ComponentName="Two";
        if (mode==4) c->ComponentName="Consumer";
        SceneDiagnostics diagnostics; const bool valid=WorldAccess::Prepare(world,diagnostics);
        Check(valid==(mode==0 || mode==3),"Optional, ambiguous or named type validation incorrect");
    }
    std::string trace;
    auto cleanup=Description(); cleanup.Actors[0].Components.push_back(ComponentDescription::Make<Probe>("Probe","Probe",[&](auto& p){p.Trace=&trace;}));
    cleanup.CameraComponent="Missing"; auto failure=SceneBuilder::Build(cleanup,registry);
    Check(!failure && trace=="ProbeD ","Candidate failure began gameplay or missed attachment cleanup");
}
int main()
{
    try
    {
        Lifecycle(); ClosingWorld(); MutationsAndHandles(); FrozenBoundaries(); Hierarchy(); BuilderAndDependencies();
        std::cout << "PASS: lifecycle, frozen mutations, stale handles, hierarchy, transforms, registry, dependencies and candidate scenes\n";
    }
    catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
