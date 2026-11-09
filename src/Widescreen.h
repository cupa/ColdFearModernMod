#pragma once
#include <algorithm>
#include <cmath>

namespace Display {
struct Canvas {
    float width,height;
    bool valid() const {return width>=640 && height>=480 && width<=16384 && height<=16384;}
    float aspect() const {return width/height;}
    float hudScale() const {return valid()?std::min(1.0f,(4.0f/3.0f)/aspect()):1.0f;}
    float projectionScale() const {return valid()?std::max(1.0f,aspect()/(4.0f/3.0f)):1.0f;}
    float hudX(float x,float edge=0) const {
        const float margin=320*(1-hudScale());
        return x*hudScale()+margin*(1+edge);
    }
};

struct Box {float x,y,width,height;};
enum class VideoMode {Native,Fit,Fill};
inline Box videoBox(Canvas screen,float width,float height,VideoMode mode) {
    if(!screen.valid()||!(width>0&&height>0)||mode==VideoMode::Native)return {0,0,width,height};
    float sx=screen.width/width,sy=screen.height/height;
    float scale=mode==VideoMode::Fill?std::max(sx,sy):std::min(sx,sy);
    width*=scale;height*=scale;
    return {(screen.width-width)*0.5f,(screen.height-height)*0.5f,width,height};
}

inline int hudEdge(float left,float top,float right,float bottom) {
    if(top<310||bottom>468||right<=left)return 0;
    if(left>=0&&right<=192)return -1;
    if(left>=416&&right<=640)return 1;
    return 0;
}
}
