#pragma once
#include <Vector2.hpp>
#include <Vector3.hpp>
#include <functional>
#include <vector>

using namespace CommonUtilities;

struct NavMeshQueryResult
{
	Vector2f nearestPosition;
	uint32_t nodeIndex;
	bool isValidPoint;
};

class Pathfinder
{
	public:
		Pathfinder();
		~Pathfinder();

		void QueueRequest(const Vector3f& aStartPos, const Vector3f& aTargetPos, std::function<void(std::vector<Vector3f>& outWaypoints)> aResultHandler);
		void ImmediateRequest(const Vector3<float>& aStartPos, const Vector3<float>& aTargetPos, std::vector<Vector3<float>>& outWaypoints) const;

	private:
		void BuildNavMesh();
};

