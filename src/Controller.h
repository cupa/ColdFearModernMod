#pragma once
#include <xinput.h>
#include "PlayStation.h"
namespace Controller {
static XINPUT_STATE state{};
static int selected=-1;

#ifdef CONTROLS_TEST
static bool connected[4]{};
static XINPUT_STATE samples[4]{};
static DWORD read(DWORD slot,XINPUT_STATE* value){if(!connected[slot])return ERROR_DEVICE_NOT_CONNECTED;*value=samples[slot];return ERROR_SUCCESS;}
#else
using Read=DWORD(WINAPI*)(DWORD,XINPUT_STATE*);
static Read api=nullptr;
static bool loaded=false;

static DWORD read(DWORD slot,XINPUT_STATE* value) {
    if(!loaded) {
        loaded=true;
        const wchar_t* names[]={L"xinput1_4.dll",L"xinput1_3.dll",L"xinput9_1_0.dll"};
        for(auto name:names) {
            HMODULE library=LoadLibraryExW(name,nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
            if(!library)continue;
            api=reinterpret_cast<Read>(GetProcAddress(library,"XInputGetState"));
            if(api)break;
            FreeLibrary(library);
        }
    }
    return api?api(slot,value):ERROR_DEVICE_NOT_CONNECTED;
}
#endif
static void stick(SHORT rawX,SHORT rawY,float deadzone,float& x,float& y) {
    x=rawX/(rawX<0?32768.0f:32767.0f);y=rawY/(rawY<0?32768.0f:32767.0f);
    float length=sqrtf(x*x+y*y);
    if(length<=deadzone){x=0;y=0;return;}
    float magnitude=(fminf(length,1)-deadzone)/(1-deadzone);
    x=x/length*magnitude;y=y/length*magnitude;
}
struct Device {
    bool online=false,ready=false,fresh=false,active=false;
    DWORD scanAfter=0;
    XINPUT_STATE value{};
    uint32_t buttons=0,previous=0,menu=0,menuPrevious=0;
    float lx=0,ly=0,rx=0,ry=0;
    float markLX=0,markLY=0,markRX=0,markRY=0;
};
static Device devices[5]{};
static void clearOutput() {
    Controls.padConnected=Controls.padActive=0;
    Controls.padLX=Controls.padLY=Controls.padRX=Controls.padRY=Controls.padLT=Controls.padRT=0;
    Controls.padPrevious=Controls.padButtons=0;selected=-1;
    Controls.menuPrevious=Controls.menuButtons=0;
}
static void clear() {
    clearOutput();for(auto& device:devices)device={};
}
static void update(Device& d,const XINPUT_STATE& value) {
    d.value=value;d.previous=d.buttons;d.menuPrevious=d.menu;
    d.buttons=value.Gamepad.wButtons;
    if(value.Gamepad.bLeftTrigger>30)d.buttons|=0x10000;
    if(value.Gamepad.bRightTrigger>30)d.buttons|=0x20000;
    stick(value.Gamepad.sThumbLX,value.Gamepad.sThumbLY,Controls.moveDeadzone,d.lx,d.ly);
    stick(value.Gamepad.sThumbRX,value.Gamepad.sThumbRY,Controls.lookDeadzone,d.rx,d.ry);
    d.menu=d.buttons;
    if(d.lx>0.5f)d.menu|=XINPUT_GAMEPAD_DPAD_RIGHT;
    if(d.lx< -0.5f)d.menu|=XINPUT_GAMEPAD_DPAD_LEFT;
    if(d.ly>0.5f)d.menu|=XINPUT_GAMEPAD_DPAD_UP;
    if(d.ly< -0.5f)d.menu|=XINPUT_GAMEPAD_DPAD_DOWN;
    d.active=d.buttons||d.lx||d.ly||d.rx||d.ry;d.fresh=false;
    if(!d.ready) {
        d.ready=!d.active;d.previous=d.buttons;d.menuPrevious=d.menu;return;
    }
    // Fresh input selects the active controller.
    bool stickChange=(fabsf(d.lx)>0.1f||fabsf(d.ly)>0.1f||fabsf(d.rx)>0.1f||fabsf(d.ry)>0.1f)&&
        (fabsf(d.lx-d.markLX)>0.1f||fabsf(d.ly-d.markLY)>0.1f||fabsf(d.rx-d.markRX)>0.1f||fabsf(d.ry-d.markRY)>0.1f);
    d.fresh=(d.buttons&~d.previous)!=0||stickChange;
    if(stickChange||(!d.lx&&!d.ly&&!d.rx&&!d.ry)) {
        d.markLX=d.lx;d.markLY=d.ly;d.markRX=d.rx;d.markRY=d.ry;
    }
}
static bool keyboardActivity() {
    const int keys[]={'W','A','S','D','R','F','E','C',VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,VK_RETURN,VK_ESCAPE,VK_TAB,'P',VK_CONTROL,VK_LCONTROL,VK_RCONTROL,VK_SHIFT,VK_SPACE,VK_LBUTTON,VK_RBUTTON};
    for(int key:keys)if(down(key))return true;
    return fabsf(*reinterpret_cast<float*>(Controls.base+0x47b7a0))>0.01f||fabsf(*reinterpret_cast<float*>(Controls.base+0x47b7a4))>0.01f;
}
static bool promptKeyboardKey(int key) {
    // Ignore reserved virtual keys, including 0x07.
    static bool valid[256]{};
    static bool initialized=false;
    if(!initialized) {
        for(int i=8;i<256;++i)valid[i]=MapVirtualKeyW(i,MAPVK_VK_TO_VSC)!=0;
        valid[VK_LBUTTON]=valid[VK_RBUTTON]=valid[VK_MBUTTON]=valid[VK_XBUTTON1]=valid[VK_XBUTTON2]=true;
        initialized=true;
    }
    return key>0&&key<256&&valid[key];
}
static void poll() {
    bool keyboard=false;
    if(focused()) {
        for(int key=1;key<256;++key)if(promptKeyboardKey(key)&&down(key)){keyboard=true;break;}
        keyboard=keyboard||fabsf(*reinterpret_cast<float*>(Controls.base+0x47b7a0))>0.01f||fabsf(*reinterpret_cast<float*>(Controls.base+0x47b7a4))>0.01f;
        if(keyboard)Controls.lastController=0;
    }
    Controls.menuPrevious=Controls.menuButtons;Controls.menuButtons=0;
    Controls.keyPrevious=Controls.keyButtons;
    Controls.keyButtons=((down(VK_CONTROL)||down(VK_LCONTROL)||down(VK_RCONTROL))?1u:0u)|(down(VK_LBUTTON)?2u:0u);
    PlayStation::enable(Controls.controllerEnabled!=0&&Controls.enabled!=0);
    if(!Controls.controllerEnabled){clear();return;}
    int first=-1,candidate=-1;
    for(int i=0;i<5;++i) {
        Device& d=devices[i];XINPUT_STATE next{};
        if(Controls.padIndex>=0&&Controls.padIndex!=i){d={};continue;}
#ifndef CONTROLS_TEST
        DWORD now=GetTickCount();
        if(i<4&&!d.online&&static_cast<LONG>(now-d.scanAfter)<0)continue;
#endif
        bool found=i==4?PlayStation::read(next):read(static_cast<DWORD>(i),&next)==ERROR_SUCCESS;
        if(!found) {
            d={};
#ifndef CONTROLS_TEST
            if(i<4)d.scanAfter=now+1000;
#endif
            continue;
        }
        d.online=true;update(d,next);
        if(first<0)first=i;
        if(d.ready&&d.fresh&&candidate<0)candidate=i;
    }
    if(first<0){clearOutput();return;}
    bool current=selected>=0&&devices[selected].online;
    // Keep the current device on simultaneous activity.
    if(candidate>=0&&(!current||!devices[selected].fresh))selected=candidate;
    else if(!current)selected=first;
    const Device& d=devices[selected];state=d.value;Controls.padConnected=selected+1;
    if(!d.ready) {
        int keep=selected;clearOutput();selected=keep;Controls.padConnected=keep+1;return;
    }
    if(focused()&&!keyboard&&d.fresh)Controls.lastController=1;
    Controls.padPrevious=d.previous;Controls.padButtons=d.buttons;
    Controls.padLT=state.Gamepad.bLeftTrigger/255.0f;Controls.padRT=state.Gamepad.bRightTrigger/255.0f;
    Controls.padLX=d.lx;Controls.padLY=d.ly;Controls.padRX=d.rx;Controls.padRY=d.ry;
    if(!Controls.enabled||!Controls.modernMode||!focused()||keyboardActivity())Controls.padActive=0;
    else if(d.active)Controls.padActive=1;
    if(Controls.padActive){Controls.menuButtons=d.menu;Controls.menuPrevious=d.menuPrevious;}

}
static uint32_t menuState(unsigned kind,uint32_t mask) {
    bool now=(Controls.menuButtons&mask)!=0,previous=(Controls.menuPrevious&mask)!=0;
    return kind==1?now&&!previous:kind==6?!now&&previous:now;
}
static uint32_t menuDevice(uint32_t native,uintptr_t caller) {
    uintptr_t site=caller-Controls.base;
    if(Controls.controllerEnabled&&Controls.enabled&&Controls.padActive&&focused()&&
       (site==0xac79||site==0xcb02))return 5; // Use PC menu queries.
    return native;
}
static uint32_t menuQuery(unsigned kind,unsigned action,uintptr_t caller) {
    constexpr uint32_t Native=0xffffffff;
    if(!Controls.controllerEnabled||!Controls.enabled||!Controls.padActive||!focused())return Native;
    bool menu=caller>=Controls.base+0x7000&&caller<Controls.base+0x1a000;
    // Map B/Circle to Escape only inside existing menus.
    uintptr_t site=caller-Controls.base;
    bool cancel=site==0xaca1||site==0xaf08||site==0x107a4||site==0x14528||site==0x15637;
    // Menu keyboard queries use DirectInput scan codes.
    if(kind>=3&&kind<=5) {
        uint32_t mask=0;
        if(action==1)mask=XINPUT_GAMEPAD_BACK|(cancel?XINPUT_GAMEPAD_B:0);
        else if(menu)switch(action) {
            case 0x1c:mask=XINPUT_GAMEPAD_A|(!Controls.hero?XINPUT_GAMEPAD_START:0);break;
            case 0x39:mask=XINPUT_GAMEPAD_A;break;
            case 0xc8:mask=XINPUT_GAMEPAD_DPAD_UP;break;
            case 0xd0:mask=XINPUT_GAMEPAD_DPAD_DOWN;break;
            case 0xcb:mask=XINPUT_GAMEPAD_DPAD_LEFT;break;
            case 0xcd:mask=XINPUT_GAMEPAD_DPAD_RIGHT;break;
            default:return Native;
        }
        if(!mask)return Native;
        return menuState(kind==3?1:kind==5?6:0,mask);
    }
    if(action==5&&kind!=2)return Controls.hero?menuState(kind,XINPUT_GAMEPAD_START):0;
    // Action 4 is inventory; confirmation uses Escape-menu handlers.
    if(action==4&&kind!=2)return 0;
    if(!menu)return Native;
    if(kind==2)return action<4?0:Native; // Avoid double navigation.
    uint32_t mask=0;
    switch(action) {
        case 0:mask=XINPUT_GAMEPAD_DPAD_UP;break;
        case 1:mask=XINPUT_GAMEPAD_DPAD_RIGHT;break;
        case 2:mask=XINPUT_GAMEPAD_DPAD_DOWN;break;
        case 3:mask=XINPUT_GAMEPAD_DPAD_LEFT;break;
        case 6:mask=XINPUT_GAMEPAD_B;break;
        case 8:mask=XINPUT_GAMEPAD_A;break;
        default:return Native;
    }
    return menuState(kind,mask);
}
static uint32_t query(unsigned kind,unsigned action,uintptr_t caller) {
    constexpr uint32_t Native=0xffffffff;
    if(kind>=3||caller<Controls.base+0xa6550||caller>=Controls.base+0xa7b60)return menuQuery(kind,action,caller);
    if(!Controls.enabled||!Controls.modernMode||!focused())return Native;
    if(!Controls.padActive) {
        if(kind!=2&&action==14&&(Controls.keyButtons&1))return kind==1?(Controls.keyPrevious&1)==0:1;
        // Reserve Ctrl for crouching.
        if(kind!=2&&action==6&&(Controls.keyButtons&1))return 0;
        return Native;
    }
    if(kind==2) {
        float value=0;
        switch(action){case 0:value=Controls.padRX*Controls.aimScale;break;case 1:value=-Controls.padRY*Controls.aimScale;break;case 2:value=Controls.padLX;break;case 3:value=-Controls.padLY;break;default:return Native;}
        uint32_t bits;memcpy(&bits,&value,4);return bits;
    }
    uint32_t mask=0;
    switch(action) {
        case 7:mask=XINPUT_GAMEPAD_A;break;
        case 5:mask=XINPUT_GAMEPAD_START;break;
        case 6:mask=XINPUT_GAMEPAD_RIGHT_SHOULDER;break;
        case 8:mask=XINPUT_GAMEPAD_LEFT_SHOULDER|XINPUT_GAMEPAD_LEFT_THUMB;break;
        case 9:mask=XINPUT_GAMEPAD_X;break;
        case 10:mask=0x10000;break;
        case 11:mask=0x20000;break;
        case 12:mask=XINPUT_GAMEPAD_DPAD_LEFT;break;
        case 13:mask=XINPUT_GAMEPAD_DPAD_RIGHT|XINPUT_GAMEPAD_Y;break;
        case 14:mask=XINPUT_GAMEPAD_B|XINPUT_GAMEPAD_RIGHT_THUMB;break;
        default:if(action>22)return Native;break;
    }
    bool held=(Controls.padButtons&mask)!=0;
    return kind==1?held&&(Controls.padPrevious&mask)==0:held;
}
}
