#pragma once
#include "Transform.hpp"
#include <DirectXMath.h>
#include <cmath>

enum class ReparentMode { KeepLocal, KeepWorld };
namespace GameFrameworkInternal
{
    inline DirectX::XMMATRIX ToDirectX(const CommonUtilities::Matrix4f& matrix)
    {
        DirectX::XMFLOAT4X4 data;
        for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c) data.m[r][c] = matrix(r + 1, c + 1);
        return DirectX::XMLoadFloat4x4(&data);
    }
    inline CommonUtilities::Matrix4f FromDirectX(DirectX::FXMMATRIX matrix)
    {
        DirectX::XMFLOAT4X4 data; DirectX::XMStoreFloat4x4(&data, matrix);
        CommonUtilities::Matrix4f result;
        for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c) result(r + 1, c + 1) = data.m[r][c];
        return result;
    }
    // Reject shear and singular transforms instead of silently losing information.
    inline bool SetLocalMatrix(CommonUtilities::Transform& target, const CommonUtilities::Matrix4f& matrix)
    {
        DirectX::XMVECTOR scale, rotation, position;
        if (!DirectX::XMMatrixDecompose(&scale, &rotation, &position, ToDirectX(matrix))) return false;
        DirectX::XMFLOAT3 s, p; DirectX::XMFLOAT4 q;
        DirectX::XMStoreFloat3(&s, scale); DirectX::XMStoreFloat3(&p, position); DirectX::XMStoreFloat4(&q, rotation);
        CommonUtilities::Transform candidate({p.x,p.y,p.z}, {q.w,q.x,q.y,q.z}, {s.x,s.y,s.z});
        auto rebuilt = candidate.GetLocalMatrix();
        for (int r = 1; r <= 4; ++r) for (int c = 1; c <= 4; ++c)
            if (!std::isfinite(matrix(r,c)) || std::abs(rebuilt(r,c)-matrix(r,c)) > 0.0001f * (1.f + std::abs(matrix(r,c)))) return false;
        target.SetPosition(candidate.GetPosition()); target.SetRotation(candidate.GetRotation()); target.SetScale(candidate.GetScale());
        return true;
    }
    inline bool SetWorldMatrix(CommonUtilities::Transform& local, const CommonUtilities::Matrix4f& matrix)
    {
        auto world = ToDirectX(matrix);
        if (auto* parent = local.GetParent())
        {
            auto parentWorld = ToDirectX(parent->GetWorldMatrix());
            const float determinant = DirectX::XMVectorGetX(DirectX::XMMatrixDeterminant(parentWorld));
            if (!std::isfinite(determinant) || std::abs(determinant) < 1e-8f) return false;
            world = DirectX::XMMatrixMultiply(world, DirectX::XMMatrixInverse(nullptr,parentWorld));
        }
        return SetLocalMatrix(local,FromDirectX(world));
    }
    inline bool ChangeParent(CommonUtilities::Transform& local, CommonUtilities::Transform* parent, ReparentMode mode)
    {
        if (mode == ReparentMode::KeepWorld)
        {
            auto world = ToDirectX(local.GetWorldMatrix());
            if (parent)
            {
                auto parentWorld = ToDirectX(parent->GetWorldMatrix());
                const float determinant = DirectX::XMVectorGetX(DirectX::XMMatrixDeterminant(parentWorld));
                if (!std::isfinite(determinant) || std::abs(determinant) < 1e-8f) return false;
                world = DirectX::XMMatrixMultiply(world, DirectX::XMMatrixInverse(nullptr, parentWorld));
            }
            if (!SetLocalMatrix(local, FromDirectX(world))) return false;
        }
        local.SetParent(parent); return true;
    }
}
