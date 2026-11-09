#define DISPLAY_TEST
#include "DisplayPlugin.cpp"
#include <cstdlib>
static void check(bool ok,const char* message){if(!ok){printf("FAIL: %s\n",message);exit(1);}}
static uint32_t blendState,blendValue,blendDx;
static __declspec(naked) void blendTail(){__asm{
    mov blendState,eax
    mov blendValue,ecx
    mov blendDx,edx
    ret
}}
static void testBlend(uint32_t state,uint32_t value,uint32_t expected){
    afterBlend=reinterpret_cast<uintptr_t>(blendTail);
    __asm{
        mov edx,12345678h
        push value
        push state
        call hookBlend
        add esp,8
    }
    check(blendState==state&&blendValue==expected&&blendDx==0x12345678,"blend hook arguments and registers");
}
static __declspec(naked) void nativeTail(){__asm{ret}}
static __declspec(naked) void startupTail(){__asm{
    add esp,4
    ret
}}
static unsigned initCalls;
static int __cdecl fakeInit(uint32_t*){++initCalls;return 123;}
static __declspec(naked) void startupCall(uint32_t*,uint32_t*){__asm{
    push ebp
    mov ebp,esp
    sub esp,0x40
    mov dword ptr [ebp-0x34],1024
    mov dword ptr [ebp-0x30],768
    call hookStartup
    mov ecx,[ebp+8]
    mov eax,[ebp-0x34]
    mov [ecx],eax
    mov ecx,[ebp+12]
    mov eax,[ebp-0x30]
    mov [ecx],eax
    mov esp,ebp
    pop ebp
    ret
}}
static void testSlot(uint32_t slot,uint32_t* width,uint32_t* height){
    HANDLE f=CreateFileW(resolutionPath,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);check(f!=INVALID_HANDLE_VALUE,"create test preset");
    DWORD done=0;WriteFile(f,&slot,4,&done,nullptr);CloseHandle(f);check(done==4,"write test preset");startupCall(width,height);
}
static __declspec(naked) void cycleCall(BYTE*,int){__asm{
    push ebp
    mov ebp,esp
    push ebx
    push edi
    mov ebx,[ebp+8]
    mov edi,[ebp+12]
    call hookCycle
    pop edi
    pop ebx
    pop ebp
    ret
}}
static __declspec(naked) uint32_t labelCall(BYTE*,uint32_t*,uint32_t*){__asm{
    push ebp
    mov ebp,esp
    push ebx
    mov ebx,[ebp+8]
    mov eax,0x12345678
    call hookLabel
    mov ebx,[ebp+12]
    mov [ebx],ecx
    mov ebx,[ebp+16]
    mov [ebx],edx
    pop ebx
    pop ebp
    ret
}}
struct FakeStartupCursor {
    static inline RECT bounds{-1920,20,640,1460},applied{};
    static inline bool available=true,released=false;
    static bool target(RECT& value){value=bounds;return available;}
    static BOOL set(const RECT* value){released=value==nullptr;if(value)applied=*value;return TRUE;}
};
int main(){
    const Display::Canvas hd{1920,1080},deck{1280,800},classic{1024,768},ultrawide{3440,1440};
    const auto closeEnough=[](float a,float b){return fabsf(a-b)<0.01f;};
    check(closeEnough(hd.hudX(0),80)&&closeEnough(hd.hudX(640),560)&&closeEnough(hd.hudX(320),320),"16:9 HUD fits centered 4:3 canvas");
    check(closeEnough(hd.hudX(0,-1),0)&&closeEnough(hd.hudX(640,1),640),"edge anchoring moves HUD without stretching");
    check(closeEnough(deck.hudScale(),5.0f/6.0f)&&closeEnough(classic.hudScale(),1),"16:10 scales while 4:3 is unchanged");
    check(closeEnough(hd.projectionScale(),4.0f/3.0f)&&closeEnough(ultrawide.projectionScale(),43.0f/24.0f)&&closeEnough(classic.projectionScale(),1),"projection preserves baseline framing on wider displays");
    auto fit=Display::videoBox(hd,640,480,Display::VideoMode::Fit);
    auto fill=Display::videoBox(hd,640,480,Display::VideoMode::Fill);
    check(closeEnough(fit.x,240)&&closeEnough(fit.y,0)&&closeEnough(fit.width,1440)&&closeEnough(fit.height,1080),"video fits without stretching");
    check(closeEnough(fill.x,0)&&closeEnough(fill.y,-180)&&closeEnough(fill.width,1920)&&closeEnough(fill.height,1440),"video fills with centered crop");
    check(Display::hudEdge(20,360,164,380)==-1&&Display::hudEdge(435,360,600,400)==1&&Display::hudEdge(200,410,440,440)==0,"HUD edge anchors leave center prompts alone");
    desktopMode={2560,1440};
    testBlend(171,0xffffffff,1);
    for(uint32_t operation=1;operation<=5;++operation)testBlend(171,operation,operation);
    testBlend(170,0xffffffff,0xffffffff);testBlend(172,0xffffffff,0xffffffff);
    RECT stale{0,0,1024,768};
    check(correctedStartupClip<FakeStartupCursor>(&stale)&&FakeStartupCursor::applied.left==-1920&&FakeStartupCursor::applied.right==640&&FakeStartupCursor::applied.bottom==1460,"startup confinement uses actual screen coordinates and full native resolution instead of stale dimensions");
    correctedStartupClip<FakeStartupCursor>(nullptr);check(FakeStartupCursor::released,"native cursor release stays a release");
    FakeStartupCursor::available=false;correctedStartupClip<FakeStartupCursor>(&stale);check(FakeStartupCursor::released,"failed bounds query releases stale confinement");
    check(modes.count==9&&modes.find(1280).height==1024,"stock presets retained");
    check(modes.add(1920,1080)&&modes.add(2560,1440)&&modes.count==9,"fixed full HD and native QHD preset indices retained");
    check(modes.add(2560,1440)&&modes.count==9,"no duplicates");
    check(!modes.add(0,1440)&&!modes.add(20000,1440),"reject ambiguous and invalid modes");
    check(modes.cycle(1600,230)==1920&&modes.cycle(1920,230)==2560&&modes.cycle(2560,230)==3840&&modes.cycle(3840,230)==modes.selector(8)&&modes.cycle(modes.selector(8),230)==640,"right cycling includes native mode and wraps");
    check(modes.cycle(640,210)==modes.selector(8)&&modes.cycle(modes.selector(8),210)==3840&&modes.cycle(2560,210)==1920,"left cycling wraps and returns");
    check(modes.cycle(2560,200)==2560,"label refresh leaves selection alone");
    afterStartup=reinterpret_cast<uintptr_t>(startupTail);nativeInit=reinterpret_cast<uintptr_t>(fakeInit);
    afterCycle=afterLabel=reinterpret_cast<uintptr_t>(nativeTail);
    wcscpy_s(resolutionPath,L"build\\display-test-res.bin");
    check(modes.find(modes.selector(8)).width==1280&&modes.find(modes.selector(8)).height==800&&modes.index(1280)==3,"Steam Deck and stock 1280 widths have distinct menu values");
    check(modes.add(1280,800)&&modes.count==9,"desktop detection does not duplicate Steam Deck");
    uint32_t width=0,height=0;
    testSlot(Display::Modes::deckSlot,&width,&height);check(width==1280&&height==800,"Steam Deck saved mode starts at 1280x800");
    testDeckAvailable=false;testSlot(Display::Modes::deckSlot,&width,&height);check(width==2560&&height==1440,"unsupported Steam Deck mode falls back without overwriting its preset");testDeckAvailable=true;
    testSlot(6,&width,&height);check(width==2560&&height==1440&&startupSlot==6,"startup x86 hook decodes saved native preset");
    testSlot(7,&width,&height);check(width==3840&&height==2160,"4K startup preset applies when the connected display supports it");
    test4kAvailable=false;testSlot(7,&width,&height);check(width==2560&&height==1440,"disconnected 4K TV falls back to desktop dimensions");
    test4kAvailable=true;testSlot(7,&width,&height);check(width==3840&&height==2160,"saved 4K preset survives fallback and applies when TV reconnects");
    testSlot(4,&width,&height);check(width==2560&&height==1440,"legacy stock preset does not override the desktop by default");
    testSlot(99,&width,&height);check(width==2560&&height==1440,"invalid saved preset falls back to the desktop");
    check(initCalls==8,"startup hook replays the native graphics initializer and argument cleanup");
    forcedMode={1920,1080};testSlot(6,&width,&height);check(width==1920&&height==1080&&Wide::startupCanvas.width==1920,"forced mode drives startup and presentation");
    forcedMode={3840,2160};test4kAvailable=false;testSlot(5,&width,&height);check(width==1920&&height==1080,"unavailable forced mode keeps the saved mode");test4kAvailable=true;
    forcedMode={};startupResolution=StartupResolution::Desktop;testSlot(5,&width,&height);check(width==2560&&height==1440,"desktop override uses current display");
    startupResolution=StartupResolution::Automatic;testSlot(0xffffffff,&width,&height);check(width==2560&&height==1440,"missing saved mode uses desktop");
    for(auto display:{Display::Mode{2560,1440},Display::Mode{3840,2160},Display::Mode{1280,800}}) {
        desktopMode=display;testSlot(2,&width,&height);
        check(width==display.width&&height==display.height,"automatic startup uses 1440p, 4K or 1280x800 instead of the stock preset");
    }
    desktopMode={2560,1440};
    startupResolution=StartupResolution::Saved;testSlot(2,&width,&height);
    check(width==1024&&height==768,"explicit saved mode keeps a legacy stock preset");
    startupResolution=StartupResolution::Automatic;
    testSlot(2,&width,&height);resolutionReady=false;saveResolution(640);startupCall(&width,&height);
    check(startupSlot==2,"native startup write cannot pin a resolution");
    saveResolution(1024);startupCall(&width,&height);
    check(width==1024&&height==768,"an explicit stock menu selection overrides automatic startup");
    desktopMode={};testSlot(99,&width,&height);
    check(width==1024&&height==768,"game default is safe when the desktop cannot be read");
    desktopMode={2560,1440};
    BYTE menu[0xd400]{};auto selected=reinterpret_cast<uint32_t*>(menu+0xd3b8);*selected=1920;
    float floatBefore=0.375f,floatAfter=0;__asm{fld floatBefore}
    cycleCall(menu,230);__asm{fstp floatAfter}
    check(*selected==2560&&floatAfter==floatBefore,"menu x86 hook adds native choice and preserves live x87 stack");
    check(labelCall(menu,&width,&height)==0x12345678&&width==2560&&height==1440,"label x86 hook supplies actual dimensions and preserves format pointer");
    wcscpy_s(resolutionPath,L"build\\display-test-res.bin");saveResolution(2560);
    HANDLE f=CreateFileW(resolutionPath,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);check(f!=INVALID_HANDLE_VALUE,"save selection file");
    DWORD slot=99,done=0;ReadFile(f,&slot,4,&done,nullptr);CloseHandle(f);check(done==4&&slot==6,"native res.bin format persists new slot");
    saveResolution(3840);f=CreateFileW(resolutionPath,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);check(f!=INVALID_HANDLE_VALUE,"read 4K preset");ReadFile(f,&slot,4,&done,nullptr);CloseHandle(f);check(slot==7,"4K choice persists with stable slot 7");
    *selected=3840;check(labelCall(menu,&width,&height)==0x12345678&&width==3840&&height==2160,"menu label supports 4K dimensions");
    *selected=modes.selector(8);check(labelCall(menu,&width,&height)==0x12345678&&width==1280&&height==800,"native label hook displays actual Steam Deck dimensions");
    saveResolution(*selected);f=CreateFileW(resolutionPath,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);check(f!=INVALID_HANDLE_VALUE,"read Steam Deck preset");ReadFile(f,&slot,4,&done,nullptr);CloseHandle(f);check(slot==Display::Modes::deckSlot,"Steam Deck selection persists independently from stock 1280x1024");
    cycleCall(menu,230);check(*selected==640,"native cycle hook wraps after Steam Deck");
    *selected=1280;check(labelCall(menu,&width,&height)==0x12345678&&height==1024,"stock 1280x1024 label remains unchanged");
    check(modes.add(3440,1440)&&modes.savedIndex(9)==8&&modes.loadedIndex(8)==9,"previous desktop preset slot 8 remains compatible");
    testSlot(8,&width,&height);check(width==3440&&height==1440,"legacy desktop slot still loads its desktop dimensions");
    testSlot(0x80000000u|(3440u<<16)|1440u,&width,&height);check(width==3440&&height==1440,"saved custom mode does not depend on the menu order");
    check(modes.loadToken(modes.saveToken(9)).width==3440,"custom mode round trips as dimensions");
    check(modes.add(1280,720)&&modes.find(modes.selection(1280,720)).height==720,"additional duplicate-width desktop mode stays distinct");
    BYTE* image=static_cast<BYTE*>(VirtualAlloc(nullptr,0x4d6000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    check(image!=nullptr,"mock game image");gameBase=reinterpret_cast<uintptr_t>(image);
    const BYTE a[]={0x8d,0x4d,0xc0,0x51,0xe8,0xce,0x0f,0,0};memcpy(image+0x2949,a,sizeof(a));
    const BYTE b[]={0x81,0xff,0xe6,0,0,0};memcpy(image+0xc750,b,sizeof(b));
    const BYTE c[]={0x8b,0x8b,0xb8,0xd3,0,0};memcpy(image+0xc7c3,c,sizeof(c));
    const BYTE d[]={0x55,0x8b,0xec,0x8b,0x45,8};memcpy(image+0x92b0,d,sizeof(d));
    const BYTE clip[]={0xff,0x15,0x1c,0x53,0x8a,0};memcpy(image+0x22b8,clip,sizeof(clip));
    const BYTE blend[]={0x8b,0x44,0x24,4,0x8b,0x4c,0x24,8};memcpy(image+0x239810,blend,sizeof(blend));
    const BYTE cold[]={0x89,0x58,0xfc},reset[]={0x89,0x68,0xfc};memcpy(image+0x23a143,cold,sizeof(cold));memcpy(image+0x23a7a3,reset,sizeof(reset));
    const BYTE focus[]={0x74,5};memcpy(image+0x2843,focus,sizeof(focus));
    image[0x92b0]=0;check(!installPatches()&&image[0x2949]==a[0],"unsupported code rejected before any patch");image[0x92b0]=d[0];
    image[0x239810]=0;check(!installPatches()&&image[0x2949]==a[0],"unknown renderer code rejected before any patch");image[0x239810]=blend[0];
    image[0x23a7a3]=0;check(!installPatches()&&image[0x2949]==a[0],"unknown reset code rejected before any patch");image[0x23a7a3]=reset[0];
    check(installPatches()&&installed&&image[0x2949]==0xe9&&image[0xc750]==0xe9&&image[0xc7c3]==0xe9&&image[0x92b0]==0xe9&&image[0x22b8]==0xe8&&image[0x239810]==0xe9,"all verified native sites installed");
    check(image[0x23a144]==0x50&&image[0x23a7a4]==0x50,"cold and reset texture-stage caches initialize to zero");
    check(image[0x2843]==0xeb&&image[0x2844]==5,"focus changes bypass automatic pause only");
    int32_t clipDisplacement;memcpy(&clipDisplacement,image+0x22b9,4);check(gameBase+0x22b8+5+clipDisplacement==reinterpret_cast<uintptr_t>(startupClip)&&image[0x22bd]==0x90,"startup cursor call uses stdcall replacement and retains instruction length");
    MEMORY_BASIC_INFORMATION info{};VirtualQuery(image+0xc750,&info,sizeof(info));check(info.Protect==PAGE_READWRITE,"shared-page protection restored after patch writes");
    int32_t displacement;memcpy(&displacement,image+0xc750+1,4);check(gameBase+0xc750+5+displacement==reinterpret_cast<uintptr_t>(hookCycle),"relative branch destination correct");
    memcpy(image+0x2949,a,sizeof(a));memcpy(image+0xc750,b,sizeof(b));memcpy(image+0xc7c3,c,sizeof(c));
    memcpy(image+0x92b0,d,sizeof(d));memcpy(image+0x22b8,clip,sizeof(clip));memset(image+0x239810,0,sizeof(blend));
    memcpy(image+0x23a143,cold,sizeof(cold));memcpy(image+0x23a7a3,reset,sizeof(reset));
    memcpy(image+0x2843,focus,sizeof(focus));
    fixAltTab=false;check(installPatches()&&image[0x239810]==0&&image[0x23a144]==0x58&&image[0x23a7a4]==0x68&&image[0x2843]==0x74,"graphics correction can be disabled independently");fixAltTab=true;
    const BYTE quad[]={0x55,0x8b,0xec,0xa0,0x60,0xc4,0x81,0};
    const BYTE point[]={0x55,0x8b,0xec,0x56,0x57},segment[]={0x55,0x8b,0xec,0x51,0xa1,0x78,0xc4,0x81,0},string[]={0x55,0x8b,0xec,0x83,0xec,0x0c};
    const BYTE view[]={0xe8,0x37,0xdb,0x1f,0},movie[]={0xe8,0x32,0x4a,0xfa,0xff};
    const BYTE viewA[]={0xe8,0xe2,0xb1,0x1f,0},viewB[]={0xe8,0xad,0xb1,0x1f,0},viewC[]={0xe8,0x83,0xb1,0x1f,0};
    const BYTE widget[]={0x55,0x8b,0xec,0x83,0xec,0x1c},item[]={0x55,0x8b,0xec,0xd9,0x45,0x10},mouse[]={0xdb,5,0x94,0xb7,0x87,0};
    for(auto rva:{0x2b460,0x2b610,0x2b740})memcpy(image+rva,quad,sizeof(quad));
    memcpy(image+0x2ba20,point,sizeof(point));memcpy(image+0x2bba0,segment,sizeof(segment));
    for(auto rva:{0x2c8d0,0x2ca50})memcpy(image+rva,string,sizeof(string));
    memcpy(image+0x2dff4,view,sizeof(view));memcpy(image+0x30949,viewA,sizeof(viewA));
    memcpy(image+0x3097e,viewB,sizeof(viewB));memcpy(image+0x309a8,viewC,sizeof(viewC));
    memcpy(image+0x91b90,widget,sizeof(widget));memcpy(image+0x91500,item,sizeof(item));
    for(auto rva:{0xb961,0x105a2,0x12ced})memcpy(image+rva,mouse,sizeof(mouse));
    check(!Wide::install()&&image[0x2b460]==quad[0]&&image[0x2949]==0xe9,"unknown FMV code leaves the presentation group and base display intact");
    memcpy(image+0x86ec9,movie,sizeof(movie));
    const BYTE topBorder[]={0xe8,0x1d,0xbf,0xf8,0xff},bottomBorder[]={0xe8,0xfb,0xbe,0xf8,0xff};
    memcpy(image+0x9f81e,topBorder,sizeof(topBorder));memcpy(image+0x9f840,bottomBorder,sizeof(bottomBorder));
    Wide::hideBorders=true;
    check(Wide::install()&&image[0x2b460]==0xe9&&image[0x2bba0]==0xe9&&image[0x2ca50]==0xe9,"presentation hooks install together");
    auto branch=[&](int rva){int32_t delta;memcpy(&delta,image+rva+1,4);return gameBase+rva+5+delta;};
    check(branch(0x2dff4)==reinterpret_cast<uintptr_t>(Wide::setView)&&branch(0x86ec9)==reinterpret_cast<uintptr_t>(Wide::drawMovie),"projection and movie calls target correct replacements");
    Wide::setCanvas(2560,1440);
    check(branch(0xb961)==reinterpret_cast<uintptr_t>(Wide::menuCursorX)&&closeEnough(Wide::cursorScale,0.75f)&&closeEnough(Wide::cursorInset,320),"menu hit testing uses the inverse HUD transform");
    check(branch(0x9f81e)==reinterpret_cast<uintptr_t>(Wide::skipBorder)&&branch(0x9f840)==reinterpret_cast<uintptr_t>(Wide::skipBorder),"optional border removal patches only the two cutscene bar calls");
    const auto original=reinterpret_cast<const BYTE*>(Wide::sprite);
    check(memcmp(original,quad,sizeof(quad))==0&&original[sizeof(quad)]==0xe9,"sprite trampoline retains the complete native prologue");
    VirtualFree(const_cast<BYTE*>(original),0,MEM_RELEASE);
    VirtualFree(image,0,MEM_RELEASE);puts("Display menu, persistence, startup and x86 hook checks passed.");return 0;
}
