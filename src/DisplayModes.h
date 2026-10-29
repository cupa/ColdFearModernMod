#pragma once
#include <cstdint>
#include <cstddef>
namespace Display {
struct Mode {uint32_t width,height;};
struct Modes {
    static constexpr uint32_t deckSlot=256;
    Mode values[16]{{640,480},{800,600},{1024,768},{1280,1024},{1600,1200},{1920,1080},{2560,1440},{3840,2160},{1280,800}};
    size_t count=9;
    bool add(uint32_t width,uint32_t height) {
        if(width<640||height<480||width>16384||height>16384)return false;
        for(size_t i=0;i<count;++i)if(values[i].width==width&&values[i].height==height)return true;
        if(count==16)return false;
        values[count++]={width,height};return true;
    }
    uint32_t selector(size_t i)const {
        // Duplicate widths need distinct menu keys.
        for(size_t j=0;j<i;++j)if(values[j].width==values[i].width)return 0x80000000u|static_cast<uint32_t>(i);
        return values[i].width;
    }
    int index(uint32_t key) const {
        for(size_t i=0;i<count;++i)if(selector(i)==key)return static_cast<int>(i);
        return -1;
    }
    uint32_t selection(uint32_t width,uint32_t height)const {
        for(size_t i=0;i<count;++i)if(values[i].width==width&&values[i].height==height)return selector(i);
        return width;
    }
    uint32_t savedIndex(size_t i)const {
        // Preserve legacy desktop slots; reserve 256 for Steam Deck.
        return i==8?deckSlot:static_cast<uint32_t>(i>8?i-1:i);
    }
    int loadedIndex(uint32_t slot)const {
        if(slot==deckSlot)return 8;
        if(slot>=15)return -1;
        return static_cast<int>(slot>=8?slot+1:slot);
    }
    Mode find(uint32_t key)const {int i=index(key);return values[i<0?2:i];}
    uint32_t cycle(uint32_t key,int action)const {
        int i=index(key);if(i<0)i=2;
        if(action==230)i=(i+1)%static_cast<int>(count);
        if(action==210)i=(i+static_cast<int>(count)-1)%static_cast<int>(count);
        return selector(i);
    }
};
}
