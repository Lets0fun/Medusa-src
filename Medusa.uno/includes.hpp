#pragma once
#define DIRECTINPUT_VERSION 0x0800
#define NOMINMAX
#include <Windows.h>
#include <WinInet.h>
#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <psapi.h>
#include <sstream>
#include <deque>
#include <array>
#include <ctime>

#include "xor.h"
#include <d3d9.h>
#include "color.h"
#include "Config.h"
#include "ConfigStructs.h"
#include "interfaces.h"
#include "SDK/Surface.h"
#include "SDK/Engine.h"
#include "SDK/Client.h"
#include "SDK/ClientState.h"
#include <dinput.h>
#include <tchar.h>
#include "Memory.h"
#include "SDK/GlobalVars.h"
#include "SDK/Entity.h"
#include "Hacks/AntiAim.h"
#include "Hacks/Animations.h"
#include "Hacks/Misc.h"
#include "Hacks/EnginePrediction.h"
#include "Vector2D.hpp"
#include "Vertex.hpp"

#define NUM_ENT_ENTRY_BITS         (11 + 2)
#define NUM_ENT_ENTRIES            (1 << NUM_ENT_ENTRY_BITS)
#define INVALID_EHANDLE_INDEX       0xFFFFFFFF
#define NUM_SERIAL_NUM_BITS        16 // (32 - NUM_ENT_ENTRY_BITS)
#define NUM_SERIAL_NUM_SHIFT_BITS (32 - NUM_SERIAL_NUM_BITS)
#define ENT_ENTRY_MASK             (( 1 << NUM_SERIAL_NUM_BITS) - 1)
#define TIME_TO_TICKS(t) ((int)(0.5f + (float)(t) / memory->globalVars->intervalPerTick))
#define TICKS_TO_TIME(t) (memory->globalVars->intervalPerTick * (t))
#pragma once
std::vector <int> indexes;
template <typename FuncType>
__forceinline static FuncType call_virtual(void* ppClass, int index)
{
	int* pVTable = *(int**)ppClass; //-V206
	int dwAddress = pVTable[index]; //-V108
	return (FuncType)(dwAddress);
}
class IHandleEntity;

class CBaseHandle { //-V690
public:
	CBaseHandle();
	CBaseHandle(const CBaseHandle& other);
	CBaseHandle(unsigned long value);
	CBaseHandle(int iEntry, int iSerialNumber);

	void Init(int iEntry, int iSerialNumber);
	void Term();

	// Even if this returns true, Get() still can return return a non-null value.
	// This just tells if the handle has been initted with any values.
	bool IsValid() const;

	int GetEntryIndex() const;
	int GetSerialNumber() const;

	unsigned long ToLong() const;

	int ToInt() const;
	bool operator !=(const CBaseHandle& other) const;
	bool operator ==(const CBaseHandle& other) const;
	bool operator ==(const IHandleEntity* e) const;
	bool operator !=(const IHandleEntity* e) const;
	bool operator <(const CBaseHandle& other) const;
	bool operator <(const IHandleEntity* e) const;

	// Assign a value to the handle.
	const CBaseHandle& operator=(const IHandleEntity* pEntity);
	const CBaseHandle& Set(const IHandleEntity* pEntity);

	// Use this to dereference the handle.
	// Note: this is implemented in game code (ehandle.h)
	IHandleEntity* Get() const;

protected:
	// The low NUM_SERIAL_BITS hold the index. If this value is less than MAX_EDICTS, then the entity is networkable.
	// The high NUM_SERIAL_NUM_BITS bits are the serial number.
	unsigned long	m_Index;
};

class IHandleEntity
{
public:
	virtual ~IHandleEntity() {}
	virtual void SetRefEHandle(const CBaseHandle& handle) = 0;
	virtual const CBaseHandle& GetRefEHandle() const = 0;
};

inline CBaseHandle::CBaseHandle() {
	m_Index = INVALID_EHANDLE_INDEX;
}

inline CBaseHandle::CBaseHandle(const CBaseHandle& other) {
	m_Index = other.m_Index;
}

inline CBaseHandle::CBaseHandle(unsigned long value) {
	m_Index = value;
}

inline CBaseHandle::CBaseHandle(int iEntry, int iSerialNumber) {
	Init(iEntry, iSerialNumber);
}

inline void CBaseHandle::Init(int iEntry, int iSerialNumber) {
	m_Index = iEntry | (iSerialNumber << NUM_ENT_ENTRY_BITS);
}

inline void CBaseHandle::Term() {
	m_Index = INVALID_EHANDLE_INDEX;
}

inline bool CBaseHandle::IsValid() const {
	return m_Index != INVALID_EHANDLE_INDEX;
}

inline int CBaseHandle::GetEntryIndex() const {
	return m_Index & ENT_ENTRY_MASK;
}

inline int CBaseHandle::GetSerialNumber() const {
	return m_Index >> NUM_ENT_ENTRY_BITS;
}

inline unsigned long CBaseHandle::ToLong() const {
	return (unsigned long)m_Index;
}

inline int CBaseHandle::ToInt() const {
	return (int)m_Index;
}

inline bool CBaseHandle::operator !=(const CBaseHandle& other) const {
	return m_Index != other.m_Index;
}

inline bool CBaseHandle::operator ==(const CBaseHandle& other) const {
	return m_Index == other.m_Index;
}

inline bool CBaseHandle::operator ==(const IHandleEntity* e) const {
	return Get() == e;
}

inline bool CBaseHandle::operator !=(const IHandleEntity* e) const {
	return Get() != e;
}

