#include "Runtime/Internal/GameLoop.h"
#define NOMINMAX
#include "RenderCulling.h"
#include "RenderItemRouting.h"
#include "FrameScheduler.h"
#include <iostream>
#include <limits>
#include <stdexcept>

void Check(bool condition, const char* message)
{
	if (!condition) throw std::runtime_error(message);
}

struct Snapshot
{
	int Sequence = 0;
	std::array<int, 64> Values = {};
	void Clear() { Sequence = 0; Values.fill(0); }
};

void TestCulling()
{
	using namespace RenderCulling;
	const auto frustum = CreateFrustumFromViewProjection(CU::Matrix4f());
	Check(frustum.IsValid, "identity D3D frustum invalid");
	Check(IntersectsFrustum(frustum, {{0,0,0.5f},0,true}), "inside point");
	const std::array<CU::Vector3f,6> touching = {{{-1.1f,0,0.5f},{1.1f,0,0.5f},{0,-1.1f,0.5f},{0,1.1f,0.5f},{0,0,-0.1f},{0,0,1.1f}}};
	for (auto center : touching)
		Check(IntersectsFrustum(frustum, {center,0.10001f,true}), "frustum boundary sphere rejected");
	for (auto center : touching)
		Check(!IntersectsFrustum(frustum, {center,0.09f,true}), "outside sphere retained");
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float inf = std::numeric_limits<float>::infinity();
	Check(IntersectsFrustum(frustum, {{nan,0,0},1,true}), "NaN center fallback");
	Check(IntersectsFrustum(frustum, {{99,0,0},inf,true}), "infinite radius fallback");
	Check(IntersectsFrustum(frustum, {{99,0,0},-1,true}), "negative radius fallback");
	Check(IntersectsFrustum({}, {{99,0,0},1,true}), "invalid frustum fallback");
	CU::Matrix4f invalid;
	invalid(1,1) = inf;
	Check(!CreateFrustumFromViewProjection(invalid).IsValid, "infinite matrix accepted");
	Check(!TransformBoundingSphere({},1,true,invalid).IsValid, "invalid transform accepted");
	CU::Matrix4f transform;
	transform(1,1) = -2; transform(2,2) = 3; transform(3,3) = 4; transform(4,1) = 7;
	const auto scaled = TransformBoundingSphere({1,0,0},2,true,transform);
	Check(scaled.IsValid && scaled.Center.x == 5 && scaled.Radius == 8, "nonuniform mirrored scale");
	transform = CU::Matrix4f(); transform(1,2) = 2;
	const auto sheared = TransformBoundingSphere({},1,true,transform);
	for (int i=0; i<360; ++i)
	{
		const float t = i * 0.0174532925f;
		const auto point = CU::Maths::TransformPoint(CU::Vector3f(std::cos(t),std::sin(t),0),transform);
		Check(point.Length() <= sheared.Radius + 0.00001f, "shear bound is not conservative");
	}
}

void TestRouting()
{
	const auto mixed = RenderItemRouting::Classify(std::array{true,false,true}, [](bool opaque) { return opaque; });
	Check(mixed.Opaque && mixed.Blended, "mixed mesh must enter both passes");
	const auto empty = RenderItemRouting::Classify(std::array<bool,0>{}, [](bool v) { return v; });
	Check(!empty.Opaque && !empty.Blended, "empty mesh classified");
	const auto opaqueOnly = RenderItemRouting::Classify(std::array{true,true}, [](bool v) { return v; });
	Check(opaqueOnly.Opaque && !opaqueOnly.Blended, "opaque classification");
	std::vector<size_t> opaque{0,1,2,3}, blended = opaque;
	const std::array<float,4> distances{9,1,4,4};
	RenderItemRouting::Sort(opaque, blended, [&](size_t i) { return distances[i]; });
	Check(opaque == std::vector<size_t>({1,2,3,0}), "opaque front-to-back/stable ties");
	Check(blended == std::vector<size_t>({0,2,3,1}), "blended back-to-front/stable ties");
}

