#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class FrameBenchmarkSession
{
public:
	using Clock = std::chrono::steady_clock;

	struct RuntimeInfo
	{
		std::string Adapter;
		unsigned Width = 0;
		unsigned Height = 0;
	};

	static std::unique_ptr<FrameBenchmarkSession> CreateFromEnvironment();

	void SetRuntimeInfo(RuntimeInfo anInfo);
	bool RecordPresent(Clock::time_point aPresentStart, Clock::time_point aPresentEnd);

	const std::filesystem::path& GetResultDirectory() const { return myResultDirectory; }
	double GetAverageFrameMilliseconds() const { return myAverageFrameMilliseconds; }
	bool WasWrittenSuccessfully() const { return myWasWrittenSuccessfully; }

private:
	struct Config
	{
		std::filesystem::path OutputDirectory;
		std::string Label;
		std::string Commit;
		std::string Branch;
		std::string Configuration;
		std::string RunIndex;
		std::string Notes;
		std::string Harness;
		bool SourceDirty = false;
		size_t WarmupFrames = 300;
		size_t SampleFrames = 1200;
	};

	struct FrameSample
	{
		size_t Index = 0;
		double FrameMilliseconds = 0.0;
		double PresentMilliseconds = 0.0;
	};

	explicit FrameBenchmarkSession(Config aConfig);
	bool WriteResults();

	Config myConfig;
	RuntimeInfo myRuntimeInfo;
	std::vector<FrameSample> mySamples;
	Clock::time_point myPreviousPresentEnd = {};
	size_t myWarmupFramesSeen = 0;
	bool myHasPreviousPresent = false;
	bool myIsComplete = false;
	bool myWasWrittenSuccessfully = false;
	double myAverageFrameMilliseconds = 0.0;
	std::filesystem::path myResultDirectory;
};
