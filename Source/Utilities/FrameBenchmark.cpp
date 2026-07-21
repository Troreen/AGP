#include "FrameBenchmark.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <optional>
#include <sstream>
#include <system_error>

#include <Windows.h>

namespace
{
	struct Statistics
	{
		double Mean = 0.0;
		double Median = 0.0;
		double P95 = 0.0;
		double P99 = 0.0;
		double Minimum = 0.0;
		double Maximum = 0.0;
		double StandardDeviation = 0.0;
	};

	std::optional<std::string> ReadEnvironment(const char* aName)
	{
		char* value = nullptr;
		size_t valueLength = 0;
		if (_dupenv_s(&value, &valueLength, aName) != 0 || value == nullptr || valueLength <= 1)
		{
			std::free(value);
			return std::nullopt;
		}

		std::string result(value);
		std::free(value);
		return result;
	}

	bool ReadBooleanEnvironment(const char* aName)
	{
		const std::optional<std::string> value = ReadEnvironment(aName);
		return value.has_value() && (*value == "1" || *value == "true" || *value == "TRUE");
	}

	size_t ReadPositiveSizeEnvironment(const char* aName, size_t aFallback)
	{
		const std::optional<std::string> value = ReadEnvironment(aName);
		if (!value.has_value())
		{
			return aFallback;
		}

		try
		{
			const unsigned long long parsed = std::stoull(*value);
			return parsed > 0 ? static_cast<size_t>(parsed) : aFallback;
		}
		catch (...)
		{
			return aFallback;
		}
	}

