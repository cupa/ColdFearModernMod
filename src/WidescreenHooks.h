#pragma once
#include "Widescreen.h"

namespace Wide {
static bool correctHud=true,edgeHud=true,correctProjection=true,hideBorders=false;
static Display::VideoMode videoMode=Display::VideoMode::Fit;
static Display::Canvas startupCanvas{};
static Display::Canvas canvas() {
    return startupCanvas;
}
static bool logicalCanvas() {return *reinterpret_cast<const uint32_t*>(gameBase+0x41c498)==0;}
static unsigned hudDepth;
static float edgeAmount=1;
static float anchor(float x,float y,float right,float bottom) {
    return edgeHud&&hudDepth?Display::hudEdge(x,y,right,bottom)*edgeAmount:0;
}
static float cursorInset,cursorScale=1;
static void setCanvas(float width,float height) {
    startupCanvas={width,height};
    cursorScale=startupCanvas.hudScale();
    cursorInset=width*(1-cursorScale)*0.5f;
}
static __declspec(naked) void menuCursorX() {__asm{
    fild dword ptr ds:[0x87b794]
    fsub cursorInset
    fdiv cursorScale
    ret
}}
static bool useHud(bool raw) {return correctHud&&!raw&&logicalCanvas()&&canvas().hudScale()<1;}

using Sprite=uintptr_t(__cdecl*)(float,float,float,float,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
using Gradient=uintptr_t(__cdecl*)(float,float,float,float,const void*,uint32_t,uint32_t);
using Rect=uintptr_t(__cdecl*)(float,float,float,float,uint32_t,uint32_t,uint32_t);
using Vertex=uintptr_t(__cdecl*)(float,float,uint32_t,uint32_t,uint32_t);
using Line=uintptr_t(__cdecl*)(const float*,const float*,uint32_t,uint32_t,uint32_t);
using Text=uintptr_t(__cdecl*)(float,float,const void*,uint32_t,uint32_t,uint32_t);
using TextBox=uintptr_t(__cdecl*)(float,float,float,float,const void*,uint32_t,uint32_t,uint32_t);
using Movie=uintptr_t(__cdecl*)(float,float,float,float,const void*);
using Projection=void*(__cdecl*)(void*,const float*);
using HudDraw=uintptr_t(__thiscall*)(void*,const float*,uint32_t,uint32_t,float);
using HudItem=uintptr_t(__thiscall*)(void*,uint32_t,uint32_t,float);
static HudDraw hudDraw;
static HudItem hudItem;
struct HudScope {HudScope(){++hudDepth;}~HudScope(){--hudDepth;}};
static uintptr_t __fastcall drawHud(void* self,void*,const float* position,uint32_t a,uint32_t b,float scale) {
    HudScope scope;return hudDraw(self,position,a,b,scale);
}
static uintptr_t __fastcall drawHudItem(void* self,void*,uint32_t a,uint32_t b,float scale) {
    HudScope scope;return hudItem(self,a,b,scale);
}
static Sprite sprite;
static Gradient gradient;
static Rect rect;
static Vertex vertex;
static Line line;
static Text text;
static TextBox textBox;

static uintptr_t __cdecl drawSprite(float x,float y,float right,float bottom,uint32_t a,uint32_t b,uint32_t c,uint32_t d,uint32_t e,uint32_t f,uint32_t raw) {
    if(useHud((raw&255)!=0)&&!(x<=0&&y<=0&&right>=640&&bottom>=480)){
        auto view=canvas();float edge=anchor(x,y,right,bottom);
        x=view.hudX(x,edge);right=view.hudX(right,edge);
    }
    return sprite(x,y,right,bottom,a,b,c,d,e,f,raw);
}
static uintptr_t __cdecl drawGradient(float x,float y,float w,float h,const void* colors,uint32_t options,uint32_t raw) {
    if(useHud((raw&255)!=0)&&!(x<=0&&y<=0&&w>=640&&h>=480)){
        auto view=canvas();x=view.hudX(x,anchor(x,y,x+w,y+h));w*=view.hudScale();
    }
    return gradient(x,y,w,h,colors,options,raw);
}
static uintptr_t __cdecl drawRect(float x,float y,float w,float h,uint32_t color,uint32_t options,uint32_t raw) {
    if(useHud((raw&255)!=0)&&!(x<=0&&w>=640)){
        auto view=canvas();x=view.hudX(x,anchor(x,y,x+w,y+h));w*=view.hudScale();
    }
    return rect(x,y,w,h,color,options,raw);
}
static void __cdecl skipBorder(float,float,float,float,uint32_t,uint32_t,uint32_t) {}
static uintptr_t __cdecl drawVertex(float x,float y,uint32_t a,uint32_t b,uint32_t c) {
    if(useHud(*reinterpret_cast<const BYTE*>(gameBase+0x41c484)!=0))x=canvas().hudX(x);
    return vertex(x,y,a,b,c);
}
static uintptr_t __cdecl drawLine(const float* first,const float* last,uint32_t a,uint32_t b,uint32_t raw) {
    if(!useHud((raw&255)!=0)||!first||!last)return line(first,last,a,b,raw);
    const auto view=canvas();
    const float start[]{view.hudX(first[0]),first[1]},end[]{view.hudX(last[0]),last[1]};
    return line(start,end,a,b,raw);
}
static uintptr_t __cdecl drawText(float x,float y,const void* value,uint32_t copy,uint32_t encoding,uint32_t logical) {
    if(useHud(!(logical&255)))x=canvas().hudX(x,anchor(x,y,x+1,y));
    return text(x,y,value,copy,encoding,logical);
}
static uintptr_t __cdecl drawTextBox(float x,float y,float w,float h,const void* value,uint32_t copy,uint32_t encoding,uint32_t logical) {
    if(useHud(!(logical&255))){
        auto view=canvas();x=view.hudX(x,anchor(x,y,x+w,y+h));w*=view.hudScale();
    }
    return textBox(x,y,w,h,value,copy,encoding,logical);
}
static void* __cdecl setView(void* camera,const float* native) {
    const auto original=reinterpret_cast<Projection>(gameBase+0x22bb30);
    if(!native||camera!=*reinterpret_cast<void* const*>(gameBase+0x420b90))return original(camera,native);
    const float factor=canvas().projectionScale();
    const float corrected[]{native[0]*factor,native[1]*factor};
    return original(camera,corrected);
}
static uintptr_t __cdecl drawMovie(float x,float y,float w,float h,const void* raster) {
    const auto original=reinterpret_cast<Movie>(gameBase+0x2b900);
    const auto view=canvas();
    if(!raster||!view.valid()||videoMode==Display::VideoMode::Native)return original(x,y,w,h,raster);
    const auto dimensions=static_cast<const int32_t*>(raster);
    const int width=dimensions[3],height=dimensions[4];
    if(width<1||height<1||width>16384||height>16384)return original(x,y,w,h,raster);
    const auto box=Display::videoBox(view,static_cast<float>(width),static_cast<float>(height),videoMode);
    rect(0,0,view.width,view.height,0xff000000,0,1);
    return original(box.x,box.y,box.width,box.height,raster);
}

static bool install() {
    static const BYTE quad[]{0x55,0x8b,0xec,0xa0,0x60,0xc4,0x81,0};
    static const BYTE point[]{0x55,0x8b,0xec,0x56,0x57};
    static const BYTE segment[]{0x55,0x8b,0xec,0x51,0xa1,0x78,0xc4,0x81,0};
    static const BYTE string[]{0x55,0x8b,0xec,0x83,0xec,0x0c};
    static const BYTE widget[]{0x55,0x8b,0xec,0x83,0xec,0x1c},item[]{0x55,0x8b,0xec,0xd9,0x45,0x10};
    static const BYTE mouse[]{0xdb,0x05,0x94,0xb7,0x87,0};
    static const BYTE projection[]{0xe8,0x37,0xdb,0x1f,0},projectionA[]{0xe8,0xe2,0xb1,0x1f,0},
        projectionB[]{0xe8,0xad,0xb1,0x1f,0},projectionC[]{0xe8,0x83,0xb1,0x1f,0};
    static const BYTE movie[]{0xe8,0x32,0x4a,0xfa,0xff};
    static const BYTE topBorder[]{0xe8,0x1d,0xbf,0xf8,0xff},bottomBorder[]{0xe8,0xfb,0xbe,0xf8,0xff};
    const Patch entry[]{
        {0x2b460,quad,sizeof(quad),drawSprite},{0x2b610,quad,sizeof(quad),drawGradient},
        {0x2b740,quad,sizeof(quad),drawRect},{0x2ba20,point,sizeof(point),drawVertex},
        {0x2bba0,segment,sizeof(segment),drawLine},{0x2c8d0,string,sizeof(string),drawText},
        {0x2ca50,string,sizeof(string),drawTextBox},
        {0x91b90,widget,sizeof(widget),drawHud},{0x91500,item,sizeof(item),drawHudItem},
    };
    constexpr size_t stride=16;
    Patch plan[19]{};
    size_t count=0;
    BYTE* trampolines=nullptr;
    rect=reinterpret_cast<Rect>(gameBase+0x2b740);
    if(correctHud) {
        trampolines=static_cast<BYTE*>(VirtualAlloc(nullptr,sizeof(entry)/sizeof(*entry)*stride,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
        if(!trampolines)return false;
        uintptr_t original[sizeof(entry)/sizeof(*entry)]{};
        for(size_t i=0;i<sizeof(entry)/sizeof(*entry);++i) {
            if(i>=7&&!edgeHud)continue;
            const auto& p=entry[i];BYTE* code=trampolines+i*stride;
            memcpy(code,p.expected,p.length);
            code[p.length]=0xe9;
            const uint32_t delta=static_cast<uint32_t>(gameBase+p.rva-reinterpret_cast<uintptr_t>(code)-5);
            memcpy(code+p.length+1,&delta,sizeof(delta));
            original[i]=reinterpret_cast<uintptr_t>(code);
            plan[count++]=p;
        }
        DWORD previous;
        if(!VirtualProtect(trampolines,sizeof(entry)/sizeof(*entry)*stride,PAGE_EXECUTE_READ,&previous)||
           !FlushInstructionCache(GetCurrentProcess(),trampolines,sizeof(entry)/sizeof(*entry)*stride)){
            VirtualFree(trampolines,0,MEM_RELEASE);return false;
        }
        sprite=reinterpret_cast<Sprite>(original[0]);gradient=reinterpret_cast<Gradient>(original[1]);
        rect=reinterpret_cast<Rect>(original[2]);vertex=reinterpret_cast<Vertex>(original[3]);
        line=reinterpret_cast<Line>(original[4]);text=reinterpret_cast<Text>(original[5]);textBox=reinterpret_cast<TextBox>(original[6]);
        hudDraw=reinterpret_cast<HudDraw>(original[7]);hudItem=reinterpret_cast<HudItem>(original[8]);
    }
    if(correctHud)for(uint32_t rva:{0xb961u,0x105a2u,0x12cedu})plan[count++]={rva,mouse,sizeof(mouse),menuCursorX,true};
    if(correctProjection) {
        plan[count++]={0x2dff4,projection,sizeof(projection),setView,true};
        plan[count++]={0x30949,projectionA,sizeof(projectionA),setView,true};
        plan[count++]={0x3097e,projectionB,sizeof(projectionB),setView,true};
        plan[count++]={0x309a8,projectionC,sizeof(projectionC),setView,true};
    }
    if(videoMode!=Display::VideoMode::Native)plan[count++]={0x86ec9,movie,sizeof(movie),drawMovie,true};
    if(hideBorders) {
        plan[count++]={0x9f81e,topBorder,sizeof(topBorder),skipBorder,true};
        plan[count++]={0x9f840,bottomBorder,sizeof(bottomBorder),skipBorder,true};
    }
    if(!applyPatches(plan,count)){
        if(trampolines)VirtualFree(trampolines,0,MEM_RELEASE);
        return false;
    }
    return true;
}
}
