// Cold Fear Third Person v0.6.14. MIT license.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <vector>
#include <cstdlib>
#include <cmath>
#include "ModernControls.h"
#include "CursorConfinement.h"
#pragma comment(lib,"user32.lib")

struct Handle {
    HANDLE h;
    explicit Handle(HANDLE value) : h(value) {}
    ~Handle() { if (h && h != INVALID_HANDLE_VALUE) CloseHandle(h); }
    operator HANDLE() const { return h; }
};
struct Target { DWORD pid; uintptr_t base; };
static bool allowSessionUpdate=false;
static Target findGame() {
    Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    PROCESSENTRY32W entry{}; entry.dwSize=sizeof(entry);
    DWORD pid=0; unsigned count=0;
    if(Process32FirstW(snapshot,&entry)) do {
        if(!_wcsicmp(entry.szExeFile,L"coldfear_retail.exe")) {pid=entry.th32ProcessID; ++count;}
    } while(Process32NextW(snapshot,&entry));
    if(count!=1) return {};
    Handle modules(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid));
    MODULEENTRY32W mod{}; mod.dwSize=sizeof(mod);
    if(!Module32FirstW(modules,&mod)) return {};
    return {pid,reinterpret_cast<uintptr_t>(mod.modBaseAddr)};
}
static bool read(HANDLE h,uintptr_t address,void* data,size_t size) {
    SIZE_T done=0;
    return ReadProcessMemory(h,reinterpret_cast<void*>(address),data,size,&done)&&done==size;
}
static bool writeByte(HANDLE h,uintptr_t address,BYTE value) {
    DWORD old=0,unused=0; SIZE_T done=0;
    if(!VirtualProtectEx(h,reinterpret_cast<void*>(address),1,PAGE_EXECUTE_READWRITE,&old)) return false;
    bool ok=WriteProcessMemory(h,reinterpret_cast<void*>(address),&value,1,&done)&&done==1;
    bool flushed=FlushInstructionCache(h,reinterpret_cast<void*>(address),1)!=0;
    bool restored=VirtualProtectEx(h,reinterpret_cast<void*>(address),1,old,&unused)!=0;
    BYTE actual=0;
    return ok&&flushed&&restored&&read(h,address,&actual,1)&&actual==value;
}
// Steam build 2775; preserve special-event slot 3.
static std::vector<BYTE> expected(uintptr_t base) {
    BYTE bytes[]={
        0x8a,0x83,0xdd,0x16,0,0,0x5f,0x84,0xc0,0x5e,0x74,4,0x6a,3,0xeb,0x50,
        0x8a,0x83,0xca,9,0,0,0x84,0xc0,0x75,0x1d,0x6a,1,0xb9,0x90,0x96,0x7d,0,
        0xe8,0x46,0x0e,0x0a,0,0x8b,0x8b,0xb8,0x16,0,0,0x89,0x8b,0xbc,0x16,0,0,
        0x5b,0x8b,0xe5,0x5d,0xc3,0x8b,0x83,0xb8,0x16,0,0,0x85,0xc0,0x75,0x1d,
        0x6a,2,0xb9,0x90,0x96,0x7d,0,0xe8,0x1f,0x0e,0x0a,0,0x8b,0x93,0xb8,0x16,0,0,
        0x89,0x93,0xbc,0x16,0,0,0x5b,0x8b,0xe5,0x5d,0xc3,0x6a,0,0xb9,0x90,0x96,0x7d,0,
        0xe8,2,0x0e,0x0a,0,0x8b,0x83,0xb8,0x16,0,0,0x89,0x83,0xbc,0x16,0,0,
        0x5b,0x8b,0xe5,0x5d,0xc3
    };
    std::vector<BYTE> result(bytes,bytes+sizeof(bytes));
    uint32_t manager=static_cast<uint32_t>(base+0x3d9690);
    for(size_t i=0;i+4<=result.size();++i) {
        uint32_t v=0; memcpy(&v,result.data()+i,4);
        if(v==0x7d9690) memcpy(result.data()+i,&manager,4);
    }
    return result;
}
static int run(const char* action,bool selectorHook=false) {
    Target t=findGame();
    if(!t.pid) {puts("Start Cold Fear through Steam first. Keep only one copy running."); return 2;}
    bool status=!strcmp(action,"--status"), enable=!strcmp(action,"--enable");
    Handle h(OpenProcess(status ? PROCESS_VM_READ|PROCESS_QUERY_INFORMATION :
        PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION,FALSE,t.pid));
    if(!h.h) {printf("Cannot access Cold Fear (Windows error %lu). Run at the same privilege level as the game.\n",GetLastError()); return 3;}
    auto reference=expected(t.base); std::vector<BYTE> current(reference.size());
    uintptr_t start=t.base+0xa5144;
    if(!read(h,start,current.data(),current.size())) {puts("Cannot read camera selector. Nothing changed."); return 4;}
    constexpr size_t first=0x4a5186-0x4a5144, second=0x4a51a3-0x4a5144;
    BYTE before1=current[first],before2=current[second];
    if((status||selectorHook)&&current[0]==0xe9&&current[5]==0x90)memcpy(current.data(),reference.data(),6);
    current[first]=reference[first]; current[second]=reference[second];
    if(current!=reference || (before1!=2&&before1!=1) || (before2!=0&&before2!=1)) {
        puts("Unsupported game code, game still starting, or another camera mod is active. Nothing changed."); return 5;
    }
    printf("Cold Fear found (PID %lu). Camera: %s\n",t.pid,
        before1==1&&before2==1 ? "third person enabled" : before1==2&&before2==0 ? "original" : "partially patched");
    if(status) return 0;
    BYTE want1=enable?1:2,want2=enable?1:0;
    if(before1==want1&&before2==want2) {puts("Already in the requested mode."); return 0;}
    if(!writeByte(h,start+first,want1)||!writeByte(h,start+second,want2)) {
        bool rollback1=writeByte(h,start+first,before1),rollback2=writeByte(h,start+second,before2);
        puts(rollback1&&rollback2 ? "Write failed; previous state restored." : "Write failed; restart Cold Fear to restore its original code.");
        return 6;
    }
    puts(enable ? "Third-person exploration ENABLED. Normal aiming remains available." : "Original camera selection RESTORED.");
    puts("Installed files were not changed. Restarting Cold Fear resets this mod.");
    return 0;
}
static volatile LONG stopping=0;
static BOOL WINAPI consoleEvent(DWORD event) {
    if(event==CTRL_C_EVENT||event==CTRL_BREAK_EVENT||event==CTRL_CLOSE_EVENT) {
        InterlockedExchange(&stopping,1);
        if(event==CTRL_CLOSE_EVENT) Sleep(1500);
        return TRUE;
    }
    return FALSE;
}
// Suspend threads during patch writes; retain stubs until exit.
struct PausedThreads {
    std::vector<HANDLE> threads;
    bool ok=true;
    explicit PausedThreads(DWORD pid) {
        Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0));
        THREADENTRY32 e{}; e.dwSize=sizeof(e);
        if(!Thread32First(snapshot,&e)) {ok=false;return;}
        do {
            if(e.th32OwnerProcessID!=pid) continue;
            HANDLE thread=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT,FALSE,e.th32ThreadID);
            if(!thread) {ok=false;break;}
            if(SuspendThread(thread)==static_cast<DWORD>(-1)) {CloseHandle(thread);ok=false;break;}
            threads.push_back(thread);
        } while(Thread32Next(snapshot,&e));
        if(threads.empty()) ok=false;
    }
    ~PausedThreads(){for(HANDLE t:threads){ResumeThread(t);CloseHandle(t);}}
};
static bool writeCode(HANDLE h,DWORD pid,uintptr_t address,const BYTE* data,size_t size) {
    for(int attempt=0;attempt<100;++attempt) {
      {
        PausedThreads paused(pid);if(!paused.ok)return false;
        bool safe=true;
        for(HANDLE thread:paused.threads) {
            CONTEXT context{};context.ContextFlags=CONTEXT_CONTROL;
            if(!GetThreadContext(thread,&context))return false;
            if(context.Eip>address&&context.Eip<address+size){safe=false;break;}
        }
        if(safe) {
    DWORD old=0,unused=0; SIZE_T done=0;
    if(!VirtualProtectEx(h,reinterpret_cast<void*>(address),size,PAGE_EXECUTE_READWRITE,&old)) return false;
    bool ok=WriteProcessMemory(h,reinterpret_cast<void*>(address),data,size,&done)&&done==size;
    bool flushed=FlushInstructionCache(h,reinterpret_cast<void*>(address),size)!=0;
    bool restored=VirtualProtectEx(h,reinterpret_cast<void*>(address),size,old,&unused)!=0;
    std::vector<BYTE> actual(size);
    return ok&&flushed&&restored&&read(h,address,actual.data(),size)&&!memcmp(actual.data(),data,size);
        }
      }
      Sleep(1);
    }
    return false;
}
static void eventName(wchar_t* name,size_t size,const wchar_t* kind,DWORD pid) {
    swprintf_s(name,size,L"Local\\ColdFearThirdPerson.%s.%lu",kind,pid);
}
static bool writeData(HANDLE h,uintptr_t address,const void* data,size_t size) {
    SIZE_T n=0;return WriteProcessMemory(h,reinterpret_cast<void*>(address),data,size,&n)&&n==size;
}
static uintptr_t remoteModule(DWORD pid,const wchar_t* filename) {
    Handle modules(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid));
    MODULEENTRY32W m{};m.dwSize=sizeof(m);
    if(Module32FirstW(modules,&m))do{if(!_wcsicmp(m.szModule,filename))return reinterpret_cast<uintptr_t>(m.modBaseAddr);}while(Module32NextW(modules,&m));
    return 0;
}
static uintptr_t remoteLoadLibrary(DWORD pid) {
    auto function=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
    HMODULE owner=nullptr;
    if(!function||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(function),&owner))return 0;
    wchar_t filename[MAX_PATH];if(!GetModuleFileNameW(owner,filename,MAX_PATH))return 0;
    wchar_t* tail=wcsrchr(filename,L'\\');uintptr_t remote=remoteModule(pid,tail?tail+1:filename);
    return remote?remote+reinterpret_cast<uintptr_t>(function)-reinterpret_cast<uintptr_t>(owner):0;
}
struct ModernLink {
    HANDLE process=nullptr;DWORD pid=0;uintptr_t state=0;size_t installed=0;
    uintptr_t sites[22]{};
    size_t sizes[22]={6,6,6,6,6,6,6,6,6,6,9,7,7,6,6,7,6,6,6,7,6,7};
    BYTE originals[22][9]={{0x8b,0x83,0x4c,0x0c,0,0},{0x8d,0x8b,0xb4,1,0,0},{0xd9,0x83,0xd0,0x0a,0,0},{0xd9,0x83,0xcc,0x0a,0,0},{0x8a,0x83,0xca,9,0,0},{0x8b,0x93,0xc4,0x0b,0,0},{0x88,0x83,0xc1,9,0,0},{0x8a,0x8e,0x4c,9,0,0},{0x88,0x83,0xc4,9,0,0},{0x8a,0x83,0xdd,0x16,0,0},{0x55,0x8b,0xec,0x81,0xec,0xe8,0,0,0},{0x55,0x8b,0xec,0x56,0x8b,0x75,8},{0x55,0x8b,0xec,0x56,0x8b,0x75,8},{0x55,0x8b,0xec,0x51,0x53,0x56},{0x8a,0x86,0xca,9,0,0},{0x8b,0x46,8,0x5f,0x89,0x46,0xc},{0x55,0x8b,0xec,0x8b,0x45,8},{0x55,0x8b,0xec,0x8b,0x45,8},{0x55,0x8b,0xec,0x8b,0x45,8},{0x55,0x8b,0xec,0x56,0x8b,0x75,8},{0x8b,0x41,0x08,0xc3,0x90,0x90},{0x8d,0x04,0x50,0x5d,0xc2,0x04,0x00}};
    bool set(bool on){int32_t value=on?1:0;return !state||writeData(process,state+offsetof(ModernState,enabled),&value,sizeof(value));}
    bool restore(){bool ok=set(false);for(size_t i=0;i<installed;++i)if(!writeCode(process,pid,sites[i],originals[i],sizes[i]))ok=false;installed=0;return ok;}
    bool attach(HANDLE h,Target t,const wchar_t* path,float sensitivity,float distance,float transition,bool modern,bool shift,float sceneReturn,bool reload,const ModernState& pad) {
        process=h;pid=t.pid;
        // Require a fresh game process for version changes.
        Handle libraries(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid));
        MODULEENTRY32W library{};library.dwSize=sizeof(library);
        if(Module32FirstW(libraries,&library))do {
            bool ownLibrary=!_wcsicmp(library.szModule,L"ColdFearModernControls.dll")||!_wcsnicmp(library.szModule,L"ModernControls-",15);
            if(ownLibrary&&_wcsicmp(library.szExePath,path)&&!allowSessionUpdate){puts("An earlier controls library is still mapped. Restart Cold Fear before loading a new build. Nothing changed.");return false;}
        }while(Module32NextW(libraries,&library));
        const uintptr_t rvas[]={0xa70a5,0xa4ae4,0xa4a7a,0xa4aad,0xa715a,0xa49f7,0xa65ba,0xa64a9,0xa65d5,0xa5144,0xa6550,0x85010,0x84fa0,0x85110,0xac34e,0x85549,0x84f40,0x84f60,0x84f80,0x85080,0x84ad0,0x61769};
        for(size_t i=0;i<22;++i){sites[i]=t.base+rvas[i];BYTE actual[9];if(!read(h,sites[i],actual,sizes[i])||memcmp(actual,originals[i],sizes[i])){puts("Unsupported modern-controls code. Nothing changed.");return false;}}
        HMODULE local=LoadLibraryExW(path,nullptr,DONT_RESOLVE_DLL_REFERENCES);
        if(!local){puts("Cannot read ColdFearModernControls.dll. Extract the complete ZIP.");return false;}
        const char* names[]={"HookInput","HookBasis","HookPitch","HookYaw","HookFacing","HookDistance","HookRun","HookAlternateAim","HookReload","HookSelect","HookPadFrame","HookPadHeld","HookPadPressed","HookPadAxis","HookMelee","HookMenuFrame","HookRawPressed","HookRawHeld","HookRawReleased","HookPadReleased","HookMenuDevice","HookTitlePrompt","Controls","ValidateEngine"};uintptr_t offsets[24]{};
        for(size_t i=0;i<24;++i){auto p=GetProcAddress(local,names[i]);if(!p){FreeLibrary(local);return false;}offsets[i]=reinterpret_cast<uintptr_t>(p)-reinterpret_cast<uintptr_t>(local);}
        FreeLibrary(local);
        size_t bytes=(wcslen(path)+1)*sizeof(wchar_t);void* argument=VirtualAllocEx(h,nullptr,bytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        uintptr_t loader=remoteLoadLibrary(pid);
        if(!argument||!loader||!writeData(h,reinterpret_cast<uintptr_t>(argument),path,bytes)){if(argument)VirtualFreeEx(h,argument,0,MEM_RELEASE);return false;}
        Handle thread(CreateRemoteThread(h,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(loader),argument,0,nullptr));
        if(!thread.h){VirtualFreeEx(h,argument,0,MEM_RELEASE);return false;}
        if(WaitForSingleObject(thread,10000)!=WAIT_OBJECT_0){puts("Controls library load is taking too long. Restart Cold Fear before retrying.");return false;}
        DWORD module=0;GetExitCodeThread(thread,&module);VirtualFreeEx(h,argument,0,MEM_RELEASE);
        if(!module){puts("Cold Fear could not load the controls library.");return false;}
        state=module+offsets[22];ModernState configuration{};configuration.base=static_cast<uint32_t>(t.base);configuration.sensitivity=sensitivity;
        configuration.explorationDistance=distance;configuration.transitionSeconds=transition;configuration.modernMode=modern;configuration.holdShift=shift;
        configuration.returnSeconds=sceneReturn;configuration.resumeElapsed=sceneReturn;configuration.rReload=reload;
        configuration.snapAfterDoors=pad.snapAfterDoors;
        configuration.controllerEnabled=pad.controllerEnabled;configuration.padIndex=pad.padIndex;
        configuration.moveDeadzone=pad.moveDeadzone;configuration.lookDeadzone=pad.lookDeadzone;
        configuration.lookSpeed=pad.lookSpeed;configuration.aimScale=pad.aimScale;
        configuration.backMenuFrame=static_cast<uint32_t>(sites[15]+sizes[15]);
        configuration.backRawPressed=static_cast<uint32_t>(sites[16]+sizes[16]);configuration.backRawHeld=static_cast<uint32_t>(sites[17]+sizes[17]);
        configuration.backRawReleased=static_cast<uint32_t>(sites[18]+sizes[18]);configuration.backPadReleased=static_cast<uint32_t>(sites[19]+sizes[19]);
        configuration.backMelee=static_cast<uint32_t>(sites[14]+sizes[14]);
        configuration.backPadFrame=static_cast<uint32_t>(sites[10]+sizes[10]);configuration.backPadHeld=static_cast<uint32_t>(sites[11]+sizes[11]);
        configuration.backPadPressed=static_cast<uint32_t>(sites[12]+sizes[12]);configuration.backPadAxis=static_cast<uint32_t>(sites[13]+sizes[13]);
        configuration.backInput=static_cast<uint32_t>(sites[0]+6);configuration.backBasis=static_cast<uint32_t>(sites[1]+6);
        configuration.backPitch=static_cast<uint32_t>(sites[2]+6);configuration.backYaw=static_cast<uint32_t>(sites[3]+6);
        configuration.backFacing=static_cast<uint32_t>(sites[4]+6);
        configuration.backDistance=static_cast<uint32_t>(sites[5]+6);configuration.backRun=static_cast<uint32_t>(sites[6]+6);configuration.backAlternateAim=static_cast<uint32_t>(sites[7]+6);configuration.backReload=static_cast<uint32_t>(sites[8]+6);
        if(!writeData(h,state,&configuration,sizeof(configuration)))return false;
        Handle validation(CreateRemoteThread(h,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(module+offsets[23]),nullptr,0,nullptr));
        DWORD verified=0;
        if(!validation.h||WaitForSingleObject(validation,10000)!=WAIT_OBJECT_0||!GetExitCodeThread(validation,&verified)||verified!=1){puts("Native camera rotation convention did not match. Controls were not installed.");return false;}
        for(size_t i=0;i<22;++i){BYTE hook[9];memset(hook,0x90,sizeof(hook));hook[0]=0xe9;uint32_t relative=static_cast<uint32_t>(module+offsets[i]-sites[i]-5);memcpy(hook+1,&relative,4);
            installed=i+1;if(!writeCode(h,pid,sites[i],hook,sizes[i])){restore();return false;}}
        puts("Modern exploration controls attached: mouse camera, camera-relative WASD.");return true;
    }
};
static int watch(float distance,const wchar_t* dllPath,bool modern,float sensitivity,float transition,bool shift,float sceneReturn,bool reload,const ModernState& pad,bool confineCursor) {
    Target t=findGame(); if(!t.pid) return run("--status");
    wchar_t name[128]; eventName(name,128,L"Instance",t.pid);
    Handle mutex(CreateMutexW(nullptr,FALSE,name));
    if(!mutex.h||GetLastError()==ERROR_ALREADY_EXISTS) {puts("The F4 helper is already running. Use F4 in Cold Fear.");return 7;}
    int valid=run("--status"); if(valid) return valid;
    Handle h(OpenProcess(PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|SYNCHRONIZE,FALSE,t.pid));
    if(!h.h) return 3;
    eventName(name,128,L"Stop",t.pid); Handle stop(CreateEventW(nullptr,TRUE,FALSE,name));
    eventName(name,128,L"Toggle",t.pid); Handle toggle(CreateEventW(nullptr,FALSE,FALSE,name));
    if(!stop.h||!toggle.h) return 8;
    ModernLink controls;
    if(!controls.attach(h,t,dllPath,sensitivity,distance,transition,modern,shift,sceneReturn,reload,pad)){controls.restore();puts("Could not attach controls. Restart Cold Fear before retrying.");return 9;}
    bool enabled=true; int result=run("--enable",true);
    if(!result&&!controls.set(true))result=9;
    SetConsoleCtrlHandler(consoleEvent,TRUE);
    printf("\nF4: toggle farther exploration / original camera (while Cold Fear is focused).\nExploration back offset: %.0f cm. Aiming keeps the game's original offset.\nKeep this helper open. Ctrl+C or Disable.cmd restores the game.\n",distance);
    fflush(stdout);
    bool held=false,registered=false,warned=false;CursorConfinement<> cursor;
    while(!result&&!InterlockedCompareExchange(&stopping,0,0)&&WaitForSingleObject(stop,0)!=WAIT_OBJECT_0&&WaitForSingleObject(h,0)!=WAIT_OBJECT_0) {
        DWORD foreground=0; GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        if(foreground==t.pid&&!registered) {
            registered=RegisterHotKey(nullptr,1,MOD_NOREPEAT,VK_F4)!=0;
            if(!registered&&!warned) {puts("F4 registration unavailable; using key-state fallback.");warned=true;fflush(stdout);}
        } else if(foreground!=t.pid&&registered) {UnregisterHotKey(nullptr,1);registered=false;}
        bool down=foreground==t.pid&&(GetAsyncKeyState(VK_F4)&0x8000)!=0;
        bool requested=WaitForSingleObject(toggle,0)==WAIT_OBJECT_0;
        MSG message{};
        while(PeekMessageW(&message,nullptr,WM_HOTKEY,WM_HOTKEY,PM_REMOVE)) {
            if(message.wParam==1&&foreground==t.pid) requested=true;
        }
        if((!registered&&down&&!held)||requested) {
            if(enabled&&!controls.set(false)){result=9;break;}
            result=run(enabled?"--disable":"--enable",true);
            if(!result){enabled=!enabled;if(!controls.set(enabled))result=9;}
            fflush(stdout);
        }
        cursor.update(t.pid,enabled&&confineCursor);
        held=down; Sleep(10);
    }
    cursor.release();
    if(registered) UnregisterHotKey(nullptr,1);
    if(WaitForSingleObject(h,0)!=WAIT_OBJECT_0) {
        if(!controls.restore()){puts("Controls restore failed. Restart Cold Fear.");result=9;}
        int restored=run("--disable");
        if(restored) result=restored;
    }
    SetConsoleCtrlHandler(consoleEvent,FALSE);
    puts("F4 helper stopped.");return result;
}
int main(int argc,char** argv) {
    // Use physical pixels on mixed-DPI displays.
    using DpiContext=HANDLE(WINAPI*)(HANDLE);
    auto dpi=reinterpret_cast<DpiContext>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"SetThreadDpiAwarenessContext"));
    if(dpi){if(!dpi(reinterpret_cast<HANDLE>(-4)))dpi(reinterpret_cast<HANDLE>(-3));}
    else SetProcessDPIAware();
    puts("Cold Fear Third Person v0.6.14 (experimental)\n");
    const char* action=argc>1?argv[1]:"--enable";
    for(int i=2;i<argc;++i)if(!strcmp(argv[i],"--allow-session-update"))allowSessionUpdate=true;
    bool waitReady=false;
    for(int i=2;i<argc;++i)if(!strcmp(argv[i],"--wait-ready"))waitReady=true;
    if(waitReady&&!strcmp(action,"--enable")) {
        wchar_t log[MAX_PATH];GetModuleFileNameW(nullptr,log,MAX_PATH);wchar_t* slash=wcsrchr(log,L'\\');
        if(slash){wcscpy_s(slash+1,MAX_PATH-static_cast<size_t>(slash+1-log),L"ColdFearCamera.log");FILE* file=nullptr;_wfreopen_s(&file,log,L"a",stdout);}
        puts("Waiting for Cold Fear's renderer and input manager before attaching...");fflush(stdout);
        bool ready=false;
        for(unsigned attempt=0;attempt<300;++attempt){
            Target candidate=findGame();
            if(candidate.pid){
                Handle probe(OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_INFORMATION,FALSE,candidate.pid));
                uint32_t device=0,inputs=0;
                if(probe.h&&read(probe,candidate.base+0x49c438,&device,4)&&device&&
                   read(probe,candidate.base+0x47b8b4,&inputs,4)&&inputs){ready=true;break;}
            }
            Sleep(100);
        }
        if(!ready){puts("Game was not ready within 30 seconds. No camera hooks installed.");return 2;}
    }
    int result=0;
    if(!strcmp(action,"--enable")) {
        float distance=180.0f;bool confineCursor=true;
        float sensitivity=0.0025f,transition=0.2f,sceneReturn=0.2f;bool modern=true,shift=true,reload=true;ModernState pad{};pad.controllerEnabled=1;pad.padIndex=-1;pad.snapAfterDoors=1;pad.moveDeadzone=0.15f;pad.lookDeadzone=0.18f;pad.lookSpeed=2.617993878f;pad.aimScale=1;wchar_t dllPath[MAX_PATH]{};
        wchar_t settings[MAX_PATH]; DWORD length=GetModuleFileNameW(nullptr,settings,MAX_PATH);
        if(length&&length<MAX_PATH) {
            wchar_t* slash=wcsrchr(settings,L'\\');
            if(slash) {
                *(slash+1)=0;wcscpy_s(dllPath,settings);wcscat_s(dllPath,L"ColdFearModernControls.dll");wcscat_s(settings,L"Camera.ini");
                distance=static_cast<float>(GetPrivateProfileIntW(L"Camera",L"ExplorationDistanceCm",180,settings));
                modern=GetPrivateProfileIntW(L"Controls",L"ModernControls",1,settings)!=0;
                confineCursor=GetPrivateProfileIntW(L"Controls",L"ConfineCursor",1,settings)!=0;
                sensitivity=static_cast<float>(GetPrivateProfileIntW(L"Controls",L"MouseSensitivity",250,settings))*0.00001f;
                shift=GetPrivateProfileIntW(L"Controls",L"HoldShiftToRun",1,settings)!=0;
                wchar_t duration[64]{};GetPrivateProfileStringW(L"Camera",L"AimTransitionSeconds",L"0.2",duration,64,settings);
                wchar_t* durationEnd=nullptr;transition=wcstof(duration,&durationEnd);
                if(durationEnd==duration||*durationEnd||!std::isfinite(transition)||transition<0||transition>5){puts("AimTransitionSeconds must be a number from 0 to 5.");return 1;}
                reload=GetPrivateProfileIntW(L"Controls",L"RToReload",1,settings)!=0;
                GetPrivateProfileStringW(L"Camera",L"CutsceneReturnSeconds",L"0.2",duration,64,settings);
                sceneReturn=wcstof(duration,&durationEnd);
                if(durationEnd==duration||*durationEnd||!std::isfinite(sceneReturn)||sceneReturn<0||sceneReturn>5){puts("CutsceneReturnSeconds must be a number from 0 to 5.");return 1;}
                pad.snapAfterDoors=GetPrivateProfileIntW(L"Camera",L"SnapAfterDoors",1,settings)!=0;
                pad.controllerEnabled=GetPrivateProfileIntW(L"Controller",L"Enabled",1,settings)!=0;
                pad.padIndex=static_cast<int32_t>(GetPrivateProfileIntW(L"Controller",L"PlayerIndex",-1,settings));
                int moveDZ=GetPrivateProfileIntW(L"Controller",L"MoveDeadzonePercent",15,settings),lookDZ=GetPrivateProfileIntW(L"Controller",L"LookDeadzonePercent",18,settings);
                int speed=GetPrivateProfileIntW(L"Controller",L"LookDegreesPerSecond",150,settings),aimScale=GetPrivateProfileIntW(L"Controller",L"AimSensitivityPercent",100,settings);
                if(pad.padIndex< -1||pad.padIndex>3||moveDZ<0||moveDZ>50||lookDZ<0||lookDZ>50||speed<30||speed>720||aimScale<10||aimScale>300){puts("Invalid Controller settings. See Camera.ini / README.");return 1;}
                pad.moveDeadzone=moveDZ*0.01f;pad.lookDeadzone=lookDZ*0.01f;pad.lookSpeed=speed*0.01745329252f;pad.aimScale=aimScale*0.01f;
                wchar_t overridePath[MAX_PATH]{};GetPrivateProfileStringW(L"Controls",L"Library",L"",overridePath,MAX_PATH,settings);
                if(*overridePath)wcscpy_s(dllPath,overridePath);
            }
        }
        if(argc>2&&argv[2][0]!='-') {char* end=nullptr; distance=strtof(argv[2],&end); if(!end||*end||!std::isfinite(distance)||distance<60||distance>350) {puts("Distance must be between 60 and 350 cm.");return 1;}}
        if(!std::isfinite(distance)||distance<60||distance>350) {puts("Camera.ini distance must be between 60 and 350 cm.");return 1;}
        if(sensitivity<0.0001f||sensitivity>0.02f){puts("MouseSensitivity must be between 10 and 2000.");return 1;}
        result=watch(distance,dllPath,modern,sensitivity,transition,shift,sceneReturn,reload,pad,confineCursor);
    } else if(!strcmp(action,"--disable")||!strcmp(action,"--toggle")) {
        Target t=findGame(); wchar_t name[128]; eventName(name,128,!strcmp(action,"--toggle")?L"Toggle":L"Stop",t.pid);
        Handle event(OpenEventW(EVENT_MODIFY_STATE,FALSE,name));
        if(event.h) {SetEvent(event); puts("Request sent to F4 helper.");}
        else if(!strcmp(action,"--disable")) result=run(action);
        else {puts("Start the F4 helper first.");result=2;}
    } else if(!strcmp(action,"--status")) result=run(action);
    else {puts("Usage: ColdFearThirdPerson.exe [--enable [distance-cm] | --disable | --status | --toggle]");result=1;}
    if(argc==1&&result) {puts("\nPress Enter to close.");getchar();}
    return result;
}
