#define WIN32_LEAN_AND_MEAN
#include "CursorConfinement.h"
#include <cstdio>
#include <cstdlib>
struct FakeCursorApi {
    static inline bool focused=true,valid=true;
    static inline RECT area{0,0,1920,1080},clip{-1920,0,3840,2160};
    static inline int writes=0,releases=0;
    static bool target(DWORD pid,RECT& result){result=area;return pid==42&&focused&&valid;}
    static bool get(RECT& result){result=clip;return true;}
    static bool set(const RECT* value){++writes;if(value)clip=*value;else{++releases;clip={-1920,0,3840,2160};}return true;}
};
static void check(bool value,const char* label){if(!value){printf("FAIL: %s\n",label);exit(1);}}
int main(){
    using A=FakeCursorApi;
    {
        CursorConfinement<A> guard;
        guard.update(42,true);check(A::writes==1&&A::clip.right==1920,"focused game confines cursor to client rectangle");
        guard.update(42,true);check(A::writes==1,"unchanged confinement is not reapplied");
        A::clip={-1920,0,3840,2160};guard.update(42,true);check(A::writes==2&&A::clip.right==1920,"game releasing clip is repaired on next update");
        A::area={-1600,200,-320,920};guard.update(42,true);check(A::clip.left==-1600&&A::clip.right==-320,"resizing and monitors with negative coordinates update the bounds");
        A::focused=false;guard.update(42,true);check(A::releases==1&&A::clip.right==3840,"Alt-Tab releases confinement");
        guard.update(42,true);check(A::releases==1,"background polling does not alter another app's cursor");
        A::focused=true;guard.update(42,true);guard.update(42,false);check(A::releases==2,"F4 or configuration disable releases confinement");
        guard.update(42,true);A::valid=false;guard.update(42,true);check(A::releases==3,"minimized or invalid game target releases confinement");
        A::valid=true;guard.update(42,true);A::focused=false;A::clip={20,20,100,100};guard.update(42,true);
        check(A::releases==3&&A::clip.left==20,"focus change preserves a different app's clipping rectangle");
        A::focused=true;guard.update(42,true);
    }
    check(A::releases==4,"helper exit releases owned confinement");
    puts("Cursor confinement focus, resize, release and ownership checks passed.");return 0;
}
