#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cmath>
#include <cstring>
#include "ModernControls.h"
#pragma comment(lib,"user32.lib")
extern "C" { ModernState Controls{}; }
static constexpr float Pi=3.14159265358979323846f;
static float& f(BYTE* p,size_t offset) {return *reinterpret_cast<float*>(p+offset);}
static float wrap(float a) {if(!std::isfinite(a))return 0;return remainderf(a,2*Pi);}
#ifdef CONTROLS_TEST
static bool testKeys[256]{};
static bool testFocused=true;
static bool focused() {return testFocused;}
static bool down(int key) {return testKeys[key];}
#else
static bool focused() {DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);return pid==GetCurrentProcessId();}
static bool down(int key){return (GetAsyncKeyState(key)&0x8000)!=0;}
#endif
#include "Controller.h"
extern "C" void __cdecl modernPadFrame(){} // Sampling occurs in the global input update.
extern "C" void __cdecl modernMenuFrame(){
    ++Controls.menuFrames;Controller::poll();

}
extern "C" uint32_t __cdecl modernPadQuery(unsigned kind,unsigned action,uintptr_t caller){return Controller::query(kind,action,caller);}
static bool sceneLocked(BYTE* hero) {
    bool scriptInput=*reinterpret_cast<uint32_t*>(hero+0x15f4)==1;
    if(!scriptInput)Controls.scriptCamera=0;
    else {
        BYTE* cameras=*reinterpret_cast<BYTE**>(Controls.base+0x3d96a0);
        // Whale sequence: 6 -> 12 -> 11; doors use state 4.
        if(cameras&&*reinterpret_cast<uint32_t*>(cameras+8)==6)Controls.scriptCamera=1;
    }
    // Interaction locks do not imply cinematic ownership.
    return Controls.scriptCamera||hero[0x16dd]||*reinterpret_cast<BYTE*>(Controls.base+0x47f9f2)!=0||
        *reinterpret_cast<uint32_t*>(Controls.base+0x419f0c)!=0;
}
static bool inputLocked(BYTE* hero) {
    return sceneLocked(hero)||*reinterpret_cast<uint32_t*>(hero+0x15f4)!=0;
}
static bool refreshScene(BYTE* hero) {
    bool locked=sceneLocked(hero);
    bool interaction=*reinterpret_cast<uint32_t*>(hero+0x15f4)!=0;
    bool changed=Controls.cameraHero!=reinterpret_cast<uint32_t>(hero);
    Controls.cameraHero=reinterpret_cast<uint32_t>(hero);
    if(locked||!Controls.enabled||!Controls.snapAfterDoors) {
        Controls.doorLocked=Controls.doorSnapPending=0;
    } else {
        if(changed||(Controls.doorLocked&&!interaction)) {
            Controls.doorSnapPending=1;Controls.initialized=0;
        }
        Controls.doorLocked=interaction;
    }
    if(locked) {
        Controls.scene=1;Controls.returnPending=0;Controls.resumeElapsed=0;
        Controls.active=0;Controls.initialized=0;Controls.distanceInitialized=0;
    } else if(Controls.scene) {
        Controls.scene=0;Controls.returnPending=1;
        Controls.initialized=0;Controls.distanceInitialized=0;
    }
    return locked;
}
extern "C" unsigned __cdecl modernReload(BYTE* hero,unsigned original) {
    bool held=down('R'),pressed=held&&!Controls.reloadHeld;Controls.reloadHeld=held;
    if(Controls.enabled&&Controls.modernMode&&Controls.rReload&&focused()&&!inputLocked(hero))return (original&255)||pressed;
    return original&255;
}
extern "C" void __cdecl modernSelect(BYTE* hero) {
    bool locked=refreshScene(hero);
    unsigned slot=hero[0x16dd]?3:hero[0x9ca]==0?1:*reinterpret_cast<uint32_t*>(hero+0x16b8)?0:2;
    bool useModern=Controls.enabled&&!locked;
    if(useModern)slot=1;
    BYTE* manager=reinterpret_cast<BYTE*>(Controls.base+0x3d9690);
    float saved=f(manager,0xa0);
    bool returning=useModern&&Controls.returnPending;
    bool snapping=useModern&&Controls.snapAfterDoors&&Controls.doorSnapPending==2;
    if(snapping)f(manager,0xa0)=0;
    else if(returning)f(manager,0xa0)=Controls.returnSeconds;
#ifdef CONTROLS_TEST
    // Emulate the native transition-duration capture.
    *reinterpret_cast<uint32_t*>(manager+0x14)=slot;
    f(manager,0x40)=f(manager,0xa0);
#else
    using Select=void(__thiscall*)(BYTE*,unsigned);
    reinterpret_cast<Select>(Controls.base+0x145fb0)(manager,slot);
#endif
    f(manager,0xa0)=saved;
    if(snapping) {
        manager[0x3c]=manager[0x3d]=0;f(manager,0x38)=f(manager,0x40)=0;
        Controls.doorSnapPending=0;
    }
    if(returning)Controls.returnPending=0;
    *reinterpret_cast<uint32_t*>(hero+0x16bc)=*reinterpret_cast<uint32_t*>(hero+0x16b8);
}
static void updateDistance(BYTE* hero,bool aim,bool reset) {
    float target=aim?f(hero,0xbc4):Controls.explorationDistance;
    if(reset||!Controls.distanceInitialized) {
        Controls.distanceCurrent=target;Controls.blendStart=target;
        Controls.blendElapsed=Controls.transitionSeconds;
        Controls.distanceInitialized=1;Controls.distanceAim=aim;return;
    }
    if((Controls.distanceAim!=0)!=aim) {
        Controls.distanceAim=aim;Controls.blendStart=Controls.distanceCurrent;Controls.blendElapsed=0;
    } else Controls.blendElapsed+=f(hero,0x1ac);
    float t=Controls.transitionSeconds>0?Controls.blendElapsed/Controls.transitionSeconds:1;
    if(t>1)t=1;
    // Smoothstep preserves continuous reversals.
    float eased=t*t*(3-2*t);
    Controls.distanceCurrent=Controls.blendStart+(target-Controls.blendStart)*eased;
}
extern "C" uint32_t __cdecl modernDistance(BYTE* hero) {
    float distance=f(hero,0xbc4);
    if(Controls.enabled&&Controls.snapAfterDoors&&Controls.doorSnapPending&&!sceneLocked(hero)) {
        if(Controls.doorSnapPending==1) {
            BYTE* cameras=*reinterpret_cast<BYTE**>(Controls.base+0x3d96a0);
            if(cameras) {
                BYTE* camera=cameras+0x2d0;
                // Reset the position sentinel and collision interpolation after doors.
                *reinterpret_cast<uint32_t*>(camera+0xb8)=0x7f7fffff;
                camera[0xf8]=0;
                Controls.doorSnapPending=2;++Controls.doorSnapCount;
            }
        }
        if(hero[0x9ca])distance=Controls.explorationDistance;
        uint32_t bits;memcpy(&bits,&distance,4);return bits;
    }
    if(Controls.enabled&&!sceneLocked(hero)&&Controls.distanceInitialized&&Controls.hero==reinterpret_cast<uint32_t>(hero))distance=Controls.distanceCurrent;
    uint32_t bits;memcpy(&bits,&distance,4);return bits;
}
extern "C" unsigned __cdecl modernRun(BYTE* hero,unsigned original) {
    if(Controls.padActive&&Controls.controllerEnabled)return original&255;
    if(Controls.enabled&&Controls.modernMode&&Controls.holdShift&&focused()&&hero[0x9ca]&&!inputLocked(hero))return down(VK_SHIFT)?1:0;
    return original&255;
}
extern "C" unsigned __cdecl modernAlternateAim(BYTE* hero,unsigned original) {
    // Consume alternate aim when Shift runs.
    if(Controls.enabled&&Controls.modernMode&&Controls.holdShift&&focused()&&hero[0x9ca]&&!inputLocked(hero)&&down(VK_SHIFT))return 0;
    return original&255;
}
static void rotation(float* matrix,float yaw) {
    // Match the native Euler convention.
    float c=cosf(yaw),s=sinf(yaw);
    float m[]={c,-s,0,s,c,0,0,0,1};memcpy(matrix,m,sizeof(m));
}
static float bodyYaw(BYTE* hero) {return atan2f(f(hero,0x1c0),f(hero,0x1b4));}
static void faceHeading(BYTE* hero,float yaw) {
    // Preserve body tilt during world-Z rotation.
    float delta=wrap(yaw-bodyYaw(hero)),c=cosf(delta),s=sinf(delta);
    float* matrix=reinterpret_cast<float*>(hero+0x1b4);
    for(int column=0;column<3;++column){float a=matrix[column],b=matrix[3+column];matrix[column]=c*a-s*b;matrix[3+column]=s*a+c*b;}
}
static void modernCombat(BYTE* hero) {
    Controls.meleeRequest=0;
    if(!hero[0x9ca]||hero[0x936]||hero[0x937])return;
    // Request melee after native interaction processing.
    bool pressed=Controls.padActive?(Controls.padButtons&0x20000)&&!(Controls.padPrevious&0x20000):
        (Controls.keyButtons&2)&&!(Controls.keyPrevious&2);
    hero[0x9c2]=hero[0x9c7]=hero[0x9c8]=0;
    if(pressed){hero[0x9c3]=1;Controls.meleeRequest=1;}
}
extern "C" unsigned __cdecl modernMeleeMode(BYTE* hero) {
    if(Controls.enabled&&Controls.modernMode&&Controls.meleeRequest&&focused()&&!inputLocked(hero)&&!hero[0x94f]) {
        bool ready=false;
#ifdef CONTROLS_TEST
        ready=*reinterpret_cast<int32_t*>(hero+0x9d4)==-1&&*reinterpret_cast<int32_t*>(hero+0x9d8)==-1&&
            hero[0x9c3]&&!hero[0x9c2]&&!hero[0x936]&&!hero[0x937];
#else
        using CanStrike=bool(__thiscall*)(BYTE*);
        ready=reinterpret_cast<CanStrike>(Controls.base+0xb28b0)(hero);
#endif
        if(ready){++Controls.meleeCount;return 0;}
    }
    return hero[0x9ca];
}
extern "C" void __cdecl modernInput(BYTE* hero) {
    ++Controls.frames;
    Controls.meleeRequest=0;
    bool newHero=Controls.hero!=reinterpret_cast<uint32_t>(hero);
    if(newHero)Controls.scriptCamera=0;
    Controls.hero=reinterpret_cast<uint32_t>(hero);
    bool aim=hero[0x9ca]==0;
    bool locked=refreshScene(hero);
    bool allowed=Controls.enabled&&focused()&&!inputLocked(hero)&&f(hero,0x1ac)>0&&f(hero,0x1ac)<0.2f;
    if(!allowed) {
        // Retain the camera through interaction locks and focus loss.
        if(Controls.enabled&&!newHero&&!locked)return;
        Controls.active=0;Controls.initialized=0;Controls.distanceInitialized=0;Controls.wasAim=aim;return;
    }
    updateDistance(hero,aim,newHero||Controls.doorSnapPending!=0);
    if(!Controls.modernMode){Controls.active=0;Controls.initialized=0;Controls.wasAim=aim;return;}
    modernCombat(hero);
    if(aim) {
        if(Controls.active&&Controls.initialized&&!newHero) {
            // Align the character with the camera on aim entry.
            faceHeading(hero,Controls.yaw);
            f(hero,0xacc)=0;f(hero,0xad0)=Controls.pitch;
        }
        Controls.yaw=wrap(bodyYaw(hero)+f(hero,0xacc));
        Controls.pitch=f(hero,0xad0);
        Controls.active=0;Controls.wasAim=1;Controls.initialized=1;return;
    }
    if(newHero||!Controls.initialized||Controls.wasAim) {
        Controls.yaw=wrap(bodyYaw(hero)+f(hero,0xacc));
        Controls.pitch=f(hero,0xad0);
        Controls.initialized=1;
    }
    if(!std::isfinite(Controls.pitch))Controls.pitch=0;
    float dx=*reinterpret_cast<float*>(Controls.base+0x47b7a0);
    float dy=*reinterpret_cast<float*>(Controls.base+0x47b7a4);
    if(!std::isfinite(dx)||fabsf(dx)>1000)dx=0;
    if(!std::isfinite(dy)||fabsf(dy)>1000)dy=0;
    Controls.lastDx=dx;Controls.lastDy=dy;
    Controls.yaw=wrap(Controls.yaw-dx*Controls.sensitivity);
    Controls.pitch-=dy*Controls.sensitivity;
    if(Controls.padActive) {
        Controls.yaw=wrap(Controls.yaw-Controls.padRX*Controls.lookSpeed*f(hero,0x1ac));
        Controls.pitch+=Controls.padRY*Controls.lookSpeed*f(hero,0x1ac);
    }
    if(Controls.pitch>1.10f)Controls.pitch=1.10f;
    if(Controls.pitch<-0.85f)Controls.pitch=-0.85f;
    rotation(Controls.basis,Controls.yaw);
    float x=static_cast<float>(down('D'))-static_cast<float>(down('A'));
    float y=static_cast<float>(down('W'))-static_cast<float>(down('S'));
    if(x&&y){x*=0.70710678f;y*=0.70710678f;}
    if(Controls.padActive){x=Controls.padLX;y=Controls.padLY;}
    if(Controls.resumeElapsed<Controls.returnSeconds) {
        Controls.resumeElapsed+=f(hero,0x1ac);
        float t=Controls.resumeElapsed/Controls.returnSeconds;if(t>1)t=1;
        float weight=t*t*(3-2*t);x*=weight;y*=weight;
    }
    // Feed native movement and facing the same direction.
    f(hero,0x8d0)=x;f(hero,0x8d4)=y;
    f(hero,0x8c4)=x;f(hero,0x8c8)=y;
    Controls.active=1;Controls.wasAim=0;
}
extern "C" float* __cdecl modernBasis(BYTE* hero) {
    if(Controls.enabled&&Controls.active&&hero[0x9ca]&&!sceneLocked(hero)&&Controls.hero==reinterpret_cast<uint32_t>(hero))return Controls.basis;
    return reinterpret_cast<float*>(hero+0x1b4);
}
extern "C" DWORD WINAPI ValidateEngine(void*) {
    struct Euler {float x,y,z;DWORD order;};Euler angles{0,0,0.5f,0};
    float actual[9]{},expected[9]{};rotation(expected,0.5f);
    using MakeEuler=void(__thiscall*)(float*,Euler*);
    reinterpret_cast<MakeEuler>(Controls.base+0x66df0)(actual,&angles);
    for(int i=0;i<9;++i)if(fabsf(actual[i]-expected[i])>0.0001f)return 0;
    return 1;
}
// Preserve x87/SSE and registers; replace saved ECX for the basis.
extern "C" __declspec(naked) void HookInput() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        push ebx
        call modernInput
        add esp,4
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        mov eax,[ebx+0xc4c]
        jmp dword ptr [Controls+4]
    }
}
extern "C" __declspec(naked) void HookBasis() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        push ebx
        call modernBasis
        add esp,4
        mov [ebp+24],eax
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        jmp dword ptr [Controls+8]
    }
}
extern "C" __declspec(naked) void HookDistance() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        push ebx
        call modernDistance
        add esp,4
        mov [ebp+20],eax
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        jmp dword ptr [Controls+104]
    }
}
extern "C" __declspec(naked) void HookRun() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        push eax
        push ebx
        call modernRun
        add esp,8
        mov byte ptr [ebp+28],al
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        mov [ebx+0x9c1],al
        jmp dword ptr [Controls+108]
    }
}
extern "C" __declspec(naked) void HookReload() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        push eax
        push ebx
        call modernReload
        add esp,8
        mov byte ptr [ebp+28],al
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        test al,al
        mov [ebx+0x9c4],al
        jmp dword ptr [Controls+152]
    }
}
// Preserve selector cleanup and native cinematic slots.
extern "C" __declspec(naked) void HookSelect() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        push ebx
        call modernSelect
        add esp,4
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        pop edi
        pop esi
        pop ebx
        mov esp,ebp
        pop ebp
        ret
    }
}
extern "C" __declspec(naked) void HookAlternateAim() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        push eax
        push esi
        call modernAlternateAim
        add esp,8
        mov byte ptr [ebp+28],al
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        mov cl,[esi+0x94c]
        jmp dword ptr [Controls+112]
    }
}
extern "C" __declspec(naked) void HookMelee() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        push esi
        call modernMeleeMode
        add esp,4
        mov byte ptr [ebp+28],al
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        jmp dword ptr [Controls+260]
    }
}
// Restore movement-facing after native mouse input.
extern "C" __declspec(naked) void HookFacing() {
    __asm {
        pushfd
        cmp dword ptr [Controls+20],0
        je stock
        cmp dword ptr [Controls+40],0
        je stock
        cmp byte ptr [ebx+0x9ca],0
        je stock
        push eax
        mov eax,[ebx+0x8d0]
        mov [ebx+0x8c4],eax
        mov eax,[ebx+0x8d4]
        mov [ebx+0x8c8],eax
        pop eax
    stock:
        popfd
        mov al,[ebx+0x9ca]
        jmp dword ptr [Controls+100]
    }
}
extern "C" __declspec(naked) void HookPitch() {
    __asm {
        pushfd
        cmp dword ptr [Controls+20],0
        je stock
        cmp dword ptr [Controls+40],0
        je stock
        cmp byte ptr [ebx+0x9ca],0
        je stock
        popfd
        fld dword ptr [Controls+32]
        jmp dword ptr [Controls+12]
    stock:
        popfd
        fld dword ptr [ebx+0xad0]
        jmp dword ptr [Controls+12]
    }
}
extern "C" __declspec(naked) void HookYaw() {
    __asm {
        pushfd
        cmp dword ptr [Controls+20],0
        je stock
        cmp dword ptr [Controls+40],0
        je stock
        cmp byte ptr [ebx+0x9ca],0
        je stock
        popfd
        fldz
        jmp dword ptr [Controls+16]
    stock:
        popfd
        fld dword ptr [ebx+0xacc]
        jmp dword ptr [Controls+16]
    }
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH)DisableThreadLibraryCalls(instance);
    return TRUE;
}

