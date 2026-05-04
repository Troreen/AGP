#include "Logger.h"
#pragma region WindowsIncludes
#define	WIN32_LEAN_AND_MEAN

#define NOGDICAPMASKS     
#define NOVIRTUALKEYCODES 
#define NOWINMESSAGES     
#define NOWINSTYLES       
#define NOSYSMETRICS      
#define NOMENUS           
#define NOICONS           
#define NOKEYSTATES       
#define NOSYSCOMMANDS     
#define NORASTEROPS       
#define NOSHOWWINDOW      
#define OEMRESOURCE       
#define NOATOM            
#define NOCLIPBOARD       
#define NOCOLOR           
#define NOCTLMGR          
#define NODRAWTEXT        
#define NOGDI             
#define NOKERNEL          
//#define NOUSER            
//#define NONLS - Required for CP_ACP and WideCharToMultiByte
#define NOMB              
#define NOMEMMGR          
#define NOMETAFILE        
#define NOMINMAX          
#define NOMSG             
#define NOOPENFILE        
#define NOSCROLL          
#define NOSERVICE         
#define NOSOUND           
#define NOTEXTMETRIC      
#define NOWH              
#define NOWINOFFSETS      
#define NOCOMM            
#define NOKANJI           
#define NOHELP            
#define NOPROFILER        
#define NODEFERWINDOWPOS  
#define NOMCX
#include <Windows.h>  
#pragma endregion

#include <fstream>
#include <chrono>
#include <iostream>
#include <string>

unsigned Logger::LogCategoryBase::ourNextId = 0;

Logger::LogStream::~LogStream()
{
	if(File.is_open())
	{
		File.flush();
		File.close();
	}
}

Logger::LogCategoryBase::LogCategoryBase(std::string aName, LogVerbosity::Type aVerbosity): Name(std::move(aName)), Verbosity(aVerbosity), Id(ourNextId++)
{  }

Logger::LogCategoryBase::~LogCategoryBase() = default;

Logger::Logger()
	: myStdErrHandle(GetStdHandle(STD_ERROR_HANDLE)), myIsRunning(true)
{
	myLogThread = std::thread(&Logger::WorkerThread, this);
}

Logger::~Logger()
{
	myIsRunning = false;
	myQueueCV.notify_all();
	myLogThread.join();
}

std::string Logger::Timestamp(bool aIncludeDate /*= false*/) const
{
	static std::string dateFormat = "%Y-%m-%d %H:%M:%S";
	static std::string noDateFormat = "%H:%M:%S";

	const std::chrono::time_point now = std::chrono::system_clock::now();
	const std::time_t time = std::chrono::system_clock::to_time_t(now);

	tm timeInfo{};
	const int error = localtime_s(&timeInfo, &time);

	char buffer[20]{};
	const size_t wcsTimeErr = strftime(buffer, 20, aIncludeDate ? dateFormat.c_str() : noDateFormat.c_str(), &timeInfo);
	return buffer;
}

void Logger::LogIntl(const LogCategoryBase& aCategory, LogVerbosity::Type aVerbosity, const char* aMessage)
{
	// [hh:mm:ss][   LOG   ][ModelViewer] ModelViewer starting...
	SetConsoleTextAttribute(myStdErrHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);	

	myStream << "[" << Timestamp() << "] ";

	switch(aVerbosity)
	{
	case LogVerbosity::None:
		break;
	case LogVerbosity::Fatal:
	case LogVerbosity::Error:
		{
			SetConsoleTextAttribute(myStdErrHandle, BACKGROUND_RED);
			myStream << "[  ERROR  ]";
			SetConsoleTextAttribute(myStdErrHandle, FOREGROUND_RED | FOREGROUND_INTENSITY);
		}
		break;
	case LogVerbosity::Warning:
		{
			SetConsoleTextAttribute(myStdErrHandle, BACKGROUND_RED | BACKGROUND_GREEN);
			myStream << "[ WARNING ]";
			SetConsoleTextAttribute(myStdErrHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		}
		break;
	case LogVerbosity::Log:
	case LogVerbosity::Verbose:
		{
			SetConsoleTextAttribute(myStdErrHandle, BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
			myStream << "[   LOG   ]";
			SetConsoleTextAttribute(myStdErrHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		}
		break;
	}

	myStream << " [" << aCategory.Name << "] ";

	myStream << aMessage << LogStream::endl;

	SetConsoleTextAttribute(myStdErrHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void Logger::WorkerThread()
{
	char imagePath[MAX_PATH]{};
	GetModuleFileNameA(NULL, imagePath, MAX_PATH);

	std::string imageFileName(imagePath);
	imageFileName.shrink_to_fit();

	const size_t lastSlashPos = imageFileName.find_last_of('\\');
	std::string inFileNameOnly = imageFileName;
	if(lastSlashPos != std::string::npos)
	{
		inFileNameOnly = inFileNameOnly.substr(lastSlashPos + 1);
	}

	inFileNameOnly = inFileNameOnly.substr(0, inFileNameOnly.size() - 4);

	myLogFilePath = LOGGING_PATH;
	if(!myLogFilePath.has_filename())
	{
		TCHAR exeFileName[MAX_PATH];
		GetModuleFileName(NULL, exeFileName, MAX_PATH);
		const std::filesystem::path exePath = exeFileName;
		myLogFilePath = myLogFilePath / (exePath.stem().string() + ".log");
	}

	const std::string timeStamp = Timestamp(true);

	myStream.File = std::ofstream(myLogFilePath, std::ios_base::app);
	myStream.File << std::string(100, '*') << '\n';
	myStream.File << "Logging has started at " << timeStamp << "." << '\n';
	myStream.File << std::string(100, '*') << '\n';

	std::queue<LogEntry> localLogQueue;

	while(myIsRunning.load())
	{
		// Lock and swap the queues
		{
			std::unique_lock lock(myQueueMutex);	

			// Sleep until log messages arrive or dtor runs.
			myQueueCV.wait(lock, [&]
			{
				return !myIsRunning.load(std::memory_order_acquire) || !myLogQueue.empty();
			});

			// Finish if dtor ran.
			if (!myIsRunning.load(std::memory_order_relaxed) && myLogQueue.empty())
				break;

			// Swap queues for processing.
			std::swap(localLogQueue, myLogQueue);
		}		

		while(!localLogQueue.empty())
		{
			const LogEntry& entry = localLogQueue.front();
			LogIntl(*entry.Category, entry.Verbosity, entry.Message.c_str());
			localLogQueue.pop();
		}
	}
}

void Logger::Log(const LogCategoryBase& aCategory, LogVerbosity::Type aVerbosity, const char* aMessage)
{
	if(aCategory.Verbosity >= aVerbosity)
	{
		Logger& logger = Get();
		if (logger.myFilter.empty() || logger.myFilter.contains(aCategory))
		{
			LogEntry entry;
			entry.Category = &aCategory;
			entry.Verbosity = aVerbosity;
			entry.Message = aMessage;

			{
				std::scoped_lock lock(logger.myQueueMutex);
				logger.myLogQueue.push(std::move(entry));
			}

			logger.myQueueCV.notify_one();
		}
	}
}

void Logger::Flush()
{
	Get().myStream.File.flush();
}

void Logger::AddFilter(const LogCategoryBase& aCategory)
{
	Get().myFilter.emplace(aCategory);
}

void Logger::RemoveFilter(const LogCategoryBase& aCategory)
{
	Get().myFilter.erase(aCategory);
}

void Logger::ClearFilters()
{
	Get().myFilter.clear();
}
