#pragma once

#include <cstddef>

#include "../Interfaces.h"

#include "Cvar.h"
#include "ConVar.h"
#include "Vector.h"
#include "VirtualMethod.h"
#include "WeaponData.h"
#pragma region MASKS

#define   DISPSURF_FLAG_SURFACE           (1<<0)
#define   DISPSURF_FLAG_WALKABLE          (1<<1)
#define   DISPSURF_FLAG_BUILDABLE         (1<<2)
#define   DISPSURF_FLAG_SURFPROP1         (1<<3)
#define   DISPSURF_FLAG_SURFPROP2         (1<<4)

#define   CONTENTS_EMPTY                0

#define   CONTENTS_SOLID                0x1       
#define   CONTENTS_WINDOW               0x2
#define   CONTENTS_AUX                  0x4
#define   CONTENTS_GRATE                0x8
#define   CONTENTS_SLIME                0x10
#define   CONTENTS_WATER                0x20
#define   CONTENTS_BLOCKLOS             0x40 
#define   CONTENTS_OPAQUE               0x80 
#define   LAST_VISIBLE_CONTENTS         CONTENTS_OPAQUE

#define   ALL_VISIBLE_CONTENTS            (LAST_VISIBLE_CONTENTS | (LAST_VISIBLE_CONTENTS-1))

#define   CONTENTS_TESTFOGVOLUME        0x100
#define   CONTENTS_UNUSED               0x200     
#define   CONTENTS_BLOCKLIGHT           0x400
#define   CONTENTS_TEAM1                0x800 
#define   CONTENTS_TEAM2                0x1000 
#define   CONTENTS_IGNORE_NODRAW_OPAQUE 0x2000
#define   CONTENTS_MOVEABLE             0x4000
#define   CONTENTS_AREAPORTAL           0x8000
#define   CONTENTS_PLAYERCLIP           0x10000
#define   CONTENTS_MONSTERCLIP          0x20000
#define   CONTENTS_CURRENT_0            0x40000
#define   CONTENTS_CURRENT_90           0x80000
#define   CONTENTS_CURRENT_180          0x100000
#define   CONTENTS_CURRENT_270          0x200000
#define   CONTENTS_CURRENT_UP           0x400000
#define   CONTENTS_CURRENT_DOWN         0x800000

#define   CONTENTS_ORIGIN               0x1000000 

#define   CONTENTS_MONSTER              0x2000000 
#define   CONTENTS_DEBRIS               0x4000000
#define   CONTENTS_DETAIL               0x8000000 
#define   CONTENTS_TRANSLUCENT          0x10000000
#define   CONTENTS_LADDER               0x20000000
#define   CONTENTS_HITBOX               0x40000000

#define   SURF_LIGHT                    0x0001 
#define   SURF_SKY2D                    0x0002 
#define   SURF_SKY                      0x0004 
#define   SURF_WARP                     0x0008 
#define   SURF_TRANS                    0x0010
#define   SURF_NOPORTAL                 0x0020 
#define   SURF_TRIGGER                  0x0040 
#define   SURF_NODRAW                   0x0080 

#define   SURF_HINT                     0x0100 

#define   SURF_SKIP                     0x0200   
#define   SURF_NOLIGHT                  0x0400   
#define   SURF_BUMPLIGHT                0x0800   
#define   SURF_NOSHADOWS                0x1000   
#define   SURF_NODECALS                 0x2000   
#define   SURF_NOPAINT                  SURF_NODECALS
#define   SURF_NOCHOP                   0x4000   
#define   SURF_HITBOX                   0x8000   

