#include "lcd.h"
#include "pluginbitmaps/brickmania_ball.h"
const unsigned short brickmania_ball[] = {
0x0000, 0xef5d, 0xce79, 0xce79, 0x0000, 
0xdedb, 0xe73c, 0xef5d, 0xa534, 0xb5b6, 
0xa534, 0xce59, 0xce59, 0xad55, 0x73ae, 
0xb5b6, 0x8c71, 0x9cf3, 0x6b6d, 0x9cd3, 
0x0000, 0xa534, 0x632c, 0x9cd3, 0x0000, 

};

const struct bitmap bm_brickmania_ball = { 
    .width = BMPWIDTH_brickmania_ball, 
    .height = BMPHEIGHT_brickmania_ball, 
    .format = FORMAT_NATIVE, 
    .data = (unsigned char*)brickmania_ball,
};
