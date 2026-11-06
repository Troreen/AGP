#include "GameFramework.pch.h"
#include "GameFramework/AssetHandling/MaterialAsset.h"
#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GameFramework/Collision/CollisionSystem.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/ColliderComponent.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Components/ParticleComponent.h"
#include "GameFramework/World/World.h"
#include "Maths.hpp"

#include <AISystems/Pathfinder.hpp>
#include <algorithm>
#include <CommonUtilities/InputHandler.h>
#include <CommonUtilities/Intersection.hpp>
#include <CommonUtilities/Sphere.hpp>
#include <GameFramework/Components/AnimatorComponent.h>
#include <GraphicsEngine/GraphicsEngine.h>
#include <GraphicsEngine/Objects/Mesh.h>
#include <GraphicsEngine/Drawers/LineDrawer.h>
#include <ServiceLocator.h>
#include <stdexcept>

namespace
{
	constexpr float MaxFrameDeltaSeconds = 0.25f;
}

World::World() : myCollisionSystem(std::make_unique<CollisionSystem>())
{
}

void World::RegisterCollider(Component& component)
{
	if (auto* collider = dynamic_cast<ColliderComponent*>(&component))
	{
		myCollisionSystem->RegisterCollider(*collider);
	}
}

ColliderComponent* World::FindCollider(std::uint64_t id) const
{
	// Disabled colliders are still findable. Destroyed Actors and Components are treated as already gone.
	for (const auto& actor : myActors)
	{
		if (actor->IsPendingDestroy())
		{
			continue;
		}
		if (auto* collider = actor->GetComponent<ColliderComponent>(); collider && collider->GetColliderId() == id)
		{
			return collider;
		}
	}
	return nullptr;
}

World::~World()
{
	Clear();
}

World& World::operator=(World&&) = default;

Actor* World::SpawnActor(std::string_view name)
{
	if (myClearing)
	{
		throw std::logic_error("Cannot spawn during World cleanup");
	}
	if (name.empty())
	{
		throw std::invalid_argument(std::format("Actor name must be nonempty: {}", name));
	}

	std::string finalName(name);
	++myActorNameCount[finalName];
	if (myActorNameCount[finalName] > 1)
	{
		finalName += "_" + std::to_string(myActorNameCount[finalName]);
	}

	std::unique_ptr<Actor> actor = std::make_unique<Actor>(*this, std::move(finalName));
	Actor* spawnedActor = actor.get();
	myActors.push_back(std::move(actor));
	return spawnedActor;
}

Actor* World::FindActor(std::string_view name) const
{
	for (const auto& actor : myActors)
	{
		if (!actor->IsPendingDestroy() && actor->GetName() == name)
		{
			return actor.get();
		}
	}
	return nullptr;
}

bool World::SetCameraResolution(const CommonUtilities::Vector2u& aResolution)
{
	// Resize every camera so switching the active camera keeps the new aspect ratio.
	for (const std::unique_ptr<Actor>& actor : myActors)
	{
		if (actor->IsPendingDestroy())
		{
			continue;
		}

		for (const std::unique_ptr<Component>& component : actor->myComponents)
		{
			if (component->IsPendingDestroy())
			{
				continue;
			}

			if (CameraComponent* camera = dynamic_cast<CameraComponent*>(component.get()))
			{
				if (!camera->SetResolution(aResolution))
				{
					return false;
				}
			}
		}
	}

	return true;
}

bool World::SetActiveCamera(CameraComponent* camera)
{
	if (camera && (&camera->GetWorld() != this || camera->IsPendingDestroy() || camera->GetOwner()->IsPendingDestroy()))
	{
		return false;
	}
	myCamera = camera;
	ServiceLocator::GetInstance().SetActiveCamera(myCamera);
	return true;
}

CameraComponent* World::GetActiveCamera() const
{
	return myCamera && !myCamera->IsPendingDestroy() && !myCamera->GetOwner()->IsPendingDestroy() ? myCamera : nullptr;
}