// -----------------------------------------------------
// spatial content masks - used for spatial queries (traceline,etc.)
// -----------------------------------------------------
#define   MASK_ALL                      (0xFFFFFFFF)
#define   MASK_SOLID                    (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
#define   MASK_PLAYERSOLID              (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
#define   MASK_NPCSOLID                 (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
#define   MASK_NPCFLUID                 (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define   MASK_WATER                    (CONTENTS_WATER|CONTENTS_MOVEABLE|CONTENTS_SLIME)
#define   MASK_OPAQUE                   (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_OPAQUE)
#define   MASK_OPAQUE_AND_NPCS          (MASK_OPAQUE|CONTENTS_MONSTER)
#define   MASK_BLOCKLOS                 (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_BLOCKLOS)
#define   MASK_BLOCKLOS_AND_NPCS        (MASK_BLOCKLOS|CONTENTS_MONSTER)
#define   MASK_VISIBLE                  (MASK_OPAQUE|CONTENTS_IGNORE_NODRAW_OPAQUE)
#define   MASK_VISIBLE_AND_NPCS         (MASK_OPAQUE_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE)
#define   MASK_SHOT                     (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_HITBOX)
#define   MASK_SHOT_BRUSHONLY           (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_DEBRIS)
#define   MASK_SHOT_HULL                (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_GRATE)
#define   MASK_SHOT_PORTAL              (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define   MASK_SOLID_BRUSHONLY          (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_GRATE)
#define   MASK_PLAYERSOLID_BRUSHONLY    (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_PLAYERCLIP|CONTENTS_GRATE)
#define   MASK_NPCSOLID_BRUSHONLY       (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)
#define   MASK_NPCWORLDSTATIC           (CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)
#define   MASK_NPCWORLDSTATIC_FLUID     (CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP)
#define   MASK_SPLITAREAPORTAL          (CONTENTS_WATER|CONTENTS_SLIME)
#define   MASK_CURRENT                  (CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)
#define   MASK_DEADSOLID                (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_GRATE)
#pragma endregion
class matrix3x4;

struct Ray {
    Ray(const Vector& src, const Vector& dest)
        : start(src), delta(dest - src), extents(Vector{ }), startOffset(Vector{ }), worldAxisTransform(NULL), isRay(true) { isSwept = delta.x || delta.y || delta.z; }

    Ray(const Vector& src, const Vector& dest, const Vector& mins, const Vector& maxs)
        : delta(dest - src), extents(maxs - mins), startOffset(maxs + mins), worldAxisTransform(NULL)
    {
        isSwept = delta.x || delta.y || delta.z;
        extents *= 0.5f;
        isRay = (extents.squareLength() < 1e-6);

        startOffset *= 0.5f;
        start = src + startOffset;
        startOffset *= -1.0f;
    }
    Vector start{ };
    float pad{ };
    Vector delta{ };
    float pad1{ };
    Vector startOffset{ };
    float pad2{ };
    Vector extents{ };
    float pad3{ };
    const matrix3x4* worldAxisTransform;
    bool isRay{ };
    bool isSwept{ };
};

class Entity;

struct TraceFilter {
    TraceFilter(const Entity* entity) : skip{ entity } { }
    virtual bool shouldHitEntity(Entity* entity, int) { return entity != skip; }
    virtual int getTraceType() const { return 0; }
    const void* skip;
};



namespace HitGroup {
    enum {
        Invalid = -1,
        Generic,
        Head,
        Chest,
        Stomach,
        LeftArm,
        RightArm,
        LeftLeg,
        RightLeg,
        Gear = 10
    };

    float getDamageMultiplier(int hitGroup, const WeaponInfo* weaponData, bool hasHeavyArmor, int teamNumber) noexcept;
    bool isArmored(int hitGroup, bool helmet, int armorValue, bool hasHeavyArmor) noexcept;
}

enum class TraceType
{
    TRACE_EVERYTHING = 0,
    TRACE_WORLD_ONLY,
    TRACE_ENTITIES_ONLY,
    TRACE_EVERYTHING_FILTER_PROPS,
};

class TraceFilter_t {
public:
    virtual bool shouldHitEntity(Entity* entity, int) = 0;
    virtual TraceType getTraceType() const = 0;
};

