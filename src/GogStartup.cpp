#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include "CameraStartup.h"
#include "GameEdition.h"

using CreateD3D9=IDirect3D9*(WINAPI*)(UINT);
static CreateD3D9 systemCreate;
static INIT_ONCE graphicsOnce=INIT_ONCE_STATIC_INIT;
static INIT_ONCE helperOnce=INIT_ONCE_STATIC_INIT;

static BOOL CALLBACK loadGraphics(PINIT_ONCE,PVOID,PVOID*) {
    wchar_t path[MAX_PATH];
    UINT length=GetSystemDirectoryW(path,MAX_PATH);
    if(!length||length+_countof(L"\\d3d9.dll")>MAX_PATH)return FALSE;
    wcscat_s(path,L"\\d3d9.dll");
    HMODULE graphics=LoadLibraryW(path);
    if(!graphics)return FALSE;
    systemCreate=reinterpret_cast<CreateD3D9>(GetProcAddress(graphics,"Direct3DCreate9"));
    if(systemCreate)return TRUE;
    FreeLibrary(graphics);
    return FALSE;
}

static BOOL CALLBACK startHelper(PINIT_ONCE,PVOID,PVOID*) {
    auto image=reinterpret_cast<const BYTE*>(GetModuleHandleW(nullptr));
    auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
    auto nt=reinterpret_cast<const IMAGE_NT_HEADERS*>(image+dos->e_lfanew);
    if(editionFromImageSize(nt->OptionalHeader.SizeOfImage)!=Edition::Gog)return TRUE;

    wchar_t path[MAX_PATH];
    DWORD length=GetModuleFileNameW(nullptr,path,MAX_PATH);
    if(!length||length>=MAX_PATH)return TRUE;
    auto name=wcsrchr(path,L'\\');
    if(!name||_wcsicmp(name+1,L"ColdFear_retail.exe"))return TRUE;
    *name=L'\0';
    DWORD error;
    launchCameraHelper(path,error);
    return TRUE;
}

extern "C" IDirect3D9* WINAPI Direct3DCreate9(UINT sdkVersion) {
    if(!InitOnceExecuteOnce(&graphicsOnce,loadGraphics,nullptr,nullptr))return nullptr;
    auto graphics=systemCreate(sdkVersion);
    if(graphics)InitOnceExecuteOnce(&helperOnce,startHelper,nullptr,nullptr);
    return graphics;
}
