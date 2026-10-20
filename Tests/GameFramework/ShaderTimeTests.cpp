#include <GameFramework/World/World.h>
#include <limits>
#include <iostream>
#include <stdexcept>

void Check(bool value, const char* message)
{
	if (!value) throw std::runtime_error(message);
}

void ShaderSimulationTime()
{
	World world;
	Check(world.GenerateRenderSnapshot().TotalTime == 0.0f, "New world shader time was not zero");
	world.Update(0.125f);
	world.Update(0.125f);
	Check(world.GenerateRenderSnapshot().TotalTime == 0.25f, "Shader time did not accumulate simulation updates");
	Check(world.GenerateRenderSnapshot().TotalTime == 0.25f, "Generating another snapshot advanced shader time");
	world.Update(-1.0f);
	world.Update(std::numeric_limits<float>::quiet_NaN());
	world.Update(std::numeric_limits<float>::infinity());
	Check(world.GenerateRenderSnapshot().TotalTime == 0.25f, "Invalid delta poisoned shader time");
	world.Update(10.0f);
	Check(world.GenerateRenderSnapshot().TotalTime == 0.5f, "Shader time did not use clamped simulation delta");
	world.Clear();
	Check(world.GenerateRenderSnapshot().TotalTime == 0.0f, "Clearing world did not reset shader time");
	world.Update(0.125f);
	Check(world.GenerateRenderSnapshot().TotalTime == 0.125f, "Cleared world shader time did not restart");
}

int main()
{
	try
	{
		ShaderSimulationTime();
		std::cout << "PASS: shader simulation time progression, sanitized deltas, stable snapshots and reset\n";
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