class TraceFilterWorldOnly : public TraceFilter_t {
public:
    virtual bool shouldHitEntity(Entity* entity, int) { return entity != skip; }
    virtual TraceType getTraceType() const
    { 
        return TraceType::TRACE_WORLD_ONLY; 
    }
    const void* skip;
};

class TraceFilterWorldAndPropsOnly : public TraceFilter_t {
public:
    virtual bool shouldHitEntity(Entity*, int) { return false; }
    virtual TraceType getTraceType() const
    {
        return TraceType::TRACE_EVERYTHING;
    }
};

class TraceFilterEntitiesOnly : public TraceFilter_t {
public:
    virtual bool shouldHitEntity(Entity* entity, int) { return entity != skip; }
    virtual TraceType getTraceType() const
    {
        return TraceType::TRACE_ENTITIES_ONLY;
    }
    const void* skip;
};

class TraceFilterEverythingFilterProps : public TraceFilter_t {
public:
    virtual bool shouldHitEntity(Entity* entity, int) { return entity != skip; }
    virtual TraceType getTraceType() const
    {
        return TraceType::TRACE_EVERYTHING_FILTER_PROPS;
    }
    const void* skip;
};

struct Trace {
    Vector startpos;
    Vector endpos;
    struct Plane {
        Vector normal{ };
        float dist{ };
        std::byte type{ };
        std::byte signBits{ };
        std::byte pad[2]{ };
    } plane;
    float fraction;
    int contents;
    unsigned short dispFlags;
    bool allSolid;
    bool startSolid;
    std::byte pad1[4];
    struct Surface {
        const char* name;
        short surfaceProps;
        unsigned short flags;
    } surface;
    int hitgroup;
    std::byte pad2[4];
    Entity* entity;
    int hitbox;

    bool didHit() { return (fraction < 1.0f || allSolid || startSolid); }
};

// #define TRACE_STATS // - enable to see how many rays are cast per frame

#ifdef TRACE_STATS
#include "../Memory.h"
#include "GlobalVars.h"
#endif

class EngineTrace {
public:
    VIRTUAL_METHOD(int, getPointContents, 0, (const Vector& absPosition, int contentsMask), (this, std::cref(absPosition), contentsMask, nullptr))
    VIRTUAL_METHOD(void, _traceRay, 5, (const Ray& ray, unsigned int mask, const TraceFilter& filter, Trace& trace), (this, std::cref(ray), mask, std::cref(filter), std::ref(trace)))
    VIRTUAL_METHOD(void, _traceRay1, 5, (const Ray& ray, unsigned int mask, const TraceFilter_t& filter, Trace& trace), (this, std::cref(ray), mask, std::cref(filter), std::ref(trace)))
        void traceRay1(const Ray& ray, unsigned int mask, const TraceFilter_t& filter, Trace& trace) noexcept
    {
#ifdef TRACE_STATS
        static int tracesThisFrame, lastFrame;

        if (lastFrame != memory->globalVars->framecount) {
            memory->debugMsg("traces: frame - %d | count - %d\n", lastFrame, tracesThisFrame);
            tracesThisFrame = 0;
            lastFrame = memory->globalVars->framecount;
        }

        ++tracesThisFrame;
#endif
        _traceRay1(ray, mask, filter, trace);
    }
    void traceRay(const Ray& ray, unsigned int mask, const TraceFilter& filter, Trace& trace) noexcept
    {
#ifdef TRACE_STATS
        static int tracesThisFrame, lastFrame;

        if (lastFrame != memory->globalVars->framecount) {
            memory->debugMsg("traces: frame - %d | count - %d\n", lastFrame, tracesThisFrame);
            tracesThisFrame = 0;
            lastFrame = memory->globalVars->framecount;
        }

        ++tracesThisFrame;
#endif
        _traceRay(ray, mask, filter, trace);
    }
};
