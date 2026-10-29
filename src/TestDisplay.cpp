#define DISPLAY_TEST
#include "DisplayPlugin.cpp"
#include <cstdlib>
static void check(bool ok,const char* message){if(!ok){printf("FAIL: %s\n",message);exit(1);}}
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
    testSlot(4,&width,&height);check(width==1600&&height==1200,"stock saved preset retains dimensions");
    testSlot(99,&width,&height);check(width==1024&&height==768,"invalid saved preset preserves safe defaults");
    check(initCalls==8,"startup hook replays the native graphics initializer and argument cleanup");
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
    check(modes.add(1280,720)&&modes.find(modes.selection(1280,720)).height==720,"additional duplicate-width desktop mode stays distinct");
    BYTE* image=static_cast<BYTE*>(VirtualAlloc(nullptr,0x4d6000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    check(image!=nullptr,"mock game image");gameBase=reinterpret_cast<uintptr_t>(image);
    const BYTE a[]={0x8d,0x4d,0xc0,0x51,0xe8,0xce,0x0f,0,0};memcpy(image+0x2949,a,sizeof(a));
    const BYTE b[]={0x81,0xff,0xe6,0,0,0};memcpy(image+0xc750,b,sizeof(b));
    const BYTE c[]={0x8b,0x8b,0xb8,0xd3,0,0};memcpy(image+0xc7c3,c,sizeof(c));
    const BYTE d[]={0x55,0x8b,0xec,0x8b,0x45,8};memcpy(image+0x92b0,d,sizeof(d));
    const BYTE clip[]={0xff,0x15,0x1c,0x53,0x8a,0};memcpy(image+0x22b8,clip,sizeof(clip));
    image[0x92b0]=0;check(!installPatches()&&image[0x2949]==a[0],"unsupported code rejected before any patch");image[0x92b0]=d[0];
    check(installPatches()&&installed&&image[0x2949]==0xe9&&image[0xc750]==0xe9&&image[0xc7c3]==0xe9&&image[0x92b0]==0xe9&&image[0x22b8]==0xe8,"all verified native sites installed");
    int32_t clipDisplacement;memcpy(&clipDisplacement,image+0x22b9,4);check(gameBase+0x22b8+5+clipDisplacement==reinterpret_cast<uintptr_t>(startupClip)&&image[0x22bd]==0x90,"startup cursor call uses stdcall replacement and retains instruction length");
    MEMORY_BASIC_INFORMATION info{};VirtualQuery(image+0xc750,&info,sizeof(info));check(info.Protect==PAGE_READWRITE,"shared-page protection restored after patch writes");
    int32_t displacement;memcpy(&displacement,image+0xc750+1,4);check(gameBase+0xc750+5+displacement==reinterpret_cast<uintptr_t>(hookCycle),"relative branch destination correct");
    VirtualFree(image,0,MEM_RELEASE);puts("Display menu, persistence, startup and x86 hook checks passed.");return 0;
}