extern "C" __declspec(naked) void HookPadFrame() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        call modernPadFrame
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        push ebp
        mov ebp,esp
        sub esp,0xe8
        jmp dword ptr [Controls+180]
    }
}
extern "C" __declspec(naked) void HookPadHeld() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        push dword ptr [ebp+36]
        push dword ptr [ebp+40]
        push 0
        call modernPadQuery
        add esp,12
        mov [ebp+28],eax
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        cmp eax,0xffffffff
        je nativePath
        ret 4
    nativePath:
        push ebp
        mov ebp,esp
        push esi
        mov esi,[ebp+8]
        jmp dword ptr [Controls+184]
    }
}
extern "C" __declspec(naked) void HookPadPressed() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        push dword ptr [ebp+36]
        push dword ptr [ebp+40]
        push 1
        call modernPadQuery
        add esp,12
        mov [ebp+28],eax
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        cmp eax,0xffffffff
        je nativePath
        ret 4
    nativePath:
        push ebp
        mov ebp,esp
        push esi
        mov esi,[ebp+8]
        jmp dword ptr [Controls+188]
    }
}
extern "C" __declspec(naked) void HookPadAxis() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        push dword ptr [ebp+36]
        push dword ptr [ebp+40]
        push 2
        call modernPadQuery
        add esp,12
        mov [ebp+28],eax
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        cmp eax,0xffffffff
        je nativePath
        push eax
        fld dword ptr [esp]
        add esp,4
        ret 4
    nativePath:
        push ebp
        mov ebp,esp
        push ecx
        push ebx
        push esi
        jmp dword ptr [Controls+192]
    }
}

