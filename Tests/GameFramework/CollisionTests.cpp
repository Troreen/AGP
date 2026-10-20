#include "GameFramework/Components/ColliderComponent.h"
#include "GameFramework/Collision/CollisionOverlapEvent.h"
#include "GameFramework/World/World.h"
#include "GameFramework/EventBus.hpp"
#include "GameFramework/ServiceLocator.h"
#include "Maths.hpp"
#include "GraphicsEngine/Drawers/LineDrawer.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <functional>
#include <limits>

namespace
{
	void Check(bool value, const char* message)
	{
		if (!value)
		{
			throw std::runtime_error(message);
		}
	}

	struct Events
	{
		std::vector<CollisionOverlapEvent> Values;
		std::uint32_t Listener;

		explicit Events(World& world)
		{
			Listener = ServiceLocator::GetInstance().GetEventBus().AddEventListener<CollisionOverlapEvent>(
			    [this, &world](const CollisionOverlapEvent& event)
			{
				if (event.SourceWorld == &world)
				{
					Values.push_back(event);
				}
			});
		}

		~Events()
		{
			ServiceLocator::GetInstance().GetEventBus().RemoveEventListener(Listener);
		}
	};

	ColliderComponent* Sphere(World& world, const CommonUtilities::Vector3f& position = {})
	{
		Actor* actor = world.SpawnActor("Volume");
		actor->GetTransform().SetLocalPosition(position);
		return actor->AddComponent<ColliderComponent>("Collider");
	}

	struct CollisionListener : Component
	{
		CollisionSubscription Subscription;
		int Calls = 0;
		Actor* LastActor = nullptr;

		void OnCollision(const Collision& collision)
		{
			++Calls;
			LastActor = collision.OtherActor();
		}
	};

	void ColliderCallbacks()
	{
		World world;
		auto* first = Sphere(world);
		auto* second = Sphere(world);
		second->SetBoxHalfExtents({1, 1, 1});
		auto* listener = first->GetOwner()->AddComponent<CollisionListener>("Listener");
		listener->Subscription = first->OnEnter(listener, &CollisionListener::OnCollision);
		std::vector<CollisionOverlapPhase> phases;
		auto stay = first->OnStay([&](const Collision& collision)
		{
			phases.push_back(collision.Phase);
		});
		auto exit = first->OnExit([&](const Collision& collision)
		{
			phases.push_back(collision.Phase);
			Check(collision.OtherCollider() == nullptr && collision.OtherActor() == nullptr,
			      "Destroyed collision participant resolves to null");
		});
		int reverseCalls = 0;
		auto reverse = second->OnEnter([&](const Collision& collision)
		{
			++reverseCalls;
			Check(collision.OtherCollider() == first, "Each side sees the opposite collider");
		});
		World unrelatedWorld;
		Sphere(unrelatedWorld);
		Sphere(unrelatedWorld);
		unrelatedWorld.Update(0);
		Check(listener->Calls == 0, "Callbacks are local to their World");
		world.Update(0);
		Check(listener->Calls == 1 && listener->LastActor == second->GetOwner() && reverseCalls == 1,
		      "Sphere and box support member and lambda callbacks on both sides");
		world.Update(0);
		second->GetOwner()->Destroy();
		world.Update(0);
		Check(phases == std::vector{CollisionOverlapPhase::Stay, CollisionOverlapPhase::Exit},
		      "Stay and Exit callbacks are phase filtered");
		world.Clear();
		stay.Reset();
		exit.Reset();
		reverse.Reset();
	}

