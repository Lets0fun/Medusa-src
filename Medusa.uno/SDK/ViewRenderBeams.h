/*#pragma once

#include "Vector.h"
#include "VirtualMethod.h"

class Beam_t;
class Entity;
enum
{
	TE_BEAMPOINTS = 0x00, // beam effect between two points
	TE_SPRITE = 0x01, // additive sprite, plays 1 cycle
	TE_BEAMDISK = 0x02, // disk that expands to max radius over lifetime
	TE_BEAMCYLINDER = 0x03, // cylinder that expands to max radius over lifetime
	TE_BEAMFOLLOW = 0x04, // create a line of decaying beam segments until entity stops moving
	TE_BEAMRING = 0x05, // connect a beam ring to two entities
	TE_BEAMSPLINE = 0x06,
	TE_BEAMRINGPOINT = 0x07,
	TE_BEAMLASER = 0x08, // Fades according to viewpoint
	TE_BEAMTESLA = 0x09,
};
struct BeamInfo_t
{
	int m_nType;
	Entity* m_pStartEnt;
	int m_nStartAttachment;
	Entity* m_pEndEnt;
	int m_nEndAttachment;
	Vector m_vecStart;
	Vector m_vecEnd;
	int m_nModelIndex;
	const char* m_pszModelName;
	int m_nHaloIndex;
	const char* m_pszHaloName;
	float m_flHaloScale;
	float m_flLife;
	float m_flWidth;
	float m_flEndWidth;
	float m_flFadeLength;
	float m_flAmplitude;
	float m_flBrightness;
	float m_flSpeed;
	int m_nStartFrame;
	float m_flFrameRate;
	float m_flRed;
	float m_flGreen;
	float m_flBlue;
	bool m_bRenderable;
	int m_nSegments;
	int m_nFlags;
	Vector m_vecCenter;
	float m_flStartRadius;
	float m_flEndRadius;

	BeamInfo_t()
	{
		m_nType = TE_BEAMPOINTS;
		m_nSegments = -1;
		m_pszModelName = NULL;
		m_pszHaloName = NULL;
		m_nModelIndex = -1;
		m_nHaloIndex = -1;
		m_bRenderable = true;
		m_nFlags = 0;
	}
};

struct BeamInfo {
    int	type;
    Entity* startEnt;
    int startAttachment;
    Entity* endEnt;
    int	endAttachment;
    Vector start;
    Vector end;
    int modelIndex;
    const char* modelName;
    int haloIndex;
    const char* haloName;
    float haloScale;
    float life;
    float width;
    float endWidth;
    float fadeLength;
    float amplitude;
    float brightness;
    float speed;
    int	startFrame;
    float frameRate;
    float red;
    float green;
    float blue;
    bool renderable;
    int segments;
    int	flags;
    Vector ringCenter;
    float ringStartRadius;
    float ringEndRadius;
};

struct Beam {
    PAD(52)
    int flags;
    PAD(144)
    float die;
};

class ViewRenderBeams {
public:
	virtual void DrawBeam(Beam_t* pbeam) = 0;
	virtual Beam_t* CreateBeamCirclePoints(BeamInfo_t& beamInfo) = 0;
	virtual Beam_t* CreateBeamRingPoint(BeamInfo_t& beamInfo) = 0;
    VIRTUAL_METHOD(Beam*, createBeamPoints, 12, (BeamInfo& beamInfo), (this, std::ref(beamInfo)))
};*/
#pragma once

#include "Vector.h"
#include "VirtualMethod.h"
#include "EngineTrace.h"

class Entity;

class C_Beam;
class Beam_t;

#pragma region beams
enum BeamEffects
{
    TE_BEAMPOINTS = 0x00,		// beam effect between two points
    TE_SPRITE = 0x01,	// additive sprite, plays 1 cycle
    TE_BEAMDISK = 0x02,	// disk that expands to max radius over lifetime
    TE_BEAMCYLINDER = 0x03,		// cylinder that expands to max radius over lifetime
    TE_BEAMFOLLOW = 0x04,		// create a line of decaying beam segments until entity stops moving
    TE_BEAMRING = 0x05,		// connect a beam ring to two entities
    TE_BEAMSPLINE = 0x06,
    TE_BEAMRINGPOINT = 0x07,
    TE_BEAMLASER = 0x08,		// Fades according to viewpoint
    TE_BEAMTESLA = 0x09,
};


enum BeamTypes
{
    FBEAM_STARTENTITY = 0x00000001,
    FBEAM_ENDENTITY = 0x00000002,
    FBEAM_FADEIN = 0x00000004,
    FBEAM_FADEOUT = 0x00000008,
    FBEAM_SINENOISE = 0x00000010,
    FBEAM_SOLID = 0x00000020,
    FBEAM_SHADEIN = 0x00000040,
    FBEAM_SHADEOUT = 0x00000080,
    FBEAM_ONLYNOISEONCE = 0x00000100,		// Only calculate our noise once
    FBEAM_NOTILE = 0x00000200,
    FBEAM_USE_HITBOXES = 0x00000400,		// Attachment indices represent hitbox indices instead when this is set.
    FBEAM_STARTVISIBLE = 0x00000800,		// Has this client actually seen this beam's start entity yet?
    FBEAM_ENDVISIBLE = 0x00001000,		// Has this client actually seen this beam's end entity yet?
    FBEAM_ISACTIVE = 0x00002000,
    FBEAM_FOREVER = 0x00004000,
    FBEAM_HALOBEAM = 0x00008000,		// When drawing a beam with a halo, don't ignore the segments and endwidth
    FBEAM_REVERSED = 0x00010000,
    NUM_BEAM_FLAGS = 17	// KEEP THIS UPDATED!
};
#pragma endregion

struct BeamTrail_t
{
    // NOTE: Don't add user defined fields except after these four fields.
    BeamTrail_t* next;
    float die;
    Vector org;
    Vector vel;
};

struct BeamInfo {
    int	type;
    Entity* startEnt;
    int startAttachment;
    Entity* endEnt;
    int	endAttachment;
    Vector start;
    Vector end;
    int modelIndex;
    const char* modelName;
    int haloIndex;
    const char* haloName;
    float haloScale;
    float life;
    float width;
    float endWidth;
    float fadeLength;
    float amplitude;
    float brightness;
    float speed;
    int	startFrame;
    float frameRate;
    float red;
    float green;
    float blue;
    bool renderable;
    int segments;
    int	flags;
    Vector ringCenter;
    float ringStartRadius;
    float ringEndRadius;
};

struct Beam {
    PAD(52)
        int flags;
    PAD(144)
        float die;
};

class ViewRenderBeams {
public:
    VIRTUAL_METHOD(void, drawBeam, 6, (Beam* beam), (this, std::ref(beam)))

        VIRTUAL_METHOD(Beam*, createBeamPoints, 12, (BeamInfo& beamInfo), (this, std::ref(beamInfo)))
        VIRTUAL_METHOD(Beam*, createBeamRing, 14, (BeamInfo& beamInfo), (this, std::ref(beamInfo)))
        VIRTUAL_METHOD(Beam*, createBeamRingPoints, 16, (BeamInfo& beamInfo), (this, std::ref(beamInfo)))
};
