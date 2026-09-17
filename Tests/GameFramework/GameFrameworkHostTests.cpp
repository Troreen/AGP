#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <crtdbg.h>
#include "GameFramework/Runtime/GameApplication.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/Runtime/IGame.h"
#include "GameFramework/Integration/GameFramework/Integration/ISceneSource.h"
#include "GameFramework/Runtime/Internal/WorldAccess.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Scenes/ComponentRegistry.h"
#include "GameFramework/Scenes/References.h"
#include "GameFramework/Scenes/SceneReader.h"
#include "GameFramework/Runtime/GameTime.h"
#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>
#include <stdexcept>

using namespace GameFrameworkIntegration;
void Check(bool condition,const char* message) { if (!condition) throw std::runtime_error(message); }
void CheckLoadTransients(const GameTime& time,const GameInput& input)
{
    Check(time.GetDeltaTime()==0 && input.MouseDeltaX==0 && input.MouseDeltaY==0 && std::none_of(input.KeysPressed.begin(),input.KeysPressed.end(),[](bool value){return value;}),"Scene callback observed stale time/input transients");
}
struct Lifetime : Component
{
    int* Begins=nullptr;
    int* Ends=nullptr;
    bool ThrowOnBegin=false;
    bool RejectBootstrap=false;
    std::string BeginScene,EndScene;
    bool* EndRequestAccepted=nullptr;
    std::function<void()> ResolveAttempt;
    void ResolveReferences(References& references) override
    {
        if (ResolveAttempt) ResolveAttempt();
        if (RejectBootstrap) references.Error("fixture","Invalid bootstrap");
    }
    void BeginPlay() override
    {
        CheckLoadTransients(GetTime(),GetInput());
        if (ThrowOnBegin) throw std::runtime_error("Expected BeginPlay failure");++*Begins;
        if(!BeginScene.empty()) Check(GetScenes().Load(SceneId{BeginScene}),"New BeginPlay scene request refused");
    }
    void EndPlay() override
    {
        ++*Ends;
        if(!EndScene.empty()) *EndRequestAccepted=GetScenes().Load(SceneId{EndScene});
    }
};
class HostGame final : public IGame
{
public:
    std::string Scenario;
    bool Threaded=true;
    int Begins=0,Ends=0,Builds=0,Shutdowns=0,Updates=0,Registrations=0,Initializations=0,Loaded=0,Failed=0;
    bool Succeeded=false,SourceDestroyed=false,SourceDestroyedEarly=false,EndRequestAccepted=false;
    std::thread::id Platform=std::this_thread::get_id();
    ActorRef Original;
    ComponentRef<Lifetime> OriginalComponent;
    ComponentRef<CameraComponent> OriginalCamera;
    ComponentRef<Lifetime> RejectedAddition;
    int Phase=0;
    std::chrono::steady_clock::time_point Deadline;
    bool IsBootstrap() const
    {
        return Scenario=="direct" || Scenario=="initialize-failure" || Scenario=="bootstrap-invalid" || Scenario=="runtime-invalid" || Scenario=="resolve-quit";
    }
    bool IsRecoverable() const {return Scenario=="invalid" || Scenario=="source-failure" || Scenario=="source-throw";}
    SceneSourceResult ReadScene(const SceneId& id)
    {
        Check(std::this_thread::get_id()==Platform,"Scene source executed on gameplay worker");++Builds;
        if(Scenario=="empty"){SceneSourceResult result;result.Data=SceneData{};return result;}
        if (id.Value=="SourceThrow") throw std::runtime_error("Expected source exception");
        if (id.Value=="SourceBadAlloc") throw std::bad_alloc{};
        if (id.Value=="SourceFailure")
        {
            SceneSourceResult result;result.Diagnostics.push_back({"export-object","","source","Expected adapter rejection","host-fixture.scene","adapter-failed","source",""});return result;
        }
        SceneData scene;scene.Source.File="host-fixture";
        ActorRecord actor;actor.Id="camera-actor";actor.Name="Camera";
        ComponentRecord camera;camera.Id="camera";camera.Name="Camera";camera.Type="agp.Camera";
        ComponentRecord lifetime;lifetime.Id="lifetime";lifetime.Name="Lifetime";lifetime.Type="test.Lifetime";
        lifetime.Properties["throw"]=id.Value=="BeginFailure";
        if(Scenario=="begin-request" && id.Value=="Initial") lifetime.Properties["beginScene"]=std::string("Replacement");
        if(Scenario=="end-request" && id.Value=="Initial") lifetime.Properties["endScene"]=std::string("ForbiddenEndRequest");
        actor.Components={camera,lifetime};scene.Actors.push_back(std::move(actor));
        scene.RequireCamera=true;scene.ActiveCamera=ObjectAddress{"camera-actor",id.Value=="Invalid" ? "missing" : "camera"};
        SceneSourceResult result;result.Data=std::move(scene);return result;
    }
    void RegisterComponents(ComponentRegistry& registry) override
    {
        ++Registrations;
        registry.Register<Lifetime>("test.Lifetime",[this](Lifetime& lifetime,SceneReader& data)
        {
            lifetime.Begins=&Begins;lifetime.Ends=&Ends;lifetime.ThrowOnBegin=data.OptionalBool("throw",false);
            lifetime.BeginScene=data.OptionalString("beginScene");lifetime.EndScene=data.OptionalString("endScene");lifetime.EndRequestAccepted=&EndRequestAccepted;
        });
        if (Scenario=="registration-failure") registry.Register<Lifetime>("agp.Camera");
    }
    void Initialize(GameContext& context) override
    {
        ++Initializations;Check(Registrations==1,"Registration did not precede Initialize");
        Deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);
        if (IsBootstrap())
        {
            auto* actor=context.GetWorld().SpawnActor("Direct camera");
            auto* camera=actor->AddComponent<CameraComponent>("Camera");
            Check(context.GetWorld().SetActiveCamera(camera),"Direct camera selection failed");
            auto* lifetime=actor->AddComponent<Lifetime>("Lifetime");
            lifetime->Begins=&Begins;lifetime->Ends=&Ends;lifetime->RejectBootstrap=Scenario=="bootstrap-invalid";
            Original=actor->GetRef();OriginalComponent=lifetime->GetRef<Lifetime>();
            if (Scenario=="initialize-failure") throw std::runtime_error("Expected Initialize failure");
            return;
        }
        if (Scenario=="last-request") context.GetScenes().Load(SceneId{"Invalid"});
        Check(context.GetScenes().Load(SceneId{Scenario=="initial-invalid" ? "Invalid" : "Initial"}),"Initial scene request refused");
        Check(context.GetScenes().GetStatus()==SceneLoadStatus::Requested,"Request status was not immediate");
    }
    void OnSceneLoaded(GameContext& context,const SceneId& id) override
    {
        ++Loaded;
        CheckLoadTransients(context.GetTime(),context.GetInput());
        Check(Scenario=="empty" ? Begins==0 && Ends==0 : Begins==Loaded && Ends==Loaded-1,"Completion callback preceded BeginPlay or old-world cleanup");
        Check(context.GetScenes().GetCurrent()==std::optional<SceneId>{id},"Current scene identity differs from completion");
        const auto expectedStatus=Scenario=="begin-request" && Loaded==1 ? SceneLoadStatus::Requested : SceneLoadStatus::Loaded;
        Check(context.GetScenes().GetStatus()==expectedStatus && !context.GetScenes().GetLastError(),"Successful load did not clear failure status or retain new request status");
        if(Scenario=="begin-request" && Loaded==1)
        {
            Check(Builds==1,"BeginPlay recursively processed another scene");
            auto* actor=context.GetWorld().FindActor("Camera");Original=actor->GetRef();OriginalComponent=actor->GetComponent<Lifetime>()->GetRef<Lifetime>();
        }
        if (Scenario=="callback-failure" && Loaded==2) throw std::runtime_error("Expected completion failure");
    }
    void OnSceneLoadFailed(GameContext& context,const SceneLoadError& error) override
    {
        ++Failed;Check(!error.Diagnostics.empty(),"Load failure lost diagnostics");
        CheckLoadTransients(context.GetTime(),context.GetInput());
        if(Scenario=="source-failure") Check(error.Diagnostics[0].Code=="adapter-failed" && error.Diagnostics[0].File=="host-fixture.scene" && error.Diagnostics[0].Actor=="export-object","Adapter error lost original source context");
        Check(context.GetScenes().GetStatus()==SceneLoadStatus::Failed && context.GetScenes().GetLastError().has_value(),"Failure status unavailable in callback");
        if (Original.Get())
        {
            Check(OriginalComponent.Get() && context.GetWorld().GetActiveCamera()==OriginalCamera.Get() && Begins==1 && Ends==0,"Rejected load damaged old world or camera");
            Check(context.GetScenes().GetCurrent()==std::optional<SceneId>{SceneId{"Initial"}},"Rejected load changed current scene identity");
        }
    }
    void Update(GameContext& context,float)
    {
        ++Updates;
        Check((std::this_thread::get_id()!=Platform)==Threaded,"Wrong thread for gameplay mode");
        Check(std::chrono::steady_clock::now()<Deadline,"Host transition timed out");
        if(Scenario=="empty")
        {Check(Loaded==1 && !context.GetWorld().GetActiveCamera(),"Empty CPU scene required presentation");Succeeded=true;context.RequestQuit();return;}
        if(Scenario=="begin-request")
        {
            if(Loaded<2)return;
            Check(Builds==2 && !Original.Get() && !OriginalComponent.Get(),"Deferred BeginPlay request did not replace old objects");
            Succeeded=true;context.RequestQuit();return;
        }
        if (Scenario=="runtime-invalid" || Scenario=="resolve-quit")
        {
            if (Phase++==0)
            {
                auto* invalid=Original.Get()->AddComponent<Lifetime>("Rejected addition");
                invalid->Begins=&Begins;invalid->Ends=&Ends;invalid->RejectBootstrap=Scenario=="runtime-invalid";
                if (Scenario=="resolve-quit") invalid->ResolveAttempt=[&context] {try {context.RequestQuit();} catch (const std::logic_error&) {}};
                RejectedAddition=invalid->GetRef<Lifetime>();return;
            }
            Check(!RejectedAddition.Get() && Original.Get() && OriginalComponent.Get() && Begins==1 && Ends==0,"Runtime rejection damaged current scene");
            Succeeded=true;context.RequestQuit();return;
        }
        if (Scenario=="direct" || Scenario=="last-request")
        {
            Check(Begins==1 && Ends==0 && Builds==(Scenario=="direct" ? 0 : 1) && Failed==0,"Direct bootstrap or last request contract failed");
            Succeeded=true;context.RequestQuit();return;
        }
        if (Phase==0)
        {
            Check(Loaded==1,"First update preceded scene completion");
            auto* actor=context.GetWorld().FindActor("Camera");Original=actor->GetRef();
            OriginalComponent=actor->GetComponent<Lifetime>()->GetRef<Lifetime>();
            OriginalCamera=context.GetWorld().GetActiveCamera()->GetRef<CameraComponent>();Phase=1;
            if (Scenario=="reload") Check(context.GetScenes().Reload(),"Reload refused current scene");
            else context.GetScenes().Load(SceneId{Scenario=="invalid" ? "Invalid" : Scenario=="source-failure" ? "SourceFailure" : Scenario=="source-throw" ? "SourceThrow" : Scenario=="source-badalloc" ? "SourceBadAlloc" : Scenario=="begin-failure" ? "BeginFailure" : "Replacement"});
            Check(Original.Get() && OriginalComponent.Get(),"Load replaced world inline");return;
        }
        if (IsRecoverable() && Phase==1)
        {
            if (Failed==0) return;
            Check(Failed==1 && Loaded==1 && Original.Get() && OriginalComponent.Get(),"Failed load lost old references");
            Phase=2;context.GetScenes().Load(SceneId{"Replacement"});return;
        }
        if (Loaded<2) return;
        Check(!Original.Get() && !OriginalComponent.Get() && !OriginalCamera.Get() && Begins==2 && Ends==1,"Replacement retained old refs or broke lifecycle");
        Check(!EndRequestAccepted,"Old EndPlay queued a scene during teardown");
        Succeeded=true;context.RequestQuit();
    }
    void Shutdown(GameContext& context) override
    {
        ++Shutdowns;
        Check(!context.GetWorld().SpawnActor("During shutdown") && !context.GetScenes().Load(SceneId{"During shutdown"}),"Shutdown accepted new work");
        if (IsBootstrap())
        {
            Check(Original.Get() && OriginalComponent.Get(),"Shutdown lost bootstrap borrows");
            Check(!Original.Get()->AddComponent<Lifetime>("During shutdown"),"Shutdown allowed new components");
        }
        Check(std::this_thread::get_id()==Platform,"Shutdown ran on worker");
        Check(GameFrameworkInternal::WorldAccess::GetState(context.GetWorld())!=GameFrameworkInternal::WorldAccess::State::Ending,"Shutdown lost world access");
    }
};
class HostSource final : public ISceneSource
{
public:
    explicit HostSource(HostGame& game):myGame(game){}
    ~HostSource() override {myGame.SourceDestroyed=true;myGame.SourceDestroyedEarly=myGame.Ends!=myGame.Begins;}
    SceneSourceResult Load(const SceneId& id,SceneLoadContext&) override {return myGame.ReadScene(id);}
private:
    HostGame& myGame;
};
int RunExtractionFixture();
int main(int argc,char** argv)
{
    _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    _set_abort_behavior(0,_WRITE_ABORT_MSG | _CALL_REPORTFAULT);SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    if(argc>1 && std::string(argv[1])=="extraction") return RunExtractionFixture();
    HostGame game;game.Threaded=argc<2 || std::string(argv[1])!="sync";game.Scenario=argc>2?argv[2]:"valid";
    GameApplication::Config config;config.Width=320;config.Height=240;config.ShowWindow=false;config.ThreadedUpdate=game.Threaded;
    config.ContentRoot=std::filesystem::current_path()/"Assets";
    ApplicationSetup setup;if(game.Scenario!="no-source") setup.SceneSource=std::make_unique<HostSource>(game);
    try
    {
        const int result=GameApplication{}.Run(game,config,std::move(setup));
        Check(!game.SourceDestroyedEarly && (game.SourceDestroyed || game.Scenario=="no-source"),"Source lifetime ended before component cleanup");
        if(game.Scenario=="initial-invalid" || game.Scenario=="no-source")
        {
            Check(result!=0 && game.Failed==1 && game.Loaded==0 && game.Updates==0 && game.Shutdowns==1 && game.Begins==0,"Initial load failure entered gameplay or lost callback");
            std::cout<<"PASS: reported initial load failure without gameplay\n";return 0;
        }
        const int expectedEnds=game.Scenario=="empty" ? 0 : (game.IsBootstrap() || game.Scenario=="last-request") ? 1 : 2;
        Check(result==0 && game.Succeeded && game.Shutdowns==1 && game.Ends==expectedEnds,"Host did not complete orderly teardown");
        std::cout<<"PASS: "<<game.Scenario<<" scene/session contract ("<<(game.Threaded?"threaded":"sync")<<")\n";
    }
    catch(const std::exception& error)
    {
        const std::string message=error.what();
        if(game.Scenario=="source-badalloc" && dynamic_cast<const std::bad_alloc*>(&error) && game.Failed==0 && game.Loaded==1 && game.Shutdowns==1 && game.Ends==1 && !game.Original.Get() && !game.OriginalComponent.Get())
        {std::cout<<"PASS: allocation failure remained fatal and cleaned up\n";return 0;}
        if(game.Scenario=="registration-failure" && game.Registrations==1 && game.Initializations==0 && game.Shutdowns==0)
        {std::cout<<"PASS: registration failed before Initialize\n";return 0;}
        if(game.Scenario=="initialize-failure" && message=="Expected Initialize failure" && game.Shutdowns==1 && game.Begins==0 && game.Ends==0)
        {std::cout<<"PASS: partial Initialize cleanup\n";return 0;}
        if(game.Scenario=="bootstrap-invalid" && message=="Invalid initial scene" && game.Shutdowns==1 && game.Updates==0 && game.Begins==0 && game.Ends==0 && !game.Original.Get() && !game.OriginalComponent.Get())
        {std::cout<<"PASS: bootstrap failure retained Shutdown borrows\n";return 0;}
        if(game.Scenario=="begin-failure" && message=="Expected BeginPlay failure" && game.Shutdowns==1 && game.Ends==1 && !game.Original.Get() && !game.OriginalComponent.Get())
        {std::cout<<"PASS: fatal postcommit BeginPlay cleanup\n";return 0;}
        if(game.Scenario=="callback-failure" && message=="Expected completion failure" && game.Shutdowns==1 && game.Ends==2 && !game.Original.Get() && !game.OriginalComponent.Get())
        {std::cout<<"PASS: fatal completion callback cleanup\n";return 0;}
        std::cerr<<message<<'\n';return 1;
    }
}
