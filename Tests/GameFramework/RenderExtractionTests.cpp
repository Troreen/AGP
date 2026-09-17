#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "GameFramework/World/World.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/StaticMeshComponent.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Runtime/Internal/WorldAccess.h"
#include "GraphicsEngine/GraphicsEngine.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
void Expect(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Close(float value,float expected,const char* message){Expect(std::abs(value-expected)<.001f,message);}
std::shared_ptr<Mesh> Triangle()
{
    auto mesh=std::make_shared<Mesh>();std::vector<Vertex> vertices(3);
    vertices[0].Position={-1,-1,0,1};vertices[1].Position={1,-1,0,1};vertices[2].Position={0,1,0,1};
    Mesh::Element element;element.NumVertices=3;element.NumIndices=3;
    mesh->Initialize("CPU extraction triangle",std::vector<Mesh::Element>{element},std::move(vertices),std::vector<unsigned>{0,1,2});return mesh;
}
}
int RunExtractionFixture()
{
    try
    {
        GraphicsEngine::RenderSceneSnapshot snapshot;
        std::weak_ptr<Mesh> retainedMesh;std::weak_ptr<MaterialInterface> retainedMaterial;
        {
            auto mesh=Triangle();auto material=std::make_shared<Material>();retainedMesh=mesh;retainedMaterial=material;
            World world;
            auto* cameraActor=world.SpawnActor("Camera");cameraActor->GetTransform().SetLocalPosition({0,0,-100});
            auto* camera=cameraActor->AddComponent<CameraComponent>();camera->SetPerspective(90,1,5000,{320,240});world.SetActiveCamera(camera);
            auto* actor=world.SpawnActor("Offset prop");actor->GetTransform().SetLocalPosition({10,20,30});actor->GetTransform().SetLocalScale({2,3,4});
            auto* pivot=actor->AddComponent<SceneComponent>("Pivot");pivot->GetTransform().SetLocalPosition({1,2,3});
            auto* first=actor->AddComponent<StaticMeshComponent>("First");first->SetMesh(mesh);first->SetMaterial(0,material);first->GetTransform().SetLocalPosition({4,5,6});first->SetParent(pivot,ReparentMode::KeepLocal);
            auto* second=actor->AddComponent<StaticMeshComponent>("Second");second->SetMesh(mesh);second->SetMaterial(0,material);second->GetTransform().SetLocalPosition({-3,0,0});
            auto skinMesh=Triangle();Skeleton skeleton;Skeleton::Joint root;root.Name="Root";skeleton.Joints.push_back(root);skeleton.JointNameToIndex["Root"]=0;skinMesh->SetSkeleton(std::move(skeleton));
            auto animation=std::make_shared<Animation>();animation->Name="Move";animation->FramesPerSecond=2;animation->Duration=1;animation->Frames.resize(2);
            animation->Frames[0].Transforms["Root"]=CU::Matrix4f();auto pose=CU::Matrix4f();pose(4,1)=8;animation->Frames[1].Transforms["Root"]=pose;skinMesh->AddAnimation(animation);
            auto* skinned=actor->AddComponent<SkeletalMeshComponent>("Skinned");skinned->SetMesh(skinMesh);skinned->SetMaterial(0,material);Expect(skinned->PlayAnimation("Move",true),"CPU skeletal fixture animation rejected");
            auto* light=actor->AddComponent<PointLightComponent>("Light");light->SetParent(pivot,ReparentMode::KeepLocal);light->GetTransform().SetLocalPosition({0,1,0});light->SetColor({.2f,.3f,.4f});light->SetIntensity(5);light->SetRadius(200);
            SceneDiagnostics diagnostics;Expect(GameFrameworkInternal::WorldAccess::Prepare(world,diagnostics),"CPU extraction world failed validation");GameFrameworkInternal::WorldAccess::Activate(world);GameFrameworkInternal::WorldAccess::Update(world,.5f);
            // Typed lookup includes these immediately, extraction must wait for start.
            auto* pending=actor->AddComponent<StaticMeshComponent>("Pending");pending->SetMesh(mesh);pending->SetMaterial(0,material);
            actor->AddComponent<PointLightComponent>("Pending light");
            Expect(GraphicsEngine::Get().BuildRenderSnapshot(*camera,world,snapshot),"CPU extraction failed without a device");
            Expect(snapshot.HasCamera && snapshot.ShadowCasters.size()==3 && snapshot.RelevantLights.size()==1,"Extraction included unstarted objects or lost fixture objects");
            Expect(snapshot.Stats.TotalRenderItems==3 && snapshot.Stats.VisibleRenderItems==3 && snapshot.Stats.TotalLights==1,"Snapshot totals differ from fixture");
            Expect(snapshot.OpaqueRenderItems==std::vector<size_t>({1,2,0}) && snapshot.BlendedRenderItems.empty(),"Opaque routing/sort changed");
            const auto& a=snapshot.ShadowCasters[0];const auto& b=snapshot.ShadowCasters[1];const auto& skin=snapshot.ShadowCasters[2];
            Close(a.World(4,1),20,"First offset X changed");Close(a.World(4,2),41,"First offset Y changed");Close(a.World(4,3),66,"First offset Z changed");
            Close(b.World(4,1),4,"Second offset X changed");Close(b.World(4,2),20,"Second offset Y changed");Close(b.World(4,3),30,"Second offset Z changed");
            Expect(a.Mesh==mesh && a.Materials.size()==1 && a.Materials[0]==material && a.HasBounds,"Snapshot resource/bounds identity changed");
            Close(a.BoundsCenter.x,20,"Bounds did not follow component offset");Close(a.BoundsRadius,std::sqrt(2.0f)*4,"Nonuniform bounds scale changed");
            Expect(skin.HasSkinning && !skin.HasBounds,"Skeletal culling eligibility changed");Close(skin.JointTransforms[0](4,1),8,"Snapshot lost current skeletal pose");
            const auto& copiedLight=snapshot.RelevantLights[0];Close(copiedLight.Position.x,12,"Light X attachment changed");Close(copiedLight.Position.y,29,"Light Y attachment changed");Close(copiedLight.Position.z,42,"Light Z attachment changed");
            Close(copiedLight.Intensity,5,"Light intensity changed");Close(copiedLight.Radius,200,"Light radius inherited scale");Close(copiedLight.Color.y,.3f,"Light color changed");
            Close(snapshot.Camera.GetTransform().GetPosition().z,-100,"Camera copy lost world position");
            actor->Destroy();Expect(snapshot.ShadowCasters[0].Mesh==mesh,"Logical death altered a copied snapshot");
        }
        Expect(!retainedMesh.expired() && !retainedMaterial.expired(),"World destruction released resources retained by snapshot");
        snapshot.Clear();Expect(retainedMesh.expired() && retainedMaterial.expired(),"Cleared snapshot retained resource ownership");
        std::cout<<"PASS: no-device extraction offsets, bounds, opaque order, skin pose, camera/light copies and resource retention\n";return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