	void CallbackSubscriptionLifetime()
	{
		World world;
		auto* first = Sphere(world);
		auto* second = Sphere(world);
		int disconnectedCalls = 0;
		CollisionSubscription later;
		CollisionSubscription added;
		int addedCalls = 0;
		auto enter = first->OnEnter([&](const Collision&)
		{
			later.Reset();
			added = first->OnEnter([&](const Collision&)
			{
				++addedCalls;
			});
		});
		later = first->OnEnter([&](const Collision&)
		{
			++disconnectedCalls;
		});
		{
			auto temporary = first->OnEnter([&](const Collision&)
			{
				++disconnectedCalls;
			});
		}
		world.Update(0);
		Check(disconnectedCalls == 0 && addedCalls == 0,
		      "Disconnect skips pending callbacks; new callbacks wait for the next notification");
		enter.Reset();
		CollisionSubscription moved = std::move(added);
		second->SetEnabled(false);
		world.Update(0);
		second->SetEnabled(true);
		world.Update(0);
		Check(addedCalls == 1, "Moving a subscription retains its callback");
		moved = first->OnStay([&](const Collision&)
		{
			++disconnectedCalls;
		});
		world.Update(0);
		Check(disconnectedCalls == 1, "Replacing a subscription binds the new callback");
		moved.Reset();
		world.Update(0);
		Check(disconnectedCalls == 1, "Reset stops later delivery");
	}

	void CallbackParticipantDestruction()
	{
		World world;
		auto* first = Sphere(world);
		auto* second = Sphere(world);
		auto* third = Sphere(world);
		const ColliderId secondId = second->GetColliderId();
		int destroyedSideCalls = 0;
		int missingOtherCalls = 0;
		auto destroy = first->OnEnter([&](const Collision& collision)
		{
			if (collision.OtherColliderId == secondId)
			{
				second->GetOwner()->Destroy();
			}
		});
		auto destroyedSide = second->OnEnter([&](const Collision&)
		{
			++destroyedSideCalls;
		});
		auto survivingSide = third->OnEnter([&](const Collision& collision)
		{
			if (collision.OtherColliderId == secondId)
			{
				Check(collision.OtherActor() == nullptr, "Later callbacks resolve pending destruction to null");
				++missingOtherCalls;
			}
		});
		world.Update(0);
		Check(destroyedSideCalls == 0 && missingOtherCalls == 1,
		      "Frozen batch reaches surviving colliders without calling destroyed colliders");
	}

	void CallbackReceiverLifetime()
	{
		World world;
		auto* first = Sphere(world);
		Sphere(world);
		auto* receiver = world.SpawnActor("Receiver")->AddComponent<CollisionListener>("Listener");
		receiver->Subscription = first->OnStay(receiver, &CollisionListener::OnCollision);
		world.Update(0);
		world.Update(0);
		Check(receiver->Calls == 1, "Active member receiver gets Stay");
		receiver->SetEnabled(false);
		world.Update(0);
		Check(receiver->Calls == 1, "Disabled receiver gets no callback");
		receiver->SetEnabled(true);
		receiver->GetOwner()->SetActive(false);
		world.Update(0);
		Check(receiver->Calls == 1, "Inactive receiver gets no callback");
		receiver->GetOwner()->SetActive(true);
		world.Update(0);
		Check(receiver->Calls == 2, "Reactivated receiver resumes callbacks");
		auto killer = first->OnStay([&](const Collision&)
		{
			receiver->Destroy();
		});
		// Rebind after the killer so pending destruction is checked during the same batch.
		receiver->Subscription = first->OnStay(receiver, &CollisionListener::OnCollision);
		world.Update(0);
		Check(receiver->Calls == 2, "Destroyed receiver skips the later callback in this batch");
		killer.Reset();
		world.Update(0);
		world.Clear();
		first = Sphere(world);
		Sphere(world);
		world.Update(0);
	}

	void SpherePhases()
	{
		World world;
		Events events(world);
		auto* first = Sphere(world);
		auto* second = Sphere(world, {2, 0, 0});
		Check(first->GetColliderId() != 0 && second->GetColliderId() > first->GetColliderId(), "IDs are assigned on attachment");
		world.Update(0);
		world.Update(0);
		second->GetTransform().SetLocalPosition({1, 0, 0});
		world.Update(0);
		Check(events.Values.size() == 3, "One pair event per pass, including touching");
		Check(events.Values[0].Phase == CollisionOverlapPhase::Enter && events.Values[1].Phase == CollisionOverlapPhase::Stay &&
		          events.Values[2].Phase == CollisionOverlapPhase::Exit,
		      "Sphere Enter Stay Exit");
		Check(events.Values[0].FirstColliderId == first->GetColliderId() && events.Values[0].SecondColliderId == second->GetColliderId(),
		      "Ordered pair IDs");
	}

