#pragma once

#include "Vector.h"
#include "Pad.h"

struct ViewSetup {
public:
    int			iX;
    int			iUnscaledX;
    int			iY;
    int			iUnscaledY;
    int			iWidth;
    int			iUnscaledWidth;
    int			iHeight;
    int			iUnscaledHeight;
    bool		bOrtho;
    std::byte	pad0[0x8F];
    float		fov;
    float		flViewModelFOV;
    Vector		origin;
    Vector		angles;
    float		nearZ;
    float		farZ;
    float		flNearViewmodelZ;
    float		flFarViewmodelZ;
    float		flAspectRatio;
    float		flNearBlurDepth;
    float		flNearFocusDepth;
    float		flFarFocusDepth;
    float		flFarBlurDepth;
    float		flNearBlurRadius;
    float		flFarBlurRadius;
    float		flDoFQuality;
    int			nMotionBlurMode;
    float		flShutterTime;
    Vector		vecShutterOpenPosition;
    Vector		vecShutterOpenAngles;
    Vector		vecShutterClosePosition;
    Vector		vecShutterCloseAngles;
    float		flOffCenterTop;
    float		flOffCenterBottom;
    float		flOffCenterLeft;
    float		flOffCenterRight;
    bool		bOffCenter;
    bool		bRenderToSubrectOfLargerScreen;
    bool		bDoBloomAndToneMapping;
    bool		bDoDepthOfField;
    bool		bHDRTarget;
    bool		bDrawWorldNormal;
    bool		bCullFontFaces;
    bool		bCacheFullSceneState;
    bool		bCSMView;
};
