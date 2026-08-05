#include "lcd.h"
#include "pluginbitmaps/osx.h"
const unsigned char osx[] = {
0x00, 

};

const struct bitmap bm_osx = { 
    .width = BMPWIDTH_osx, 
    .height = BMPHEIGHT_osx, 
    .data = (unsigned char*)osx,
};
