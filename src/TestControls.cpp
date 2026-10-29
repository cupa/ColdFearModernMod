#define CONTROLS_TEST
#include "ModernControls.cpp"
#include <cstdio>
#include <cstdlib>
static void require(bool value,const char* message){if(!value){printf("FAIL: %s\n",message);exit(1);}}
static bool closeEnough(float a,float b){return fabsf(a-b)<0.0001f;}
extern "C" __declspec(naked) void returnFromHook(){__asm{ret}}
extern "C" __declspec(naked) void invokeSelector(BYTE*) {
    __asm {
        push ebp
        mov ebp,esp
        push ebx
        push esi
        push edi
        mov ebx,[ebp+8]
        jmp HookSelect
    }
}
int main(){
    BYTE hero[8192]{};
    BYTE* globals=static_cast<BYTE*>(VirtualAlloc(nullptr,0x480000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));require(globals!=nullptr,"allocate globals");
    Controls={};Controls.base=reinterpret_cast<uint32_t>(globals);Controls.enabled=1;Controls.sensitivity=0.0025f;
    Controls.modernMode=1;Controls.holdShift=1;Controls.explorationDistance=180;Controls.transitionSeconds=0.2f;f(hero,0xbc4)=45;
    Controls.rReload=1;Controls.returnSeconds=0.2f;Controls.resumeElapsed=0.2f;
    rotation(reinterpret_cast<float*>(hero+0x1b4),0);f(hero,0x1ac)=0.016f;hero[0x9ca]=1;
    *reinterpret_cast<float*>(globals+0x47b7a0)=10;*reinterpret_cast<float*>(globals+0x47b7a4)=5;
    modernInput(hero);
    require(closeEnough(Controls.yaw,-0.025f)&&closeEnough(Controls.pitch,-0.0125f),"mouse yaw/pitch independent of body");
    require(closeEnough(bodyYaw(hero),0),"idle mouse does not turn character");
    require(closeEnough(f(hero,0x8d0),0)&&closeEnough(f(hero,0x8d4),0),"idle does not move");
    require(modernBasis(hero)==Controls.basis,"exploration camera uses independent basis");
    *reinterpret_cast<float*>(globals+0x47b7a0)=0;*reinterpret_cast<float*>(globals+0x47b7a4)=0;
    const char keys[]={'W','A','S','D'};const float xs[]={0,-1,0,1},ys[]={1,0,-1,0};
    for(int i=0;i<4;++i){memset(testKeys,0,sizeof(testKeys));testKeys[static_cast<unsigned char>(keys[i])]=true;modernInput(hero);
        require(closeEnough(f(hero,0x8d0),xs[i])&&closeEnough(f(hero,0x8d4),ys[i]),"cardinal WASD movement");
        require(closeEnough(f(hero,0x8c4),xs[i])&&closeEnough(f(hero,0x8c8),ys[i]),"facing follows movement input");}
    testKeys['W']=testKeys['D']=true;modernInput(hero);
    require(closeEnough(f(hero,0x8d0)*f(hero,0x8d0)+f(hero,0x8d4)*f(hero,0x8d4),1),"diagonal speed normalized");
    float heading=Controls.yaw,pitch=Controls.pitch;hero[0x9ca]=0;modernInput(hero);
    require(closeEnough(bodyYaw(hero),heading)&&closeEnough(f(hero,0xacc),0)&&closeEnough(f(hero,0xad0),pitch),"enter aim along view heading");
    require(modernBasis(hero)==reinterpret_cast<float*>(hero+0x1b4),"aim uses native camera basis");
    Controls.enabled=0;f(hero,0x8d0)=0.125f;modernInput(hero);require(closeEnough(f(hero,0x8d0),0.125f),"disabled leaves native inputs untouched");
    // Check hook ABI with live FPU state.
    Controls.backInput=reinterpret_cast<uint32_t>(&returnFromHook);BYTE* actor=hero;DWORD ecxAfter=0,eaxAfter=0,flags=0;float fpuBefore=3.5f,fpuAfter=0;
    *reinterpret_cast<DWORD*>(hero+0xc4c)=0x3e800000;
    __asm {
        push ebx
        mov ebx,actor
        mov ecx,12345678h
        fld fpuBefore
        stc
        call HookInput
        mov ecxAfter,ecx
        mov eaxAfter,eax
        pushfd
        pop flags
        fstp fpuAfter
        pop ebx
    }
    require(ecxAfter==0x12345678&&eaxAfter==0x3e800000&&(flags&1)&&closeEnough(fpuBefore,fpuAfter),"hook register, flags and x87 preservation");
    Controls.enabled=1;Controls.active=1;Controls.hero=reinterpret_cast<uint32_t>(hero);hero[0x9ca]=1;
    Controls.backBasis=reinterpret_cast<uint32_t>(&returnFromHook);Controls.backPitch=Controls.backBasis;Controls.backYaw=Controls.backBasis;
    DWORD basisAfter=0;float pitchAfter=0,yawAfter=1;Controls.pitch=0.4f;
    __asm {
        push ebx
        push esi
        mov ebx,actor
        call HookBasis
        mov basisAfter,ecx
        call HookPitch
        fstp pitchAfter
        call HookYaw
        fstp yawAfter
        pop esi
        pop ebx
    }
    require(basisAfter==reinterpret_cast<uint32_t>(Controls.basis)&&closeEnough(pitchAfter,0.4f)&&closeEnough(yawAfter,0),"independent exploration basis/pitch/yaw hooks");
    Controls.backFacing=reinterpret_cast<uint32_t>(&returnFromHook);
    f(hero,0x8d0)=-1;f(hero,0x8d4)=0;f(hero,0x8c4)=0.25f;f(hero,0x8c8)=0.5f;
    __asm {
        push ebx
        mov ebx,actor
        mov eax,12345678h
        stc
        call HookFacing
        mov eaxAfter,eax
        pushfd
        pop flags
        pop ebx
    }
    require(closeEnough(f(hero,0x8c4),-1)&&closeEnough(f(hero,0x8c8),0)&&eaxAfter==0x12345601&&(flags&1),"late facing hook repairs native mouse overwrite and preserves registers/flags");
    hero[0x9ca]=0;f(hero,0xad0)=0.2f;f(hero,0xacc)=-0.3f;
    __asm {
        push ebx
        mov ebx,actor
        call HookPitch
        fstp pitchAfter
        call HookYaw
        fstp yawAfter
        pop ebx
    }
    require(closeEnough(pitchAfter,0.2f)&&closeEnough(yawAfter,-0.3f),"aim reads native camera angles");
    // Check endpoints, reversal and zero-duration transitions.
    Controls.distanceInitialized=0;hero[0x9ca]=1;f(hero,0x1ac)=0.05f;
    updateDistance(hero,false,false);require(closeEnough(Controls.distanceCurrent,180),"initial exploration distance");
    updateDistance(hero,true,false);require(closeEnough(Controls.distanceCurrent,180),"aim starts without a position jump");
    updateDistance(hero,true,false);require(closeEnough(Controls.distanceCurrent,158.90625f),"quarter-duration smoothstep");
    updateDistance(hero,true,false);require(closeEnough(Controls.distanceCurrent,112.5f),"half-duration blend");
    updateDistance(hero,false,false);require(closeEnough(Controls.distanceCurrent,112.5f),"rapid aim reversal is continuous");
    for(int i=0;i<4;++i)updateDistance(hero,false,false);
    require(closeEnough(Controls.distanceCurrent,180),"release finishes at exploration offset");
    updateDistance(hero,true,false);for(int i=0;i<4;++i)updateDistance(hero,true,false);
    require(closeEnough(Controls.distanceCurrent,45),"aim finishes at original offset after configured duration");
    Controls.transitionSeconds=0;updateDistance(hero,false,false);require(closeEnough(Controls.distanceCurrent,180),"zero duration instant exploration");
    updateDistance(hero,true,false);require(closeEnough(Controls.distanceCurrent,45),"zero duration instant aim");
    Controls.transitionSeconds=0.2f;hero[0x9ca]=1;memset(testKeys,0,sizeof(testKeys));
    testKeys[VK_SHIFT]=true;require(modernRun(hero,0)==1&&modernAlternateAim(hero,1)==0,"Shift requests native run and consumes alternate aim");
    testKeys[VK_SHIFT]=false;require(modernRun(hero,1)==0&&modernAlternateAim(hero,1)==1,"release Shift stops running");
    hero[0x9ca]=0;testKeys[VK_SHIFT]=true;require(modernRun(hero,0)==0&&modernAlternateAim(hero,1)==1,"aim leaves native controls intact");
    hero[0x9ca]=1;Controls.enabled=0;require(modernRun(hero,0)==0&&modernAlternateAim(hero,1)==1,"F4 off restores stock bindings");
    Controls.enabled=1;Controls.distanceCurrent=123;Controls.distanceInitialized=1;
    Controls.backDistance=reinterpret_cast<uint32_t>(&returnFromHook);Controls.backRun=Controls.backDistance;Controls.backAlternateAim=Controls.backDistance;
    DWORD edxAfter=0;hero[0x94c]=7;
    __asm {
        push ebx
        push esi
        mov ebx,actor
        fld fpuBefore
        stc
        call HookDistance
        mov edxAfter,edx
        fstp fpuAfter
        pushfd
        pop flags
        mov eax,12345600h
        call HookRun
        mov eaxAfter,eax
        mov esi,actor
        mov eax,12345601h
        mov ecx,87654321h
        call HookAlternateAim
        mov ecxAfter,ecx
        pop esi
        pop ebx
    }
    float actualDistance;memcpy(&actualDistance,&edxAfter,4);
    require(closeEnough(actualDistance,123)&&closeEnough(fpuBefore,fpuAfter)&&(flags&1),"distance hook supplies blend while preserving x87/flags");
    require(hero[0x9c1]==1&&eaxAfter==0x12345601&&ecxAfter==0x87654307,"run and alternate aim hooks preserve registers and original instructions");
    memset(testKeys,0,sizeof(testKeys));Controls.reloadHeld=0;testKeys['R']=true;
    require(modernReload(hero,0)==1&&modernReload(hero,0)==0,"R reload is a single press, without repeated held input");
    testKeys['R']=false;modernReload(hero,0);testKeys['R']=true;hero[0x9ca]=0;
    require(modernReload(hero,0)==1,"R reload works while aiming");
    testKeys['R']=false;require(modernReload(hero,1)==1,"original reload binding remains usable");
    hero[0x9ca]=1;Controls.reloadHeld=0;testKeys['R']=true;Controls.backReload=reinterpret_cast<uint32_t>(&returnFromHook);
    __asm {
        push ebx
        mov ebx,actor
        mov eax,12345600h
        test al,al
        call HookReload
        mov eaxAfter,eax
        pushfd
        pop flags
        pop ebx
    }
    require(hero[0x9c4]==1&&eaxAfter==0x12345601&&!(flags&0x40),"reload hook updates native request and branch flags");
    BYTE* manager=globals+0x3d9690;f(manager,0xa0)=0.75f;
    modernSelect(hero);require(*reinterpret_cast<uint32_t*>(manager+0x14)==1,"modern selector uses shoulder camera");
    *reinterpret_cast<uint32_t*>(hero+0x15f4)=1;
    float doorYaw=Controls.yaw,doorDistance=Controls.distanceCurrent;
    modernInput(hero);modernSelect(hero);
    require(!Controls.scene&&Controls.active&&*reinterpret_cast<uint32_t*>(manager+0x14)==1&&closeEnough(doorYaw,Controls.yaw)&&closeEnough(doorDistance,Controls.distanceCurrent),"door input lock retains modern camera pose and distance without a return transition");
    Controls.reloadHeld=0;require(modernReload(hero,0)==0&&modernRun(hero,0)==0,"door retains native input lock");
    *reinterpret_cast<uint32_t*>(hero+0x15f4)=0;testFocused=false;
    modernInput(hero);require(Controls.active&&closeEnough(doorYaw,Controls.yaw)&&closeEnough(doorDistance,Controls.distanceCurrent),"lost focus retains last camera pose");
    testFocused=true;*reinterpret_cast<uint32_t*>(hero+0x15f4)=1;
    *reinterpret_cast<BYTE*>(globals+0x47f9f2)=1;
    modernSelect(hero);require(Controls.scene&&Controls.active==0&&*reinterpret_cast<uint32_t*>(manager+0x14)==2,"script lock restores native exploration camera");
    require(modernBasis(hero)==reinterpret_cast<float*>(hero+0x1b4)&&modernDistance(hero)==0x42340000,"script lock restores native basis and distance");
    Controls.reloadHeld=0;require(modernReload(hero,0)==0&&modernRun(hero,0)==0,"script lock prevents modern input overrides");
    *reinterpret_cast<uint32_t*>(hero+0x16b8)=1;modernSelect(hero);require(*reinterpret_cast<uint32_t*>(manager+0x14)==0,"script lock preserves alternate native camera");
    hero[0x16dd]=1;modernSelect(hero);require(*reinterpret_cast<uint32_t*>(manager+0x14)==3,"special native camera retained");
    hero[0x16dd]=0;*reinterpret_cast<uint32_t*>(hero+0x16b8)=0;*reinterpret_cast<uint32_t*>(hero+0x15f4)=0;*reinterpret_cast<BYTE*>(globals+0x47f9f2)=0;
    memset(testKeys,0,sizeof(testKeys));testKeys['W']=true;f(hero,0x1ac)=0.05f;
    modernInput(hero);require(!Controls.scene&&Controls.returnPending&&f(hero,0x8d4)>0&&f(hero,0x8d4)<1,"movement eases back after script releases controls");
    invokeSelector(hero);
    require(*reinterpret_cast<uint32_t*>(manager+0x14)==1&&closeEnough(f(manager,0x40),0.2f)&&closeEnough(f(manager,0xa0),0.75f)&&!Controls.returnPending,"selector hook restores modern camera using configured native transition and preserves manager setting");
    for(int i=0;i<4;++i)modernInput(hero);
    require(closeEnough(f(hero,0x8d4),1),"modern movement reaches full input after return duration");
    BYTE nativeCameras[64]{};*reinterpret_cast<BYTE**>(manager+0x10)=nativeCameras;
    *reinterpret_cast<uint32_t*>(hero+0x15f4)=1;*reinterpret_cast<uint32_t*>(hero+0x16b8)=1;
    *reinterpret_cast<uint32_t*>(nativeCameras+8)=4;modernSelect(hero);
    require(!Controls.scene&&*reinterpret_cast<uint32_t*>(manager+0x14)==1,"recorded door camera state 4 does not release modern camera");
    *reinterpret_cast<uint32_t*>(nativeCameras+8)=6;modernSelect(hero);
    require(Controls.scriptCamera&&Controls.scene&&*reinterpret_cast<uint32_t*>(manager+0x14)==0,"whale scripted shot restores native fixed camera without cinematic flag");
    *reinterpret_cast<uint32_t*>(nativeCameras+8)=12;modernSelect(hero);
    require(Controls.scene&&*reinterpret_cast<uint32_t*>(manager+0x14)==0,"whale follow-up shot retains script ownership");
    *reinterpret_cast<uint32_t*>(nativeCameras+8)=11;modernSelect(hero);require(Controls.scene,"final whale shot remains native until script unlock");
    *reinterpret_cast<uint32_t*>(hero+0x15f4)=0;modernSelect(hero);
    require(!Controls.scene&&!Controls.scriptCamera&&*reinterpret_cast<uint32_t*>(manager+0x14)==1&&closeEnough(f(manager,0x40),0.2f),"whale unlock returns with configured camera easing");
    *reinterpret_cast<uint32_t*>(hero+0x16b8)=0;
    BYTE doorCameras[0xb40]{};*reinterpret_cast<BYTE**>(manager+0x10)=doorCameras;
    Controls.snapAfterDoors=1;Controls.cameraHero=reinterpret_cast<uint32_t>(hero);Controls.doorSnapPending=0;
    *reinterpret_cast<uint32_t*>(doorCameras+8)=4;*reinterpret_cast<uint32_t*>(hero+0x15f4)=1;
    modernSelect(hero);require(Controls.doorLocked&&!Controls.doorSnapPending&&!Controls.scene,"door lock is tracked without releasing modern camera");
    *reinterpret_cast<uint32_t*>(hero+0x15f4)=0;modernInput(hero);
    require(Controls.doorSnapPending==1&&!Controls.returnPending,"door unlock queues a snap instead of cinematic easing");
    doorCameras[0x2d0+0xf8]=1;
    require(modernDistance(hero)==0x43340000&&*reinterpret_cast<uint32_t*>(doorCameras+0x2d0+0xb8)==0x7f7fffff&&!doorCameras[0x2d0+0xf8],"door snap resets native target and collision interpolation history and supplies exploration distance");
    manager[0x3c]=1;modernSelect(hero);
    require(!Controls.doorSnapPending&&Controls.doorSnapCount==1&&!manager[0x3c]&&f(manager,0x40)==0&&closeEnough(f(manager,0xa0),0.75f),"door snap disables the stale view blend and preserves normal transition configuration");
    Controls.cameraHero=0;modernSelect(hero);require(Controls.doorSnapPending==1,"new room actor also queues immediate camera placement");
    *reinterpret_cast<uint32_t*>(hero+0x15f4)=1;*reinterpret_cast<uint32_t*>(doorCameras+8)=6;modernSelect(hero);
    require(Controls.scene&&!Controls.doorSnapPending,"true cutscene cancels door snap");
    *reinterpret_cast<uint32_t*>(hero+0x15f4)=0;modernSelect(hero);
    require(!Controls.doorSnapPending&&closeEnough(f(manager,0x40),0.2f),"cutscene return retains configured easing with door snapping enabled");
    Controls.enabled=0;invokeSelector(hero);require(*reinterpret_cast<uint32_t*>(manager+0x14)==2,"F4 off uses native selector");
    VirtualFree(globals,0,MEM_RELEASE);puts("Modern-controls deterministic and assembly checks passed.");return 0;
}
