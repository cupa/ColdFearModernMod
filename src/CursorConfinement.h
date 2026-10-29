#pragma once
#include <windows.h>
struct WindowsCursorApi {
    static bool target(DWORD pid,RECT& result) {
        HWND window=GetForegroundWindow();DWORD foreground=0;
        GetWindowThreadProcessId(window,&foreground);
        if(foreground!=pid||!IsWindowVisible(window)||IsIconic(window))return false;
        RECT client{};if(!GetClientRect(window,&client))return false;
        POINT top{client.left,client.top},bottom{client.right,client.bottom};
        if(!ClientToScreen(window,&top)||!ClientToScreen(window,&bottom))return false;
        RECT desktop{GetSystemMetrics(SM_XVIRTUALSCREEN),GetSystemMetrics(SM_YVIRTUALSCREEN),0,0};
        desktop.right=desktop.left+GetSystemMetrics(SM_CXVIRTUALSCREEN);
        desktop.bottom=desktop.top+GetSystemMetrics(SM_CYVIRTUALSCREEN);
        RECT area{top.x,top.y,bottom.x,bottom.y};
        return IntersectRect(&result,&area,&desktop)&&result.right>result.left&&result.bottom>result.top&&GetForegroundWindow()==window;
    }
    static bool get(RECT& rect){return GetClipCursor(&rect)!=0;}
    static bool set(const RECT* rect){return ClipCursor(rect)!=0;}
};
template<class Api=WindowsCursorApi> class CursorConfinement {
    bool owned=false;
    RECT applied{};
    static bool same(const RECT& a,const RECT& b){return a.left==b.left&&a.top==b.top&&a.right==b.right&&a.bottom==b.bottom;}
public:
    ~CursorConfinement(){release();}
    void release() {
        if(!owned)return;
        RECT current{};if(!Api::get(current))return;
        // Release only this helper's last clipping rectangle.
        if(!same(current,applied)||Api::set(nullptr))owned=false;
    }
    void update(DWORD pid,bool enabled) {
        RECT target{};
        if(!enabled||!Api::target(pid,target)){release();return;}
        RECT current{};if(!Api::get(current))return;
        if(same(current,target))return;
        if(Api::set(&target)){applied=target;owned=true;}
    }
};