void TestQueue()
{
	EngineScheduling::TripleBufferedSnapshotQueue<Snapshot,3> queue;
	Check(queue.AcquireLatest() == nullptr, "unpublished snapshot visible");
	auto* cancelled = queue.BeginBuild(); queue.CancelBuild(cancelled);
	queue.Publish(cancelled);
	Check(queue.AcquireLatest() == nullptr, "cancelled snapshot published");
	auto* first = queue.BeginBuild(); first->Sequence = 1; queue.Publish(first);
	const auto* held = queue.AcquireLatest();
	Check(held && held->Sequence == 1, "publication");
	Check(queue.AcquireLatest() == held, "snapshot reuse");
	for (int i=2; i<=8; ++i)
	{
		auto* next = queue.BeginBuild();
		Check(next != held, "producer overwrote held snapshot");
		next->Sequence=i; queue.Publish(next);
		Check(held->Sequence == 1, "held snapshot mutated");
	}
	Check(queue.AcquireLatest()->Sequence == 8, "latest publication wins");
	Check(queue.GetStats().DroppedReadySnapshots == 6, "drop accounting");
	queue.ReleaseRendering(); queue.Reset();
	Check(queue.GetStats().PublishedSnapshots == 0, "queue reset");
	std::atomic<bool> done = false;
	std::jthread producer([&]
	{
		for (int i=1; i<=10000; ++i)
		{
			auto* next = queue.BeginBuild();
			if (!next) continue;
			next->Sequence=i; next->Values.fill(i); queue.Publish(next);
		}
		done = true;
	});
	while (!done)
		if (const auto* next = queue.AcquireLatest())
			for (int value : next->Values) Check(value == next->Sequence, "torn snapshot");
	producer.join(); queue.ReleaseRendering();
}

struct Input { void ClearPressed() {} };
void TestWorker()
{
	EngineScheduling::FixedStepUpdateWorker<Input> worker;
	worker.Start({}, {}, {}, [] { throw std::runtime_error("expected worker failure"); }, {});
	worker.Stop();
	bool caught = false;
	try { worker.RethrowIfFailed(); } catch (const std::runtime_error&) { caught = true; }
	Check(caught, "worker exception was lost");
}

void TestGameLoop()
{
    using GameFrameworkInternal::GameLoop;
    GameLoop loop(0.125f);
    GameInput input;
    input.KeysPressed[static_cast<size_t>(Keys::P)] = true;
    input.KeysDown[static_cast<size_t>(Keys::P)] = true;
    input.MouseDeltaX = 3;
    int fixedCount = 0, updateCount = 0, lateCount = 0;
    std::string order;
    auto fixed = [&](float dt, const GameInput& sample)
    {
        Check(dt == 0.125f, "fixed delta changed");
        Check(sample.IsKeyDown(Keys::P), "held input lost");
        Check(sample.IsKeyPressed(Keys::P) == (fixedCount == 0), "fixed edge lost or repeated");
        Check(sample.MouseDeltaX == (fixedCount == 0 ? 3.0f : 0.0f), "fixed mouse delta lost or repeated");
        ++fixedCount; order += 'F';
    };
    auto update = [&](float, const GameInput& sample)
    {
        Check(sample.IsKeyPressed(Keys::P) == (updateCount == 0), "variable edge lost or repeated");
        ++updateCount; order += 'U';
    };
    auto late = [&](float, const GameInput&) { ++lateCount; order += 'L'; };
    loop.Advance(0.0625f, input, fixed, update, late);
    Check(order == "UL", "frame without fixed tick has wrong order");
    GameFrameworkInternal::InputAccess::ClearPressed(input);
    loop.Advance(0.25f, input, fixed, update, late);
    Check(order == "ULFFUL" && fixedCount == 2 && lateCount == 2, "catch-up phase order");

    GameInput older, newer;
    older.KeysPressed[static_cast<size_t>(Keys::P)] = true;
    older.MouseDeltaX = 2;
    newer.MouseDeltaX = 4;
    GameFrameworkInternal::InputAccess::Merge(older,newer);
    Check(older.IsKeyPressed(Keys::P) && !older.IsKeyDown(Keys::P) && older.MouseDeltaX == 6, "mailbox coalescing");
    Check(!older.IsKeyDown(static_cast<Keys>(-1)), "invalid input key");

    GameLoop bounded(0.001f);
    int steps = 0;
    bounded.Advance(10.0f, {}, [&](float, const GameInput&) { ++steps; }, [](float dt, const GameInput&) {
        Check(dt == 0.25f, "variable delta not clamped");
    }, [](float, const GameInput&) {});
    Check(steps == 5, "unbounded fixed catch-up");
    bool caught = false;
    try { GameLoop invalid(0); } catch (const std::invalid_argument&) { caught = true; }
    Check(caught, "invalid timestep accepted");
    caught = false;
    bool lateAfterFailure = false;
    try { loop.Advance(0, {}, [](float, const GameInput&) {}, [](float, const GameInput&) {
        throw std::runtime_error("expected gameplay failure");
    }, [&](float, const GameInput&) { lateAfterFailure = true; }); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && !lateAfterFailure, "gameplay failure swallowed or late ran after failure");
}

int main()
{
	try { TestCulling(); TestRouting(); TestQueue(); TestWorker(); TestGameLoop(); }
	catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
	std::cout << "PASS: culling, transformed bounds, material routing, snapshot queue, worker failure and gameplay loop\n";
}
