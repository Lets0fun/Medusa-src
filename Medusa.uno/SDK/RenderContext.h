/*#pragma once

#include <functional>

#include "VirtualMethod.h"

class RenderContext {
public:
    VIRTUAL_METHOD(void, release, 1, (), (this))
    VIRTUAL_METHOD(void, beginRender, 2, (), (this))
    VIRTUAL_METHOD(void, endRender, 3, (), (this))
    VIRTUAL_METHOD(void, getViewport, 41, (int& x, int& y, int& width, int& height), (this, std::ref(x), std::ref(y), std::ref(width), std::ref(height)))
};*/

#pragma once

#include <functional>

#include "VirtualMethod.h"

#include "MaterialSystem.h"

class IRefCounted
{
public:
    virtual int AddReference() = 0;
    virtual int Release() = 0;
};
template <class T>
class CBaseAutoPtr
{
public:
    CBaseAutoPtr() :
        pObject(nullptr) { }

    CBaseAutoPtr(T* pObject) :
        pObject(pObject) { }

    operator const void* () const
    {
        return pObject;
    }

    operator void* () const
    {
        return pObject;
    }

    operator const T* () const
    {
        return pObject;
    }

    operator const T* ()
    {
        return pObject;
    }

    operator T* ()
    {
        return pObject;
    }

    int	operator=(int i)
    {
        pObject = nullptr;
        return 0;
    }

    T* operator=(T* pSecondObject)
    {
        pObject = pSecondObject;
        return pSecondObject;
    }

    bool operator!() const
    {
        return (!pObject);
    }

    bool operator==(const void* pSecondObject) const
    {
        return (pObject == pSecondObject);
    }

    bool operator!=(const void* pSecondObject) const
    {
        return (pObject != pSecondObject);
    }

    bool operator==(T* pSecondObject) const
    {
        return operator==(static_cast<void*>(pSecondObject));
    }

    bool operator!=(T* pSecondObject) const
    {
        return operator!=(static_cast<void*>(pSecondObject));
    }

    bool operator==(const CBaseAutoPtr<T>& pSecondPtr) const
    {
        return operator==(static_cast<const void*>(pSecondPtr));
    }

    bool operator!=(const CBaseAutoPtr<T>& pSecondPtr) const
    {
        return operator!=(static_cast<const void*>(pSecondPtr));
    }

    T* operator->()
    {
        return pObject;
    }

    T& operator*()
    {
        return *pObject;
    }

    T** operator&()
    {
        return &pObject;
    }

    const T* operator->() const
    {
        return pObject;
    }

    const T& operator*() const
    {
        return *pObject;
    }

    T* const* operator&() const
    {
        return &pObject;
    }

    CBaseAutoPtr(const CBaseAutoPtr<T>& pSecondPtr) :
        pObject(pSecondPtr.pObject) { }

    void operator=(const CBaseAutoPtr<T>& pSecondPtr)
    {
        pObject = pSecondPtr.pObject;
    }

    T* pObject;
};

template <class T>
class CRefPtr : public CBaseAutoPtr<T>
{
    typedef CBaseAutoPtr<T> CBaseClass;
public:
    CRefPtr() { }

    CRefPtr(T* pInit)
        : CBaseClass(pInit) { }

    CRefPtr(const CRefPtr<T>& pRefPtr)
        : CBaseClass(pRefPtr) { }

    ~CRefPtr()
    {
        if (CBaseClass::pObject != nullptr)
            CBaseClass::pObject->Release();
    }

    void operator=(const CRefPtr<T>& pSecondRefPtr)
    {
        CBaseClass::operator=(pSecondRefPtr);
    }

    int operator=(int i)
    {
        return CBaseClass::operator=(i);
    }

    T* operator=(T* pSecond)
    {
        return CBaseClass::operator=(pSecond);
    }

    operator bool() const
    {
        return !CBaseClass::operator!();
    }

    operator bool()
    {
        return !CBaseClass::operator!();
    }

    void SafeRelease()
    {
        if (CBaseClass::pObject != nullptr)
            CBaseClass::pObject->Release();

        CBaseClass::pObject = nullptr;
    }

    void AssignAddReference(T* pFrom)
    {
        if (pFrom != nullptr)
            pFrom->AddReference();

        SafeRelease();
        CBaseClass::pObject = pFrom;
    }

    void AddReferenceAssignTo(T*& pTo)
    {
        if (CBaseClass::pObject != nullptr)
            CBaseClass::pObject->AddReference();

        SafeRelease(pTo);
        pTo = CBaseClass::pObject;
    }
};

class CRefCounted
{
public:
    virtual void Destructor(char bDelete) = 0;
    virtual bool OnFinalRelease() = 0;

    void Release()
    {
        if (InterlockedDecrement(&vlRefCount) == 0 && OnFinalRelease())
            Destructor(1);
    }

private:
    volatile long vlRefCount;
};

