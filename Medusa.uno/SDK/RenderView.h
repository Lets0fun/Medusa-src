#pragma once

#include "VirtualMethod.h"
#include "matrix3x4.h"
#include "ViewSetup.h"

enum ClearFlags_t
{
    VIEW_CLEAR_COLOR = 0x1,
    VIEW_CLEAR_DEPTH = 0x2,
    VIEW_CLEAR_FULL_TARGET = 0x4,
    VIEW_NO_DRAW = 0x8,
    VIEW_CLEAR_OBEY_STENCIL = 0x10, // Draws a quad allowing stencil test to clear through portals
    VIEW_CLEAR_STENCIL = 0x20,
};

class RenderView {
public:
    VIRTUAL_METHOD(void, setBlend, 4, (float alpha), (this, alpha))
    VIRTUAL_METHOD(void, setColorModulation, 6, (const float* colors), (this, colors))

    void setColorModulation(float r, float g, float b) noexcept
    {
        float color[3]{ r, g, b };
        setColorModulation(color);
    }
    VIRTUAL_METHOD(void, GetMatricesForView, 60, (const ViewSetup& view, Matrix4x4* pWorldToView, Matrix4x4* pViewToProjection, Matrix4x4* pWorldToProjection, Matrix4x4* pWorldToPixels), (this, view, pWorldToView, pViewToProjection, pWorldToProjection, pWorldToPixels))
    //virtual void      GetMatricesForView(const ViewSetup& view, Matrix4x4* pWorldToView, Matrix4x4* pViewToProjection, Matrix4x4* pWorldToProjection, Matrix4x4* pWorldToPixels) = 0;
};
