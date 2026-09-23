#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "InputHandler.h"
#include "InputMapper.h"
#include "GameFramework/ServiceLocator.h"

// Native messages exercise the same InputHandler -> InputMapper path as the host.
struct InputFixture
{
    CommonUtilities::InputHandler Handler;
    CommonUtilities::InputMapper& Input;
    int MouseX = 16000, MouseY = 16000;

    InputFixture() : Input(*ServiceLocator::GetInstance().SetInputMapper(new CommonUtilities::InputMapper))
    {
        Input.Init(&Handler);
        Handler.UpdateEvents(WM_MOUSEMOVE, 0, MAKELPARAM(MouseX, MouseY));
        Input.Update();
    }
    ~InputFixture() { ServiceLocator::GetInstance().KillServices(); }
    void Key(EKeyCode key, bool down)
    {
        UINT message = down ? WM_KEYDOWN : WM_KEYUP;
        if (key == EKeyCode::MOUSERBUTTON) message = down ? WM_RBUTTONDOWN : WM_RBUTTONUP;
        Handler.UpdateEvents(message, static_cast<WPARAM>(key), 0);
    }
    void Move(int dx, int dy)
    {
        MouseX += dx; MouseY += dy;
        Handler.UpdateEvents(WM_MOUSEMOVE, 0, MAKELPARAM(MouseX, MouseY));
    }
};
