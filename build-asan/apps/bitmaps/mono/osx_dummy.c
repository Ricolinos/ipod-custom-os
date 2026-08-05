#include "lcd.h"
#include "bitmaps/osx_dummy.h"
const unsigned char osx_dummy[] = {
0x00, 

};

const struct bitmap bm_osx_dummy = { 
    .width = BMPWIDTH_osx_dummy, 
    .height = BMPHEIGHT_osx_dummy, 
    .data = (unsigned char*)osx_dummy,
};
