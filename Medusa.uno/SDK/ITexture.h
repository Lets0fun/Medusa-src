#pragma once
#include <cstddef>
struct Texture_t
{
    std::byte			pad0[0xC];		// 0x0000
    //IDirect3DTexture9* lpRawTexture;	// 0x000C
    void* lpRawTexture;	// 0x000C
};

class ITexture
{
private:
    std::byte	pad0[0x50];		 // 0x0000
public:
    Texture_t** pTextureHandles; // 0x0050
};

//struct Texture_t
//{
//    std::byte			pad0[0xC];		// 0x0000
//    IDirect3DTexture9* lpRawTexture;	// 0x000C
//};
//
//class ITexture
//{
//private:
//    template <typename T, typename ... args_t>
//    constexpr T CallVFunc(void* thisptr, std::size_t nIndex, args_t... argList)
//    {
//        using VirtualFn = T(__thiscall*)(void*, decltype(argList)...);
//        return (*static_cast<VirtualFn**>(thisptr))[nIndex](thisptr, argList...);
//    }
//private:
//    std::byte	pad0[0x50];		 // 0x0000
//public:
//    Texture_t** pTextureHandles; // 0x0050
//
//    int GetActualWidth()
//    {
//        return CallVFunc<int>(this, 3);
//    }
//
//    int GetActualHeight()
//    {
//        return CallVFunc<int>(this, 4);
//    }
//
//    void IncrementReferenceCount()
//    {
//        CallVFunc<void>(this, 10);
//    }
//
//    void DecrementReferenceCount()
//    {
//        CallVFunc<void>(this, 11);
//    }
//};