void World::RemoveDestroyed()
{
	if (!GetActiveCamera())
	{
		myCamera = nullptr;
	}
	// Detach objects before callbacks so callbacks cannot invalidate vector iteration.
	std::vector<std::unique_ptr<Actor>> actors;
	actors.reserve(myActors.size());
	std::vector<std::unique_ptr<Component>> components;
	components.reserve(myActors.size());
	for (std::unique_ptr<Actor>& actor : myActors)
	{
		if (actor->IsPendingDestroy())
		{
			--myActorNameCount[actor->GetName()];
			actors.push_back(std::move(actor));
			continue;
		}

		for (std::unique_ptr<Component>& component : actor->myComponents)
		{
			if (component->IsPendingDestroy())
			{
				components.push_back(std::move(component));
			}
		}
		std::erase(actor->myComponents, nullptr);
	}
	std::erase(myActors, nullptr);
	for (std::unique_ptr<Component>& component : components)
	{
		if (component->myBegun)
		{
			component->EndPlay();
		}
	}
	// Destroy detached components before Actors; their GetOwner still refers to live memory.
	components.clear();
	actors.clear();
}

std::vector<Component*> World::CollectFrame()
{
	std::vector<Component*> frame;
	frame.reserve(myActors.size());
	for (const auto& actor : myActors)
	{
		if (!actor->IsPendingDestroy())
		{
			for (const auto& component : actor->myComponents)
			{
				if (!component->IsPendingDestroy())
				{
					frame.push_back(component.get());
				}
			}
		}
	}
	return frame;
}

void World::BeginPlay()
{
	if (myUpdating || myClearing)
	{
		throw std::logic_error("World lifecycle cannot run recursively");
	}
	myUpdating = true;
	try
	{
		// Freeze this list: additions made by BeginPlay wait for the next update.
		for (Component* component : CollectFrame())
		{
			if (!component->myBegun && !component->IsPendingDestroy() && !component->GetOwner()->IsPendingDestroy())
			{
				component->myBegun = true;
				component->BeginPlay();
			}
		}
	}
	catch (...)
	{
		myUpdating = false;
		throw;
	}
	myUpdating = false;
}

void World::Update(float deltaTime)
{
	if (myUpdating || myClearing)
	{
		throw std::logic_error("World lifecycle cannot run recursively");
	}
	deltaTime = CU::IsFinite(deltaTime) ? CU::Clamp(deltaTime, 0.0f, MaxFrameDeltaSeconds) : 0.0f;
	mySimulationTime += deltaTime;
	myUpdating = true;
	try
	{
		RemoveDestroyed();
		const std::vector<Component*> frame = CollectFrame();
		for (Component* component : frame)
		{
			if (!component->myBegun && !component->IsPendingDestroy() && !component->GetOwner()->IsPendingDestroy())
			{
				component->myBegun = true;
				component->BeginPlay();
			}
		}
		for (Component* component : frame)
		{
			if (component->IsEnabled() && component->GetOwner()->IsActive())
			{
				component->Update(deltaTime);
			}
		}
		myCollisionSystem->Update(*this, frame);
		ServiceLocator::GetInstance().GetPathFinder().Update();

		// Draw after gameplay and overlap callbacks have finished moving volumes.
		// Disabled colliders can still draw; inactive or destroyed objects cannot.
		for (Component* component : frame)
		{
			if (component->HasBegunPlay() && !component->IsPendingDestroy() && component->GetOwner()->IsActive())
			{
				if (auto* collider = dynamic_cast<ColliderComponent*>(component))
				{
					collider->DrawDebug();
				}
			}
		}
	}
	catch (...)
	{
		myUpdating = false;
		throw;
	}
	myUpdating = false;
}

void World::Clear()
{
	if (myUpdating)
	{
		throw std::logic_error("Cannot clear World from a component callback");
	}
	if (myClearing)
	{
		return;
	}
	myClearing = true;
	myCollisionSystem->Clear();
	myCamera = nullptr;
	std::vector<std::unique_ptr<Actor>> actors = std::move(myActors);
	actors.clear();
	myActorNameCount.clear();
	mySimulationTime = 0.0;
	myClearing = false;
}

