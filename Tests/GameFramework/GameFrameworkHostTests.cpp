#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <crtdbg.h>
#include "GameFramework/Runtime/GameApplication.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/Runtime/IGame.h"
#include "GameFramework/Integration/GameFramework/Integration/LegacySceneBridge.h"
#include "GameFramework/Runtime/Internal/WorldAccess.h"
#include "GameFramework/Scenes/ConnectionContext.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <stdexcept>

void Check(bool condition,const char* message) { if (!condition) throw std::runtime_error(message); }
struct Lifetime : Component
{
    int* Begins=nullptr;
    int* Ends=nullptr;
    bool ThrowOnBegin=false;
    bool RejectBootstrap=false;
    void Connect(ConnectionContext& references) override { if (RejectBootstrap) references.Error("fixture","Invalid bootstrap"); }
    void BeginPlay() override { if (ThrowOnBegin) throw std::runtime_error("Expected BeginPlay failure"); ++*Begins; }
    void EndPlay() override { ++*Ends; }
};
class HostGame final : public IGame
{
public:
    std::string Scenario;
    bool Threaded=true;
    int Begins=0,Ends=0,Builds=0,Shutdowns=0,Updates=0,Registrations=0,Initializations=0;
    bool Succeeded=false;
    std::thread::id Platform=std::this_thread::get_id();

