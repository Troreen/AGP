#pragma once 

#include "InputHandler.h"
#include "Vector.hpp"


struct ApplicationStateData
{
    CU::InputHandler InputHandler;
    float CameraSpeed = 1000.0f;
    float CameraRotationSpeed = 0.1f;

    CU::Vector3f InputVector = CU::Vector3f::Zero;
    CU::Vector3f InputRotation = CU::Vector3f::Zero;
    bool IsMouseTrapped = false;
    bool IsMouseMovementMode = false;
};

inline ApplicationStateData ApplicationState;