RenderSnapshot& World::GenerateRenderSnapshot()
{
	myWorldSnapshot.Clear();
	myWorldSnapshot.TotalTime = static_cast<float>(mySimulationTime);

	myWorldSnapshot.ActiveCamera = myCamera;
	Frustum cameraFrustum;
	if (myCamera != nullptr)
	{
		cameraFrustum = myCamera->GetFrustum();
	}

	for (const std::unique_ptr<Actor>& actor : myActors)
	{
		if (!actor->IsActive())
		{
			continue;
		}

		for (const std::unique_ptr<Component>& component : actor->myComponents)
		{
			if (!component->HasBegunPlay() || !component->IsEnabled() || component->IsPendingDestroy())
			{
				continue;
			}

			MeshComponentBase* meshComponent = dynamic_cast<MeshComponentBase*>(component.get());
			if (meshComponent != nullptr)
			{
				myWorldSnapshot.AllMeshes.emplace_back(meshComponent);

				if (cameraFrustum.IsCulled(meshComponent->GetCullingSphere()))
				{
					continue;
				}

				const std::shared_ptr<Mesh>& mesh = meshComponent->GetMesh()->GetMesh();
				const std::vector<std::shared_ptr<MaterialAsset>>& materials = meshComponent->GetMaterialList();

				bool addedOpaque = false;
				bool addedBlended = false;

				const MaterialInterface* currentMaterial = nullptr;
				for (const Mesh::Element& element : mesh->GetElements())
				{
					const MaterialInterface* elementMaterial = materials[element.MaterialIndex] == nullptr ? nullptr : materials[element.MaterialIndex]->GetMaterial().get();
					if (elementMaterial != nullptr && elementMaterial != currentMaterial)
					{
						currentMaterial = elementMaterial;
					}
					else if (elementMaterial == nullptr)
					{
						currentMaterial = GraphicsEngine::Get().GetDefaultMaterial();
					}

					switch (currentMaterial->GetBlendMode())
					{
						case BlendMode::Opaque:
							if (!addedOpaque)
							{
								myWorldSnapshot.OpaqueMeshes.emplace_back(meshComponent);
								addedOpaque = true;
							}
							break;

						default:
							if (!addedBlended)
							{
								myWorldSnapshot.BlendedMeshes.emplace_back(meshComponent);
								addedBlended = true;
							}
							break;
					}
				}

				continue;
			}

			LightComponent* lightComponent = dynamic_cast<LightComponent*>(component.get());
			if (lightComponent != nullptr)
			{
				myWorldSnapshot.Lights.emplace_back(lightComponent);
				continue;
			}

			ParticleComponent* particleComponent = dynamic_cast<ParticleComponent*>(component.get());
			if (particleComponent != nullptr)
			{
				myWorldSnapshot.Particles.emplace_back(particleComponent);
				continue;
			}
		}
	}

	std::jthread sortAllMeshes([this]()
		{
			std::sort(myWorldSnapshot.AllMeshes.begin(), myWorldSnapshot.AllMeshes.end(), [this](const MeshComponentBase* aLeft, const MeshComponentBase* aRight)
				{
					const float distanceToLeft = (myCamera->GetOwner()->GetTransform().GetWorldPosition() - aLeft->GetOwner()->GetTransform().GetWorldPosition()).LengthSqr();
					const float distanceToRight = (myCamera->GetOwner()->GetTransform().GetWorldPosition() - aRight->GetOwner()->GetTransform().GetWorldPosition()).LengthSqr();
					return distanceToLeft < distanceToRight;
				});
		});

	std::jthread sortOpaqueMeshes([this]()
		{
			std::sort(myWorldSnapshot.OpaqueMeshes.begin(), myWorldSnapshot.OpaqueMeshes.end(), [this](const MeshComponentBase* aLeft, const MeshComponentBase* aRight)
				{
					const float distanceToLeft = (myCamera->GetOwner()->GetTransform().GetWorldPosition() - aLeft->GetOwner()->GetTransform().GetWorldPosition()).LengthSqr();
					const float distanceToRight = (myCamera->GetOwner()->GetTransform().GetWorldPosition() - aRight->GetOwner()->GetTransform().GetWorldPosition()).LengthSqr();
					return distanceToLeft < distanceToRight;
				});
		});

	std::jthread sortBlendedMeshes([this]()
		{
			std::sort(myWorldSnapshot.BlendedMeshes.begin(), myWorldSnapshot.BlendedMeshes.end(), [this](const MeshComponentBase* aLeft, const MeshComponentBase* aRight)
				{
					const float distanceToLeft = (myCamera->GetOwner()->GetTransform().GetWorldPosition() - aLeft->GetOwner()->GetTransform().GetWorldPosition()).LengthSqr();
					const float distanceToRight = (myCamera->GetOwner()->GetTransform().GetWorldPosition() - aRight->GetOwner()->GetTransform().GetWorldPosition()).LengthSqr();
					return distanceToLeft > distanceToRight;
				});
		});

	sortAllMeshes.join();
	sortOpaqueMeshes.join();
	sortBlendedMeshes.join();

	return myWorldSnapshot;
}
