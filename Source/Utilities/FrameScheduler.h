#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <thread>

namespace EngineScheduling
{
	template <class SnapshotType, size_t BufferCount>
	class TripleBufferedSnapshotQueue
	{
	public:
		static_assert(BufferCount >= 2);

		struct Stats
		{
			uint64_t PublishedSnapshots = 0;
			uint64_t ReusedSnapshots = 0;
			uint64_t DroppedReadySnapshots = 0;
		};

		void Reset()
		{
			std::scoped_lock lock(myMutex);
			for (SnapshotBuffer& buffer : myBuffers)
			{
				buffer.Snapshot.Clear();
				buffer.State = SnapshotState::Free;
				buffer.Sequence = 0;
			}

			myRenderingSnapshotIndex = InvalidSnapshotIndex;
			myNextSnapshotSequence = 1;
			myStats = {};
		}

		SnapshotType* BeginBuild()
		{
			std::scoped_lock lock(myMutex);
			size_t selectedIndex = InvalidSnapshotIndex;
			for (size_t bufferIndex = 0; bufferIndex < myBuffers.size(); ++bufferIndex)
			{
				if (myBuffers[bufferIndex].State == SnapshotState::Free)
				{
					selectedIndex = bufferIndex;
					break;
				}
			}

			if (selectedIndex == InvalidSnapshotIndex)
			{
				uint64_t oldestReadySequence = (std::numeric_limits<uint64_t>::max)();
				for (size_t bufferIndex = 0; bufferIndex < myBuffers.size(); ++bufferIndex)
				{
					const SnapshotBuffer& buffer = myBuffers[bufferIndex];
					if (buffer.State == SnapshotState::Ready && buffer.Sequence < oldestReadySequence)
					{
						selectedIndex = bufferIndex;
						oldestReadySequence = buffer.Sequence;
					}
				}

				if (selectedIndex != InvalidSnapshotIndex)
				{
					++myStats.DroppedReadySnapshots;
				}
			}

			if (selectedIndex == InvalidSnapshotIndex)
			{
				return nullptr;
			}

			myBuffers[selectedIndex].State = SnapshotState::Building;
			return &myBuffers[selectedIndex].Snapshot;
		}

		void Publish(SnapshotType* aSnapshot)
		{
			std::scoped_lock lock(myMutex);
			for (size_t bufferIndex = 0; bufferIndex < myBuffers.size(); ++bufferIndex)
			{
				SnapshotBuffer& buffer = myBuffers[bufferIndex];
				if (&buffer.Snapshot == aSnapshot)
				{
					buffer.Sequence = myNextSnapshotSequence++;
					buffer.State = SnapshotState::Ready;
					++myStats.PublishedSnapshots;
				}
				else if (buffer.State == SnapshotState::Ready)
				{
					buffer.State = SnapshotState::Free;
					++myStats.DroppedReadySnapshots;
				}
			}
		}

		void CancelBuild(SnapshotType* aSnapshot)
		{
			std::scoped_lock lock(myMutex);
			for (SnapshotBuffer& buffer : myBuffers)
			{
				if (&buffer.Snapshot == aSnapshot && buffer.State == SnapshotState::Building)
				{
					buffer.State = SnapshotState::Free;
					return;
				}
			}
		}

		const SnapshotType* AcquireLatest()
		{
			std::scoped_lock lock(myMutex);
			size_t selectedIndex = InvalidSnapshotIndex;
			uint64_t latestSequence = 0;
			for (size_t bufferIndex = 0; bufferIndex < myBuffers.size(); ++bufferIndex)
			{
				const SnapshotBuffer& buffer = myBuffers[bufferIndex];
				if (buffer.State == SnapshotState::Ready && buffer.Sequence > latestSequence)
				{
					selectedIndex = bufferIndex;
					latestSequence = buffer.Sequence;
				}
			}

			if (selectedIndex != InvalidSnapshotIndex)
			{
				ReleaseRenderingLocked();
				myBuffers[selectedIndex].State = SnapshotState::Rendering;
				myRenderingSnapshotIndex = selectedIndex;
				return &myBuffers[selectedIndex].Snapshot;
			}

			if (myRenderingSnapshotIndex != InvalidSnapshotIndex
				&& myBuffers[myRenderingSnapshotIndex].State == SnapshotState::Rendering)
			{
				++myStats.ReusedSnapshots;
				return &myBuffers[myRenderingSnapshotIndex].Snapshot;
			}

			return nullptr;
		}

		void ReleaseRendering()
		{
			std::scoped_lock lock(myMutex);
			ReleaseRenderingLocked();
		}

		Stats GetStats() const
		{
			std::scoped_lock lock(myMutex);
			return myStats;
		}

