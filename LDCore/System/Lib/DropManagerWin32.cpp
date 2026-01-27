#include <Ludens/Header/Platform.h>
#ifdef LD_PLATFORM_WIN32
#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/DropManager.h>

#include <vector>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <oleidl.h>
#pragma comment(lib, "Ole32.lib")

namespace LD {

static bool sHasOLEInit = false;

class DropTargetWin32 : IDropTarget
{
public:
    DropTargetWin32(HWND handle);

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
    virtual ULONG STDMETHODCALLTYPE AddRef() override;
    virtual ULONG STDMETHODCALLTYPE Release() override;
    virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* pDataObj, DWORD, POINTL, DWORD* pdwEffect) override;
    virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
    virtual HRESULT STDMETHODCALLTYPE DragLeave() override;
    virtual HRESULT STDMETHODCALLTYPE Drop(IDataObject* pDataObj, DWORD, POINTL, DWORD* pdwEffect) override;

private:
    uint64_t mDragRefCount = 0;
};

struct DropTargetObj : public DropTargetWin32
{
    DropTargetObj(HWND handle)
        : DropTargetWin32(handle) {}

    DropTargetFileCallback onDropFile = nullptr;
};

DropTargetWin32::DropTargetWin32(HWND handle)
{
    HRESULT result = RegisterDragDrop(handle, static_cast<IDropTarget*>(this));
    LD_ASSERT(SUCCEEDED(result));
}

HRESULT STDMETHODCALLTYPE DropTargetWin32::QueryInterface(REFIID riid, void** ppv)
{
    HRESULT result = E_NOINTERFACE;
    *ppv = nullptr;

    if (riid == IID_IUnknown || riid == IID_IDropTarget)
    {
        *ppv = static_cast<IDropTarget*>(this);
        AddRef();
        result = S_OK;
    }

    return result;
}

ULONG STDMETHODCALLTYPE DropTargetWin32::AddRef()
{
    return (ULONG)InterlockedIncrement(&mDragRefCount);
}

ULONG STDMETHODCALLTYPE DropTargetWin32::Release()
{
    return (ULONG)InterlockedDecrement(&mDragRefCount);
}

HRESULT STDMETHODCALLTYPE DropTargetWin32::DragEnter(IDataObject*, DWORD, POINTL, DWORD* pdwEffect)
{
    *pdwEffect = DROPEFFECT_COPY;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DropTargetWin32::DragOver(DWORD, POINTL, DWORD* pdwEffect)
{
    *pdwEffect = DROPEFFECT_COPY;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DropTargetWin32::DragLeave()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DropTargetWin32::Drop(IDataObject* pDataObj, DWORD, POINTL, DWORD*)
{
    LD_PROFILE_SCOPE;

    DropTargetObj* obj = (DropTargetObj*)this;
    FORMATETC format = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM storageMedium;
    HRESULT result = pDataObj->GetData(&format, &storageMedium);
    wchar_t path[MAX_PATH];

    if (SUCCEEDED(result))
    {
        HDROP hDrop = (HDROP)GlobalLock(storageMedium.hGlobal);

        if (hDrop)
        {
            UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
            std::vector<FS::Path> files((size_t)fileCount);

            for (UINT i = 0; i < fileCount; ++i)
            {
                DragQueryFileW(hDrop, i, path, MAX_PATH);
                files[i] = std::filesystem::absolute(FS::Path(path));
            }

            if (obj->onDropFile)
                obj->onDropFile(files.size(), files.data());

            GlobalUnlock(storageMedium.hGlobal);
        }

        ReleaseStgMedium(&storageMedium);
    }

    return S_OK;
}

//
// PUBLIC API
//

DropTarget DropTarget::create(GLFWwindow* handle, DropTargetFileCallback onDropFile)
{
    if (!sHasOLEInit)
    {
        HRESULT result = OleInitialize(nullptr);
        (void)result; // TODO:
        sHasOLEInit = true;
    }

    HWND nativeHandle = glfwGetWin32Window(handle);
    DropTargetObj* obj = heap_new<DropTargetObj>(MEMORY_USAGE_MISC, nativeHandle);
    obj->onDropFile = onDropFile;

    return DropTarget(obj);
}

void DropTarget::destroy(DropTarget target)
{
    DropTargetObj* obj = target.unwrap();

    heap_delete<DropTargetObj>(obj);
}

} // namespace LD
#endif // LD_PLATFORM_WIN32