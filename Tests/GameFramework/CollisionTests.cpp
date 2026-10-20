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
		int Enters = 0;
		int Stays = 0;
		int Exits = 0;
		Actor* LastActor = nullptr;
		std::function<void(const Collision&)> EnterAction;
		std::function<void(const Collision&)> StayAction;
		std::function<void(const Collision&)> ExitAction;

		void OnCollisionEnter(const Collision& collision) override
		{
			++Enters;
			LastActor = collision.OtherActor();
			if (EnterAction) { EnterAction(collision); }
		}
		void OnCollisionStay(const Collision& collision) override
		{
			++Stays;
			LastActor = collision.OtherActor();
			if (StayAction) { StayAction(collision); }
		}
		void OnCollisionExit(const Collision& collision) override
		{
			++Exits;
			LastActor = collision.OtherActor();
			if (ExitAction) { ExitAction(collision); }
		}
	};

	void ComponentCollisionCallbacks()
	{
		World world;
		auto* first = Sphere(world);
		auto* second = Sphere(world);
		second->SetBoxHalfExtents({1, 1, 1});
		auto* listener = first->GetOwner()->AddComponent<CollisionListener>("Listener");
		auto* anotherListener = first->GetOwner()->AddComponent<CollisionListener>("AnotherListener");
		auto* reverse = second->GetOwner()->AddComponent<CollisionListener>("Reverse");
		reverse->EnterAction = [&](const Collision& collision)
		{
			Check(collision.OtherCollider() == first, "Each side sees the opposite collider");
		};
		listener->ExitAction = [](const Collision& collision)
		{
			Check(collision.OtherCollider() == nullptr && collision.OtherActor() == nullptr,
			      "Destroyed collision participant resolves to null");
		};
		World unrelatedWorld;
		Sphere(unrelatedWorld);
		Sphere(unrelatedWorld);
		unrelatedWorld.Update(0);
		Check(listener->Enters == 0, "Collision methods are local to their World");
		world.Update(0);
		Check(listener->Enters == 1 && anotherListener->Enters == 1 &&
		      listener->LastActor == second->GetOwner() && reverse->Enters == 1,
		      "Every eligible Component on both colliding Actors receives Enter without registration");
		world.Update(0);
		second->GetOwner()->Destroy();
		world.Update(0);
		Check(listener->Stays == 1 && listener->Exits == 1, "Component methods receive Stay and Exit");
		world.Clear();
	}

	void CollisionComponentMembership()
	{
		World world;
		auto* first = Sphere(world);
		Sphere(world);
		auto* earlier = first->GetOwner()->AddComponent<CollisionListener>("Earlier");
		auto* later = first->GetOwner()->AddComponent<CollisionListener>("Later");
		auto* unrelated = world.SpawnActor("Unrelated")->AddComponent<CollisionListener>("UnrelatedListener");
		CollisionListener* added = nullptr;
		earlier->EnterAction = [&](const Collision&)
		{
			later->SetEnabled(false);
			added = first->GetOwner()->AddComponent<CollisionListener>("AddedDuringEnter");
		};
		world.Update(0);
		Check(later->Enters == 0 && added && !added->HasBegunPlay() && added->Enters == 0,
		      "Disabled Components skip pending delivery; newly added Components wait for the next frame");
		earlier->SetEnabled(false);
		later->SetEnabled(true);
		world.Update(0);
		Check(later->Stays == 1 && added->HasBegunPlay() && added->Stays == 1 && earlier->Stays == 0,
		      "Eligible Components resume on the next phase after BeginPlay");
		Check(unrelated->Enters == 0 && unrelated->Stays == 0, "Components on noncolliding Actors receive nothing");
	}

	void CollisionParticipantDestruction()
	{
		World world;
		auto* first = Sphere(world);
		auto* second = Sphere(world);
		auto* third = Sphere(world);
		const ColliderId secondId = second->GetColliderId();
		int destroyedSideCalls = 0;
		int missingOtherCalls = 0;
		first->GetOwner()->AddComponent<CollisionListener>("DestroyOther")->EnterAction = [&](const Collision& collision)
		{
			if (collision.OtherColliderId == secondId)
			{
				second->GetOwner()->Destroy();
			}
		};
		second->GetOwner()->AddComponent<CollisionListener>("DestroyedSide")->EnterAction = [&](const Collision&)
		{
			++destroyedSideCalls;
		};
		third->GetOwner()->AddComponent<CollisionListener>("SurvivingSide")->EnterAction = [&](const Collision& collision)
		{
			if (collision.OtherColliderId == secondId)
			{
				Check(collision.OtherActor() == nullptr, "Later handlers resolve pending destruction to null");
				++missingOtherCalls;
			}
		};
		world.Update(0);
		Check(destroyedSideCalls == 0 && missingOtherCalls == 1,
		      "Frozen batch reaches surviving Actors without calling destroyed Actors");
	}

	void CollisionReceiverLifetime()
	{
		World world;
		auto* first = Sphere(world);
		Sphere(world);
		auto* killer = first->GetOwner()->AddComponent<CollisionListener>("Killer");
		auto* receiver = first->GetOwner()->AddComponent<CollisionListener>("Receiver");
		world.Update(0);
		world.Update(0);
		Check(receiver->Enters == 1 && receiver->Stays == 1, "Active Component receives collision methods");
		receiver->SetEnabled(false);
		world.Update(0);
		Check(receiver->Stays == 1, "Disabled Component receives nothing");
		receiver->SetEnabled(true);
		first->GetOwner()->SetActive(false);
		world.Update(0);
		Check(receiver->Exits == 0 && receiver->Stays == 1, "Inactive Actor receives nothing");
		first->GetOwner()->SetActive(true);
		world.Update(0);
		Check(receiver->Enters == 2, "Reactivated Actor receives a fresh Enter");
		killer->StayAction = [&](const Collision&) { receiver->Destroy(); };
		world.Update(0);
		Check(receiver->Stays == 1, "Destroyed Component skips later delivery in the same batch");
		killer->StayAction = {};
		world.Update(0); // Frees the receiver. No registration can still refer to it.
		world.Clear();
		first = Sphere(world);
		Sphere(world);
		auto* replacement = first->GetOwner()->AddComponent<CollisionListener>("Replacement");
		world.Update(0);
		Check(replacement->Enters == 1, "Cleared World starts fresh with no listener cleanup");
	}

	void CollisionColliderDestruction()
	{
		World world;
		auto* first = Sphere(world);
		auto* second = Sphere(world);
		auto* destroyer = first->GetOwner()->AddComponent<CollisionListener>("DestroyCollider");
		auto* later = first->GetOwner()->AddComponent<CollisionListener>("Later");
		auto* opposite = second->GetOwner()->AddComponent<CollisionListener>("Opposite");
		destroyer->EnterAction = [&](const Collision&) { first->Destroy(); };
		world.Update(0);
		Check(destroyer->Enters == 1 && later->Enters == 0 && opposite->Enters == 1 && opposite->LastActor == nullptr,
		      "Destroying the collider stops its Actor's remaining handlers while the opposite Actor resolves it to null");
		world.Update(0);
		Check(later->Exits == 0 && opposite->Exits == 1, "Only the surviving collider's Actor receives removal Exit");
	}

	void CollisionHandlerExceptionRecovery()
	{
		World world;
		auto* first = Sphere(world);
		Sphere(world);
		auto* receiver = first->GetOwner()->AddComponent<CollisionListener>("ThrowsOnEnter");
		receiver->EnterAction = [](const Collision&) { throw std::runtime_error("Expected collision handler failure"); };
		bool caught = false;
		try { world.Update(0); }
		catch (const std::runtime_error&) { caught = true; }
		Check(caught, "Component collision exception reaches the caller");
		world.Update(0);
		Check(receiver->Enters == 1 && receiver->Stays == 1, "World can update again after a collision handler throws");
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
		const auto highestLayer = static_cast<CollisionLayer>(31);
		const auto outOfRangeLayer = static_cast<CollisionLayer>(32);
		Check(first->GetLayer() == CollisionLayer::Default && first->GetMask() == UINT32_MAX, "Neutral filter defaults");
		Check(second->SetLayer(highestLayer) && !second->SetLayer(outOfRangeLayer) && second->GetLayer() == highestLayer, "Layer range preserves valid state");
		world.Update(0);
		Check(events.Values.size() == 1, "Default masks include layer31");
		first->SetOverlapsWith({CollisionLayer::Default});
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Exit, "One-sided rejection exits");
		first->SetOverlapsWith({highestLayer});
		second->SetOverlapsWith({CollisionLayer::Default});
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Enter, "Bilateral masks reenter");
		second->SetOverlapsWithNone();
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Exit, "Zero mask exits");
		events.Values.clear();
		world.Update(0);
		Check(events.Values.empty(), "Exited pairs do not repeat Exit");

		Check(first->SetOverlapsWith({CollisionLayer::Default, highestLayer, highestLayer}), "Multiple accepted layers allow duplicates and layer31");
		Check(!first->SetOverlapsWith({CollisionLayer::Player, outOfRangeLayer}) && first->GetMask() == ((std::uint32_t{1} << 31) | 1u),
		      "Invalid list preserves the previous filter without applying its valid prefix");
		second->SetOverlapsWithAll();
		world.Update(0);
		Check(events.Values.size() == 1 && events.Values.back().Phase == CollisionOverlapPhase::Enter,
		      "All-layer filter accepts the other collider");
		Check(first->SetOverlapsWith({CollisionLayer::Default}), "Accepted layer list replaces previous layers");
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Exit, "Replacement list removes previous layer31");
		Check(first->SetOverlapsWith({}), "Empty accepted layer list is valid");
		Check(first->GetMask() == 0, "Empty list accepts no layers");
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

	void SphereBoxPhases()
	{
		for (bool boxFirst : {true, false})
		{
			World world;
			Events events(world);
			auto* first = Sphere(world);
			auto* second = Sphere(world, {2, 0, 0});
			auto* box = boxFirst ? first : second;
			box->SetBoxHalfExtents({1, 1, 1});
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
		world.Clear();
		events.Values.clear();
		auto* changing = Sphere(world);
		Sphere(world, {2.5f, 0, 0});
		world.Update(0);
		Check(events.Values.empty(), "Separated initial spheres");
		changing->SetBoxHalfExtents({2, 1, 1});
		world.Update(0);
		Check(events.Values.size() == 1 && events.Values[0].Phase == CollisionOverlapPhase::Enter, "Enabled shape change creates collision");
		changing->SetSphereRadius(1);
		world.Update(0);
		Check(events.Values.back().Phase == CollisionOverlapPhase::Exit, "Sphere setter switches shape back");
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
		Sphere(world, {2.5f, 0, 0});
		first->GetTransform().SetLocalScale({2, 2, 2});
		world.Update(0);
		Check(events.Values.size() == 1 && events.Values[0].Phase == CollisionOverlapPhase::Enter,
		      "Uniformly scaled sphere reaches the other collider");
		first->GetTransform().SetMatrix(CommonUtilities::Matrix4f{});
		world.Update(0);
		Check(events.Values.size() == 2 && events.Values[1].Phase == CollisionOverlapPhase::Exit,
		      "Current matrix overrides the previously authored scale");
	}
}

void RunCollisionImportTests();

int main()
{
	try
	{
		ServiceLocator::GetInstance().Initialize();
		RunCollisionImportTests();
		ComponentCollisionCallbacks();
		CollisionComponentMembership();
		CollisionParticipantDestruction();
		CollisionReceiverLifetime();
		CollisionColliderDestruction();
		CollisionHandlerExceptionRecovery();
		SpherePhases();
		Filtering();
		Lifetime();
		FrozenMembershipAndCurrentTransforms();
		CallbackBatchAndExceptionRecovery();
		SubscriptionLifetime();
		SphereBoxPhases();
		BoxBoxTouching();
		OrientedGeometry();
		BoxScaleOffsetAndShapeChanges();
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
