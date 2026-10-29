#define CONTROLS_TEST
// Assembly labels intentionally limit fixture optimization.
#pragma warning(disable:4740)
#include "ModernControls.cpp"
#include <cstdio>
#include <cstdlib>
static void check(bool value,const char* label){if(!value){printf("FAIL: %s\n",label);exit(1);}}
static float asFloat(uint32_t bits){float value;memcpy(&value,&bits,4);return value;}
static bool closeValue(float a,float b){return fabsf(a-b)<0.0001f;}
extern "C" __declspec(naked) void titleHookFixture(){__asm{
    push ebp
    mov ebp,esp
    mov eax,[ebp+8]
    xor edx,edx
    jmp HookTitlePrompt
}}
static float nativeAxis=0.375f;
extern "C" __declspec(naked) void nativeButtonTail(){__asm{
 pop esi
 mov esp,ebp
 pop ebp
 mov eax,5ah
 ret 4
}}
extern "C" __declspec(naked) void nativeAxisTail(){__asm{
 pop esi
 pop ebx
 pop ecx
 mov esp,ebp
 pop ebp
 fld nativeAxis
 ret 4
}}
extern "C" __declspec(naked) void nativeFrameTail(){__asm{
 mov esp,ebp
 pop ebp
 ret
}}
extern "C" __declspec(naked) void meleeTail(){__asm{ret}}
extern "C" __declspec(naked) void rawTail(){__asm{
    pop ebp
    mov eax,5ah
    ret 4
}}
extern "C" __declspec(naked) void menuTail(){__asm{
    pop esi
    pop ebp
    ret 4
}}
extern "C" __declspec(naked) void invokeMenu(BYTE*){__asm{
    push ebp
    mov ebp,esp
    push esi
    push edi
    mov esi,[ebp+8]
    jmp HookMenuFrame
}}
int main() {
    BYTE* globals=static_cast<BYTE*>(VirtualAlloc(nullptr,0x480000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));check(globals!=nullptr,"allocate fixture");
    BYTE hero[8192]{};f(hero,0x1ac)=0.016f;hero[0x9ca]=1;f(hero,0xbc4)=45;rotation(reinterpret_cast<float*>(hero+0x1b4),0);
    Controls={};Controls.base=reinterpret_cast<uint32_t>(globals);Controls.enabled=Controls.modernMode=Controls.controllerEnabled=Controls.holdShift=1;
    Controls.padIndex=-1;Controls.moveDeadzone=0.15f;Controls.lookDeadzone=0.18f;Controls.lookSpeed=2.5f;Controls.aimScale=1;
    Controls.explorationDistance=180;Controls.sensitivity=0.0025f;
    uintptr_t caller=Controls.base+0xa7000;
    Controller::connected[0]=true;Controller::poll();check(Controls.padConnected==1&&!Controls.padActive,"connect neutral without taking over keyboard");
    Controller::samples[0].Gamepad.sThumbLX=16384;Controller::samples[0].Gamepad.sThumbRX=32767;
    Controller::samples[0].Gamepad.wButtons=XINPUT_GAMEPAD_X|XINPUT_GAMEPAD_LEFT_SHOULDER;
    Controller::samples[0].Gamepad.bLeftTrigger=100;Controller::samples[0].Gamepad.bRightTrigger=200;
    Controller::poll();check(Controls.padActive&&Controls.padLX>0.4f&&Controls.padLX<0.42f&&closeValue(Controls.padRX,1),"analog movement and radial deadzone rescaling");
    check(Controller::query(0,10,caller)==1&&Controller::query(0,11,caller)==1,"independent aim and fire triggers");
    check(Controller::query(1,9,caller)==1&&Controller::query(1,9,caller)==1,"reload press stable across queries in one frame");
    check(Controller::query(0,8,caller)==1&&modernRun(hero,1)==1,"controller run survives keyboard Shift override");
    check(closeValue(asFloat(Controller::query(2,0,caller)),1),"native aiming uses right stick axis");
    modernInput(hero);check(closeValue(Controls.yaw,-0.04f)&&closeValue(f(hero,0x8d0),Controls.padLX)&&closeValue(f(hero,0x8d4),0),"frame-rate scaled exploration camera and analog travel");
    Controller::poll();check(Controller::query(1,9,caller)==0,"held reload does not retrigger");
    check(Controller::query(0,9,Controls.base+0x1000)==0xffffffff,"native UI queries pass through");
    float x=0,y=0;Controller::stick(32767,32767,0.15f,x,y);check(closeValue(x*x+y*y,1),"stick diagonals capped at unit length");
    Controller::stick(-32768,0,0.15f,x,y);check(closeValue(x,-1)&&closeValue(y,0),"negative stick endpoint");
    Controller::stick(1000,1000,0.15f,x,y);check(x==0&&y==0,"deadzone removes drift");
    testKeys['W']=true;Controller::poll();check(!Controls.padActive&&Controller::query(0,11,caller)==0xffffffff,"keyboard takes over and native bindings return");
    testKeys['W']=false;Controller::poll();check(Controls.padActive,"controller can take over again");
    Controller::connected[0]=false;Controller::poll();check(!Controls.padConnected&&!Controls.padActive&&Controls.padButtons==0&&Controls.padLX==0,"disconnect clears movement and held buttons");
    Controller::connected[0]=true;Controller::poll();check(!Controls.padActive,"held input on reconnect is not armed");
    Controller::samples[0]={};Controller::poll();Controller::samples[0].Gamepad.wButtons=XINPUT_GAMEPAD_A;Controller::poll();
    check(Controller::query(1,7,caller)==1,"south button interacts after neutral reconnect");
    Controller::clear();Controls.padIndex=1;Controller::connected[1]=true;Controller::poll();check(Controls.padConnected==2,"explicit controller slot selection");
    // USB DualSense byte layout, including independent analog triggers.
    BYTE sony[78]{};sony[0]=1;sony[1]=128;sony[2]=128;sony[3]=255;sony[4]=0;
    sony[5]=100;sony[6]=200;sony[8]=0x28;sony[9]=0xe3;
    XINPUT_STATE parsed{};check(PlayStation::decode(0x0ce6,sony,64,parsed),"DualSense USB report recognized");
    check(parsed.Gamepad.sThumbLX==0&&parsed.Gamepad.sThumbLY==0&&parsed.Gamepad.sThumbRX==32767&&parsed.Gamepad.sThumbRY==32767,"Sony stick centers, direction and endpoints");
    check(parsed.Gamepad.bLeftTrigger==100&&parsed.Gamepad.bRightTrigger==200&&parsed.Gamepad.wButtons==(XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_LEFT_SHOULDER|XINPUT_GAMEPAD_RIGHT_SHOULDER|XINPUT_GAMEPAD_START|XINPUT_GAMEPAD_LEFT_THUMB|XINPUT_GAMEPAD_RIGHT_THUMB),"Sony face buttons, shoulders, options and triggers");
    check(PlayStation::decode(0x0df2,sony,64,parsed),"DualSense Edge USB report recognized");
    sony[10]=2;check(PlayStation::decode(0x0ce6,sony,64,parsed)&&(parsed.Gamepad.wButtons&XINPUT_GAMEPAD_BACK)&&(parsed.Gamepad.wButtons&XINPUT_GAMEPAD_START),"DualSense touchpad click and Options map to separate menu and pause buttons");sony[10]=0;
    check(!PlayStation::decode(0x0ce6,sony,8,parsed)&&!PlayStation::decode(0x1234,sony,64,parsed),"truncated and unsupported reports rejected");
    sony[5]=0x15;sony[6]=0;sony[8]=55;sony[9]=99;
    check(PlayStation::decode(0x09cc,sony,64,parsed)&&parsed.Gamepad.bLeftTrigger==55&&parsed.Gamepad.bRightTrigger==99&&(parsed.Gamepad.wButtons&XINPUT_GAMEPAD_X)&&(parsed.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_DOWN)&&(parsed.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_LEFT),"DualShock 4 USB buttons, diagonal hat and trigger layout");
    check(PlayStation::decode(0x0ce6,sony,10,parsed),"Bluetooth simple report recognized");
    // Full Bluetooth headers and CRC; corrupt/truncated packets cannot drive inputs.
    const USHORT sonyProducts[]={0x0ce6,0x05c4};
    for(USHORT product:sonyProducts) {
        memset(sony,0,sizeof(sony));sony[0]=product==0x0ce6?0x31:0x11;
        size_t start=product==0x0ce6?2:3;sony[start]=128;sony[start+1]=128;sony[start+2]=128;sony[start+3]=128;
        sony[start+(product==0x0ce6?7:4)]=0x88;
        DWORD crc=0xffffffff;BYTE data[75]{};data[0]=0xa1;memcpy(data+1,sony,74);
        for(BYTE byte:data){crc^=byte;for(int bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320:0);}
        crc=~crc;memcpy(sony+74,&crc,4);
        check(PlayStation::decode(product,sony,78,parsed)&&parsed.Gamepad.wButtons==XINPUT_GAMEPAD_Y,"Bluetooth full report header and CRC");
        sony[10]^=1;check(!PlayStation::decode(product,sony,78,parsed),"Bluetooth corrupted report rejected");
    }
    Controller::clear();Controls.padIndex=-1;Controller::connected[0]=Controller::connected[1]=false;
    PlayStation::connected=true;PlayStation::sample={};Controller::poll();check(Controls.padConnected==5&&!Controls.padActive,"native PlayStation connects neutral");
    PlayStation::sample.Gamepad.sThumbLX=32767;PlayStation::sample.Gamepad.wButtons=XINPUT_GAMEPAD_A;
    Controller::poll();check(Controls.padActive&&closeValue(Controls.padLX,1)&&Controller::query(1,7,caller)==1,"native PlayStation uses existing modern gameplay path");
    PlayStation::connected=false;Controller::poll();check(!Controls.padConnected&&!Controls.padButtons,"native PlayStation disconnect clears input");
    Controls.padIndex=0;PlayStation::connected=true;Controller::poll();check(!Controls.padConnected,"explicit XInput slot excludes HID fallback");
    PlayStation::connected=false;
    Controls.padIndex=-1;Controller::clear();
    testKeys[VK_LCONTROL]=true;Controller::poll();
    check(Controller::query(0,14,caller)==1&&Controller::query(1,14,caller)==1&&Controller::query(0,6,caller)==0,"left Ctrl crouches and consumes original grab binding");
    Controller::poll();check(Controller::query(1,14,caller)==0,"held Ctrl does not retrigger pressed query");
    testKeys[VK_LCONTROL]=false;Controller::poll();check(Controller::query(0,14,caller)==0xffffffff,"Ctrl release retains native crouch binding");
    testKeys[VK_RCONTROL]=true;Controller::poll();check(Controller::query(0,14,caller)==1,"right Ctrl crouches");
    testKeys[VK_RCONTROL]=false;Controller::connected[0]=true;Controller::samples[0]={};Controller::poll();
    Controller::samples[0].Gamepad.wButtons=XINPUT_GAMEPAD_RIGHT_THUMB;Controller::poll();
    check(Controller::query(0,14,caller)==1&&Controller::query(1,14,caller)==1,"right-stick click crouches");
    testKeys[VK_CONTROL]=true;Controller::poll();check(!Controls.padActive&&Controller::query(0,14,caller)==1,"Ctrl takes over from active controller");
    testKeys[VK_CONTROL]=false;Controller::connected[0]=false;Controller::poll();
    Controls.controllerEnabled=0;testKeys[VK_CONTROL]=true;Controller::poll();check(Controller::query(0,14,caller)==1,"Ctrl works with controller support disabled");
    testKeys[VK_CONTROL]=false;Controls.controllerEnabled=1;Controller::poll();
    // Choose one controller without mixing axes or button edges.
    Controller::clear();Controller::connected[0]=true;Controller::samples[0]={};PlayStation::connected=true;PlayStation::sample={};Controller::poll();
    Controller::samples[0].Gamepad.sThumbLX=22000;Controller::poll();check(Controls.padConnected==1&&Controls.padLX>0.5f,"Xbox can move with Sony connected");
    PlayStation::sample.Gamepad.sThumbRX=22000;Controller::poll();check(Controls.padConnected==5&&Controls.padRX>0.5f&&Controls.padLX==0,"Sony takes over from a held Xbox stick without mixing movement");
    Controller::poll();check(Controls.padConnected==5,"unchanged held Xbox input does not steal control back");
    Controller::samples[0].Gamepad.bRightTrigger=255;Controller::poll();check(Controls.padConnected==1&&Controller::query(1,11,caller)==1,"Xbox trigger takes over and produces a fresh press");
    PlayStation::sample.Gamepad.bRightTrigger=255;Controller::poll();check(Controls.padConnected==5&&Controller::query(1,11,caller)==1,"same trigger on Sony retains its own press edge during handoff");
    Controller::samples[0].Gamepad.wButtons=XINPUT_GAMEPAD_START;Controller::poll();check(Controls.padConnected==1&&Controller::query(1,5,Controls.base+0xaf00)==1,"Xbox pause takes over while Sony holds fire");
    PlayStation::sample.Gamepad.wButtons=XINPUT_GAMEPAD_START;Controller::poll();check(Controls.padConnected==5&&Controller::query(1,5,Controls.base+0xaf00)==1,"Sony pause has a separate press edge");
    Controller::samples[0].Gamepad.wButtons=0;Controller::poll();check(Controls.padConnected==5&&Controller::query(6,5,Controls.base+0xaf00)==0,"releasing old controller pause cannot release the active controller button");
    PlayStation::sample.Gamepad.wButtons=0;Controller::poll();check(Controller::query(6,5,Controls.base+0xaf00)==1,"active controller pause release works after handoff");
    PlayStation::connected=false;Controller::poll();check(Controls.padConnected==1&&Controller::query(1,11,caller)==0,"disconnect falls back to connected Xbox without phantom trigger press");
    Controls.padIndex=0;PlayStation::connected=true;PlayStation::sample={};Controller::poll();PlayStation::sample.Gamepad.wButtons=XINPUT_GAMEPAD_A;Controller::poll();
    check(Controls.padConnected==1,"explicit slot selection still prevents cross-device takeover");
    Controls.padIndex=-1;Controller::clear();Controller::connected[0]=false;PlayStation::connected=false;Controller::poll();
    // Non-aim fire requests melee and suppresses shooting.
    Controls.hero=reinterpret_cast<uint32_t>(hero);Controls.padActive=0;hero[0x9ca]=1;
    *reinterpret_cast<int32_t*>(hero+0x9d4)=-1;*reinterpret_cast<int32_t*>(hero+0x9d8)=-1;
    Controls.keyButtons=2;Controls.keyPrevious=0;hero[0x9c2]=hero[0x9c7]=hero[0x9c8]=1;
    modernCombat(hero);check(Controls.meleeRequest&&hero[0x9c3]&&!hero[0x9c2]&&!hero[0x9c7]&&!hero[0x9c8],"mouse fire outside aim requests melee and suppresses bullet flags");
    check(modernMeleeMode(hero)==0&&hero[0x9ca]==1,"melee uses native strike branch without changing camera aim flag");
    *reinterpret_cast<int32_t*>(hero+0x9d8)=0;check(modernMeleeMode(hero)==1,"context ownership blocks strike override");
    *reinterpret_cast<int32_t*>(hero+0x9d8)=-1;hero[0x94f]=1;check(modernMeleeMode(hero)==1,"native strike lock blocks override");hero[0x94f]=0;
    Controls.keyPrevious=2;hero[0x9c3]=0;modernCombat(hero);check(!Controls.meleeRequest&&!hero[0x9c3],"held fire does not repeat melee");
    Controls.padActive=1;Controls.padButtons=0x20000;Controls.padPrevious=0;modernCombat(hero);check(Controls.meleeRequest,"controller RT requests melee outside aim");
    hero[0x9ca]=0;hero[0x9c2]=hero[0x9c7]=1;modernCombat(hero);check(!Controls.meleeRequest&&hero[0x9c2]&&hero[0x9c7],"aimed fire stays native");
    hero[0x9ca]=1;hero[0x936]=1;modernCombat(hero);check(!Controls.meleeRequest&&hero[0x9c2],"native special-state combat stays native");hero[0x936]=0;
    Controls.hero=0;
    Controls.menuButtons=XINPUT_GAMEPAD_A;Controls.menuPrevious=0;Controls.padActive=1;
    check(Controller::menuDevice(3,Controls.base+0xac79)==5&&Controller::menuDevice(4,Controls.base+0xcb02)==5,"native pad types select PC menu adapter at verified menu callers");
    check(Controller::menuDevice(3,Controls.base+0x4000)==3,"gameplay device queries remain native");
    Controls.menuButtons=XINPUT_GAMEPAD_START;
    check(Controller::query(3,0x1c,Controls.base+0xaf2b)==1&&Controller::query(0,5,Controls.base+0x486e)==0,"Start confirms in pregame menus without pausing them");
    Controls.hero=1;check(Controller::query(3,0x1c,Controls.base+0xaf2b)==0&&Controller::query(0,5,Controls.base+0x486e)==1,"Start returns to pause after player creation");
    uintptr_t menuCaller=Controls.base+0xaf00;
    Controller::clear();Controller::connected[0]=true;Controller::samples[0]={};modernMenuFrame();
    Controller::samples[0].Gamepad.wButtons=XINPUT_GAMEPAD_START|XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_DPAD_DOWN;
    modernMenuFrame();check(Controls.menuFrames==2&&Controls.padActive,"global input polling takes control without player frame");
    check(Controller::query(3,1,menuCaller)==0&&Controller::query(4,1,menuCaller)==0&&Controller::query(0,5,menuCaller)==1,"Start pauses without opening the Escape menu");
    check(Controller::query(3,0x1c,menuCaller)==1&&Controller::query(1,8,menuCaller)==1&&Controller::query(1,4,menuCaller)==0,"A/Cross confirms through Enter or Space action and never toggles Tab menu");
    check(Controller::query(1,2,menuCaller)==1&&Controller::query(3,0xd0,menuCaller)==1,"D-pad feeds logical and raw navigation paths");
    check(Controller::query(1,2,menuCaller)==1,"multiple menu queries retain the same frame edge");
    modernMenuFrame();check(Controller::query(1,2,menuCaller)==0&&Controller::query(0,2,menuCaller)==1,"held navigation uses native repeat instead of repeated presses");
    Controller::samples[0]={};modernMenuFrame();
    check(Controller::query(5,1,menuCaller)==0&&Controller::query(6,5,Controls.base+0x486e)==1,"Start release reaches pause only");
    Controller::samples[0].Gamepad.wButtons=XINPUT_GAMEPAD_BACK;modernMenuFrame();
    check(Controller::query(3,1,menuCaller)==1&&Controller::query(3,1,caller)==1&&Controller::query(1,5,menuCaller)==0&&Controller::query(1,4,menuCaller)==0,"View/touchpad opens Escape menu without pause or Tab toggle");
    Controller::samples[0]={};modernMenuFrame();
    Controller::samples[0].Gamepad.wButtons=XINPUT_GAMEPAD_B;modernMenuFrame();
    check(Controller::query(3,1,Controls.base+0xaf08)==1&&Controller::query(1,6,menuCaller)==1&&Controller::query(1,5,menuCaller)==0&&Controller::query(3,1,caller)==0,"B backs out only through menu cancel handlers without pausing gameplay");
    check(Controller::query(5,1,Controls.base+0x17297)==0&&Controller::query(3,1,Controls.base+0x17297)==0&&Controller::query(0,14,caller)==1,"B/Circle crouches and never activates the HUD menu opener");
    Controller::samples[0]={};modernMenuFrame();
    check(Controller::query(5,1,Controls.base+0x17297)==0&&Controller::query(5,1,Controls.base+0xaf08)==1,"releasing B/Circle cancels an open menu and cannot open the HUD menu");
    Controller::samples[0].Gamepad.wButtons=XINPUT_GAMEPAD_BACK;modernMenuFrame();
    check(Controller::query(3,1,Controls.base+0x17297)==1,"View/touchpad still reaches the HUD menu opener");
    Controller::samples[0]={};Controller::samples[0].Gamepad.sThumbLY=32767;modernMenuFrame();
    check(Controller::query(1,0,menuCaller)==1&&Controller::query(3,0xc8,menuCaller)==1&&Controller::query(2,3,menuCaller)==0,"left stick navigates with no duplicate analog menu axis");
    check(Controller::query(0,14,Controls.base+0x300000)==0xffffffff,"unrelated gameplay callers retain native inputs");
    testKeys[VK_DOWN]=true;modernMenuFrame();check(!Controls.padActive&&Controller::query(3,0xd0,menuCaller)==0xffffffff,"keyboard arrows take over in menus");testKeys[VK_DOWN]=false;
    // Check native fallback ABI and x87 preservation.
    Controls.backPadFrame=reinterpret_cast<uint32_t>(&nativeFrameTail);Controls.backPadHeld=reinterpret_cast<uint32_t>(&nativeButtonTail);
    Controls.backPadPressed=Controls.backPadHeld;Controls.backPadAxis=reinterpret_cast<uint32_t>(&nativeAxisTail);
    DWORD result=0,ecxAfter=0;float before=3.5f,after=0,axis=0;Controls.padActive=0;
    __asm {
        mov ecx,12345678h
        fld before
        push 10
        call HookPadHeld
        mov result,eax
        mov ecxAfter,ecx
        fstp after
        push 0
        call HookPadAxis
        fstp axis
        call HookPadFrame
    }
    check(result==0x5a&&ecxAfter==0x12345678&&closeValue(before,after)&&closeValue(axis,nativeAxis),"native query fallback prologues and floating point stack");
    // Exercise the handled RET 4 and float return paths.
    uint32_t savedBase=Controls.base,syntheticBase=0;
    __asm {
 lea eax,handledReturn
 sub eax,0xa7000
 mov syntheticBase,eax
}
    Controls.base=syntheticBase;Controls.padActive=1;Controls.padButtons=0x10000;Controls.padRX=0.6f;Controls.aimScale=1;
    __asm {
        mov ecx,12345678h
        push 10
        call HookPadHeld
    handledReturn:
        mov result,eax
        mov ecxAfter,ecx
        push 0
        call HookPadAxis
        fstp axis
    }
    check(result==1&&ecxAfter==0x12345678&&closeValue(axis,0.6f),"handled button/axis query ABI and stack cleanup");
    Controls.base=savedBase;
    Controls.backMelee=reinterpret_cast<uint32_t>(&meleeTail);Controls.meleeRequest=1;
    Controls.hero=reinterpret_cast<uint32_t>(hero);hero[0x9ca]=1;hero[0x9c2]=0;hero[0x9c3]=1;
    DWORD flags=0;BYTE* actor=hero;
    __asm {
        push esi
        mov esi,actor
        mov eax,12345678h
        mov ecx,87654321h
        fld before
        stc
        call HookMelee
        mov result,eax
        mov ecxAfter,ecx
        fstp after
        pushfd
        pop flags
        pop esi
    }
    check(result==0x12345600&&ecxAfter==0x87654321&&closeValue(after,before)&&(flags&1),"melee hook preserves upper EAX, registers, flags and floating point stack");
    Controls.enabled=0;
    __asm {
        push esi
        mov esi,actor
        call HookMelee
        mov result,eax
        pop esi
    }
    check((result&255)==1,"disabled melee hook replays original camera-mode byte");
    Controls.enabled=1;Controls.padActive=1;
    Controls.backRawPressed=Controls.backRawHeld=Controls.backRawReleased=reinterpret_cast<uint32_t>(&rawTail);
    Controls.backPadReleased=reinterpret_cast<uint32_t>(&nativeButtonTail);
    Controls.menuButtons=XINPUT_GAMEPAD_BACK;Controls.menuPrevious=0;
    __asm {
        mov ecx,12345678h
        fld before
        push 1
        call HookRawPressed
        mov result,eax
        mov ecxAfter,ecx
        fstp after
    }
    check(result==1&&ecxAfter==0x12345678&&closeValue(after,before),"raw menu pressed hook ABI and floating point preservation");
    __asm {
        push 1
        call HookRawHeld
        mov result,eax
    }
    check(result==1,"raw menu held hook handled path");
    Controls.menuButtons=0;Controls.menuPrevious=XINPUT_GAMEPAD_BACK;
    __asm {
        push 1
        call HookRawReleased
        mov result,eax
    }
    check(result==1,"raw menu released hook handled path");
    Controls.menuPrevious=XINPUT_GAMEPAD_START;
    __asm {
        push 5
        call HookPadReleased
        mov result,eax
    }
    check(result==1,"logical pause released hook handled path");
    Controls.padActive=0;
    __asm {
        push 1
        call HookRawPressed
        mov result,eax
        push 1
        call HookRawHeld
        push 1
        call HookRawReleased
        push 5
        call HookPadReleased
        mov ecxAfter,eax
    }
    check(result==0x5a&&ecxAfter==0x5a,"raw and logical release fallback prologues return and clean arguments");
    BYTE inputManager[32]{};*reinterpret_cast<uint32_t*>(inputManager+8)=0x3f800000;
    Controls.backMenuFrame=reinterpret_cast<uint32_t>(&menuTail);uint32_t framesBefore=Controls.menuFrames;
    invokeMenu(inputManager);
    check(Controls.menuFrames==framesBefore+1&&*reinterpret_cast<uint32_t*>(inputManager+12)==0x3f800000,"global menu hook samples input and replays manager stores and stack pops");
    BYTE menuDeviceInput[16]{};*reinterpret_cast<uint32_t*>(menuDeviceInput+8)=4;
    uint32_t nativeDeviceResult=0;float deviceFloatBefore=0.3125f,deviceFloatAfter=0;
    __asm {
        fld deviceFloatBefore
        lea ecx,menuDeviceInput
        call HookMenuDevice
        mov nativeDeviceResult,eax
        fstp deviceFloatAfter
    }
    check(nativeDeviceResult==4&&deviceFloatAfter==deviceFloatBefore,"menu device hook preserves native fallback ABI and live floating point context");
    // Retain prompt selection through neutral input and focus loss.
    Controller::clear();memset(testKeys,0,sizeof(testKeys));Controls.padIndex=-1;
    for(int i=0;i<4;++i){Controller::connected[i]=false;Controller::samples[i]={};}
    Controller::connected[0]=true;Controller::poll();Controls.lastController=0;
    testKeys[7]=true;Controller::samples[0].Gamepad.wButtons=XINPUT_GAMEPAD_X;Controller::poll();
    check(Controls.lastController==1,"controller activity selects prompt despite reserved Windows key state");
    Controller::samples[0]={};Controller::poll();testFocused=false;Controller::poll();
    check(Controls.lastController==1,"neutral and focus loss preserve last prompt");
    testFocused=true;testKeys['Z']=true;Controller::poll();testKeys['Z']=false;
    check(Controls.lastController==0,"non-gameplay keyboard key restores native title prompt");
    Controller::samples[0].Gamepad.sThumbRX=20000;Controller::poll();
    check(Controls.lastController==1,"stick activity selects controller prompt");
    *reinterpret_cast<float*>(globals+0x47b7a0)=3;Controller::poll();
    check(Controls.lastController==0,"mouse movement restores native prompt");
    *reinterpret_cast<float*>(globals+0x47b7a0)=0;
    const wchar_t* original=L"Click to START";
    check(titlePrompt(original,7,7,true)!=original&&wcscmp(titlePrompt(original,7,7,true),L"Press any button to START")==0,"controller title wording");
    check(titlePrompt(original,7,7,false)==original&&titlePrompt(original,8,7,true)==original&&titlePrompt(original,-1,-1,true)==original,"native localization and unrelated labels preserved");
    TitleGlyph glyphMap[128]{};
    for(unsigned i=1;i<128;++i){glyphMap[i].character=static_cast<uint16_t>(i);glyphMap[i].glyph=static_cast<uint16_t>(i+100);}
    wchar_t encodedTitle[29]{};
    check(encodeTitlePrompt(glyphMap,128,encodedTitle)&&encodedTitle[0]==L'P'+100&&encodedTitle[5]==L' '+100&&encodedTitle[25]==0,"title is converted through native glyph map including spaces and terminator");
    check(titlePrompt(original,7,7,true,encodedTitle)==encodedTitle,"native lookup returns encoded title buffer");
    glyphMap['P'].character=0;
    check(!encodeTitlePrompt(glyphMap,128,encodedTitle),"missing title glyph uses native fallback");
    check(centeredTitleX(640,200)==220&&centeredTitleX(640,100)==270,"both title wordings share the canvas center");
    check(centeredTitleX(3840/6.0f,200)==220&&centeredTitleX(2560/4.0f,200)==220,"title centering uses current UI scale at 1440p and 4K");
    uint32_t titleResult=0;float titleFloatAfter=0;
    __asm {
        fld deviceFloatBefore
        push original
        call titleHookFixture
        mov titleResult,eax
        fstp titleFloatAfter
    }
    check(titleResult==reinterpret_cast<uint32_t>(original)&&titleFloatAfter==deviceFloatBefore,"title lookup hook preserves pointer, stack and floating point context");
    VirtualFree(globals,0,MEM_RELEASE);
    puts("Controller polling, analog, button, reconnect and assembly checks passed.");return 0;
}