extern "C" __declspec(naked) void HookMenuFrame() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        call modernMenuFrame
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        mov eax,[esi+8]
        pop edi
        mov [esi+0xc],eax
        jmp dword ptr [Controls+280]
    }
}

extern "C" __declspec(naked) void HookRawPressed() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        push dword ptr [ebp+36]
        push dword ptr [ebp+40]
        push 3
        call modernPadQuery
        add esp,12
        mov [ebp+28],eax
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        cmp eax,0xffffffff
        je nativePath
        ret 4
    nativePath:
        push ebp
        mov ebp,esp
        mov eax,[ebp+8]
        jmp dword ptr [Controls+284]
    }
}

extern "C" __declspec(naked) void HookRawHeld() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        push dword ptr [ebp+36]
        push dword ptr [ebp+40]
        push 4
        call modernPadQuery
        add esp,12
        mov [ebp+28],eax
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        cmp eax,0xffffffff
        je nativePath
        ret 4
    nativePath:
        push ebp
        mov ebp,esp
        mov eax,[ebp+8]
        jmp dword ptr [Controls+288]
    }
}

extern "C" __declspec(naked) void HookRawReleased() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        push dword ptr [ebp+36]
        push dword ptr [ebp+40]
        push 5
        call modernPadQuery
        add esp,12
        mov [ebp+28],eax
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        cmp eax,0xffffffff
        je nativePath
        ret 4
    nativePath:
        push ebp
        mov ebp,esp
        mov eax,[ebp+8]
        jmp dword ptr [Controls+292]
    }
}

