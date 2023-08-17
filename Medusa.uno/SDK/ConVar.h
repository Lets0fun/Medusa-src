#pragma once

#include <type_traits>

#include "Platform.h"
#include "UtlVector.h"
#include "VirtualMethod.h"
#include "Pad.h"

struct ConVar {
    VIRTUAL_METHOD(float, getFloat, 12, (), (this))
    VIRTUAL_METHOD(int, getInt, 13, (), (this))
    VIRTUAL_METHOD(void, setValue, 14, (const char* value), (this, value))
    VIRTUAL_METHOD(void, setValue, 15, (float value), (this, value))
    VIRTUAL_METHOD(void, setValue, 16, (int value), (this, value))

    PAD(24)
    std::add_pointer_t<void __cdecl()> changeCallback;
    ConVar* parent;
    const char* defaultValue;
    char* string;
    PAD(44)
    UtlVector<void(__cdecl*)()> onChangeCallbacks;
};