	void Filtering()
	{
		World world;
		Events events(world);
		auto* first = Sphere(world);
		auto* second = Sphere(world);
		Check(first->GetLayer() == 0 && first->GetMask() == UINT32_MAX, "Neutral filter defaults");
		Check(second->SetLayer(31) && !second->SetLayer(32) && second->GetLayer() == 31, "Layer range preserves valid state");
		world.Update(0);
		Check(events.Values.size() == 1, "Default masks include layer31");
		first->SetMask(1);
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Exit, "One-sided rejection exits");
		first->SetMask(std::uint32_t{1} << 31);
		second->SetMask(1);
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Enter, "Bilateral masks reenter");
		second->SetMask(0);
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Exit, "Zero mask exits");
		events.Values.clear();
		world.Update(0);
		Check(events.Values.empty(), "Exited pairs do not repeat Exit");
	}

	void Lifetime()
	{
		World world;
		Events events(world);
		auto* first = Sphere(world);
		auto* second = Sphere(world);
		const ColliderId removedId = second->GetColliderId();
		second->SetEnabled(false);
		bool rejected = false;
		try
		{
			second->GetOwner()->AddComponent<ColliderComponent>("Second");
		}
		catch (const std::invalid_argument&)
		{
			rejected = true;
		}
		Check(rejected && world.FindCollider(removedId) == second, "Disabled collider still owns its Actor slot and lookup");
		second->SetEnabled(true);
		world.Update(0);
		second->SetEnabled(false);
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Exit, "Disable exits");
		second->SetEnabled(true);
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Enter, "Reenable enters");
		second->GetOwner()->SetActive(false);
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Exit, "Inactive owner exits");
		second->GetOwner()->SetActive(true);
		world.Update(0);
		Actor* owner = second->GetOwner();
		second->Destroy();
		Check(world.FindCollider(removedId) == nullptr, "Pending destroy is absent from lookup immediately");
		auto* replacement = owner->AddComponent<ColliderComponent>("Collider");
		const ColliderId replacementId = replacement->GetColliderId();
		Check(replacementId > removedId, "Replacement has fresh ID");
		events.Values.clear();
		world.Update(0);
		Check(events.Values.size() == 2 && world.FindCollider(removedId) == nullptr && world.FindCollider(replacementId) == replacement,
		      "Replacement enters while released component exits safely");
		Check(events.Values.back().FirstColliderId == first->GetColliderId() && events.Values.back().SecondColliderId == removedId &&
		          events.Values.back().Phase == CollisionOverlapPhase::Exit,
		      "Exit retains released ID");
		events.Values.clear();
		world.Clear();
		Check(events.Values.empty(), "Clear emits no teardown events");
		Check(Sphere(world)->GetColliderId() > replacementId, "Clear preserves monotonic identities");
	}

	class Action : public Component
	{
	public:
		std::function<void()> OnBegin, OnUpdate;

		void BeginPlay() override
		{
			if (OnBegin)
			{
				OnBegin();
			}
		}

		void Update(float) override
		{
			if (OnUpdate)
			{
				OnUpdate();
			}
		}
	};

	void FrozenMembershipAndCurrentTransforms()
	{
		World world;
		Events events(world);
		auto* first = Sphere(world);
		first->GetOwner()->GetTransform().SetLocalPosition({10, 0, 0});
		first->GetOwner()->GetTransform().SetLocalScale({2, 2, 2});
		first->GetTransform().SetLocalPosition({1, 0, 0});
		first->GetTransform().SetLocalRotationDegrees(30, 70, 20);
		auto* action = world.SpawnActor("Spawner")->AddComponent<Action>("Action");
		action->OnBegin = [&]
		{
			Sphere(world, {15, 0, 0});
		};
		world.Update(0);
		Check(events.Values.empty(), "BeginPlay additions wait for next frame");
		world.Update(0);
		Check(events.Values.size() == 1 && events.Values.back().Phase == CollisionOverlapPhase::Enter,
		      "Live composed sphere offset scale and rotation");
		action->OnBegin = {};
		action->OnUpdate = [&]
		{
			Sphere(world, {9, 0, 0});
			action->OnUpdate = {};
		};
		world.Update(0);
		Check(events.Values.size() == 2, "Update additions wait for next frame");
		world.Update(0);
		Check(events.Values.size() == 4, "Independent occupants produce distinct pairs");
	}

	void CallbackBatchAndExceptionRecovery()
	{
		World world;
		Events events(world);
		Sphere(world, {-1.5f, 0, 0});
		auto* middle = Sphere(world);
		const ColliderId destroyedId = middle->GetColliderId();
		Sphere(world, {1.5f, 0, 0});
		auto& bus = ServiceLocator::GetInstance().GetEventBus();
		const auto listener = bus.AddEventListener<CollisionOverlapEvent>([&](const CollisionOverlapEvent& event)
		{
			if (event.SourceWorld == &world && event.Phase == CollisionOverlapPhase::Enter)
			{
				middle->GetOwner()->Destroy();
			}
		});
		world.Update(0);
		Check(events.Values.size() == 2 && events.Values[1].Phase == CollisionOverlapPhase::Enter,
		      "Callback destruction does not change collected batch");
		bus.RemoveEventListener(listener);
		events.Values.clear();
		world.Update(0);
		Check(events.Values.size() == 2 && events.Values[0].Phase == CollisionOverlapPhase::Exit &&
		          events.Values[1].Phase == CollisionOverlapPhase::Exit && world.FindCollider(destroyedId) == nullptr,
		      "Released actor exits all pairs without dangling lookup");
		world.Clear();
		events.Values.clear();
		Sphere(world);
		Sphere(world);
		const auto throwing = bus.AddEventListener<CollisionOverlapEvent>([&](const CollisionOverlapEvent& event)
		{
			if (event.SourceWorld == &world)
			{
				throw std::runtime_error("callback");
			}
		});
		bool propagated = false;
		try
		{
			world.Update(0);
		}
		catch (const std::runtime_error&)
		{
			propagated = true;
		}
		bus.RemoveEventListener(throwing);
		Check(propagated, "Callback exceptions propagate");
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Stay, "World guards and sampled pairs survive callback exception");
	}

	class Receiver : public Component
	{
	public:
		int* Deliveries = nullptr;
		std::uint32_t Listener = 0;

		void BeginPlay() override
		{
			Listener = ServiceLocator::GetInstance().GetEventBus().AddEventListener<CollisionOverlapEvent>(
			    [this](const CollisionOverlapEvent& event)
			{
				if (event.SourceWorld == &GetWorld())
				{
					++*Deliveries;
				}
			});
		}

		void EndPlay() noexcept override
		{
			ServiceLocator::GetInstance().GetEventBus().RemoveEventListener(Listener);
		}
	};

	void SubscriptionLifetime()
	{
		World world;
		int deliveries = 0;
		auto* receiver = world.SpawnActor("Receiver")->AddComponent<Receiver>("Receiver");
		receiver->Deliveries = &deliveries;
		Sphere(world);
		Sphere(world);
		world.Update(0);
		Check(deliveries == 1, "Gameplay subscribes in BeginPlay before detection");
		world.Clear();
		Sphere(world);
		Sphere(world);
		world.Update(0);
		Check(deliveries == 1, "World Clear calls EndPlay and removes old listener");
	}

	void InvalidSphereConfigurationAndTransforms()
	{
		World world;
		Events events(world);
		auto* first = Sphere(world);
		Sphere(world);
		Check(!first->SetSphereRadius(0) && !first->SetSphereRadius(-1) &&
		          !first->SetSphereRadius(std::numeric_limits<float>::infinity()) && first->GetSphereRadius() == 1,
		      "Invalid sphere dimensions preserve valid value");
		world.Update(0);
		first->GetTransform().SetLocalScale({1, 2, 1});
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Exit, "Nonuniform sphere excluded");
		first->GetTransform().SetLocalScale({1, 1, 1});
		world.Update(0);
		CommonUtilities::Matrix4f reflection;
		reflection(1, 1) = -1;
		first->GetOwner()->GetTransform().SetMatrix(reflection);
		first->GetTransform().SetMatrix(reflection);
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Exit, "Source reflections excluded even when final reflections cancel");
	}

	void SphereBoxPhases()
	{
		for (bool boxFirst : {true, false})
		{
			World world;
			Events events(world);
			auto* first = Sphere(world);
			auto* second = Sphere(world, {2, 0, 0});
			auto* box = boxFirst ? first : second;
			Check(box->SetBoxHalfExtents({1, 1, 1}), "Box dimensions accepted");
			world.Update(0);
			world.Update(0);
			second->GetTransform().SetLocalPosition({0.1f, 0, 0});
			world.Update(0);
			Check(events.Values.size() == 3 && events.Values[0].Phase == CollisionOverlapPhase::Enter &&
			          events.Values[1].Phase == CollisionOverlapPhase::Stay && events.Values[2].Phase == CollisionOverlapPhase::Exit,
			      "Sphere box face touching and phases in either order");
		}
	}

	void BoxBoxTouching()
	{
		World world;
		Events events(world);
		auto* first = Sphere(world);
		auto* second = Sphere(world, {2, 0, 0});
		first->SetBoxHalfExtents({1, 1, 1});
		second->SetBoxHalfExtents({1, 1, 1});
		world.Update(0);
		Check(events.Values.size() == 1 && events.Values[0].Phase == CollisionOverlapPhase::Enter, "Box faces touching collision");
		second->GetOwner()->GetTransform().SetLocalPosition({2, 2, 2});
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Stay, "Box corner touching overlaps");
		second->GetOwner()->GetTransform().SetLocalPosition({2, 2.1f, 2});
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Exit, "Vertical gap separates boxes");
	}

	void OrientedGeometry()
	{
		World world;
		Events events(world);
		auto* box = Sphere(world);
		box->SetBoxHalfExtents({2, 1, 0.2f});
		box->GetTransform().SetLocalRotationDegrees(45, 0, 0);
		auto* sphere = Sphere(world, {1.4f, 0, 1.4f});
		sphere->SetSphereRadius(0.1f);
		world.Update(0);
		Check(events.Values.empty(), "Rotated box enclosing AABB does not establish sphere collision");
		sphere->GetOwner()->GetTransform().SetLocalPosition({1.4f, 0, -1.4f});
		world.Update(0);
		Check(events.Values.size() == 1, "Sphere reaches long oriented box end");
		sphere->GetOwner()->GetTransform().SetLocalPosition({0, 2, 0});
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Exit, "Vertical separation excludes sphere box");
		world.Clear();
		events.Values.clear();
		box = Sphere(world);
		box->SetBoxHalfExtents({2, 1, 0.2f});
		box->GetTransform().SetLocalRotationDegrees(45, 0, 0);
		auto* other = Sphere(world, {0, 0, 1});
		other->SetBoxHalfExtents({2, 1, 0.2f});
		other->GetTransform().SetLocalRotationDegrees(45, 0, 0);
		world.Update(0);
		Check(events.Values.empty(), "Parallel thin boxes separate despite overlapping enclosing AABBs");
		other->GetOwner()->GetTransform().SetLocalPosition({1, 0, -1});
		world.Update(0);
		Check(events.Values.size() == 1, "Parallel yaw boxes collision along their length");
		other->GetTransform().SetLocalRotationDegrees(-45, 0, 0);
		other->GetOwner()->GetTransform().SetLocalPosition({0, 0, 0});
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Stay, "Different yaw boxes cross");
	}

	void BoxScaleOffsetAndShapeChanges()
	{
		World world;
		Events events(world);
		auto* box = Sphere(world, {10, 0, 0});
		box->SetBoxHalfExtents({1, 1, 1});
		box->GetOwner()->GetTransform().SetLocalScale({2, 3, 4});
		box->GetOwner()->GetTransform().SetLocalRotationDegrees(90, 0, 0);
		box->GetTransform().SetLocalPosition({1, 0, 0});
		Sphere(world, {10, 0, -5});
		world.Update(0);
		Check(events.Values.size() == 1, "Actor-relative offset yaw and positive nonuniform box scaling compose");
		box->GetTransform().SetLocalRotationDegrees(45, 0, 0);
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Exit, "Composed nonuniform-scale shear excluded");
		world.Clear();
		events.Values.clear();
		auto* changing = Sphere(world);
		Sphere(world, {2.5f, 0, 0});
		world.Update(0);
		Check(events.Values.empty(), "Separated initial spheres");
		changing->SetBoxHalfExtents({2, 1, 1});
		world.Update(0);
		Check(events.Values.size() == 1 && events.Values[0].Phase == CollisionOverlapPhase::Enter, "Enabled shape change creates collision");
		Check(!changing->SetBoxHalfExtents({0, 1, 1}) && !changing->SetBoxHalfExtents({1, -1, 1}) &&
		          !changing->SetBoxHalfExtents({1, 1, std::numeric_limits<float>::quiet_NaN()}) &&
		          changing->GetShape() == ColliderShape::Box && changing->GetBoxHalfExtents().x == 2,
		      "Invalid box dimensions preserve shape and size");
		changing->SetSphereRadius(1);
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Exit, "Sphere setter switches shape back");
	}

	void UnsupportedBoxTransforms()
	{
		World world;
		Events events(world);
		auto* box = Sphere(world);
		box->SetBoxHalfExtents({1, 1, 1});
		Sphere(world);
		world.Update(0);
		const auto checkExcluded = [&](const CommonUtilities::Matrix4f& matrix, const char* message)
		{
			box->GetTransform().SetMatrix(matrix);
			world.Update(0);
			Check(events.Values.back().Phase == CollisionOverlapPhase::Exit, message);
			box->GetTransform().SetMatrix(CommonUtilities::Matrix4f{});
			world.Update(0);
			Check(events.Values.back().Phase == CollisionOverlapPhase::Enter, "Restored geometry reenters");
		};
		Transform tilted;
		tilted.SetLocalRotationDegrees(0, 15, 0);
		checkExcluded(tilted.GetLocalMatrix(), "Pitch excluded");
		tilted.SetLocalRotationDegrees(0, 0, 15);
		checkExcluded(tilted.GetLocalMatrix(), "Roll excluded");
		CommonUtilities::Matrix4f matrix;
		matrix(1, 3) = 0.4f;
		checkExcluded(matrix, "Raw shear excluded");
		matrix = CommonUtilities::Matrix4f{};
		matrix(1, 1) = -1;
		checkExcluded(matrix, "Raw reflection excluded");
		matrix = CommonUtilities::Matrix4f{};
		matrix(1, 4) = 0.2f;
		checkExcluded(matrix, "Nonaffine transform excluded");
		matrix = CommonUtilities::Matrix4f{};
		matrix(4, 1) = std::numeric_limits<float>::infinity();
		checkExcluded(matrix, "Nonfinite transform excluded");
		matrix = CommonUtilities::Matrix4f{};
		matrix(2, 2) = 0;
		checkExcluded(matrix, "Zero scale excluded");
	}

	void DebugDrawing()
	{
		World world;
		auto* collider = Sphere(world, {10, 20, 30});
		auto& drawer = LineDrawer::Get();
		drawer.Clear();
		world.Update(0);
		Check(!collider->IsDebugDrawEnabled() && drawer.GetLines().empty(), "Debug drawing defaults off");

		collider->SetDebugDrawEnabled(true);
		collider->SetDebugColor({0.2f, 0.3f, 0.4f, 1});
		collider->SetDebugDisabledColor({0.6f, 0.7f, 0.8f, 1});
		collider->SetSphereRadius(2);
		collider->GetTransform().SetLocalPosition({1, 2, 3});
		collider->GetOwner()->GetTransform().SetLocalScale({2, 2, 2});
		// This component updates after the collider, so drawing must wait for all updates.
		auto* mover = collider->GetOwner()->AddComponent<Action>("Mover");
		mover->OnUpdate = [collider]() { collider->GetOwner()->GetTransform().SetWorldPosition({40, 50, 60}); };
		world.Update(0);
		Check(!drawer.GetLines().empty(), "Enabled sphere queues an outline");
		CommonUtilities::Vector3f minimum{1000, 1000, 1000};
		CommonUtilities::Vector3f maximum{-1000, -1000, -1000};
		for (const auto& vertex : drawer.GetLines())
		{
			Check(vertex.Color == collider->GetDebugColor(), "Configured enabled color reaches the drawer");
			minimum.x = (std::min)(minimum.x, vertex.Position.x);
			minimum.y = (std::min)(minimum.y, vertex.Position.y);
			minimum.z = (std::min)(minimum.z, vertex.Position.z);
			maximum.x = (std::max)(maximum.x, vertex.Position.x);
			maximum.y = (std::max)(maximum.y, vertex.Position.y);
			maximum.z = (std::max)(maximum.z, vertex.Position.z);
		}
		Check((minimum - CommonUtilities::Vector3f{38, 50, 62}).Length() < 0.001f &&
			(maximum - CommonUtilities::Vector3f{46, 58, 70}).Length() < 0.001f,
			"Sphere drawing follows current movement, offset and scale");

		drawer.Clear();
		collider->SetEnabled(false);
		collider->SetBoxHalfExtents({1, 2, 3});
		world.Update(0);
		Check(drawer.GetLines().size() == 24, "Disabled box still draws twelve edges");
		for (const auto& vertex : drawer.GetLines())
		{
			Check(vertex.Color == collider->GetDebugDisabledColor(), "Disabled color reaches the drawer");
			Check(std::abs(vertex.Position.x - 42) == 2 && std::abs(vertex.Position.y - 54) == 4 &&
				std::abs(vertex.Position.z - 66) == 6, "Box corners follow composed scale and offset");
		}
		drawer.Clear();
		collider->SetDebugDrawEnabled(false);
		world.Update(0);
		Check(drawer.GetLines().empty(), "Drawing can be toggled off independently");
		collider->SetDebugDrawEnabled(true);
		collider->GetOwner()->SetActive(false);
		world.Update(0);
		Check(drawer.GetLines().empty(), "Inactive owners do not draw");
		collider->GetOwner()->SetActive(true);
		mover->OnUpdate = {};
		collider->Destroy();
		world.Update(0);
		Check(drawer.GetLines().empty(), "Destroyed colliders do not draw");
	}

	void RawMatrixOverridesAuthoredScale()
	{
		World world;
		Events events(world);
		auto* first = Sphere(world);
		Sphere(world);
		first->GetTransform().SetLocalScale({-1, 1, 1});
		world.Update(0);
		Check(events.Values.empty(), "Reflected source matrix excluded");
		first->GetTransform().SetMatrix(CommonUtilities::Matrix4f{});
		world.Update(0);
		Check(events.Values.size() == 1 && events.Values[0].Phase == CollisionOverlapPhase::Enter,
		      "Valid current matrix overrides stale authored scale");
	}
}

void RunCollisionImportTests();

int main()
{
	try
	{
		ServiceLocator::GetInstance().Initialize();
		RunCollisionImportTests();
		ColliderCallbacks();
		CallbackSubscriptionLifetime();
		CallbackParticipantDestruction();
		CallbackReceiverLifetime();
		SpherePhases();
		Filtering();
		Lifetime();
		FrozenMembershipAndCurrentTransforms();
		CallbackBatchAndExceptionRecovery();
		SubscriptionLifetime();
		InvalidSphereConfigurationAndTransforms();
		SphereBoxPhases();
		BoxBoxTouching();
		OrientedGeometry();
		BoxScaleOffsetAndShapeChanges();
		UnsupportedBoxTransforms();
		RawMatrixOverridesAuthoredScale();
		DebugDrawing();
		ServiceLocator::GetInstance().KillServices();
		std::cout << "PASS: CollisionTests" << std::endl;
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL: " << error.what() << '\n';
		return 1;
	}
}
