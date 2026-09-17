#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <crtdbg.h>
#include "GameFramework/Runtime/GameApplication.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/Runtime/IGame.h"
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
    void BeginPlay() override { if (ThrowOnBegin) throw std::runtime_error("Expected BeginPlay failure"); ++*Begins; }
    void EndPlay() override { ++*Ends; }
};
class HostGame final : public IGame
{
public:
    std::string Scenario;
    bool Threaded=true;
    int Begins=0,Ends=0,Builds=0,Shutdowns=0,Updates=0;
    bool Succeeded=false;
    std::thread::id Platform=std::this_thread::get_id();
    ComponentRegistry Registry;
    ActorHandle Original;
    int Phase=0;
    std::chrono::steady_clock::time_point Deadline;
    void Request(GameContext& context,bool invalid=false,bool throwing=false)
    {
        context.RequestScene([this,invalid,throwing](const GameInput* input)
        {
            Check(std::this_thread::get_id()==Platform,"Scene factory executed on gameplay worker"); ++Builds;
            SceneDescription scene; ActorDescription actor; actor.Id="Camera";
            ComponentDescription camera; camera.Name="Camera";camera.Type="Camera";
            actor.Components.push_back(camera);
            actor.Components.push_back(ComponentDescription::Make<Lifetime>("Lifetime","Lifetime",[this,throwing](auto& c)
            { c.Begins=&Begins;c.Ends=&Ends;c.ThrowOnBegin=throwing; }));
            scene.Actors.push_back(std::move(actor));scene.CameraActor="Camera";scene.CameraComponent=invalid ? "Missing" : "Camera";
            return SceneBuilder::Build(scene,Registry,input);
        });
    }
    void Initialize(GameContext& context) override
    {
        Registry.Register<CameraComponent>("Camera");Registry.Register<Lifetime>("Lifetime");Registry.Freeze();
        Deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);
        Request(context,Scenario=="initial-invalid");
    }
    void Update(GameContext& context,float) override
    {
        ++Updates;
        Check((std::this_thread::get_id()!=Platform)==Threaded,"Wrong thread for gameplay mode");
        Check(std::chrono::steady_clock::now()<Deadline,"Host transition timed out");
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
        Check(std::this_thread::get_id()==Platform,"Shutdown ran on worker");
        Check(context.GetWorld().GetState()!=World::State::Ending,"Game Shutdown lost live world access");
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
        Check(game.Succeeded && game.Shutdowns==1 && game.Ends==2,"Host did not complete orderly teardown");
        std::cout<<"PASS: host scene replacement and shutdown ("<<(game.Threaded?"threaded":"sync")<<")\n";
    }
    catch (const std::exception& e)
    {
        if (game.Scenario=="begin-failure" && std::string(e.what())=="Expected BeginPlay failure" && game.Shutdowns==1 && game.Ends==1)
        { std::cout<<"PASS: committed BeginPlay failure joined and cleaned up\n";return 0; }
        if (game.Scenario=="initial-invalid" && game.Updates==0 && game.Shutdowns==1 && game.Begins==0)
        { std::cout<<"PASS: invalid initial scene never entered gameplay\n";return 0; }
        std::cerr<<e.what()<<'\n';return 1;
    }
}