extern "C" __declspec(naked) void HookPadReleased() {
    __asm {
        pushfd
        pushad
        mov ebp,esp
        sub esp,528
        and esp,0xfffffff0
        fxsave [esp]
        fninit
        push dword ptr [ebp+36]
        push dword ptr [ebp+40]
        push 6
        call modernPadQuery
        add esp,12
        mov [ebp+28],eax
        fxrstor [esp]
        mov esp,ebp
        popad
        popfd
        cmp eax,0xffffffff
        je nativePath
        ret 4
    nativePath:
        push ebp
        mov ebp,esp
        push esi
        mov esi,[ebp+8]
        jmp dword ptr [Controls+296]
    }
}

extern "C" uint32_t __cdecl modernMenuDevice(BYTE* input,uintptr_t caller){
    return Controller::menuDevice(*reinterpret_cast<uint32_t*>(input+8),caller);
}
extern "C" __declspec(naked) void HookMenuDevice(){__asm{
    pushfd
    pushad
    mov ebp,esp
    sub esp,528
    and esp,0xfffffff0
    fxsave [esp]
    fninit
    push dword ptr [ebp+36]
    push ecx
    call modernMenuDevice
    add esp,8
    mov [ebp+28],eax
    fxrstor [esp]
    mov esp,ebp
    popad
    popfd
    ret
}}