inline bool CBaseHandle::operator <(const CBaseHandle& other) const {
	return m_Index < other.m_Index;
}

inline bool CBaseHandle::operator <(const IHandleEntity* pEntity) const {
	unsigned long otherIndex = (pEntity) ? pEntity->GetRefEHandle().m_Index : INVALID_EHANDLE_INDEX;
	return m_Index < otherIndex;
}

inline const CBaseHandle& CBaseHandle::operator=(const IHandleEntity* pEntity) {
	return Set(pEntity);
}

inline const CBaseHandle& CBaseHandle::Set(const IHandleEntity* pEntity) {
	if (pEntity) {
		*this = pEntity->GetRefEHandle();
	}
	else {
		m_Index = INVALID_EHANDLE_INDEX;
	}

	return *this;
}
class IClientNetworkable;
class IClientEntity;
class IClientEntityList
{
public:
    virtual IClientNetworkable* GetClientNetworkable(int entnum) = 0;
    virtual void* vtablepad0x1(void) = 0;
    virtual void* vtablepad0x2(void) = 0;
    virtual IClientEntity* GetClientEntity(int entNum) = 0;
    virtual IClientEntity* GetClientEntityFromHandle(CBaseHandle hEnt) = 0;
    virtual int                   NumberOfEntities(bool bIncludeNonNetworkable) = 0;
    virtual int                   GetHighestEntityIndex(void) = 0;
    virtual void                  SetMaxEntities(int maxEnts) = 0;
    virtual int                   GetMaxEntities() = 0;
};
inline int get_moving_flag(const UserCmd* cmd)
{
	if (cmd->buttons & UserCmd::IN_USE && config->condAA.onUse)
		return AntiAim::latest_moving_flag = AntiAim::on_use;
	if (config->condAA.slowwalk)
		if (config->misc.slowwalkKey.isActive())
			return AntiAim::latest_moving_flag = AntiAim::slow_walking;
	if (!localPlayer->getAnimstate()->onGround && cmd->buttons & UserCmd::IN_DUCK)
		if (config->condAA.cjump)
			return AntiAim::latest_moving_flag = AntiAim::duck_jumping;
	if (!localPlayer->getAnimstate()->onGround)
		if (config->condAA.jumping)
			return AntiAim::latest_moving_flag = AntiAim::jumping;
	if (cmd->buttons & UserCmd::IN_DUCK)
		if (config->condAA.chrouch)
			return AntiAim::latest_moving_flag = AntiAim::ducking;
	if (localPlayer->velocity().length2D() > 5.f)
	{
		if (config->condAA.moving)
			return AntiAim::latest_moving_flag = AntiAim::moving;
	}
	return AntiAim::latest_moving_flag = AntiAim::freestanding;
}
/*int WeaponClassSex()
{
	if (!localPlayer || !localPlayer->isAlive())
		return -1;

	auto weapon = localPlayer->getActiveWeapon();
	if (!weapon)
		return -1;

	if (weapon->itemDefinitionIndex2() == WeaponId::Elite || weapon->itemDefinitionIndex2() == WeaponId::Hkp2000 || weapon->itemDefinitionIndex2() == WeaponId::P250 || weapon->itemDefinitionIndex2() == WeaponId::Usp_s || weapon->itemDefinitionIndex2() == WeaponId::Cz75a || weapon->itemDefinitionIndex2() == WeaponId::Tec9 || weapon->itemDefinitionIndex2() == WeaponId::Fiveseven || weapon->itemDefinitionIndex2() == WeaponId::Glock)
		return 0;

	if (weapon->itemDefinitionIndex2() == WeaponId::Deagle)
		return 1;

	if (weapon->itemDefinitionIndex2() == WeaponId::Revolver)
		return 2;

	if (weapon->itemDefinitionIndex2() == WeaponId::Mac10 || weapon->itemDefinitionIndex2() == WeaponId::Mp9 || weapon->itemDefinitionIndex2() == WeaponId::Mp7 || weapon->itemDefinitionIndex2() == WeaponId::Mp5sd || weapon->itemDefinitionIndex2() == WeaponId::Ump45 || weapon->itemDefinitionIndex2() == WeaponId::P90 || weapon->itemDefinitionIndex2() == WeaponId::Bizon)
		return 3;

	if (weapon->itemDefinitionIndex2() == WeaponId::M249 || weapon->itemDefinitionIndex2() == WeaponId::Negev)
		return 4;

	if (weapon->itemDefinitionIndex2() == WeaponId::Sawedoff || weapon->itemDefinitionIndex2() == WeaponId::Mag7 || weapon->itemDefinitionIndex2() == WeaponId::Xm1014 || weapon->itemDefinitionIndex2() == WeaponId::Nova)
		return 5;

	if (weapon->itemDefinitionIndex2() == WeaponId::Ak47 || weapon->itemDefinitionIndex2() == WeaponId::M4A1 || weapon->itemDefinitionIndex2() == WeaponId::M4a1_s || weapon->itemDefinitionIndex2() == WeaponId::GalilAr || weapon->itemDefinitionIndex2() == WeaponId::Aug || weapon->itemDefinitionIndex2() == WeaponId::Sg553 || weapon->itemDefinitionIndex2() == WeaponId::Famas)
		return 6;

	if (weapon->itemDefinitionIndex2() == WeaponId::Scar20 || weapon->itemDefinitionIndex2() == WeaponId::G3SG1)
		return 7;

	if (weapon->itemDefinitionIndex2() == WeaponId::Ssg08)
		return 8;

	if (weapon->itemDefinitionIndex2() == WeaponId::Awp)
		return 9;
}*/