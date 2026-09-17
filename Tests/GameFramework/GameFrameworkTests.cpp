#include "GameFramework/Scenes/SceneBuilder.h"
#include "GameFramework/Scenes/References.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Runtime/Internal/GameLoop.h"
#include "GameFramework/Runtime/Internal/WorldAccess.h"
#include "GameFramework/Runtime/Internal/RegistryAccess.h"
#include "TestSession.h"
using GameFrameworkInternal::WorldAccess;
using GameFrameworkInternal::RegistryAccess;
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <limits>
#include <type_traits>

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
    std::function<void()> OnResolve, OnBegin, OnFixed, OnUpdate, OnLate, OnEnd;
    void Record(const char* phase) { if (Trace) *Trace += GetName() + phase + " "; }
    void ResolveReferences(References&) override { Record("C"); if (OnResolve) OnResolve(); }
    void BeginPlay() override { Record("B"); if (OnBegin) OnBegin(); }
    void FixedUpdate(float) override { Record("F"); if (OnFixed) OnFixed(); }
    void Update(float) override { Record("U"); if (OnUpdate) OnUpdate(); }
    void LateUpdate(float) override { Record("L"); if (OnLate) OnLate(); }
    void EndPlay() override { Record("E"); if (OnEnd) OnEnd(); }
    ~Probe() override { Record("D"); }
};
struct Consumer : Component
{
    std::string ActorName, ComponentName;
    bool IsOptional = false;
    ComponentHandle<Probe> Dependency;
    void ResolveReferences(References& context) override
    {
        if (ActorName.empty()) Dependency = IsOptional ? context.Optional<Probe>(ComponentName) : context.Require<Probe>(ComponentName);
        else Dependency = IsOptional ? context.Optional<Probe>(ActorName,ComponentName) : context.Require<Probe>(ActorName,ComponentName);
    }
};
struct Invalid : Component
{
    void ResolveReferences(References& context) override { context.Error("required","Deliberate invalid fixture"); }
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
    Start(world); Check(trace=="AC BC CC AB BB CB ","Resolve/Begin order or disabled initialization incorrect");
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
void PendingQueriesAndNames()
{
    World world;
    auto* actor=world.SpawnActor("Repeated");
    auto stable=actor->GetRef();
    auto* first=actor->AddComponent<Probe>();
    auto* second=actor->AddComponent<Probe>();
    Check(first && second && first->GetName()!=second->GetName(),"Unnamed additions did not receive distinct names");
    Check(actor->GetComponent<Probe>()==first && actor->GetComponents<Probe>().size()==2,"Constructing typed queries lost components");
    Start(world);
    auto* pending=actor->AddComponent<Probe>("Pending");
    auto* pendingActor=world.SpawnActor("Repeated");
    Check(actor->GetComponents<Probe>().size()==3 && actor->FindComponent<Probe>("Pending")==pending,"Typed queries missed pending additions");
    Check(!world.FindActor("Repeated") && world.FindActors("Repeated").size()==2,"Duplicate display names did not report ambiguity");
    actor->SetName("Renamed");
    Check(stable.Get()==actor && world.FindActor("Repeated")==pendingActor && world.FindActor("Renamed")==actor,"Rename changed identity or query results");
    bool duplicate=false;
    try { actor->AddComponent<Probe>("Pending"); } catch (const std::invalid_argument&) { duplicate=true; }
    Check(duplicate,"Duplicate explicit component name accepted");
    first->Destroy(); pending->Destroy();
    Check(actor->GetComponent<Probe>()==second && actor->GetComponents<Probe>().size()==1 && !actor->FindComponent<Probe>("Pending"),"Queries exposed logical death");
    auto* reused=actor->AddComponent<Probe>("Pending");
    Check(reused && actor->FindComponent<Probe>("Pending")==reused,"Dead component reserved its name");
}
void ResolveIsReadOnly()
{
    for (int operation=0;operation<10;++operation)
    {
        World world;
        World externalWorld;auto* external=externalWorld.SpawnActor("External");
        auto* actor=world.SpawnActor("Existing");
        auto* peer=actor->AddComponent<Probe>("Peer");
        auto* other=world.SpawnActor("Other");
        Start(world);
        auto actorRef=actor->GetRef();auto peerRef=peer->GetRef<Probe>();
        const auto pose=actor->GetWorldMatrix();
        auto* invalid=actor->AddComponent<Probe>("Invalid resolver");
        auto invalidRef=invalid->GetRef<Probe>();
        invalid->OnResolve=[&]
        {
            switch(operation)
            {
            case 0: world.SpawnActor("Forbidden");break;
            case 1: actor->AddComponent<Probe>("Forbidden");break;
            case 2: peer->Destroy();break;
            case 3: actor->SetParent(other,ReparentMode::KeepLocal);break;
            case 4: actor->SetActive(false);break;
            case 5: peer->SetEnabled(false);break;
            case 6: actor->GetTransform().SetLocalPosition({7,8,9});break;
            case 7: actor->SetName("Forbidden");break;
            case 8: try {actor->SetActive(false);} catch (const std::logic_error&) {} break;
            case 9: external->SetActive(false);break;
            }
        };
        SceneDiagnostics diagnostics;
        Check(!WorldAccess::Flush(world,diagnostics) && !diagnostics.empty() && !invalidRef.Get(),"Resolve mutation did not reject its batch");
        Check(actorRef.Get()==actor && peerRef.Get()==peer && actor->IsActive() && peer->IsEnabled() && !actor->GetParent(),"Invalid resolver altered existing objects");
        Check(actor->GetName()=="Existing" && !world.FindActor("Forbidden") && !actor->FindComponent("Forbidden"),"Resolve mutation leaked names or objects");
        Check(external->IsActive(),"Candidate validation mutated an external world");
        MatrixNear(pose,actor->GetWorldMatrix());
    }
}
void LifecycleFailures()
{
    std::string trace;
    {
        World world;
        auto* actor=world.SpawnActor("Begin failure");
        auto* completed=actor->AddComponent<Probe>("Completed");completed->Trace=&trace;
        auto* throwing=actor->AddComponent<Probe>("Throwing");throwing->Trace=&trace;
        auto* later=actor->AddComponent<Probe>("Later");later->Trace=&trace;
        throwing->OnBegin=[] {throw std::runtime_error("Expected Begin failure");};
        bool caught=false;try { Start(world); } catch (const std::runtime_error&) {caught=true;}
        Check(caught,"Begin exception disappeared");
        WorldAccess::Shutdown(world);
        Check(trace.find("CompletedE ")!=std::string::npos && trace.find("ThrowingE ")==std::string::npos && trace.find("LaterB ")==std::string::npos,"Begin failure paired incomplete lifecycle callbacks");
        Check(trace.find("CompletedD ")!=std::string::npos && trace.find("ThrowingD ")!=std::string::npos && trace.find("LaterD ")!=std::string::npos,"Begin failure skipped RAII cleanup");
    }
    trace.clear();
    {
        World world;auto* actor=world.SpawnActor("End failure");
        auto* first=actor->AddComponent<Probe>("First");first->Trace=&trace;
        auto* second=actor->AddComponent<Probe>("Second");second->Trace=&trace;
        second->OnEnd=[] {throw std::runtime_error("Expected End failure");};
        Start(world);trace.clear();WorldAccess::Shutdown(world);
        Check(trace.find("SecondE FirstE ")==0 && trace.find("FirstD ")!=std::string::npos && trace.find("SecondD ")!=std::string::npos,"End failure interrupted remaining cleanup");
    }
}
void CpuSessionHappyPath()
{
    TestSession session;
    ComponentRef<Probe> spawned; int fixedTicks=0,updates=0;
    session.Initialize([&](World& world)
    {
        auto* actor=world.SpawnActor("Composer");
        auto* source=actor->AddComponent<Probe>("Source");
        source->OnFixed=[&,actor]
        {
            if (spawned.Get()) return;
            auto* next=actor->AddComponent<Probe>();
            next->OnFixed=[&] {++fixedTicks;};next->OnUpdate=[&] {++updates;};
            spawned=next->GetRef<Probe>();
            Check(actor->FindComponent<Probe>(next->GetName())==next,"Immediate public lookup failed");
        };
    });
    Check(session.Step(.035f) && spawned.Get() && fixedTicks==0 && updates==0,"New object joined its creation frame");
    Check(session.Step(.01f) && fixedTicks==1 && updates==1,"Engine frame boundary did not start public composition");
    spawned.Get()->Destroy();
    Check(!spawned.Get() && session.Step(.02f) && fixedTicks==1 && updates==1,"Destroyed object participated in later frame");
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
        static_assert(!std::is_constructible_v<ComponentRef<SceneComponent>,ComponentRef<Probe>>);
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
    for (int phase=1;phase<6;++phase)
    {
        World world;
        auto* actor=world.CreateActor("Actor"); auto* source=actor->AddComponent<Probe>("Source");
        ComponentHandle<Probe> spawned;
        auto spawn=[&] { if (!spawned.Get()) spawned=actor->AddComponent<Probe>("Spawned")->GetHandle<Probe>(); };
        if (phase==1) source->OnBegin=spawn;
        if (phase==2) source->OnFixed=spawn;
        if (phase==3) source->OnUpdate=spawn;
        if (phase==4) source->OnLate=spawn;
        if (phase==5) source->OnEnd=spawn;
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
void FrozenActorAdditions()
{
    for (int phase=0;phase<5;++phase)
    {
        World world;int ticks=0;
        auto* source=world.SpawnActor("Source")->AddComponent<Probe>("Source");
        ActorRef spawned;ComponentRef<Probe> behavior;
        auto spawn=[&]
        {
            if (spawned.Get()) return;
            if (auto* actor=world.SpawnActor("Spawned"))
            {
                spawned=actor->GetRef();auto* probe=actor->AddComponent<Probe>();
                behavior=probe->GetRef<Probe>();probe->OnUpdate=[&]{++ticks;};probe->OnFixed=[&]{++ticks;};
            }
        };
        if (phase==0) source->OnBegin=spawn;
        if (phase==1) source->OnFixed=spawn;
        if (phase==2) source->OnUpdate=spawn;
        if (phase==3) source->OnLate=spawn;
        if (phase==4) source->OnEnd=spawn;
        Start(world);
        if (phase==4) {source->Destroy();Flush(world);}
        else {WorldAccess::FixedUpdate(world,.01f);WorldAccess::Update(world,.01f);}
        Check(spawned.Get() && behavior.Get() && !behavior.Get()->HasBegunPlay() && ticks==0,"Spawned actor entered its creation frame");
        Flush(world);WorldAccess::FixedUpdate(world,.01f);WorldAccess::Update(world,.01f);
        Check(behavior.Get()->HasBegunPlay() && ticks==2,"Spawned actor missed its next eligible frame");
        WorldAccess::Shutdown(world);
    }
}
void Hierarchy()
{
    World world;
    auto* root=world.CreateActor("Root"); root->SetPosition({10,0,0}); root->SetRotation(90,0,0); root->SetScale({2,2,2});
    auto* child=world.CreateActor("Child"); child->SetPosition({0,0,5});
    Check(child->SetParent(root,ReparentMode::KeepLocal),"Valid actor attachment failed");
    Near(child->GetWorldMatrix()(4,1),20,"Rotated/scaled actor parent not inherited");
    auto* pivot=child->AddComponent<SceneComponent>("Pivot"); pivot->GetTransform().SetLocalPosition({0,3,0});
    auto* spatial=child->AddComponent<SceneComponent>("Spatial"); spatial->GetTransform().SetLocalPosition({0,0,2});
    Check(spatial->SetParent(pivot,ReparentMode::KeepLocal),"Valid spatial attachment failed");
    Near(spatial->GetWorldPosition().x,24,"Nested spatial placement wrong"); Near(spatial->GetWorldPosition().y,6,"Nested spatial height wrong");
    Check(!root->SetParent(child,ReparentMode::KeepLocal) && !pivot->SetParent(spatial,ReparentMode::KeepLocal),"Cycle accepted");
    World other; Check(!child->SetParent(other.CreateActor("Foreign"),ReparentMode::KeepLocal),"Cross-world parent accepted");
    Check(!spatial->SetParent(root->AddComponent<SceneComponent>("Other owner"),ReparentMode::KeepLocal),"Cross-actor component parent accepted");
    auto before=spatial->GetWorldMatrix(); Check(spatial->SetParent(nullptr,ReparentMode::KeepWorld),"KeepWorld failed"); MatrixNear(before,spatial->GetWorldMatrix());
    auto* singular=child->AddComponent<SceneComponent>("Singular"); singular->GetTransform().SetLocalScale({0,1,1});
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
    Check(child->SetParent(nullptr,ReparentMode::KeepWorld),"Active detach failed"); Check(!child->GetParent(),"Successful detach was deferred"); Flush(world);
    Check(!child->GetParent(),"Detached parent returned after boundary");
    auto handle=spatial->GetHandle<SceneComponent>();
    spatial->SetParent(pivot,ReparentMode::KeepLocal); Flush(world); pivot->Destroy(); Check(!handle.Get(),"Spatial subtree remained alive"); Flush(world);
    child->SetParent(root,ReparentMode::KeepLocal); Flush(world); auto childHandle=child->GetHandle(); root->Destroy();
    Check(!childHandle.Get(),"Actor descendants remained alive"); Flush(world); Check(WorldAccess::GetActors(world).empty(),"Subtree memory not collected");
}
void TransformValidationAndAdmission()
{
    World world;
    auto* actor=world.SpawnActor("Live child");
    auto* spatial=actor->AddComponent<SceneComponent>("Live spatial");
    auto& transform=actor->GetTransform();
    Check(transform.SetLocalPosition({10,20,30}) && transform.SetLocalScale({-2,3,4}),"Finite negative/nonuniform TRS rejected");
    Check(transform.SetLocalRotation({2,0,0,0}),"Normalizable quaternion rejected");
    Near(transform.GetLocalRotation().Length(),1,"Quaternion was not normalized");
    const auto before=transform.GetWorldMatrix();
    const float nan=std::numeric_limits<float>::quiet_NaN();
    const float infinity=std::numeric_limits<float>::infinity();
    Check(!transform.SetLocalPosition({nan,0,0}) && !transform.SetLocalScale({1,infinity,1}) && !transform.SetLocalRotation({0,0,0,0}),"Invalid TRS accepted");
    auto invalid=transform.GetLocalPose();invalid.Position.z=nan;
    Check(!transform.SetLocalPose(invalid),"Invalid pose accepted");
    MatrixNear(before,transform.GetWorldMatrix());
    auto* copy=world.SpawnActor("Pose copy");
    Check(copy->GetTransform().SetLocalPose(transform.GetLocalPose()),"Parentless pose copy failed");
    MatrixNear(before,copy->GetWorldMatrix());
    Start(world);
    auto* pendingParent=world.SpawnActor("Pending parent");
    auto* pendingSpatial=actor->AddComponent<SceneComponent>("Pending spatial");
    Check(!actor->SetParent(pendingParent,ReparentMode::KeepLocal) && !spatial->SetParent(pendingSpatial,ReparentMode::KeepLocal),"Admitted child attached beneath unadmitted parent");
    Check(!actor->GetParent() && !spatial->GetParent(),"Rejected admission changed parent");
    MatrixNear(before,actor->GetWorldMatrix());
    Check(pendingParent->SetParent(copy,ReparentMode::KeepLocal) && pendingSpatial->SetParent(spatial,ReparentMode::KeepLocal),"Pending child could not attach beneath admitted parent");
    auto* rejected=world.SpawnActor("Invalid batch actor");rejected->AddComponent<Invalid>("Invalid");
    auto rejectedRef=rejected->GetRef();auto pendingRef=pendingParent->GetRef();
    SceneDiagnostics diagnostics;
    Check(!WorldAccess::Flush(world,diagnostics) && !rejectedRef.Get() && !pendingRef.Get(),"Invalid addition batch was partially admitted");
    Check(actor->GetRef().Get()==actor && spatial->GetRef<SceneComponent>().Get()==spatial && copy->GetRef().Get()==copy,"Rejected batch destroyed live hierarchy");
    auto* admittedParent=world.SpawnActor("Next parent");Flush(world);
    Check(actor->SetParent(admittedParent,ReparentMode::KeepLocal) && actor->GetParent()==admittedParent,"Boundary did not admit actor parent immediately");
    Check(admittedParent->GetTransform().SetLocalScale({0,1,1}),"Zero local scale should be representable");
    const auto singularPose=actor->GetWorldMatrix();
    Check(!actor->GetTransform().SetWorldPosition({1,2,3}),"World editing accepted singular parent");
    MatrixNear(singularPose,actor->GetWorldMatrix());
}
void BuilderAndDependencies()
{
    for (int mode=0;mode<5;++mode)
    {
        World world;auto* actor=world.SpawnActor("A");auto* consumer=actor->AddComponent<Consumer>("Consumer");
        consumer->IsOptional=(mode==0 || mode==1);
        if(mode==1) consumer->ComponentName="Provided missing target";
        if(mode>=2) {actor->AddComponent<Probe>("One");actor->AddComponent<Probe>("Two");}
        if(mode==3) consumer->ComponentName="Two";
        if(mode==4) consumer->ComponentName="Consumer";
        SceneDiagnostics diagnostics;const bool valid=WorldAccess::Prepare(world,diagnostics);
        Check(valid==(mode==0 || mode==3),"Optional, ambiguous or named type validation incorrect");
    }
}
void SceneConstructionTests();
int main(int argc,char** argv)
{
    if (argc>1 && std::string(argv[1])=="--logger-exit")
    {
        World world;world.SpawnActor("Repeated");world.SpawnActor("Repeated");
        Check(!world.FindActor("Repeated"),"Ambiguous name resolved during logger fixture");
        std::cout<<"PASS: logger startup reached immediate process teardown"<<std::endl;
        return 0;
    }
    try
    {
        Lifecycle(); ClosingWorld(); PendingQueriesAndNames(); ResolveIsReadOnly(); LifecycleFailures(); CpuSessionHappyPath();
        MutationsAndHandles(); FrozenBoundaries(); FrozenActorAdditions(); Hierarchy(); TransformValidationAndAdmission(); BuilderAndDependencies(); SceneConstructionTests();
        std::cout << "PASS: lifecycle, frozen mutations, stale handles, hierarchy, transforms, registry, dependencies and candidate scenes" << std::endl;
    }
    catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
