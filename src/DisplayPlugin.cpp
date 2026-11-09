// Display extension for the original Steam build.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <unknwn.h>
#include "DisplayModes.h"
#include "CameraStartup.h"
#pragma comment(lib,"user32.lib")
static Display::Modes modes;
static uintptr_t gameBase;
static uintptr_t afterStartup,afterCycle,afterLabel,nativeInit,afterBlend;
static bool fixAltTab=true;
// Normalize an invalid D3DRS_BLENDOP during device recovery.
static __declspec(naked) void hookBlend(){__asm{
    mov eax,[esp+4]
    mov ecx,[esp+8]
    cmp eax,171
    jne nativeBlend
    cmp ecx,-1
    jne nativeBlend
    mov ecx,1
 nativeBlend:
    jmp dword ptr [afterBlend]
}}
static wchar_t gameFolder[MAX_PATH],settingsPath[MAX_PATH],resolutionPath[MAX_PATH],logPath[MAX_PATH];
static bool installed;
static HWND gameWindow;
static void logLine(const char* text) {
    HANDLE f=CreateFileW(logPath,FILE_APPEND_DATA,FILE_SHARE_READ,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(f==INVALID_HANDLE_VALUE)return;
    DWORD done=0;WriteFile(f,text,static_cast<DWORD>(strlen(text)),&done,nullptr);CloseHandle(f);
}
struct Patch {uint32_t rva;const BYTE* expected;size_t length;const void* destination;bool call=false;bool bytes=false;};
static bool applyPatches(const Patch* patches,size_t patchCount) {
    if(patchCount>20)return false;
    for(size_t i=0;i<patchCount;++i){const auto& p=patches[i];if(memcmp(reinterpret_cast<void*>(gameBase+p.rva),p.expected,p.length)){
        char message[100];sprintf_s(message,"Unexpected game code at RVA %06x. Patch group skipped.\r\n",p.rva);logLine(message);return false;
    }}
    DWORD protections[20]{};
    size_t unlocked=0;
    for(size_t i=0;i<patchCount;++i){
        const auto& p=patches[i];
        if(!VirtualProtect(reinterpret_cast<void*>(gameBase+p.rva),p.length,PAGE_EXECUTE_READWRITE,&protections[unlocked]))break;
        ++unlocked;
    }
    if(unlocked!=patchCount){
        for(size_t i=unlocked;i-- >0;){DWORD ignored;VirtualProtect(reinterpret_cast<void*>(gameBase+patches[i].rva),patches[i].length,protections[i],&ignored);}
        return false;
    }
    for(size_t i=0;i<patchCount;++i){
        const auto& p=patches[i];BYTE* target=reinterpret_cast<BYTE*>(gameBase+p.rva);
        if(p.bytes){memcpy(target,p.destination,p.length);FlushInstructionCache(GetCurrentProcess(),target,p.length);continue;}
        target[0]=p.call?0xe8:0xe9;int32_t displacement=static_cast<int32_t>(reinterpret_cast<uintptr_t>(p.destination)-reinterpret_cast<uintptr_t>(target)-5);
        memcpy(target+1,&displacement,4);memset(target+5,0x90,p.length-5);
        FlushInstructionCache(GetCurrentProcess(),target,p.length);
    }
    // Restore shared-page protections in reverse order.
    for(size_t i=patchCount;i-- >0;){DWORD ignored;VirtualProtect(reinterpret_cast<void*>(gameBase+patches[i].rva),patches[i].length,protections[i],&ignored);}
    return true;
}
#include "WidescreenHooks.h"

static uint32_t startupSlot;
static Display::Mode desktopMode{};
static Display::Mode forcedMode{};
enum class StartupResolution { Automatic, Saved, Desktop };
static StartupResolution startupResolution=StartupResolution::Automatic;
static bool resolutionReady;
#ifndef DISPLAY_TEST
static bool supported(uint32_t width,uint32_t height);
static void startCameraHelper();
#else
static bool test4kAvailable=true,testDeckAvailable=true;
static bool supported(uint32_t width,uint32_t height){return (width!=3840||height!=2160||test4kAvailable)&&(width!=1280||height!=800||testDeckAvailable);}
#endif
static Display::Mode chooseStartupMode(uint32_t slot,Display::Mode native) {
    const auto valid=[](Display::Mode m){return m.width>=640&&m.height>=480&&m.width<=16384&&m.height<=16384;};
    const bool desktopAvailable=valid(desktopMode);
    const auto available=[&](Display::Mode m){
        return valid(m)&&((m.width==desktopMode.width&&m.height==desktopMode.height)||supported(m.width,m.height));
    };
    if(forcedMode.width||forcedMode.height) {
        if(available(forcedMode))return forcedMode;
        logLine("Forced resolution unavailable. Checking the saved and desktop modes.\r\n");
    }
    // The stock game can write these slots without a menu selection.
    if(desktopAvailable&&(startupResolution==StartupResolution::Desktop||
       (startupResolution==StartupResolution::Automatic&&slot<5)))return desktopMode;
    const auto saved=modes.loadToken(slot);
    if(saved.width&&saved.height){
        if(available(saved))return saved;
        logLine("Saved display unavailable. Using the desktop or game default.\r\n");
    }
    return desktopAvailable?desktopMode:native;
}
static void __cdecl readStartup(uint32_t* config) {
    gameWindow=reinterpret_cast<HWND>(config[1]);
    HANDLE f=CreateFileW(resolutionPath,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    DWORD slot=0xffffffff,done=0;
    if(f!=INVALID_HANDLE_VALUE){if(!ReadFile(f,&slot,4,&done,nullptr)||done!=4)slot=0xffffffff;CloseHandle(f);}
    startupSlot=slot;
    const Display::Mode selected=chooseStartupMode(slot,{config[3],config[4]});
    config[3]=selected.width;config[4]=selected.height;
    modes.add(selected.width,selected.height);
    Wide::setCanvas(static_cast<float>(selected.width),static_cast<float>(selected.height));
    resolutionReady=true;
#ifndef DISPLAY_TEST
    *reinterpret_cast<uint32_t*>(gameBase+0x361208)=modes.selection(selected.width,selected.height);
    HWND window=gameWindow;
    RECT rect{0,0,static_cast<LONG>(selected.width),static_cast<LONG>(selected.height)};
    DWORD style=static_cast<DWORD>(GetWindowLongW(window,GWL_STYLE));
    DWORD exStyle=static_cast<DWORD>(GetWindowLongW(window,GWL_EXSTYLE));
    AdjustWindowRectEx(&rect,style,FALSE,exStyle);
    SetWindowPos(window,nullptr,0,0,rect.right-rect.left,rect.bottom-rect.top,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
#endif
#ifndef DISPLAY_TEST
    startCameraHelper();
#endif
    char line[128];sprintf_s(line,"Renderer startup preset %u: %u x %u.\r\n",slot,config[3],config[4]);logLine(line);
}
static void __cdecl cycleMenu(uint32_t menu,int action) {
    auto width=reinterpret_cast<uint32_t*>(menu+0xd3b8);
    *width=modes.cycle(*width,action);
}
static void __cdecl labelMenu(uint32_t* registers) {
    uint32_t width=*reinterpret_cast<uint32_t*>(registers[4]+0xd3b8);
    Display::Mode mode=modes.find(width);registers[6]=mode.width;registers[5]=mode.height;
}
static void __cdecl saveResolution(uint32_t width) {
    if(!resolutionReady)return;
    int i=modes.index(width);if(i<0)return;
    HANDLE f=CreateFileW(resolutionPath,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(f==INVALID_HANDLE_VALUE){logLine("Could not save res.bin.\r\n");return;}
    DWORD slot=modes.saveToken(static_cast<size_t>(i)),done=0;
    bool ok=WriteFile(f,&slot,sizeof(slot),&done,nullptr)&&done==sizeof(slot);
    CloseHandle(f);if(!ok)logLine("Could not write resolution selection.\r\n");
}
// PUSHAD layout: EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX.
static __declspec(naked) void hookStartup(){__asm{
    pushfd
    pushad
    mov ebp,esp
    sub esp,528
    and esp,0xfffffff0
    fxsave [esp]
    fninit
    mov eax,[ebp+8]
    sub eax,0x40
    push eax
    call readStartup
    add esp,4
    fxrstor [esp]
    mov esp,ebp
    popad
    popfd
    lea ecx,[ebp-0x40]
    push ecx
    call dword ptr [nativeInit]
    jmp dword ptr [afterStartup]
}}
static __declspec(naked) void hookCycle(){__asm{
    pushfd
    pushad
    mov ebp,esp
    sub esp,528
    and esp,0xfffffff0
    fxsave [esp]
    fninit
    push edi
    push ebx
    call cycleMenu
    add esp,8
    fxrstor [esp]
    mov esp,ebp
    popad
    popfd
    jmp dword ptr [afterCycle]
}}
static __declspec(naked) void hookLabel(){__asm{
    pushfd
    pushad
    mov ebp,esp
    sub esp,528
    and esp,0xfffffff0
    fxsave [esp]
    fninit
    push ebp
    call labelMenu
    add esp,4
    fxrstor [esp]
    mov esp,ebp
    popad
    popfd
    jmp dword ptr [afterLabel]
}}
// Replace the startup cursor clip only.
#ifndef DISPLAY_TEST
struct StartupCursorApi {
    static bool target(RECT& area) {
        if(!gameWindow||!IsWindow(gameWindow))return false;
        RECT client{};if(!GetClientRect(gameWindow,&client))return false;
        POINT top{client.left,client.top},bottom{client.right,client.bottom};
        if(!ClientToScreen(gameWindow,&top)||!ClientToScreen(gameWindow,&bottom))return false;
        area={top.x,top.y,bottom.x,bottom.y};return area.right>area.left&&area.bottom>area.top;
    }
    static BOOL set(const RECT* area){return ClipCursor(area);}
};
#endif
template<class Api> static BOOL correctedStartupClip(const RECT* requested) {
    if(!requested)return Api::set(nullptr);
    RECT actual{};
    // Release stale bounds if the query fails.
    return Api::set(Api::target(actual)?&actual:nullptr);
}
static BOOL WINAPI startupClip(const RECT* requested) {
#ifndef DISPLAY_TEST
    // Query cursor bounds in physical screen pixels.
    using ChangeDpi=HANDLE(WINAPI*)(HANDLE);
    auto change=reinterpret_cast<ChangeDpi>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"SetThreadDpiAwarenessContext"));
    HANDLE previous=change?change(reinterpret_cast<HANDLE>(-4)):nullptr;
    BOOL result=correctedStartupClip<StartupCursorApi>(requested);
    if(previous)change(previous);
    return result;
#else
    (void)requested;return TRUE;
#endif
}
static bool installPatches() {
    static const BYTE startup[]={0x8d,0x4d,0xc0,0x51,0xe8,0xce,0x0f,0x00,0x00};
    static const BYTE cycle[]={0x81,0xff,0xe6,0x00,0x00,0x00};
    static const BYTE label[]={0x8b,0x8b,0xb8,0xd3,0x00,0x00};
    static const BYTE save[]={0x55,0x8b,0xec,0x8b,0x45,0x08};
    static const BYTE clip[]={0xff,0x15,0x1c,0x53,0x8a,0x00};
    static const BYTE blend[]={0x8b,0x44,0x24,0x04,0x8b,0x4c,0x24,0x08};
    static const BYTE coldCache[]={0x89,0x58,0xfc},resetCache[]={0x89,0x68,0xfc},zeroCache[]={0x89,0x50,0xfc};
    static const BYTE focusPause[]={0x74,0x05},skipPause[]={0xeb,0x05};
    const Patch patches[]={{0x2949,startup,sizeof(startup),hookStartup},{0xc750,cycle,sizeof(cycle),hookCycle},
        {0xc7c3,label,sizeof(label),hookLabel},{0x92b0,save,sizeof(save),saveResolution},{0x22b8,clip,sizeof(clip),startupClip,true},
        {0x239810,blend,sizeof(blend),hookBlend},
        {0x23a143,coldCache,sizeof(coldCache),zeroCache,false,true},
        {0x23a7a3,resetCache,sizeof(resetCache),zeroCache,false,true},
        {0x2843,focusPause,sizeof(focusPause),skipPause,false,true}};
    size_t patchCount=sizeof(patches)/sizeof(patches[0])-(fixAltTab?0:4);
    // Signatures target the fixed-base Steam executable.
#ifndef DISPLAY_TEST
    if(gameBase!=0x400000)return false;
#endif
    afterBlend=gameBase+0x239818;
    afterStartup=gameBase+0x2952;nativeInit=gameBase+0x3920;afterCycle=gameBase+0xc793;afterLabel=gameBase+0xc894;
    if(!applyPatches(patches,patchCount))return false;
    if(fixAltTab)logLine("Alt-Tab blend, texture-stage cache and focus-handler corrections installed.\r\n");
    installed=true;logLine("Resolution menu extension installed. Selection applies on next launch.\r\n");return true;
}
#ifndef DISPLAY_TEST
static bool compatibleGame,configured,enabled=true;
static void configure();
static void tryInstall(){
    if(!compatibleGame||installed)return;
    if(!configured){configure();configured=true;enabled=GetPrivateProfileIntW(L"Display",L"Enabled",1,settingsPath)!=0;}
    if(enabled&&installPatches()){
        logLine(Wide::install()?"Widescreen presentation configured.\r\n":"Widescreen hooks unavailable; base display and camera remain active.\r\n");
    }
}
static bool supported(uint32_t width,uint32_t height){
    DEVMODEW dm{};dm.dmSize=sizeof(dm);
    for(DWORD i=0;EnumDisplaySettingsW(nullptr,i,&dm);++i)
        if(dm.dmPelsWidth==width&&dm.dmPelsHeight==height&&dm.dmBitsPerPel==32)return true;
    return false;
}
static void configure() {
    GetModuleFileNameW(nullptr,gameFolder,MAX_PATH);
    wchar_t* slash=wcsrchr(gameFolder,L'\\');if(slash)*slash=0;
    swprintf_s(settingsPath,L"%s\\ColdFearDisplay.ini",gameFolder);
    fixAltTab=GetPrivateProfileIntW(L"Graphics",L"FixAltTab",1,settingsPath)!=0;
    swprintf_s(resolutionPath,L"%s\\res.bin",gameFolder);
    swprintf_s(logPath,L"%s\\ColdFearDisplay.log",gameFolder);
    const auto desktop=GetPrivateProfileIntW(L"Display",L"UseDesktopResolution",-1,settingsPath);
    startupResolution=desktop==0?StartupResolution::Saved:desktop==1?StartupResolution::Desktop:StartupResolution::Automatic;
    forcedMode={GetPrivateProfileIntW(L"Display",L"ResolutionWidth",0,settingsPath),
                GetPrivateProfileIntW(L"Display",L"ResolutionHeight",0,settingsPath)};
    Wide::correctHud=GetPrivateProfileIntW(L"Widescreen",L"CorrectHUD",1,settingsPath)!=0;
    Wide::edgeHud=GetPrivateProfileIntW(L"Widescreen",L"HUDAtEdges",1,settingsPath)!=0;
    Wide::edgeAmount=std::min(100u,GetPrivateProfileIntW(L"Widescreen",L"HUDEdgePercent",100,settingsPath))*0.01f;
    Wide::correctProjection=GetPrivateProfileIntW(L"Widescreen",L"CorrectFOV",1,settingsPath)!=0;
    Wide::hideBorders=GetPrivateProfileIntW(L"Widescreen",L"RemoveCutsceneBorders",0,settingsPath)!=0;
    auto video=GetPrivateProfileIntW(L"Widescreen",L"VideoMode",1,settingsPath);
    Wide::videoMode=video==0?Display::VideoMode::Native:video==2?Display::VideoMode::Fill:Display::VideoMode::Fit;
    uint32_t width=GetPrivateProfileIntW(L"Display",L"NativeWidth",0,settingsPath);
    uint32_t height=GetPrivateProfileIntW(L"Display",L"NativeHeight",0,settingsPath);
    DEVMODEW dm{};dm.dmSize=sizeof(dm);
    if(EnumDisplaySettingsW(nullptr,ENUM_CURRENT_SETTINGS,&dm))desktopMode={dm.dmPelsWidth,dm.dmPelsHeight};
    if(!width&&!height){width=desktopMode.width;height=desktopMode.height;}
    if(supported(width,height)&&modes.add(width,height)){
        char line[128];sprintf_s(line,"Native display: %u x %u. Menu choices: %u.\r\n",width,height,static_cast<unsigned>(modes.count));logLine(line);
    }else logLine("Native mode unsupported or the mode list is full. Fixed choices retained.\r\n");
    if(forcedMode.width&&forcedMode.height&&supported(forcedMode.width,forcedMode.height))modes.add(forcedMode.width,forcedMode.height);
}
static void startCameraHelper(){
    if(!GetPrivateProfileIntW(L"CameraMod",L"AutoStartHelper",1,settingsPath))return;
    DWORD error;
    if(launchCameraHelper(gameFolder,error))logLine("Camera/controller helper started automatically.\r\n");
    else if(error){char line[128];sprintf_s(line,"Camera/controller helper could not start: Windows error %lu.\r\n",error);logLine(line);}
}
static INIT_ONCE systemOnce=INIT_ONCE_STATIC_INIT;
static HMODULE systemInput;
static BOOL CALLBACK loadSystemInput(PINIT_ONCE,PVOID,PVOID*) {
    wchar_t path[MAX_PATH];if(!GetSystemDirectoryW(path,MAX_PATH))return FALSE;
    wcscat_s(path,L"\\dinput8.dll");systemInput=LoadLibraryW(path);return systemInput!=nullptr;
}
static FARPROC systemProc(const char* name){
    if(!InitOnceExecuteOnce(&systemOnce,loadSystemInput,nullptr,nullptr))return nullptr;
    return GetProcAddress(systemInput,name);
}
extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE instance,DWORD version,REFIID iid,LPVOID* out,LPUNKNOWN outer){
    // Install before graphics initialization at 0x402944.
    tryInstall();
    using Fn=HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
    auto fn=reinterpret_cast<Fn>(systemProc("DirectInput8Create"));return fn?fn(instance,version,iid,out,outer):E_FAIL;
}
extern "C" HRESULT WINAPI DllCanUnloadNow(){using Fn=HRESULT(WINAPI*)();auto fn=reinterpret_cast<Fn>(systemProc("DllCanUnloadNow"));return fn?fn():S_FALSE;}
extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID cls,REFIID iid,LPVOID* out){using Fn=HRESULT(WINAPI*)(REFCLSID,REFIID,LPVOID*);auto fn=reinterpret_cast<Fn>(systemProc("DllGetClassObject"));return fn?fn(cls,iid,out):CLASS_E_CLASSNOTAVAILABLE;}
extern "C" HRESULT WINAPI DllRegisterServer(){using Fn=HRESULT(WINAPI*)();auto fn=reinterpret_cast<Fn>(systemProc("DllRegisterServer"));return fn?fn():E_FAIL;}
extern "C" HRESULT WINAPI DllUnregisterServer(){using Fn=HRESULT(WINAPI*)();auto fn=reinterpret_cast<Fn>(systemProc("DllUnregisterServer"));return fn?fn():E_FAIL;}
extern "C" const void* WINAPI GetdfDIJoystick(){using Fn=const void*(WINAPI*)();auto fn=reinterpret_cast<Fn>(systemProc("GetdfDIJoystick"));return fn?fn():nullptr;}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){
    if(reason==DLL_PROCESS_ATTACH){
        DisableThreadLibraryCalls(instance);gameBase=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(gameBase);
        auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(gameBase+dos->e_lfanew);
        if(gameBase!=0x400000||nt->OptionalHeader.SizeOfImage!=0x4d6000)return TRUE;
        compatibleGame=true;

    }
    return TRUE;
}
#endif