	private:
		enum class SnapshotState : uint8_t
		{
			Free,
			Building,
			Ready,
			Rendering
		};

		struct SnapshotBuffer
		{
			SnapshotType Snapshot;
			SnapshotState State = SnapshotState::Free;
			uint64_t Sequence = 0;
		};

		void ReleaseRenderingLocked()
		{
			if (myRenderingSnapshotIndex == InvalidSnapshotIndex)
			{
				return;
			}

			SnapshotBuffer& buffer = myBuffers[myRenderingSnapshotIndex];
			if (buffer.State == SnapshotState::Rendering)
			{
				buffer.State = SnapshotState::Free;
			}
			myRenderingSnapshotIndex = InvalidSnapshotIndex;
		}

		static constexpr size_t InvalidSnapshotIndex = static_cast<size_t>(-1);

		std::array<SnapshotBuffer, BufferCount> myBuffers;
		size_t myRenderingSnapshotIndex = InvalidSnapshotIndex;
		uint64_t myNextSnapshotSequence = 1;
		Stats myStats;
		mutable std::mutex myMutex;
	};

	template <class InputFrameType>
	class FixedStepUpdateWorker
	{
	public:
		struct Config
		{
			float FixedDeltaTime = 1.0f / 60.0f;
			float MaxFrameDeltaTime = 0.25f;
			int MaxFixedStepsPerWake = 5;
		};

		using ConsumeInputFn = std::function<bool(InputFrameType&)>;
		using FixedUpdateFn = std::function<void(float, InputFrameType&)>;
		using PublishSnapshotFn = std::function<void()>;
		using IdleWaitFn = std::function<void(std::stop_token)>;

		~FixedStepUpdateWorker()
		{
			Stop();
		}

		void Start(
			Config aConfig,
			ConsumeInputFn aConsumeInput,
			FixedUpdateFn aFixedUpdate,
			PublishSnapshotFn aPublishSnapshot,
			IdleWaitFn anIdleWait)
		{
			Stop();
			myTickCount = 0;
			myWorker = std::jthread(
				[this,
				config = aConfig,
				consumeInput = std::move(aConsumeInput),
				fixedUpdate = std::move(aFixedUpdate),
				publishSnapshot = std::move(aPublishSnapshot),
				idleWait = std::move(anIdleWait)](std::stop_token stopToken)
				{
					Run(stopToken, config, consumeInput, fixedUpdate, publishSnapshot, idleWait);
				});
		}

		void Stop()
		{
			if (!myWorker.joinable())
			{
				return;
			}

			myWorker.request_stop();
			myWorker.join();
		}

		uint64_t GetTickCount() const
		{
			return myTickCount.load();
		}

	private:
		void Run(
			std::stop_token aStopToken,
			const Config& aConfig,
			const ConsumeInputFn& aConsumeInput,
			const FixedUpdateFn& aFixedUpdate,
			const PublishSnapshotFn& aPublishSnapshot,
			const IdleWaitFn& anIdleWait)
		{
			using namespace std::chrono_literals;

			InputFrameType latestInputFrame;
			aPublishSnapshot();

			auto previousTime = std::chrono::steady_clock::now();
			float fixedUpdateAccumulator = 0.0f;

			while (!aStopToken.stop_requested())
			{
				aConsumeInput(latestInputFrame);

				const auto currentTime = std::chrono::steady_clock::now();
				const float deltaTime = (std::min)(
					std::chrono::duration<float>(currentTime - previousTime).count(),
					aConfig.MaxFrameDeltaTime);
				previousTime = currentTime;
				fixedUpdateAccumulator += deltaTime;

				int fixedStepsThisWake = 0;
				while (fixedUpdateAccumulator >= aConfig.FixedDeltaTime && fixedStepsThisWake < aConfig.MaxFixedStepsPerWake)
				{
					aFixedUpdate(aConfig.FixedDeltaTime, latestInputFrame);
					++myTickCount;
					aPublishSnapshot();
					latestInputFrame.ClearPressed();
					fixedUpdateAccumulator -= aConfig.FixedDeltaTime;
					++fixedStepsThisWake;
				}

				if (fixedStepsThisWake == aConfig.MaxFixedStepsPerWake && fixedUpdateAccumulator >= aConfig.FixedDeltaTime)
				{
					fixedUpdateAccumulator = 0.0f;
				}

				if (anIdleWait)
				{
					anIdleWait(aStopToken);
				}
				else
				{
					std::this_thread::sleep_for(1ms);
				}
			}
		}

		std::jthread myWorker;
		std::atomic<uint64_t> myTickCount = 0;
	};
}