    ActorHandle Original;
    ComponentHandle<Lifetime> OriginalComponent;
    int Phase=0;
    std::chrono::steady_clock::time_point Deadline;
    void Request(GameContext& context,bool invalid=false,bool throwing=false)
    {
        GameFrameworkIntegration::LegacySceneBridge::Request(context,[this,invalid,throwing](const ComponentRegistry& registry,const GameInput* input)
        {
            Check(std::this_thread::get_id()==Platform,"Scene factory executed on gameplay worker"); ++Builds;
            SceneDescription scene; ActorDescription actor; actor.Id="Camera";
            ComponentDescription camera; camera.Name="Camera";camera.Type="agp.Camera";
            actor.Components.push_back(camera);
            actor.Components.push_back(ComponentDescription::Make<Lifetime>("Lifetime","Lifetime",[this,throwing](auto& c)
            { c.Begins=&Begins;c.Ends=&Ends;c.ThrowOnBegin=throwing; }));
            scene.Actors.push_back(std::move(actor));scene.CameraActor="Camera";scene.CameraComponent=invalid ? "Missing" : "Camera";
            return SceneBuilder::Build(scene,registry,input);
        });
    }
    void RegisterComponents(ComponentRegistry& registry) override
    {
        ++Registrations;
        registry.Register<Lifetime>("Lifetime");
        if (Scenario=="registration-failure") registry.Register<Lifetime>("agp.Camera");
    }
    void Initialize(GameContext& context) override
    {
        ++Initializations;
        Check(Registrations==1,"Game registration did not precede Initialize");
        Deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);
        if (Scenario=="direct" || Scenario=="initialize-failure" || Scenario=="bootstrap-invalid")
        {
            auto* actor=context.GetWorld().SpawnActor("Direct camera");
            auto* camera=actor->AddComponent<CameraComponent>("Camera");
            Check(context.GetWorld().SetActiveCamera(camera),"Direct camera selection failed");
            auto* lifetime=actor->AddComponent<Lifetime>("Lifetime");
            lifetime->Begins=&Begins; lifetime->Ends=&Ends;
            lifetime->RejectBootstrap=Scenario=="bootstrap-invalid";
            Original=actor->GetRef(); OriginalComponent=lifetime->GetRef<Lifetime>();
            if (Scenario=="initialize-failure") throw std::runtime_error("Expected Initialize failure");
            return;
        }
        Request(context,Scenario=="initial-invalid");
    }
    void Update(GameContext& context,float) override
    {
        ++Updates;
        Check((std::this_thread::get_id()!=Platform)==Threaded,"Wrong thread for gameplay mode");
        Check(std::chrono::steady_clock::now()<Deadline,"Host transition timed out");
        if (Scenario=="direct")
        {
            Check(Begins==1 && Ends==0 && Builds==0,"Direct bootstrap did not begin automatically");
            Succeeded=true;context.RequestQuit();return;
        }
        if (Phase==0)
        {
            Original=context.GetWorld().FindActor("Camera")->GetHandle(); Phase=1;
            Request(context,Scenario=="invalid",Scenario=="begin-failure");return;
        }
        if (Builds<2) return;
        if (Scenario=="invalid")
        {
            Check(Original.Get()!=nullptr && Begins==1 && Ends==0,"Failed candidate damaged live world");
            // Follow a failed replacement with a valid one to prove the worker resumed.
            Phase=2;Scenario="recover";Request(context);return;
        }
        if (Scenario=="recover" && Builds<3) return;
        Check(!Original.Get() && Begins==2 && Ends==1,"Successful replacement lifecycle incorrect");
        Succeeded=true;context.RequestQuit();
    }
    void Shutdown(GameContext& context) override
    {
        ++Shutdowns;
        Check(context.GetWorld().SpawnActor("During shutdown")==nullptr,"Shutdown allowed new actors");
        if (Scenario=="bootstrap-invalid" || Scenario=="initialize-failure" || Scenario=="direct")
        {
            Check(Original.Get() && OriginalComponent.Get(),"Shutdown lost bootstrap borrows");
            Check(!Original.Get()->AddComponent<Lifetime>("During shutdown"),"Shutdown allowed new components");
        }
        Check(std::this_thread::get_id()==Platform,"Shutdown ran on worker");
        Check(GameFrameworkInternal::WorldAccess::GetState(context.GetWorld())!=GameFrameworkInternal::WorldAccess::State::Ending,"Game Shutdown lost live world access");
    }
};
int main(int argc,char** argv)
{
    // Assertion scenarios run isolated: report to stderr, never open a modal dialog.
    _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    _set_abort_behavior(0,_WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    HostGame game;game.Threaded=argc<2 || std::string(argv[1])!="sync";game.Scenario=argc>2?argv[2]:"valid";
    GameApplication::Config config;config.Width=320;config.Height=240;config.ShowWindow=false;config.ThreadedUpdate=game.Threaded;
    config.ContentRoot=std::filesystem::current_path()/"Assets";
    try
    {
        GameApplication app;app.Run(game,config);
        Check(game.Succeeded && game.Shutdowns==1 && game.Ends==(game.Scenario=="direct" ? 1 : 2),"Host did not complete orderly teardown");
        std::cout<<"PASS: host scene replacement and shutdown ("<<(game.Threaded?"threaded":"sync")<<")\n";
    }
    catch (const std::exception& e)
    {
        if (game.Scenario=="registration-failure" && game.Registrations==1 && game.Initializations==0 && game.Shutdowns==0)
        { std::cout<<"PASS: duplicate engine type rejected before Initialize without Shutdown\n";return 0; }
        if (game.Scenario=="initialize-failure" && std::string(e.what())=="Expected Initialize failure" && game.Initializations==1 && game.Shutdowns==1 && game.Begins==0 && game.Ends==0)
        { std::cout<<"PASS: partial Initialize cleaned up without beginning bootstrap\n";return 0; }
        if (game.Scenario=="bootstrap-invalid" && std::string(e.what())=="Invalid initial scene" && game.Initializations==1 && game.Shutdowns==1 && game.Updates==0 && game.Begins==0 && game.Ends==0 && !game.Original.Get() && !game.OriginalComponent.Get())
        { std::cout<<"PASS: invalid bootstrap retained Shutdown borrows and then expired refs\n";return 0; }
        if (game.Scenario=="begin-failure" && std::string(e.what())=="Expected BeginPlay failure" && game.Shutdowns==1 && game.Ends==1)
        { std::cout<<"PASS: committed BeginPlay failure joined and cleaned up\n";return 0; }
        if (game.Scenario=="initial-invalid" && game.Updates==0 && game.Shutdowns==1 && game.Begins==0)
        { std::cout<<"PASS: invalid initial scene never entered gameplay\n";return 0; }
        std::cerr<<e.what()<<'\n';return 1;
    }
}
