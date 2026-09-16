#include "GameFramework/World/World.h"
#include <iostream>
#include <stdexcept>
#include <string>

struct Probe final : Component
{
    Probe(std::string& trace, const char* label) : Trace(trace), Label(label) {}
    void FixedUpdate(float) override { Trace += Label + "F "; }
    void Update(float) override { Trace += Label + "U "; }
    void LateUpdate(float) override { Trace += Label + "L "; }
    std::string& Trace;
    std::string Label;
};

void Check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main()
{
    try
    {
        std::string trace;
        World world;
        auto* first = world.CreateActor("First");
        auto* a = first->AddComponent<Probe>("A", trace, "A");
        auto* b = first->AddComponent<Probe>("B", trace, "B");
        auto* second = world.CreateActor("Second");
        second->AddComponent<Probe>("C", trace, "C");
        world.FixedUpdate(1.0f / 60.0f);
        world.Update(0.02f);
        Check(trace == "AF BF CF AU BU CU AL BL CL ", "component phase or insertion order changed");
        trace.clear();
        a->SetEnabled(false);
        second->SetActive(false);
        world.FixedUpdate(1.0f / 60.0f);
        world.Update(0.02f);
        Check(trace == "BF BU BL ", "disabled components or inactive actors still tick");
        trace.clear();
        a->SetEnabled(true);
        b->SetEnabled(false);
        second->SetActive(true);
        world.FixedUpdate(1.0f / 60.0f);
        Check(trace == "AF CF ", "reactivated components did not resume fixed updates");
        std::cout << "PASS: world component phase order, enabled flags and actor activation\n";
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
