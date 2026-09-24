#pragma once
#include <filesystem>
#include <string>

#include "TgaFbxStructs.h"

namespace TGA
{
	namespace FBX
	{
		class Importer
		{
		public:

			/**
			* Initializes the Importer (or re-initializes if it has been uninitialized) to make it ready for use.
			*/
			static void InitImporter();

			/**
			 * Tell the Importer to clean up. Intended to be used to save memory when the Importer is no longer needed.
			 */
			static void UninitImporter();

			/**
			 * Attempts to load a FBX model into the provided Mesh structure. Supports skeletal meshes, multi-element meshes
			 * and multiple materials. Will treat multiple meshes in the file as belonging to the same Model.
			 * @param aFilePath The path of the FBX file to load, in UTF-8 format.
			 * @param outMesh The model data read from the file.
			 * @param aRegenerateNormals If True existing normal data will be discarded and completely regenerated from smoothing groups.
			 * @returns True if the model was successfully loaded, otherwise false.
			 */
			static FbxImportStatus LoadMesh(const std::filesystem::path& aFilePath, Mesh& outMesh, bool aRegenerateNormals = false, bool aMergeDuplicateVertices = true);

			/**
			 * Attempts to load a FBX animation into the provided Animation structure. Does not require a mesh present in the
			 * file to function.
			 * @param aFilePath The path of the FBX file to load, in UTF-8 format.
			 * @param outAnimation The animation data read from the file.
			 * @returns True if the animation was successfully loaded, otherwise false.
			 */
			static FbxImportStatus LoadAnimation(const std::filesystem::path& aFilePath, Animation& outAnimation);

			/**
			 * Attempts to load an FBX file exported by the TGA Unreal Plugin navmesh exporter. May work with other exports as well.
			 * Will ignore all data except the control points and indices in the mesh.
			 * @param aFilePath The path of the FBX file to load, in UTF-8 format.
			 * @param outNavMesh The NavMesh data read from the file.
			 * @param aShouldTriangulate If import should triangulate the NavMesh, this will result in more polygons.
			 */
			static FbxImportStatus LoadNavMesh(const std::filesystem::path& aFilePath, NavMesh& outNavMesh, bool aShouldTriangulate = false);

			static std::string_view GetLastSDKError();
		};

	}
}