	std::string SanitizePathPart(std::string aValue)
	{
		for (char& character : aValue)
		{
			const bool isAsciiLetter = (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
			const bool isDigit = character >= '0' && character <= '9';
			if (!isAsciiLetter && !isDigit && character != '-' && character != '_')
			{
				character = '-';
			}
		}

		if (aValue.empty())
		{
			return "run";
		}

		return aValue;
	}

	std::string EscapeJson(const std::string& aValue)
	{
		std::ostringstream escaped;
		for (const unsigned char character : aValue)
		{
			switch (character)
			{
			case '"': escaped << "\\\""; break;
			case '\\': escaped << "\\\\"; break;
			case '\b': escaped << "\\b"; break;
			case '\f': escaped << "\\f"; break;
			case '\n': escaped << "\\n"; break;
			case '\r': escaped << "\\r"; break;
			case '\t': escaped << "\\t"; break;
			default:
				if (character < 0x20)
				{
					escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(character);
				}
				else
				{
					escaped << character;
				}
				break;
			}
		}
		return escaped.str();
	}

	double Percentile(const std::vector<double>& someSortedValues, double aPercentile)
	{
		if (someSortedValues.empty())
		{
			return 0.0;
		}

		const double index = aPercentile * static_cast<double>(someSortedValues.size() - 1);
		const size_t lower = static_cast<size_t>(std::floor(index));
		const size_t upper = static_cast<size_t>(std::ceil(index));
		const double fraction = index - static_cast<double>(lower);
		return someSortedValues[lower] + (someSortedValues[upper] - someSortedValues[lower]) * fraction;
	}

	Statistics CalculateStatistics(std::vector<double> someValues)
	{
		Statistics result;
		if (someValues.empty())
		{
			return result;
		}

		std::sort(someValues.begin(), someValues.end());
		result.Minimum = someValues.front();
		result.Maximum = someValues.back();
		result.Median = Percentile(someValues, 0.50);
		result.P95 = Percentile(someValues, 0.95);
		result.P99 = Percentile(someValues, 0.99);
		result.Mean = std::accumulate(someValues.begin(), someValues.end(), 0.0) / static_cast<double>(someValues.size());

		double squaredDifferenceSum = 0.0;
		for (const double value : someValues)
		{
			const double difference = value - result.Mean;
			squaredDifferenceSum += difference * difference;
		}
		result.StandardDeviation = std::sqrt(squaredDifferenceSum / static_cast<double>(someValues.size()));
		return result;
	}

	double CalculateOnePercentLowFps(std::vector<double> someFrameTimes)
	{
		if (someFrameTimes.empty())
		{
			return 0.0;
		}

		std::sort(someFrameTimes.begin(), someFrameTimes.end(), std::greater<>());
		const size_t slowFrameCount = (std::max)(size_t{ 1 }, static_cast<size_t>(std::ceil(static_cast<double>(someFrameTimes.size()) * 0.01)));
		const double slowFrameMean = std::accumulate(someFrameTimes.begin(), someFrameTimes.begin() + slowFrameCount, 0.0)
			/ static_cast<double>(slowFrameCount);
		return slowFrameMean > 0.0 ? 1000.0 / slowFrameMean : 0.0;
	}

	std::string CreateTimestamp()
	{
		SYSTEMTIME utc = {};
		GetSystemTime(&utc);
		std::ostringstream timestamp;
		timestamp << std::setfill('0')
			<< std::setw(4) << utc.wYear
			<< std::setw(2) << utc.wMonth
			<< std::setw(2) << utc.wDay << '-'
			<< std::setw(2) << utc.wHour
			<< std::setw(2) << utc.wMinute
			<< std::setw(2) << utc.wSecond << '-'
			<< std::setw(3) << utc.wMilliseconds << 'Z';
		return timestamp.str();
	}

	void WriteStatistics(std::ostream& anOutput, const Statistics& someStatistics, unsigned anIndent)
	{
		const std::string indent(anIndent, ' ');
		anOutput << indent << "\"mean\": " << someStatistics.Mean << ",\n";
		anOutput << indent << "\"median\": " << someStatistics.Median << ",\n";
		anOutput << indent << "\"p95\": " << someStatistics.P95 << ",\n";
		anOutput << indent << "\"p99\": " << someStatistics.P99 << ",\n";
		anOutput << indent << "\"min\": " << someStatistics.Minimum << ",\n";
		anOutput << indent << "\"max\": " << someStatistics.Maximum << ",\n";
		anOutput << indent << "\"stddev\": " << someStatistics.StandardDeviation << '\n';
	}
}

std::unique_ptr<FrameBenchmarkSession> FrameBenchmarkSession::CreateFromEnvironment()
{
	if (!ReadBooleanEnvironment("AGP_BENCHMARK"))
	{
		return nullptr;
	}

	const std::optional<std::string> outputDirectory = ReadEnvironment("AGP_BENCHMARK_OUTPUT");
	if (!outputDirectory.has_value())
	{
		return nullptr;
	}

	Config config;
	config.OutputDirectory = *outputDirectory;
	config.Label = ReadEnvironment("AGP_BENCHMARK_LABEL").value_or("unlabelled");
	config.Commit = ReadEnvironment("AGP_BENCHMARK_COMMIT").value_or("unknown");
	config.Branch = ReadEnvironment("AGP_BENCHMARK_BRANCH").value_or("unknown");
	config.Configuration = ReadEnvironment("AGP_BENCHMARK_CONFIGURATION").value_or("unknown");
	config.Scenario = ReadEnvironment("AGP_BENCHMARK_SCENARIO").value_or("default");
	config.ComparisonId = ReadEnvironment("AGP_BENCHMARK_COMPARISON_ID").value_or("standalone");
	config.RequestedRef = ReadEnvironment("AGP_BENCHMARK_REQUESTED_REF").value_or(config.Branch);
	config.ExecutionIndex = ReadPositiveSizeEnvironment("AGP_BENCHMARK_EXECUTION_INDEX", config.ExecutionIndex);
	config.RunIndex = ReadPositiveSizeEnvironment("AGP_BENCHMARK_RUN", config.RunIndex);
	config.Notes = ReadEnvironment("AGP_BENCHMARK_NOTES").value_or("");
	config.Harness = ReadEnvironment("AGP_BENCHMARK_HARNESS").value_or("in-tree");
	config.SourceDirty = ReadBooleanEnvironment("AGP_BENCHMARK_DIRTY");
	config.WarmupFrames = ReadPositiveSizeEnvironment("AGP_BENCHMARK_WARMUP_FRAMES", config.WarmupFrames);
	config.SampleFrames = ReadPositiveSizeEnvironment("AGP_BENCHMARK_SAMPLE_FRAMES", config.SampleFrames);
	return std::unique_ptr<FrameBenchmarkSession>(new FrameBenchmarkSession(std::move(config)));
}

FrameBenchmarkSession::FrameBenchmarkSession(Config aConfig)
	: myConfig(std::move(aConfig))
{
	mySamples.reserve(myConfig.SampleFrames);
}

void FrameBenchmarkSession::SetRuntimeInfo(RuntimeInfo anInfo)
{
	myRuntimeInfo = std::move(anInfo);
}

bool FrameBenchmarkSession::RecordPresent(Clock::time_point aPresentStart, Clock::time_point aPresentEnd)
{
	if (myIsComplete)
	{
		return false;
	}

	if (!myHasPreviousPresent)
	{
		myPreviousPresentEnd = aPresentEnd;
		myHasPreviousPresent = true;
		return false;
	}

	const double frameMilliseconds = std::chrono::duration<double, std::milli>(aPresentEnd - myPreviousPresentEnd).count();
	const double presentMilliseconds = std::chrono::duration<double, std::milli>(aPresentEnd - aPresentStart).count();
	myPreviousPresentEnd = aPresentEnd;

	if (myWarmupFramesSeen < myConfig.WarmupFrames)
	{
		++myWarmupFramesSeen;
		return false;
	}

	mySamples.push_back(FrameSample{
		.Index = mySamples.size(),
		.FrameMilliseconds = frameMilliseconds,
		.PresentMilliseconds = presentMilliseconds
	});

	if (mySamples.size() < myConfig.SampleFrames)
	{
		return false;
	}

	myIsComplete = true;
	myWasWrittenSuccessfully = WriteResults();
	return true;
}

bool FrameBenchmarkSession::WriteResults()
{
	std::vector<double> frameTimes;
	std::vector<double> presentTimes;
	frameTimes.reserve(mySamples.size());
	presentTimes.reserve(mySamples.size());
	for (const FrameSample& sample : mySamples)
	{
		frameTimes.push_back(sample.FrameMilliseconds);
		presentTimes.push_back(sample.PresentMilliseconds);
	}

	const Statistics frameStatistics = CalculateStatistics(frameTimes);
	const Statistics presentStatistics = CalculateStatistics(presentTimes);
	myAverageFrameMilliseconds = frameStatistics.Mean;
	const double averageFps = frameStatistics.Mean > 0.0 ? 1000.0 / frameStatistics.Mean : 0.0;
	const double onePercentLowFps = CalculateOnePercentLowFps(frameTimes);

	std::string shortCommit = myConfig.Commit.substr(0, (std::min)(size_t{ 8 }, myConfig.Commit.size()));
	const std::string runName = CreateTimestamp()
		+ '_' + SanitizePathPart(myConfig.Label)
		+ '_' + SanitizePathPart(shortCommit)
		+ "_run" + std::to_string(myConfig.RunIndex);
	myResultDirectory = myConfig.OutputDirectory / runName;

	std::error_code error;
	std::filesystem::create_directories(myResultDirectory, error);
	if (error)
	{
		return false;
	}

	std::ofstream framesFile(myResultDirectory / "frames.csv", std::ios::binary);
	if (!framesFile)
	{
		return false;
	}
	framesFile << "frame,frame_ms,present_ms\n";
	framesFile << std::fixed << std::setprecision(6);
	for (const FrameSample& sample : mySamples)
	{
		framesFile << sample.Index << ',' << sample.FrameMilliseconds << ',' << sample.PresentMilliseconds << '\n';
	}
	framesFile.close();

	std::ofstream summaryFile(myResultDirectory / "summary.json", std::ios::binary);
	if (!summaryFile)
	{
		return false;
	}

	const std::string computerName = ReadEnvironment("COMPUTERNAME").value_or("unknown");
	const std::string processor = ReadEnvironment("PROCESSOR_IDENTIFIER").value_or("unknown");
	summaryFile << std::fixed << std::setprecision(6);
	summaryFile << "{\n";
	summaryFile << "  \"schema_version\": 2,\n";
	summaryFile << "  \"timestamp_utc\": \"" << EscapeJson(runName.substr(0, 19) + "Z") << "\",\n";
	summaryFile << "  \"label\": \"" << EscapeJson(myConfig.Label) << "\",\n";
	summaryFile << "  \"commit\": \"" << EscapeJson(myConfig.Commit) << "\",\n";
	summaryFile << "  \"branch\": \"" << EscapeJson(myConfig.Branch) << "\",\n";
	summaryFile << "  \"configuration\": \"" << EscapeJson(myConfig.Configuration) << "\",\n";
	summaryFile << "  \"scenario\": \"" << EscapeJson(myConfig.Scenario) << "\",\n";
	summaryFile << "  \"comparison_id\": \"" << EscapeJson(myConfig.ComparisonId) << "\",\n";
	summaryFile << "  \"execution_index\": " << myConfig.ExecutionIndex << ",\n";
	summaryFile << "  \"run_index\": " << myConfig.RunIndex << ",\n";
	summaryFile << "  \"requested_ref\": \"" << EscapeJson(myConfig.RequestedRef) << "\",\n";
	summaryFile << "  \"notes\": \"" << EscapeJson(myConfig.Notes) << "\",\n";
	summaryFile << "  \"harness\": \"" << EscapeJson(myConfig.Harness) << "\",\n";
	summaryFile << "  \"source_dirty\": " << (myConfig.SourceDirty ? "true" : "false") << ",\n";
	summaryFile << "  \"warmup_frames\": " << myConfig.WarmupFrames << ",\n";
	summaryFile << "  \"sample_frames\": " << mySamples.size() << ",\n";
	summaryFile << "  \"system\": {\n";
	summaryFile << "    \"computer\": \"" << EscapeJson(computerName) << "\",\n";
	summaryFile << "    \"processor\": \"" << EscapeJson(processor) << "\",\n";
	summaryFile << "    \"adapter\": \"" << EscapeJson(myRuntimeInfo.Adapter) << "\",\n";
	summaryFile << "    \"width\": " << myRuntimeInfo.Width << ",\n";
	summaryFile << "    \"height\": " << myRuntimeInfo.Height << "\n";
	summaryFile << "  },\n";
	summaryFile << "  \"metrics\": {\n";
	summaryFile << "    \"average_fps\": " << averageFps << ",\n";
	summaryFile << "    \"one_percent_low_fps\": " << onePercentLowFps << ",\n";
	summaryFile << "    \"frame_ms\": {\n";
	WriteStatistics(summaryFile, frameStatistics, 6);
	summaryFile << "    },\n";
	summaryFile << "    \"present_ms\": {\n";
	WriteStatistics(summaryFile, presentStatistics, 6);
	summaryFile << "    }\n";
	summaryFile << "  }\n";
	summaryFile << "}\n";
	return summaryFile.good();
}