class RenderContext : public IRefCounted
{
private:
    template <typename T, typename ... args_t>
    constexpr T CallVFunc(void* thisptr, std::size_t nIndex, args_t... argList)
    {
        using VirtualFn = T(__thiscall*)(void*, decltype(argList)...);
        return (*static_cast<VirtualFn**>(thisptr))[nIndex](thisptr, argList...);
    }
public:
    void beginRender()
    {
        CallVFunc<void>(this, 2);
    }

    void endRender()
    {
        CallVFunc<void>(this, 3);
    }

    void BindLocalCubemap(void* pTexture)
    {
        CallVFunc<void>(this, 5, pTexture);
    }

    void SetRenderTarget(void* pTexture)
    {
        CallVFunc<void>(this, 6, pTexture);
    }

    void* GetRenderTarget()/*ITexture*/
    {
        return CallVFunc<void*>(this, 7);
    }

    void ClearBuffers(bool bClearColor, bool bClearDepth, bool bClearStencil = false)
    {
        CallVFunc<void>(this, 12, bClearColor, bClearDepth, bClearStencil);
    }

    void SetLights(int nCount, const void* pLights)
    {
        CallVFunc<void>(this, 17, nCount, pLights);
    }

    void SetAmbientLightCube(ImVec4 vecCube[6])
    {
        CallVFunc<void>(this, 18, vecCube);
    }

    void SetIntRenderingParameter(int a1, int a2)
    {
        //return GetVirtual < void(__thiscall*)(void*, int, int) >(this, 126)(this, a1, a2);
    }

    void Viewport(int x, int y, int iWidth, int iHeight)
    {
        CallVFunc<void>(this, 40, x, y, iWidth, iHeight);
    }

    void getViewport(int& x, int& y, int& iWidth, int& iHeight)
    {
        CallVFunc<void, int&, int&, int&, int&>(this, 41, x, y, iWidth, iHeight);
    }

    void ClearColor3ub(unsigned char r, unsigned char g, unsigned char b)
    {
        CallVFunc<void>(this, 78, r, g, b);
    }

    void ClearColor4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
    {
        CallVFunc<void>(this, 79, r, g, b, a);
    }

    //void DrawScreenSpaceRectangle(Material* pMaterial, int iDestX, int iDestY, int iWidth, int iHeight, float flTextureX0, float flTextureY0, float flTextureX1, float flTextureY1, int iTextureWidth, int iTextureHeight, void* pClientRenderable = nullptr, int nXDice = 1, int nYDice = 1)
    //{
    //    CallVFunc<void>(this, 114, pMaterial, iDestX, iDestY, iWidth, iHeight, flTextureX0, flTextureY0, flTextureX1, flTextureY1, iTextureWidth, iTextureHeight, pClientRenderable, nXDice, nYDice);
    //}

    void PushRenderTargetAndViewport()
    {
        CallVFunc<void>(this, 119);
    }

    void PopRenderTargetAndViewport()
    {
        CallVFunc<void>(this, 120);
    }

    void SetLightingOrigin(/*Vector vecLightingOrigin*/float x, float y, float z)
    {
        CallVFunc<void>(this, 158, x, y, z);
    }

};

class RenderContextPtr : public CRefPtr<RenderContext>
{
    typedef CRefPtr<RenderContext> CBaseClass;
public:
    RenderContextPtr() = default;

    RenderContextPtr(RenderContext* pInit) : CBaseClass(pInit)
    {
        if (CBaseClass::pObject != nullptr)
            CBaseClass::pObject->beginRender();
    }

    RenderContextPtr(MaterialSystem* pFrom) : CBaseClass(pFrom->getRenderContext())
    {
        if (CBaseClass::pObject != nullptr)
            CBaseClass::pObject->beginRender();
    }

    ~RenderContextPtr()
    {
        if (CBaseClass::pObject != nullptr)
            CBaseClass::pObject->endRender();
    }

    RenderContext* operator=(RenderContext* pSecondContext)
    {
        if (pSecondContext != nullptr)
            pSecondContext->beginRender();

        return CBaseClass::operator=(pSecondContext);
    }

    void SafeRelease()
    {
        if (CBaseClass::pObject != nullptr)
            CBaseClass::pObject->endRender();

        CBaseClass::SafeRelease();
    }

    void AssignAddReference(RenderContext* pFrom)
    {
        if (CBaseClass::pObject)
            CBaseClass::pObject->endRender();

        CBaseClass::AssignAddReference(pFrom);
        CBaseClass::pObject->beginRender();
    }

    void GetFrom(MaterialSystem* pFrom)
    {
        AssignAddReference(pFrom->getRenderContext());
    }

private:
    RenderContextPtr(const RenderContextPtr& pRefPtr);
    void operator=(const RenderContextPtr& pSecondRefPtr);
};
