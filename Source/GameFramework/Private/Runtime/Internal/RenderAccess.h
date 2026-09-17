#pragma once
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/MeshComponentBase.h"

namespace GameFrameworkInternal
{
	// Copies/borrows backend data only at the engine's serialized extraction point.
	class RenderAccess
	{
	public:
		static CommonUtilities::Camera3D Camera(CameraComponent& camera)
		{
			camera.SyncCameraToOwner();
			return camera.myCamera;
		}

		static std::shared_ptr<::Mesh> Mesh(const MeshComponentBase& mesh)
		{
			return mesh.myMesh;
		}

		static const std::vector<std::shared_ptr<::MaterialInterface>>& Materials(const MeshComponentBase& mesh)
		{
			return mesh.myMaterials;
		}

		static bool HasSkinning(const MeshComponentBase& mesh)
		{
			return mesh.HasSkinning();
		}

		static const std::array<CU::Matrix4f, 128>* JointTransforms(const MeshComponentBase& mesh)
		{
			return mesh.GetJointTransforms();
		}
	};
}
