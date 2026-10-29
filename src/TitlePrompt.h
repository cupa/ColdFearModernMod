#pragma once
#include <cstdint>
struct TitleGlyph {uint16_t glyph,pad,character,pad2;};
// Strings contain native font glyph IDs.
static inline bool encodeTitlePrompt(const TitleGlyph* map,unsigned count,wchar_t (&output)[29]) {
    const wchar_t text[]=L"Press any button to START";
    for(unsigned i=0;i<sizeof(text)/sizeof(*text);++i) {
        if(!text[i]){output[i]=0;return true;}
        bool found=false;
        for(unsigned j=1;j<count;++j)if(map[j].character==text[i]) {
            output[i]=map[j].glyph;found=true;break;
        }
        if(!found)return false;
    }
    return false;
}
// Lookup supplies measurement and drawing.
static const wchar_t* titlePrompt(const wchar_t* original,int id,int titleId,bool controller,
                                 const wchar_t* encoded=L"Press any button to START") {
    return controller&&titleId>=0&&id==titleId?encoded:original;
}


static inline float centeredTitleX(float canvasWidth,float textWidth){return (canvasWidth-textWidth)*0.5f;}