#include "TitlePrompt.h"
extern "C" const wchar_t* __cdecl modernTitlePrompt(const wchar_t* original,int id,uintptr_t lookupFrame){
    static int titleId=-1;
    static wchar_t encoded[29]{};
    bool valid=false;
#ifndef CONTROLS_TEST
    using Lookup=int(__thiscall*)(void*,const char*);
    if(titleId<0&&*reinterpret_cast<uint32_t*>(Controls.base+0x47693c))
        titleId=reinterpret_cast<Lookup>(Controls.base+0x61730)(reinterpret_cast<void*>(Controls.base+0x476918),"STR_MN_GENERIC_PRESS_START");
#endif
#ifndef CONTROLS_TEST
    auto map=*reinterpret_cast<TitleGlyph**>(Controls.base+0x476924);
    unsigned count=*reinterpret_cast<unsigned*>(Controls.base+0x476920);
    if(map&&count>1&&count<65536)valid=encodeTitlePrompt(map,count,encoded);
#endif
    const wchar_t* result=titlePrompt(original,id,titleId,valid&&Controls.enabled&&Controls.controllerEnabled&&Controls.lastController,encoded);
#ifndef CONTROLS_TEST
    // Adjust the title draw frame only at its verified caller.
    if(id==titleId&&titleId>=0&&lookupFrame&&
       *reinterpret_cast<uintptr_t*>(lookupFrame+4)==Controls.base+0x61c02) {
        uintptr_t drawFrame=*reinterpret_cast<uintptr_t*>(lookupFrame);
        using Measure=float(__thiscall*)(void*,const wchar_t*);
        float width=reinterpret_cast<Measure>(Controls.base+0x61960)(reinterpret_cast<void*>(Controls.base+0x476918),result);
        float scale=*reinterpret_cast<float*>(Controls.base+0x41c44c);
        uint32_t pixels=*reinterpret_cast<uint32_t*>(Controls.base+0x4a2620);
        if(scale>0&&pixels>0&&pixels<32768&&width>=0&&width<32768)
            *reinterpret_cast<float*>(drawFrame+12)=centeredTitleX(pixels/scale,width);
    }
#else
    (void)lookupFrame;
#endif
    return result;
}
extern "C" __declspec(naked) void HookTitlePrompt(){__asm{
    lea eax,[eax+edx*2]
    pushfd
    pushad
    mov ebx,ebp
    mov ebp,esp
    sub esp,528
    and esp,0xfffffff0
    fxsave [esp]
    fninit
    push ebx
    push dword ptr [ebx+8]
    push eax
    call modernTitlePrompt
    add esp,12
    mov [ebp+28],eax
    fxrstor [esp]
    mov esp,ebp
    popad
    popfd
    pop ebp
    ret 4
}}
