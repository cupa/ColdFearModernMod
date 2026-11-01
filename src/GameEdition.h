#pragma once
#include <cstring>
#include <cstdint>
enum class Edition { Unknown, Steam, Gog };
inline Edition editionFromHash(const char* hash) {
    if(!strcmp(hash,"b512f1d47fb85a41c784c71d987044f204cfec900fa9a61f28778dd05e351131"))return Edition::Steam;
    if(!strcmp(hash,"1966fa18ecd6f08644cbedb1d4632373799fb03ed138a3d998e42ad713436cf1"))return Edition::Gog;
    return Edition::Unknown;
}
inline Edition editionFromImageSize(uint32_t size) {
    return size==0x4d6000?Edition::Steam:size==0x4d4000?Edition::Gog:Edition::Unknown;
}
